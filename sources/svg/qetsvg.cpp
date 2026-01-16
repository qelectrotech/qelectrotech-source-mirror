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

#include "qetsvg.h"

#include <QDomDocument>
#include <QRect>
#include <../qet.h>

/**
 * @brief QETSVG::rectToElmt
 * Write a QRect as a svg rect element.
 * @param rect
 * @param parent_document
 * @return
 */
QDomElement QETSVG::rectToElmt(const QRectF &rect, QDomDocument &parent_document)
{
    auto dom_element = parent_document.createElement(QStringLiteral("rect"));
    if (!rect.isNull()) {
        dom_element.setAttribute(QStringLiteral("x"), rect.x());
        yToAttribute(rect.y(), dom_element);
        dom_element.setAttribute(QStringLiteral("width"), rect.width());
        heightToAttribute(rect.height(), dom_element);
    }
    return dom_element;
}

/**
 * @brief QETSVG::rectFromElmt
 * @param xml_element : xml_element of an svg rect.
 * The tag name must be 'rect' if not, the returned QRect is null.
 * @return a svg rect to QRect
 */
QRectF QETSVG::rectFromElmt(const QDomElement &xml_element)
{
    QRectF rect_;
    if (xml_element.tagName() == QLatin1String("rect")) {
        rect_.setRect(xml_element.attribute(QStringLiteral("x"), QStringLiteral("0")).toDouble(),
                      yFromAttribute(xml_element, 0),
                      xml_element.attribute(QStringLiteral("width"), QStringLiteral("10")).toDouble(),
                      heightFromAttribute(xml_element, 10));
    }
    return rect_;
}

/**
 * @brief QETSVG::yToAttribute
 * @param y
 * @param xml_element
 */
void QETSVG::yToAttribute(const qreal &y, QDomElement &xml_element) {
    xml_element.setAttribute(QStringLiteral("y"), QString::number(y));
}

/**
 * @brief QETSVG::yFromAttribute
 * @param xml_element
 * @param def_value
 * @return
 */
qreal QETSVG::yFromAttribute(const QDomElement &xml_element, const qreal &def_value) {
    qreal value_;
    if (QET::attributeIsAReal(xml_element, QStringLiteral("y"), &value_)) {
        return value_;
    }
    return def_value;
}

/**
 * @brief QETSVG::heightToAttribute
 * @param height
 * @param xml_element
 */
void QETSVG::heightToAttribute(const qreal &height, QDomElement &xml_element) {
    xml_element.setAttribute(QStringLiteral("height"), QString::number(height));
}

qreal QETSVG::heightFromAttribute(const QDomElement &xml_element, const qreal &def_value) {
    qreal value_;
    if (QET::attributeIsAReal(xml_element, QStringLiteral("height"), &value_)) {
        return value_;
    }
    return def_value;
}

void QETSVG::rToAttribute(const qreal &r, QDomElement &xml_element) {
    xml_element.setAttribute(QStringLiteral("r"), QString::number(r));
}

qreal QETSVG::rFromAttribute(const QDomElement &xml_element, const qreal &def_value) {
    qreal value_;
    if (QET::attributeIsAReal(xml_element, QStringLiteral("r"), &value_)) {
        return value_;
    }
    return def_value;

}

void QETSVG::pointsToAttribute(const QVector<QPointF> &points, QDomElement &xml_element)
{
        QStringList strl_;
        for (const auto &point : points) {
            strl_.append(QString::number(point.x()) +
                        QString(",") +
                        QString::number(point.y()));
        }

        xml_element.setAttribute(QStringLiteral("points"), strl_.join(" "));
}

/**
 * @brief QETSVG::pointsFromAttribute
 * @param xml_element
 * @return a vector of points stored in attribute 'points' of @xml_element.
 * The returned vector can be empty.
 */
QVector<QPointF> QETSVG::pointsFromAttribute(const QDomElement &xml_element)
{
    QVector<QPointF> vector_;
    if (const auto string_points = xml_element.attribute(QStringLiteral("points")).split(QStringLiteral(" ")) ;
        !string_points.isEmpty()) {
        bool x_ok, y_ok;    for (const auto &point : string_points) {
            const auto string_x_y = point.split(QStringLiteral(","));
            if (string_x_y.size() == 2) {
                const auto x = string_x_y[0].toDouble(&x_ok);
                const auto y = string_x_y[1].toDouble(&y_ok);
                if (x_ok && y_ok) {
                    vector_.append(QPointF{x,y});
                }
            }
        }
    }

    return vector_;
}
