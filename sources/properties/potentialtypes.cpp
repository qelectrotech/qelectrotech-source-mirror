// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "potentialtypes.h"

#include <QDomDocument>

bool PotentialType::operator==(const PotentialType &other) const
{
	return name == other.name
			&& color == other.color
			&& style == other.style
			&& qFuzzyCompare(width + 1, other.width + 1);
}

/**
	@brief PotentialTypes::defaultTypes
	@return the types a new project starts with: the phase and neutral
	conductors, the protective earth and the two DC rails, in the colours
	of IEC 60445 as far as a drawing colour can tell them apart.
*/
QList<PotentialType> PotentialTypes::defaultTypes()
{
	return {
		{QStringLiteral("L"),    QColor(Qt::black),         Qt::SolidLine,   1.0},
		{QStringLiteral("N"),    QColor(0, 70, 220),        Qt::DashLine,    1.0},
		{QStringLiteral("PE"),   QColor(60, 160, 40),       Qt::DashDotLine, 1.0},
		{QStringLiteral("24V"),  QColor(200, 30, 30),       Qt::SolidLine,   1.0},
		{QStringLiteral("0V"),   QColor(0, 70, 220),        Qt::SolidLine,   1.0}
	};
}

/**
	@brief PotentialTypes::find
	@param name : the name a conductor carries
	@param found : if not null, the type is copied there when found
	@return true if the project has a type of that name
*/
bool PotentialTypes::find(const QString &name, PotentialType *found) const
{
	if (name.isEmpty())
		return false;
	for (const PotentialType &type : m_types) {
		if (type.name == name) {
			if (found)
				*found = type;
			return true;
		}
	}
	return false;
}

/**
	@return true when the project holds nothing different from a new one,
	so there is nothing to write
*/
bool PotentialTypes::isDefault() const
{
	return !m_enabled && m_types == defaultTypes();
}

bool PotentialTypes::operator==(const PotentialTypes &other) const
{
	return m_enabled == other.m_enabled && m_types == other.m_types;
}

QDomElement PotentialTypes::toXml(QDomDocument &document) const
{
	QDomElement root = document.createElement(QStringLiteral("potentialtypes"));
	root.setAttribute(QStringLiteral("enabled"), m_enabled ? 1 : 0);
	for (const PotentialType &type : m_types) {
		QDomElement elmt = document.createElement(QStringLiteral("potentialtype"));
		elmt.setAttribute(QStringLiteral("name"), type.name);
		elmt.setAttribute(QStringLiteral("color"), type.color.name());
		elmt.setAttribute(QStringLiteral("style"), static_cast<int>(type.style));
		elmt.setAttribute(QStringLiteral("width"), QString::number(type.width));
		root.appendChild(elmt);
	}
	return root;
}

/**
	@brief PotentialTypes::fromXml
	@param element : the <potentialtypes> element, may be null
	@return the types it holds. A null element gives a project without
	the feature: off, with the default list.
*/
PotentialTypes PotentialTypes::fromXml(const QDomElement &element)
{
	PotentialTypes result;
	if (element.isNull())
		return result;

	result.m_enabled = element.attribute(QStringLiteral("enabled")).toInt() != 0;
	result.m_types.clear();
	for (QDomElement elmt = element.firstChildElement(QStringLiteral("potentialtype"));
		 !elmt.isNull();
		 elmt = elmt.nextSiblingElement(QStringLiteral("potentialtype")))
	{
		PotentialType type;
		type.name = elmt.attribute(QStringLiteral("name"));
		if (type.name.isEmpty())
			continue;
		const QColor color(elmt.attribute(QStringLiteral("color")));
		type.color = color.isValid() ? color : QColor(Qt::black);
		const int style = elmt.attribute(QStringLiteral("style"),
										 QString::number(Qt::SolidLine)).toInt();
		type.style = (style > Qt::NoPen && style <= Qt::DashDotDotLine)
				? static_cast<Qt::PenStyle>(style) : Qt::SolidLine;
		bool ok = false;
		const qreal width = elmt.attribute(QStringLiteral("width")).toDouble(&ok);
		type.width = (ok && width > 0) ? width : 1.0;
		result.m_types << type;
	}
	return result;
}
