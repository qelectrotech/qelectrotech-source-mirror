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
#include "terminalstripdrawer.h"

#include <QPainter>
#include <QFontMetricsF>
#include <QHash>

#include <algorithm>

namespace TerminalStripDrawer {

/**
 * @brief TerminalStripDrawer::TerminalStripDrawer
 * @param strip
 * @param pattern
 */
TerminalStripDrawer::TerminalStripDrawer(QSharedPointer<AbstractTerminalStripInterface> strip,
										 QSharedPointer<TerminalStripLayoutPattern> layout) :
	m_strip { strip },
	m_pattern { layout }
{}

void TerminalStripDrawer::setStrip(QSharedPointer<AbstractTerminalStripInterface> strip)
{
	m_strip = strip;
}

namespace {

/*
 * The symbols are described in a box of 20 x 14 units centered on (0,0).
 * The current flow from the top to the bottom, like in the terminal strip.
 * drawTypeSymbol() scale this box to fit the real available area.
 */
constexpr qreal symbol_width{20};
constexpr qreal symbol_height{14};

void drawFuseSymbol(QPainter *painter)
{
	painter->drawLine(QPointF{0, -7}, QPointF{0, 7});
	painter->drawRect(QRectF{-3, -5, 6, 10});
}

void drawSectionalSymbol(QPainter *painter)
{
	painter->drawLine(QPointF{0, -7}, QPointF{0, 7});
	painter->drawLine(QPointF{-4, 2}, QPointF{4, -2});
}

void drawDiodeSymbol(QPainter *painter)
{
	const qreal t{symbol_height / 3};
	const QPointF triangle[3] { {-t, -t}, {t, -t}, {0, t} };
	painter->drawLine(QPointF{0, -7}, QPointF{0, -t});
	painter->drawPolygon(triangle, 3);
	painter->drawLine(QPointF{-t, t}, QPointF{t, t});
	painter->drawLine(QPointF{0, t}, QPointF{0, 7});
}

void drawLedSymbol(QPainter *painter)
{
	drawDiodeSymbol(painter);
	const qreal t{symbol_height / 3};
		//The two rays
	painter->drawLine(QPointF{t + 0.5, -1}, QPointF{t + 3.5, -4});
	painter->drawLine(QPointF{t + 0.5, 2}, QPointF{t + 3.5, -1});
}

void drawGroundSymbol(QPainter *painter)
{
	painter->drawLine(QPointF{0, -7}, QPointF{0, -2});
	painter->drawLine(QPointF{-5, -2}, QPointF{5, -2});
	painter->drawLine(QPointF{-3, 1}, QPointF{3, 1});
	painter->drawLine(QPointF{-1, 4}, QPointF{1, 4});
}

/**
 * @brief drawScaled
 * Call @a draw with the painter moved by @a dx and scaled by @a factor.
 * The width of the pen is kept constant.
 */
template<typename Func>
void drawScaled(QPainter *painter, qreal dx, qreal factor, Func draw)
{
	painter->save();
	painter->translate(dx, 0);
	painter->scale(factor, factor);
	auto pen_{painter->pen()};
	pen_.setWidthF(pen_.widthF() / factor);
	painter->setPen(pen_);
	draw(painter);
	painter->restore();
}

/**
 * @brief drawTypeSymbol
 * Draw the symbol of a terminal type in @a box. The symbol is scaled
 * to fit in @a box, so it never overflow on the neighbouring terminal.
 * The painter must have a pen set, the brush is ignored.
 * @param painter
 * @param box : area available for the symbol, in the painter coordinates
 * @param type : type of the terminal, a generic terminal have no symbol
 * @param led : if true a led symbol is drawn. If the terminal have
 * also a type (fuse...), the type symbol and the led are drawn side by side.
 */
void drawTypeSymbol(QPainter *painter, const QRectF &box, ElementData::TerminalType type, bool led)
{
	const bool have_type{type != ElementData::TTGeneric};
	if (!led && !have_type) {
		return;
	}

	const qreal scale_{qMin(box.width() / symbol_width, box.height() / symbol_height)};
	if (scale_ <= 0) {
		return;
	}

	painter->save();
	painter->setBrush(Qt::NoBrush);
	painter->translate(box.center());
	painter->scale(scale_, scale_);
	auto pen_{painter->pen()};
	pen_.setWidthF(pen_.widthF() / scale_);
	painter->setPen(pen_);

	auto draw_type = [type](QPainter *p)
	{
		switch (type)
		{
			case ElementData::TTFuse      : drawFuseSymbol(p); break;
			case ElementData::TTSectional : drawSectionalSymbol(p); break;
			case ElementData::TTDiode     : drawDiodeSymbol(p); break;
			case ElementData::TTGround    : drawGroundSymbol(p); break;
			default: break;
		}
	};

	if (led && have_type)
	{
			//Not enough place for two full size symbols, draw them smaller side by side
		drawScaled(painter, -5, 0.7, draw_type);
		drawScaled(painter, 2.5, 0.7, drawLedSymbol);
	}
	else if (led) {
		drawLedSymbol(painter);
	}
	else {
		draw_type(painter);
	}

	painter->restore();
}

/**
 * @brief drawConnections
 * Draw a connection above and, if @a draw_bottom is true, a connection under
 * @a terminal_rect, in the middle of the rect.
 * A connection is a short line ended by a little circle.
 * @param painter
 * @param terminal_rect
 * @param length : the total length of a connection, circle included
 * @param draw_bottom : false if the cable is drawn under the terminal
 */
void drawConnections(QPainter *painter, const QRectF &terminal_rect, qreal length, bool draw_bottom)
{
	const qreal radius{qMin<qreal>(length / 4, 2)};
	const qreal x{terminal_rect.width() / 2};
	const qreal top{terminal_rect.top()};
	const qreal bottom{terminal_rect.top() + terminal_rect.height()};

	painter->drawLine(QPointF{x, top}, QPointF{x, top - length + radius * 2});
	painter->drawEllipse(QPointF{x, top - length + radius}, radius, radius);

	if (draw_bottom)
	{
		painter->drawLine(QPointF{x, bottom}, QPointF{x, bottom + length - radius * 2});
		painter->drawEllipse(QPointF{x, bottom + length - radius}, radius, radius);
	}
}

/**
 * @brief The CableCell struct
 * What is needed to draw the cable under a terminal.
 */
struct CableCell
{
	qreal x{0};          ///< x of the middle of the terminal, in the strip coordinates
	qreal bottom{0};     ///< y of the bottom of the terminal
	QString cable;       ///< name of the cable (hose)
	QString wire;        ///< color / number of the wire
	bool shield{false};  ///< true if the terminal is the shield of the cable
};

/**
 * @brief drawCables
 * Draw the cables under the terminals. Consecutive terminals who have the same cable name
 * share the same cable: the wires go down from the terminals, join in the cable,
 * and leave the cable with the same wire marks. The shield is linked to a dashed ellipse
 * drawn around the cable.
 * @param painter : the pen and the font must be set
 * @param cells : one cell per terminal, in the drawing order
 * @param pattern
 */
void drawCables(QPainter *painter, const QVector<CableCell> &cells, const TerminalStripLayoutPattern &pattern)
{
	const qreal wire_length{pattern.m_cable_wire_length};
	if (wire_length <= 0 || cells.isEmpty()) {
		return;
	}

	const qreal cable_length{qMax<qreal>(0, pattern.m_cable_length)};
	const qreal end_length{qMax<qreal>(0, pattern.m_cable_end_length)};
	const qreal radius{2};
	const QFontMetricsF font_metrics{painter->font()};
	const qreal text_height{font_metrics.height()};

		//Draw a mark (small oblique line) on a wire and the number of the wire next to it.
		//The text is written from the bottom to the top, at the left of the wire.
	const auto draw_wire_mark = [&](qreal x, qreal mark_y, const QString &text, bool text_above)
	{
		painter->drawLine(QPointF{x - 3, mark_y + 3}, QPointF{x + 3, mark_y - 3});
		if (text.isEmpty()) {
			return;
		}

		const qreal text_width{font_metrics.horizontalAdvance(text)};
		painter->save();
		painter->translate(x - 1, text_above ? mark_y - 4 : mark_y + 4 + text_width);
		painter->rotate(270);
		painter->drawText(QRectF{0, -text_height, text_width + 2, text_height},
						  Qt::AlignLeft | Qt::AlignVCenter,
						  text);
		painter->restore();
	};

	auto first{0};
	while (first < cells.size())
	{
		if (cells.at(first).cable.isEmpty()) {
			++first;
			continue;
		}

			//Find the last terminal of the cable
		auto last{first};
		while (last + 1 < cells.size() &&
			   cells.at(last + 1).cable == cells.at(first).cable) {
			++last;
		}

		QVector<CableCell> wires, shields;
		qreal max_bottom{cells.at(first).bottom};
		for (auto i = first ; i <= last ; ++i)
		{
			max_bottom = qMax(max_bottom, cells.at(i).bottom);
			if (cells.at(i).shield) {
				shields << cells.at(i);
			} else {
				wires << cells.at(i);
			}
		}
			//A cable with only a shield is drawn like a cable with only one wire
		if (wires.isEmpty()) {
			wires = shields;
			shields.clear();
		}

		const qreal min_x{wires.first().x};
		const qreal max_x{wires.last().x};
		const qreal cable_x{(min_x + max_x) / 2};
		const qreal bar_1_y{max_bottom + wire_length};
		const qreal bar_2_y{bar_1_y + cable_length};
		const qreal end_y{bar_2_y + end_length};

			//Wires between the terminals and the cable
		for (const auto &wire : std::as_const(wires))
		{
			painter->drawLine(QPointF{wire.x, wire.bottom}, QPointF{wire.x, bar_1_y});
			draw_wire_mark(wire.x, qMax(wire.bottom + 4, bar_1_y - 10), wire.wire, true);
		}
		painter->drawLine(QPointF{min_x, bar_1_y}, QPointF{max_x, bar_1_y});

			//The cable
		painter->drawLine(QPointF{cable_x, bar_1_y}, QPointF{cable_x, bar_2_y});

			//Wires after the cable
		painter->drawLine(QPointF{min_x, bar_2_y}, QPointF{max_x, bar_2_y});
		for (const auto &wire : std::as_const(wires))
		{
			painter->drawLine(QPointF{wire.x, bar_2_y}, QPointF{wire.x, end_y});
			draw_wire_mark(wire.x, qMin(bar_2_y + 10, end_y), wire.wire, false);
			painter->drawEllipse(QPointF{wire.x, end_y + radius}, radius, radius);
		}

			//The shield, linked to an ellipse around the cable
		const qreal ellipse_ry{6};
		const qreal ellipse_rx{qMax<qreal>((max_x - min_x) / 2 + 4, 12)};
		const qreal ellipse_y{shields.isEmpty() ? bar_2_y : qMax(bar_1_y + ellipse_ry, bar_2_y - 12)};

		if (!shields.isEmpty())
		{
			for (const auto &shield : std::as_const(shields))
			{
				painter->drawLine(QPointF{shield.x, shield.bottom}, QPointF{shield.x, ellipse_y});
				const qreal edge_x{shield.x > cable_x ? cable_x + ellipse_rx : cable_x - ellipse_rx};
				painter->drawLine(QPointF{shield.x, ellipse_y}, QPointF{edge_x, ellipse_y});
			}

			painter->save();
			auto dashed_pen{painter->pen()};
			dashed_pen.setStyle(Qt::DashLine);
			painter->setPen(dashed_pen);
			painter->drawEllipse(QPointF{cable_x, ellipse_y}, ellipse_rx, ellipse_ry);
			painter->restore();
		}

			//Name of the cable, at the left of the cable, above the ellipse
		const qreal name_width{font_metrics.horizontalAdvance(cells.at(first).cable)};
		const qreal name_top{bar_1_y};
		const qreal name_bottom{shields.isEmpty() ? bar_2_y : ellipse_y - ellipse_ry};
		painter->save();
		painter->translate(cable_x - 2, (name_top + name_bottom) / 2 + name_width / 2);
		painter->rotate(270);
		painter->drawText(QRectF{0, -text_height, name_width + 2, text_height},
						  Qt::AlignLeft | Qt::AlignVCenter,
						  cells.at(first).cable);
		painter->restore();

		first = last + 1;
	}
}

} //End anonymous namespace

/**
 * @brief TerminalStripDrawer::paint
 * @param painter
 */
void TerminalStripDrawer::paint(QPainter *painter)
{
    if (m_strip && m_pattern)
    {
		m_united_xref_text_rect = QRectF();
            //To draw text, QPainter need a Qrect. Instead of create an instance
            //for each text, we re-use the same instance of QRect.
        QRect text_rect;
        painter->save();

		auto pen_{painter->pen()};
		pen_.setColor(Qt::black);
		pen_.setWidth(1);

		auto brush_ = painter->brush();
		brush_.setColor(Qt::white);

        painter->setFont(m_pattern->font());

        painter->setPen(pen_);
        painter->setBrush(brush_);

        if (m_preview_draw)
        {
            painter->save();
            painter->setPen(Qt::blue);
            painter->drawRect(boundingRect());
            painter->drawLine(QPointF{boundingRect().left(), boundingRect().center().y()},
                              QPointF{boundingRect().right(), boundingRect().center().y()});
            painter->restore();
        }

			//Draw header
		painter->drawRect(m_pattern->m_header_rect);

			//Draw the header text
		painter->save();

		if (m_pattern->m_header_text_orientation == Qt::Horizontal)
		{
			text_rect.setRect(0,m_pattern->m_header_rect.y(),m_pattern->m_header_rect.width(),m_pattern->m_header_rect.height());
		}
		else
		{
			painter->translate(m_pattern->m_header_rect.bottomLeft());
			painter->rotate(270);
			text_rect.setRect(0,0,m_pattern->m_header_rect.height(),m_pattern->m_header_rect.width());
		}

        const auto text_{m_strip->installation() + " " + m_strip->location() + " " + m_strip->name()};
        painter->drawText(text_rect, text_, m_pattern->headerTextOption());
		painter->restore();

            //Move painter pos to next drawing
        painter->translate(m_pattern->m_header_rect.width(),0);
        qreal x_offset{m_pattern->m_header_rect.width()};

			//Draw spacer
		painter->drawRect(m_pattern->m_spacer_rect);
			//Move painter pos to next drawing
		painter->translate(m_pattern->m_spacer_rect.width(),0);
		x_offset += m_pattern->m_spacer_rect.width();

            //Draw terminals
        const auto terminals_text_orientation{m_pattern->m_terminals_text_orientation};
        const auto terminals_text_option{m_pattern->terminalsTextOption()};
        const auto terminals_text_height{m_pattern->m_terminals_text_height};
        const auto terminals_text_y{m_pattern->m_terminals_text_y};
        QRectF terminal_rect;

		const auto xref_text_orientation{m_pattern->m_xref_text_orientation};
		const auto xref_text_option{m_pattern->xrefTextOption()};
		const auto xref_text_height{m_pattern->m_xref_text_height};
		const auto xref_text_y{m_pattern->m_xref_text_y};
		QRectF xref_rect;

		QHash<QUuid, QVector<QPointF>> bridges_anchor_points;
		QVector<CableCell> cable_cells;

		m_hovered_xref = hoverTerminal{};
		int physical_index = 0;
            //Loop over physical terminals
        for (const auto &physical_t : m_strip->physicalTerminal())
        {
                //Get the good offset according to how many level have the current physical terminal
            const QVector<QSharedPointer<AbstractRealTerminalInterface>> real_terminal_vector{physical_t->realTerminals()};
            const auto real_t_count{real_terminal_vector.size()};
            const auto offset_{4 - real_t_count};

				//Loop over real terminals
			for (auto i=0 ; i<real_t_count ; ++i)
			{
				const auto index_ = offset_ + i;
				if (index_ >= 4) {
					break;
				}

                terminal_rect = m_pattern->m_terminal_rect[index_];

					//Cable (hose) of this terminal
				CableCell cable_cell;
				if (real_terminal_vector[i])
				{
					cable_cell.x = x_offset + terminal_rect.width()/2;
					cable_cell.bottom = terminal_rect.y() + terminal_rect.height();
					cable_cell.cable = real_terminal_vector[i]->cable();
					cable_cell.wire = real_terminal_vector[i]->cableWire();
					cable_cell.shield = real_terminal_vector[i]->isShield();
				}
				const bool have_cable{m_pattern->m_cable_wire_length > 0 && !cable_cell.cable.isEmpty()};
				cable_cells.append(cable_cell);
                    //Draw terminal rect
                painter->drawRect(terminal_rect);

					//Draw the symbol of the terminal type (fuse, ground, led...)
				if (m_pattern->m_type_symbol_height > 0 && real_terminal_vector[i])
				{
					drawTypeSymbol(painter,
								   QRectF{0, m_pattern->m_type_symbol_y,
										  terminal_rect.width(), m_pattern->m_type_symbol_height},
								   real_terminal_vector[i]->type(),
								   real_terminal_vector[i]->isLed());
				}

					//Draw the connections, above and under the terminal
				if (m_pattern->m_connection_length > 0) {
					//The cable replace the connection under the terminal
					drawConnections(painter, terminal_rect, m_pattern->m_connection_length, !have_cable);
				}
                    //Draw a stronger line if the current terminal have level
                    //and the current level is the first
                if (real_t_count > 1 && i == 0)
                {
                    painter->save();
                    pen_ = painter->pen();
                    pen_.setWidth(4);
                    pen_.setCapStyle(Qt::FlatCap);
                    painter->setPen(pen_);
                    const auto p1 { terminal_rect.topLeft() };
                        //We can't use terminal_rect.bottomLeft for p2 because
                        //the returned value deviate from the true value
                        //(see Qt documentation about QRect)
                    const QPointF p2 { p1.x(), p1.y() + terminal_rect.height() };
                    painter->drawLine(p1, p2);
                    painter->restore();
                }

				if(m_preview_draw)
                {
                    painter->save();
                    painter->setPen(Qt::yellow);
                    painter->drawLine(QPointF{terminal_rect.x(), terminal_rect.y() + terminal_rect.height()/2},
                                      QPointF{terminal_rect.width(), terminal_rect.y() + terminal_rect.height()/2});
                    painter->restore();
				}

                    //Draw text
                painter->save();
                text_rect.setRect(0, terminals_text_y, terminal_rect.width(), terminals_text_height);
                if (terminals_text_orientation == Qt::Vertical)
                {
                    painter->translate(text_rect.bottomLeft());
                    painter->rotate(270);
                    text_rect.setRect(0, 0, text_rect.height(), text_rect.width());
                }

                const auto shared_real_terminal{real_terminal_vector[i]};
				painter->drawText(text_rect,
								  shared_real_terminal ? shared_real_terminal->label() : QLatin1String(),
								  terminals_text_option);

				if (m_preview_draw)
				{
					painter->setPen(Qt::blue);
					painter->drawRect(text_rect);
				}

				painter->restore();

					//Draw xref
				xref_rect.setRect(0, xref_text_y, terminal_rect.width(), xref_text_height);
				painter->save();
				if (xref_text_orientation == Qt::Vertical)
				{
					painter->translate(xref_rect.bottomLeft());
					painter->rotate(270);
					xref_rect.setRect(0, 0, xref_rect.height(), xref_rect.width());
				}

				QTransform transform;
				transform.translate(x_offset, 0);

				if (xref_text_orientation == Qt::Vertical)
				{
					transform.translate(0, xref_text_y + xref_text_height);
					transform.rotate(270);
				}

				auto xref_string = shared_real_terminal->xref();

				const auto mapped_xref_text_rect = transform.mapRect(painter->boundingRect(xref_rect, xref_string, xref_text_option));
				if (m_united_xref_text_rect.isNull()) {
					m_united_xref_text_rect = mapped_xref_text_rect;
				} else {
					m_united_xref_text_rect = m_united_xref_text_rect.united(mapped_xref_text_rect);
				}

					//if mouse hover the xref text, draw it in blue to advise user the xref is clickable.
				if (!m_mouse_hover_pos.isNull() && mapped_xref_text_rect.contains(m_mouse_hover_pos)) {
					painter->setPen(Qt::blue);
					m_hovered_xref.physical = physical_index;
					m_hovered_xref.real = i;
				}

				painter->drawText(xref_rect, xref_string, xref_text_option);

				if (m_preview_draw)
				{
					painter->setPen(Qt::blue);
					painter->drawRect(xref_rect);
				}
				painter->restore();

                    //Add bridge anchor
                if (shared_real_terminal->isBridged())
                {
                    painter->save();
                    if (QScopedPointer<AbstractBridgeInterface> bridge_ {
                        shared_real_terminal->bridge() })
                    {
                        const auto x_anchor{terminal_rect.width()/2};
                        const auto y_anchor {m_pattern->m_bridge_point_y_offset[index_]};
                        const auto radius_anchor{m_pattern->m_bridge_point_d/2};

						painter->setBrush(Qt::SolidPattern);
						painter->drawEllipse(QPointF(x_anchor, y_anchor),
											 radius_anchor, radius_anchor);

						auto anchor_points{bridges_anchor_points.value(bridge_->uuid())};
						anchor_points.append(QPointF(x_offset + x_anchor, y_anchor));
						bridges_anchor_points.insert(bridge_->uuid(), anchor_points);
					}
					painter->restore();
				}

                    //Move painter pos to next drawing
                painter->translate(terminal_rect.width(),0);
                x_offset += terminal_rect.width();
            }
			physical_index++;
        }
        painter->restore();

			//Draw the bridges
		for (const auto &points_ : std::as_const(bridges_anchor_points))
		{
			painter->save();
			auto pen_{painter->pen()};
			pen_.setWidth(2);
			painter->setPen(pen_);
			painter->drawPolyline(QPolygonF(points_));
			painter->restore();
		}

			//Draw the cables
		painter->save();
		auto cable_pen{painter->pen()};
		cable_pen.setColor(Qt::black);
		cable_pen.setWidth(1);
		painter->setPen(cable_pen);
		painter->setFont(m_pattern->font());
		painter->setBrush(Qt::NoBrush);
		drawCables(painter, cable_cells, *m_pattern);
		painter->restore();
	}
}

QRectF TerminalStripDrawer::boundingRect() const
{
	QRectF rect_{0, 0, width(), height()};

	if (m_pattern)
	{
		qreal extra_bottom{0};

			//The connections are drawn above and under the terminals
		if (m_pattern->m_connection_length > 0)
		{
			const auto length_{m_pattern->m_connection_length};
			qreal top_{0};
			for (const auto &terminal_rect : std::as_const(m_pattern->m_terminal_rect)) {
				top_ = std::min(top_, terminal_rect.top() - length_);
			}
			rect_.setTop(top_);
			extra_bottom = length_;
		}

			//The cables are drawn under the terminals
		if (m_strip && m_pattern->m_cable_wire_length > 0)
		{
			bool have_cable{false};
			for (const auto &physical_t : m_strip->physicalTerminal())
			{
				for (const auto &real_t : physical_t->realTerminals())
				{
					if (real_t && !real_t->cable().isEmpty()) {
						have_cable = true;
						break;
					}
				}
				if (have_cable) {
					break;
				}
			}

			if (have_cable)
			{
					//4 is the diameter of the circle at the end of the wires
				extra_bottom = std::max(extra_bottom,
										m_pattern->m_cable_wire_length
										+ std::max<qreal>(0, m_pattern->m_cable_length)
										+ std::max<qreal>(0, m_pattern->m_cable_end_length)
										+ 4);
			}
		}

		rect_.setBottom(rect_.bottom() + extra_bottom);
	}

	return rect_;
}

void TerminalStripDrawer::setLayout(QSharedPointer<TerminalStripLayoutPattern> layout)
{
	m_pattern = layout;
}

bool TerminalStripDrawer::haveLayout() const
{
	return !m_pattern.isNull();
}

void TerminalStripDrawer::setPreviewDraw(bool draw) {
	m_preview_draw = draw;
}

void TerminalStripDrawer::setMouseHoverPos(const QPointF &pos)
{
	m_last_mouse_pos_in_xrefs_rect = m_united_xref_text_rect.contains(m_mouse_hover_pos);
	m_mouse_hover_pos = pos;
}

/**
 * @brief TerminalStripDrawer::mouseHoverXref
 * @return True if the mouse position (given through the function setMouseHoverPos)
 * hover the rect of a xref.
 */
bool TerminalStripDrawer::mouseHoverXref() const {
	return m_united_xref_text_rect.contains(m_mouse_hover_pos);
}

bool TerminalStripDrawer::needUpdate()
{
	if (mouseHoverXref()) {
		return true;
	} else if (m_last_mouse_pos_in_xrefs_rect) {
		return true;
	}
	return false;
}

/**
 * @brief TerminalStripDrawer::hoveredXref
 * @return the current terminal hovered by the mouse
 * in the xref bounding rectangle
 */
hoverTerminal TerminalStripDrawer::hoveredXref() const
{
	return m_hovered_xref;
}

qreal TerminalStripDrawer::height() const
{
	if (m_pattern)
	{
		auto height_{m_pattern->m_header_rect.y() + m_pattern->m_header_rect.height()};

		height_ = std::max(height_, m_pattern->m_spacer_rect.y() + m_pattern->m_spacer_rect.height());

		for (const auto &rect : m_pattern->m_terminal_rect) {
			height_ = std::max(height_, rect.y() + rect.height());
		}

		return height_;
	}

	return 0;
}

qreal TerminalStripDrawer::width() const
{
    if (m_pattern)
    {
        qreal width_{m_pattern->m_header_rect.width() + m_pattern->m_spacer_rect.width()};

		if (m_strip)
		{
			//Loop over physical terminals
			for (const auto &physical_t : m_strip->physicalTerminal())
			{
				//Get the good offset according to how many level have the current physical terminal
				const QVector<QSharedPointer<AbstractRealTerminalInterface>> real_terminal_vector{physical_t->realTerminals()};
				const auto real_t_count{real_terminal_vector.size()};
				const auto offset_{4 - real_t_count};

				//Loop over real terminals
				for (auto i=0 ; i<real_t_count ; ++i)
				{
					const auto index_ = offset_ + i;
					if (index_ >= 4) {
						break;
					}

					width_ += m_pattern->m_terminal_rect[index_].width();
				}
			}
		}

		return width_;
	}

	return 0;
}

} //End namespace TerminalStripDrawer
