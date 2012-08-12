/*
	Copyright 2006-2012 Xavier Guerrin
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
class Conductor;
class Diagram;
class Element;
class IndependentTextItem;
class QETDiagramEditor;
/**
	Classe representant graphiquement un schema electrique
*/
class DiagramView : public QGraphicsView {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	DiagramView(Diagram * = 0, QWidget * = 0);
	virtual ~DiagramView();
	
	private:
	DiagramView(const DiagramView &);
	
	// attributs
	private:
	Diagram *scene;
	QMenu *context_menu;
	QAction *paste_here;
	QAction *find_element_;
	QPoint paste_here_pos;
	bool is_adding_text;
	bool is_moving_view_;               ///< Indicate whether the visualisation mode has been enabled due to mouse/keyboard interactions
	bool fresh_focus_in_;               ///< Indicate the focus was freshly gained
	ElementsLocation next_location_;
	QPoint next_position_;
	
	// methodes
	public:
	QString title() const;
	void editDiagramProperties();
	void addColumn();
	void removeColumn();
	void addRow();
	void removeRow();
	/// @return Le schema visualise par ce DiagramView
	Diagram *diagram() { return(scene); }
	QETDiagramEditor *diagramEditor() const;
	bool hasSelectedItems();
	bool hasCopiableItems();
	bool hasDeletableItems();
	void addText();
	IndependentTextItem *addDiagramTextAtPos(const QPointF &);
	
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
	void dragEnterEvent(QDragEnterEvent *);
	void dragLeaveEvent(QDragLeaveEvent *);
	void dragMoveEvent(QDragMoveEvent *);
	void dropEvent(QDropEvent *);
	void handleElementDrop(QDropEvent *);
	void handleTitleBlockDrop(QDropEvent *);
	QRectF viewedSceneRect() const;
	bool mustIntegrateElement(const ElementsLocation &) const;
	bool mustIntegrateTitleBlockTemplate(const TitleBlockTemplateLocation &) const;
	bool addElementAtPos(const ElementsLocation &, const QPoint &);
	
	signals:
	/// Signal emis lorsque la selection change
	void selectionChanged();
	/// Signal emis lorsque le mode de selection change
	void modeChanged();
	/// Signal emis lorsqu'un texte a ete pose
	void textAdded(bool);
	/// Signal emis lorsque le titre du schema change
	void titleChanged(DiagramView *, const QString &);
	/// Signal emis avant l'integration d'un element
	void aboutToAddElement();
	/// Signal emitted before integrating a title block template
	void aboutToSetDroppedTitleBlockTemplate(const TitleBlockTemplateLocation &);
	/// Signal emis lorsque l'utilisateur souhaite retrouver un element du schema dans les collections
	void findElementRequired(const ElementsLocation &);
	/// Signal emis lorsque l'utilisateur souhaite editer un element du schema
	void editElementRequired(const ElementsLocation &);
	/// Signal emitted when the user wants to edit and/or duplicate an existing title block template
	void editTitleBlockTemplate(const QString &, bool);
	
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
	void editElement(Element *);
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
