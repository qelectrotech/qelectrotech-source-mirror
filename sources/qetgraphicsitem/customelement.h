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
#ifndef CUSTOM_ELEMENT_H
#define CUSTOM_ELEMENT_H
#include "fixedelement.h"
#include "nameslist.h"
#include "elementslocation.h"
#include <QPicture>

class ElementTextItem;
class Terminal;

/**
	This class represents an electrical element; it may be used like a fixed
	element, the difference being that the CustomElement reads its description
	(names, drawing, behavior) from an XML document.
*/
class CustomElement : public FixedElement
{
	Q_OBJECT
	
		// constructors, destructor
	public:
		CustomElement (const ElementsLocation &, QGraphicsItem * = 0, int * = 0);

	virtual ~CustomElement();
	
	private:
		CustomElement(const CustomElement &);
	
		// attributes
	protected:
		NamesList names;
		ElementsLocation location_;
		QPicture drawing;
		QPicture low_zoom_drawing;
		QList<Terminal *> list_terminals;
		QList<ElementTextItem *> list_texts_;
		bool forbid_antialiasing;


	QList<QLineF *> list_lines_;
	QList<QRectF *> list_rectangles_;
	QList<QRectF *> list_circles_;
	QList<QVector<QPointF> *> list_polygons_;
	QList<QVector<qreal> *> list_arcs_;
	
	// methods
	public:
	virtual QList<Terminal *> terminals() const;
	virtual QList<Conductor *> conductors() const;
	virtual QList<ElementTextItem *> texts() const;
	virtual QList<QLineF *> lines() const;
	virtual QList<QRectF *> rectangles() const;
	virtual QList<QRectF *> circles() const;
	virtual QList<QVector<QPointF> *> polygons() const;
	virtual QList<QVector<qreal> *> arcs() const;
	virtual int terminalsCount() const;
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *);
	QString typeId() const;
	ElementsLocation location() const;
	QString name() const;
	ElementTextItem* taggedText(const QString &tagg) const;
	
	protected:
	virtual bool buildFromXml(const QDomElement &, int * = 0);
	virtual bool parseElement(QDomElement &, QPainter &);
	virtual bool parseLine(QDomElement &, QPainter &);
	virtual bool parseRect(QDomElement &, QPainter &);
	virtual bool parseEllipse(QDomElement &, QPainter &);
	virtual bool parseCircle(QDomElement &, QPainter &);
	virtual bool parseArc(QDomElement &, QPainter &);
	virtual bool parsePolygon(QDomElement &, QPainter &);
	virtual bool parseText(QDomElement &, QPainter &);
	virtual ElementTextItem *parseInput(QDomElement &);
	virtual Terminal *parseTerminal(QDomElement &);
	virtual void setQPainterAntiAliasing(QPainter &, bool);
	virtual bool validOrientationAttribute(const QDomElement &);
	virtual void setPainterStyle(QDomElement &, QPainter &);
	ElementTextItem* setTaggedText(const QString &tagg, const QString &newstr, const bool noeditable=false);
};

/**
	@return The element type ID; considering a CustomElement, this means the
	@location of its XML description.
	@see location()
*/
inline QString CustomElement::typeId() const {
	return(location_.path());
}

/**
	@return the location of the XML document describing this element.
*/
inline ElementsLocation CustomElement::location() const {
	return(location_);
}

/**
	@return The name of this element.
*/
inline QString CustomElement::name() const {
	return(names.name(location_.baseName()));
}

#endif
