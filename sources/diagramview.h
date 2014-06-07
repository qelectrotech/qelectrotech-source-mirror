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
#ifndef DIAGRAMVIEW_H
#define DIAGRAMVIEW_H
#include <QtGui>
#include "elementslocation.h"
#include "templatelocation.h"
#include "qetgraphicsitem/qetshapeitem.h"
class Conductor;
class Diagram;
class Element;
class IndependentTextItem;
class DiagramImageItem;
class QETDiagramEditor;
/**
	This class provides a widget to render an electric diagram in an editable,
	interactive way.
*/
class DiagramView : public QGraphicsView {
	Q_OBJECT
	
	// constructors, destructor
	public:
	DiagramView(Diagram * = 0, QWidget * = 0);
	virtual ~DiagramView();
	
	Q_ENUMS(behavior)
	enum behavior {noAction		   =1,
				   addingText	   =2,
				   addingImage	   =4,
				   addingLine	   =8,
				   addingRectangle =16,
				   addingEllipse   =32,
				   addingShape	   =56,
				   dragView        =64};

	private:
	DiagramView(const DiagramView &);
	
	// attributes
	private:
	Diagram *scene;
	QMenu *context_menu;
	QAction *paste_here;
	QAction *find_element_;
	QPoint paste_here_pos;
	behavior current_behavior;
	bool fresh_focus_in_;               ///< Indicate the focus was freshly gained
	ElementsLocation next_location_;
	QPoint next_position_;
	QPointF center_view_;
	QImage image_to_add_;
	QetShapeItem *newItem;
	QPointF rubber_band_origin;
	
	// methods
	public:
	QString title() const;
	void editDiagramProperties();
	void addColumn();
	void removeColumn();
	void addRow();
	void removeRow();
	/// @return the diagram rendered by this view
	Diagram *diagram() { return(scene); }
	QETDiagramEditor *diagramEditor() const;
	bool hasSelectedItems();
	bool hasCopiableItems();
	bool hasDeletableItems();
	void addText();
	void editText();
	void addImage();
	void addLine();
	void addRectangle();
	void addEllipse();
	void editImage();
	void editShape();
	IndependentTextItem *addDiagramTextAtPos(const QPointF &, const QString &text = 0);
	DiagramImageItem *addDiagramImageAtPos(const QPointF &);
	
	protected:
	virtual void mouseDoubleClickEvent(QMouseEvent *);
	virtual void contextMenuEvent(QContextMenuEvent *);
	virtual void wheelEvent(QWheelEvent *);
	virtual void focusInEvent(QFocusEvent *);
	virtual void keyPressEvent(QKeyEvent *);
	virtual void keyReleaseEvent(QKeyEvent *);
	virtual bool event(QEvent *);
	virtual bool switchToVisualisationModeIfNeeded(QInputEvent *e);
	virtual bool switchToSelectionModeIfNeeded(QInputEvent *e);
	virtual bool isCtrlShifting(QInputEvent *);
	virtual bool selectedItemHasFocus();
	
	private:
	void mousePressEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);
	void mouseReleaseEvent(QMouseEvent *);
	void dragEnterEvent(QDragEnterEvent *);
	void dragLeaveEvent(QDragLeaveEvent *);
	void dragMoveEvent(QDragMoveEvent *);
	void dropEvent(QDropEvent *);
	void handleElementDrop(QDropEvent *);
	void handleTitleBlockDrop(QDropEvent *);
	void handleTextDrop(QDropEvent *);
	QRectF viewedSceneRect() const;
	bool mustIntegrateElement(const ElementsLocation &) const;
	bool mustIntegrateTitleBlockTemplate(const TitleBlockTemplateLocation &) const;
	bool addElementAtPos(const ElementsLocation &, const QPoint &);
	
	signals:
	/// Signal emitted after the selection changed
	void selectionChanged();
	/// Signal emitted after the selection mode changed
	void modeChanged();
	/// Signal emitted after the diagram title changed
	void titleChanged(DiagramView *, const QString &);
	/// Signal emitted before integrating an element
	void aboutToAddElement();
	/// Signal emitted before integrating a title block template
	void aboutToSetDroppedTitleBlockTemplate(const TitleBlockTemplateLocation &);
	/// Signal emitted when users wish to locate an element from the diagram within elements collection
	void findElementRequired(const ElementsLocation &);
	/// Signal emitted when users wish to edit an element from the diagram
	void editElementRequired(const ElementsLocation &);
	/// Signal emitted when users want to edit and/or duplicate an existing title block template
	void editTitleBlockTemplate(const QString &, bool);
	/// Signal emitted after an item is added
	void itemAdded();
	/// Signal emmitted fater windows selection image have been canceled
	void ImageAddedCanceled(bool);
	/// Signal emmitted when diagram must be show
	void showDiagram (Diagram *);
	
	public slots:
	void selectNothing();
	void selectAll();
	void selectInvert();
	void deleteSelection();
	void rotateSelection();
	void rotateTexts();
	void setVisualisationMode();
	void setSelectionMode();
	void zoomIn();
	void zoomOut();
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
	void editConductor();
	void editConductor(Conductor *);
	void editConductorColor(Conductor *);
	void resetConductors();
	
	private slots:
	void addDroppedElement();
	void setDroppedTitleBlockTemplate(const TitleBlockTemplateLocation &);
	void adjustGridToZoom();
	void applyReadOnly();
};
#endif
