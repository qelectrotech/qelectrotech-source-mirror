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
#include "terminalstriplayoutpatternxml.h"

#include "../TerminalStrip/GraphicsItem/properties/terminalstriplayoutpattern.h"
#include "../qetxml.h"
#include "../svg/qetsvg.h"

const QString LAYOUT_PATTERN_TAG_NAME { QStringLiteral("terminal_strip_layout_pattern") };
const QString LAYOUTS_PATTERN_TAG_NAME { QStringLiteral("terminal_strip_layouts_pattern") };

/**
 * @brief TerminalStripLayoutPatternXml::toXml
 * Save a vector of @class TerminalStripLayoutPattern into main xml element
 * with tagg name 'terminal_strip_layouts_pattern' who itself embedded each single @class TerminalStripLayoutPattern
 * into a xml element with tag name 'terminal_strip_layout_pattern' (layout without 'S' at the end)
 * @param patterns
 * @param document
 * @return
 */
QDomElement TerminalStripLayoutPatternXml::toXml(const QVector<QSharedPointer<TerminalStripLayoutPattern> > &patterns,
                                                 QDomDocument &document)
{
    auto dom_element = document.createElement(LAYOUTS_PATTERN_TAG_NAME);
    for (const auto &pattern : patterns)
    {
        const auto child_ = toXml(pattern, document);
        if (!child_.isNull()) {
            dom_element.appendChild(child_);
        }
    }

    return dom_element;
}

/**
 * @brief TerminalStripLayoutPatternXml::fromXml
 * Load a vector of @class TerminalStripLayoutPattern from a main xml element
 * with tagg name 'terminal_strip_layouts_pattern' who itself have several child
 * with tag name 'terminal_strip_layout_pattern' (layout without 'S' at the end) for every
 * @class TerminalStripLayoutPattern to load
 * @param element
 * @return
 */
QVector<QSharedPointer<TerminalStripLayoutPattern> > TerminalStripLayoutPatternXml::fromXml(const QDomElement &element)
{
    QVector <QSharedPointer<TerminalStripLayoutPattern>> returned_vector;

    for (const auto &dom_elmt : QETXML::subChild(element,
                                                 LAYOUTS_PATTERN_TAG_NAME,
                                                 LAYOUT_PATTERN_TAG_NAME))
    {
        auto layout_pattern = QSharedPointer<TerminalStripLayoutPattern>::create();
        fromXml(layout_pattern, dom_elmt);

        returned_vector << layout_pattern;
    }

    return returned_vector;
}

/**
 * @brief TerminalStripLayoutPatternXml::toXml
 * Save a @class TerminalStripLayoutPattern to a xml element with tag name terminal_strip_layout_pattern
 * @param pattern
 * @param document
 * @return
 */
QDomElement TerminalStripLayoutPatternXml::toXml(const QSharedPointer<TerminalStripLayoutPattern> &pattern, QDomDocument &document)
{
    auto pattern_xml = document.createElement(LAYOUT_PATTERN_TAG_NAME);
    if (!pattern.isNull()) {
            //Write strip pattern attributes
        pattern_xml.setAttribute(QStringLiteral("name"), pattern->m_name);
        pattern_xml.setAttribute(QStringLiteral("uuid"), pattern->m_uuid.toString());

            //Write header properties
        auto header_xml = document.createElement(QStringLiteral("header"));
        header_xml.appendChild(QETSVG::rectToElmt(pattern->m_header_rect, document));

        auto header_text_xml = document.createElement(QStringLiteral("text"));
        QETXML::orientationToAttribute(pattern->m_header_text_orientation, header_text_xml);
        QETXML::alignmentToAttribute(pattern->headerTextAlignment(),header_text_xml);

            //Write spacer properties
        auto spacer_xml = document.createElement(QStringLiteral("spacer"));
        spacer_xml.appendChild(QETSVG::rectToElmt(pattern->m_spacer_rect, document));

        pattern_xml.appendChild(header_xml).appendChild(header_text_xml);
        pattern_xml.appendChild(spacer_xml);

            //Write terminals properties
        auto terminals_xml = document.createElement(QStringLiteral("terminals"));
        for (const auto &rect : pattern->m_terminal_rect) {
            terminals_xml.appendChild(QETSVG::rectToElmt(rect, document));
        }

        auto terminals_text_xml = document.createElement(QStringLiteral("text"));
        QETXML::orientationToAttribute(pattern->m_terminals_text_orientation, terminals_text_xml);
        QETXML::alignmentToAttribute(pattern->terminalsTextAlignment(),terminals_text_xml);
        QETSVG::yToAttribute(pattern->m_terminals_text_y, terminals_text_xml);
        QETSVG::heightToAttribute(pattern->m_terminals_text_height, terminals_text_xml);
        terminals_xml.appendChild(terminals_text_xml);

		auto xref_text_xml = document.createElement(QStringLiteral("xref"));
		QETXML::orientationToAttribute(pattern->m_xref_text_orientation, xref_text_xml);
		QETXML::alignmentToAttribute(pattern->xrefTextAlignment(),xref_text_xml);
		QETSVG::yToAttribute(pattern->m_xref_text_y, xref_text_xml);
		QETSVG::heightToAttribute(pattern->m_xref_text_height, xref_text_xml);
		terminals_xml.appendChild(xref_text_xml);

        auto bridge_xml = document.createElement(QStringLiteral("bridges"));
        QETSVG::rToAttribute(pattern->m_bridge_point_d/2, bridge_xml);
        QVector<QPointF> points_vector;
        for (const auto &y : pattern->m_bridge_point_y_offset) {
            points_vector.append(QPointF{0,y});
        }
        QETSVG::pointsToAttribute(points_vector, bridge_xml);
        terminals_xml.appendChild(bridge_xml);

        pattern_xml.appendChild(terminals_xml);
    }

    return pattern_xml;
}

/**
 * @brief TerminalStripLayoutPatternXml::fromXml
 * LOad a @class TerminalStripLayoutPattern from a xml element with tag name terminal_strip_layout_pattern
 * @param layout
 * @param element
 */
void TerminalStripLayoutPatternXml::fromXml(QSharedPointer<TerminalStripLayoutPattern> &layout, const QDomElement &element)
{
    if (element.tagName() != LAYOUT_PATTERN_TAG_NAME) return;
    layout->m_name = element.attribute(QStringLiteral("name"), layout->m_name);
    layout->m_uuid.fromString(element.attribute(QStringLiteral("uuid"), layout->m_uuid.toString()));

    const auto header_xml = element.firstChildElement(QStringLiteral("header"));
    if (!header_xml.isNull() && header_xml.hasAttribute(QStringLiteral("rect"))) {
        layout->m_header_rect = QETSVG::rectFromElmt(header_xml);
    }

    if (const auto header_text_xml = header_xml.firstChildElement(QStringLiteral("text"));
        !header_text_xml.isNull()) {
        layout->m_header_text_orientation = QETXML::orientationFromAttribute(header_text_xml, layout->m_header_text_orientation);
        layout->setHeaderTextAlignment(QETXML::alignmentFromAttribute(header_text_xml));
    }

    const auto spacer_xml = element.firstChildElement(QStringLiteral("spacer"));
    if (const auto rect = QETSVG::rectFromElmt(spacer_xml);
        !rect.isNull()) {
        layout->m_spacer_rect = rect;
    }

    if (const auto terminals_xml = element.firstChildElement(QStringLiteral("terminals"));
        !terminals_xml.isNull()) {
        layout->m_terminal_rect.clear();
        for (const auto &terminal_rect_xml : QETXML::directChild(terminals_xml, QStringLiteral("rect"))) {
            layout->m_terminal_rect.append(QETSVG::rectFromElmt(terminal_rect_xml)) ;
        }

        if (const auto terminals_text_xml = terminals_xml.firstChildElement(QStringLiteral("text"));
            !terminals_text_xml.isNull()) {
            layout->m_terminals_text_orientation = QETXML::orientationFromAttribute(terminals_text_xml, layout->m_terminals_text_orientation);
            layout->setTerminalsTextAlignment(QETXML::alignmentFromAttribute(terminals_text_xml));
            layout->m_terminals_text_y = QETSVG::yFromAttribute(terminals_text_xml);
            layout->m_terminals_text_height = QETSVG::heightFromAttribute(terminals_text_xml);
        }

		if (const auto xref_text_xml = terminals_xml.firstChildElement(QStringLiteral("xref"));
			!xref_text_xml.isNull()) {
			layout->m_xref_text_orientation = QETXML::orientationFromAttribute(xref_text_xml, layout->m_xref_text_orientation);
			layout->setXrefTextAlignment(QETXML::alignmentFromAttribute(xref_text_xml));
			layout->m_xref_text_y = QETSVG::yFromAttribute(xref_text_xml);
			layout->m_xref_text_height = QETSVG::heightFromAttribute(xref_text_xml);
		}

        if (const auto bridge_xml = terminals_xml.firstChildElement(QStringLiteral("bridges"));
            !bridge_xml.isNull()) {
            layout->m_bridge_point_d = QETSVG::rFromAttribute(bridge_xml, 2.5)*2;
            if (const auto points = QETSVG::pointsFromAttribute(bridge_xml);
                points.size() == 4) {
                layout->m_bridge_point_y_offset[0]= points[0].y();
                layout->m_bridge_point_y_offset[1]= points[1].y();
                layout->m_bridge_point_y_offset[2]= points[2].y();
                layout->m_bridge_point_y_offset[3]= points[3].y();
            }
        }
    }
}
