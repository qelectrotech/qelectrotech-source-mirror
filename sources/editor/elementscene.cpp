/*
	Copyright 2006-2025 The QElectroTech Team
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

#include "../NameList/ui/namelistdialog.h"
#include "../NameList/ui/namelistwidget.h"
#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "editorcommands.h"
#include "elementcontent.h"
#include "elementprimitivedecorator.h"
#include "esevent/eseventinterface.h"
#include "graphicspart/partarc.h"
#include "graphicspart/partdynamictextfield.h"
#include "graphicspart/partellipse.h"
#include "graphicspart/partline.h"
#include "graphicspart/partpolygon.h"
#include "graphicspart/partrectangle.h"
#include "graphicspart/partterminal.h"
#include "graphicspart/parttext.h"
#include "ui/qetelementeditor.h"
#include "ui/elementpropertieseditorwidget.h"
#include "../qetversion.h"

#include <QKeyEvent>
#include <algorithm>
#include <cmath>

/**
	@brief ElementScene::ElementScene
	constructor
	\~French Constructeur
	\~ @param editor : Element editor concerned
	\~French L'editeur d'element concerne
	\~ @param parent : Widget parent
	\~French le Widget parent
*/
ElementScene::ElementScene(QETElementEditor *editor, QObject *parent) :
	QGraphicsScene(parent),
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
	m_decorator_lock = new QMutex();
	connect(&m_undo_stack, SIGNAL(indexChanged(int)),
		this, SLOT(managePrimitivesGroups()));
	connect(this, SIGNAL(selectionChanged()),
			this, SLOT(managePrimitivesGroups()));
}

/**
 * @brief ElementScene::elementData
 * @return the elementdata using by the scene
 */
ElementData ElementScene::elementData() {
	return m_element_data;
}

void ElementScene::setElementData(ElementData data)
{
	bool emit_ = m_element_data.m_informations != data.m_informations;

	m_element_data = data;

	if (emit_)
		emit elementInfoChanged();
}

/**
	@brief ElementScene::~ElementScene
*/
ElementScene::~ElementScene()
{
	//Disconnect to avoid crash, see bug report N° 122.
	disconnect(&m_undo_stack, SIGNAL(indexChanged(int)),
		   this, SLOT(managePrimitivesGroups()));
	delete m_decorator_lock;

	if (m_event_interface)
		delete m_event_interface;

	if (m_decorator)
		delete m_decorator;
}

/**
	@brief ElementScene::mouseMoveEvent
	@param e
*/
void ElementScene::mouseMoveEvent(QGraphicsSceneMouseEvent *e)
{
	if (m_event_interface) {
		if (m_event_interface -> mouseMoveEvent(e)) {
			if (m_event_interface->isFinish()) {
				delete m_event_interface;
				m_event_interface = nullptr;
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
	@brief ElementScene::mousePressEvent
	@param e
*/
void ElementScene::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
	if (m_event_interface) {
		if (m_event_interface -> mousePressEvent(e)) {
			if (m_event_interface->isFinish()) {
				delete m_event_interface;
				m_event_interface = nullptr;
			}
			return;
		}
	}

	QGraphicsScene::mousePressEvent(e);
}

/**
	@brief ElementScene::mouseReleaseEvent
	@param e
*/
void ElementScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *e)
{
	if (m_event_interface) {
		if (m_event_interface -> mouseReleaseEvent(e)) {
			if (m_event_interface->isFinish()) {
				delete m_event_interface;
				m_event_interface = nullptr;
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
	@brief ElementScene::mouseDoubleClickEvent
	@param event
*/
void ElementScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_event_interface) {
		if (m_event_interface -> mouseDoubleClickEvent(event)) {
			if (m_event_interface->isFinish()) {
				delete m_event_interface;
				m_event_interface = nullptr;
			}
			return;
		}
	}

	QGraphicsScene::mouseDoubleClickEvent(event);
}

/**
	@brief ElementScene::keyPressEvent
	manage key press event
	@param event
*/
void ElementScene::keyPressEvent(QKeyEvent *event)
{
	if (m_event_interface)
	{
		if (m_event_interface -> keyPressEvent(event))
		{
			if (m_event_interface->isFinish())
			{
				delete m_event_interface;
				m_event_interface = nullptr;
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
			QPropertyUndoCommand *undo =
					new QPropertyUndoCommand(
						qgo,"pos",QVariant(original_pos),QVariant(p));
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
	@brief ElementScene::contextMenuEvent
	Display the context menu event, only if behavior are Normal
	@param event
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
	@brief ElementScene::drawForeground
	Draws the background of the editor, ie the hotspot indicator.
	\~French Dessine l'arriere-plan de l'editeur, cad l'indicateur de hotspot.
	\~ @param p : The QPainter to use for drawing
	\~French Le QPainter a utiliser pour dessiner
*/
void ElementScene::drawForeground(QPainter *p, const QRectF &)
{
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
	@brief ElementScene::setEventInterface
	Set a new event interface
	@param event_interface
*/
void ElementScene::setEventInterface(ESEventInterface *event_interface)
{
	if (m_event_interface)
	{
		delete m_event_interface;
		//We must re-init because previous interface
		//Reset its own init when deleted
		event_interface->init();
	}
	m_event_interface = event_interface;
}

/**
	@brief ElementScene::clearEventInterface
	Clear the current event interface
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
	@brief ElementScene::setBehavior
	Modify the current behavior of this scene
	\~French Modifie the current behavior of this scene
	\~ @param b
*/
void ElementScene::setBehavior(ElementScene::Behavior b)
{
	m_behavior = b;
}

ElementScene::Behavior ElementScene::behavior() const
{
	return m_behavior;
}

/**
	@brief ElementScene::xGrid
	@return the horizontal size of the grid
	\~French la taille horizontale de la grille
*/
int ElementScene::xGrid() const
{
	return(m_x_grid);
}

/**
	@brief ElementScene::yGrid
	@return vertical grid size
	\~French la taille verticale de la grille
*/
int ElementScene::yGrid() const
{
	return(m_y_grid);
}

/**
	@brief ElementScene::setGrid
	\~ @param x_g : Horizontal grid size
	\~French Taille horizontale de la grille
	\~ @param y_g : Vertical grid size
	\~French Taille verticale de la grille
*/
void ElementScene::setGrid(int x_g, int y_g)
{
	m_x_grid = x_g ? x_g : 1;
	m_y_grid = y_g ? y_g : 1;
}

/**
	@brief ElementScene::toXml
	Export this element as a xml file
	@param all_parts : (true by default)
	if true, export the entire element in xml,
	if false, only export the selected parts.
	@return an xml document that describe the element.
*/
const QDomDocument ElementScene::toXml(bool all_parts)
{
	QRectF size= elementSceneGeometricRect();

	// if the element doesn't contain the origin point of the scene
	// we move the element to the origin to solve this default before saving
	if (!size.contains(0,0) && all_parts)
	{
		centerElementToOrigin();
		//recalculate the size after movement
		size = elementSceneGeometricRect();
	}

	// define the size of the element by the upper multiple of 10
	int upwidth = ((qRound(size.width())/10)*10)+10;
	if ((qRound(size.width())%10) > 6)
		upwidth += 10;

	int upheight = ((qRound(size.height())/10)*10)+10;
	if ((qRound(size.height())%10) > 6)
		upheight += 10;

	// the margin between the real size of the element and the rectangle that delimits
	int xmargin = qRound(upwidth - size.width());
	int ymargin = qRound(upheight - size.height());

	// document XML
	QDomDocument xml_document;

	//Root of xml document
	QDomElement root = xml_document.createElement("definition");
	root.setAttribute("type",        "element");
	root.setAttribute("width",       QString("%1").arg(upwidth));
	root.setAttribute("height",      QString("%1").arg(upheight));
	root.setAttribute("hotspot_x",   QString("%1").arg(
				  -(qRound(size.x() - (xmargin/2)))));
	root.setAttribute("hotspot_y",   QString("%1").arg(
				  -(qRound(size.y() - (ymargin/2)))));

	QetVersion::toXmlAttribute(root);
	root.setAttribute("link_type", m_element_data.typeToString(m_element_data.m_type));

	//Uuid used to compare two elements
	QDomElement uuid = xml_document.createElement("uuid");
	uuid.setAttribute("uuid", QUuid::createUuid().toString());
	root.appendChild(uuid);

	//names of element
	root.appendChild(m_element_data.m_names_list.toXml(xml_document));

	auto type_ = m_element_data.m_type;
	if (type_ == ElementData::Slave  ||
		type_ == ElementData::Master ||
		type_ == ElementData::Terminal)
	{
		root.appendChild(m_element_data.kindInfoToXml(xml_document));
	}

	if (type_ == ElementData::Simple ||
		type_ == ElementData::Master ||
		type_ == ElementData::Terminal ||
		type_ == ElementData::Thumbnail)
	{
		QDomElement element_info = xml_document.createElement("elementInformations");
		m_element_data.m_informations.toXml(element_info, "elementInformation");
		root.appendChild(element_info);
	}

	//complementary information about the element, when available
	if (!(m_element_data.m_drawing_information.trimmed().isEmpty())) {
		QDomElement informations_element = xml_document.createElement("informations");
		root.appendChild(informations_element);
		informations_element.appendChild(xml_document.createTextNode(m_element_data.m_drawing_information.trimmed()));
	}

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
	@brief ElementScene::boundingRectFromXml
	@param xml_document : an XML document describing an element
	\~French un document XML decrivant un element
	\~ @return the boundingRect of the element's content
	\~French le boundingRect du contenu de l'element
*/
QRectF ElementScene::boundingRectFromXml(const QDomDocument &xml_document)
{
	// load parts from XML document
	// charge les parties depuis le document XML
	ElementContent loaded_content = loadContent(xml_document);
	if (loaded_content.isEmpty()) return(QRectF());

	// calculate the boundingRect
	// calcule le boundingRect
	QRectF bounding_rect = elementContentBoundingRect(loaded_content);

	// destroy loaded parts
	// detruit les parties chargees
	qDeleteAll(loaded_content);

	return(bounding_rect);
}

/**
	@brief ElementScene::fromXml
	Imports the element described in an XML document. If a position is
	specified, the imported elements are positioned in such a way that the
	upper left corner of the smallest rectangle that can surround them all
	(the bounding rect) either at this position.
	\~French Importe l'element decrit dans un document XML. Si une position est
	precisee, les elements importes sont positionnes de maniere a ce que le
	coin superieur gauche du plus petit rectangle pouvant les entourant tous
	(le bounding rect) soit a cette position.
	\~ @param xml_document : an XML document describing the element
	\~French un document XML decrivant l'element
	\~ @param position : The position of the imported parts
	\~French La position des parties importees
	\~ @param consider_informations : If true, additional information
	(dimensions, hotspot, etc.) will be taken into account
	\~French Si vrai, les informations complementaires
	(dimensions, hotspot, etc.) seront prises en compte
	\~ @param content_ptr :
	if this pointer to an ElementContent is different from 0,
	it will be filled with the content added to the element by the fromXml
	\~French si ce pointeur vers un ElementContent est different de 0,
	il sera rempli avec le contenu ajoute a l'element par le fromXml
*/
void ElementScene::fromXml(
		const QDomDocument &xml_document,
		const QPointF &position,
		bool consider_informations,
		ElementContent *content_ptr)
{
	bool state = true;

		//Consider the informations of the element
	if (consider_informations)
	{
			// Root must be an element definition
		QDomElement root = xml_document.documentElement();

		if (root.tagName() == "definition" &&
			root.attribute("type") == "element") {
			m_element_data.fromXml(root);
		}
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
	@brief ElementScene::elementSceneGeometricRect
	@return the minimum, margin-less rectangle the element can fit into,
	in scene coordinates.
	It is different from itemsBoundingRect() because it is not supposed
	to imply any margin.
*/
QRectF ElementScene::elementSceneGeometricRect() const
{
	QRectF esgr;
	foreach (QGraphicsItem *qgi, items()) {
		if (qgi->type() == ElementPrimitiveDecorator::Type) continue;
		if (qgi->type() == QGraphicsRectItem::Type) continue;
		if (qgi->type() == PartText::Type) continue;
		if (qgi->type() == PartDynamicTextField::Type) continue;
		if (CustomElementPart *cep = dynamic_cast <CustomElementPart*> (qgi)) {
			esgr |= cep -> sceneGeometricRect();
		}
	}
	return (esgr);
}

/**
	@brief ElementScene::containsTerminals
	@return true if the element has at least one terminal,
	false if it has none.
	\~French true si l'element comporte au moins une borne,
	false s'il n'en a aucune.
*/
bool ElementScene::containsTerminals() const
{
	foreach(QGraphicsItem *qgi,items())
	{
		if (qgraphicsitem_cast<PartTerminal *>(qgi))
		{
			return(true);
		}
	}
	return(false);
}

/**
	@brief ElementScene::undoStack
	@return the undo stack of this element editor
	\~French la pile d'annulations de cet editeur d'element
*/
QUndoStack &ElementScene::undoStack()
{
	return(m_undo_stack);
}

/**
	@brief ElementScene::qgiManager
	@return the QGraphicsItem manager of this item editor
	\~French le gestionnaire de QGraphicsItem de cet editeur d'element
*/
QGIManager &ElementScene::qgiManager()
{
	return(m_qgi_manager);
}

/**
	@brief ElementScene::clipboardMayContainElement
	@return true if the clipboard appears to contain an element
	\~French true si le presse-papier semble contenir un element
*/
bool ElementScene::clipboardMayContainElement()
{
	QString clipboard_text = QApplication::clipboard() -> text().trimmed();
	bool may_be_element = clipboard_text.startsWith("<definition")
			&& clipboard_text.endsWith("</definition>");
	return(may_be_element);
}

/**
	@brief ElementScene::wasCopiedFromThisElement
	\~ @param clipboard_content :
	character string, probably coming from the clipboard.
	\~French chaine de caractere, provenant vraisemblablement du presse-papier.
	\~ @return
	true if clipboard_content has been copied from this element.
	\~French true si clipboard_content a ete copie depuis cet element.
*/
bool ElementScene::wasCopiedFromThisElement(const QString &clipboard_content)
{
	return(clipboard_content == m_last_copied);
}

/**
	@brief ElementScene::cut
	Handles the fact of cutting the selection
	= exporting it in XML to the clipboard then deleting it.
	\~French Gere le fait de couper la selection
	= l'exporter en XML dans le presse-papier puis la supprimer.
*/
void ElementScene::cut()
{
	copy();
	QList<QGraphicsItem *> cut_content = selectedItems();
	clearSelection();
	undoStack().push(new CutPartsCommand(this, cut_content));
}

/**
	@brief ElementScene::copy
	Handles the fact of copying the selection
	= exporting it as XML to the clipboard.
	\~French Gere le fait de copier la selection
	= l'exporter en XML dans lepresse-papier.
*/
void ElementScene::copy()
{
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

/**
	@brief ElementScene::editor
	@return
*/
QETElementEditor* ElementScene::editor() const
{
	return m_element_editor;
}

/**
 * @brief ElementScene::addItems
 * Add items to the scene and emit partsAdded.
 * Prefer always use this method instead of QGraphicsScene::addItem
 * even if you want to add one item, for gain the signal emission
 * @param items
 */
void ElementScene::addItems(QVector<QGraphicsItem *> items)
{
	for (const auto &item : items) {
		addItem(item);
	}

	emit partsAdded();
}

/**
 * @brief ElementScene::removeItems
 * Remove items from the scene and emit partsRemoved.
 * Prefer always use this method instead of QGraphicsScene::removeItem
 * even if you want to remove one item, for gain the signal emission
 * @param items
 */
void ElementScene::removeItems(QVector<QGraphicsItem *> items)
{
	const qsizetype previous_selected_count{selectedItems().size()};

		//block signal to avoid multiple emit of selection changed,
		//we emit this signal only once at the end of this function.
	blockSignals(true);
	for (const auto &item : items) {
		removeItem(item);
	}
	blockSignals(false);

	if (previous_selected_count != selectedItems().size()) {
		emit selectionChanged();
	}

	emit partsRemoved();
}

/**
	@brief ElementScene::slot_select
	Select the item in content,
	every others items in the scene are deselected
	@param content
*/
void ElementScene::slot_select(const ElementContent &content)
{
	blockSignals(true);

	/* Before clearing selection,
	 * we must remove the handlers items in @content,
	 * because if in @content there are a selected item,
	 * but also its handlers items, When item is deselected,
	 * the item deletes its handlers items,
	 * then handlers in content doesn't exist anymore and cause segfault
	 */
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
	@brief ElementScene::slot_selectAll
	Select all items
*/
void ElementScene::slot_selectAll()
{
	slot_select(items());
}

/**
	@brief ElementScene::slot_deselectAll
	deselect all item
*/
void ElementScene::slot_deselectAll()
{
	slot_select(ElementContent());
}

/**
	@brief ElementScene::slot_invertSelection
	Inverse Selection
	\~French Inverse la selection
*/
void ElementScene::slot_invertSelection()
{
	blockSignals(true);
	foreach(QGraphicsItem *qgi,
			items()) qgi -> setSelected(!qgi -> isSelected());
	blockSignals(false);
	emit(selectionChanged());
}

/**
	@brief ElementScene::slot_delete
	Delete selected items
	\~French Supprime les elements selectionnes
*/
void ElementScene::slot_delete()
{
	const auto selected_items{selectedItems().toVector()};
	if (selected_items.isEmpty()) {
		return;
	}

	m_undo_stack.push(new DeletePartsCommand(this, selected_items));

		// removing items does not trigger QGraphicsScene::selectionChanged()
	emit selectionChanged();
}

/**
	@brief ElementScene::slot_editAuthorInformations
	Starts a dialog to edit the additional information of this element.
	Concretely, this free field is intended to receive information
	on the author of the element, its license, etc.
	\~French Lance un dialogue pour editer les informations complementaires
	de cet element. Concretement, ce champ libre est destine a accueillir
	des informations sur l'auteur de l'element, sa licence, etc.
*/
void ElementScene::slot_editAuthorInformations()
{
	bool is_read_only = m_element_editor && m_element_editor -> isReadOnly();

	// create a dialogue
	// cree un dialogue
	QDialog dialog_author(m_element_editor);
	dialog_author.setModal(true);
#ifdef Q_OS_MACOS
	dialog_author.setWindowFlags(Qt::Sheet);
#endif
	dialog_author.setMinimumSize(400, 260);
	dialog_author.setWindowTitle(
				tr("Éditer les informations sur l'auteur", "window title"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&dialog_author);

	// adds an explanatory field to the dialogue
	// ajoute un champ explicatif au dialogue
	QLabel *information_label = new QLabel(tr("Vous pouvez utiliser ce champ libre pour mentionner les auteurs de l'élément, sa licence, ou tout autre renseignement que vous jugerez utile."));
	information_label -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	information_label -> setWordWrap(true);
	dialog_layout -> addWidget(information_label);

	// add a QTextEdit to the dialog
	// ajoute un QTextEdit au dialogue
	QTextEdit *text_field = new QTextEdit();
	text_field -> setAcceptRichText(false);
	text_field -> setPlainText(m_element_data.m_drawing_information);
	text_field -> setReadOnly(is_read_only);
	dialog_layout -> addWidget(text_field);

	// ajoute deux boutons au dialogue
	QDialogButtonBox *dialog_buttons = new QDialogButtonBox(
				is_read_only ? QDialogButtonBox::Ok :
						   QDialogButtonBox::Ok
						   | QDialogButtonBox::Cancel);
	dialog_layout -> addWidget(dialog_buttons);
	connect(dialog_buttons, SIGNAL(accepted()),&dialog_author, SLOT(accept()));
	connect(dialog_buttons, SIGNAL(rejected()),&dialog_author, SLOT(reject()));

	// start the dialogue
	// lance le dialogue
	if (dialog_author.exec() == QDialog::Accepted && !is_read_only)
	{
		QString new_infos = text_field -> toPlainText().remove(QChar(13)); // CR-less text
		if (new_infos != m_element_data.m_drawing_information)
		{
			undoStack().push(new ChangeInformationsCommand(
								 this, m_element_data.m_drawing_information, new_infos));
		}
	}
}

/**
	@brief ElementScene::slot_editProperties
	Open dialog to edit the element properties
*/
void  ElementScene::slot_editProperties()
{
	ElementPropertiesEditorWidget epew(m_element_data);
	epew.exec();

	if (m_element_data != epew.editedData())
	{
		undoStack().push(new changeElementDataCommand(this,
													  m_element_data,
													  epew.editedData()));
	}
}

/**
	@brief ElementScene::slot_editNames
	Launch a dialog for edit the names of the edited element
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
	nlw_->setNames(m_element_data.m_names_list);
	nlw_->setReadOnly(is_read_only);

	if (dialog_.exec() == QDialog::Accepted && !is_read_only && !nlw_->isEmpty())
	{
		NamesList new_names = nlw_->names();
		if (new_names != m_element_data. m_names_list) {
			undoStack().push(new ChangeNamesCommand(this,
								m_element_data.m_names_list,
								new_names));
		}
	}
}

/**
	@brief ElementScene::primitives
	@return the list of primitives currently present on the scene.
*/
QList<CustomElementPart *> ElementScene::primitives() const
{
	QList<CustomElementPart *> primitives_list;
	foreach (QGraphicsItem *item, items())
	{
		if (CustomElementPart *primitive = dynamic_cast<CustomElementPart *>(item))
		{
			primitives_list << primitive;
		}
	}
	return(primitives_list);
}

/**
	@brief ElementScene::zItems
	@param options
	@return the parts of the element ordered by increasing zValue
	\~French les parties de l'element ordonnes par zValue croissante
*/
QList<QGraphicsItem *> ElementScene::zItems(ItemOptions options) const
{
	// handle dummy request, i.e. when neither Selected nor NonSelected are set
	if (!(options & ElementScene::Selected)
			&&
			!(options & ElementScene::NonSelected))
	{
		return(QList<QGraphicsItem *>());
	}

	// retrieve all items
	QList<QGraphicsItem *> all_items_list(items());
	QMutableListIterator<QGraphicsItem *> i(all_items_list);

	// remove unrequired items
	if ((options & ElementScene::SelectedOrNot)
			!= ElementScene::SelectedOrNot) {
		bool keep_selected = options & ElementScene::Selected;
		while (i.hasNext()) {
			if (i.next() -> isSelected() != keep_selected) {
				i.remove();
			}
		}
	}

	QList<QGraphicsItem *> terminals;
	QList<QGraphicsItem *> helpers;
	for (i.toFront(); i.hasNext(); )
	{
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
		else if (qgraphicsitem_cast<PartTerminal *>(qgi))
		{
			i.remove();
			terminals << qgi;
		}
	}

	// orders the parts by their zValue
	// ordonne les parties par leur zValue
	if (options & SortByZValue)
	{
		std::sort (
					all_items_list.begin(),
					all_items_list.end(),
					ElementScene::zValueLessThan);
	}

	// possibly add the limits
	// rajoute eventuellement les bornes
	if (options & ElementScene::IncludeTerminals)
	{
		all_items_list += terminals;
	}
	if (options & ElementScene::IncludeHelperItems)
	{
		all_items_list += helpers;
	}
	return(all_items_list);
}

/**
	@brief ElementScene::selectedContent
	@return the selected graphic parts
	\~French les parties graphiques selectionnees
*/
ElementContent ElementScene::selectedContent() const
{
	ElementContent content;
	foreach(QGraphicsItem *qgi, zItems())
	{
		if (qgi -> isSelected()) content << qgi;
	}
	return(content);
}

/**
	@brief ElementScene::getPasteArea
	\~ @param to_paste : Rectangle enclosing the parts to be glued
	\~French Rectangle englobant les parties a coller
	\~ @return the rectangle where you will have to glue these parts
	\~French le rectangle ou il faudra coller ces parties
*/
void ElementScene::getPasteArea(const QRectF &to_paste)
{
	// we draw it on the stage
	// on le dessine sur la scene
	m_paste_area -> setRect(to_paste);
	addItem(m_paste_area);
	//we switch the scene to "zone search for copy / paste" mode
	// on passe la scene en mode "recherche de zone pour copier/coller"
	m_behavior = PasteArea;
}

/**
	@brief ElementScene::reset
	Remove all QGraphicsItems in the scene and clear the undo stack.
*/
void ElementScene::reset()
{
	clearSelection();
	undoStack().clear();

	// We don't add handlers,
	// because it's the role of the primitive or decorator to remove it.
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
	@brief ElementScene::elementContentBoundingRect
	\~ @param content : Content (= parts) of an element
	\~French Contenu ( = parties) d'un element
	\~ @return the boundingRect of these parts,
	expressed in the coordinates of the scene
	\~French  le boundingRect de ces parties,
	exprime dans les coordonnes de la scene
*/
QRectF ElementScene::elementContentBoundingRect(
		const ElementContent &content) const
{
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
	@brief ElementScene::loadContent
	Create and load the content describe in the xml document.
	@param xml_document : xml dom document to analyze
	@return the loaded content
*/
ElementContent ElementScene::loadContent(const QDomDocument &xml_document)
{
	ElementContent loaded_parts;

	// The root is supposed to be an element definition
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
	@brief ElementScene::addContent
	Add content content to this element
	\~French Ajoute le contenu content a cet element
	\~ @param content :
	content (= list of parts) to load
	\~French contenu ( = liste de parties) a charger
	\~ @return Content adds
	\~French Le contenu ajoute
*/
ElementContent ElementScene::addContent(const ElementContent &content)
{
	foreach(QGraphicsItem *part, content) {
		addPrimitive(part);
	}
	return(content);
}

/**
	@brief ElementScene::addContentAtPos
	Add content content to this element
	\~French Ajoute le contenu content a cet element
	\~ @param content :
	content (= list of parts) to load
	\~French contenu ( = liste de parties) a charger
	\~ @param pos :
	Position of the upper left corner of the content after being added
	\~French Position du coin superieur gauche du contenu apres avoir ete ajoute
	\~ @return Content adds
	\~French Le contenu ajoute
*/
ElementContent ElementScene::addContentAtPos(
		const ElementContent &content,
		const QPointF &pos)
{
	// calculate the boundingRect of the content to add
	// calcule le boundingRect du contenu a ajouter
	QRectF bounding_rect = elementContentBoundingRect(content);

	/*   deduced the offset to be applied to the parts to place
	 * them at the point requested
	 *   en deduit le decalage a appliquer aux parties pour les poser
	 * au point demander
	 */
	QPointF offset = pos - bounding_rect.topLeft();

	// add the parts with the correct offset
	// ajoute les parties avec le decalage adequat
	foreach(QGraphicsItem *part, content) {
		part -> setPos(part -> pos() + offset);
		addPrimitive(part);
	}
	return(content);
}

/**
	@brief ElementScene::addPrimitive
	Add a primitive to the scene by wrapping it within an
	ElementPrimitiveDecorator group.
	@param primitive
*/
void ElementScene::addPrimitive(QGraphicsItem *primitive)
{
	if (!primitive) return;
	addItem(primitive);
}

/**
	@brief ElementScene::initPasteArea
	Initializes the paste area
	\~French Initialise la zone de collage
*/
void ElementScene::initPasteArea()
{
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
	@brief ElementScene::snapToGrid
	Rounds the coordinates of the point passed as a parameter
	so that this point is aligned with the grid.
	\~French Arrondit les coordonnees du point passees en parametre
	de facon a ce que ce point soit aligne sur la grille.
	\~ @param point :
	a reference to a QPointF. This object will be modified.
	\~French une reference vers un QPointF. Cet objet sera modifie.
	\~ @return point
*/
QPointF ElementScene::snapToGrid(QPointF point)
{
	point.rx() = qRound(point.x() / m_x_grid) * m_x_grid;
	point.ry() = qRound(point.y() / m_y_grid) * m_y_grid;
	return point;
}

/**
	@brief ElementScene::zValueLessThan
	@param item1 : QGraphicsItem
	@param item2 : QGraphicsItem
	@return true if \a item1's zValue() is less than \a item2's.
*/
bool ElementScene::zValueLessThan(QGraphicsItem *item1, QGraphicsItem *item2)
{
	return(item1-> zValue() < item2 -> zValue());
}

/**
	@brief ElementScene::centerElementToOrigin
	try to center better is possible the element to the scene
*/
void ElementScene::centerElementToOrigin()
{
	QRectF size= elementSceneGeometricRect();
	// relative move of each primitive with @move with integer values
	int offsetX = qRound(size.center().x()) * (-1);
	int offsetY = qRound(size.center().y()) * (-1);
	foreach (QGraphicsItem *qgi, items()) {
		if (qgi -> type() == ElementPrimitiveDecorator::Type) continue;
		if (qgi -> type() == QGraphicsRectItem::Type) continue;
		// deselect item to disable decorator
		qgi -> setSelected(false);
		qgi -> moveBy(offsetX, offsetY);
	}
	emit (needZoomFit());
}

/**
	@brief ElementScene::managePrimitivesGroups
	Ensure the decorator is adequately shown, hidden or updated so it always
	represents the current selection.
*/
void ElementScene::managePrimitivesGroups()
{
	//this function is not supposed to be reentrant
	if (!m_decorator_lock->tryLock())
		return;

	if (!m_decorator)
	{
		m_decorator = new ElementPrimitiveDecorator();
		connect(m_decorator,
			SIGNAL(actionFinished(ElementEditionCommand*)),
			this, SLOT(stackAction(ElementEditionCommand *)));
		addItem(m_decorator);
		m_decorator -> hide();
	}

	if (m_single_selected_item) {
		m_single_selected_item->removeHandler();
		m_single_selected_item.clear();
	}

		// should we hide the decorator?
	const auto selected_items{zItems(ElementScene::Selected | ElementScene::IncludeTerminals)};
	if (selected_items.size() <= 1)
	{
		m_decorator->hide();

		if (!selected_items.isEmpty())
		{
			if (CustomElementGraphicPart *item_ = dynamic_cast<CustomElementGraphicPart *>(selected_items.first()))
			{
				item_->addHandler();
				m_single_selected_item = item_;
			}
		}
	}
	else
	{
		m_decorator -> setZValue(1000000);
		m_decorator -> setPos(0, 0);
		m_decorator -> setItems(selected_items);
	}
	m_decorator_lock -> unlock();
}

/**
	@brief ElementScene::stackAction
	Push the provided \a command on the undo stack.
	@param command
*/
void ElementScene::stackAction(ElementEditionCommand *command)
{
	if (command -> elementScene()) {
		if (command -> elementScene() != this) return;
	} else {
		command -> setElementScene(this);
	}

	if (!command -> elementView()) {
		foreach (QGraphicsView *view, views()) {
			if (ElementView *element_view =
					dynamic_cast<ElementView *>(view)) {
				command -> setElementView(element_view);
				break;
			}
		}
	}

	undoStack().push(command);
}
