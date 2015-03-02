/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "parttextfield.h"
#include "partarc.h"
#include "editorcommands.h"
#include "elementcontent.h"
#include "nameslist.h"
#include "ui/elementpropertieseditorwidget.h"
#include "eseventinterface.h"

#include <QKeyEvent>

/**
	Constructeur
	@param editor L'editeur d'element concerne
	@param parent le Widget parent
*/
ElementScene::ElementScene(QETElementEditor *editor, QObject *parent) :
	QGraphicsScene(parent),
	m_elmt_type("simple"),
	qgi_manager(this),
	m_event_interface(nullptr),
	element_editor(editor),
	decorator_(0)
{
	behavior = Normal;
	setItemIndexMethod(NoIndex);
	setGrid(1, 1);
	initPasteArea();
	undo_stack.setClean();
	decorator_lock_ = new QMutex(QMutex::NonRecursive);
	connect(&undo_stack, SIGNAL(indexChanged(int)), this, SLOT(managePrimitivesGroups()));
	connect(this, SIGNAL(selectionChanged()), this, SLOT(managePrimitivesGroups()));
}

/// Destructeur
ElementScene::~ElementScene() {
	delete decorator_lock_;
	if (m_event_interface) delete m_event_interface;
}

/**
 * @brief ElementScene::mouseMoveEvent
 * @param e
 */
void ElementScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	if (m_event_interface) {
		if (m_event_interface -> mouseMoveEvent(e)) {
			if (m_event_interface->isFinish()) {
				emit(partsAdded());
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}
	}

	QPointF event_pos = e -> scenePos();
	if (!e -> modifiers() & Qt::ControlModifier)
		event_pos = snapToGrid(event_pos);
	
	if (behavior == PasteArea) {
		QRectF current_rect(paste_area_ -> rect());
		current_rect.moveCenter(event_pos);
		paste_area_ -> setRect(current_rect);
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
				emit(partsAdded());
				delete m_event_interface; m_event_interface = nullptr;
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
				emit(partsAdded());
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}
	}
	
	if (behavior == PasteArea) {
		defined_paste_area_ = paste_area_ -> rect();
		removeItem(paste_area_);
		emit(pasteAreaDefined(defined_paste_area_));
		behavior = Normal;
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
				emit(partsAdded());
				delete m_event_interface; m_event_interface = nullptr;
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
void ElementScene::keyPressEvent(QKeyEvent *event) {
	if (m_event_interface) {
		if (m_event_interface -> keyPressEvent(event)) {
			if (m_event_interface->isFinish()) {
				emit(partsAdded());
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}
	}
	QGraphicsScene::keyPressEvent(event);
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
void ElementScene::setEventInterface(ESEventInterface *event_interface) {
	if (m_event_interface) {
		delete m_event_interface;
		//We must to re-init because previous interface
		//Reset his own init when deleted
		event_interface->init();
	}
	m_event_interface = event_interface;
}

/**
	@return la taille horizontale de la grille
*/
int ElementScene::xGrid() const {
	return(x_grid);
}

/**
	@return la taille verticale de la grille
*/
int ElementScene::yGrid() const {
	return(y_grid);
}

/**
	@param x_g Taille horizontale de la grille
	@param y_g Taille verticale de la grille
*/
void ElementScene::setGrid(int x_g, int y_g) {
	x_grid = x_g ? x_g : 1;
	y_grid = y_g ? y_g : 1;
}

/**
	Exporte l'element en XML
	@param all_parts Booleen (a vrai par defaut) indiquant si le XML genere doit
	representer tout l'element ou seulement les elements selectionnes
	@return un document XML decrivant l'element
*/
const QDomDocument ElementScene::toXml(bool all_parts) {
	QRectF size= elementSceneGeometricRect();
	//if the element doesn't contains the origin point of the scene
	//we move the element to the origin for solve this default before saving
	if (!size.contains(0,0) && all_parts) {
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
	// racine du document XML
	QDomElement root = xml_document.createElement("definition");
	root.setAttribute("type",        "element");
	root.setAttribute("width",       QString("%1").arg(upwidth));
	root.setAttribute("height",      QString("%1").arg(upheight));
	root.setAttribute("hotspot_x",   QString("%1").arg(-(qRound(size.x() - (xmargin/2)))));
	root.setAttribute("hotspot_y",   QString("%1").arg(-(qRound(size.y() - (ymargin/2)))));
	root.setAttribute("orientation", "dyyy"); //we keep the orientation for compatibility with previous version of qet
	root.setAttribute("version", QET::version);
	root.setAttribute("link_type", m_elmt_type);
	
	// noms de l'element
	root.appendChild(_names.toXml(xml_document));

	if (m_elmt_type == "slave" || m_elmt_type == "master") {
		QDomElement kindInfo = xml_document.createElement("kindInformations");
		m_elmt_kindInfo.toXml(kindInfo, "kindInformation");
		root.appendChild(kindInfo);
	}
	
	// informations complementaires de l'element
	QDomElement informations_element = xml_document.createElement("informations");
	root.appendChild(informations_element);
	informations_element.appendChild(xml_document.createTextNode(informations()));
	
	QDomElement description = xml_document.createElement("description");
	// description de l'element
	foreach(QGraphicsItem *qgi, zItems()) {
		// si l'export ne concerne que la selection, on ignore les parties non selectionnees
		if (!all_parts && !qgi -> isSelected()) continue;
		if (CustomElementPart *ce = dynamic_cast<CustomElementPart *>(qgi)) {
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
void ElementScene::fromXml(
	const QDomDocument &xml_document,
	const QPointF &position,
	bool consider_informations,
	ElementContent *content_ptr
) {
	QString error_message;
	bool state = true;
	
	// prend en compte les informations de l'element
	if (consider_informations) {
		state = applyInformations(xml_document, &error_message);
	}
	
	// parcours des enfants de la definition : parties de l'element
	if (state) {
		ElementContent loaded_content = loadContent(xml_document, &error_message);
		if (position != QPointF()) {
			addContentAtPos(loaded_content, position, &error_message);
		} else {
			addContent(loaded_content, &error_message);
		}
		
		// renvoie le contenu ajoute a l'element
		if (content_ptr) {
			*content_ptr = loaded_content;
		}
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
		if (qgi -> type() == ElementPrimitiveDecorator::Type) continue;
		if (qgi -> type() == QGraphicsRectItem::Type) continue;
		if (qgi -> type() == PartTextField::Type) continue;
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
	return(undo_stack);
}

/**
	@return le gestionnaire de QGraphicsItem de cet editeur d'element
*/
QGIManager &ElementScene::qgiManager() {
	return(qgi_manager);
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
	return(clipboard_content == last_copied_);
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
	last_copied_ = clipboard_content;
}

void ElementScene::contextMenu(QContextMenuEvent *event) {
	if (behavior == ElementScene::Normal)
		element_editor -> contextMenu(event);
}

QETElementEditor* ElementScene::editor() const {
	return element_editor;
}

/**
	Selectionne une liste de parties
	@param content liste des parties a selectionner
*/
void ElementScene::slot_select(const ElementContent &content) {
	blockSignals(true);
	clearSelection();
	foreach(QGraphicsItem *qgi, content) qgi -> setSelected(true);
	blockSignals(false);
	emit(selectionChanged());
}

/**
	Selectionne tout
*/
void ElementScene::slot_selectAll() {
	slot_select(items());
}

/**
	Deselectionne tout
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
	undo_stack.push(new DeletePartsCommand(this, selected_items));
	
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
	bool is_read_only = element_editor && element_editor -> isReadOnly();
	
	// cree un dialogue
	QDialog dialog_author(element_editor);
	dialog_author.setModal(true);
#ifdef Q_OS_MAC
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
void  ElementScene::slot_editProperties() {
	QString type = m_elmt_type;
	DiagramContext info = m_elmt_kindInfo;
	ElementPropertiesEditorWidget epew(type, info);
	epew.exec();
	if (type != m_elmt_type || info != m_elmt_kindInfo)
		undoStack().push(new ChangePropertiesCommand(this, type, info));
}

/**
	Lance un dialogue pour editer les noms de cet element
*/
void ElementScene::slot_editNames() {
	bool is_read_only = element_editor && element_editor -> isReadOnly();
	
	// cree un dialogue
	QDialog dialog(element_editor);
#ifdef Q_OS_MAC
	dialog.setWindowFlags(Qt::Sheet);
#endif
	dialog.setModal(true);
	dialog.setMinimumSize(400, 330);
	dialog.setWindowTitle(tr("Éditer les noms", "window title"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&dialog);
	
	// ajoute un champ explicatif au dialogue
	QLabel *information_label = new QLabel(tr("Vous pouvez spécifier le nom de l'élément dans plusieurs langues."));
	information_label -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	information_label -> setWordWrap(true);
	dialog_layout -> addWidget(information_label);
	
	// ajoute un NamesListWidget au dialogue
	NamesListWidget *names_widget = new NamesListWidget();
	names_widget -> setNames(_names);
	names_widget -> setReadOnly(is_read_only);
	dialog_layout -> addWidget(names_widget);
	
	// ajoute deux boutons au dialogue
	QDialogButtonBox *dialog_buttons = new QDialogButtonBox(is_read_only ? QDialogButtonBox::Ok : QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout -> addWidget(dialog_buttons);
	connect(dialog_buttons, SIGNAL(accepted()),     names_widget, SLOT(check()));
	connect(names_widget,   SIGNAL(inputChecked()), &dialog,      SLOT(accept()));
	connect(dialog_buttons, SIGNAL(rejected()),     &dialog,      SLOT(reject()));
	
	// lance le dialogue
	if (dialog.exec() == QDialog::Accepted && !is_read_only) {
		NamesList new_names(names_widget -> names());
		if (new_names != _names) undoStack().push(new ChangeNamesCommand(this, _names, new_names));
	}
}

/**
	Amene les elements selectionnes au premier plan
*/
void ElementScene::slot_bringForward() {
	undoStack().push(new ChangeZValueCommand(this, ChangeZValueCommand::BringForward));
	emit(partsZValueChanged());
}

/**
	Remonte les elements selectionnes d'un plan
*/
void ElementScene::slot_raise() {
	undoStack().push(new ChangeZValueCommand(this, ChangeZValueCommand::Raise));
	emit(partsZValueChanged());
}

/**
	Descend les elements selectionnes d'un plan
*/
void ElementScene::slot_lower() {
	undoStack().push(new ChangeZValueCommand(this, ChangeZValueCommand::Lower));
	emit(partsZValueChanged());
}

/**
	Envoie les elements selectionnes au fond
*/
void ElementScene::slot_sendBackward() {
	undoStack().push(new ChangeZValueCommand(this, ChangeZValueCommand::SendBackward));
	emit(partsZValueChanged());
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
			qgi -> type() == QGraphicsRectItem::Type
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
		qSort(all_items_list.begin(), all_items_list.end(), ElementScene::zValueLessThan);
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
	paste_area_ -> setRect(to_paste);
	addItem(paste_area_);
	
	// on passe la scene en mode "recherche de zone pour copier/coller"
	behavior = PasteArea;
}

/**
	Supprime les parties de l'element et les objets d'annulations.
	Les autres caracteristiques sont conservees.
*/
void ElementScene::reset() {
	// supprime les objets d'annulation
	undoStack().clear();

	// enleve les elements de la scene
	foreach (QGraphicsItem *qgi, items()) {	
		removeItem(qgi);
		qgiManager().release(qgi);
	}
	decorator_ = 0;
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
	@param error_message pointeur vers une QString ; si error_message est
	different de 0, un message d'erreur sera stocke dedans si necessaire
	@return true si la lecture et l'application des informations s'est bien
	passee, false sinon.
*/
bool ElementScene::applyInformations(const QDomDocument &xml_document, QString *error_message) {
	// Root must be an element definition
	QDomElement root = xml_document.documentElement();
	if (root.tagName() != "definition" || root.attribute("type") != "element") {
		if (error_message) {
			*error_message = tr("Ce document XML n'est pas une définition d'élément.", "error message");
		}
		return(false);
	}

	//Extract info about element type
	m_elmt_type = root.attribute("link_type", "simple");
	m_elmt_kindInfo.fromXml(root.firstChildElement("kindInformations"), "kindInformation");

	//Extract names of xml definition
	_names.fromXml(root);
	
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
ElementContent ElementScene::loadContent(const QDomDocument &xml_document, QString *error_message) {
	ElementContent loaded_parts;
	
	// la racine est supposee etre une definition d'element
	QDomElement root = xml_document.documentElement();
	if (root.tagName() != "definition" || root.attribute("type") != "element") {
		if (error_message) {
			*error_message = tr("Ce document XML n'est pas une définition d'élément.", "error message");
		}
		return(loaded_parts);
	}
	
	// chargement de la description graphique de l'element
	for (QDomNode node = root.firstChild() ; !node.isNull() ; node = node.nextSibling()) {
		QDomElement elmts = node.toElement();
		if (elmts.isNull()) continue;
		if (elmts.tagName() == "description") {
			
			//  = parcours des differentes parties du dessin
			int z = 1;
			for (QDomNode n = node.firstChild() ; !n.isNull() ; n = n.nextSibling()) {
				QDomElement qde = n.toElement();
				if (qde.isNull()) continue;
				CustomElementPart *cep;
				if      (qde.tagName() == "line")     cep = new PartLine     (element_editor);
				else if (qde.tagName() == "rect")     cep = new PartRectangle(element_editor);
				else if (qde.tagName() == "ellipse")  cep = new PartEllipse  (element_editor);
				else if (qde.tagName() == "circle")   cep = new PartEllipse  (element_editor);
				else if (qde.tagName() == "polygon")  cep = new PartPolygon  (element_editor);
				else if (qde.tagName() == "terminal") cep = new PartTerminal (element_editor);
				else if (qde.tagName() == "text")     cep = new PartText     (element_editor);
				else if (qde.tagName() == "input")    cep = new PartTextField(element_editor);
				else if (qde.tagName() == "arc")      cep = new PartArc      (element_editor);
				else continue;
				if (QGraphicsItem *qgi = dynamic_cast<QGraphicsItem *>(cep)) {
					if (!qgi -> zValue()) qgi -> setZValue(z++);
					loaded_parts << qgi;
				}
				cep -> fromXml(qde);
			}
		}
	}
	
	return(loaded_parts);
}

/**
	Ajoute le contenu content a cet element
	@param content contenu ( = liste de parties) a charger
	@param error_message pointeur vers une QString ; si error_message est
	different de 0, un message d'erreur sera stocke dedans si necessaire
	@return Le contenu ajoute
*/
ElementContent ElementScene::addContent(const ElementContent &content, QString *error_message) {
	Q_UNUSED(error_message);
	foreach(QGraphicsItem *part, content) {
		addPrimitive(part);
	}
	return(content);
}

/**
	Ajoute le contenu content a cet element
	@param content contenu ( = liste de parties) a charger
	@param pos Position du coin superieur gauche du contenu apres avoir ete ajoute
	@param error_message pointeur vers une QString ; si error_message est
	different de 0, un message d'erreur sera stocke dedans si necessaire
	@return Le contenu ajoute
*/
ElementContent ElementScene::addContentAtPos(const ElementContent &content, const QPointF &pos, QString *error_message) {
	Q_UNUSED(error_message);
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
	paste_area_ = new QGraphicsRectItem();
	paste_area_ -> setZValue(1000000);
	
	QPen paste_area_pen;
	paste_area_pen.setStyle(Qt::DashDotLine);
	paste_area_pen.setColor(QColor(30, 56, 86, 255));
	
	QBrush paste_area_brush;
	paste_area_brush.setStyle(Qt::SolidPattern);
	paste_area_brush.setColor(QColor(90, 167, 255, 64));
	
	paste_area_ -> setPen(paste_area_pen);
	paste_area_ -> setBrush(paste_area_brush);
}

/**
	Arrondit les coordonnees du point passees en parametre de facon a ce que ce
	point soit aligne sur la grille.
	@param point une reference vers un QPointF. Cet objet sera modifie.
	
*/
QPointF ElementScene::snapToGrid(QPointF point) {
	point.rx() = qRound(point.x() / x_grid) * x_grid;
	point.ry() = qRound(point.y() / y_grid) * y_grid;
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
	Ensure the decorator is adequately shown, hidden or updated so it always
	represents the current selection.
*/
void ElementScene::managePrimitivesGroups() {
	// this function is not supposed to be reentrant
	if (!decorator_lock_ -> tryLock()) return;
	
	if (!decorator_) {
		decorator_ = new ElementPrimitiveDecorator();
		connect(decorator_, SIGNAL(actionFinished(ElementEditionCommand*)), this, SLOT(stackAction(ElementEditionCommand *)));
		addItem(decorator_);
		decorator_ -> hide();
	}
	
	// should we hide the decorator?
	QList<QGraphicsItem *> selected_items = zItems(ElementScene::Selected | ElementScene::IncludeTerminals);
	if (!selected_items.count()) {
		decorator_ -> hide();
	} else {
		decorator_ -> setZValue(1000000);
		decorator_ -> setPos(0, 0);
		decorator_ -> setItems(selected_items);
	}
	decorator_lock_ -> unlock();
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
