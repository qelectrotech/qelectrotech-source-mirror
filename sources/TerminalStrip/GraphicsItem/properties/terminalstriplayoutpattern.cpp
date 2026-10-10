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
#include "terminalstriplayoutpattern.h"
#include <utils/qetutils.h>

#include <QDomDocument>

TerminalStripLayoutPattern::TerminalStripLayoutPattern()
{
    m_font.setPixelSize(15);
	updateHeaderTextOption();
	updateTerminalsTextOption();
}

/**
 * @brief TerminalStripLayoutPattern::setHeaderTextAlignment
 * Set text alignment to @param alignment. If alignment have no
 * flag this function do nothing
 * @param alignment
 */
void TerminalStripLayoutPattern::setHeaderTextAlignment(const Qt::Alignment &alignment)
{
    if (!alignment) return;
	m_header_text_alignment = alignment;
	updateHeaderTextOption();
}

Qt::Alignment TerminalStripLayoutPattern::headerTextAlignment() const
{
	return m_header_text_alignment;
}

QTextOption TerminalStripLayoutPattern::headerTextOption() const {
	return m_header_text_option;
}

QFont TerminalStripLayoutPattern::font() const {
    return m_font;
}

void TerminalStripLayoutPattern::setFont(const QFont &font) {
    m_font = font;
    QETUtils::pixelSizedFont(m_font);
}

/**
 * @brief TerminalStripLayoutPattern::setTerminalsTextAlignment
 * Set text alignment to @param alignment. If alignment have no
 * flag this function do nothing
 * @param alignment
 */
void TerminalStripLayoutPattern::setTerminalsTextAlignment(const Qt::Alignment &alignment)
{
    if (!alignment) return;
	m_terminals_text_alignment = alignment;
	updateTerminalsTextOption();
}

Qt::Alignment TerminalStripLayoutPattern::terminalsTextAlignment() const
{
	return m_terminals_text_alignment;
}

QTextOption TerminalStripLayoutPattern::terminalsTextOption() const
{
	return m_terminals_text_option;
}

/**
 * @brief TerminalStripLayoutPattern::setXrefTextAlignment
 * Set text alignment to @param alignment. If alignment have no
 * flag this function do nothing
 * @param alignment
 */
void TerminalStripLayoutPattern::setXrefTextAlignment(const Qt::Alignment &alignment)
{
	if (!alignment) return;
	m_xref_text_alignment = alignment;
	updateTerminalsTextOption();
}

Qt::Alignment TerminalStripLayoutPattern::xrefTextAlignment() const
{
	return m_xref_text_alignment;
}

QTextOption TerminalStripLayoutPattern::xrefTextOption() const
{
	return m_xref_text_option;
}

void TerminalStripLayoutPattern::updateHeaderTextOption()
{
	m_header_text_option.setAlignment(m_header_text_alignment);
	m_header_text_option.setWrapMode(QTextOption::WordWrap);
}

void TerminalStripLayoutPattern::updateTerminalsTextOption()
{
    m_terminals_text_option.setAlignment(m_terminals_text_alignment);
    m_terminals_text_option.setWrapMode(QTextOption::WordWrap);

	m_xref_text_option.setAlignment(m_xref_text_alignment);
	m_xref_text_option.setWrapMode(QTextOption::WordWrap);
}

namespace
{
	void rectToXml(QDomElement &element, const QString &name, const QRectF &rect)
	{
		element.setAttribute(name + QStringLiteral("_x"), rect.x());
		element.setAttribute(name + QStringLiteral("_y"), rect.y());
		element.setAttribute(name + QStringLiteral("_w"), rect.width());
		element.setAttribute(name + QStringLiteral("_h"), rect.height());
	}

	QRectF rectFromXml(const QDomElement &element, const QString &name, const QRectF &default_rect)
	{
		auto value = [&element, &name](const QString &suffix, qreal default_value)
		{
			bool ok = false;
			const auto v = element.attribute(name + suffix).toDouble(&ok);
			return ok ? v : default_value;
		};

		return QRectF(value(QStringLiteral("_x"), default_rect.x()),
					  value(QStringLiteral("_y"), default_rect.y()),
					  value(QStringLiteral("_w"), default_rect.width()),
					  value(QStringLiteral("_h"), default_rect.height()));
	}

	qreal realFromXml(const QDomElement &element, const QString &name, qreal default_value)
	{
		bool ok = false;
		const auto v = element.attribute(name).toDouble(&ok);
		return ok ? v : default_value;
	}

	QString orientationToString(Qt::Orientation orientation) {
		return orientation == Qt::Horizontal ? QStringLiteral("horizontal") : QStringLiteral("vertical");
	}

	Qt::Orientation orientationFromXml(const QDomElement &element, const QString &name, Qt::Orientation default_value)
	{
		const auto v = element.attribute(name);
		if (v == QLatin1String("horizontal")) return Qt::Horizontal;
		if (v == QLatin1String("vertical")) return Qt::Vertical;
		return default_value;
	}

	Qt::Alignment alignmentFromXml(const QDomElement &element, const QString &name, Qt::Alignment default_value)
	{
		bool ok = false;
		const auto v = element.attribute(name).toInt(&ok);
		return ok && v ? Qt::Alignment(static_cast<Qt::AlignmentFlag>(v)) : default_value;
	}
}

/**
 * @brief TerminalStripLayoutPattern::xmlTagName
 * @return the tag name of the xml element used by toXml / fromXml
 */
QString TerminalStripLayoutPattern::xmlTagName() {
	return QStringLiteral("terminal_strip_layout");
}

/**
 * @brief TerminalStripLayoutPattern::toXml
 * Save all the values of this layout, including the font.
 * The font is saved by hand (family, pixel size, bold, italic) and not with
 * QFont::toString() because the format of this string is different between Qt5 and Qt6.
 * @param document
 * @return the xml element
 */
QDomElement TerminalStripLayoutPattern::toXml(QDomDocument &document) const
{
	auto element = document.createElement(xmlTagName());

	rectToXml(element, QStringLiteral("header"), m_header_rect);
	element.setAttribute(QStringLiteral("header_text_orientation"), orientationToString(m_header_text_orientation));
	element.setAttribute(QStringLiteral("header_text_alignment"), static_cast<int>(m_header_text_alignment));

	rectToXml(element, QStringLiteral("spacer"), m_spacer_rect);

	element.setAttribute(QStringLiteral("font_family"), m_font.family());
	element.setAttribute(QStringLiteral("font_pixel_size"), m_font.pixelSize());
	element.setAttribute(QStringLiteral("font_bold"), m_font.bold() ? 1 : 0);
	element.setAttribute(QStringLiteral("font_italic"), m_font.italic() ? 1 : 0);

	for (auto i = 0; i < m_terminal_rect.size(); ++i) {
		rectToXml(element, QStringLiteral("terminal_%1").arg(i), m_terminal_rect.at(i));
	}

	element.setAttribute(QStringLiteral("terminals_text_height"), m_terminals_text_height);
	element.setAttribute(QStringLiteral("terminals_text_y"), m_terminals_text_y);
	element.setAttribute(QStringLiteral("terminals_text_orientation"), orientationToString(m_terminals_text_orientation));
	element.setAttribute(QStringLiteral("terminals_text_alignment"), static_cast<int>(m_terminals_text_alignment));

	element.setAttribute(QStringLiteral("xref_text_height"), m_xref_text_height);
	element.setAttribute(QStringLiteral("xref_text_y"), m_xref_text_y);
	element.setAttribute(QStringLiteral("xref_text_orientation"), orientationToString(m_xref_text_orientation));
	element.setAttribute(QStringLiteral("xref_text_alignment"), static_cast<int>(m_xref_text_alignment));

	element.setAttribute(QStringLiteral("type_symbol_y"), m_type_symbol_y);
	element.setAttribute(QStringLiteral("type_symbol_height"), m_type_symbol_height);
	element.setAttribute(QStringLiteral("connection_length"), m_connection_length);
	element.setAttribute(QStringLiteral("cable_wire_length"), m_cable_wire_length);
	element.setAttribute(QStringLiteral("cable_length"), m_cable_length);
	element.setAttribute(QStringLiteral("cable_end_length"), m_cable_end_length);

	element.setAttribute(QStringLiteral("bridge_point_d"), m_bridge_point_d);
	for (auto i = 0; i < m_bridge_point_y_offset.size(); ++i) {
		element.setAttribute(QStringLiteral("bridge_point_y_offset_%1").arg(i), m_bridge_point_y_offset.at(i));
	}

	return element;
}

/**
 * @brief TerminalStripLayoutPattern::fromXml
 * Load the values saved by toXml. A value missing in @a layout_element
 * (project saved with an older version) keep the current value of this layout.
 * @param layout_element
 */
void TerminalStripLayoutPattern::fromXml(const QDomElement &layout_element)
{
	if (layout_element.isNull()) {
		return;
	}

	m_header_rect = rectFromXml(layout_element, QStringLiteral("header"), m_header_rect);
	m_header_text_orientation = orientationFromXml(layout_element, QStringLiteral("header_text_orientation"), m_header_text_orientation);
	setHeaderTextAlignment(alignmentFromXml(layout_element, QStringLiteral("header_text_alignment"), m_header_text_alignment));

	m_spacer_rect = rectFromXml(layout_element, QStringLiteral("spacer"), m_spacer_rect);

	auto font_ = m_font;
	if (layout_element.hasAttribute(QStringLiteral("font_family"))) {
		font_.setFamily(layout_element.attribute(QStringLiteral("font_family")));
	}
	const auto pixel_size = layout_element.attribute(QStringLiteral("font_pixel_size")).toInt();
	if (pixel_size > 0) {
		font_.setPixelSize(pixel_size);
	}
	if (layout_element.hasAttribute(QStringLiteral("font_bold"))) {
		font_.setBold(layout_element.attribute(QStringLiteral("font_bold")).toInt() != 0);
	}
	if (layout_element.hasAttribute(QStringLiteral("font_italic"))) {
		font_.setItalic(layout_element.attribute(QStringLiteral("font_italic")).toInt() != 0);
	}
	setFont(font_);

	for (auto i = 0; i < m_terminal_rect.size(); ++i) {
		m_terminal_rect[i] = rectFromXml(layout_element, QStringLiteral("terminal_%1").arg(i), m_terminal_rect.at(i));
	}

	m_terminals_text_height = realFromXml(layout_element, QStringLiteral("terminals_text_height"), m_terminals_text_height);
	m_terminals_text_y = realFromXml(layout_element, QStringLiteral("terminals_text_y"), m_terminals_text_y);
	m_terminals_text_orientation = orientationFromXml(layout_element, QStringLiteral("terminals_text_orientation"), m_terminals_text_orientation);
	setTerminalsTextAlignment(alignmentFromXml(layout_element, QStringLiteral("terminals_text_alignment"), m_terminals_text_alignment));

	m_xref_text_height = realFromXml(layout_element, QStringLiteral("xref_text_height"), m_xref_text_height);
	m_xref_text_y = realFromXml(layout_element, QStringLiteral("xref_text_y"), m_xref_text_y);
	m_xref_text_orientation = orientationFromXml(layout_element, QStringLiteral("xref_text_orientation"), m_xref_text_orientation);
	setXrefTextAlignment(alignmentFromXml(layout_element, QStringLiteral("xref_text_alignment"), m_xref_text_alignment));

	m_type_symbol_y = realFromXml(layout_element, QStringLiteral("type_symbol_y"), m_type_symbol_y);
	m_type_symbol_height = realFromXml(layout_element, QStringLiteral("type_symbol_height"), m_type_symbol_height);
	m_connection_length = realFromXml(layout_element, QStringLiteral("connection_length"), m_connection_length);
	m_cable_wire_length = realFromXml(layout_element, QStringLiteral("cable_wire_length"), m_cable_wire_length);
	m_cable_length = realFromXml(layout_element, QStringLiteral("cable_length"), m_cable_length);
	m_cable_end_length = realFromXml(layout_element, QStringLiteral("cable_end_length"), m_cable_end_length);

	m_bridge_point_d = realFromXml(layout_element, QStringLiteral("bridge_point_d"), m_bridge_point_d);
	for (auto i = 0; i < m_bridge_point_y_offset.size(); ++i) {
		m_bridge_point_y_offset[i] = realFromXml(layout_element, QStringLiteral("bridge_point_y_offset_%1").arg(i), m_bridge_point_y_offset.at(i));
	}
}
