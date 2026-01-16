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
#include "../../../utils/qetutils.h"

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
