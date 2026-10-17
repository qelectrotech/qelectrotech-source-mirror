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
#include "terminalstripdrawerhelpers.h"
#include "realisticterminalstripdrawer.h"

#include <QPainter>
#include <QPolygonF>
#include <QPainterPath>
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

	//Two diodes side by side, wired in anti-parallel between the top and
	//the bottom of the terminal (current flows through the left one from
	//the top to the bottom and through the right one the other way)
void drawAntiParallelDiodesSymbol(QPainter *painter)
{
	painter->drawLine(QPointF{0, -7}, QPointF{0, -5});
	painter->drawLine(QPointF{-5, -5}, QPointF{5, -5});
	painter->drawLine(QPointF{0, 5}, QPointF{0, 7});
	painter->drawLine(QPointF{-5, 5}, QPointF{5, 5});

		//Left diode, anode on top
	const QPointF left_diode[3] { {-8, -3}, {-2, -3}, {-5, 2} };
	painter->drawLine(QPointF{-5, -5}, QPointF{-5, -3});
	painter->drawPolygon(left_diode, 3);
	painter->drawLine(QPointF{-8, 2}, QPointF{-2, 2});
	painter->drawLine(QPointF{-5, 2}, QPointF{-5, 5});

		//Right diode, anode at the bottom
	const QPointF right_diode[3] { {2, 2}, {8, 2}, {5, -3} };
	painter->drawLine(QPointF{5, -5}, QPointF{5, -3});
	painter->drawPolygon(right_diode, 3);
	painter->drawLine(QPointF{2, -3}, QPointF{8, -3});
	painter->drawLine(QPointF{5, 2}, QPointF{5, 5});
}

void drawResistorSymbol(QPainter *painter)
{
	painter->drawLine(QPointF{0, -7}, QPointF{0, -5});
	painter->drawRect(QRectF{-3, -5, 6, 10});
	painter->drawLine(QPointF{0, 5}, QPointF{0, 7});
}

	//A resistor crossed by an oblique line ended by a short bar
void drawVaristorSymbol(QPainter *painter)
{
	drawResistorSymbol(painter);
	painter->drawLine(QPointF{-5, 5}, QPointF{5, -5});
	painter->drawLine(QPointF{5, -5}, QPointF{8, -5});
}

	//A measuring point : the line is interrupted by a small circle
void drawTestSocketSymbol(QPainter *painter)
{
	const qreal r{2.5};
	painter->drawLine(QPointF{0, -7}, QPointF{0, -r});
	painter->drawEllipse(QPointF{0, 0}, r, r);
	painter->drawLine(QPointF{0, r}, QPointF{0, 7});
}

	//Same as the diode, the anode is at the bottom
void drawDiodeReversedSymbol(QPainter *painter)
{
	const qreal t{symbol_height / 3};
	const QPointF triangle[3] { {-t, t}, {t, t}, {0, -t} };
	painter->drawLine(QPointF{0, -7}, QPointF{0, -t});
	painter->drawLine(QPointF{-t, -t}, QPointF{t, -t});
	painter->drawPolygon(triangle, 3);
	painter->drawLine(QPointF{0, t}, QPointF{0, 7});
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
			case ElementData::TTAntiParallelDiodes : drawAntiParallelDiodesSymbol(p); break;
			case ElementData::TTResistor   : drawResistorSymbol(p); break;
			case ElementData::TTVaristor   : drawVaristorSymbol(p); break;
			case ElementData::TTTestSocket : drawTestSocketSymbol(p); break;
			case ElementData::TTDiodeReversed : drawDiodeReversedSymbol(p); break;
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
 * A connection is a short line, without circle at its end.
 * @param painter
 * @param terminal_rect
 * @param length : the total length of a connection
 * @param draw_bottom : false if the cable is drawn under the terminal
 */
void drawConnections(QPainter *painter, const QRectF &terminal_rect, qreal length, bool draw_bottom)
{
	const qreal x{terminal_rect.width() / 2};
	const qreal top{terminal_rect.top()};
	const qreal bottom{terminal_rect.top() + terminal_rect.height()};

	painter->drawLine(QPointF{x, top}, QPointF{x, top - length});

	if (draw_bottom) {
		painter->drawLine(QPointF{x, bottom}, QPointF{x, bottom + length});
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

/*
 * Width and color of a terminal.
 */

/**
 * The width of the terminals of the layout is the width of a terminal
 * of 5.2 mm (the usual width of the terminals of 2.5 mm2).
 * The width set by the user, in mm, is converted according to this reference.
 */
constexpr qreal reference_terminal_width_mm{5.2};

/**
 * @brief terminalRect
 * @param pattern
 * @param index : level of the terminal (0 to 3)
 * @param real_t : can be null
 * @return The rectangle of the terminal : the one of the layout,
 * with the width adapted to the width (in mm) of @a real_t if it is set.
 */
QRectF terminalRect(const TerminalStripLayoutPattern &pattern,
					int index,
					const QSharedPointer<AbstractRealTerminalInterface> &real_t)
{
	auto rect_{pattern.m_terminal_rect[index]};
	if (real_t && real_t->widthMm() > 0) {
		rect_.setWidth(rect_.width() * real_t->widthMm() / reference_terminal_width_mm);
	}
	return rect_;
}

/**
 * @brief The TerminalColors struct
 * Colors used to fill the rectangle of a terminal.
 * An invalid fill color means that the rectangle is not filled.
 * The stripe color (if valid) is drawn as stripes over the fill color.
 */
struct TerminalColors
{
	QColor fill;
	QColor stripe;
};

/**
 * @brief terminalColors
 * @param real_t : can be null
 * @return The colors of the terminal : the color set by the user,
 * else the default color of the type of the terminal.
 */
TerminalColors terminalColors(const QSharedPointer<AbstractRealTerminalInterface> &real_t)
{
	if (!real_t) {
		return {};
	}
	if (real_t->color().isValid()) {
		return {real_t->color(), real_t->stripeColor()};
	}
	switch (real_t->type()) {
		case ElementData::TTGround: return {QColor(255, 216, 0), QColor(46, 158, 46)};	//Green/Yellow
		default: return {QColor(150, 150, 150), QColor()};	//Grey housing, like a real terminal block
	}
}

/**
 * @brief terminalBodyPath
 * @param rect : rectangle of the terminal
 * @return The outline of a terminal block seen from the front : a body of the full width
 * with, at the top and at the bottom, a slightly narrower cap (the wire entry),
 * linked to the body by a small slope.
 * Two neighbouring terminals touch each other by their bodies and leave a small notch between their caps.
 */
QPainterPath terminalBodyPath(const QRectF &rect)
{
	QPainterPath path_;
	if (rect.width() <= 0 || rect.height() <= 0) {
		return path_;
	}

	const qreal inset_ = qMin(qreal(1.0), rect.width() * 0.1);
	const qreal cap_ = qMin(qreal(6.0), rect.height() * 0.06);
	const qreal slope_ = qMin(qreal(2.0), rect.height() * 0.03);
	const qreal l_ = rect.left(), r_ = rect.right(), t_ = rect.top(), b_ = rect.bottom();

	QPolygonF outline_;
	outline_ << QPointF(l_ + inset_, t_)
			 << QPointF(r_ - inset_, t_)
			 << QPointF(r_ - inset_, t_ + cap_)
			 << QPointF(r_, t_ + cap_ + slope_)
			 << QPointF(r_, b_ - cap_ - slope_)
			 << QPointF(r_ - inset_, b_ - cap_)
			 << QPointF(r_ - inset_, b_)
			 << QPointF(l_ + inset_, b_)
			 << QPointF(l_ + inset_, b_ - cap_)
			 << QPointF(l_, b_ - cap_ - slope_)
			 << QPointF(l_, t_ + cap_ + slope_)
			 << QPointF(l_ + inset_, t_ + cap_);
	path_.addPolygon(outline_);
	path_.closeSubpath();
	return path_;
}

/**
 * @brief The Zone struct
 * A vertical zone (relative to the top of a terminal) already used by a text or a symbol.
 */
struct Zone
{
	qreal top{0};
	qreal bottom{0};
};

/**
 * @brief drawTerminalBody Draw a terminal housing seen from the front,
 * like in a terminal line-up diagram : a grey body with wire entries (caps) at both ends,
 * and clamping screws drawn in the free zones of the body.
 * @param painter
 * @param rect : rectangle of the terminal
 * @param used_zones : zones used by the texts and the symbols (in the frame of the strip),
 * the screws are drawn out of them.
 * @param bottom_line : draw the line between the body and the lower wire entry.
 * Must be false if a symbol (fuse, led...) is drawn over this line.
 * The details are deliberately schematic and scale with the terminal width.
 */
void drawTerminalBody(QPainter *painter, const QRectF &rect, const QVector<Zone> &used_zones, bool bottom_line)
{
	if (rect.width() <= 0 || rect.height() <= 0) {
		return;
	}

	painter->drawPath(terminalBodyPath(rect));

	if (rect.width() < 5.0 || rect.height() < 12.0) {
		return;
	}

	const qreal cap_ = qMin(qreal(6.0), rect.height() * 0.06);
	const qreal slope_ = qMin(qreal(2.0), rect.height() * 0.03);
	const qreal body_top_ = rect.top() + cap_ + slope_;
	const qreal body_bottom_ = rect.bottom() - cap_ - slope_;

		//Limit between the body and the wire entries
	painter->drawLine(QPointF(rect.left(), body_top_),
					  QPointF(rect.right(), body_top_));
	if (bottom_line) {
		painter->drawLine(QPointF(rect.left(), body_bottom_),
						  QPointF(rect.right(), body_bottom_));
	}

		//Free zones of the body : what remains when the used zones are removed
	QVector<Zone> free_zones_;
	free_zones_.append(Zone{body_top_, body_bottom_});
	const qreal margin_ = 2.5;
	for (const auto &used_ : used_zones)
	{
		QVector<Zone> new_free_;
		for (const auto &free_ : std::as_const(free_zones_))
		{
			if (used_.bottom + margin_ <= free_.top || used_.top - margin_ >= free_.bottom) {
				new_free_.append(free_);
				continue;
			}
			if (used_.top - margin_ > free_.top) {
				new_free_.append(Zone{free_.top, used_.top - margin_});
			}
			if (used_.bottom + margin_ < free_.bottom) {
				new_free_.append(Zone{used_.bottom + margin_, free_.bottom});
			}
		}
		free_zones_ = new_free_;
	}

		//In each free zone, draw the clamping parts like on a terminal block seen from the front :
		//screw, castle tab, big center circle, castle tab, screw.
		//If the zone is too small, only the center circle or one screw is drawn.
	const qreal screw_r_ = qMin(qreal(5.0), rect.width() * 0.3);
	const qreal center_r_ = qMin(screw_r_ * 1.5, rect.width() * 0.4);
	if (screw_r_ < 1.0) {
		return;
	}
	const qreal screw_h_ = screw_r_ * 2 + 3;
	const qreal center_h_ = center_r_ * 2 + 2;
	const qreal tab_h_ = 4.0;
	const qreal tab_w_ = qMin(rect.width() * 0.3, qreal(5.0));
	const qreal cx_ = rect.center().x();

	auto draw_screw = [&](qreal y_, qreal r_, bool window_)
	{
		const QPointF c_{cx_, y_};
		if (window_) {
			const qreal half_{qMin(r_ + 1.5, rect.width() / 2 - 0.8)};
			if (half_ > r_ + 0.3) {
				painter->drawRoundedRect(QRectF(cx_ - half_, c_.y() - half_, half_ * 2, half_ * 2), 1.0, 1.0);
			}
		}
		painter->drawEllipse(c_, r_, r_);
		painter->drawLine(QPointF(c_.x() - r_ * 0.6, c_.y()), QPointF(c_.x() + r_ * 0.6, c_.y()));
	};
	auto draw_tab = [&](qreal y_)
	{
		painter->drawRect(QRectF(cx_ - tab_w_, y_ - tab_h_ / 2, tab_w_ * 2, tab_h_));
	};

	for (const auto &free_ : std::as_const(free_zones_))
	{
		const qreal h_ = free_.bottom - free_.top;
		if (h_ >= screw_h_ * 2 + center_h_ + tab_h_ * 2 + 2)
		{
			const qreal top_y_ = free_.top + screw_h_ / 2;
			const qreal bottom_y_ = free_.bottom - screw_h_ / 2;
			const qreal middle_y_ = (top_y_ + bottom_y_) / 2;
			draw_screw(top_y_, screw_r_, true);
			draw_tab((top_y_ + screw_h_ / 2 + middle_y_ - center_h_ / 2) / 2);
			draw_screw(middle_y_, center_r_, false);
			draw_tab((middle_y_ + center_h_ / 2 + bottom_y_ - screw_h_ / 2) / 2);
			draw_screw(bottom_y_, screw_r_, true);
		}
		else if (h_ >= center_h_ + 2) {
			draw_screw((free_.top + free_.bottom) / 2, center_r_, false);
		}
		else if (h_ >= screw_h_) {
			draw_screw((free_.top + free_.bottom) / 2, screw_r_, true);
		}
	}
}

/**
 * @brief markerTagLength
 * Like the real marker tags, every tag of a strip have the same size,
 * whatever the text written on it ("1", "10"...).
 * The length is the one needed by the longest number of the strip
 * (at least two digits), limited to the text zone.
 * @param metrics : metrics of the font used to draw the text
 * @param longest_number : the longest number of the strip
 * @param zone_length : length of the text zone, along the text
 * @return the length of the marker tag along the text
 */
qreal markerTagLength(const QFontMetricsF &metrics, const QString &longest_number, qreal zone_length)
{
	const qreal text_ = qMax(metrics.horizontalAdvance(QStringLiteral("00")),
							 metrics.horizontalAdvance(longest_number));
	return qMin(zone_length, text_ + 4.0);
}

/**
 * @brief markerTagZone
 * @return The vertical zone (in the frame of the strip) really covered by the marker tag of a text,
 * which is smaller than the rectangle of the text.
 * @param metrics : metrics of the font used to draw the text
 * @param text_y : y of the rectangle of the text
 * @param text_height : height of the rectangle of the text
 * @param text : text drawn
 * @param option : option used to draw the text
 * @param vertical : true if the text is rotated
 * @param tag_length : length of the marker tags of the strip, along the text
 */
Zone markerTagZone(const QFontMetricsF &metrics, qreal text_y, qreal text_height,
				   const QString &text, const QTextOption &option, bool vertical,
				   qreal tag_length)
{
	if (!vertical)
	{
		const qreal cross_ = qMin(text_height, metrics.height() + 2);
		return Zone{text_y + (text_height - cross_) / 2, text_y + (text_height + cross_) / 2};
	}
	Q_UNUSED(text)
		//Same length for every terminal, so the body of every terminal is drawn the same way
	const qreal along_ = qMin(text_height, tag_length);
	if (option.alignment() & Qt::AlignRight) {
		return Zone{text_y, text_y + along_};
	}
	if (option.alignment() & Qt::AlignHCenter) {
		return Zone{text_y + (text_height - along_) / 2, text_y + (text_height + along_) / 2};
	}
	return Zone{text_y + text_height - along_, text_y + text_height};
}

/**
 * @brief drawMarkerTag
 * Draw the marker tag of a terminal (white, or black and ribbed for an isolating terminal)
 * behind a text. The tag fits the text and follows its alignment.
 * Must be called with the painter in the frame of @a text_rect (the one used to draw the text).
 * @param painter
 * @param text_rect : rectangle used to draw the text
 * @param text : text drawn
 * @param option : option used to draw the text
 * @param vertical : true if the text is rotated
 * @param sectional : true for an isolating terminal
 * @param tag_length : length of the marker tags of the strip, along the text
 */
void drawMarkerTag(QPainter *painter, const QRectF &text_rect, const QString &text,
				   const QTextOption &option, bool vertical, bool sectional,
				   qreal tag_length)
{
	if (text.isEmpty()) {
		return;
	}
	const QFontMetricsF metrics_{painter->font()};
		//Vertical text : fixed length, like a real marker tag.
		//Horizontal text : the tag take the whole width of the terminal.
	const qreal along_ = vertical ? qMin(text_rect.width(), tag_length) : text_rect.width();
	const qreal cross_ = vertical ?
							 text_rect.height() - 2 * qMin(qreal(1.2), text_rect.height() * 0.12) :
							 qMin(text_rect.height(), metrics_.height() + 2);
	qreal x_{text_rect.left()};
	if (option.alignment() & Qt::AlignRight) {
		x_ = text_rect.right() - along_;
	} else if (option.alignment() & Qt::AlignHCenter) {
		x_ = text_rect.left() + (text_rect.width() - along_) / 2;
	}
	const QRectF tag_rect_{x_, text_rect.top() + (text_rect.height() - cross_) / 2, along_, cross_};

	painter->save();
	painter->setBrush(sectional ? Qt::black : Qt::white);
	painter->drawRoundedRect(tag_rect_, 0.8, 0.8);
	if (sectional)
	{
			//Ribbed grip of the isolating terminals
		painter->setPen(QPen(QColor(90, 90, 90), 0));
		for (auto rib_x_ = tag_rect_.left() + 1.5 ; rib_x_ < tag_rect_.right() - 0.5 ; rib_x_ += 1.5) {
			painter->drawLine(QPointF(rib_x_, tag_rect_.top() + 0.5),
							  QPointF(rib_x_, tag_rect_.bottom() - 0.5));
		}
	}
	painter->restore();
	painter->setPen(sectional ? Qt::white : Qt::black);

		//The text is longer than the tag ("100", "PE"...) : reduce the font, like on a real tag.
	const qreal available_ = along_ - 2.0;
	const qreal text_length_ = metrics_.horizontalAdvance(text);
	if (text_length_ > available_ && available_ > 0)
	{
		auto font_{painter->font()};
		const auto size_{font_.pixelSize() > 0 ? qreal(font_.pixelSize()) : font_.pointSizeF()};
		const auto new_size_{std::max<qreal>(3.0, size_ * available_ / text_length_)};
		if (font_.pixelSize() > 0) {
			font_.setPixelSize(std::max(1, static_cast<int>(new_size_)));
		} else {
			font_.setPointSizeF(new_size_);
		}
		painter->setFont(font_);
	}
}

/**
 * @brief terminalNumber
 * @param label : label of a terminal, for example "XAUZ1:1"
 * @return The number of the terminal, the part after the last ':' ("1").
 * The strip name is already written in the header of the strip.
 * If the label have no ':', return the label.
 */
QString terminalNumber(const QString &label)
{
	const auto index_ = label.lastIndexOf(QLatin1Char(':'));
	return index_ < 0 ? label : label.mid(index_ + 1).trimmed();
}

/**
 * @brief fitTextToCell
 * The text of a terminal is drawn vertically (or not) inside the width of the terminal.
 * If the font is too big for a narrow terminal, the text overlaps the next ones.
 * Clip the painter to the width of the terminal and reduce the font if needed.
 * Must be called with the painter at the left of the terminal, before any rotation.
 * @param painter
 * @param cell_width : width of the terminal
 * @param text_is_vertical : true if the text is rotated (the height of the font is then along the width of the terminal)
 */
void fitTextToCell(QPainter *painter, qreal cell_width, bool text_is_vertical)
{
	painter->setClipRect(QRectF(0, -100000, cell_width, 200000), Qt::IntersectClip);

	if (!text_is_vertical || cell_width <= 0) {
		return;
	}
	auto font_{painter->font()};
	const auto available_{std::max<qreal>(1.0, cell_width - 1.0)};
	const QFontMetricsF metrics_{font_};
	if (metrics_.height() > available_ && metrics_.height() > 0)
	{
		const auto size_{font_.pixelSize() > 0 ? qreal(font_.pixelSize()) : font_.pointSizeF()};
		const auto new_size_{std::max<qreal>(3.0, size_ * available_ / metrics_.height())};
		if (font_.pixelSize() > 0) {
			font_.setPixelSize(static_cast<int>(new_size_));
		} else {
			font_.setPointSizeF(new_size_);
		}
		painter->setFont(font_);
	}
}

/**
 * @brief drawStripes
 * Draw diagonal stripes of @a color inside @a rect.
 */
void drawStripes(QPainter *painter, const QRectF &rect, const QColor &color)
{
	painter->save();
	painter->setClipPath(terminalBodyPath(rect));
	painter->setPen(Qt::NoPen);
	painter->setBrush(color);

	const auto band_{std::max<qreal>(2, rect.width() / 2)};
	for (auto x = rect.left() - rect.height() ; x < rect.right() ; x += 2 * band_)
	{
		QPolygonF polygon_;
		polygon_ << QPointF(x, rect.bottom())
				 << QPointF(x + band_, rect.bottom())
				 << QPointF(x + band_ + rect.height(), rect.top())
				 << QPointF(x + rect.height(), rect.top());
		painter->drawPolygon(polygon_);
	}
	painter->restore();
}

/**
 * @return The color (black or white) the more readable on @a background.
 * If @a background is invalid, return black.
 */
QColor contrastColor(const QColor &background)
{
	if (!background.isValid()) {
		return Qt::black;
	}
	const auto luminance_{0.299 * background.redF() + 0.587 * background.greenF() + 0.114 * background.blueF()};
	return luminance_ < 0.5 ? QColor(Qt::white) : QColor(Qt::black);
}

} //End anonymous namespace

/*
 * Helpers : give to the other drawing styles access to
 * the functions of the anonymous namespace.
 */
qreal Helpers::referenceTerminalWidthMm() {
	return reference_terminal_width_mm;
}

void Helpers::terminalColors(const QSharedPointer<AbstractRealTerminalInterface> &real_t,
							 QColor &fill, QColor &stripe)
{
	const auto colors_{::TerminalStripDrawer::terminalColors(real_t)};
	fill = colors_.fill;
	stripe = colors_.stripe;
}

QColor Helpers::contrastColor(const QColor &background) {
	return ::TerminalStripDrawer::contrastColor(background);
}

QString Helpers::terminalNumber(const QString &label) {
	return ::TerminalStripDrawer::terminalNumber(label);
}

void Helpers::drawTypeSymbol(QPainter *painter, const QRectF &box,
							 ElementData::TerminalType type, bool led) {
	::TerminalStripDrawer::drawTypeSymbol(painter, box, type, led);
}

/**
 * @brief TerminalStripDrawer::paint
 * @param painter
 */
void TerminalStripDrawer::paint(QPainter *painter)
{
	if (m_strip && m_pattern && m_draw_style != TerminalStripLayoutPattern::DrawStyle::Schematic)
	{
			//The realistic styles don't write the xrefs, but each terminal
			//is a clickable zone, like the xref text of the schematic style.
		m_united_xref_text_rect = QRectF();
		m_xref_rects.clear();
		m_hovered_xref = hoverTerminal{};
		Realistic::paint(painter, *m_strip, *m_pattern, m_draw_style, m_preview_draw,
						 m_mouse_hover_pos, &m_xref_rects);
		for (const auto &xref_rect : std::as_const(m_xref_rects))
		{
			m_united_xref_text_rect = m_united_xref_text_rect.isNull() ?
										  xref_rect.rect :
										  m_united_xref_text_rect.united(xref_rect.rect);
			if (m_hovered_xref.physical < 0 && !m_mouse_hover_pos.isNull()
				&& xref_rect.rect.contains(m_mouse_hover_pos)) {
				m_hovered_xref.physical = xref_rect.physical;
				m_hovered_xref.real = xref_rect.real;
			}
		}
		return;
	}

    if (m_strip && m_pattern)
    {
		m_united_xref_text_rect = QRectF();
		m_xref_rects.clear();
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

			//Length of the longest xref of the strip, used to reserve the same
			//zone in the body of every terminal.
		qreal xref_zone_length_{0};
		{
			const QFontMetricsF xref_metrics_{painter->font()};
			for (const auto &physical_t : m_strip->physicalTerminal()) {
				for (const auto &real_t : physical_t->realTerminals()) {
					if (real_t && !real_t->xref().isEmpty()) {
						xref_zone_length_ = qMax(xref_zone_length_,
												 xref_metrics_.horizontalAdvance(real_t->xref()) + 4.0);
					}
				}
			}
			xref_zone_length_ = qMin(xref_zone_length_, qreal(xref_text_height));
		}

			//Length of the marker tags of the strip, calibrated on the longest number
		qreal tag_length_{0};
		{
			QString longest_number_;
			const QFontMetricsF tag_metrics_{painter->font()};
			for (const auto &physical_t : m_strip->physicalTerminal()) {
				for (const auto &real_t : physical_t->realTerminals()) {
					if (!real_t) {
						continue;
					}
					const auto number_{terminalNumber(real_t->label())};
					if (tag_metrics_.horizontalAdvance(number_) > tag_metrics_.horizontalAdvance(longest_number_)) {
						longest_number_ = number_;
					}
				}
			}
			tag_length_ = markerTagLength(tag_metrics_, longest_number_, terminals_text_height);
		}

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

                terminal_rect = terminalRect(*m_pattern, index_, real_terminal_vector[i]);

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
                    //Draw terminal rect, filled with the color of the terminal (if any)
				const auto colors_{terminalColors(real_terminal_vector[i])};
				const auto fill_color_{colors_.fill};
				const auto text_color_{contrastColor(fill_color_)};
				painter->save();
				if (fill_color_.isValid()) {
					painter->setBrush(QBrush(fill_color_));
				}
				QVector<Zone> used_zones_;
				if (real_terminal_vector[i])
				{
					const QFontMetricsF metrics_{painter->font()};
					used_zones_.append(markerTagZone(metrics_, terminals_text_y, terminals_text_height,
													 terminalNumber(real_terminal_vector[i]->label()),
													 terminals_text_option,
													 terminals_text_orientation == Qt::Vertical,
													 tag_length_));
						//The zone of the xref have the length of the longest xref of the strip,
						//so the body of every terminal is drawn the same way.
					if (xref_text_orientation == Qt::Vertical)
					{
						if (xref_text_option.alignment() & Qt::AlignRight) {
							used_zones_.append(Zone{xref_text_y, xref_text_y + xref_zone_length_});
						} else if (xref_text_option.alignment() & Qt::AlignHCenter) {
							used_zones_.append(Zone{xref_text_y + (xref_text_height - xref_zone_length_) / 2,
													xref_text_y + (xref_text_height + xref_zone_length_) / 2});
						} else {
							used_zones_.append(Zone{xref_text_y + xref_text_height - xref_zone_length_,
													xref_text_y + xref_text_height});
						}
					} else {
						const qreal cross_ = qMin(xref_text_height, metrics_.height() + 2);
						used_zones_.append(Zone{xref_text_y + (xref_text_height - cross_) / 2,
												xref_text_y + (xref_text_height + cross_) / 2});
					}
				}
				const bool have_symbol_{m_pattern->m_type_symbol_height > 0 && real_terminal_vector[i]
										&& (real_terminal_vector[i]->type() != ElementData::TTGeneric
											|| real_terminal_vector[i]->isLed())};
				if (have_symbol_) {
					used_zones_.append(Zone{m_pattern->m_type_symbol_y,
											m_pattern->m_type_symbol_y + m_pattern->m_type_symbol_height});
				}
                drawTerminalBody(painter, terminal_rect, used_zones_, !have_symbol_);
				if (colors_.stripe.isValid())
				{
					drawStripes(painter, terminal_rect, colors_.stripe);
					painter->setBrush(Qt::NoBrush);
					drawTerminalBody(painter, terminal_rect, used_zones_, !have_symbol_);	//Draw again the outline and the details, hidden by stripes
				}
				painter->restore();

					//Draw the symbol of the terminal type (fuse, ground, led...)
				if (m_pattern->m_type_symbol_height > 0 && real_terminal_vector[i])
				{
					painter->save();
					if (fill_color_.isValid()) {
						painter->setPen(text_color_);
					}
					drawTypeSymbol(painter,
								   QRectF{0, m_pattern->m_type_symbol_y,
										  terminal_rect.width(), m_pattern->m_type_symbol_height},
								   real_terminal_vector[i]->type(),
								   real_terminal_vector[i]->isLed());
					painter->restore();
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
				if (fill_color_.isValid()) {
					painter->setPen(text_color_);
				}
                fitTextToCell(painter, terminal_rect.width(), terminals_text_orientation == Qt::Vertical);
                text_rect.setRect(0, terminals_text_y, terminal_rect.width(), terminals_text_height);
                if (terminals_text_orientation == Qt::Vertical)
                {
                    painter->translate(text_rect.bottomLeft());
                    painter->rotate(270);
                    text_rect.setRect(0, 0, text_rect.height(), text_rect.width());
                }

                const auto shared_real_terminal{real_terminal_vector[i]};
				if (shared_real_terminal && terminal_rect.width() >= 4.0) {
					drawMarkerTag(painter, text_rect, terminalNumber(shared_real_terminal->label()),
								  terminals_text_option, terminals_text_orientation == Qt::Vertical,
								  shared_real_terminal->type() == ElementData::TTSectional,
								  tag_length_);
				}
				painter->drawText(text_rect,
								  shared_real_terminal ? terminalNumber(shared_real_terminal->label()) : QString(),
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
				if (fill_color_.isValid()) {
					painter->setPen(text_color_);
				}
				fitTextToCell(painter, terminal_rect.width(), xref_text_orientation == Qt::Vertical);
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
				if (!xref_string.isEmpty()) {
					m_xref_rects.append({mapped_xref_text_rect, physical_index, i});
				}
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

				//Separator plate after the physical terminal : a thick line
				//on the full height of the terminal, after its last level
			const bool separator_after_ = std::any_of(real_terminal_vector.cbegin(), real_terminal_vector.cend(),
													  [](const QSharedPointer<AbstractRealTerminalInterface> &real_t) {
				return real_t && real_t->hasSeparatorAfter();
			});
			if (separator_after_ && real_t_count > 0 && offset_ >= 0 && offset_ < 4)
			{
				const auto &rect_{m_pattern->m_terminal_rect.at(offset_)};
				painter->save();
				QPen separator_pen_{painter->pen()};
				separator_pen_.setColor(Qt::black);
				separator_pen_.setWidthF(3);
				separator_pen_.setCapStyle(Qt::FlatCap);
				painter->setPen(separator_pen_);
				painter->drawLine(QPointF{0, rect_.top() - 3}, QPointF{0, rect_.top() + rect_.height() + 3});
				painter->restore();
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
	if (m_pattern && m_draw_style != TerminalStripLayoutPattern::DrawStyle::Schematic) {
		return Realistic::boundingRect(m_strip.data(), *m_pattern);
	}

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

/**
 * @brief TerminalStripDrawer::setDrawStyle
 * Set how the terminals are drawn (schematic, realistic...).
 * The values used by each style come from the layout.
 */
void TerminalStripDrawer::setDrawStyle(TerminalStripLayoutPattern::DrawStyle style) {
	m_draw_style = style;
}

TerminalStripLayoutPattern::DrawStyle TerminalStripDrawer::drawStyle() const {
	return m_draw_style;
}

/**
 * @brief TerminalStripDrawer::xrefAt
 * @param pos : position in the coordinates of the item
 * @return the physical and real index of the terminal whose xref zone
 * (drawn by the last call of paint()) contains @a pos, or -1 if none.
 */
hoverTerminal TerminalStripDrawer::xrefAt(const QPointF &pos) const
{
	for (const auto &xref_rect : m_xref_rects) {
		if (xref_rect.rect.contains(pos)) {
			return hoverTerminal{xref_rect.physical, xref_rect.real};
		}
	}
	return hoverTerminal{};
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

/**
 * @brief TerminalStripDrawer::xrefRects
 * @return the rectangle of every xref text drawn by the last call
 * of paint(), in the coordinates of the painter.
 */
QVector<XrefRect> TerminalStripDrawer::xrefRects() const
{
	return m_xref_rects;
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

					width_ += terminalRect(*m_pattern, index_, real_terminal_vector[i]).width();
				}
			}
		}

		return width_;
	}

	return 0;
}

} //End namespace TerminalStripDrawer
