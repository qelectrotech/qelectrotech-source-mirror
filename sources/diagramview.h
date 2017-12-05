/*
	Copyright 2006-2017 The QElectroTech Team
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

#include <QGraphicsView>
#include <QClipboard>
#include "elementslocation.h"
#include "templatelocation.h"

class Conductor;
class Diagram;
class QETDiagramEditor;
class DVEventInterface;
class QMenu;
class QInputEvent;
class QGestureEvent;

/**
	This class provides a widget to render an electric diagram in an editable,
	interactive way.
*/
class DiagramView : public QGraphicsView
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
		QMenu            *m_context_menu = nullptr;
		QAction          *m_paste_here = nullptr;
		QPoint            m_paste_here_pos;
		QPointF           m_rubber_band_origin;
		bool              m_fresh_focus_in,
						  m_first_activation = true;
	public:
		QString title() const;
		void editDiagramProperties();
		void addColumn();
		void removeColumn();
		void addRow();
		void removeRow();
		Diagram *diagram() { return(m_diagram); }
		QETDiagramEditor *diagramEditor() const;
		bool hasCopiableItems();
		bool hasTextItems();
		void editSelection();
		void setEventInterface (DVEventInterface *event_interface);
	
	protected:
		void mouseDoubleClickEvent(QMouseEvent *) override;
		void contextMenuEvent(QContextMenuEvent *) override;
		void wheelEvent(QWheelEvent *) override;
		void focusInEvent(QFocusEvent *) override;
		void keyPressEvent(QKeyEvent *) override;
		void keyReleaseEvent(QKeyEvent *) override;
		bool event(QEvent *) override;
		virtual bool switchToVisualisationModeIfNeeded(QInputEvent *e);
		virtual bool switchToSelectionModeIfNeeded(QInputEvent *e);
		virtual bool isCtrlShifting(QInputEvent *);
		virtual bool selectedItemHasFocus();
	
	private:
		void mousePressEvent(QMouseEvent *) override;
		void mouseMoveEvent(QMouseEvent *) override;
		void mouseReleaseEvent(QMouseEvent *) override;
		void dragEnterEvent(QDragEnterEvent *) override;
		void dragMoveEvent(QDragMoveEvent *) override;
		void dropEvent(QDropEvent *) override;
		void handleElementDrop(QDropEvent *);
		void handleTitleBlockDrop(QDropEvent *);
		void handleTextDrop(QDropEvent *);
		void scrollOnMovement(QKeyEvent *);
		bool gestureEvent(QGestureEvent *event);
		QRectF viewedSceneRect() const;
		bool mustIntegrateTitleBlockTemplate(const TitleBlockTemplateLocation &) const;
		bool gestures() const;

	signals:
			/// Signal emitted after the selection changed
		void selectionChanged();
			/// Signal emitted after the selection mode changed
		void modeChanged();
			/// Signal emitted after the diagram title changed
		void titleChanged(DiagramView *, const QString &);
			/// Signal emitted when users wish to locate an element from the diagram within elements collection
		void findElementRequired(const ElementsLocation &);
			/// Signal emitted when users wish to edit an element from the diagram
		void editElementRequired(const ElementsLocation &);
			/// Signal emmitted when diagram must be show
		void showDiagram (Diagram *);
	
	public slots:
		void selectNothing();
		void selectAll();
		void selectInvert();
		void setVisualisationMode();
		void setSelectionMode();
		void zoom(const qreal zoom_factor);
		void zoomFit();
		void zoomContent();
		void zoomReset();
		void cut();
		void copy();
		void paste(const QPointF & = QPointF(), QClipboard::Mode = QClipboard::Clipboard);
		void pasteHere();
		void adjustSceneRect();
		void updateWindowTitle();
		void editSelectionProperties();
		void editSelectedConductorColor();
		void editConductorColor(Conductor *);
		void resetConductors();
	
	private slots:
		void adjustGridToZoom();
		void applyReadOnly();
};
#endif
