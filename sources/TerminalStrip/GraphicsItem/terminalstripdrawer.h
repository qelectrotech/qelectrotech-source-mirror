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
#ifndef TERMINALSTRIPDRAWER_H
#define TERMINALSTRIPDRAWER_H

#include <QPointer>

#include "properties/terminalstriplayoutpattern.h"

class QPainter;
class TerminalStrip;

namespace TerminalStripDrawer
{
	/**
	 * @brief The hoverTerminal struct
	 * Just a little struct use to know what is the physical and real terminal
	 * when the mouse hover the Xref string of a terminal.
	 * If mouse don't hover a Xref the value is set to -1;
	 */
	struct hoverTerminal{
		int physical{-1};
		int real{-1};
	};

    class AbstractBridgeInterface
    {
        public:
            AbstractBridgeInterface() {}
            virtual ~AbstractBridgeInterface() {}
            virtual QUuid uuid() const = 0;
    };

    class AbstractRealTerminalInterface
    {
        public:
            AbstractRealTerminalInterface() {}
            virtual ~AbstractRealTerminalInterface() {}
            virtual QString label() const = 0;
            virtual bool isBridged() const = 0;
            virtual AbstractBridgeInterface* bridge() const = 0;
			virtual QString xref() const = 0;
    };

	class AbstractPhysicalTerminalInterface
	{
		public:
			AbstractPhysicalTerminalInterface() {}
			virtual ~AbstractPhysicalTerminalInterface() {}
			virtual QVector<QSharedPointer<AbstractRealTerminalInterface>> realTerminals() const = 0;
	};

	class AbstractTerminalStripInterface
	{
		public:
			AbstractTerminalStripInterface() {}
			virtual ~AbstractTerminalStripInterface() {}
			virtual QString installation() const = 0;
			virtual QString location() const = 0;
			virtual QString name() const = 0;
			virtual QVector<QSharedPointer<AbstractPhysicalTerminalInterface>> physicalTerminal() const = 0;
	};

	class TerminalStripDrawer
	{
		public:
			TerminalStripDrawer(QSharedPointer<AbstractTerminalStripInterface> strip = QSharedPointer<AbstractTerminalStripInterface> { nullptr },
								QSharedPointer<TerminalStripLayoutPattern> layout = QSharedPointer<TerminalStripLayoutPattern>());

			void setStrip(QSharedPointer<AbstractTerminalStripInterface> strip);
			void paint(QPainter *painter);
			QRectF boundingRect() const;

			void setLayout(QSharedPointer<TerminalStripLayoutPattern> layout);
			bool haveLayout() const;

			void setPreviewDraw(bool draw = true);

			void setMouseHoverPos(const QPointF &pos);
			bool mouseHoverXref() const;
			bool needUpdate();
			hoverTerminal hoveredXref() const;

        private:
            qreal height() const;
            qreal width() const;

        private:
            QSharedPointer <AbstractTerminalStripInterface> m_strip;
            QSharedPointer<TerminalStripLayoutPattern> m_pattern;
            bool m_preview_draw { false };
			QPointF m_mouse_hover_pos;
			QRectF m_united_xref_text_rect;
			bool m_last_mouse_pos_in_xrefs_rect{false};
			hoverTerminal m_hovered_xref;
    };
}

#endif // TERMINALSTRIPDRAWER_H
