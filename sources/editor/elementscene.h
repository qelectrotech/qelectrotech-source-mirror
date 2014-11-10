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
#ifndef ELEMENT_SCENE_H
#define ELEMENT_SCENE_H
#include <QtGui>
#include <QtXml>
#include "nameslistwidget.h"
#include "qgimanager.h"
#include "elementcontent.h"
#include "diagramcontext.h"

class CustomElementPart;
class ElementEditionCommand;
class ElementPrimitiveDecorator;
class QETElementEditor;
class ESEventInterface;
class QKeyEvent;
/**
	This class is the canvas allowing the visual edition of an electrial element.
	It displays the various primitives composing the drawing of the element, the
	border due to its fixed size and its hotspot.
*/
class ElementScene : public QGraphicsScene {
	friend class ChangePropertiesCommand;
	Q_OBJECT
	
	// enum
	public:
	enum Behavior { Normal, PasteArea };
	enum ItemOption {
		SortByZValue = 1,
		IncludeTerminals = 2,
		IncludeHelperItems = 4,
		Selected = 8,
		NonSelected = 16,
		SelectedOrNot = 24
	};
	Q_DECLARE_FLAGS(ItemOptions, ItemOption)
	
	// constructors, destructor
	public:
	ElementScene(QETElementEditor *, QObject * = 0);
	virtual ~ElementScene();
	
	private:
	ElementScene(const ElementScene &);
	
	// attributes
	private:
		/// List of localized names
		NamesList _names;
		/// Extra informations
		QString informations_;
		/// element type
		QString m_elmt_type;
		/// element kind info
		DiagramContext m_elmt_kindInfo;
		/// QGraphicsItem manager
		QGIManager qgi_manager;
		/// Undo stack
		QUndoStack undo_stack;
		/**
			fsi_pos (first selected item pos) : Position of the forst selected item: used
			to cancel mouse movements; also used to handle movements using keybard
			arrwows.
		*/
		QPointF fsi_pos;
		QPointF moving_press_pos;
	
		/// Variables related to drawing
		ESEventInterface *m_event_interface;
		Behavior behavior;
		QETElementEditor *element_editor;
	
		/// Variables to manage the paste area on the scene
		QGraphicsRectItem *paste_area_;
		QRectF defined_paste_area_;
	
		/// Variables to handle copy/paste with offset
		QString last_copied_;
	
		/// Decorator item displayed when at least one item is selected
		ElementPrimitiveDecorator *decorator_;
	
		///< Size of the horizontal grid step
		int x_grid;
		///< Size of the vertical grid step
		int y_grid;
	
	// methods
	public:
		void setEventInterface (ESEventInterface *interface);
		QPointF snapToGrid(QPointF point);
	void setNames(const NamesList &);
	NamesList names() const;
	QString informations() const;
	void setInformations(const QString &);
	QString elementType () const {return m_elmt_type;}
	DiagramContext elementKindInfo () const {return m_elmt_kindInfo;}
	virtual int xGrid() const;
	virtual int yGrid() const;
	virtual void setGrid(int, int);
	virtual const QDomDocument toXml(bool = true);
	virtual QRectF boundingRectFromXml(const QDomDocument &);
	virtual void fromXml(const QDomDocument &, const QPointF & = QPointF(), bool = true, ElementContent * = 0);
	virtual void reset();
	virtual QList<CustomElementPart *> primitives() const;
	virtual QList<QGraphicsItem *> zItems(ItemOptions options = ItemOptions(SortByZValue | IncludeTerminals | SelectedOrNot)) const;
	virtual ElementContent selectedContent() const;
	virtual void getPasteArea(const QRectF &);
	QRectF elementSceneGeometricRect () const;
	bool containsTerminals() const;
	QUndoStack &undoStack();
	QGIManager &qgiManager();
	static bool clipboardMayContainElement();
	bool wasCopiedFromThisElement(const QString &);
	void cut();
	void copy();
	void contextMenu (QContextMenuEvent *event);
	QETElementEditor* editor() const;
	
	protected:
		virtual void mouseMoveEvent         (QGraphicsSceneMouseEvent *);
		virtual void mousePressEvent       (QGraphicsSceneMouseEvent *);
		virtual void mouseReleaseEvent     (QGraphicsSceneMouseEvent *);
		virtual void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event);
		virtual void keyPressEvent         (QKeyEvent *event);

	virtual void drawForeground(QPainter *, const QRectF &);
	
	private:
	QRectF elementContentBoundingRect(const ElementContent &) const;
	bool applyInformations(const QDomDocument &, QString * = 0);
	ElementContent loadContent(const QDomDocument &, QString * = 0);
	ElementContent addContent(const ElementContent &, QString * = 0);
	ElementContent addContentAtPos(const ElementContent &, const QPointF &, QString * = 0);
	void addPrimitive(QGraphicsItem *);
	void initPasteArea();
	bool mustSnapToGrid(QGraphicsSceneMouseEvent *);
	static bool zValueLessThan(QGraphicsItem *, QGraphicsItem *);
	QMutex *decorator_lock_;
	void centerElementToOrigine();
	
	public slots:
	void slot_move();
	void slot_select(const ElementContent &);
	void slot_selectAll();
	void slot_deselectAll();
	void slot_invertSelection();
	void slot_delete();
	void slot_editNames();
	void slot_editAuthorInformations();
	void slot_editProperties();
	void slot_bringForward();
	void slot_raise();
	void slot_lower();
	void slot_sendBackward();
	void managePrimitivesGroups();
	void stackAction(ElementEditionCommand *);
	
	signals:
	/// Signal emitted after one or several parts were added
	void partsAdded();
	/// Signal emitted after one or several parts were removed
	void partsRemoved();
	/// Signal emitted when the zValue of one or several parts change
	void partsZValueChanged();
	/// Signal emitted when users have defined the copy/paste area
	void pasteAreaDefined(const QRectF &);
	/// Signal emitted when need zoomFit
	void needZoomFit();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ElementScene::ItemOptions)


/**
	@param nameslist New set of naes for the currently edited element
*/
inline void ElementScene::setNames(const NamesList &nameslist) {
	_names = nameslist;
}

/**
	@return the list of names of the currently edited element
*/
inline NamesList ElementScene::names() const {
	return(_names);
}

/**
	@return extra informations of the currently edited element
*/
inline QString ElementScene::informations() const {
	return(informations_);
}

/**
	@param infos new extra information for the currently edited element
*/
inline void ElementScene::setInformations(const QString &infos) {
	informations_ = infos;
}

#endif
