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
#include "terminallabel.h"

#include "terminaldata.h"

#include <qet.h>
#include <qetapp.h>
#include <qetxml.h>
#include <utils/qetutils.h>

/**
	@brief TerminalLabel::fromData
	@return the label @p data holds
*/
TerminalLabel TerminalLabel::fromData(const TerminalData &data)
{
	TerminalLabel label;
	label.m_name = data.m_name;
	label.m_show_name = data.m_show_name;
	label.m_label_pos = data.m_label_pos;
	label.m_label_font = data.m_label_font;
	label.m_label_rotation = data.m_label_rotation;
	label.m_label_halignment = data.m_label_halignment;
	label.m_label_valignment = data.m_label_valignment;
	label.m_label_frame = data.m_label_frame;
	label.m_label_color = data.m_label_color;
	return label;
}

/**
	@brief TerminalLabel::applyTo
	Write this label into @p data, so the terminal holding it draws itself
	again the way this label says
*/
void TerminalLabel::applyTo(TerminalData &data) const
{
	data.m_name = m_name;
	data.m_show_name = m_show_name;
	data.m_label_pos = m_label_pos;
	data.m_label_font = m_label_font;
	data.m_label_rotation = m_label_rotation;
	data.m_label_halignment = m_label_halignment;
	data.m_label_valignment = m_label_valignment;
	data.m_label_frame = m_label_frame;
	data.m_label_color = m_label_color;
}

/**
	@brief TerminalLabel::operator==
	Two labels are equal when every value they hold is the same, so that a
	label which differs from the definition in nothing is told apart from
	the definition (@see Element::setTerminalLabel())
*/
bool TerminalLabel::operator==(const TerminalLabel &other) const
{
	return m_name == other.m_name
			&& m_show_name == other.m_show_name
			&& m_label_pos == other.m_label_pos
			&& m_label_font == other.m_label_font
			&& m_label_rotation == other.m_label_rotation
			&& m_label_halignment == other.m_label_halignment
			&& m_label_valignment == other.m_label_valignment
			&& m_label_frame == other.m_label_frame
			&& m_label_color == other.m_label_color;
}

/**
	@brief TerminalLabel::toXml
	@return a <terminalLabel> element holding this label, the attributes
	named as the <terminal> of a definition names them, so the two read
	alike. Element::toXml() adds the terminal it belongs to
	(@see Element::appendTerminalLabelsXml())
*/
QDomElement TerminalLabel::toXml(QDomDocument &document) const
{
	QDomElement xml_element = document.createElement(QStringLiteral("terminalLabel"));

	xml_element.setAttribute(QStringLiteral("name"), m_name);
	xml_element.setAttribute(QStringLiteral("show_name"), QETXML::boolToString(m_show_name));
	xml_element.setAttribute(QStringLiteral("label_x"), QString::number(m_label_pos.x()));
	xml_element.setAttribute(QStringLiteral("label_y"), QString::number(m_label_pos.y()));
	xml_element.setAttribute(QStringLiteral("label_font"), QETUtils::fontToString(m_label_font));
	xml_element.setAttribute(QStringLiteral("label_rotation"), QString::number(m_label_rotation));
	xml_element.setAttribute(QStringLiteral("label_halign"), static_cast<int>(m_label_halignment));
	xml_element.setAttribute(QStringLiteral("label_valign"), static_cast<int>(m_label_valignment));
	xml_element.setAttribute(QStringLiteral("label_frame"), QETXML::boolToString(m_label_frame));
	xml_element.setAttribute(QStringLiteral("label_color"), m_label_color.name());

	return xml_element;
}

/**
	@brief TerminalLabel::fromXml
	Read a label back from @p xml_element, a value the file leaves out
	taking the value it would have had in a definition.
	@return false if @p xml_element is not a label, in which case nothing
	is read
*/
bool TerminalLabel::fromXml(const QDomElement &xml_element)
{
	if (xml_element.tagName() != QLatin1String("terminalLabel"))
		return false;

	m_name = xml_element.attribute(QStringLiteral("name"));
	m_show_name = QETXML::boolFromString(
				xml_element.attribute(QStringLiteral("show_name")), false);

	qreal lx = 0.0, ly = 0.0;
	QET::attributeIsAReal(xml_element, QStringLiteral("label_x"), &lx);
	QET::attributeIsAReal(xml_element, QStringLiteral("label_y"), &ly);
	m_label_pos = QPointF(lx, ly);

	m_label_font = QETApp::diagramTextsFont();
	const QString font = xml_element.attribute(QStringLiteral("label_font"));
	if (!font.isEmpty())
		QETUtils::fontFromString(m_label_font, font);

	m_label_rotation = xml_element.attribute(
				QStringLiteral("label_rotation"), QStringLiteral("0")).toDouble();
	m_label_halignment = static_cast<Qt::Alignment>(
				xml_element.attribute(QStringLiteral("label_halign"),
									  QString::number(static_cast<int>(Qt::AlignHCenter))).toInt());
	m_label_valignment = static_cast<Qt::Alignment>(
				xml_element.attribute(QStringLiteral("label_valign"),
									  QString::number(static_cast<int>(Qt::AlignVCenter))).toInt());
	m_label_frame = QETXML::boolFromString(
				xml_element.attribute(QStringLiteral("label_frame")), false);

	const QString color = xml_element.attribute(QStringLiteral("label_color"));
	if (!color.isEmpty())
		m_label_color = QColor(color);

	return true;
}
