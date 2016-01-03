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
#include "qetxml.h"
#include <QPen>

/**
 * @brief QETXML::penToXml
 * Write attribute of a QPen in xml element
 * @param parent_document : parent document for create the QDomElement
 * @param pen : the pen to store
 * @return : A QDomElement with the attribute stored. The tagName of QDomeElement is "pen".
 */
QDomElement QETXML::penToXml(QDomDocument &parent_document,QPen pen)
{
	QDomElement element = parent_document.createElement("pen");

	QString style;
	switch(pen.style())
	{
		case Qt::SolidLine      : style = "SolidLine";      break;
		case Qt::DashLine       : style = "DashLine";       break;
		case Qt::DotLine        : style = "DotLine";        break;
		case Qt::DashDotLine    : style = "DashDotLine";    break;
		case Qt::DashDotDotLine : style = "DashDotDotLine"; break;
		default                 : style = "Unknow";         break;
	}

	element.setAttribute("style", style);
	element.setAttribute("color", pen.color().name());
	element.setAttribute("widthF", pen.widthF());
    return element;
}

/**
 * @brief QETXML::penFromXml
 * Build a QPen from a xml description
 * @param element, The QDomElement that describe the pen
 * @return the created pen. If @element is null or tagName isn't "pen"
 * return a default constructed QPen
 */
QPen QETXML::penFromXml(const QDomElement &element)
{
    QPen pen;

	if (!(!element.isNull() && element.tagName() == "pen")) return pen;

	QString style = element.attribute("style", "DashLine");
	if      (style == "SolidLine")      pen.setStyle(Qt::SolidLine);
	else if (style == "DashLine")       pen.setStyle(Qt::DashLine);
	else if (style == "DotLine")        pen.setStyle(Qt::DotLine);
	else if (style == "DashDotLine")    pen.setStyle(Qt::DashDotLine);
	else if (style == "DashDotDotLine") pen.setStyle(Qt::DashDotDotLine);
	else                                pen.setStyle(Qt::DashLine);

	pen.setColor(QColor(element.attribute("color", "#000000")));
	pen.setWidthF(element.attribute("widthF", "1").toDouble());
    return pen;
}
