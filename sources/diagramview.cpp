/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "titleblockpropertieswidget.h"
#include "templatelocation.h"
#include "qetapp.h"
#include "qetproject.h"
#include "borderpropertieswidget.h"
#include "integrationmoveelementshandler.h"
#include "integrationmovetemplateshandler.h"
#include "qetdiagrameditor.h"
#include "qeticons.h"
#include "qetmessagebox.h"
#include "qtextorientationspinboxwidget.h"
#include <QGraphicsObject>
#include <ui/elementpropertieswidget.h>
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneMouseEvent>
#include "factory/elementfactory.h"


/**
	Constructeur
	@param diagram Schema a afficher ; si diagram vaut 0, un nouveau Diagram est utilise
	@param parent Le QWidget parent de cette vue de schema
*/
DiagramView::DiagramView(Diagram *diagram, QWidget *parent) : QGraphicsView(parent), newItem(0){
	setAttribute(Qt::WA_DeleteOnClose, true);
	setInteractive(true);
	current_behavior = noAction;

	QString whatsthis = tr(
		"Ceci est la zone dans laquelle vous concevez vos sch\351mas en y ajoutant"
		" des \351l\351ments et en posant des conducteurs entre leurs bornes. Il est"
		" \351galement possible d'ajouter des textes ind\351pendants.",
		"\"What's this?\" tip"
	);
	setWhatsThis(whatsthis);
	
	// active l'antialiasing
	setRenderHint(QPainter::Antialiasing, true);
	setRenderHint(QPainter::TextAntialiasing, true);
	setRenderHint(QPainter::SmoothPixmapTransform, true);
	
	scene = diagram ? diagram : new Diagram(this);
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
	connect(scene, SIGNAL(readOnlyChanged(bool)), this, SLOT(applyReadOnly()));
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
#ifdef Q_WS_MAC
	ori_text_dialog.setWindowFlags(Qt::Sheet);
#endif
	ori_text_dialog.setWindowTitle(tr("Orienter les textes s\351lectionn\351s", "window title"));
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
	
	next_location_ = location;
	next_position_ = e-> pos();
	
	emit(aboutToAddElement());
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
 *handle the drop of text, html markup are automatically detected.
 * @param e the QDropEvent describing the current drag'n drop
 */
void DiagramView::handleTextDrop(QDropEvent *e) {
	if (e -> mimeData() -> hasText()) {
		if (e -> mimeData() -> hasHtml()) addDiagramTextAtPos(e->pos()) -> setHtml(e -> mimeData() -> text());
		else addDiagramTextAtPos(e -> pos(), e -> mimeData() -> text());
	}
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
	rubber_band_origin = mapToScene(e -> pos());

	if (fresh_focus_in_) {
		switchToVisualisationModeIfNeeded(e);
		fresh_focus_in_ = false;
	}

	if (isInteractive() && !scene -> isReadOnly() && e -> buttons() == Qt::LeftButton) {
		switch (current_behavior) {
			case noAction:
				QGraphicsView::mousePressEvent(e);
				break;
			case addingText:
				addDiagramTextAtPos(rubber_band_origin);
				current_behavior = noAction;
				break;
			case addingImage:
				addDiagramImageAtPos(rubber_band_origin);
				current_behavior = noAction;
				break;
			case addingLine:
				newItem = new QetShapeItem(rubber_band_origin, rubber_band_origin, QetShapeItem::Line, false);
				scene -> addItem(newItem);
				break;
			case addingRectangle:
				newItem = new QetShapeItem(rubber_band_origin, rubber_band_origin, QetShapeItem::Rectangle);
				scene -> addItem(newItem);
				break;
			case addingEllipse:
				newItem = new QetShapeItem(rubber_band_origin, rubber_band_origin, QetShapeItem::Ellipse);
				scene -> addItem(newItem);
				break;
			case dragView:
				current_behavior = noAction;
				QGraphicsView::mousePressEvent(e);
				break;
			default:
				current_behavior = noAction;
				break;
		}
	}
	// workaround for drag view with hold wheel click and drag mouse
	// see also mouseMoveEvent() and mouseReleaseEvent()
	else if (e -> buttons() == Qt::MidButton) {
		setCursor(Qt::ClosedHandCursor);
		center_view_ = mapToScene(this -> viewport() -> rect()).boundingRect().center();
		return;
	}
	else QGraphicsView::mousePressEvent(e);
}

/**
 * @brief DiagramView::mouseMoveEvent
 * Manage the event move mouse
 */
void DiagramView::mouseMoveEvent(QMouseEvent *e) {
	if ((e -> buttons() & Qt::MidButton) == Qt::MidButton) {
		QPointF move = rubber_band_origin - mapToScene(e -> pos());
		this -> centerOn(center_view_ + move);
		center_view_ = mapToScene(this -> viewport() -> rect()).boundingRect().center();
		return;
	}
	else if (e -> buttons() == Qt::LeftButton && current_behavior & addingShape) {
		QRectF rec = QRectF(rubber_band_origin, mapToScene(e->pos())).normalized();
		scene ->removeItem(newItem);
		newItem -> setBoundingRect(rec);
		if (current_behavior == addingLine)
			newItem -> setLineAngle(rubber_band_origin != rec.topLeft() && rubber_band_origin != rec.bottomRight());
		scene ->addItem(newItem);
	}
	else QGraphicsView::mouseMoveEvent(e);
}

/**
 * @brief DiagramView::mouseReleaseEvent
 * Manage event release click mouse
 */
void DiagramView::mouseReleaseEvent(QMouseEvent *e) {
	if (e -> button() == Qt::MidButton) {
		setCursor(Qt::ArrowCursor);
		return;
	}
	else if (current_behavior & addingShape) {
		newItem -> setFullyBuilt(true);
		// place it to the good position with an undo command
		scene -> undoStack().push(new AddShapeCommand(scene, newItem, rubber_band_origin));
		adjustSceneRect();
		emit(itemAdded());
		current_behavior = noAction;
	}
	else QGraphicsView::mouseReleaseEvent(e);
}

/**
	Manage wheel event of mouse
	@param e QWheelEvent
*/
void DiagramView::wheelEvent(QWheelEvent *e) {
	//Zoom and scrolling
	if (e->buttons() != Qt::MidButton) {
		if (!(e -> modifiers() & Qt::ControlModifier)) {
			if (e -> delta() > 0){
				zoomIn();
			}
			else{
				zoomOut();
			}
		}
		else {
			QAbstractScrollArea::wheelEvent(e);
		}
	}
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
	switchToVisualisationModeIfNeeded(e);
	QGraphicsView::keyPressEvent(e);
}

/**
	Handles "key release" events. Reimplemented here to switch to selection
	mode if needed.
*/
void DiagramView::keyReleaseEvent(QKeyEvent *e) {
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
	Edite les informations du schema.
*/
void DiagramView::editDiagramProperties() {
	bool diagram_is_read_only = scene -> isReadOnly();
	
	// recupere le cartouche et les dimensions du schema
	TitleBlockProperties  titleblock  = scene -> border_and_titleblock.exportTitleBlock();
	BorderProperties border = scene -> border_and_titleblock.exportBorder();
	ConductorProperties conductors = scene -> defaultConductorProperties;
	
	// construit le dialogue
	QDialog popup(diagramEditor());
	popup.setWindowModality(Qt::WindowModal);
#ifdef Q_WS_MAC
	popup.setWindowFlags(Qt::Sheet);
#endif
	
	popup.setWindowTitle(tr("Propri\351t\351s du sch\351ma", "window title"));
	
	BorderPropertiesWidget *border_infos = new BorderPropertiesWidget(border, &popup);
	border_infos -> setReadOnly(diagram_is_read_only);
	
	TitleBlockPropertiesWidget  *titleblock_infos  = new TitleBlockPropertiesWidget(titleblock, false, &popup);
	if (QETProject *parent_project = scene -> project()) {
		titleblock_infos -> setTitleBlockTemplatesCollection(parent_project -> embeddedTitleBlockTemplatesCollection());
		titleblock_infos -> setTitleBlockTemplatesVisible(true);
		// we have to parse again the TitleBlockProperties object, since the
		// first parsing did not know of our templates
		titleblock_infos -> setTitleBlockProperties(titleblock);
		// relay the signal that requires a title block template edition
		connect(titleblock_infos, SIGNAL(editTitleBlockTemplate(QString, bool)), this, SIGNAL(editTitleBlockTemplate(QString, bool)));
	}
	titleblock_infos -> setReadOnly(diagram_is_read_only);
	
	ConductorPropertiesWidget *cpw = new ConductorPropertiesWidget(conductors);
	cpw -> setReadOnly(diagram_is_read_only);
	
	// boutons
	QDialogButtonBox boutons(diagram_is_read_only ? QDialogButtonBox::Ok : QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(&boutons, SIGNAL(accepted()), &popup, SLOT(accept()));
	connect(&boutons, SIGNAL(rejected()), &popup, SLOT(reject()));
	
	// usual layout for these three widgets
	QHBoxLayout *hlayout1 = new QHBoxLayout();
	QVBoxLayout *vlayout2 = new QVBoxLayout();
	
	vlayout2 -> addWidget(border_infos);
	vlayout2 -> addWidget(titleblock_infos);
	vlayout2 -> setSpacing(5);
	hlayout1 -> addLayout(vlayout2);
	hlayout1 -> addWidget(cpw);
	hlayout1 -> setAlignment(cpw, Qt::AlignTop);
	
	// ajout dans une disposition verticale
	QVBoxLayout layout_v(&popup);
	layout_v.addLayout(hlayout1);
	layout_v.addStretch();
	layout_v.addWidget(&boutons);


	// si le dialogue est accepte
	if (popup.exec() == QDialog::Accepted && !diagram_is_read_only) {
		TitleBlockProperties new_titleblock   = titleblock_infos  -> titleBlockProperties();
		BorderProperties new_border = border_infos -> borderProperties();
		ConductorProperties new_conductors = cpw -> conductorProperties();
		
		bool adjust_scene = false;
		
		// s'il y a des modifications au cartouche
		if (new_titleblock != titleblock) {
			scene -> undoStack().push(new ChangeTitleBlockCommand(scene, titleblock, new_titleblock));
			adjust_scene = true;
		}
		
		// s'il y a des modifications aux dimensions du schema
		if (new_border != border) {
			scene -> undoStack().push(new ChangeBorderCommand(scene, border, new_border));
			adjust_scene = true;
		}
		
		// if modifcations have been made to the conductors properties
		if (new_conductors != conductors) {
			/// TODO implement an undo command to allow the user to undo/redo this action
			scene -> defaultConductorProperties = new_conductors;
		}
		// adjustSceneRect shall be called whenever the user accepts the dialog
		// even if no changes have been made.
		// Added so that diagram refreshes after back-ground color change.
		//if (adjust_scene)
			adjustSceneRect();
	}
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
	QRectF border_bounding_rect = scene -> border().adjusted(-Diagram::margin, -Diagram::margin, Diagram::margin, Diagram::margin);
	
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
	Element *el = ElementFactory::Instance()->createElement(location, 0, diagram(), &state);
	if (state) {
		delete el;
		return(false);
	}
	
	// pose de l'element sur le schema
	diagram() -> undoStack().push(new AddElementCommand(diagram(), el, mapToScene(pos)));
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
			editConductor(selection.conductors().first());
		// edit element
		else if (selection.elements.size())
			selection.elements.toList().first() -> editProperty();
	}
	
	else {
		QET::MessageBox::information(
			this,
			tr("Propri\351t\351s de la s\351lection"),
			QString(
				tr(
					"La s\351lection contient %1.",
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
	Affiche un dialogue permettant d'editer le conducteur selectionne.
	Ne fait rien s'il y a 0 ou plusieurs conducteurs selectionnes.
*/
void DiagramView::editConductor() {
	QList<Conductor *> selected_conductors(scene -> selectedConductors().toList());
	
	// on ne peut editer qu'un conducteur a la fois
	if (selected_conductors.count() != 1) return;
	Conductor *edited_conductor = selected_conductors.first();
	
	editConductor(edited_conductor);
}

/**
	Edite le conducteur passe en parametre
	@param edited_conductor Conducteur a editer
*/
void DiagramView::editConductor(Conductor *edited_conductor) {
	if (scene -> isReadOnly() || !edited_conductor) return;
	
	// initialise l'editeur de proprietes pour le conducteur
	ConductorProperties old_properties = edited_conductor -> properties();
	ConductorPropertiesWidget *cpw = new ConductorPropertiesWidget(old_properties);
	
	// l'insere dans un dialogue
	QDialog conductor_dialog(diagramEditor());
#ifdef Q_WS_MAC
	conductor_dialog.setWindowFlags(Qt::Sheet);
#endif
	conductor_dialog.setWindowTitle(tr("\311diter les propri\351t\351s d'un conducteur", "window title"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&conductor_dialog);
	dialog_layout -> addWidget(cpw);
	QCheckBox *cb_apply_all = new QCheckBox(tr("Appliquer les propri\351t\351s \340 l'ensemble des conducteurs de ce potentiel"), &conductor_dialog);
	cb_apply_all->setChecked(true);
	dialog_layout -> addStretch();
	dialog_layout -> addWidget(cb_apply_all);
	QDialogButtonBox *dbb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dbb -> setParent(&conductor_dialog);
	dialog_layout -> addWidget(dbb);
	connect(dbb, SIGNAL(accepted()), &conductor_dialog, SLOT(accept()));
	connect(dbb, SIGNAL(rejected()), &conductor_dialog, SLOT(reject()));
	cpw -> setFocus(Qt::ActiveWindowFocusReason);
	
	// execute le dialogue et met a jour le conducteur
	if (conductor_dialog.exec() == QDialog::Accepted) {
		// recupere les nouvelles proprietes
		ConductorProperties new_properties = cpw -> conductorProperties();

		if (new_properties != old_properties) {
				if (cb_apply_all -> isChecked()) {
					QList <Conductor *> conductorslist = edited_conductor -> relatedPotentialConductors().toList();
					conductorslist << edited_conductor;
					QList <ConductorProperties> old_properties_list;

					foreach (Conductor *c, conductorslist) {
						if (c == edited_conductor) {
							old_properties_list << old_properties;
						}
						else {
							old_properties_list << c -> properties();
							c -> setProperties(new_properties);
						}
					}
					//initialize the corresponding UndoCommand object
					ChangeSeveralConductorsPropertiesCommand *cscpc = new ChangeSeveralConductorsPropertiesCommand(conductorslist);
					cscpc -> setOldSettings(old_properties_list);
					cscpc -> setNewSettings(new_properties);
					diagram() -> undoStack().push(cscpc);
				}

			else {
				// initialise l'objet UndoCommand correspondant
				ChangeConductorPropertiesCommand *ccpc = new ChangeConductorPropertiesCommand(edited_conductor);
				ccpc -> setOldSettings(old_properties);
				ccpc -> setNewSettings(new_properties);
				diagram() -> undoStack().push(ccpc);
			}
		}
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
#ifdef Q_WS_MAC
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
			current_behavior = dragView;
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
	if (current_behavior == dragView && !selectedItemHasFocus() && !isCtrlShifting(e)) {
		setSelectionMode();
		current_behavior = noAction;
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
	Passe le DiagramView en mode "ajout de texte". Un clic cree alors un
	nouveau champ de texte.
*/
void DiagramView::addText() {
	if (scene -> isReadOnly()) return;
	current_behavior = addingText;
}


/**	To edit the text through the htmlEditor
*/
void DiagramView::editText() {
	if (scene -> isReadOnly()) return;
	// Get text to edit
	QList<DiagramTextItem *> texts_to_edit;
	foreach (QGraphicsItem *item, scene -> selectedItems()) {
		if (IndependentTextItem *iti = qgraphicsitem_cast<IndependentTextItem *>(item)) {
			texts_to_edit << iti;
		} else if (ElementTextItem *eti = qgraphicsitem_cast<ElementTextItem *>(item)) {
			// here...
			texts_to_edit << eti;
		}
	}
	// Test if any text existe..
	if (texts_to_edit.isEmpty()) return;	
	else texts_to_edit.at(0)->edit();	
}


/**
* @brief DiagramView::addImage
*/
void DiagramView::addImage() {
	if (scene -> isReadOnly()) return;

	QString pathPictures = QDesktopServices::storageLocation ( QDesktopServices::PicturesLocation );
	QString fileName = QFileDialog::getOpenFileName(this, tr("Selectionner une image..."), pathPictures.toStdString().c_str(), tr("Image Files (*.png *.jpg *.bmp *.svg)"));
	if(fileName.isEmpty()) {
		emit ImageAddedCanceled(false);
		return;
	}

	int ret = image_to_add_.load(fileName);
	if(!ret){
		QMessageBox::critical(this, tr("Erreur"), tr("Impossible de charger l'image...D\351soler :("));
		return;
	}
	current_behavior = addingImage;
}

/**
* @brief DiagramView::addLine
*/
void DiagramView::addLine() {
	current_behavior = addingLine;
}

/**
* @brief DiagramView::addRectangle
*/
void DiagramView::addRectangle() {
	current_behavior = addingRectangle;
}

/**
* @brief DiagramView::addEllipse
*/
void DiagramView::addEllipse() {
	current_behavior = addingEllipse;
}

/**
 * @brief DiagramView::editImage
 * open edit image dialog if only one image is selected
 */
void DiagramView::editImage() {
	if (scene -> isReadOnly()) return;
	QList <QGraphicsItem *> images = diagram() -> selectedContent().items(DiagramContent::Images);
	if (images.count() != 1) return;
	DiagramImageItem *image;
	if ((image = qgraphicsitem_cast<DiagramImageItem *> (images.first()))) {
		image -> editProperty();
	}
}

/**
 * @brief DiagramView::editShape
 * open edit image dialog if only one shape is selected
 */
void DiagramView::editShape() {
	if (scene -> isReadOnly()) return;
	QList <QGraphicsItem *> shapes = diagram() -> selectedContent().items(DiagramContent::Shapes);
	if (shapes.count() != 1) return;
	QetShapeItem *shape;
	if ((shape = qgraphicsitem_cast<QetShapeItem *> (shapes.first()))) {
		shape -> editProperty();
	}
}

/**
* @brief DiagramView::addDiagramImageAtPos
* @param pos
* @return
*/
DiagramImageItem *DiagramView::addDiagramImageAtPos(const QPointF &pos) {

	if (!isInteractive() || scene -> isReadOnly()) return(0);

	// cree un nouveau champ image
	DiagramImageItem *Imageitem = new DiagramImageItem( QPixmap::fromImage(image_to_add_) );

	// le place a la position pos en gerant l'annulation
	scene -> undoStack().push(new AddImageCommand(scene, Imageitem, pos));
	adjustSceneRect();
	
	// emet le signal ImageAdded
	emit(itemAdded());

	return(Imageitem);

}

/**
	Cree un nouveau champ de texte et le place a la position pos
	en gerant l'annulation ; enfin, le signal textAdded est emis.
	@param pos Position du champ de texte ajoute
	@return le champ de texte ajoute
*/
IndependentTextItem *DiagramView::addDiagramTextAtPos(const QPointF &pos, const QString &text) {
	if (!isInteractive() || scene -> isReadOnly()) return(0);
	
	// cree un nouveau champ de texte
	IndependentTextItem *iti;
	if (text.isEmpty()) {
		iti = new IndependentTextItem("_");
	} else iti = new IndependentTextItem(text);
	
	// le place a la position pos en gerant l'annulation
	scene -> undoStack().push(new AddTextCommand(scene, iti, pos));
	adjustSceneRect();
	
	// emet le signal textAdded
	emit(itemAdded());

	return(iti);
}

/**
	Gere le menu contextuel
	@param e Evenement decrivant la demande de menu contextuel
*/
void DiagramView::contextMenuEvent(QContextMenuEvent *e) {
	if (QGraphicsItem *qgi = scene -> itemAt(mapToScene(e -> pos()))) {
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
			context_menu -> addAction(qde -> prj_diagramNum);
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
	Gere les double-clics sur le schema
*/
void DiagramView::mouseDoubleClickEvent(QMouseEvent *e) {
	BorderTitleBlock &bi = scene -> border_and_titleblock;
	
	// recupere le rectangle corespondant au cartouche
	QRectF titleblock_rect(
		Diagram::margin,
		Diagram::margin + bi.diagramHeight(),
		bi.titleBlockWidth(),
		bi.titleBlockHeight()
	);
	
	// recupere le rectangle correspondant aux en-tetes des colonnes
	QRectF columns_rect(
		Diagram::margin,
		Diagram::margin,
		bi.borderWidth(),
		bi.columnsHeaderHeight()
	);
	
	// recupere le rectangle correspondant aux en-tetes des lignes
	QRectF rows_rect(
		Diagram::margin,
		Diagram::margin,
		bi.rowsHeaderWidth(),
		bi.diagramHeight()
	);
	
	// coordonnees du clic par rapport au schema
	QPointF click_pos = viewportTransform().inverted().map(e -> pos());
	
	// detecte le double-clic sur le cartouche ou les colonnes
	if (QGraphicsItem *qgi = itemAt(e -> pos())) {
		if (Conductor *c = qgraphicsitem_cast<Conductor *>(qgi)) {
			editConductor(c);
		} else {
			QGraphicsView::mouseDoubleClickEvent(e);
		}
	} else if (titleblock_rect.contains(click_pos) || columns_rect.contains(click_pos) || rows_rect.contains(click_pos)) {
		// edite les proprietes du schema
		editDiagramProperties();
	} else {
		QGraphicsView::mouseDoubleClickEvent(e);
	}
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
