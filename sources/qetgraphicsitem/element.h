/*
	Copyright 2006-2013 The QElectroTech Team
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
#ifndef ELEMENT_H
#define ELEMENT_H
#include <QtGui>
#include "terminal.h"
#include "qetgraphicsitem.h"
#include <QUuid>
class Diagram;
class ElementTextItem;


/**
	This is the base class for electrical elements.
*/
class Element : public QetGraphicsItem {
	
	Q_OBJECT
	
	// constructors, destructor
	public:
	Element(QGraphicsItem * = 0, Diagram * = 0);
	virtual ~Element();
	
	private:
	Element(const Element &);
	
	// attributes
	public:
	enum { Type = UserType + 1000 };
	// this enum is use to know the kind of element and
	// to use flag for element provider class
	enum {Simple = 1,
		  NextReport = 2,
		  PreviousReport = 4,
		  AllReport = 6,
		  Master = 8,
		  SlaveNO = 16,
		  SlaveNC = 32,
		  AllSlave = 48,
		  Bornier = 64};
	
	protected:
	QList <Element *> connected_elements;
	QList <QUuid> tmp_uuids_link;
	
	private:
	QSize   dimensions;
	QPoint  hotspot_coord;
	QPixmap preview;
	QUuid uuid_;
	
	// methods
	public:
	/**
		Enable the use of qgraphicsitem_cast to safely cast a QGraphicsItem into an
		Element.
		@return the QGraphicsItem type
	*/
	virtual int type() const { return Type; }
	
	// pure virtual methods to be defined in derived classes
	/// @return the list of terminals for this element
	virtual QList<Terminal *> terminals() const = 0;
	/// @return the list of conductors attached to this element
	virtual QList<Conductor *> conductors() const = 0;
	/// @return the list of text items attached to this element
	virtual QList<ElementTextItem *> texts() const = 0;
	/// @return the list of lines items in this element
	virtual QList<QLineF *> lines() const = 0;
	/// @return the list of rectangles items in this element
	virtual QList<QRectF *> rectangles() const = 0;
	/// @return the list of bounding rectangles for circles items in this element
	virtual QList<QRectF *> circles() const = 0;
	/// @return the list of polygons in this element
	virtual QList<QVector<QPointF> *> polygons() const = 0;	
	/// @return the list of arcs in this element
	virtual QList<QVector<qreal> *> arcs() const = 0;
	/// @return the current number of terminals of this element
	virtual int terminalsCount() const = 0;
	/// @return the minimum number of terminals for this element
	virtual int minTerminalsCount() const = 0;
	/// @return the maximum number of terminals for this element
	virtual int maxTerminalsCount() const = 0;

	// related method for link between element
	bool isFree () const;
	virtual void linkToElement(Element *) {}
	virtual void unlinkAllElements() {}
	virtual void unlinkElement(Element *elmt) {}
	void initLink(QETProject *);
	QList<Element *> linkedElements () const;

	/**
		Draw this element
	*/
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *) = 0;
	/// @return This element type ID
	virtual QString typeId() const = 0;
	/// @return the human name for this element
	virtual QString name() const = 0;
	/// @return the linkable type
	virtual int linkType() const = 0;
	
	virtual bool isHighlighted() const;
	virtual void setHighlighted(bool);
	void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
	QRectF boundingRect() const;
	QSize setSize(int, int);
	QSize size() const;
	QPixmap  pixmap();
	
	// methods related to the hotspot
	QPoint setHotspot(QPoint);
	QPoint hotspot() const;
	
	// selection-related methods
	void select();
	void deselect();	
	
	// methods related to internal connections
	bool internalConnections();
	void setInternalConnections(bool);
	virtual void rotateBy(const qreal &);
	virtual void editProperty();
	
	// methods related to XML import/export
	static bool valideXml(QDomElement &);
	virtual bool fromXml(QDomElement &, QHash<int, Terminal *> &, bool = false);
	virtual QDomElement toXml(QDomDocument &, QHash<Terminal *, int> &) const;
	QUuid uuid() const;
	
	// orientation-related methods
	int orientation() const;
	
	protected:
	void drawAxes(QPainter *, const QStyleOptionGraphicsItem *);
	
	private:
	bool internal_connections_;
	bool must_highlight_;
	void drawSelection(QPainter *, const QStyleOptionGraphicsItem *);
	void drawHighlight(QPainter *, const QStyleOptionGraphicsItem *);
	void updatePixmap();
};

inline bool Element::isFree() const {
	return (connected_elements.isEmpty());
}

/**
	Indicate whether this element allows internal connections, i.e. whether its
	terminals can be linked together using a conductor.
	@return true if internal connections are accepted, false otherwise
*/
inline bool Element::internalConnections() {
	return(internal_connections_);
}

/**
	Specify whether this element allows internal connections, i.e. whether its
	terminals can be linked together using a conductor.
	@return true for internal connections to be accepted, false otherwise
*/
inline void Element::setInternalConnections(bool ic) {
	internal_connections_ = ic;
}

/**
	Indicate the current orientation of this element
	O = 0°
	1 = 90°
	2 = 180°
	3 = 270°
	@return the current orientation of this element
*/
inline int Element::orientation() const {
	return(QET::correctAngle(rotation())/90);
}

/**
 * @brief Element::uuid
 * @return the uuid of this element
 */
inline QUuid Element::uuid() const {
	return uuid_;
}

inline QList <Element *> Element::linkedElements() const {
	return connected_elements;
}

#endif
