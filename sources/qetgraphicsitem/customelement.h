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
#ifndef CUSTOM_ELEMENT_H
#define CUSTOM_ELEMENT_H
#include "fixedelement.h"
#include <QtGui>
#include "nameslist.h"
#include "elementslocation.h"
class ElementTextItem;
class Terminal;
/**
	This class represents an electrical element; it may be used like a fixed
	element, the difference being that the CustomElement reads its description
	(names, drawing, behavior) from an XML document.
*/
class CustomElement : public FixedElement {
	
	Q_OBJECT
	
	// constructors, destructor
	public:
	CustomElement(const ElementsLocation &, QGraphicsItem * = 0, Diagram * = 0, int * = 0);
	virtual ~CustomElement();
	
	private:
	CustomElement(const CustomElement &);
	
	// attributes
	protected:
	int elmt_state; // hold the error code in case the instanciation fails, or 0 if everything went well
	NamesList names;
	ElementsLocation location_;
	QPicture drawing;
	QPicture low_zoom_drawing;
	QList<Terminal *> list_terminals;
	QList<ElementTextItem *> list_texts_;
	bool forbid_antialiasing;
	
	// methods
	public:
	virtual QList<Terminal *> terminals() const;
	virtual QList<Conductor *> conductors() const;
	virtual QList<ElementTextItem *> texts() const;
	virtual int terminalsCount() const;
	virtual void paint(QPainter *, const QStyleOptionGraphicsItem *);
	QString typeId() const;
	ElementsLocation location() const;
	bool isNull() const;
	int state() const;
	QString name() const;
	
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
	@return true if this element is null, i.e. if its XML description could not
	be loaded.
*/
inline bool CustomElement::isNull() const {
	return(elmt_state);
}

/**
	@return An integer representing the state of this element:
		- 0: instantiation succeeded
		- 1: the file does not exist
		- 2: the file could not be opened
		- 3: The file is not a valid XML document
		- 4: The XML document does not have a "definition" root element.
		- 5: The definition attributes are missing or invalid
		- 6: The definition is empty
		- 7: The parsing of an XML element describing an element drawing primitive failed
		- 8: No primitive could be loadedAucune partie du dessin n'a pu etre chargee
*/
inline int CustomElement::state() const {
	return(elmt_state);
}

/**
	@return The name of this element.
*/
inline QString CustomElement::name() const {
	return(names.name(location_.baseName()));
}

#endif
