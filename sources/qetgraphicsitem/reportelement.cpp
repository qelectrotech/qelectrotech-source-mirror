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
#include "reportelement.h"
#include "diagramposition.h"
#include "qetproject.h"
#include "diagram.h"
#include "terminal.h"
#include "conductor.h"
#include "dynamicelementtextitem.h"

ReportElement::ReportElement(const ElementsLocation &location, const QString& link_type,QGraphicsItem *qgi, int *state) :
	CustomElement(location, qgi, state)
{
	link_type == "next_report"? m_link_type=NextReport : m_link_type=PreviousReport;
	link_type == "next_report"? m_inverse_report=PreviousReport : m_inverse_report=NextReport;
}

/**
 * @brief ReportElement::~ReportElement
 * Destructor
 */
ReportElement::~ReportElement()
{
	unlinkAllElements();
	if (terminals().size())
		disconnect(terminals().first(), nullptr, nullptr, nullptr);
}

/**
 * @brief ReportElement::linkToElement
 * Link this element to the other element
 * @param elmt
 * element to be linked with this
 */
void ReportElement::linkToElement(Element * elmt)
{
	if (!diagram() && !elmt -> diagram())
	{
		qDebug() << "ReportElement : linkToElement : Unable to link this or element to link isn't in a diagram";
		return;
	}

		//ensure elmt isn't already linked
	bool i = true;
	if (!this -> isFree() && (connected_elements.first() == elmt))
		i = false;

		//ensure elmt is an inverse report of this element
	if ((elmt->linkType() == m_inverse_report) && i)
	{
		unlinkAllElements();
		connected_elements << elmt;
		elmt->linkToElement(this);
		emit linkedElementChanged();
	}
}

/**
 * @brief ReportElement::unLinkAllElements
 * Unlink all of the element in the QList connected_elements
 */
void ReportElement::unlinkAllElements()
{
	if (isFree())
		return;

	const QList <Element *> tmp_elmt = connected_elements;

	for (Element *elmt : tmp_elmt)
		connected_elements.removeAll(elmt);

	for(Element *elmt : tmp_elmt)
	{
		elmt -> setHighlighted(false);
		elmt -> unlinkAllElements();
	}
	
	emit linkedElementChanged();
}
/**
 * @brief ReportElement::unlinkElement
 *unlink the specified element.
 *for reportelement, they must be only one linked element, so we call
 *unlinkAllElements for clear the connected_elements list.
 * @param elmt
 */
void ReportElement::unlinkElement(Element *elmt) {
	Q_UNUSED (elmt);
	unlinkAllElements();
}
