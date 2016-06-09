/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "diagramview.h"
#include "diagram.h"
#include "qetgraphicsitem/customelement.h"
#include "qetgraphicsitem/ghostelement.h"
#include "qetgraphicsitem/conductor.h"
#include "diagramcommands.h"
#include "diagramposition.h"
#include "conductorpropertieswidget.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/elementtextitem.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/diagramimageitem.h"
#include "templatelocation.h"
#include "qetapp.h"
#include "qetproject.h"
#include "projectview.h"
#include "integrationmovetemplateshandler.h"
#include "qetdiagrameditor.h"
#include "qeticons.h"
#include "qetmessagebox.h"
#include "qtextorientationspinboxwidget.h"
#include <QGraphicsObject>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include "factory/elementfactory.h"
#include "diagrampropertiesdialog.h"
#include "dveventinterface.h"
#include "diagrameventaddelement.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"

/**
	Constructeur
	@param diagram Schema a afficher ; si diagram vaut 0, un nouveau Diagram est utilise
	@param parent Le QWidget parent de cette vue de schema
*/
DiagramView::DiagramView(Diagram *diagram, QWidget *parent) :
	QGraphicsView     (parent),
	scene             (diagram),
	m_event_interface (nullptr),
	m_first_activation (true)
{
	grabGesture(Qt::PinchGesture);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setInteractive(true);

	QString whatsthis = tr(
		"Ceci est la zone dans laquelle vous concevez vos schémas en y ajoutant"
		" des éléments et en posant des conducteurs entre leurs bornes. Il est"
		" également possible d'ajouter des textes indépendants.",
		"\"What's this?\" tip"
	);
	setWhatsThis(whatsthis);

	// active l'antialiasing
	setRenderHint(QPainter::Antialiasing, true);
	setRenderHint(QPainter::TextAntialiasing, true);
	setRenderHint(QPainter::SmoothPixmapTransform, true);

	setScene(scene);
	scene -> undoStack().setClean();
	setWindowIcon(QET::Icons::QETLogo);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setResizeAnchor(QGraphicsView::AnchorUnderMouse);
	setAlignment(Qt::AlignLeft | Qt::AlignTop);
	setSelectionMode();
	adjustSceneRect();
	updateWindowTitle();

	context_menu = new QMenu(this);
	paste_here = new QAction(QET::Icons::EditPaste, tr("Coller ici", "context menu action"), this);
	connect(paste_here, SIGNAL(triggered()), this, SLOT(pasteHere()));

	connect(scene, SIGNAL(showDiagram(Diagram*)), this, SIGNAL(showDiagram(Diagram*)));
	connect(scene, SIGNAL(selectionChanged()), this, SIGNAL(selectionChanged()));
	connect(scene, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(adjustSceneRect()));
	connect(&(scene -> border_and_titleblock), SIGNAL(diagramTitleChanged(const QString &)), this, SLOT(updateWindowTitle()));
	connect(diagram, SIGNAL(editElementRequired(ElementsLocation)), this, SIGNAL(editElementRequired(ElementsLocation)));
	connect(diagram, SIGNAL(findElementRequired(ElementsLocation)), this, SIGNAL(findElementRequired(ElementsLocation)));

	connect(
		this, SIGNAL(aboutToSetDroppedTitleBlockTemplate(const TitleBlockTemplateLocation &)),
		this, SLOT(setDroppedTitleBlockTemplate(const TitleBlockTemplateLocation &)),
		Qt::QueuedConnection
	);
	QShortcut *edit_conductor_color_shortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
	connect(edit_conductor_color_shortcut, SIGNAL(activated()), this, SLOT(editSelectedConductorColor()));
}

/**
	Destructeur
*/
DiagramView::~DiagramView() {
}

/**
	Selectionne tous les objets du schema
*/
void DiagramView::selectAll() {
	scene -> selectAll();
}

/**
	Deslectionne tous les objets selectionnes
*/
void DiagramView::selectNothing() {
	scene -> deselectAll();
}

/**
	Inverse l'etat de selection de tous les objets du schema
*/
void DiagramView::selectInvert() {
	scene -> invertSelection();
}

/**
	Supprime les composants selectionnes
*/
void DiagramView::deleteSelection() {
	if (scene -> isReadOnly()) return;
	DiagramContent removed_content = scene -> selectedContent();
	scene -> clearSelection();
	scene -> undoStack().push(new DeleteElementsCommand(scene, removed_content));
	adjustSceneRect();
}

/**
	Pivote les composants selectionnes
*/
void DiagramView::rotateSelection() {
	if (scene -> isReadOnly()) return;

	// recupere les elements et les champs de texte a pivoter
	QList<Element *> elements_to_rotate;
	QList<DiagramTextItem *> texts_to_rotate;
	QList<DiagramImageItem *> images_to_rotate;
	foreach (QGraphicsItem *item, scene -> selectedItems()) {
		if (Element *e = qgraphicsitem_cast<Element *>(item)) {
			elements_to_rotate << e;
		} else if (ConductorTextItem *cti = qgraphicsitem_cast<ConductorTextItem *>(item)) {
			texts_to_rotate << cti;
		} else if (IndependentTextItem *iti = qgraphicsitem_cast<IndependentTextItem *>(item)) {
			texts_to_rotate << iti;
		} else if (ElementTextItem *eti = qgraphicsitem_cast<ElementTextItem *>(item)) {
			// on ne pivote un texte d'element que si son parent n'est pas selectionne
			if (eti -> parentItem() && !eti -> parentItem() -> isSelected()) {
				texts_to_rotate << eti;
			}
		} else if (DiagramImageItem *dii = qgraphicsitem_cast<DiagramImageItem *>(item)) {
			images_to_rotate << dii;
		}
	}

	// effectue les rotations s'il y a quelque chose a pivoter
	if (elements_to_rotate.isEmpty() && texts_to_rotate.isEmpty() && images_to_rotate.isEmpty()) return;
	scene -> undoStack().push(new RotateElementsCommand(elements_to_rotate, texts_to_rotate, images_to_rotate));
}

void DiagramView::rotateTexts() {
	if (scene -> isReadOnly()) return;

	// recupere les champs de texte a orienter
	QList<DiagramTextItem *> texts_to_rotate;
	foreach (QGraphicsItem *item, scene -> selectedItems()) {
		if (ConductorTextItem *cti = qgraphicsitem_cast<ConductorTextItem *>(item)) {
			texts_to_rotate << cti;
		} else if (IndependentTextItem *iti = qgraphicsitem_cast<IndependentTextItem *>(item)) {
			texts_to_rotate << iti;
		} else if (ElementTextItem *eti = qgraphicsitem_cast<ElementTextItem *>(item)) {
			// ici, on pivote un texte d'element meme si son parent est selectionne
			texts_to_rotate << eti;
		}
	}

	// effectue les rotations s'il y a quelque chose a pivoter
	if (texts_to_rotate.isEmpty()) return;

	// demande un angle a l'utilisateur
	QDialog ori_text_dialog(diagramEditor());
	ori_text_dialog.setSizeGripEnabled(false);
#ifdef Q_OS_MAC
	ori_text_dialog.setWindowFlags(Qt::Sheet);
#endif
	ori_text_dialog.setWindowTitle(tr("Orienter les textes sélectionnés", "window title"));
// 	ori_text_dialog.setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);


	QTextOrientationSpinBoxWidget *ori_widget = QETApp::createTextOrientationSpinBoxWidget();
	ori_widget -> setParent(&ori_text_dialog);
	if (texts_to_rotate.count() == 1) {
		ori_widget -> setOrientation(texts_to_rotate.at(0) -> rotationAngle());
	}
	ori_widget -> spinBox() -> selectAll();

	// boutons
	QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(&buttons, SIGNAL(accepted()), &ori_text_dialog, SLOT(accept()));
	connect(&buttons, SIGNAL(rejected()), &ori_text_dialog, SLOT(reject()));

	// ajout dans une disposition verticale
	QVBoxLayout layout_v(&ori_text_dialog);
	layout_v.setSizeConstraint(QLayout::SetFixedSize);
	layout_v.addWidget(ori_widget);
	layout_v.addStretch();
	layout_v.addWidget(&buttons);

	// si le dialogue est accepte
	if (ori_text_dialog.exec() == QDialog::Accepted) {
		scene -> undoStack().push(new RotateTextsCommand(texts_to_rotate, ori_widget -> orientation()));
	}
}

/**
	Accepte ou refuse le drag'n drop en fonction du type de donnees entrant
	@param e le QDragEnterEvent correspondant au drag'n drop tente
*/
void DiagramView::dragEnterEvent(QDragEnterEvent *e) {
	if (e -> mimeData() -> hasFormat("application/x-qet-element-uri")) {
		e -> acceptProposedAction();
	} else if (e -> mimeData() -> hasFormat("application/x-qet-titleblock-uri")) {
		e -> acceptProposedAction();
	} else if (e -> mimeData() -> hasText()) {
		e -> acceptProposedAction();
	} else {
		e -> ignore();
	}
}

/**
	Gere les dragleave
	@param e le QDragEnterEvent correspondant au drag'n drop sortant
*/
void DiagramView::dragLeaveEvent(QDragLeaveEvent *e) {
	Q_UNUSED(e);
}

/**
	Accepte ou refuse le drag'n drop en fonction du type de donnees entrant
	@param e le QDragMoveEvent correspondant au drag'n drop tente
*/
void DiagramView::dragMoveEvent(QDragMoveEvent *e) {
	if (e -> mimeData() -> hasFormat("text/plain")) e -> acceptProposedAction();
	else e-> ignore();
}

/**
	Handle the drops accepted on diagram (elements and title block templates).
	@param e the QDropEvent describing the current drag'n drop
*/
void DiagramView::dropEvent(QDropEvent *e) {

	if (e -> mimeData() -> hasFormat("application/x-qet-element-uri")) {
		handleElementDrop(e);
	} else if (e -> mimeData() -> hasFormat("application/x-qet-titleblock-uri")) {
		handleTitleBlockDrop(e);
	} else if (e -> mimeData() -> hasText()) {
		handleTextDrop(e);
	}
}

/**
	Handle the drop of an element.
	@param e the QDropEvent describing the current drag'n drop
*/
void DiagramView::handleElementDrop(QDropEvent *event)
{
		//Build an element from the text of the mime data
	ElementsLocation location(event->mimeData()->text());

	if ( !(location.isElement() && location.exist()) )
	{
		qDebug() << "DiagramView::handleElementDrop, location can't be use : " << location;
		return;
	}

	diagram()->setEventInterface(new DiagramEventAddElement(location, diagram(), mapToScene(event->pos())));
		//Set focus to the view to get event
	this->setFocus();
}

/**
	Handle the drop of an element.
	@param e the QDropEvent describing the current drag'n drop
*/
void DiagramView::handleTitleBlockDrop(QDropEvent *e) {
	// fetch the title block template location from the drop event
	TitleBlockTemplateLocation tbt_loc;
	tbt_loc.fromString(e -> mimeData() -> text());
	if (tbt_loc.isValid()) {
		emit(aboutToSetDroppedTitleBlockTemplate(tbt_loc));
	}
}

/**
 * @brief DiagramView::handleTextDrop
 *handle the drop of text
 * @param e the QDropEvent describing the current drag'n drop
 */
void DiagramView::handleTextDrop(QDropEvent *e) {
	if (scene -> isReadOnly() || (e -> mimeData() -> hasText() == false) ) return;

	IndependentTextItem *iti = new IndependentTextItem (e -> mimeData() -> text());

	if (e -> mimeData() -> hasHtml()) {
		iti -> setHtml (e -> mimeData() -> text());
	}

	scene -> undoStack().push(new AddItemCommand<IndependentTextItem *>(iti, scene, mapToScene(e->pos())));
}

/**
	Set the Diagram in visualisation mode
*/
void DiagramView::setVisualisationMode() {
	setDragMode(ScrollHandDrag);
	applyReadOnly();
	setInteractive(false);
	emit(modeChanged());
}

/**
	Set the Diagram in Selection mode
*/
void DiagramView::setSelectionMode() {
	setDragMode(RubberBandDrag);
	setInteractive(true);
	applyReadOnly();
	emit(modeChanged());
}

/**
 * @brief DiagramView::zoom
 * Zomm the view.
 * A zoom_factor > 1 zoom in.
 * A zoom_factor < 1 zoom out
 * @param zoom_factor
 */
void DiagramView::zoom(const qreal zoom_factor)
{
	if (zoom_factor >= 1){
		scale(zoom_factor, zoom_factor);
	}
	else
	{
		QSettings settings;
		if (settings.value("diagrameditor/zoom-out-beyond-of-folio", false).toBool() ||
			(horizontalScrollBar()->maximum() || verticalScrollBar()->maximum()) )
			scale(zoom_factor, zoom_factor);
	}
	scene->adjustSceneRect();
	adjustGridToZoom();
}

/**
	Agrandit ou rectrecit le schema de facon a ce que tous les elements du
	schema soient visibles a l'ecran. S'il n'y a aucun element sur le schema,
	le zoom est reinitialise
*/
void DiagramView::zoomFit() {
	adjustSceneRect();
	fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
	adjustGridToZoom();
}

/**
	Adjust zoom to fit all elements in the view, regardless of diagram borders.
*/
void DiagramView::zoomContent() {
	fitInView(scene -> itemsBoundingRect(), Qt::KeepAspectRatio);
	adjustGridToZoom();
}

/**
	Reinitialise le zoom
*/
void DiagramView::zoomReset() {
	resetMatrix();
	adjustGridToZoom();
}

/**
	Copie les elements selectionnes du schema dans le presse-papier puis les supprime
*/
void DiagramView::cut() {
	copy();
	DiagramContent cut_content = scene -> selectedContent();
	scene -> clearSelection();
	scene -> undoStack().push(new CutDiagramCommand(scene, cut_content));
}

/**
	Copie les elements selectionnes du schema dans le presse-papier
*/
void DiagramView::copy() {
	QClipboard *presse_papier = QApplication::clipboard();
	QString contenu_presse_papier = scene -> toXml(false).toString(4);
	if (presse_papier -> supportsSelection()) presse_papier -> setText(contenu_presse_papier, QClipboard::Selection);
	presse_papier -> setText(contenu_presse_papier);
}

/**
	Importe les elements contenus dans le presse-papier dans le schema
	@param pos coin superieur gauche (en coordonnees de la scene) du rectangle
	englobant le contenu colle
	@param clipboard_mode Type de presse-papier a prendre en compte
*/
void DiagramView::paste(const QPointF &pos, QClipboard::Mode clipboard_mode) {
	if (!isInteractive() || scene -> isReadOnly()) return;

	QString texte_presse_papier = QApplication::clipboard() -> text(clipboard_mode);
	if ((texte_presse_papier).isEmpty()) return;

	QDomDocument document_xml;
	if (!document_xml.setContent(texte_presse_papier)) return;

	// objet pour recuperer le contenu ajoute au schema par le coller
	DiagramContent content_pasted;
	scene -> fromXml(document_xml, pos, false, &content_pasted);

	// si quelque chose a effectivement ete ajoute au schema, on cree un objet d'annulation
	if (content_pasted.count()) {
		scene -> clearSelection();
		scene -> undoStack().push(new PasteDiagramCommand(scene, content_pasted));
		adjustSceneRect();
	}
}

/**
	Colle le contenu du presse-papier sur le schema a la position de la souris
*/
void DiagramView::pasteHere() {
	paste(mapToScene(paste_here_pos));
}

/**
	Manage the events press click :
	 *  click to add an independent text field
*/
void DiagramView::mousePressEvent(QMouseEvent *e)
{
	if (fresh_focus_in_)
	{
		switchToVisualisationModeIfNeeded(e);
		fresh_focus_in_ = false;
	}

	if (m_event_interface && m_event_interface->mousePressEvent(e)) return;

		//Start drag view when hold the middle button
	if (e->button() == Qt::MidButton)
	{
		rubber_band_origin = e->pos();
		viewport()->setCursor(Qt::ClosedHandCursor);
	}

	else QGraphicsView::mousePressEvent(e);
}

/**
 * @brief DiagramView::mouseMoveEvent
 * Manage the event move mouse
 */
void DiagramView::mouseMoveEvent(QMouseEvent *e)
{
	if (m_event_interface && m_event_interface->mouseMoveEvent(e)) return;

		//Drag the view
	if (e->buttons() == Qt::MidButton)
	{
		QScrollBar *h = horizontalScrollBar();
		QScrollBar *v = verticalScrollBar();
		QPointF pos = rubber_band_origin - e -> pos();
		rubber_band_origin = e -> pos();
		h -> setValue(h -> value() + pos.x());
		v -> setValue(v -> value() + pos.y());
	}

	else QGraphicsView::mouseMoveEvent(e);
}

/**
 * @brief DiagramView::mouseReleaseEvent
 * Manage event release click mouse
 */
void DiagramView::mouseReleaseEvent(QMouseEvent *e)
{
	if (m_event_interface && m_event_interface->mouseReleaseEvent(e)) return;

	//Stop drag view
	if (e -> button() == Qt::MidButton) viewport()->setCursor(Qt::ArrowCursor);

	else QGraphicsView::mouseReleaseEvent(e);
}

/**
 * @brief DiagramView::gestures
 * @return
 */
bool DiagramView::gestures() const
{
	QSettings settings;
	return(settings.value("diagramview/gestures", false).toBool());
}

/**
	Manage wheel event of mouse
	@param e QWheelEvent
*/
void DiagramView::wheelEvent(QWheelEvent *event)
{
	if (m_event_interface && m_event_interface->wheelEvent(event)) return;

		//Zoom and scrolling
	QPoint angle = event->angleDelta();

	if (gestures()) //When gesture mode is enable, we suppose the wheel event are made from a trackpad.
	{
		if (event->modifiers() == Qt::ControlModifier) //zoom
		{
			qreal value = angle.y();
			zoom(1 + value/1000);
		}
		else //scroll
		{
			horizontalScrollBar()->setValue(horizontalScrollBar()->value() - angle.x());
			verticalScrollBar()->setValue(verticalScrollBar()->value() - angle.y());
		}
	}
	else if (event->modifiers() == Qt::NoModifier) //Else we suppose the wheel event are made from a mouse.
	{
		qreal value = angle.y();
		zoom(1 + value/1000);
	}
	else
		QGraphicsView::wheelEvent(event);
}

/**
 * @brief DiagramView::gestureEvent
 * Use the pinch of the trackpad for zoom
 * @param event
 * @return
 */
bool DiagramView::gestureEvent(QGestureEvent *event)
{
	if (QGesture *gesture = event->gesture(Qt::PinchGesture))
	{
		QPinchGesture *pinch = static_cast<QPinchGesture *>(gesture);
		if (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged)
		{
			qreal value = gesture->property("scaleFactor").toReal();
			value > 1 ? zoom(1.02) : zoom(0.98);
		}
	}
	return true;
}



/**
	Handles "Focus in" events. Reimplemented here to store the fact the focus
	was freshly acquired again using the mouse. This information is later used
	in DiagramView::mousePressEvent().
*/
void DiagramView::focusInEvent(QFocusEvent *e) {
	if (e -> reason() == Qt::MouseFocusReason) {
		fresh_focus_in_ = true;
	}
}

/**
	Handles "key press" events. Reimplemented here to switch to visualisation
	mode if needed.
*/
void DiagramView::keyPressEvent(QKeyEvent *e) {
	if (m_event_interface && m_event_interface->keyPressEvent(e)) return;
	ProjectView *current_project = this->diagramEditor()->acessCurrentProject();
switch(e -> key())
	{
		case Qt::Key_PageUp:
			current_project->changeTabUp();
			return;
		case Qt::Key_PageDown:
			current_project->changeTabDown();
			return;
		case Qt::Key_Home:
			current_project->changeFirstTab();
			return;
		case Qt::Key_End:
			current_project->changeLastTab();
			return;
		case Qt::Key_Minus:
			zoom(0.85);
			return;
		case Qt::Key_Plus:
			zoom(1.15);
			return;
		case Qt::Key_Up:
			if(!scene->selectedContent().elements.isEmpty()){
				scrollOnMovement(e);
			}
		case Qt::Key_Down:
			if(!scene->selectedContent().elements.isEmpty()){
				scrollOnMovement(e);
			}
		case Qt::Key_Left:
			if(!scene->selectedContent().elements.isEmpty()){
				scrollOnMovement(e);
			}
		case Qt::Key_Right:
			if(!scene->selectedContent().elements.isEmpty()){
				scrollOnMovement(e);
			}
	}
	switchToVisualisationModeIfNeeded(e);
	QGraphicsView::keyPressEvent(e);
}

/**
	Handles "key release" events. Reimplemented here to switch to selection
	mode if needed.
*/
void DiagramView::keyReleaseEvent(QKeyEvent *e) {
	if (m_event_interface && m_event_interface->KeyReleaseEvent(e)) return;

	switchToSelectionModeIfNeeded(e);
	QGraphicsView::keyReleaseEvent(e);
}

/**
	Handles element movement when editor is zoomed in and scrolls vertical
	and horizontal bar. If element is moved to the right side of the editor
	or below the editor SceneRect is expanded
*/
void DiagramView::scrollOnMovement(QKeyEvent *e){
			QList<QGraphicsItem *> selected_elmts = scene ->selectedItems();
			QRectF viewed_scene = viewedSceneRect();
			foreach (QGraphicsItem *qgi, selected_elmts){
				qreal x = qgi->pos().x();
				qreal y = qgi->pos().y();
				qreal bottom = viewed_scene.bottom();
				qreal top = viewed_scene.top();
				qreal left = viewed_scene.left();
				qreal right = viewed_scene.right();
				qreal elmt_top = y + qgi->boundingRect().top();
				qreal elmt_bottom = y + qgi->boundingRect().bottom();
				qreal elmt_right = x + qgi->boundingRect().right();
				qreal elmt_left = x + qgi->boundingRect().left();
				bool elmt_right_of_left_margin = elmt_left>=left;
				bool elmt_left_of_right_margin = elmt_right<=right;
				bool elmt_below_top_margin = elmt_top>=top;
				bool elmt_above_bottom_margin = elmt_bottom<=bottom;
				if (!(elmt_right_of_left_margin && elmt_left_of_right_margin) ||
					!(elmt_below_top_margin    && elmt_above_bottom_margin )  ) {
						QScrollBar *h = horizontalScrollBar();
						QScrollBar *v = verticalScrollBar();
						int h_increment=0;
						int v_increment=0;
						if (e->key()==Qt::Key_Up && elmt_above_bottom_margin)
							v_increment = 2*qgi->boundingRect().top();
						else if(e->key()==Qt::Key_Down && elmt_below_top_margin){
							v_increment = 2*qgi->boundingRect().bottom();
						}
						else if (e->key()==Qt::Key_Left && elmt_left_of_right_margin)
							h_increment = 2*qgi->boundingRect().left();
						else if (e->key()==Qt::Key_Right && elmt_right_of_left_margin){
							h_increment = 2*qgi->boundingRect().right();
						}
						if (((elmt_right  >= scene->sceneRect().right() -  qgi->boundingRect().right())  ||
							(elmt_bottom >= scene->sceneRect().bottom() - qgi->boundingRect().bottom())) &&
							(e->key()==Qt::Key_Right || e->key()==Qt::Key_Down)){
							scene->adjustSceneRect();
						}
						h -> setValue(h -> value() + h_increment);
						v -> setValue(v -> value() + v_increment);
				}
			}
}


/**
	@return le titre de cette vue ; cela correspond au titre du schema
	visualise precede de la mention "Schema". Si le titre du schema est vide,
	la mention "Schema sans titre" est utilisee
	@see Diagram::title()
*/
QString DiagramView::title() const {
	QString view_title;
	QString diagram_title(scene -> title());
	if (diagram_title.isEmpty()) {
		view_title = tr("Sans titre", "what to display for untitled diagrams");
	} else {
		view_title = diagram_title;
	}
	return(view_title);
}

/**
 * @brief DiagramView::editDiagramProperties
 * Edit the properties of the viewed digram
 */
void DiagramView::editDiagramProperties() {
	DiagramPropertiesDialog::diagramPropertiesDialog(scene, diagramEditor());
}

/**
	@return true s'il y a des items selectionnes sur le schema, false sinon
*/
bool DiagramView::hasSelectedItems() {
	return(scene -> selectedItems().size() > 0);
}

/**
	@return true s'il y a des items selectionnes sur le schema et que ceux-ci
	peuvent etre copies dans le presse-papier, false sinon
*/
bool DiagramView::hasCopiableItems() {
	foreach(QGraphicsItem *qgi, scene -> selectedItems()) {
		if (
			qgraphicsitem_cast<Element *>(qgi) ||
			qgraphicsitem_cast<IndependentTextItem *>(qgi) ||
			qgraphicsitem_cast<QetShapeItem *>(qgi) ||
			qgraphicsitem_cast<DiagramImageItem *>(qgi)
		) {
			return(true);
		}
	}
	return(false);
}

/**
	@return true s'il y a des items selectionnes sur le schema et que ceux-ci
	peuvent etre supprimes, false sinon
*/
bool DiagramView::hasDeletableItems() {
	foreach(QGraphicsItem *qgi, scene -> selectedItems()) {
		if (
			qgraphicsitem_cast<Element *>(qgi) ||
			qgraphicsitem_cast<Conductor *>(qgi) ||
			qgraphicsitem_cast<IndependentTextItem *>(qgi) ||
			qgraphicsitem_cast<QetShapeItem *>(qgi) ||
			qgraphicsitem_cast<DiagramImageItem *>(qgi)
		) {
			return(true);
		}
	}
	return(false);
}

/**
	Ajoute une colonne au schema.
*/
void DiagramView::addColumn() {
	if (scene -> isReadOnly()) return;
	BorderProperties old_bp = scene -> border_and_titleblock.exportBorder();
	BorderProperties new_bp = scene -> border_and_titleblock.exportBorder();
	new_bp.columns_count += 1;
	scene -> undoStack().push(new ChangeBorderCommand(scene, old_bp, new_bp));
}

/**
	Enleve une colonne au schema.
*/
void DiagramView::removeColumn() {
	if (scene -> isReadOnly()) return;
	BorderProperties old_bp = scene -> border_and_titleblock.exportBorder();
	BorderProperties new_bp = scene -> border_and_titleblock.exportBorder();
	new_bp.columns_count -= 1;
	scene -> undoStack().push(new ChangeBorderCommand(scene, old_bp, new_bp));
}

/**
	Agrandit le schema en hauteur
*/
void DiagramView::addRow() {
	if (scene -> isReadOnly()) return;
	BorderProperties old_bp = scene -> border_and_titleblock.exportBorder();
	BorderProperties new_bp = scene -> border_and_titleblock.exportBorder();
	new_bp.rows_count += 1;
	scene -> undoStack().push(new ChangeBorderCommand(scene, old_bp, new_bp));
}

/**
	Retrecit le schema en hauteur
*/
void DiagramView::removeRow() {
	if (scene -> isReadOnly()) return;
	BorderProperties old_bp = scene -> border_and_titleblock.exportBorder();
	BorderProperties new_bp = scene -> border_and_titleblock.exportBorder();
	new_bp.rows_count -= 1;
	scene -> undoStack().push(new ChangeBorderCommand(scene, old_bp, new_bp));
}

/**
 * @brief DiagramView::adjustSceneRect
 * Calcul and set the area of the scene visualized by this view
 * The area are diagram sceneRect * 2.
 */
void DiagramView::adjustSceneRect()
{
	QRectF scene_rect = scene->sceneRect();
	scene_rect.adjust(-Diagram::margin, -Diagram::margin, Diagram::margin, Diagram::margin);
	setSceneRect(scene_rect);
}

/**
	Met a jour le titre du widget
*/
void DiagramView::updateWindowTitle() {
	emit(titleChanged(this, title()));
}

/**
	Enables or disables the drawing grid according to the amount of pixels display
*/
void DiagramView::adjustGridToZoom() {
	QRectF viewed_scene = viewedSceneRect();
	scene -> setDisplayGrid(viewed_scene.width() < 2000 || viewed_scene.height() < 2000);
}

/**
	@return le rectangle du schema (classe Diagram) visualise par ce DiagramView
*/
QRectF DiagramView::viewedSceneRect() const {
	// recupere la taille du widget viewport
	QSize viewport_size = viewport() -> size();

	// recupere la transformation viewport -> scene
	QTransform view_to_scene   = viewportTransform().inverted();

	// mappe le coin superieur gauche et le coin inferieur droit de la viewport sur la scene
	QPointF scene_left_top     = view_to_scene.map(QPointF(0.0, 0.0));
	QPointF scene_right_bottom = view_to_scene.map(QPointF(viewport_size.width(), viewport_size.height()));

	// en deduit le rectangle visualise par la scene
	return(QRectF(scene_left_top, scene_right_bottom));
}

/**
	@param tbt_loc A title block template location
	@return true if the title block template needs to be integrated in the
	parent project before being applied to the current diagram, or false if it
	can be directly applied
*/
bool DiagramView::mustIntegrateTitleBlockTemplate(const TitleBlockTemplateLocation &tbt_loc) const {
	// unlike elements, the integration of title block templates is mandatory, so we simply check whether the parent project of the template is also the parent project of the diagram
	QETProject *tbt_parent_project = tbt_loc.parentProject();
	if (!tbt_parent_project) return(true);

	return(tbt_parent_project != scene -> project());
}

/**
	Fait en sorte que le schema ne soit editable que s'il n'est pas en lecture
	seule
*/
void DiagramView::applyReadOnly() {
	if (!scene) return;

	bool is_writable = !scene -> isReadOnly();
	setInteractive(is_writable);
	setAcceptDrops(is_writable);
}

/**
	Edite les proprietes des objets selectionnes
*/
void DiagramView::editSelectionProperties() {
	// get selection
	DiagramContent selection = scene -> selectedContent();

	// if selection contains nothing return
	int selected_items_count = selection.count(DiagramContent::All | DiagramContent::SelectedOnly);
	if (!selected_items_count) return;

	// if selection contains one item and this item can be editable, edit this item with an appropriate dialog
	if (selected_items_count == 1 && selection.items(DiagramContent::Elements |
													 DiagramContent::AnyConductor |
													 DiagramContent::SelectedOnly).size()) {
		// edit conductor
		if (selection.conductors(DiagramContent::AnyConductor | DiagramContent::SelectedOnly).size())
			selection.conductors().first()->editProperty();
		// edit element
		else if (selection.elements.size())
			selection.elements.toList().first() -> editProperty();
	}

	else {
		QET::QetMessageBox::information(
			this,
			tr("Propriétés de la sélection"),
			QString(
				tr(
					"La sélection contient %1.",
					"%1 is a sentence listing the selected objects"
				)
			).arg(selection.sentence(DiagramContent::All | DiagramContent::SelectedOnly))
		);
	}
}

/**
	Edit the color of the selected conductor; does nothing if multiple conductors are selected
*/
void DiagramView::editSelectedConductorColor() {
	// retrieve selected content
	DiagramContent selection = scene -> selectedContent();

	// we'll focus on the selected conductor (we do not handle multiple conductors edition)
	QList<Conductor *> selected_conductors = selection.conductors(DiagramContent::AnyConductor | DiagramContent::SelectedOnly);
	if (selected_conductors.count() == 1) {
		editConductorColor(selected_conductors.at(0));
	}
}

/**
	Edit the color of the given conductor
	@param edited_conductor Conductor we want to change the color
*/
void DiagramView::editConductorColor(Conductor *edited_conductor)
{
	if (scene -> isReadOnly() || !edited_conductor) return;

		// store the initial properties of the provided conductor
	ConductorProperties initial_properties = edited_conductor -> properties();

		// prepare a color dialog showing the initial conductor color
	QColorDialog *color_dialog = new QColorDialog(this);
	color_dialog -> setWindowTitle(tr("Choisir la nouvelle couleur de ce conducteur"));
#ifdef Q_OS_MAC
	color_dialog -> setWindowFlags(Qt::Sheet);
#endif
	color_dialog -> setCurrentColor(initial_properties.color);

		// asks the user what color he wishes to apply
	if (color_dialog -> exec() == QDialog::Accepted)
	{
		QColor new_color = color_dialog -> selectedColor();
		if (new_color != initial_properties.color)
		{
				// the user chose a different color
			QVariant old_value, new_value;
			old_value.setValue(initial_properties);
			initial_properties.color = new_color;
			new_value.setValue(initial_properties);

			QPropertyUndoCommand *undo = new QPropertyUndoCommand(edited_conductor, "properties", old_value, new_value);
			undo->setText(tr("Modifier les propriétés d'un conducteur", "undo caption"));
			diagram() -> undoStack().push(undo);
		}
	}
}

/**
	Reinitialise le profil des conducteurs selectionnes
*/
void DiagramView::resetConductors() {
	if (scene -> isReadOnly()) return;
	// recupere les conducteurs selectionnes
	QSet<Conductor *> selected_conductors = scene -> selectedConductors();

	// repere les conducteurs modifies (= profil non nul)
	QHash<Conductor *, ConductorProfilesGroup> conductors_and_profiles;
	foreach(Conductor *conductor, selected_conductors) {
		ConductorProfilesGroup profile = conductor -> profiles();
		if (
			!profile[Qt::TopLeftCorner].isNull() ||\
			!profile[Qt::TopRightCorner].isNull() ||\
			!profile[Qt::BottomLeftCorner].isNull() ||\
			!profile[Qt::BottomRightCorner].isNull()
		) {
			conductors_and_profiles.insert(conductor, profile);
		}
	}

	if (conductors_and_profiles.isEmpty()) return;
	scene -> undoStack().push(new ResetConductorCommand(conductors_and_profiles));
}

/**
	Gere les evenements de la DiagramView
	@param e Evenement
*/
/**
 * @brief DiagramView::event
 * Manage the event on this diagram view.
 * -At first activation (QEvent::WindowActivate or QEvent::Show) we zoomFit.
 * -Convert event interpreted to mouse event to gesture event if needed.
 * -send Shortcut to view (by default send to QMenu /QAction)
 * @param e the event.
 * @return
 */
bool DiagramView::event(QEvent *e) {
	if (Q_UNLIKELY(m_first_activation)) {
		if (e -> type() == QEvent::Show) {
			zoomFit();
			m_first_activation = false;
		}
	}
	// By default touch events are converted to mouse events. So
	// after this event we will get a mouse event also but we want
	// to handle touch events as gestures only. So we need this safeguard
	// to block mouse events that are actually generated from touch.
	if (e->type() == QEvent::Gesture)
		return gestureEvent(static_cast<QGestureEvent *>(e));

	// fait en sorte que les raccourcis clavier arrivent prioritairement sur la
	// vue plutot que de remonter vers les QMenu / QAction
	if (
		e -> type() == QEvent::ShortcutOverride &&
		selectedItemHasFocus()
	) {
		e -> accept();
		return(true);
	}
	return(QGraphicsView::event(e));
}

/**
	Switch to visualisation mode if the user is pressing Ctrl and Shift.
	@return true if the view was switched to visualisation mode, false
	otherwise.
*/
bool DiagramView::switchToVisualisationModeIfNeeded(QInputEvent *e) {
	if (isCtrlShifting(e) && !selectedItemHasFocus()) {
		if (dragMode() != QGraphicsView::ScrollHandDrag) {
			setVisualisationMode();
			return(true);
		}
	}
	return(false);
}

/**
	Switch back to selection mode if the user is not pressing Ctrl and Shift.
	@return true if the view was switched to selection mode, false
	otherwise.
*/
bool DiagramView::switchToSelectionModeIfNeeded(QInputEvent *e) {
	if (!selectedItemHasFocus() && !isCtrlShifting(e)) {
		setSelectionMode();
		return(true);
	}
	return(false);
}

/**
	@return true if the user is pressing Ctrl and Shift simultaneously.
*/
bool DiagramView::isCtrlShifting(QInputEvent *e) {
	bool result = false;
	// note: QInputEvent::modifiers and QKeyEvent::modifiers() do not return the
	// same values, hence the casts
	if (e -> type() == QEvent::KeyPress || e -> type() == QEvent::KeyRelease) {
		if (QKeyEvent *ke = static_cast<QKeyEvent *>(e)) {
			result = (ke -> modifiers() == (Qt::ControlModifier | Qt::ShiftModifier));
		}
	} else if (e -> type() >= QEvent::MouseButtonPress && e -> type() <= QEvent::MouseMove) {
		if (QMouseEvent *me = static_cast<QMouseEvent *>(e)) {
			result = (me -> modifiers() == (Qt::ControlModifier | Qt::ShiftModifier));
		}
	}
	return(result);
}

/**
	@return true if there is a selected item and that item has the focus.
*/
bool DiagramView::selectedItemHasFocus() {
	return(
		scene -> hasFocus() &&
		scene -> focusItem() &&
		scene -> focusItem() -> isSelected()
	);
}

/**
 * @brief DiagramView::editSelection
 * Edit the selected item if he can be edited and if only  one item is selected
 */
void DiagramView::editSelection() {
	if (scene -> isReadOnly() || scene -> selectedItems().size() != 1 ) return;

	QGraphicsItem *item = scene->selectedItems().first();

		//We use dynamic_cast instead of qgraphicsitem_cast for QetGraphicsItem
		//because they haven't got they own type().
		//Use qgraphicsitem_cast will have weird behavior for this class.
	if (IndependentTextItem *iti = qgraphicsitem_cast<IndependentTextItem *>(item))
		iti -> edit();
	else if (QetGraphicsItem *qgi = dynamic_cast<QetGraphicsItem *> (item))
		qgi -> editProperty();
	else if (Conductor *c = qgraphicsitem_cast<Conductor *>(item))
		c -> editProperty();
}

/**
 * @brief DiagramView::setEventInterface
 * Set an event interface to diagram view.
 * If diagram view already have an event interface, he delete it before.
 * Diagram view take ownership of event interface and delete it when event interface is finish
 */
void DiagramView::setEventInterface(DVEventInterface *event_interface)
{
	if (m_event_interface) delete m_event_interface;
	m_event_interface = event_interface;
	connect(m_event_interface, &DVEventInterface::finish, this, [=](){delete this->m_event_interface; this->m_event_interface = nullptr;}, Qt::QueuedConnection);
}

/**
	Gere le menu contextuel
	@param e Evenement decrivant la demande de menu contextuel
*/
void DiagramView::contextMenuEvent(QContextMenuEvent *e) {
	if (QGraphicsItem *qgi = scene -> itemAt(mapToScene(e -> pos()), transform())) {
		if (!qgi -> isSelected()) scene -> clearSelection();
		qgi -> setSelected(true);
	}

	if (QETDiagramEditor *qde = diagramEditor()) {
		context_menu -> clear();
		if (scene -> selectedItems().isEmpty()) {
			paste_here_pos = e -> pos();
			paste_here -> setEnabled(Diagram::clipboardMayContainDiagram());
			context_menu -> addAction(paste_here);
			context_menu -> addSeparator();
			context_menu -> addAction(qde -> infos_diagram);
			context_menu -> addActions(qde -> m_row_column_actions_group.actions());
		} else {
			context_menu -> addAction(qde -> cut);
			context_menu -> addAction(qde -> copy);
			context_menu -> addSeparator();
			context_menu -> addAction(qde -> conductor_reset);
			context_menu -> addSeparator();
			context_menu -> addActions(qde -> m_selection_actions_group.actions());
		}

		// affiche le menu contextuel
		context_menu -> popup(e -> globalPos());
	}
	e -> accept();
}

/**
	@return l'editeur de schemas parent ou 0
*/
QETDiagramEditor *DiagramView::diagramEditor() const {
	// remonte la hierarchie des widgets
	QWidget *w = const_cast<DiagramView *>(this);
	while (w -> parentWidget() && !w -> isWindow()) {
		w = w -> parentWidget();
	}
	// la fenetre est supposee etre un QETDiagramEditor
	return(qobject_cast<QETDiagramEditor *>(w));
}

/**
 * @brief DiagramView::mouseDoubleClickEvent
 * @param e
 */
void DiagramView::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (m_event_interface && m_event_interface -> mouseDoubleClickEvent(e)) return;

	BorderTitleBlock &bi = scene -> border_and_titleblock;

	//Get the click pos on the diagram
	QPointF click_pos = viewportTransform().inverted().map(e -> pos());

	if (bi.titleBlockRect().contains(click_pos) || bi.columnsRect().contains(click_pos) || bi.rowsRect().contains(click_pos)) {
		e->accept();
		editDiagramProperties();
		return;
	}
	QGraphicsView::mouseDoubleClickEvent(e);
}

/**
	@param tbt TitleBlockTemplateLocation
*/
void DiagramView::setDroppedTitleBlockTemplate(const TitleBlockTemplateLocation &tbt) {
	// fetch the current title block properties
	TitleBlockProperties titleblock_properties_before = scene -> border_and_titleblock.exportTitleBlock();

	// check the provided template is not already applied
	QETProject *tbt_parent_project = tbt.parentProject();
	if (tbt_parent_project && tbt_parent_project == scene -> project()) {
		// same parent project and same name = same title block template
		if (tbt.name() == titleblock_properties_before.template_name) return;
	}

	// integrate the provided template into the project if needed
	QString integrated_template_name = tbt.name();
	if (mustIntegrateTitleBlockTemplate(tbt)) {
		IntegrationMoveTitleBlockTemplatesHandler *handler = new IntegrationMoveTitleBlockTemplatesHandler(this);
		//QString error_message;
		integrated_template_name = scene -> project() -> integrateTitleBlockTemplate(tbt, handler);
		if (integrated_template_name.isEmpty()) return;
	}

	// apply the provided title block template
	if (titleblock_properties_before.template_name == integrated_template_name) return;
	TitleBlockProperties titleblock_properties_after = titleblock_properties_before;
	titleblock_properties_after.template_name = integrated_template_name;
	scene -> undoStack().push(new ChangeTitleBlockCommand(scene, titleblock_properties_before, titleblock_properties_after));
	adjustSceneRect();
}
