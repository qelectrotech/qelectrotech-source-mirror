/*
    Copyright 2006-2026 The QElectroTech Team
    This file is part of QElectroTech.

    QElectroTech is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    QElectroTech is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef QETSVG_H
#define QETSVG_H

#include <QDomElement>

class QDomDocument;
class QPointF;
class QRectF;

/**
 * @namespace QETSVG
 * @brief the QETSVG namespace provide function read and write svg.
 * Some function work on xml element (ex rect) and some other
 * work on attribute of an element (ex x)
 */
namespace QETSVG
{
    QDomElement rectToElmt(const QRectF &rect, QDomDocument &parent_document);
    QRectF rectFromElmt(const QDomElement &xml_element);

    void yToAttribute(const qreal &y, QDomElement &xml_element);
    qreal yFromAttribute(const QDomElement &xml_element, const qreal &def_value=0);

    void heightToAttribute(const qreal &height, QDomElement &xml_element);
    qreal heightFromAttribute(const QDomElement &xml_element, const qreal &def_value=10);

    void rToAttribute(const qreal &r, QDomElement &xml_element);
    qreal rFromAttribute(const QDomElement &xml_element, const qreal &def_value=1);

    void pointsToAttribute(const QVector<QPointF> &points, QDomElement &xml_element);
    QVector<QPointF> pointsFromAttribute (const QDomElement &xml_element);
}

#endif // QETSVG_H
