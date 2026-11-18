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
#include "slaveelement.h"

#include <diagram.h>
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
{}

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
		//The stored list, not linkedElements(): a master whose folio
		//is away is left out of that one, so the check would never see
		//the link it is standing in -- and these two functions call
		//each other, which ends in a stack overflow
	if (elmt->linkType() == Master && !connected_elements.contains(elmt))
	{
		//The stored partners, not the ones to show: taking another
		//master has to drop what is kept here, whichever folio it
		//stands on, or it stays behind as a dangling pointer
		if(!connected_elements.isEmpty())
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
		//("free" as in nothing stored: a partner whose folio is away
		//still has to be unlinked here, or the element it points at
		//is freed while this list goes on holding it)
	if (!connected_elements.isEmpty())
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
