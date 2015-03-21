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
#include "integrationmoveelementshandler.h"
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

/**
	Constructeur
	@param diagram Schema a afficher ; si diagram vaut 0, un nouveau Diagram est utilise
	@param parent Le QWidget parent de cette vue de schema
*/
DiagramView::DiagramView(Diagram *diagram, QWidget *parent) :
	QGraphicsView     (parent),
	scene             (diagram),
	m_event_interface (nullptr)
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
	connect(&(scene -> border_and_titleblock), SIGNAL(borderChanged(QRectF, QRectF)), this, SLOT(adjustSceneRect()));
	connect(&(scene -> border_and_titleblock), SIGNAL(displayChanged()),              this, SLOT(adjustSceneRect()));
	connect(&(scene -> border_and_titleblock), SIGNAL(diagramTitleChanged(const QString &)), this, SLOT(updateWindowTitle()));
	connect(diagram, SIGNAL(editElementRequired(ElementsLocation)), this, SIGNAL(editElementRequired(ElementsLocation)));
	connect(diagram, SIGNAL(findElementRequired(ElementsLocation)), this, SIGNAL(findElementRequired(ElementsLocation)));
	
	connect(this, SIGNAL(aboutToAddElement()), this, SLOT(addDroppedElement()), Qt::QueuedConnection);
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
	Gere les dragleaveevent
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
void DiagramView::handleElementDrop(QDropEvent *e) {
	// fetch the element location from the drop event
	QString elmt_path = e -> mimeData() -> text();
	
	ElementsLocation location(ElementsLocation::locationFromString(elmt_path));
	
	// verifie qu'il existe un element correspondant a cet emplacement
	ElementsCollectionItem *dropped_item = QETApp::collectionItem(location);
	if (!dropped_item) return;

	diagram()->setEventInterface(new DiagramEventAddElement(location, diagram(), mapToScene(e->pos())));
		//Set focus to the view to get event
	this->setFocus();
	
//	next_location_ = location;
//	next_position_ = e-> pos();
	
//	emit(aboutToAddElement());
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
	Agrandit le schema (+33% = inverse des -25 % de zoomMoins())
*/
void DiagramView::zoomIn() {
	scale(4.0/3.0, 4.0/3.0);
	adjustGridToZoom();
}

/**
	Retrecit le schema (-25% = inverse des +33 % de zoomPlus())
*/
void DiagramView::zoomOut() {
	scale(0.75, 0.75);
	if ((mapFromScene(0,0).rx() == 0) && (mapFromScene(0,0).ry() == 0)){
		fitInView(sceneRect(), Qt::KeepAspectRatio);
	}
	adjustGridToZoom();
}

/**
	Agrandit le schema avec le trackpad
*/
void DiagramView::zoomInSlowly() {
	scale(1.02, 1.02);
	adjustGridToZoom();
}

/**
	Retrecit le schema avec le trackpad
*/
void DiagramView::zoomOutSlowly() {
	scale(0.98, 0.98);
    // Interdit le dezoome plus grand que le folio
	if ((mapFromScene(0,0).rx() == 0) && (mapFromScene(0,0).ry() == 0)){
		fitInView(sceneRect(), Qt::KeepAspectRatio);
	}
	adjustGridToZoom();
}

/**
	Agrandit ou rectrecit le schema de facon a ce que tous les elements du
	schema soient visibles a l'ecran. S'il n'y a aucun element sur le schema,
	le zoom est reinitialise
*/
void DiagramView::zoomFit() {
	adjustSceneRect();
	fitInView(sceneRect(), Qt::KeepAspectRatio);
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
	Manage the events press click mouse :
	 *  click to add an independent text field
*/
void DiagramView::mousePressEvent(QMouseEvent *e) {

	if (fresh_focus_in_) {
		switchToVisualisationModeIfNeeded(e);
		fresh_focus_in_ = false;
	}

	if (m_event_interface) {
		if (m_event_interface -> mousePressEvent(e)) {
			if (m_event_interface->isFinish()) {
				emit (itemAdded());
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}
	}

	//Start drag view when hold the middle button
	if (e -> button() == Qt::MidButton) {
		rubber_band_origin = mapToScene(e -> pos());
		setCursor(Qt::ClosedHandCursor);
		center_view_ = mapToScene(this -> viewport() -> rect().center());
	}

	else QGraphicsView::mousePressEvent(e);
}

/**
 * @brief DiagramView::mouseMoveEvent
 * Manage the event move mouse
 */
void DiagramView::mouseMoveEvent(QMouseEvent *e) {

	if (m_event_interface) {
		if (m_event_interface -> mouseMoveEvent(e)) {
			if (m_event_interface->isFinish()) {
				emit (itemAdded());
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}
	}
	//Drag the view
	if (e -> buttons() == Qt::MidButton) {
		QPointF move = rubber_band_origin - mapToScene(e -> pos());
		this -> centerOn(center_view_ + move);
		center_view_ = mapToScene( this -> viewport() -> rect().center() );
	}

	else QGraphicsView::mouseMoveEvent(e);
}

/**
 * @brief DiagramView::mouseReleaseEvent
 * Manage event release click mouse
 */
void DiagramView::mouseReleaseEvent(QMouseEvent *e) {

	if (m_event_interface) {
		if (m_event_interface -> mouseReleaseEvent(e)) {
			if (m_event_interface->isFinish()) {
				emit (itemAdded());
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}
	}
	//Stop drag view
	if (e -> button() == Qt::MidButton) setCursor(Qt::ArrowCursor);

	else QGraphicsView::mouseReleaseEvent(e);
}

/**
 * @brief DiagramView::gestures
 * @return
 */
bool DiagramView::gestures() const {
	return(QETApp::settings().value("diagramview/gestures", false).toBool());
}

/**
	Manage wheel event of mouse
	@param e QWheelEvent
*/
void DiagramView::wheelEvent(QWheelEvent *e) {
	if (m_event_interface) {
		if (m_event_interface -> wheelEvent(e)) {
			if (m_event_interface->isFinish()) {
				emit (itemAdded());
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}
	}

	//Zoom and scrolling
	if ( gestures() ) {
		if (e -> modifiers() & Qt::ControlModifier)
			e -> delta() > 0 ? zoomInSlowly() : zoomOutSlowly();
		else
			QGraphicsView::wheelEvent(e);
	} else {
		e -> delta() > 0 ? zoomIn(): zoomOut();
	}
}


/**
 * Utilise le pincement du trackpad pour zoomer
 * @brief DiagramView::gestureEvent
 * @param event
 * @return
 */


bool DiagramView::gestureEvent(QGestureEvent *event){
	if (QGesture *gesture = event->gesture(Qt::PinchGesture)) {
		QPinchGesture *pinch = static_cast<QPinchGesture *>(gesture);
		if (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged){
			qreal value = gesture->property("scaleFactor").toReal();
			if (value > 1){
				zoomInSlowly();
			}else{
				zoomOutSlowly();
			}
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
	if (m_event_interface) {
		if (m_event_interface -> keyPressEvent(e)) {
			if (m_event_interface->isFinish()) {
				emit (itemAdded());
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
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
	if (m_event_interface) {
		if (m_event_interface -> KeyReleaseEvent(e)) {
			if (m_event_interface->isFinish()) {
				emit (itemAdded());
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}
	}
	switchToSelectionModeIfNeeded(e);
	QGraphicsView::keyReleaseEvent(e);
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
	Ajuste le sceneRect (zone du schema visualisee par le DiagramView) afin que
	celui inclut a la fois les elements dans et en dehors du cadre et le cadre
	lui-meme.
*/
void DiagramView::adjustSceneRect() {
	QRectF old_scene_rect = sceneRect();
	
	// rectangle delimitant l'ensemble des elements
	QRectF elements_bounding_rect = scene -> itemsBoundingRect();
	
	// rectangle contenant le cadre = colonnes + cartouche
	QRectF border_bounding_rect = scene -> border_and_titleblock.borderAndTitleBlockRect().adjusted(-Diagram::margin, -Diagram::margin, Diagram::margin, Diagram::margin);
	
	// ajuste la sceneRect
	QRectF new_scene_rect = elements_bounding_rect.united(border_bounding_rect);
	setSceneRect(new_scene_rect);
	
	// met a jour la scene
	scene -> update(old_scene_rect.united(new_scene_rect));
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
	Cette methode permet de determiner s'il faut ou non integrer au projet un
	element dont on connait l'emplacement.
	L'element droppe est integre a la collection du projet :
	  * s'il appartient a un autre projet, quelque soit la specification de
	  l'utilisateur a ce propos ;
	  * s'il appartient a la collection commune ou a la collection
	  personnelle ET que l'utilisateur a autorise l'integration automatique
	  des elements dans les projets.
	@param location Emplacement de l'element
	@return true si l'element doit etre integre, false sinon
	
*/
bool DiagramView::mustIntegrateElement(const ElementsLocation &location) const {
	// l'utilisateur a-t-il autorise l'integration automatique des elements dans les projets ?
	bool auto_integration_enabled = QETApp::settings().value("diagrameditor/integrate-elements", true).toBool();
	
	// l'element appartient-il a un projet et si oui, est-ce un autre projet ?
	bool elmt_from_project = location.project();
	bool elmt_from_another_project = elmt_from_project && location.project() != scene -> project();
	
	// faut-il integrer l'element ?
	bool must_integrate_element = (elmt_from_another_project || (auto_integration_enabled && !elmt_from_project));
	
	return(must_integrate_element);
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
	@param location Emplacement de l'element a ajouter sur le schema
	@param pos Position (dans les coordonnees de la vue) a laquelle l'element sera ajoute
*/
bool DiagramView::addElementAtPos(const ElementsLocation &location, const QPoint &pos) {
	// construit une instance de l'element correspondant a l'emplacement
	int state;
	Element *el = ElementFactory::Instance()->createElement(location, 0, &state);
	if (state) {
		delete el;
		return(false);
	}
	
	//Add element to diagram
	diagram() -> undoStack().push (new AddItemCommand<Element *>(el, diagram(), mapToScene(pos)));
	return(true);
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
void DiagramView::editConductorColor(Conductor *edited_conductor) {
	if (scene -> isReadOnly()) return;
	if (!edited_conductor) return;
	
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
	if (color_dialog -> exec() == QDialog::Accepted) {
		QColor new_color = color_dialog -> selectedColor();
		if (new_color != initial_properties.color) {
			// the user chose a different color
			ConductorProperties new_properties = initial_properties;
			new_properties.color = new_color;
			
			ChangeConductorPropertiesCommand *ccpc = new ChangeConductorPropertiesCommand(edited_conductor);
			ccpc -> setOldSettings(initial_properties);
			ccpc -> setNewSettings(new_properties);
			diagram() -> undoStack().push(ccpc);
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
bool DiagramView::event(QEvent *e) {
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
 */
void DiagramView::setEventInterface(DVEventInterface *event_interface) {
	if (m_event_interface) delete m_event_interface;
	m_event_interface = event_interface;
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
void DiagramView::mouseDoubleClickEvent(QMouseEvent *e) {

	if (m_event_interface) {
		if (m_event_interface -> mouseDoubleClickEvent(e)) {
			if (m_event_interface->isFinish()) {
				emit (itemAdded());
				delete m_event_interface; m_event_interface = nullptr;
			}
			return;
		}
	}

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
	Cette methode ajoute l'element designe par l'emplacement location a la
	position pos. Si necessaire, elle demande l'integration de l'element au
	projet.
	@see mustIntegrateElement
*/
void DiagramView::addDroppedElement() {
	ElementsLocation location = next_location_;
	QPoint pos = next_position_;
	
	if (!mustIntegrateElement(location)) {
		addElementAtPos(location, pos);
	} else {
		QString error_msg;
		IntegrationMoveElementsHandler *integ_handler = new IntegrationMoveElementsHandler(this);
		QString integ_path = scene -> project() -> integrateElement(location.toString(), integ_handler, error_msg);
		delete integ_handler;
		if (integ_path.isEmpty()) {
			qDebug() << "DiagramView::addDroppedElement : Impossible d'ajouter l'element. Motif : " << qPrintable(error_msg);
			return;
		}
		addElementAtPos(ElementsLocation::locationFromString(integ_path), pos);
	}
	adjustSceneRect();
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
