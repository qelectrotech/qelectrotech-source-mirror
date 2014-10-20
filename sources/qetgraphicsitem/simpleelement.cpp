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
#include "simpleelement.h"
/**
 * @brief SimpleElement::SimpleElement
 * @param location
 * @param qgi
 * @param s
 * @param state
 */
SimpleElement::SimpleElement(const ElementsLocation &location, QGraphicsItem *qgi, Diagram *s, int *state) :
	CustomElement(location, qgi, s, state)
{
	link_type_ = Simple;
	connect(this, SIGNAL(elementInfoChange(DiagramContext)), this, SLOT(updateLabel()));
}

/**
 * @brief SimpleElement::~SimpleElement
 */
SimpleElement::~SimpleElement() {
	disconnect(this, SIGNAL(elementInfoChange(DiagramContext)), this, SLOT(updateLabel()));
}

/**
 * @brief SimpleElement::updateLabel
 * update label of this element
 */
void SimpleElement::updateLabel() {
	QString label = elementInformations()["label"].toString();
	bool	show  = elementInformations().keyMustShow("label");

	// setup the label
	(label.isEmpty() || !show)?
				setTaggedText("label", "_", false):
				setTaggedText("label", label, true);
}
