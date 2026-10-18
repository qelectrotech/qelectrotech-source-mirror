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

#include <QColor>
#include <QPointer>
#include <QUuid>

#include "properties/terminalstriplayoutpattern.h"
#include "../../properties/elementdata.h"

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

	/**
	 * @brief The XrefRect struct
	 * Where the xref text of a real terminal was drawn, in the coordinates
	 * of the item, with the physical and real index of that terminal.
	 * Filled by paint(): used to make the xref clickable in an exported PDF.
	 */
	struct XrefRect{
		QRectF rect;
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
			//Type and led are only used to draw the symbol of the terminal.
			//They are not pure virtual, so an implementation can ignore them.
			virtual ElementData::TerminalType type() const { return ElementData::TTGeneric; }
			virtual bool isLed() const { return false; }
			//Cable (hose) connected to the terminal, drawn under the strip.
			virtual QString cable() const { return QString(); }
			virtual QString cableWire() const { return QString(); }
			virtual bool isShield() const { return false; }
			//Color and width (mm) of the terminal, only used to draw the terminal.
			//An invalid color means "no override" : the default color of the type is used.
			//A width <= 0 means "unknown" : the default width of the layout is used.
			virtual QColor color() const { return QColor(); }
			//Second color of a two-tone color (like green/yellow), drawn as stripes over color()
			virtual QColor stripeColor() const { return QColor(); }
			virtual qreal widthMm() const { return 0; }
			//Label of the terminal chosen in the catalog (TerminalCatalog), empty if unknown.
			//The realistic styles draw its clamp (screw or push-in) instead of the one of the layout.
			virtual QString catalogEntry() const { return QString(); }
			//A separator plate is mounted after the terminal (drawn by the realistic styles)
			virtual bool hasSeparatorAfter() const { return false; }
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

			void setDrawStyle(TerminalStripLayoutPattern::DrawStyle style);
			TerminalStripLayoutPattern::DrawStyle drawStyle() const;

			void setMouseHoverPos(const QPointF &pos);
			bool mouseHoverXref() const;
			bool needUpdate();
			hoverTerminal hoveredXref() const;
			hoverTerminal xrefAt(const QPointF &pos) const;
			QVector<XrefRect> xrefRects() const;

        private:
            qreal height() const;
            qreal width() const;

        private:
            QSharedPointer <AbstractTerminalStripInterface> m_strip;
            QSharedPointer<TerminalStripLayoutPattern> m_pattern;
            bool m_preview_draw { false };
			TerminalStripLayoutPattern::DrawStyle m_draw_style { TerminalStripLayoutPattern::DrawStyle::Schematic };
			QPointF m_mouse_hover_pos;
			QRectF m_united_xref_text_rect;
			bool m_last_mouse_pos_in_xrefs_rect{false};
			hoverTerminal m_hovered_xref;
			QVector<XrefRect> m_xref_rects;
    };
}

#endif // TERMINALSTRIPDRAWER_H
