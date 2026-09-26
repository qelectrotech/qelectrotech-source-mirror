/*
	Copyright 2006-2026 The QElectroTech Team
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
#ifndef DIAGRAMVIEW_H
#define DIAGRAMVIEW_H

#include "../ElementsCollection/elementslocation.h"
#include "titleblock/templatelocation.h"

#include <QClipboard>
#include "palettegraphicsview.h"

class CellRuler;
class Conductor;
class Diagram;
class DiagramContextToolbar;
class DiagramGestureOverlay;
class QETDiagramEditor;
class DVEventInterface;
class QInputEvent;
class QGestureEvent;

/**
	This class provides a widget to render an electric diagram in an editable,
	interactive way.
*/
class DiagramView : public PaletteGraphicsView
{
	Q_OBJECT
	
		// constructors, destructor
	public:
		DiagramView(Diagram *diagram, QWidget * = nullptr);
		~DiagramView() override;

	private:
		DiagramView(const DiagramView &);
	
		// attributes

		Diagram          *m_diagram = nullptr;
		DVEventInterface *m_event_interface = nullptr;
		QAction          *m_paste_here = nullptr;
		QAction			 *m_multi_paste = nullptr;
		QAction          *m_create_template = nullptr;
		QPoint            m_paste_here_pos;
		QPoint            m_press_pos;
		DiagramContextToolbar *m_context_toolbar = nullptr;
			/// Right-drag gestures: tracking since the right button went down
		bool              m_gesture_tracking = false;
			/// The platform's own right-click menu event is to be ignored:
			/// the view opens the menu itself on release
		bool              m_swallow_native_menu = false;
		bool              m_menu_from_gesture = false;
		QPoint            m_gesture_origin;
		DiagramGestureOverlay *m_gesture_overlay = nullptr;
		QPoint            m_last_mouse_pos = QPoint(-1, -1);
		QPointF           m_drag_last_pos;
		bool              m_fresh_focus_in,
						  m_first_activation = true;
		QList<QAction *>  m_separators;
		QPolygonF m_free_rubberband;
		bool m_free_rubberbanding = false;
		CellRuler *m_top_ruler = nullptr;
		CellRuler *m_side_ruler = nullptr;
		bool m_cell_rulers_shown = false;
		/// Last viewport transform the rulers were painted for
		QTransform m_rulers_transform;
		bool m_cell_lines_shown = false;

		
	public:
		QString title() const;
		void editDiagramProperties();
		Diagram *diagram() { return(m_diagram); }
		QETDiagramEditor *diagramEditor() const;
		void editSelection();
		void setEventInterface (DVEventInterface *event_interface);
		QList<QAction *> contextMenuActions() const;
		/// Last mouse position seen by mouseMoveEvent(), in viewport
		/// coordinates -- (-1, -1) if the mouse hasn't moved over this
		/// view yet. Filled from ordinary Qt mouse events, not a global
		/// cursor query (QCursor::pos()/setPos() are silently ignored by
		/// several window managers and compositors, Wayland included).
		QPoint lastMousePos() const { return m_last_mouse_pos; }
	
		bool startElementPlacement(const ElementsLocation &location,
					   const QPointF &scene_pos);
		QPointF defaultPlacementPos() const;
		void setCellRulersShown(bool shown);
		void setCellLinesShown(bool shown);

	protected:
		void mouseDoubleClickEvent(QMouseEvent *) override;
		void contextMenuEvent(QContextMenuEvent *) override;
		void wheelEvent(QWheelEvent *) override;
		void focusInEvent(QFocusEvent *) override;
		void keyPressEvent(QKeyEvent *) override;
		void keyReleaseEvent(QKeyEvent *) override;
		bool event(QEvent *) override;
		bool focusNextPrevChild(bool next) override;
		///Set for one call only, by the Escape handler, to let focus leave the view.
		bool m_releasing_focus = false;
		void paintEvent(QPaintEvent *event) override;
		bool viewportEvent(QEvent *event) override;
		void drawBackground(QPainter *painter, const QRectF &rect) override;
		void paintingInverted(bool inverted) override;
		void mousePressEvent(QMouseEvent *) override;
		void mouseMoveEvent(QMouseEvent *) override;
		void mouseReleaseEvent(QMouseEvent *) override;
		void dragEnterEvent(QDragEnterEvent *) override;
		void dragMoveEvent(QDragMoveEvent *) override;
		void dropEvent(QDropEvent *) override;
		
		virtual bool switchToVisualisationModeIfNeeded(QInputEvent *e);
		virtual bool switchToSelectionModeIfNeeded(QInputEvent *e);
		virtual bool isCtrlShifting(QInputEvent *);
		virtual bool selectedItemHasFocus();
	
	private:
		void handleElementDrop(QDropEvent *);
		void handleTitleBlockDrop(QDropEvent *);
		void handleTextDrop(QDropEvent *);
		void scrollOnMovement(QKeyEvent *);
		bool gestureEvent(QGestureEvent *event);
		QRectF viewedSceneRect() const;
		bool mustIntegrateTitleBlockTemplate(const TitleBlockTemplateLocation &) const;
		bool gestures() const;
		void updateCellRulers();
		void placeCellRulers();
		void showContextToolbar(const QPoint &viewport_pos);
		QList<QAction *> selectionCommands() const;

		/// Lowest and highest allowed value of the view transform scale (m11).
		/// Prevents wheel-zoom from driving the transform to overflow, which
		/// crashes the editor (see GitHub issue #798, same class of bug).
		static constexpr qreal m_min_zoom = 0.01;
		static constexpr qreal m_max_zoom = 200.0;

	signals:
			/// Signal emitted after the selection mode changed
		void modeChanged();
			/// Signal emitted after the diagram title changed
		void titleChanged(DiagramView *, const QString &);
			/// Signal emitted when users wish to locate an element from the diagram within elements collection
		void findElementRequired(const ElementsLocation &);
			/// Signal emitted when diagram must be show
		void showDiagram (Diagram *);
			/// Signal emitted when free rubberband changed.
			/// When free rubberband selection ends this signal will be emitted with null value.
		void freeRubberBandChanged(QPolygonF polygon);
			/// Signal emitted when the placement mode is entered for an
			/// element (not a macro), whether from a drop or not.
		void elementPlacementStarted(const ElementsLocation &);
	
	public slots:
		void setVisualisationMode();
		void setSelectionMode();
		void zoom(const qreal zoom_factor);
		void zoomFit();
		void zoomContent();
		void zoomReset();
		void zoomToRect(const QRectF &rect);
		void cut();
		void copy();
		void paste(const QPointF & = QPointF(), QClipboard::Mode = QClipboard::Clipboard);
		void pasteHere();
		void duplicate(const QPoint &stepOffset);
		void adjustSceneRect();
		void updateWindowTitle();
		void resetConductors();
	
	private slots:
		void adjustGridToZoom();
		void applyReadOnly();
		void createTemplateFromSelection();
};
#endif
