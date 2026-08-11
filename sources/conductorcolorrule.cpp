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
#include "conductorcolorrule.h"

/**
	@brief ConductorColorRule::toXml
	@param xml_document : document used to create the returned element
	@return a "color_rule" element. The caller sets the "text" attribute
	(the key this rule is stored under) -- see QETProject::
	writeDefaultPropertiesXml(), which mirrors how XRefProperties::toXml()
	and its "type" attribute are handled the same way.
*/
QDomElement ConductorColorRule::toXml(QDomDocument &xml_document) const
{
	QDomElement e = xml_document.createElement(QStringLiteral("color_rule"));
	e.setAttribute(QStringLiteral("color"),   color.name());
	e.setAttribute(QStringLiteral("bicolor"), m_bicolor ? QStringLiteral("true") : QStringLiteral("false"));
	e.setAttribute(QStringLiteral("color2"),  color_2.name());
	return e;
}

/**
	@brief ConductorColorRule::fromXml
	@param xml_element
*/
void ConductorColorRule::fromXml(const QDomElement &xml_element)
{
	const QColor xml_color(xml_element.attribute(QStringLiteral("color")));
	color = xml_color.isValid() ? xml_color : QColor(Qt::black);

	m_bicolor = xml_element.attribute(QStringLiteral("bicolor")) == QLatin1String("true");

	const QColor xml_color_2(xml_element.attribute(QStringLiteral("color2")));
	color_2 = xml_color_2.isValid() ? xml_color_2 : QColor(Qt::black);
}

bool ConductorColorRule::operator==(const ConductorColorRule &other) const
{
	return color == other.color && color_2 == other.color_2 && m_bicolor == other.m_bicolor;
}

bool ConductorColorRule::operator!=(const ConductorColorRule &other) const
{
	return !(*this == other);
}

/**
	@brief ConductorColorRule::defaultRules
	@return see the declaration's doc comment
*/
QHash<QString, ConductorColorRule> ConductorColorRule::defaultRules()
{
	return {
		{QStringLiteral("L1"), ConductorColorRule(QColor(0x8B, 0x45, 0x13), false)},
		{QStringLiteral("L2"), ConductorColorRule(QColor(Qt::black),         false)},
		{QStringLiteral("L3"), ConductorColorRule(QColor(Qt::gray),          false)},
		{QStringLiteral("N"),  ConductorColorRule(QColor(Qt::blue),          false)},
		{QStringLiteral("PE"), ConductorColorRule(QColor(0x00, 0x80, 0x00), true, QColor(0xFF, 0xD7, 0x00))},
	};
}
