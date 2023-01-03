/*
	Copyright 2006-2022 The QElectroTech Team
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
#include <QDebug>

TerminalStripLayoutPattern::TerminalStripLayoutPattern()
{
	updateHeaderTextOption();
	updateTerminalsTextOption();
}

void TerminalStripLayoutPattern::setHeaderTextAlignment(const Qt::Alignment &alignment)
{
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

void TerminalStripLayoutPattern::setTerminalsTextAlignment(const QVector<Qt::Alignment> &alignment)
{
	m_terminals_text_alignment = alignment;
	updateTerminalsTextOption();
}

QVector<Qt::Alignment> TerminalStripLayoutPattern::terminalsTextAlignment() const
{
	return m_terminals_text_alignment;
}

QVector<QTextOption> TerminalStripLayoutPattern::terminalsTextOption() const
{
	return m_terminals_text_option;
}

void TerminalStripLayoutPattern::updateHeaderTextOption()
{
	m_header_text_option.setAlignment(m_header_text_alignment);
	m_header_text_option.setWrapMode(QTextOption::WordWrap);
}

void TerminalStripLayoutPattern::updateTerminalsTextOption()
{
	if (m_terminals_text_option.size() ==
		m_terminals_text_alignment.size())
	{
		for (auto i = 0 ; i<m_terminals_text_option.size() ; ++i)
		{
			m_terminals_text_option[i].setAlignment(m_terminals_text_alignment.at(i));
			m_terminals_text_option[i].setWrapMode(QTextOption::WordWrap);
		}
	}
	else {
		qDebug() << "TerminalStripLayoutPattern::updateTerminalsTextOption() : Wrong vector size";
	}
}
