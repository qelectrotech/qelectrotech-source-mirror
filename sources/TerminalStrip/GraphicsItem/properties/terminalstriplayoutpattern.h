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
#ifndef TERMINALSTRIPLAYOUTPATTERN_H
#define TERMINALSTRIPLAYOUTPATTERN_H

#include <QFont>
#include <QRect>
#include <QSize>
#include <QTextOption>
#include <QUuid>
#include <QVector>

/**
 * @brief The TerminalStripLayoutPattern class
 * A class with all values used to define how a terminal strip must be drawn.
 * Most of the value are public, some values are private and have getter / setter
 * because when these values change we need to compute the change.
 *
 * The values with name '_y_offset' mean offset is relating to the top
 * of the QGraphicsItem used to display the terminal strip.
 *
 * The terminal strip can display up to 4 terminal level,
 * the value used for multilevel terminal are stored in  several QVector (m_terminal_y_offset, m_terminal_height, m_bridge_point_y_offset).
 * The order of the values are from the most back terminal to the front terminal.
 */
class TerminalStripLayoutPattern
{
	public:
		TerminalStripLayoutPattern();

			//Header of terminal strip
        QRectF m_header_rect{0,30,50,130};
		Qt::Orientation m_header_text_orientation{Qt::Horizontal};
		void setHeaderTextAlignment(const Qt::Alignment &alignment);
		Qt::Alignment headerTextAlignment() const;
		QTextOption headerTextOption() const;

			//Spacer between the header and the terminals
        QRectF m_spacer_rect{0, 50, 10, 90};

            //Font
        QFont font() const;
        void setFont (const QFont &font);

			//Terminals
        QVector<QRectF> m_terminal_rect
		{
            QRectF{0, 0, 20, 190},
            QRectF{0, 10, 20, 170},
            QRectF{0, 20, 20, 150},
            QRectF{0, 30, 20, 130}
		};

            //Terminal text
        void setTerminalsTextAlignment(const Qt::Alignment &alignment);
        Qt::Alignment terminalsTextAlignment() const;
        QTextOption terminalsTextOption() const;
        qreal m_terminals_text_height{50};
        qreal m_terminals_text_y{35};
        Qt::Orientation m_terminals_text_orientation {Qt::Vertical};

			//Xref text
		void setXrefTextAlignment(const Qt::Alignment &alignment);
		Qt::Alignment xrefTextAlignment() const;
		QTextOption xrefTextOption() const;
		qreal m_xref_text_height{60};
		qreal m_xref_text_y{95};
		Qt::Orientation m_xref_text_orientation {Qt::Vertical};

        qreal m_bridge_point_d{5};
        QVector<qreal> m_bridge_point_y_offset{50,70,90,110};

		QUuid m_uuid{QUuid::createUuid()};
		QString m_name;

	private:
		void updateHeaderTextOption();
		void updateTerminalsTextOption();

    private:
        QFont m_font;
		Qt::Alignment m_header_text_alignment{Qt::AlignCenter};
		QTextOption m_header_text_option;

		Qt::Alignment
			m_terminals_text_alignment {Qt::AlignRight | Qt::AlignVCenter},
			m_xref_text_alignment {Qt::AlignLeft | Qt::AlignVCenter};
		QTextOption
			m_terminals_text_option{QTextOption()},
			m_xref_text_option{QTextOption()};
};

#endif // TERMINALSTRIPLAYOUTPATTERN_H
