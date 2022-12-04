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
#include "slaveelement.h"

#include "../diagram.h"
#include "../diagramposition.h"
#include "../qetapp.h"
#include "dynamicelementtextitem.h"

/**
	@brief SlaveElement::SlaveElement
	Default constructor
	@param location location of xml definition
	@param qgi parent QGraphicItem
	@param state int used to know if the creation of element have error
*/
SlaveElement::SlaveElement(const ElementsLocation &location,
			   QGraphicsItem *qgi,
			   int *state) :
	Element(location, qgi, state, Element::Slave)
{
	m_xref_item = nullptr;
}

/**
	@brief SlaveElement::~SlaveElement
	default destructor
*/
SlaveElement::~SlaveElement()
{
	unlinkAllElements();
}

/**
	@brief SlaveElement::linkToElement
	Link this slave to another element
	For this class element must be a master
	@param elmt
*/
void SlaveElement::linkToElement(Element *elmt)
{
		// check if element is master and if isn't already linked
	if (elmt->linkType() == Master && !connected_elements.contains(elmt))
	{
		if(!isFree())
			unlinkAllElements();

		connected_elements << elmt;

		elmt -> linkToElement(this);
		emit linkedElementChanged();
	}
}

/**
	@brief SlaveElement::unlinkAllElements
	Unlink all of the element in the QList connected_elements
*/
void SlaveElement::unlinkAllElements()
{
		// if this element is free no need to do something
	if (!isFree())
	{
		foreach(Element *elmt, connected_elements)
			unlinkElement(elmt);
		emit linkedElementChanged();
	}
}

/**
	@brief SlaveElement::unlinkElement
	Unlink the given element in parameter
	@param elmt
*/
void SlaveElement::unlinkElement(Element *elmt)
{
		//Ensure elmt is linked to this element
	if (connected_elements.contains(elmt))
	{
		connected_elements.removeOne(elmt);

		elmt -> unlinkElement  (this) ;
		elmt -> setHighlighted (false);
		emit linkedElementChanged();
	}
}
