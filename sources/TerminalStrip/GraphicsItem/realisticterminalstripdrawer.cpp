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
#include "realisticterminalstripdrawer.h"
#include "terminalstripdrawerhelpers.h"
#include "../terminalcatalog.h"

#include <QFontMetricsF>
#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QPair>
#include <QScopedPointer>

#include <algorithm>

namespace TerminalStripDrawer {
namespace Realistic {

namespace {

	//Width (mm) of the accessories. Values of usual products, enough for a drawing.
constexpr qreal end_stop_width_mm{8.0};
constexpr qreal group_marker_width_mm{11.0};
constexpr qreal separator_width_mm{1.5};
	//Biggest radius of a wire entry (mm), for the wide terminals
constexpr qreal max_entry_radius_mm{1.9};

	//Vertical layout of a terminal, in fraction of its length (top = 0, bottom = 1).
	//The wire entries are in [entries_top, band_top] and [band_bottom, 1 - entries_top],
	//the middle band (marker tag, bridge channels) is in [band_top, band_bottom].
constexpr qreal entries_top{0.04};
constexpr qreal band_top{0.40};
constexpr qreal band_bottom{0.60};
constexpr qreal channels_top{0.405};
constexpr qreal channels_bottom{0.445};

	//Colors of the accessories, close to the real products
const QColor end_stop_color{232, 228, 220};
const QColor end_stop_grip_color{35, 35, 35};
const QColor group_marker_color{168, 168, 168};
const QColor group_marker_text_color{200, 0, 0};
const QColor band_color{85, 85, 85};
const QColor channel_color{55, 55, 55};
const QColor bridge_color{200, 30, 30};
const QColor separator_color{196, 196, 198};
const QColor fuse_holder_color{38, 38, 40};
const QColor knife_color{240, 120, 0};
const QColor led_color{220, 20, 20};

/**
 * @brief The PhysicalGeometry struct
 * What is needed to draw a physical terminal.
 */
struct PhysicalGeometry
{
	qreal width_mm{0};
	bool separator_after{false};	//A separator plate is mounted after this terminal
	QVector<QSharedPointer<AbstractRealTerminalInterface>> reals;
		//Index of the physical terminal in the strip, and of each real terminal
		//in the physical terminal : the same indexes than the schematic style,
		//used to find the element of a clicked terminal.
	int physical_index{-1};
	QVector<int> real_indexes;
};

/**
 * @return the physical terminals of @a strip with their width, in the drawing order.
 * The width of a physical terminal is the biggest width of its real terminals,
 * or the reference width if no width is known.
 */
QVector<PhysicalGeometry> geometry(const AbstractTerminalStripInterface *strip)
{
	QVector<PhysicalGeometry> geometry_;
	if (!strip) {
		return geometry_;
	}

	int physical_index_{-1};
	for (const auto &physical_t : strip->physicalTerminal())
	{
		++physical_index_;
		if (!physical_t) {
			continue;
		}
		PhysicalGeometry g_;
		g_.physical_index = physical_index_;
		int real_index_{-1};
		for (const auto &real_t : physical_t->realTerminals())
		{
			++real_index_;
			if (!real_t) {
				continue;
			}
			g_.reals.append(real_t);
			g_.real_indexes.append(real_index_);
			g_.width_mm = std::max(g_.width_mm, real_t->widthMm());
			g_.separator_after = g_.separator_after || real_t->hasSeparatorAfter();
			if (g_.reals.size() == 4) {	//Same limit than the schematic style
				break;
			}
		}
		if (g_.reals.isEmpty()) {
			continue;
		}
		if (g_.width_mm <= 0) {
			g_.width_mm = Helpers::referenceTerminalWidthMm();
		}
		geometry_.append(g_);
	}
	return geometry_;
}

qreal endStopWidthMm(const TerminalStripLayoutPattern &pattern) {
	if (!pattern.m_realistic_end_stops) {
		return 0;
	}
	return end_stop_width_mm;
}

qreal groupMarkerWidthMm(const TerminalStripLayoutPattern &pattern) {
	return pattern.m_realistic_group_marker ? group_marker_width_mm : 0;
}

/**
 * @brief fitFont
 * Reduce the pixel size of the font of @a painter so that @a text fit in
 * @a length (along the text) and @a thickness (across the text).
 * The font is never enlarged.
 */
void fitFont(QPainter *painter, const QString &text, qreal length, qreal thickness)
{
	auto font_{painter->font()};
	qreal size_{font_.pixelSize() > 0 ? qreal(font_.pixelSize()) : font_.pointSizeF()};
	const QFontMetricsF metrics_{font_};
	qreal factor_{1};
	if (metrics_.horizontalAdvance(text) > length && length > 0) {
		factor_ = std::min(factor_, length / metrics_.horizontalAdvance(text));
	}
	if (metrics_.height() > thickness && thickness > 0) {
		factor_ = std::min(factor_, thickness / metrics_.height());
	}
	if (factor_ < 1)
	{
		size_ = std::max<qreal>(2.0, size_ * factor_);
		if (font_.pixelSize() > 0) {
			font_.setPixelSize(std::max(1, static_cast<int>(size_)));
		} else {
			font_.setPointSizeF(size_);
		}
		painter->setFont(font_);
	}
}

/**
 * @brief drawVerticalText
 * Draw @a text from the bottom to the top, centered in @a rect.
 * The font is reduced if needed.
 */
void drawVerticalText(QPainter *painter, const QRectF &rect, const QString &text, const QColor &color)
{
	if (text.isEmpty() || rect.isEmpty()) {
		return;
	}
	painter->save();
	fitFont(painter, text, rect.height() - 2, rect.width() - 1);
	painter->setPen(color);
	painter->translate(rect.left(), rect.bottom());
	painter->rotate(270);
	painter->drawText(QRectF{0, 0, rect.height(), rect.width()},
					  Qt::AlignCenter, text);
	painter->restore();
}

/**
 * @brief drawGrip
 * Draw a black ribbed grip (end of an end stop).
 */
void drawGrip(QPainter *painter, const QRectF &rect)
{
	painter->save();
	painter->setBrush(end_stop_grip_color);
	painter->drawRect(rect);
	painter->setPen(QPen(QColor(150, 150, 150), 0));
	const qreal step_{std::max<qreal>(1.5, rect.height() / 5)};
	for (qreal y = rect.top() + step_ ; y < rect.bottom() - 0.5 ; y += step_) {
		painter->drawLine(QPointF{rect.left() + 1, y}, QPointF{rect.right() - 1, y});
	}
	painter->restore();
}

/**
 * @brief drawEndStop
 * Draw an end stop seen from above : a light body with a black grip at each end,
 * the fixing screw, the key slot and the locking part.
 * @param rect : the whole end stop
 */
void drawEndStop(QPainter *painter, const QRectF &rect)
{
	painter->save();
	painter->setBrush(end_stop_color);
	painter->drawRect(rect);

	const qreal w_{rect.width()};
	const qreal h_{rect.height()};
	const qreal cx_{rect.center().x()};
	const qreal grip_h_{h_ * 0.08};
	drawGrip(painter, QRectF{rect.left(), rect.top(), w_, grip_h_});
	drawGrip(painter, QRectF{rect.left(), rect.bottom() - grip_h_, w_, grip_h_});

		//Fixing screw
	const qreal screw_r_{std::min(w_ * 0.3, h_ * 0.05)};
	const QPointF screw_c_{cx_, rect.top() + h_ * 0.25};
	painter->setBrush(QColor(60, 60, 60));
	painter->drawEllipse(screw_c_, screw_r_, screw_r_);
	painter->setBrush(QColor(20, 20, 20));
	painter->drawEllipse(screw_c_, screw_r_ * 0.65, screw_r_ * 0.65);

		//Key slot : a narrow slot with a wider middle
	painter->setBrush(QColor(210, 205, 195));
	const qreal slot_w_{w_ * 0.22};
	painter->drawRoundedRect(QRectF{cx_ - slot_w_ / 2, rect.top() + h_ * 0.36, slot_w_, h_ * 0.26}, 1, 1);
	painter->drawRect(QRectF{cx_ - slot_w_, rect.top() + h_ * 0.45, slot_w_ * 2, h_ * 0.06});

		//Locking part
	painter->setBrush(QColor(20, 20, 20));
	const qreal lock_w_{w_ * 0.36};
	painter->drawRect(QRectF{cx_ - lock_w_ / 2, rect.top() + h_ * 0.68, lock_w_, h_ * 0.07});
	painter->drawEllipse(QPointF{cx_, rect.top() + h_ * 0.79}, lock_w_ * 0.35, lock_w_ * 0.35);

	painter->restore();
}

/**
 * @brief leverZone
 * Vertical zone (in fraction of the length of the terminal) of the lever of a
 * fuse terminal or of the knife of a disconnect terminal, centered on the middle
 * band. For a terminal with one level, the lever goes from a wire entry to the other
 * (a bit shorter with screw clamps, whose screw heads are nearer the middle),
 * for a terminal with levels it stay on the middle band, between the wire entries.
 */
QPair<qreal, qreal> leverZone(int levels, bool screw)
{
	if (levels > 1) {
		return qMakePair(qreal(0.395), qreal(0.605));
	}
	return screw ? qMakePair(qreal(0.345), qreal(0.655))
				 : qMakePair(qreal(0.315), qreal(0.685));
}

/**
 * @brief drawFuseHolder
 * Draw the hinged fuse holder of a fuse terminal, seen from above : a black lever
 * on the middle of the terminal with its hinge pin, a window on the fuse,
 * a ribbed grip at the other end and, for a fuse with led, the led.
 * The marker tag and the symbol are drawn after, on the lever.
 * @param x : left of the terminal
 * @param w : width of the terminal
 * @param length : length of the terminal
 */
void drawFuseHolder(QPainter *painter, qreal x, qreal w, qreal length, int levels, bool screw, bool led)
{
	const auto zone_{leverZone(levels, screw)};
	const qreal top_{length * zone_.first};
	const qreal bottom_{length * zone_.second};
	const qreal inset_{w * 0.08};
	const QRectF lever_{x + inset_, top_, w - 2 * inset_, bottom_ - top_};
	const qreal cx_{x + w / 2};

	painter->save();
	painter->setPen(QPen(Qt::black, 0.5));
	painter->setBrush(fuse_holder_color);
	painter->drawRoundedRect(lever_, w * 0.12, w * 0.12);

		//Hinge pin
	const qreal pin_r_{w * 0.12};
	painter->setPen(QPen(QColor(90, 90, 90), 0.4));
	painter->setBrush(QColor(150, 150, 150));
	painter->drawEllipse(QPointF{cx_, top_ + pin_r_ * 2}, pin_r_, pin_r_);

		//Window on the fuse, under the hinge pin and above the marker tag,
		//only when there is enough room
	const qreal window_top_{std::max(length * 0.38, top_ + pin_r_ * 3.4)};
	const qreal window_bottom_{length * 0.445};
	if (levels == 1 && window_bottom_ - window_top_ > length * 0.03)
	{
		painter->setPen(QPen(QColor(110, 110, 110), 0.4));
		painter->setBrush(QColor(205, 205, 200));
		painter->drawRoundedRect(QRectF{cx_ - w * 0.25, window_top_, w * 0.5, window_bottom_ - window_top_}, 1, 1);
			//The fuse in the window
		painter->setPen(QPen(QColor(120, 90, 40), std::max<qreal>(0.5, w * 0.08)));
		const qreal margin_{(window_bottom_ - window_top_) * 0.1};
		painter->drawLine(QPointF{cx_, window_top_ + margin_}, QPointF{cx_, window_bottom_ - margin_});
	}

		//Ribbed grip at the other end
	painter->setPen(QPen(QColor(110, 110, 110), 0.4));
	const qreal grip_top_{bottom_ - (bottom_ - top_) * 0.13};
	for (int i = 0 ; i < 3 ; ++i) {
		const qreal y_{grip_top_ + i * (bottom_ - grip_top_) / 3};
		painter->drawLine(QPointF{lever_.left() + w * 0.12, y_}, QPointF{lever_.right() - w * 0.12, y_});
	}

		//Led : lit when the fuse is blown. Between the type symbol
		//(bottom of the middle band) and the grip, whatever the length of the lever.
	const qreal symbol_bottom_{length * 0.595};
	if (led && grip_top_ > symbol_bottom_)
	{
		const qreal led_r_{std::min({w * 0.13, length * 0.012, (grip_top_ - symbol_bottom_) * 0.45})};
		painter->setPen(QPen(led_color.darker(160), 0.4));
		painter->setBrush(led_color);
		painter->drawEllipse(QPointF{cx_, (symbol_bottom_ + grip_top_) / 2}, led_r_, led_r_);
	}
	painter->restore();
}

/**
 * @brief drawDisconnectKnife
 * Draw the orange knife of a disconnect terminal, seen from above :
 * a narrow lever on the middle of the terminal with its hinge and its grip.
 */
void drawDisconnectKnife(QPainter *painter, qreal x, qreal w, qreal length, int levels, bool screw)
{
	const auto zone_{leverZone(levels, screw)};
	const qreal top_{length * zone_.first};
	const qreal bottom_{length * zone_.second};
	const qreal cx_{x + w / 2};
	const qreal knife_w_{w * 0.56};

	painter->save();
	painter->setPen(QPen(knife_color.darker(170), 0.5));
	painter->setBrush(knife_color);
	painter->drawRoundedRect(QRectF{cx_ - knife_w_ / 2, top_, knife_w_, bottom_ - top_},
							 knife_w_ * 0.3, knife_w_ * 0.3);

		//Hinge
	const qreal pin_r_{knife_w_ * 0.22};
	painter->setBrush(knife_color.darker(150));
	painter->drawEllipse(QPointF{cx_, top_ + pin_r_ * 2}, pin_r_, pin_r_);

		//Grip
	painter->setPen(QPen(knife_color.darker(190), 0.5));
	const qreal grip_top_{bottom_ - (bottom_ - top_) * 0.12};
	for (int i = 0 ; i < 3 ; ++i) {
		const qreal y_{grip_top_ + i * (bottom_ - grip_top_) / 3};
		painter->drawLine(QPointF{cx_ - knife_w_ * 0.3, y_}, QPointF{cx_ + knife_w_ * 0.3, y_});
	}
	painter->restore();
}

/**
 * @brief drawTestSockets
 * Draw the two test sockets of a test terminal : a brass ring with a black hole.
 * For a terminal with one level they are between the wire entries and the middle band,
 * for a terminal with levels (no room between the entries) they are in the middle band,
 * in place of the bridge channels and of the type symbol.
 */
void drawTestSockets(QPainter *painter, qreal x, qreal w, qreal length, qreal s, int levels, bool screw, const QColor &fill)
{
	const qreal r_{std::min({w * 0.22, 1.2 * s, length * 0.018})};
	const qreal offset_{levels > 1 ? 0.425 : (screw ? 0.37 : 0.355)};
	painter->save();
	for (const qreal f_ : {offset_, 1 - offset_})
	{
		const QPointF c_{x + w / 2, length * f_};
		painter->setPen(QPen(fill.darker(200), 0.4));
		painter->setBrush(QColor(200, 170, 60));	//Brass
		painter->drawEllipse(c_, r_, r_);
		painter->setPen(Qt::NoPen);
		painter->setBrush(Qt::black);
		painter->drawEllipse(c_, r_ * 0.55, r_ * 0.55);
	}
	painter->restore();
}

/**
 * @brief drawSeparator
 * Draw a separator plate (partition plate) seen from above :
 * a thin plate as long as the terminals, with rounded ends and a light edge.
 */
void drawSeparator(QPainter *painter, const QRectF &rect)
{
	painter->save();
	painter->setPen(QPen(separator_color.darker(170), 0.5));
	painter->setBrush(separator_color);
	const qreal r_{std::min(rect.width() / 2, qreal(2.0))};
	painter->drawRoundedRect(rect, r_, r_);
		//Light edge, the plate is a bit higher than the terminals
	painter->setPen(QPen(separator_color.lighter(118), std::max<qreal>(0.5, rect.width() * 0.25)));
	painter->drawLine(QPointF{rect.left() + rect.width() * 0.35, rect.top() + r_},
					  QPointF{rect.left() + rect.width() * 0.35, rect.bottom() - r_});
	painter->restore();
}

/**
 * @brief drawGroupMarker
 * Draw the group marker carrier with the name of the strip on a white label.
 */
void drawGroupMarker(QPainter *painter, const QRectF &rect, const QString &text)
{
	painter->save();
	painter->setBrush(group_marker_color);
	painter->drawRect(rect);

	if (!text.isEmpty())
	{
			//White label : as long as the text, centered on the carrier
		const qreal label_w_{rect.width() * 0.6};
		fitFont(painter, text, rect.height() * 0.8, label_w_ - 2);
		const qreal text_len_{QFontMetricsF{painter->font()}.horizontalAdvance(text)};
		const qreal label_h_{std::min(rect.height() * 0.85, text_len_ + label_w_ * 0.6)};
		const QRectF label_{rect.center().x() - label_w_ / 2,
							rect.center().y() - label_h_ / 2,
							label_w_, label_h_};
		painter->setBrush(Qt::white);
		painter->drawRect(label_);
		drawVerticalText(painter, label_, text, group_marker_text_color);
	}
	painter->restore();
}

/**
 * @brief drawWireEntry
 * Draw one wire entry of a terminal.
 * Push-in : a black hole.
 * Screw : the head of the clamping screw, with the hole of the wire next to it.
 * @param center : center of the entry
 * @param width : width of the terminal
 * @param row_h : height available for the entry
 * @param max_r : biggest radius of the entry (a wide terminal don't have a huge hole)
 * @param outer : -1 if the end of the terminal is above the entry (top half), +1 if it is under (bottom half)
 * @param screw : true for a screw clamp
 */
void drawWireEntry(QPainter *painter, const QPointF &center, qreal width, qreal row_h,
				   qreal max_r, qreal outer, const QColor &fill, bool screw)
{
	painter->save();
	if (!screw)
	{
		const qreal r_{std::min({width * 0.32, row_h * 0.3, max_r})};
		painter->setBrush(Qt::black);
		painter->drawEllipse(center, r_, r_);
	}
	else
	{
			//Hole of the wire, toward the end of the terminal
		const qreal hole_{std::min({width * 0.45, row_h * 0.22, max_r * 1.6})};
		const qreal hole_y_{outer < 0 ? center.y() - row_h * 0.38 : center.y() + row_h * 0.38 - hole_};
		painter->setBrush(Qt::black);
		painter->drawRect(QRectF{center.x() - hole_ / 2, hole_y_, hole_, hole_});
			//Screw head, toward the middle of the terminal
		const qreal r_{std::min({width * 0.36, row_h * 0.26, max_r * 1.1})};
		const QPointF c_{center.x(), center.y() - outer * row_h * 0.08};
		painter->setBrush(fill.darker(125));
		painter->drawEllipse(c_, r_ * 1.15, r_ * 1.15);
		painter->setBrush(QColor(175, 175, 175));
		painter->drawEllipse(c_, r_, r_);
		painter->setPen(QPen(Qt::black, std::max<qreal>(0.6, r_ * 0.25)));
		painter->drawLine(QPointF{c_.x() - r_ * 0.7, c_.y()}, QPointF{c_.x() + r_ * 0.7, c_.y()});
	}
	painter->restore();
}

/**
 * @brief drawEntriesHalf
 * Draw the rows of wire entries of one half of a terminal (top or bottom).
 * One row by level. Each row have a few ribs at its outer side and a line
 * at its inner side, like the real terminals.
 * @param outer : y of the outer side of the half (end of the terminal)
 * @param inner : y of the inner side of the half (middle band). For the bottom half outer > inner.
 * @param max_r : biggest radius of a wire entry
 */
void drawEntriesHalf(QPainter *painter, const QRectF &body, qreal outer, qreal inner,
					 int levels, qreal max_r, const QColor &fill, bool screw)
{
	const qreal row_h_{(inner - outer) / levels};	//Negative for the bottom half
	const qreal abs_row_h_{std::abs(row_h_)};
	const QPen rib_pen_{fill.darker(170), 0};

	for (int k = 0 ; k < levels ; ++k)
	{
		const qreal a_{outer + k * row_h_};
			//Ribs
		painter->save();
		painter->setPen(rib_pen_);
		for (const qreal f_ : {0.12, 0.22, 0.32}) {
			const qreal y_{a_ + f_ * row_h_};
			painter->drawLine(QPointF{body.left(), y_}, QPointF{body.right(), y_});
		}
			//Limit of the row
		if (k > 0) {
			painter->drawLine(QPointF{body.left(), a_}, QPointF{body.right(), a_});
		}
		painter->restore();

		drawWireEntry(painter, QPointF{body.center().x(), a_ + 0.67 * row_h_},
					  body.width(), abs_row_h_, max_r, row_h_ > 0 ? -1 : 1, fill, screw);
	}
}

/**
 * @brief The BridgePoint struct
 * Where a bridge is plugged.
 */
struct BridgePoint
{
	QPointF point;
	qreal width{0};
};

} //End anonymous namespace

QRectF boundingRect(const AbstractTerminalStripInterface *strip,
					const TerminalStripLayoutPattern &pattern)
{
	qreal width_mm_{2 * endStopWidthMm(pattern) + groupMarkerWidthMm(pattern)};
	for (const auto &g_ : geometry(strip)) {
		width_mm_ += g_.width_mm;
		if (g_.separator_after) {
			width_mm_ += separator_width_mm;
		}
	}
	const qreal s_{pattern.m_realistic_px_per_mm};
	return QRectF{0, 0, width_mm_ * s_, pattern.m_realistic_length_mm * s_};
}

void paint(QPainter *painter,
		   const AbstractTerminalStripInterface &strip,
		   const TerminalStripLayoutPattern &pattern,
		   TerminalStripLayoutPattern::DrawStyle style,
		   bool preview_draw,
		   const QPointF &hover_pos,
		   QVector<XrefRect> *xref_rects)
{
		//Clamp of the layout, used for the terminals not chosen in the catalog
	const bool layout_screw_{style == TerminalStripLayoutPattern::DrawStyle::RealisticScrew};
	const qreal s_{pattern.m_realistic_px_per_mm};
	const qreal length_{pattern.m_realistic_length_mm * s_};
	if (s_ <= 0 || length_ <= 0) {
		return;
	}

	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setFont(pattern.font());
	painter->setPen(QPen(Qt::black, 0.5));

	if (preview_draw)
	{
		painter->save();
		painter->setPen(Qt::blue);
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(boundingRect(&strip, pattern));
		painter->restore();
	}

	qreal x_{0};

		//The accessories are a bit shorter than the terminals, centered on them
	const qreal end_stop_w_{endStopWidthMm(pattern) * s_};
	const qreal end_stop_h_{length_ * 0.88};
	const qreal end_stop_y_{(length_ - end_stop_h_) / 2};
	auto draw_end_stop = [&]()
	{
		if (end_stop_w_ <= 0) {
			return;
		}
		const QRectF rect_{x_, end_stop_y_, end_stop_w_, end_stop_h_};
		drawEndStop(painter, rect_);
		x_ += end_stop_w_;
	};

	draw_end_stop();

	if (pattern.m_realistic_group_marker)
	{
		const qreal w_{group_marker_width_mm * s_};
		const qreal h_{length_ * 0.72};
		drawGroupMarker(painter, QRectF{x_, (length_ - h_) / 2, w_, h_}, strip.name());
		x_ += w_;
	}

	QHash<QUuid, QVector<BridgePoint>> bridges_;

	for (const auto &g_ : geometry(&strip))
	{
		const qreal w_{g_.width_mm * s_};
		const QRectF body_{x_, 0, w_, length_};
		const auto first_{g_.reals.first()};
		const int levels_{static_cast<int>(g_.reals.size())};

			//A terminal chosen in the catalog is drawn with its own clamp, so that a strip
			//mixing screw and spring terminals is drawn as it is. The spring-cage clamps
			//(opened with a screwdriver) are drawn like the push-in ones.
		const auto *catalog_entry_{TerminalCatalog::entryByLabel(first_->catalogEntry())};
		const bool screw_{catalog_entry_ ? catalog_entry_->clamp == TerminalCatalog::Clamp::Screw
										 : layout_screw_};

		QColor fill_, stripe_;
		Helpers::terminalColors(first_, fill_, stripe_);
		if (!fill_.isValid()) {
			fill_ = QColor(150, 150, 150);
		}

			//Body
		painter->save();
		painter->setPen(QPen(fill_.darker(220), 0.5));
		painter->setBrush(fill_);
		painter->drawRect(body_);
		if (stripe_.isValid())
		{
				//Two-tone terminal (PE) : the stripe color on the left part, like the real ones
			painter->setPen(Qt::NoPen);
			painter->setBrush(stripe_);
			painter->drawRect(QRectF{body_.left(), body_.top(), w_ * 0.6, length_});
		}
		painter->restore();

			//Wire entries : one row by level, at the top and at the bottom
		const qreal max_r_{max_entry_radius_mm * s_};
		drawEntriesHalf(painter, body_, length_ * entries_top, length_ * band_top, levels_, max_r_, fill_, screw_);
		drawEntriesHalf(painter, body_, length_ * (1 - entries_top), length_ * band_bottom, levels_, max_r_, fill_, screw_);

			//Middle band, with one bridge channel by level
		painter->save();
		painter->setPen(Qt::NoPen);
		painter->setBrush(band_color);
		painter->drawRect(QRectF{x_, length_ * band_top, w_, length_ * (band_bottom - band_top)});
		painter->setBrush(channel_color);
		const qreal channel_h_{length_ * (channels_bottom - channels_top) / levels_};
		for (int k = 0 ; k < levels_ ; ++k) {
			painter->drawRect(QRectF{x_ + w_ * 0.15, length_ * channels_top + (k + 0.2) * channel_h_,
									 w_ * 0.7, channel_h_ * 0.6});
		}
		painter->restore();

			//Specific parts of some types of terminal
		const auto type_{first_->type()};
		switch (type_)
		{
			case ElementData::TTFuse:
				drawFuseHolder(painter, x_, w_, length_, levels_, screw_, first_->isLed());
				break;
			case ElementData::TTSectional:
				drawDisconnectKnife(painter, x_, w_, length_, levels_, screw_);
				break;
			case ElementData::TTTestSocket:
				drawTestSockets(painter, x_, w_, length_, s_, levels_, screw_, fill_);
				break;
			default:
				break;
		}

			//Symbol of the type (fuse, led...), at the bottom of the band
			//A test terminal with levels have its test sockets in place of the symbol
		const bool have_symbol_{(first_->type() != ElementData::TTGeneric || first_->isLed())
								&& !(type_ == ElementData::TTTestSocket && levels_ > 1)};
		const qreal tag_top_{length_ * 0.455};
		const bool sockets_in_band_{type_ == ElementData::TTTestSocket && levels_ > 1};
		const qreal tag_bottom_{length_ * (have_symbol_ || sockets_in_band_ ? 0.535 : 0.59)};
		if (have_symbol_)
		{
			painter->save();
				//Black on the orange knife, white on the dark band and on the fuse holder
			painter->setPen(QPen(type_ == ElementData::TTSectional ? QColor(Qt::black) : QColor(Qt::white), 0.5));
			Helpers::drawTypeSymbol(painter, QRectF{x_, length_ * 0.54, w_, length_ * 0.055},
									first_->type(), first_->isLed());
			painter->restore();
		}

			//Marker tag with the number(s) of the terminal
		QStringList numbers_;
		for (const auto &real_t : g_.reals) {
			numbers_ << Helpers::terminalNumber(real_t->label());
		}
		const bool sectional_{first_->type() == ElementData::TTSectional};
		const QRectF tag_{x_ + std::min(0.6 * s_, w_ * 0.12), tag_top_,
						  w_ - 2 * std::min(0.6 * s_, w_ * 0.12), tag_bottom_ - tag_top_};
		painter->save();
		painter->setPen(QPen(QColor(120, 120, 120), 0.3));
		painter->setBrush(sectional_ ? QColor(Qt::black) : QColor(Qt::white));
		painter->drawRect(tag_);
		painter->restore();
		drawVerticalText(painter, tag_, numbers_.join(QStringLiteral(" / ")),
						 sectional_ ? QColor(Qt::white) : QColor(Qt::black));

			//Clickable zones : the whole terminal for a terminal with one level,
			//the rows of wire entries of the level (top and bottom) for a terminal with levels.
			//Only the terminals with an xref (element placed on a folio) are clickable.
		QVector<XrefRect> terminal_rects_;
		for (int k = 0 ; k < levels_ ; ++k)
		{
			if (g_.reals.at(k)->xref().isEmpty()) {
				continue;
			}
			const int physical_{g_.physical_index};
			const int real_{g_.real_indexes.at(k)};
			if (levels_ == 1) {
				terminal_rects_.append({body_, physical_, real_});
			} else {
				const qreal top_h_{length_ * (band_top - entries_top) / levels_};
				terminal_rects_.append({QRectF{x_, length_ * entries_top + k * top_h_, w_, top_h_}, physical_, real_});
				terminal_rects_.append({QRectF{x_, length_ * (1 - entries_top) - (k + 1) * top_h_, w_, top_h_}, physical_, real_});
			}
		}
		if (xref_rects) {
			*xref_rects << terminal_rects_;
		}

			//Highlight the terminal under the mouse, like the xref of the schematic style
		if (!hover_pos.isNull())
		{
			int hovered_real_{-1};
			for (const auto &r_ : std::as_const(terminal_rects_)) {
				if (r_.rect.contains(hover_pos)) {
					hovered_real_ = r_.real;
					break;
				}
			}
			if (hovered_real_ >= 0)
			{
				painter->save();
				painter->setPen(QPen(QColor(0, 90, 255), std::max<qreal>(1.0, 0.4 * s_)));
				painter->setBrush(Qt::NoBrush);
				for (const auto &r_ : std::as_const(terminal_rects_)) {
					if (r_.real == hovered_real_) {
						painter->drawRect(r_.rect.adjusted(0.5, 0.5, -0.5, -0.5));
					}
				}
				painter->restore();
			}
		}

			//Bridge points : one channel by level
		for (int k = 0 ; k < levels_ ; ++k)
		{
			const auto &real_t{g_.reals.at(k)};
			if (!real_t->isBridged()) {
				continue;
			}
			QScopedPointer<AbstractBridgeInterface> bridge_{real_t->bridge()};
			if (!bridge_) {
				continue;
			}
			const qreal channel_h_{length_ * (channels_bottom - channels_top) / levels_};
			const qreal y_{length_ * channels_top + (k + 0.5) * channel_h_};
			bridges_[bridge_->uuid()].append(BridgePoint{QPointF{x_ + w_ / 2, y_}, w_});
		}

		x_ += w_;

			//Separator plate after the terminal
		if (g_.separator_after)
		{
			const qreal sep_w_{separator_width_mm * s_};
			drawSeparator(painter, QRectF{x_, 0, sep_w_, length_});
			x_ += sep_w_;
		}
	}

		//Bridges : a red comb plugged in the channels
	for (const auto &points_ : std::as_const(bridges_))
	{
		if (points_.size() < 2) {
			continue;
		}
		painter->save();
		const qreal thickness_{std::max<qreal>(1.0, 0.9 * s_)};
		painter->setPen(QPen(bridge_color, thickness_, Qt::SolidLine, Qt::FlatCap));
		painter->drawLine(points_.first().point, points_.last().point);
		painter->setPen(Qt::NoPen);
		painter->setBrush(bridge_color.darker(140));
		for (const auto &p_ : points_) {
			const qreal r_{std::min(p_.width * 0.18, thickness_ * 0.8)};
			painter->drawEllipse(p_.point, r_, r_);
		}
		painter->restore();
	}

	draw_end_stop();

	painter->restore();
}

} //End namespace Realistic
} //End namespace TerminalStripDrawer
