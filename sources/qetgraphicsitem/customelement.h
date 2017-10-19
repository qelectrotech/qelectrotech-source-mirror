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
		CustomElement (const ElementsLocation &, QGraphicsItem * = nullptr, int * = nullptr);

	~CustomElement() override;
	
	private:
		CustomElement(const CustomElement &);
	
		// attributes
	protected:
		NamesList names;
		ElementsLocation location_;
		QPicture drawing;
		QPicture low_zoom_drawing;
		QList<Terminal *> m_terminals;
		QList<ElementTextItem *> m_texts;
		QList<DynamicElementTextItem *> m_dynamic_texts;
		bool forbid_antialiasing;


		QList<QLineF *> m_lines;
		QList<QRectF *> m_rectangles;
		QList<QRectF *> m_circles;
		QList<QVector<QPointF> *> m_polygons;
		QList<QVector<qreal> *> m_arcs;
	
	// methods
	public:
		QList<Terminal *> terminals() const override;
		QList<Conductor *> conductors() const override;
		QList<ElementTextItem *> texts() const override;
		QList<QLineF *> lines() const override;
		QList<QRectF *> rectangles() const override;
		QList<QRectF *> circles() const override;
		QList<QVector<QPointF> *> polygons() const override;
		QList<QVector<qreal> *> arcs() const override;
		int terminalsCount() const override;
		void paint(QPainter *, const QStyleOptionGraphicsItem *) override;
		QString typeId() const override;
		ElementsLocation location() const;
		QString name() const override;
		ElementTextItem* taggedText(const QString &tagg) const override;
	
	protected:
		virtual bool buildFromXml(const QDomElement &, int * = nullptr);
		virtual bool parseElement(QDomElement &, QPainter &);
		virtual bool parseLine(QDomElement &, QPainter &);
		virtual bool parseRect(QDomElement &, QPainter &);
		virtual bool parseEllipse(QDomElement &, QPainter &);
		virtual bool parseCircle(QDomElement &, QPainter &);
		virtual bool parseArc(QDomElement &, QPainter &);
		virtual bool parsePolygon(QDomElement &, QPainter &);
		virtual bool parseText(QDomElement &, QPainter &);
		virtual bool parseInput(QDomElement &);
		virtual DynamicElementTextItem *parseDynamicText(QDomElement &);
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
