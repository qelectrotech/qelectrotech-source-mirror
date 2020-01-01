/*
	Copyright 2006-2019 The QElectroTech Team
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "elementscene.h"
#include "qetelementeditor.h"
#include "elementprimitivedecorator.h"
#include <cmath>
#include "partline.h"
#include "partrectangle.h"
#include "partellipse.h"
#include "partpolygon.h"
#include "partterminal.h"
#include "parttext.h"
#include "partarc.h"
#include "editorcommands.h"
#include "elementcontent.h"
#include "ui/elementpropertieseditorwidget.h"
#include "eseventinterface.h"
#include "QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "partdynamictextfield.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "namelistdialog.h"
#include "namelistwidget.h"

#include <algorithm>
#include <QKeyEvent>

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent le Widget parent
*/
ElementScene::ElementScene(QETElementEditor *editor, QObject *parent) :
	QGraphicsScene(parent),
	m_elmt_type("simple"),
	m_qgi_manager(this),
	m_element_editor(editor)
{
	setItemIndexMethod(QGraphicsScene::NoIndex);
		//Set to no index, because they can be the source of the crash with conductor and shape ghost.
		//https://forum.qt.io/topic/71316/qgraphicsscenefinditembsptreevisitor-visit-crashes-due-to-an-obsolete-paintevent-after-qgraphicsscene-removeitem
		//https://stackoverflow.com/questions/38458830/crash-after-qgraphicssceneremoveitem-with-custom-item-class
		//http://www.qtcentre.org/archive/index.php/t-33730.html
		//http://tech-artists.org/t/qt-properly-removing-qgraphicitems/3063
	
	m_behavior = Normal;
	setItemIndexMethod(NoIndex);
	setGrid(1, 1);
	initPasteArea();
	m_undo_stack.setClean();
	m_decorator_lock = new QMutex(QMutex::NonRecursive);
	connect(&m_undo_stack, SIGNAL(indexChanged(int)), this, SLOT(managePrimitivesGroups()));
	connect(this, SIGNAL(selectionChanged()), this, SLOT(managePrimitivesGroups()));
}

/**
 * @brief ElementScene::~ElementScene
 */
ElementScene::~ElementScene()
{
		//Disconnect to avoid crash, see bug report N° 122.
	disconnect(&m_undo_stack, SIGNAL(indexChanged(int)), this, SLOT(managePrimitivesGroups()));
	delete m_decorator_lock;

	if (m_event_interface)
		delete m_event_interface;

	if (m_decorator)
		delete m_decorator;
}

/**
 * @brief ElementScene::mouseMoveEvent
 * @param e
 */
void ElementScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	if (m_event_interface) {
		if (m_event_interface -> mouseMoveEvent(e)) {
			if (m_event_interface->isFinish()) {
				delete m_event_interface; m_event_interface = nullptr;
				emit(partsAdded());
			}
			return;
		}
	}

	QPointF event_pos = e -> scenePos();
	if (!(e -> modifiers() & Qt::ControlModifier))
		event_pos = snapToGrid(event_pos);
	
	if (m_behavior == PasteArea) {
		QRectF current_rect(m_paste_area -> rect());
		current_rect.moveCenter(event_pos);
		m_paste_area -> setRect(current_rect);
		return;
	}

	QGraphicsScene::mouseMoveEvent(e);
	
}

/**
 * @brief ElementScene::mousePressEvent
 * @param e
 */
void ElementScene::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	if (m_event_interface) {
		if (m_event_interface -> mousePressEvent(e)) {
			if (m_event_interface->isFinish()) {
				delete m_event_interface; m_event_interface = nullptr;
				emit(partsAdded());
			}
			return;
		}
	}

	QGraphicsScene::mousePressEvent(e);
}

/**
 * @brief ElementScene::mouseReleaseEvent
 * @param e
 */
void ElementScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	if (m_event_interface) {
		if (m_event_interface -> mouseReleaseEvent(e)) {
			if (m_event_interface->isFinish()) {
				delete m_event_interface; m_event_interface = nullptr;
				emit(partsAdded());
			}
			return;
		}
	}
	
	if (m_behavior == PasteArea) {
		m_defined_paste_area = m_paste_area -> rect();
		removeItem(m_paste_area);
		emit(pasteAreaDefined(m_defined_paste_area));
		m_behavior = Normal;
		return;
	}

	QGraphicsScene::mouseReleaseEvent(e);
}

/**
 * @brief ElementScene::mouseDoubleClickEvent
 * @param event
 */
void ElementScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	if (m_event_interface) {
		if (m_event_interface -> mouseDoubleClickEvent(event)) {
			if (m_event_interface->isFinish()) {
				delete m_event_interface; m_event_interface = nullptr;
				emit(partsAdded());
			}
			return;
		}
	}

	QGraphicsScene::mouseDoubleClickEvent(event);
}

/**
 * @brief ElementScene::keyPressEvent
 * manage key press event
 * @param event
 */
void ElementScene::keyPressEvent(QKeyEvent *event)
{
	if (m_event_interface)
	{
		if (m_event_interface -> keyPressEvent(event))
		{
			if (m_event_interface->isFinish())
			{
				delete m_event_interface; m_event_interface = nullptr;
				emit(partsAdded());
			}
			return;
		}
	}
	
	if(selectedItems().size() == 1)
	{
		if(selectedItems().first()->type() == PartText::Type)
		{
			PartText *t = static_cast<PartText *>(selectedItems().first());
			if(t->textInteractionFlags() & Qt::TextEditorInteraction)
			{
				QGraphicsScene::keyPressEvent(event);
				return;
			}
		}
		
		QGraphicsObject *qgo = selectedItems().first()->toGraphicsObject();
		if(qgo)
		{
		QPointF original_pos = qgo->pos();
		QPointF p = qgo->pos();
		
		if (event->modifiers() & Qt::ControlModifier) {
			
			int k = event->key();
			if(k == Qt::Key_Right)
				p.rx() += 0.1;
			else if (k == Qt::Key_Left)
				p.rx() -= 0.1;
			else if (k == Qt::Key_Up)
				p.ry() -= 0.1;
			else if (k == Qt::Key_Down)
				p.ry() += 0.1;
				}
			else {
			
			int k = event->key();
			if(k == Qt::Key_Right)
				p.rx() += 1;
			else if (k == Qt::Key_Left)
				p.rx() -= 1;
			else if (k == Qt::Key_Up)
				p.ry() -= 1;
			else if (k == Qt::Key_Down)
				p.ry() += 1;
			}
			
			qgo->setPos(p);
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(qgo, "pos", QVariant(original_pos), QVariant(p));
			undo->setText(tr("Déplacer une primitive"));
			undo->enableAnimation();
			undoStack().push(undo);
			event->accept();
			return;
		}
	}
	
	QGraphicsScene::keyPressEvent(event);
}

/**
 * @brief ElementScene::contextMenuEvent
 * Display the context menu event, only if behavior are Normal
 * @param event
 */
void ElementScene::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	QGraphicsScene::contextMenuEvent(event);
	if(event->isAccepted())
		return;
	
	if (m_behavior == ElementScene::Normal)
		m_element_editor -> contextMenu(event->screenPos());
}

/**
	Dessine l'arriere-plan de l'editeur, cad l'indicateur de hotspot.
	@param p Le QPainter a utiliser pour dessiner
	@param rect Le rectangle de la zone a dessiner
*/
void ElementScene::drawForeground(QPainter *p, const QRectF &rect) {
	Q_UNUSED(rect);
	p -> save();
	
	// desactive tout antialiasing, sauf pour le texte
	p -> setRenderHint(QPainter::Antialiasing, false);
	p -> setRenderHint(QPainter::TextAntialiasing, true);
	p -> setRenderHint(QPainter::SmoothPixmapTransform, false);
	
	QPen pen(Qt::red);
	pen.setCosmetic(true);
	p -> setPen(pen);
	p -> setBrush(Qt::NoBrush);
	p -> drawLine(-20, 0, 20, 0);
	p -> drawLine(0, -20, 0, 20);
	p -> restore();
}

/**
 * @brief ElementScene::setEventInterface
 * Set a new event interface
 * @param interface
 */
void ElementScene::setEventInterface(ESEventInterface *event_interface)
{
	if (m_event_interface)
	{
		delete m_event_interface;
		//We must to re-init because previous interface
		//Reset his own init when deleted
		event_interface->init();
	}
	m_event_interface = event_interface;
}

/**
 * @brief ElementScene::clearEventInterface
 * Clear the current event interface
 */
void ElementScene::clearEventInterface()
{
	if(m_event_interface)
	{
		delete m_event_interface;
		m_event_interface = nullptr;
	}
}

/**
 * @brief ElementScene::setBehavior
 * Modifie the current behavior of this scene
 * @param b
 */
void ElementScene::setBehavior(ElementScene::Behavior b) {
	m_behavior = b;
}

ElementScene::Behavior ElementScene::behavior() const {
	return m_behavior;
}

/**
	@return la taille horizontale de la grille
*/
int ElementScene::xGrid() const {
	return(m_x_grid);
}

/**
	@return la taille verticale de la grille
*/
int ElementScene::yGrid() const {
	return(m_y_grid);
}

/**
	@param x_g Taille horizontale de la grille
	@param y_g Taille verticale de la grille
*/
void ElementScene::setGrid(int x_g, int y_g) {
	m_x_grid = x_g ? x_g : 1;
	m_y_grid = y_g ? y_g : 1;
}

/**
 * @brief ElementScene::toXml
 * Export this element as a xml file
 * @param all_parts (true by default) if true, export the entire element in xml,
 * if false, only export the selected parts.
 * @return an xml document that describe the element.
 */
const QDomDocument ElementScene::toXml(bool all_parts)
{
	QRectF size= elementSceneGeometricRect();

		//if the element doesn't contains the origin point of the scene
		//we move the element to the origin for solve this default before saving
	if (!size.contains(0,0) && all_parts)
	{
		centerElementToOrigine();
			//recalcul the size after movement
		size= elementSceneGeometricRect();
	}

		//define the size of the element by the upper multiple of 10
	int upwidth = ((qRound(size.width())/10)*10)+10;
	if ((qRound(size.width())%10) > 6) upwidth+=10;

	int upheight = ((qRound(size.height())/10)*10)+10;
	if ((qRound(size.height())%10) > 6) upheight+=10;

		//the margin between the real size of the element and the rectangle that delimits
	int xmargin = qRound(upwidth - size.width());
	int ymargin = qRound(upheight - size.height());

		// document XML
	QDomDocument xml_document;

		//Root of xml document
	QDomElement root = xml_document.createElement("definition");
	root.setAttribute("type",        "element");
	root.setAttribute("width",       QString("%1").arg(upwidth));
	root.setAttribute("height",      QString("%1").arg(upheight));
	root.setAttribute("hotspot_x",   QString("%1").arg(-(qRound(size.x() - (xmargin/2)))));
	root.setAttribute("hotspot_y",   QString("%1").arg(-(qRound(size.y() - (ymargin/2)))));
	root.setAttribute("orientation", "dyyy"); //we keep the orientation for compatibility with previous version of qet
	root.setAttribute("version", QET::version);
	root.setAttribute("link_type", m_elmt_type);

		//Uuid used to compare two elements
	QDomElement uuid = xml_document.createElement("uuid");
	uuid.setAttribute("uuid", QUuid::createUuid().toString());
	root.appendChild(uuid);
	
		//names of element
	root.appendChild(m_names_list.toXml(xml_document));

	if (m_elmt_type == "slave" || m_elmt_type == "master")
	{
		QDomElement kindInfo = xml_document.createElement("kindInformations");
		m_elmt_kindInfo.toXml(kindInfo, "kindInformation");
		root.appendChild(kindInfo);
	}
	
	if(m_elmt_type == "simple" || m_elmt_type == "master" || m_elmt_type == "terminal")
	{
		QDomElement element_info = xml_document.createElement("elementInformations");
		m_elmt_information.toXml(element_info, "elementInformation");
		root.appendChild(element_info);
	}
	
		//complementary information about the element
	QDomElement informations_element = xml_document.createElement("informations");
	root.appendChild(informations_element);
	informations_element.appendChild(xml_document.createTextNode(informations()));
	
	QDomElement description = xml_document.createElement("description");

		//the graphic description of the element
	foreach(QGraphicsItem *qgi, zItems())
	{
			//If the export concerns only the selection, the not selected part is ignored
		if (!all_parts && !qgi -> isSelected()) continue;
		if (CustomElementPart *ce = dynamic_cast<CustomElementPart *>(qgi))
		{
			if (ce -> isUseless()) continue;
			description.appendChild(ce -> toXml(xml_document));
		}
	}
	root.appendChild(description);
	
	xml_document.appendChild(root);
	return(xml_document);
}

/**
	@param xml_document un document XML decrivant un element
	@return le boundingRect du contenu de l'element
*/
QRectF ElementScene::boundingRectFromXml(const QDomDocument &xml_document) {
	// charge les parties depuis le document XML
	ElementContent loaded_content = loadContent(xml_document);
	if (loaded_content.isEmpty()) return(QRectF());
	
	// calcule le boundingRect
	QRectF bounding_rect = elementContentBoundingRect(loaded_content);
	
	// detruit les parties chargees
	qDeleteAll(loaded_content);
	
	return(bounding_rect);
}

/**
	Importe l'element decrit dans un document XML. Si une position est
	precisee, les elements importes sont positionnes de maniere a ce que le
	coin superieur gauche du plus petit rectangle pouvant les entourant tous
	(le bounding rect) soit a cette position.
	@param xml_document un document XML decrivant l'element
	@param position La position des parties importees
	@param consider_informations Si vrai, les informations complementaires
	(dimensions, hotspot, etc.) seront prises en compte
	@param content_ptr si ce pointeur vers un ElementContent est different de 0,
	il sera rempli avec le contenu ajoute a l'element par le fromXml
	@return true si l'import a reussi, false sinon
*/
void ElementScene::fromXml(const QDomDocument &xml_document, const QPointF &position, bool consider_informations, ElementContent *content_ptr)
{
	bool state = true;
	
		//Consider the informations of the element
	if (consider_informations) {
		state = applyInformations(xml_document);
	}
	
	if (state)
	{
		ElementContent loaded_content = loadContent(xml_document);
		if (position != QPointF())
			addContentAtPos(loaded_content, position);
		else
			addContent(loaded_content);
		
		if (content_ptr)
			*content_ptr = loaded_content;
	}
}

/**
	@return the minimum, margin-less rectangle the element can fit into, in scene
	coordinates. It is different from itemsBoundingRect() because it is not supposed
	to imply any margin.
*/
QRectF ElementScene::elementSceneGeometricRect() const{
	QRectF esgr;
	foreach (QGraphicsItem *qgi, items()) {
		if (qgi->type() == ElementPrimitiveDecorator::Type) continue;
		if (qgi->type() == QGraphicsRectItem::Type) continue;
		if (qgi->type() == PartDynamicTextField::Type) continue;
		if (CustomElementPart *cep = dynamic_cast <CustomElementPart*> (qgi)) {
			esgr |= cep -> sceneGeometricRect();
		}
	}
	return (esgr);
}

/**
	@return true si l'element comporte au moins une borne, false s'il n'en a
	aucune.
*/
bool ElementScene::containsTerminals() const {
	foreach(QGraphicsItem *qgi,items()) {
		if (qgraphicsitem_cast<PartTerminal *>(qgi)) {
			return(true);
		}
	}
	return(false);
}

/**
	@return la pile d'annulations de cet editeur d'element
*/
QUndoStack &ElementScene::undoStack() {
	return(m_undo_stack);
}

/**
	@return le gestionnaire de QGraphicsItem de cet editeur d'element
*/
QGIManager &ElementScene::qgiManager() {
	return(m_qgi_manager);
}

/**
	@return true si le presse-papier semble contenir un element
*/
bool ElementScene::clipboardMayContainElement() {
	QString clipboard_text = QApplication::clipboard() -> text().trimmed();
	bool may_be_element = clipboard_text.startsWith("<definition") && clipboard_text.endsWith("</definition>");
	return(may_be_element);
}

/**
	@param clipboard_content chaine de caractere, provenant vraisemblablement du
	presse-papier.
	@return true si clipboard_content a ete copie depuis cet element.
*/
bool ElementScene::wasCopiedFromThisElement(const QString &clipboard_content) {
	return(clipboard_content == m_last_copied);
}

/**
	Gere le fait de couper la selection = l'exporter en XML dans le
	presse-papier puis la supprimer.
*/
void ElementScene::cut() {
	copy();
	QList<QGraphicsItem *> cut_content = selectedItems();
	clearSelection();
	undoStack().push(new CutPartsCommand(this, cut_content));
}

/**
	Gere le fait de copier la selection = l'exporter en XML dans le
	presse-papier.
*/
void ElementScene::copy() {
	// accede au presse-papier
	QClipboard *clipboard = QApplication::clipboard();
	
	// genere la description XML de la selection
	QString clipboard_content = toXml(false).toString(4);
	
	// met la description XML dans le presse-papier
	if (clipboard -> supportsSelection()) {
		clipboard -> setText(clipboard_content, QClipboard::Selection);
	}
	clipboard -> setText(clipboard_content);
	
	// retient le dernier contenu copie
	m_last_copied = clipboard_content;
}

QETElementEditor* ElementScene::editor() const {
	return m_element_editor;
}

void ElementScene::setElementInfo(const DiagramContext& dc)
{
	if(m_elmt_information != dc)
	{
		m_elmt_information = dc;
		emit elementInfoChanged();
	}
}

/**
 * @brief ElementScene::slot_select
 * Select the item in content, every others items in the scene are deselected
 * @param content
 */
void ElementScene::slot_select(const ElementContent &content)
{
	blockSignals(true);
	
	//Befor clear selection, we must to remove the handlers items in @content,
	//because if in @content there are a selected item, but also its handlers items,
	//When item is deselected, the item delete its handlers items,
	//then handlers in content doesn't exist anymore and cause segfault
	QList<QGraphicsItem*> items_list;
	for (QGraphicsItem *qgi : content)
	{
		if(qgi->type() != QetGraphicsHandlerItem::Type)
			items_list << qgi;
	}
	clearSelection();

	foreach(QGraphicsItem *qgi, items_list)
		qgi -> setSelected(true);
	
	blockSignals(false);
	emit(selectionChanged());
}

/**
 * @brief ElementScene::slot_selectAll
 * Select all items
 */
void ElementScene::slot_selectAll() {
	slot_select(items());
}

/**
 * @brief ElementScene::slot_deselectAll
 * deselect all item
 */
void ElementScene::slot_deselectAll() {
	slot_select(ElementContent());
}

/**
	Inverse la selection
*/
void ElementScene::slot_invertSelection() {
	blockSignals(true);
	foreach(QGraphicsItem *qgi, items()) qgi -> setSelected(!qgi -> isSelected());
	blockSignals(false);
	emit(selectionChanged());
}

/**
	Supprime les elements selectionnes
*/
void ElementScene::slot_delete() {
	// verifie qu'il y a qqc de selectionne
	QList<QGraphicsItem *> selected_items = selectedItems();
	if (selected_items.isEmpty()) return;
	
	// efface tout ce qui est selectionne
	m_undo_stack.push(new DeletePartsCommand(this, selected_items));
	
	// removing items does not trigger QGraphicsScene::selectionChanged()
	emit(partsRemoved());
	emit(selectionChanged());
}

/**
	Lance un dialogue pour editer les informations complementaires de cet
	element. Concretement, ce champ libre est destine a accueillir des informations
	sur l'auteur de l'element, sa licence, etc.
*/
void ElementScene::slot_editAuthorInformations() {
	bool is_read_only = m_element_editor && m_element_editor -> isReadOnly();
	
	// cree un dialogue
	QDialog dialog_author(m_element_editor);
	dialog_author.setModal(true);
#ifdef Q_OS_MACOS
	dialog_author.setWindowFlags(Qt::Sheet);
#endif
	dialog_author.setMinimumSize(400, 260);
	dialog_author.setWindowTitle(tr("Éditer les informations sur l'auteur", "window title"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&dialog_author);
	
	// ajoute un champ explicatif au dialogue
	QLabel *information_label = new QLabel(tr("Vous pouvez utiliser ce champ libre pour mentionner les auteurs de l'élément, sa licence, ou tout autre renseignement que vous jugerez utile."));
	information_label -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	information_label -> setWordWrap(true);
	dialog_layout -> addWidget(information_label);
	
	// ajoute un QTextEdit au dialogue
	QTextEdit *text_field = new QTextEdit();
	text_field -> setAcceptRichText(false);
	text_field -> setPlainText(informations());
	text_field -> setReadOnly(is_read_only);
	dialog_layout -> addWidget(text_field);
	
	// ajoute deux boutons au dialogue
	QDialogButtonBox *dialog_buttons = new QDialogButtonBox(is_read_only ? QDialogButtonBox::Ok : QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout -> addWidget(dialog_buttons);
	connect(dialog_buttons, SIGNAL(accepted()),    &dialog_author, SLOT(accept()));
	connect(dialog_buttons, SIGNAL(rejected()),    &dialog_author, SLOT(reject()));
	
	// lance le dialogue
	if (dialog_author.exec() == QDialog::Accepted && !is_read_only) {
		QString new_infos = text_field -> toPlainText().remove(QChar(13)); // CR-less text
		if (new_infos != informations()) {
			undoStack().push(new ChangeInformationsCommand(this, informations(), new_infos));
		}
	}
}

/**
 * @brief ElementScene::slot_editProperties
 * Open dialog to edit the element properties
 */
void  ElementScene::slot_editProperties()
{
	QString type = m_elmt_type;
	DiagramContext kind_info = m_elmt_kindInfo;
	DiagramContext elmt_info = m_elmt_information;
	
	ElementPropertiesEditorWidget epew(type, kind_info, elmt_info);
	epew.exec();
	
	if (type != m_elmt_type ||
		kind_info != m_elmt_kindInfo ||
		elmt_info != m_elmt_information)
		undoStack().push(new ChangePropertiesCommand(this, type, kind_info, elmt_info));
}

/**
 * @brief ElementScene::slot_editNames
 * Launch a dialog for edit the names of the edited element
 */
void ElementScene::slot_editNames()
{
	bool is_read_only = m_element_editor && m_element_editor -> isReadOnly();
	
	NameListDialog dialog_(m_element_editor);
	
	dialog_.setModal(true);
	dialog_.setMinimumSize(400, 330);
	dialog_.setWindowTitle(tr("Éditer les noms", "window title"));
	
	dialog_.setInformationText(tr("Vous pouvez spécifier le nom de l'élément dans plusieurs langues."));
	
	NameListWidget *nlw_ = dialog_.namelistWidget();
	nlw_->setNames(m_names_list);
	nlw_->setReadOnly(is_read_only);
	
	if (dialog_.exec() == QDialog::Accepted && !is_read_only && !nlw_->isEmpty())
	{
		NamesList new_names = nlw_->names();
		if (new_names != m_names_list) {
			undoStack().push(new ChangeNamesCommand(this, m_names_list, new_names));
		}
	}
}

/**
	@return the list of primitives currently present on the scene.
*/
QList<CustomElementPart *> ElementScene::primitives() const {
	QList<CustomElementPart *> primitives_list;
	foreach (QGraphicsItem *item, items()) {
		if (CustomElementPart *primitive = dynamic_cast<CustomElementPart *>(item)) {
			primitives_list << primitive;
		}
	}
	return(primitives_list);
}

/**
	@param include_terminals true pour inclure les bornes, false sinon
	@return les parties de l'element ordonnes par zValue croissante
*/
QList<QGraphicsItem *> ElementScene::zItems(ItemOptions options) const {
	// handle dummy request, i.e. when neither Selected nor NonSelected are set
	if (!(options & ElementScene::Selected) && !(options & ElementScene::NonSelected)) {
		return(QList<QGraphicsItem *>());
	}
	
	// retrieve all items
	QList<QGraphicsItem *> all_items_list(items());
	QMutableListIterator<QGraphicsItem *> i(all_items_list);
	
	// remove unrequired items
	if ((options & ElementScene::SelectedOrNot) != ElementScene::SelectedOrNot) {
		bool keep_selected = options & ElementScene::Selected;
		while (i.hasNext()) {
			if (i.next() -> isSelected() != keep_selected) {
				i.remove();
			}
		}
	}
	
	QList<QGraphicsItem *> terminals;
	QList<QGraphicsItem *> helpers;
	for (i.toFront(); i.hasNext(); ) {
		i.next();
		QGraphicsItem *qgi = i.value();
		if (
			qgi -> type() == ElementPrimitiveDecorator::Type ||
			qgi -> type() == QGraphicsRectItem::Type ||
			qgi->type() == QetGraphicsHandlerItem::Type
		) {
			i.remove();
			helpers << qgi;
		}
		else if (qgraphicsitem_cast<PartTerminal *>(qgi)) {
			i.remove();
			terminals << qgi;
		}
	}
	
	// ordonne les parties par leur zValue
	if (options & SortByZValue) {
		std::sort (all_items_list.begin(), all_items_list.end(), ElementScene::zValueLessThan);
	}
	
	// rajoute eventuellement les bornes
	if (options & ElementScene::IncludeTerminals) {
		all_items_list += terminals;
	}
	if (options & ElementScene::IncludeHelperItems) {
		all_items_list += helpers;
	}
	return(all_items_list);
}

/**
	@return les parties graphiques selectionnees
*/
ElementContent ElementScene::selectedContent() const {
	ElementContent content;
	foreach(QGraphicsItem *qgi, zItems()) {
		if (qgi -> isSelected()) content << qgi;
	}
	return(content);
}

/**
	@param to_paste Rectangle englobant les parties a coller
	@return le rectangle ou il faudra coller ces parties
*/
void ElementScene::getPasteArea(const QRectF &to_paste) {
	// on le dessine sur la scene
	m_paste_area -> setRect(to_paste);
	addItem(m_paste_area);
	
	// on passe la scene en mode "recherche de zone pour copier/coller"
	m_behavior = PasteArea;
}

/**
 * @brief ElementScene::reset
 * Remove all QGraphicsItems in the scene and clear the undo stack.
 */
void ElementScene::reset()
{
	clearSelection();
	undoStack().clear();

		//We don't add handlers, because it's the role of the primitive or decorator to remove it.
	QList<QGraphicsItem*> items_list;
	for (QGraphicsItem *qgi : items())
	{
		if(qgi->type() != QetGraphicsHandlerItem::Type)
			items_list << qgi;
	}
	
	for (QGraphicsItem *qgi : items_list)
	{
		removeItem(qgi);
		qgiManager().release(qgi);
	}

	delete m_decorator;
	m_decorator = nullptr;
}

/**
	@param content Contenu ( = parties) d'un element
	@return le boundingRect de ces parties, exprime dans les coordonnes de la
	scene
*/
QRectF ElementScene::elementContentBoundingRect(const ElementContent &content) const {
	QRectF bounding_rect;
	foreach(QGraphicsItem *qgi, content) {
		// skip non-primitives QGraphicsItems (paste area, selection decorator)
		if (qgi -> type() == ElementPrimitiveDecorator::Type) continue;
		if (qgi -> type() == QGraphicsRectItem::Type) continue;
		bounding_rect |= qgi -> sceneBoundingRect();
	}
	return(bounding_rect);
}

/**
	Applique les informations (dimensions, hostpot, orientations, connexions
	internes, noms et informations complementaires) contenu dans un document XML.
	@param xml_document Document XML a analyser
	@return true si la lecture et l'application des informations s'est bien
	passee, false sinon.
*/
bool ElementScene::applyInformations(const QDomDocument &xml_document)
{
		// Root must be an element definition
	QDomElement root = xml_document.documentElement();
	
	if (root.tagName() != "definition" || root.attribute("type") != "element")
		return(false);

		//Extract info about element type
	m_elmt_type = root.attribute("link_type", "simple");
	m_elmt_kindInfo.fromXml(root.firstChildElement("kindInformations"), "kindInformation");
		//Extract info of element
	m_elmt_information.fromXml(root.firstChildElement("elementInformations"), "elementInformation");

	//Extract names of xml definition
	m_names_list.fromXml(root);
	
	//extract additional informations
	setInformations(QString());
	for (QDomNode node = root.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		QDomElement elmt = node.toElement();
		if (elmt.isNull()) continue;
		if (elmt.tagName() == "informations") {
			setInformations(elmt.text());
			break;
		}
	}
	
	return(true);
}

/**
	Par le document XML xml_document et retourne le contenu ( = liste de
	parties) correspondant.
	@param xml_document Document XML a analyser
	@param error_message pointeur vers une QString ; si error_message est
	different de 0, un message d'erreur sera stocke dedans si necessaire
*/
/**
 * @brief ElementScene::loadContent
 * @param xml_document : xml dom document to analyze
 * @return 
 */
/**
 * @brief ElementScene::loadContent
 * Create and load the content describe in the xml document.
 * @param xml_document
 * @return the loaded content
 */
ElementContent ElementScene::loadContent(const QDomDocument &xml_document)
{
	ElementContent loaded_parts;
	
		//The root is supposed to be an element definition
	QDomElement root = xml_document.documentElement();
	
	if (root.tagName() != "definition" || root.attribute("type") != "element")
		return(loaded_parts);
	
		//Load the graphic description of the element
	for (QDomNode node = root.firstChild() ; !node.isNull() ; node = node.nextSibling())
	{
		QDomElement elmts = node.toElement();
		if (elmts.isNull())
			continue;
		
		if (elmts.tagName() == "description")
		{
			int z = 1;
			for (QDomNode n = node.firstChild() ; !n.isNull() ; n = n.nextSibling())
			{
				QDomElement qde = n.toElement();
				if (qde.isNull())
					continue;
				CustomElementPart *cep = nullptr;
				PartDynamicTextField *pdtf = nullptr;
				
				if      (qde.tagName() == "line")     cep = new PartLine     (m_element_editor);
				else if (qde.tagName() == "rect")     cep = new PartRectangle(m_element_editor);
				else if (qde.tagName() == "ellipse")  cep = new PartEllipse  (m_element_editor);
				else if (qde.tagName() == "circle")   cep = new PartEllipse  (m_element_editor);
				else if (qde.tagName() == "polygon")  cep = new PartPolygon  (m_element_editor);
				else if (qde.tagName() == "terminal") cep = new PartTerminal (m_element_editor);
				else if (qde.tagName() == "text")     cep = new PartText     (m_element_editor);
				else if (qde.tagName() == "arc")      cep = new PartArc      (m_element_editor);
				else if (qde.tagName() == "dynamic_text") cep = new PartDynamicTextField (m_element_editor);
					//For the input (aka the old text field) we try to convert it to the new partDynamicTextField
				else if (qde.tagName() == "input") cep = pdtf = new PartDynamicTextField(m_element_editor);
				else continue;
				
				if (QGraphicsItem *qgi = dynamic_cast<QGraphicsItem *>(cep))
				{
					if (!qgi->zValue())
						qgi->setZValue(z++);
					
					loaded_parts<<qgi;
					
					if(pdtf)
						pdtf->fromTextFieldXml(qde);
					else
						cep->fromXml(qde);
				}
				else
					delete cep;
			}
		}
	}
	
	return(loaded_parts);
}

/**
	Ajoute le contenu content a cet element
	@param content contenu ( = liste de parties) a charger
	@return Le contenu ajoute
*/
ElementContent ElementScene::addContent(const ElementContent &content) {
	foreach(QGraphicsItem *part, content) {
		addPrimitive(part);
	}
	return(content);
}

/**
	Ajoute le contenu content a cet element
	@param content contenu ( = liste de parties) a charger
	@param pos Position du coin superieur gauche du contenu apres avoir ete ajoute
	@return Le contenu ajoute
*/
ElementContent ElementScene::addContentAtPos(const ElementContent &content, const QPointF &pos) {
	// calcule le boundingRect du contenu a ajouter
	QRectF bounding_rect = elementContentBoundingRect(content);
	
	// en deduit le decalage a appliquer aux parties pour les poser au point demander
	QPointF offset = pos - bounding_rect.topLeft();
	
	// ajoute les parties avec le decalage adequat
	foreach(QGraphicsItem *part, content) {
		part -> setPos(part -> pos() + offset);
		addPrimitive(part);
	}
	return(content);
}

/**
	Add a primitive to the scene by wrapping it within an
	ElementPrimitiveDecorator group.
*/
void ElementScene::addPrimitive(QGraphicsItem *primitive) {
	if (!primitive) return;
	addItem(primitive);
}

/**
	Initialise la zone de collage
*/
void ElementScene::initPasteArea() {
	m_paste_area = new QGraphicsRectItem();
	m_paste_area -> setZValue(1000000);
	
	QPen paste_area_pen;
	paste_area_pen.setStyle(Qt::DashDotLine);
	paste_area_pen.setColor(QColor(30, 56, 86, 255));
	
	QBrush paste_area_brush;
	paste_area_brush.setStyle(Qt::SolidPattern);
	paste_area_brush.setColor(QColor(90, 167, 255, 64));
	
	m_paste_area -> setPen(paste_area_pen);
	m_paste_area -> setBrush(paste_area_brush);
}

/**
	Arrondit les coordonnees du point passees en parametre de facon a ce que ce
	point soit aligne sur la grille.
	@param point une reference vers un QPointF. Cet objet sera modifie.
	
*/
QPointF ElementScene::snapToGrid(QPointF point) {
	point.rx() = qRound(point.x() / m_x_grid) * m_x_grid;
	point.ry() = qRound(point.y() / m_y_grid) * m_y_grid;
	return point;
}

/**
	@return true if \a item1's zValue() is less than \a item2's.
*/
bool ElementScene::zValueLessThan(QGraphicsItem *item1, QGraphicsItem *item2) {
	return(item1-> zValue() < item2 -> zValue());
}

/**
 * @brief ElementScene::centerElementToOrigine
 * try to center better is possible the element to the scene
 * (the calcul isn't optimal but work good)
 */
void ElementScene::centerElementToOrigine() {
	QRectF size= elementSceneGeometricRect();
	int center_x = qRound(size.center().x());
	int center_y = qRound(size.center().y());

	//define the movement of translation
	int move_x = center_x - (center_x %10);
	if (center_x < 0) move_x -= 10;
	int move_y = center_y - (center_y %10);
	if (center_y < 0) move_y -= 10;

		//move each primitive by @move
		foreach (QGraphicsItem *qgi, items()) {
			if (qgi -> type() == ElementPrimitiveDecorator::Type) continue;
			if (qgi -> type() == QGraphicsRectItem::Type) continue;
			//deselect item for disable decorator
			qgi -> setSelected(false);
			qgi -> moveBy(-(move_x), -(move_y));
		}
		emit (needZoomFit());
}

/**
 * @brief ElementScene::managePrimitivesGroups
 * Ensure the decorator is adequately shown, hidden or updated so it always
 * represents the current selection.
 */
void ElementScene::managePrimitivesGroups()
{
		//this function is not supposed to be reentrant
	if (!m_decorator_lock->tryLock())
		return;
	
	if (!m_decorator)
	{
		m_decorator = new ElementPrimitiveDecorator();
		connect(m_decorator, SIGNAL(actionFinished(ElementEditionCommand*)), this, SLOT(stackAction(ElementEditionCommand *)));
		addItem(m_decorator);
		m_decorator -> hide();
	}
	
		// should we hide the decorator?
	QList<QGraphicsItem *> selected_items = zItems(ElementScene::Selected | ElementScene::IncludeTerminals);
    if (selected_items.size() <= 1)
	{
		m_decorator -> hide();
	}
	else
	{
		for(QGraphicsItem *qgi : selected_items)
		{
				//We recall set selected, then every primitive will remove there handler because there are several item selected
			qgi->setSelected(true);
		}
		m_decorator -> setZValue(1000000);
		m_decorator -> setPos(0, 0);
		m_decorator -> setItems(selected_items);
	}
	m_decorator_lock -> unlock();
}

/**
	Push the provided \a command on the undo stack.
*/
void ElementScene::stackAction(ElementEditionCommand *command) {
	if (command -> elementScene()) {
		if (command -> elementScene() != this) return;
	} else {
		command -> setElementScene(this);
	}
	
	if (!command -> elementView()) {
		foreach (QGraphicsView *view, views()) {
			if (ElementView *element_view = dynamic_cast<ElementView *>(view)) {
				command -> setElementView(element_view);
				break;
			}
		}
	}
	
	undoStack().push(command);
}
