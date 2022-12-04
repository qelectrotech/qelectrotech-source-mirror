/*
	Copyright 2006-2021 The QElectroTech Team
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

#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "diagramcommands.h"
#include "diagramevent/diagrameventaddelement.h"
#include "dvevent/dveventinterface.h"
#include "projectview.h"
#include "qetdiagrameditor.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qeticons.h"
#include "titleblock/integrationmovetemplateshandler.h"
#include "ui/diagrampropertiesdialog.h"
#include "ui/multipastedialog.h"
#include "undocommand/changetitleblockcommand.h"
#include "utils/conductorcreator.h"
#include "undocommand/addgraphicsobjectcommand.h"
#include "diagram.h"

#include <QDropEvent>

/**
	Constructeur
	@param diagram Schema a afficher ; si diagram vaut 0, un nouveau Diagram est utilise
	@param parent Le QWidget parent de cette vue de schema
*/
DiagramView::DiagramView(Diagram *diagram, QWidget *parent) :
	QGraphicsView (parent),
	m_diagram (diagram)
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

	setScene(m_diagram);
	m_diagram -> undoStack().setClean();
	setWindowIcon(QET::Icons::QETLogo);
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setResizeAnchor(QGraphicsView::AnchorUnderMouse);
	setAlignment(Qt::AlignLeft | Qt::AlignTop);
	setSelectionMode();
	adjustSceneRect();
	updateWindowTitle();
	m_diagram->loadElmtFolioSeq();
	m_diagram->loadCndFolioSeq();

	m_paste_here = new QAction(QET::Icons::EditPaste, tr("Coller ici", "context menu action"), this);
	connect(m_paste_here, SIGNAL(triggered()), this, SLOT(pasteHere()));

	m_multi_paste = new QAction(QET::Icons::EditPaste, tr("Collage multiple"), this);
	connect(m_multi_paste, &QAction::triggered, [this]() {
		MultiPasteDialog d(this->m_diagram, this);
		d.exec();
	});

		//setup three separators, to be use in context menu
	for(int i=0 ; i<3 ; ++i)
	{
		m_separators << new QAction(this);
		m_separators.last()->setSeparator(true);
	}

	connect(m_diagram, SIGNAL(showDiagram(Diagram*)), this, SIGNAL(showDiagram(Diagram*)));
	connect(m_diagram, SIGNAL(sceneRectChanged(QRectF)), this, SLOT(adjustSceneRect()));
	connect(&(m_diagram -> border_and_titleblock), SIGNAL(diagramTitleChanged(const QString &)), this, SLOT(updateWindowTitle()));
	connect(diagram, SIGNAL(findElementRequired(ElementsLocation)), this, SIGNAL(findElementRequired(ElementsLocation)));

	QShortcut *edit_conductor_color_shortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
	connect(edit_conductor_color_shortcut, &QShortcut::activated, [this]()
	{
		if (m_diagram->isReadOnly()) {
			return;
		}

		DiagramContent dc(m_diagram);
		auto cond_list = dc.conductors(DiagramContent::AnyConductor);
		if (cond_list.size() != 1) {
			return;
		}
		auto edited_conductor = cond_list.first();

			// store the initial properties of the provided conductor
		ConductorProperties initial_properties = edited_conductor->properties();

			// prepare a color dialog showing the initial conductor color
		QColorDialog *color_dialog = new QColorDialog(this);
		color_dialog->setWindowTitle(tr("Choisir la nouvelle couleur de ce conducteur"));
#ifdef Q_OS_MACOS
		color_dialog -> setWindowFlags(Qt::Sheet);
#endif
		color_dialog->setCurrentColor(initial_properties.color);

			// asks the user what color he wishes to apply
		if (color_dialog->exec() == QDialog::Accepted)
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
				m_diagram->undoStack().push(undo);
			}
		}
	});
}

/**
	Destructeur
*/
DiagramView::~DiagramView()
{}

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
	@brief DiagramView::handleElementDrop
	Handle the drop of an element.
	@param event the QDropEvent describing the current drag'n drop
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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)	// ### Qt 6: remove
	diagram()->setEventInterface(
				new DiagramEventAddElement(
					location, diagram(), mapToScene(event->pos())));
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
	diagram()->setEventInterface(
				new DiagramEventAddElement(
					location, diagram(), event->position()));
#endif

		//Set focus to the view to get event
	this->setFocus();
}

/**
	@brief DiagramView::handleTitleBlockDrop
	Handle the dropEvent that contain data of a titleblock
	@param e
*/
void DiagramView::handleTitleBlockDrop(QDropEvent *e) {
		// fetch the title block template location from the drop event
	TitleBlockTemplateLocation tbt_loc;
	tbt_loc.fromString(e->mimeData()->text());


	if (tbt_loc.isValid())
	{
			// fetch the current title block properties
		TitleBlockProperties titleblock_properties_before = m_diagram->border_and_titleblock.exportTitleBlock();

			// check the provided template is not already applied
		QETProject *tbt_parent_project = tbt_loc.parentProject();
		if (tbt_parent_project && tbt_parent_project == m_diagram -> project())
		{
				// same parent project and same name = same title block template
			if (tbt_loc.name() == titleblock_properties_before.template_name)
				return;
		}

			// integrate the provided template into the project if needed
		QString integrated_template_name = tbt_loc.name();
		if (mustIntegrateTitleBlockTemplate(tbt_loc))
		{
			IntegrationMoveTitleBlockTemplatesHandler *handler = new IntegrationMoveTitleBlockTemplatesHandler(this);
			//QString error_message;
			integrated_template_name = m_diagram->project()->integrateTitleBlockTemplate(tbt_loc, handler);

			if (integrated_template_name.isEmpty())
				return;
		}

			// apply the provided title block template
		if (titleblock_properties_before.template_name == integrated_template_name)
			return;

		TitleBlockProperties titleblock_properties_after = titleblock_properties_before;
		titleblock_properties_after.template_name = integrated_template_name;
		m_diagram->undoStack().push(new ChangeTitleBlockCommand(m_diagram, titleblock_properties_before, titleblock_properties_after));

		adjustSceneRect();
	}
}

/**
	@brief DiagramView::handleTextDrop
 *handle the drop of text
	@param e the QDropEvent describing the current drag'n drop
*/
void DiagramView::handleTextDrop(QDropEvent *e) {
	if (m_diagram -> isReadOnly() || (e -> mimeData() -> hasText() == false) ) return;

	IndependentTextItem *iti = new IndependentTextItem (e -> mimeData() -> text());

	if (e -> mimeData() -> hasHtml()) {
		iti -> setHtml (e -> mimeData() -> text());
	}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)	// ### Qt 6: remove

	m_diagram->undoStack().push(new AddGraphicsObjectCommand(
									iti, m_diagram, mapToScene(e->pos())));
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
	m_diagram->undoStack().push(new AddGraphicsObjectCommand(
									iti, m_diagram, e->position()));
#endif
}

/**
	Set the Diagram in visualisation mode
*/
void DiagramView::setVisualisationMode()
{
	setDragMode(ScrollHandDrag);
	applyReadOnly();
	setInteractive(false);
	emit(modeChanged());
}

/**
	Set the Diagram in Selection mode
*/
void DiagramView::setSelectionMode()
{
	setDragMode(RubberBandDrag);
	setInteractive(true);
	applyReadOnly();
	emit(modeChanged());
}

/**
	@brief DiagramView::zoom
	Zomm the view.
	A zoom_factor > 1 zoom in.
	A zoom_factor < 1 zoom out
	@param zoom_factor
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
			if (zoom_factor >= 0){
				scale(zoom_factor, zoom_factor);
			}
	}
	m_diagram->adjustSceneRect();
	adjustGridToZoom();
	adjustSceneRect();
}

/**
	Agrandit ou rectrecit le schema de facon a ce que tous les elements du
	schema soient visibles a l'ecran. S'il n'y a aucun element sur le schema,
	le zoom est reinitialise
*/
void DiagramView::zoomFit()
{
	adjustSceneRect();
	fitInView(m_diagram->sceneRect(), Qt::KeepAspectRatio);
	adjustGridToZoom();
}

/**
	Adjust zoom to fit all elements in the view, regardless of diagram borders.
*/
void DiagramView::zoomContent()
{
	fitInView(m_diagram -> itemsBoundingRect(), Qt::KeepAspectRatio);
	adjustGridToZoom();
}

/**
	Reinitialise le zoom
*/
void DiagramView::zoomReset()
{
	resetTransform();
	adjustGridToZoom();
}

/**
	Copie les elements selectionnes du schema dans le presse-papier puis les supprime
*/
void DiagramView::cut()
{
	copy();
	DiagramContent cut_content(m_diagram);
	m_diagram -> clearSelection();
	m_diagram -> undoStack().push(new CutDiagramCommand(m_diagram, cut_content));
}

/**
	Copie les elements selectionnes du schema dans le presse-papier
*/
void DiagramView::copy()
{
	QClipboard *presse_papier = QApplication::clipboard();
	QString contenu_presse_papier = m_diagram -> toXml(false).toString(4);
	if (presse_papier -> supportsSelection()) presse_papier -> setText(contenu_presse_papier, QClipboard::Selection);
	presse_papier -> setText(contenu_presse_papier);
}

/**
	@brief DiagramView::paste
	Import the element stored in the clipboard to the diagram.
	@param pos : top left corner of the bounding rect of imported elements
	@param clipboard_mode
*/
void DiagramView::paste(const QPointF &pos, QClipboard::Mode clipboard_mode) {
	if (!isInteractive() || m_diagram -> isReadOnly()) return;

	QString texte_presse_papier = QApplication::clipboard() -> text(clipboard_mode);
	if ((texte_presse_papier).isEmpty()) return;

	QDomDocument document_xml;
	if (!document_xml.setContent(texte_presse_papier)) return;

	DiagramContent content_pasted;
	m_diagram->fromXml(document_xml, pos, false, &content_pasted);

		//If something was really added to diagram, we create an undo object.
	if (content_pasted.count())
	{
		m_diagram -> clearSelection();
		m_diagram -> undoStack().push(new PasteDiagramCommand(m_diagram, content_pasted));
		adjustSceneRect();
	}
}

/**
	Colle le contenu du presse-papier sur le schema a la position de la souris
*/
void DiagramView::pasteHere()
{
	paste(mapToScene(m_paste_here_pos));
}

/**
	Manage the events press click :
	 *  click to add an independent text field
*/
void DiagramView::mousePressEvent(QMouseEvent *e)
{
	e->ignore();

	if (m_fresh_focus_in)
	{
		switchToVisualisationModeIfNeeded(e);
		m_fresh_focus_in = false;
	}

	if (m_event_interface && m_event_interface->mousePressEvent(e)) return;

		//Start drag view when hold the middle button
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 1) // ### Qt 6: remove
	if (e->button() == Qt::MidButton)
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
	if (e->button() == Qt::MiddleButton)
#endif
	{
		m_drag_last_pos = e->pos();
		viewport()->setCursor(Qt::ClosedHandCursor);
		e->accept();
		return;
	}

		//There is a good luck that user want to do a free selection
		//In this case we temporally disable the dragmode because if the QGraphicsScene don't accept the event,
		//and the drag mode is set to rubberbanddrag, the QGraphicsView start rubber band drag, and accept the event.
	if (e->button() == Qt::LeftButton &&
		e->modifiers() == Qt::CTRL)
	{
		QGraphicsView::DragMode dm = dragMode();
		setDragMode(QGraphicsView::NoDrag);
		QGraphicsView::mousePressEvent(e);
		setDragMode(dm);
	} else {
		QGraphicsView::mousePressEvent(e);
	}

	if (e->isAccepted()) {
		return;
	}

	if (e->button() == Qt::LeftButton &&
			 e->modifiers() == Qt::CTRL)
	{
		m_free_rubberbanding = true;
		m_free_rubberband = QPolygon();
		e->accept();
		return;
	}

	if (!e->isAccepted()) {
		QGraphicsView::mousePressEvent(e);
	}
}

/**
	@brief DiagramView::mouseMoveEvent
	Manage the event move mouse
*/
void DiagramView::mouseMoveEvent(QMouseEvent *e)
{
	setToolTip(tr("X: %1 Y: %2").arg(e->pos().x()).arg(e->pos().y()));
	if (m_event_interface && m_event_interface->mouseMoveEvent(e)) return;

		// Drag the view
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 1) // ### Qt 6: remove
	if (e->buttons() == Qt::MidButton)
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
	if (e->buttons() == Qt::MiddleButton)
#endif
	{
		QScrollBar *h = horizontalScrollBar();
		QScrollBar *v = verticalScrollBar();
		QPointF pos = m_drag_last_pos - e -> pos();
		m_drag_last_pos = e -> pos();
		h -> setValue(h -> value() + pos.x());
		v -> setValue(v -> value() + pos.y());
		adjustSceneRect();
	}
	else if (m_free_rubberbanding)
	{
			//Update old free rubberband
		if (viewportUpdateMode() != QGraphicsView::NoViewportUpdate && !m_free_rubberband.isEmpty())
		{
			if (viewportUpdateMode() != QGraphicsView::FullViewportUpdate) {
				viewport()->update(m_free_rubberband.boundingRect().toRect().adjusted(-10,-10,10,10));
			}
			else {
				update();
			}
		}

			//Stop polygon rubberbanding if user has let go of all buttons (even
			//if we didn't get the release events)
		if (!e->buttons()) {
			m_free_rubberbanding = false;
			m_free_rubberband = QPolygon();
			return;
		}
		m_free_rubberband.append(mapToScene(e->pos()));
		emit freeRubberBandChanged(m_free_rubberband);

		if (viewportUpdateMode() != QGraphicsView::NoViewportUpdate)
		{
			if (viewportUpdateMode() != QGraphicsView::FullViewportUpdate) {
				viewport()->update(mapFromScene(m_free_rubberband.boundingRect().adjusted(-10,-10,10,10)));
			}
			else {
				update();
			}
		}

			//Set the new selection area
		QPainterPath selection_area;
		selection_area.addPolygon(m_free_rubberband);
		m_diagram->setSelectionArea(selection_area);
	}

	else QGraphicsView::mouseMoveEvent(e);
}

/**
	@brief DiagramView::mouseReleaseEvent
	Manage event release click mouse
*/
void DiagramView::mouseReleaseEvent(QMouseEvent *e)
{
	if (m_event_interface && m_event_interface->mouseReleaseEvent(e)) return;

		// Stop drag view
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 1) // ### Qt 6: remove
	if (e->button() == Qt::MidButton)
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
	if (e->button() == Qt::MiddleButton)
#endif
	{
		viewport()->setCursor(Qt::ArrowCursor);
	}
	else if (m_free_rubberbanding && !e->buttons())
	{
		if (viewportUpdateMode() != QGraphicsView::NoViewportUpdate)
		{
			if (viewportUpdateMode() != QGraphicsView::FullViewportUpdate)
			{
				QRectF r(mapFromScene(m_free_rubberband).boundingRect());
				r.adjust(-10, -10, 10, 10);
				viewport()->update(r.toRect());
			}
			else
			{
				update();
			}
		}

		if (m_free_rubberband.count() > 3)
		{
				//Popup a menu with an action to create conductors between
				//all selected terminals.
			QAction *act = new QAction(tr("Connecter les bornes sélectionnées"), this);
			QPolygonF polygon_ = m_free_rubberband;
			connect(act, &QAction::triggered, [this, polygon_]()
			{
				ConductorCreator::create(m_diagram, polygon_);
				diagram()->clearSelection();
			});
			QMenu *menu = new QMenu(this);
			menu->addAction(act);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)	// ### Qt 6: remove
			menu->popup(e->globalPos());
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
			menu->popup(e->pos());
#endif
		}

		m_free_rubberbanding = false;
		m_free_rubberband = QPolygon();
		emit freeRubberBandChanged(m_free_rubberband);
		e->accept();
	}
	else
		QGraphicsView::mouseReleaseEvent(e);
}

/**
	@brief DiagramView::gestures
	@return
*/
bool DiagramView::gestures() const
{
	QSettings settings;
	return(settings.value("diagramview/gestures", false).toBool());
}

/**
	@brief DiagramView::wheelEvent
	Manage wheel event of mouse
	@param event QWheelEvent
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
	@brief DiagramView::gestureEvent
	Use the pinch of the trackpad for zoom
	@param event
	@return
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
		m_fresh_focus_in = true;
	}
}

/**
	@brief DiagramView::keyPressEvent
	Handles "key press" events. Reimplemented here to switch to visualisation
	mode if needed.
	@param e
*/
void DiagramView::keyPressEvent(QKeyEvent *e)
{
	if (m_event_interface && m_event_interface->keyPressEvent(e))
		return;

	ProjectView *current_project = this->diagramEditor()->currentProjectView();
	DiagramContent dc(m_diagram);
	switch(e -> key())
	{
		case Qt::Key_PageUp:
			current_project->changeTabUp();
			return;
		case Qt::Key_PageDown:
			current_project->changeTabDown();
			return;
		case Qt::Key_Home:
			if (dc.selectedTexts().isEmpty()) {
				if (
					qgraphicsitem_cast<IndependentTextItem *>(m_diagram->focusItem()) ||
					qgraphicsitem_cast<ConductorTextItem *>(m_diagram->focusItem()) ||
					qgraphicsitem_cast<DiagramTextItem *>(m_diagram->focusItem())
					)
					break;
				current_project->changeFirstTab();
				return;
			}
			else break;
		case Qt::Key_End:
			if (dc.selectedTexts().isEmpty()) {
				if (
					qgraphicsitem_cast<IndependentTextItem *>(m_diagram->focusItem()) ||
					qgraphicsitem_cast<ConductorTextItem *>(m_diagram->focusItem()) ||
					qgraphicsitem_cast<DiagramTextItem *>(m_diagram->focusItem())
					)
					break;
				current_project->changeLastTab();
				return;
			}
			else break;
		case Qt::Key_ZoomOut:
			zoom(0.85);
			return;
		case Qt::Key_ZoomIn:
			zoom(1.15);
			return;
		case Qt::Key_Minus: {
			if (e->modifiers() & Qt::ControlModifier)
				zoom(0.85);
		}
			break;
		case Qt::Key_Plus: {
			if (e->modifiers() & Qt::ControlModifier)
				zoom(1.15);
		}
			break;
		case Qt::Key_Up: {
			if(!dc.items(DiagramContent::All).isEmpty() && !dc.hasTextEditing())
				scrollOnMovement(e);
		}
			break;
		case Qt::Key_Down: {
			if(!dc.items(DiagramContent::All).isEmpty() && !dc.hasTextEditing())
				scrollOnMovement(e);
		}
			break;
		case Qt::Key_Left: {
			if(!dc.items(DiagramContent::All).isEmpty() && !dc.hasTextEditing())
				scrollOnMovement(e);
		}
			break;
		case Qt::Key_Right: {
			if(!dc.items(DiagramContent::All).isEmpty() && !dc.hasTextEditing())
				scrollOnMovement(e);
		}
			break;
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
void DiagramView::scrollOnMovement(QKeyEvent *e)
{
			const QList<QGraphicsItem *> selected_elmts = DiagramContent(m_diagram).items(DiagramContent::All);
			QRectF viewed_scene = viewedSceneRect();
			for (QGraphicsItem *qgi : selected_elmts)
			{
				if (qgraphicsitem_cast<Conductor *>(qgi))
					continue;
				if(qgi->parentItem() && qgi->parentItem()->isSelected())
					continue;

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
				bool elmt_below_top_margin     = elmt_top>=top;
				bool elmt_above_bottom_margin  = elmt_bottom<=bottom;

				if (!(elmt_right_of_left_margin && elmt_left_of_right_margin) ||
					!(elmt_below_top_margin    && elmt_above_bottom_margin )  )
				{
						QScrollBar *h = horizontalScrollBar();
						QScrollBar *v = verticalScrollBar();
						int h_increment=0;
						int v_increment=0;
						if (e->key()==Qt::Key_Up && elmt_above_bottom_margin) {
							v_increment = 2*qgi->boundingRect().top();
							if (v_increment == 0) v_increment = -2*qgi->boundingRect().height();
						}
						else if(e->key()==Qt::Key_Down && elmt_below_top_margin) {
							v_increment = 2*qgi->boundingRect().bottom();
							if (v_increment == 0) v_increment = -2*qgi->boundingRect().height();
						}
						else if (e->key()==Qt::Key_Left && elmt_left_of_right_margin) {
							h_increment = 2*qgi->boundingRect().left();
							if (h_increment == 0) h_increment = -2*qgi->boundingRect().width();
						}
						else if (e->key()==Qt::Key_Right && elmt_right_of_left_margin) {
							h_increment = 2*qgi->boundingRect().right();
							if (h_increment == 0) h_increment = -2*qgi->boundingRect().width();
						}
						if (((elmt_right  >= m_diagram->sceneRect().right() -  qgi->boundingRect().right())  ||
							(elmt_bottom >= m_diagram->sceneRect().bottom() - qgi->boundingRect().bottom())) &&
							(e->key()==Qt::Key_Right || e->key()==Qt::Key_Down)){
							m_diagram->adjustSceneRect();
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
QString DiagramView::title() const
{
	QString view_title;
	QString diagram_title(m_diagram -> title());
	if (diagram_title.isEmpty()) {
		view_title = tr("Sans titre", "what to display for untitled diagrams");
	} else {
		view_title = diagram_title;
	}
	return(view_title);
}

/**
	@brief DiagramView::editDiagramProperties
	Edit the properties of the viewed digram
*/
void DiagramView::editDiagramProperties()
{
	DiagramPropertiesDialog::diagramPropertiesDialog(m_diagram, diagramEditor());
}

/**
	@brief DiagramView::adjustSceneRect
	Calcul and set the area of the scene visualized by this view
*/
void DiagramView::adjustSceneRect()
{
	QRectF scene_rect = m_diagram->sceneRect();
	scene_rect.adjust(-Diagram::margin, -Diagram::margin, Diagram::margin, Diagram::margin);

	QSettings settings;
	if (settings.value("diagrameditor/zoom-out-beyond-of-folio", false).toBool())
	{
			//When zoom out beyond of folio is active,
			//we always adjust the scene rect to be 1/3 bigger than the wiewport
		QRectF vpbr = mapToScene(viewport()->rect()).boundingRect();
		vpbr.adjust(0, 0, vpbr.width()/3, vpbr.height()/3);
		scene_rect = scene_rect.united(vpbr);
	}
	setSceneRect(scene_rect);
}

/**
	Met a jour le titre du widget
*/
void DiagramView::updateWindowTitle()
{
	emit(titleChanged(this, title()));
}

/**
	Enables or disables the drawing grid according to the amount of pixels display
*/
void DiagramView::adjustGridToZoom()
{
	QRectF viewed_scene = viewedSceneRect();
	if (diagramEditor()->drawGrid())
		m_diagram->setDisplayGrid(viewed_scene.width() < 2000 || viewed_scene.height() < 2000);
	else
		m_diagram->setDisplayGrid(false);
}

/**
	@return le rectangle du schema (classe Diagram) visualise par ce DiagramView
*/
QRectF DiagramView::viewedSceneRect() const
{
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
bool DiagramView::mustIntegrateTitleBlockTemplate(const TitleBlockTemplateLocation &tbt_loc) const
{
	// unlike elements, the integration of title block templates is mandatory, so we simply check whether the parent project of the template is also the parent project of the diagram
	QETProject *tbt_parent_project = tbt_loc.parentProject();
	if (!tbt_parent_project) return(true);

	return(tbt_parent_project != m_diagram -> project());
}

/**
	Fait en sorte que le schema ne soit editable que s'il n'est pas en lecture
	seule
*/
void DiagramView::applyReadOnly()
{
	if (!m_diagram) return;

	bool is_writable = !m_diagram -> isReadOnly();
	setInteractive(is_writable);
	setAcceptDrops(is_writable);
}

/**
	Reinitialise le profil des conducteurs selectionnes
*/
void DiagramView::resetConductors()
{
	if (m_diagram -> isReadOnly()) return;
	// recupere les conducteurs selectionnes
	QSet<Conductor *> selected_conductors = m_diagram -> selectedConductors();

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
	m_diagram -> undoStack().push(new ResetConductorCommand(conductors_and_profiles));
}

/**
	@brief DiagramView::event
	Manage the event on this diagram view.
	-At first activation (QEvent::WindowActivate or QEvent::Show) we zoomFit.
	-Convert event interpreted to mouse event to gesture event if needed.
	-send Shortcut to view (by default send to QMenu /QAction)
	Gere les evenements de la DiagramView
	@param e : the event.
	@return
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
	@brief DiagramView::paintEvent
	Reimplemented from QGraphicsView
	@param event
*/
void DiagramView::paintEvent(QPaintEvent *event)
{
	QGraphicsView::paintEvent(event);

	if (m_free_rubberbanding && m_free_rubberband.count() >= 3)
	{
		QPainter painter(viewport());
		painter.setRenderHint(QPainter::Antialiasing);
		QPen pen(Qt::darkGreen);
		pen.setWidth(1);
		painter.setPen(pen);
		QColor color(Qt::darkGreen);
		color.setAlpha(50);
		QBrush brush(color);
		painter.setBrush(brush);
		painter.drawPolygon(mapFromScene(m_free_rubberband));
	}
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
bool DiagramView::selectedItemHasFocus()
{
	return(
		m_diagram -> hasFocus() &&
		m_diagram -> focusItem() &&
		m_diagram -> focusItem() -> isSelected()
	);
}

/**
	@brief DiagramView::editSelection
	Edit the selected item if he can be edited and if only  one item is selected
*/
void DiagramView::editSelection()
{
	if (m_diagram -> isReadOnly() || m_diagram -> selectedItems().size() != 1 ) return;

	QGraphicsItem *item = m_diagram->selectedItems().first();

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
	@brief DiagramView::setEventInterface
	Set an event interface to diagram view.
	If diagram view already have an event interface, he delete it before.
	Diagram view take ownership of event interface and delete it when event interface is finish
*/
void DiagramView::setEventInterface(DVEventInterface *event_interface)
{
	if (m_event_interface) delete m_event_interface;
	m_event_interface = event_interface;
	connect(m_event_interface, &DVEventInterface::finish, this, [=](){delete this->m_event_interface; this->m_event_interface = nullptr;}, Qt::QueuedConnection);
}

/**
	@brief DiagramView::contextMenuActions
	@return a list of actions currently available for a context menu.

*/
QList<QAction *> DiagramView::contextMenuActions() const
{
	QList<QAction *> list;
	if (QETDiagramEditor *qde = diagramEditor())
	{
		if (m_diagram->selectedItems().isEmpty())
		{
			list << m_paste_here;
			list << m_separators.at(0);
			list << qde->m_edit_diagram_properties;
			list << qde->m_row_column_actions_group.actions();
		}
		else
		{
			list << qde->m_cut;
			list << qde->m_copy;
			list << m_multi_paste;
			list << m_separators.at(0);
			list << qde->m_conductor_reset;
			list << m_separators.at(1);
			list << qde->m_selection_actions_group.actions();
			list << m_separators.at(2);
			list << qde->m_depth_action_group->actions();
		}

			//Remove from the context menu the actions which are disabled.
		const QList<QAction *> actions = list;
		for(QAction *action : actions)
		{
			if (!action->isEnabled()) {
				list.removeAll(action);
			}
		}
	}

	return list;
}

/**
	@brief DiagramView::contextMenuEvent
	@param e
*/
void DiagramView::contextMenuEvent(QContextMenuEvent *e)
{
	if (auto qgi = m_diagram->itemAt(mapToScene(e->pos()), transform()))
	{
		if (!qgi->isSelected()) {
			m_diagram->clearSelection();
		}

			// At this step qgi can be deleted for example if qgi is a QetGraphicsHandlerItem.
			// When we call clearSelection the parent item of the handler
			// is deselected and so delete all handlers, in this case,
			// qgi become a dangling pointer.
			// we need to call again itemAt.
		if (auto item_ = m_diagram->itemAt(mapToScene(e->pos()), transform())) {
			item_->setSelected(true);
		}
	}

	if (m_diagram->selectedItems().isEmpty())
	{
		m_paste_here_pos = e->pos();
		m_paste_here->setEnabled(Diagram::clipboardMayContainDiagram());
	}

	QList <QAction *> list = contextMenuActions();
	if(!list.isEmpty())
	{
		QMenu *context_menu = new QMenu(this);
		context_menu->addActions(list);
		context_menu->popup(e->globalPos());
		e->accept();
	}
}

/**
	@return l'editeur de schemas parent ou 0
*/
QETDiagramEditor *DiagramView::diagramEditor() const
{
	// remonte la hierarchie des widgets
	QWidget *w = const_cast<DiagramView *>(this);
	while (w -> parentWidget() && !w -> isWindow()) {
		w = w -> parentWidget();
	}
	// la fenetre est supposee etre un QETDiagramEditor
	return(qobject_cast<QETDiagramEditor *>(w));
}

/**
	@brief DiagramView::mouseDoubleClickEvent
	@param e
*/
void DiagramView::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (m_event_interface && m_event_interface -> mouseDoubleClickEvent(e)) return;

	BorderTitleBlock &bi = m_diagram -> border_and_titleblock;

	//Get the click pos on the diagram
	QPointF click_pos = viewportTransform().inverted().map(e -> pos());

	if (bi.titleBlockRect().contains(click_pos) || bi.columnsRect().contains(click_pos) || bi.rowsRect().contains(click_pos)) {
		e->accept();
		editDiagramProperties();
		return;
	}
	QGraphicsView::mouseDoubleClickEvent(e);
}
