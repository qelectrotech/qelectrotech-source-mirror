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
#include "dimensionitem.h"
#include "../diagram.h"
#include "../ui/dimensionpropertiesdialog.h"

#include <cmath>
#include <QApplication>
#include <QCursor>
#include <QDomDocument>
#include <QDomElement>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>

DimensionItem::DimensionItem(DimensionMode mode, QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	m_p1(0, 0),
	m_p2(100, 0),
	m_offset(20.0),
	m_mode(mode),
	m_uuid(QUuid::createUuid()),
	m_pen(QColor(0, 110, 210), 1.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin),
	m_font(QStringLiteral("Arial"), 8)
{
	setFlags(ItemIsSelectable | ItemIsMovable);
	setAcceptHoverEvents(true);
}

DimensionItem::DimensionItem(const QPointF &p1, const QPointF &p2, qreal offset,
                             DimensionMode mode, QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	m_p1(p1),
	m_p2(p2),
	m_offset(offset),
	m_mode(mode),
	m_uuid(QUuid::createUuid()),
	m_pen(QColor(0, 110, 210), 1.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin),
	m_font(QStringLiteral("Arial"), 8)
{
	setFlags(ItemIsSelectable | ItemIsMovable);
	setAcceptHoverEvents(true);
}

DimensionItem::~DimensionItem()
{
	clearHandles();
}

void DimensionItem::editProperty()
{
	if (!diagram() || diagram()->isReadOnly()) return;
	DimensionPropertiesDialog dialog(this, QApplication::activeWindow());
	dialog.exec();
}

QPointF DimensionItem::p1() const
{
	return m_p1;
}

void DimensionItem::setP1(const QPointF &p1)
{
	if (m_p1 == p1) return;
	prepareGeometryChange();
	m_p1 = p1;
	emit geometryChanged();
	update();
	updateHandlePositions();
}

QPointF DimensionItem::p2() const
{
	return m_p2;
}

void DimensionItem::setP2(const QPointF &p2)
{
	if (m_p2 == p2) return;
	prepareGeometryChange();
	m_p2 = p2;
	emit geometryChanged();
	update();
	updateHandlePositions();
}

qreal DimensionItem::offset() const
{
	return m_offset;
}

void DimensionItem::setOffset(qreal offset)
{
	if (qFuzzyCompare(m_offset, offset)) return;
	prepareGeometryChange();
	m_offset = offset;
	emit geometryChanged();
	update();
	updateHandlePositions();
}

bool DimensionItem::isAutoValue() const
{
	return m_auto_value;
}

void DimensionItem::setAutoValue(bool auto_val)
{
	if (m_auto_value == auto_val) return;
	prepareGeometryChange();
	m_auto_value = auto_val;
	emit propertiesChanged();
	update();
}

QString DimensionItem::customText() const
{
	return m_custom_text;
}

void DimensionItem::setCustomText(const QString &text)
{
	if (m_custom_text == text) return;
	prepareGeometryChange();
	m_custom_text = text;
	emit propertiesChanged();
	update();
}

QString DimensionItem::prefix() const
{
	return m_prefix;
}

void DimensionItem::setPrefix(const QString &p)
{
	if (m_prefix == p) return;
	prepareGeometryChange();
	m_prefix = p;
	emit propertiesChanged();
	update();
}

QString DimensionItem::suffix() const
{
	return m_suffix;
}

void DimensionItem::setSuffix(const QString &s)
{
	if (m_suffix == s) return;
	prepareGeometryChange();
	m_suffix = s;
	emit propertiesChanged();
	update();
}

int DimensionItem::precision() const
{
	return m_precision;
}

void DimensionItem::setPrecision(int p)
{
	if (m_precision == p) return;
	prepareGeometryChange();
	m_precision = p;
	emit propertiesChanged();
	update();
}

QString DimensionItem::unit() const
{
	return m_unit;
}

void DimensionItem::setUnit(const QString &u)
{
	if (m_unit == u) return;
	prepareGeometryChange();
	m_unit = u;
	emit propertiesChanged();
	update();
}

bool DimensionItem::showUnit() const
{
	return m_show_unit;
}

void DimensionItem::setShowUnit(bool s)
{
	if (m_show_unit == s) return;
	prepareGeometryChange();
	m_show_unit = s;
	emit propertiesChanged();
	update();
}

bool DimensionItem::isFramed() const
{
	return m_framed;
}

void DimensionItem::setFramed(bool f)
{
	if (m_framed == f) return;
	prepareGeometryChange();
	m_framed = f;
	emit propertiesChanged();
	update();
}

bool DimensionItem::useComma() const
{
	return m_use_comma;
}

void DimensionItem::setUseComma(bool c)
{
	if (m_use_comma == c) return;
	prepareGeometryChange();
	m_use_comma = c;
	emit propertiesChanged();
	update();
}

qreal DimensionItem::textDistance() const
{
	return m_text_distance;
}

void DimensionItem::setTextDistance(qreal d)
{
	if (qFuzzyCompare(m_text_distance, d)) return;
	prepareGeometryChange();
	m_text_distance = d;
	emit propertiesChanged();
	update();
}

bool DimensionItem::isTextCenter() const
{
	return m_text_center;
}

void DimensionItem::setTextCenter(bool c)
{
	if (m_text_center == c) return;
	prepareGeometryChange();
	m_text_center = c;
	emit propertiesChanged();
	update();
}

bool DimensionItem::showDualUnit() const
{
	return m_show_dual_unit;
}

void DimensionItem::setShowDualUnit(bool d)
{
	if (m_show_dual_unit == d) return;
	prepareGeometryChange();
	m_show_dual_unit = d;
	emit propertiesChanged();
	update();
}

bool DimensionItem::isTextHorizontal() const
{
	return m_text_horizontal;
}

void DimensionItem::setTextHorizontal(bool h)
{
	if (m_text_horizontal == h) return;
	prepareGeometryChange();
	m_text_horizontal = h;
	emit propertiesChanged();
	update();
}

bool DimensionItem::isTextInLine() const
{
	return m_text_in_line;
}

void DimensionItem::setTextInLine(bool il)
{
	if (m_text_in_line == il) return;
	prepareGeometryChange();
	m_text_in_line = il;
	emit propertiesChanged();
	update();
}

bool DimensionItem::suppressZero() const
{
	return m_suppress_zero;
}

void DimensionItem::setSuppressZero(bool sz)
{
	if (m_suppress_zero == sz) return;
	prepareGeometryChange();
	m_suppress_zero = sz;
	emit propertiesChanged();
	update();
}

qreal DimensionItem::lineWidth() const
{
	return m_line_width;
}

void DimensionItem::setLineWidth(qreal w)
{
	if (qFuzzyCompare(m_line_width, w)) return;
	prepareGeometryChange();
	m_line_width = w;
	m_pen.setWidthF(m_line_width);
	emit propertiesChanged();
	update();
}

qreal DimensionItem::extensionOvershoot() const
{
	return m_extension_overshoot;
}

void DimensionItem::setExtensionOvershoot(qreal o)
{
	if (qFuzzyCompare(m_extension_overshoot, o)) return;
	prepareGeometryChange();
	m_extension_overshoot = o;
	emit propertiesChanged();
	update();
}

qreal DimensionItem::extensionGap() const
{
	return m_extension_gap;
}

void DimensionItem::setExtensionGap(qreal g)
{
	if (qFuzzyCompare(m_extension_gap, g)) return;
	prepareGeometryChange();
	m_extension_gap = g;
	emit propertiesChanged();
	update();
}

qreal DimensionItem::arrowSize() const
{
	return m_arrow_size;
}

void DimensionItem::setArrowSize(qreal s)
{
	if (qFuzzyCompare(m_arrow_size, s)) return;
	prepareGeometryChange();
	m_arrow_size = s;
	emit propertiesChanged();
	update();
}

bool DimensionItem::maskBackground() const
{
	return m_mask_background;
}

void DimensionItem::setMaskBackground(bool m)
{
	if (m_mask_background == m) return;
	m_mask_background = m;
	emit propertiesChanged();
	update();
}

int DimensionItem::fontSize() const
{
	return m_font.pointSize() > 0 ? m_font.pointSize() : 8;
}

void DimensionItem::setFontSize(int s)
{
	if (m_font.pointSize() == s) return;
	prepareGeometryChange();
	m_font.setPointSize(s > 0 ? s : 8);
	emit propertiesChanged();
	update();
}

DimensionItem::DimensionMode DimensionItem::mode() const
{
	return m_mode;
}

void DimensionItem::setMode(DimensionMode mode)
{
	if (m_mode == mode) return;
	m_mode = mode;
	emit propertiesChanged();
	update();
}

DimensionItem::TerminationStyle DimensionItem::terminationStyle() const
{
	return m_termination_style;
}

void DimensionItem::setTerminationStyle(TerminationStyle style)
{
	if (m_termination_style == style) return;
	m_termination_style = style;
	emit propertiesChanged();
	update();
}

DimensionProperties DimensionItem::properties() const
{
	DimensionProperties p;
	p.isAutoValue = m_auto_value;
	p.customText = m_custom_text;
	p.prefix = m_prefix;
	p.suffix = m_suffix;
	p.precision = m_precision;
	p.unit = m_unit;
	p.showUnit = m_show_unit;
	p.isFramed = m_framed;
	p.useComma = m_use_comma;
	p.textDistance = m_text_distance;
	p.textCenter = m_text_center;
	p.showDualUnit = m_show_dual_unit;
	p.suppressZero = m_suppress_zero;
	p.isTextHorizontal = m_text_horizontal;
	p.isTextInLine = m_text_in_line;
	p.lineWidth = m_line_width;
	p.extensionOvershoot = m_extension_overshoot;
	p.extensionGap = m_extension_gap;
	p.offset = m_offset;
	p.terminationStyle = static_cast<int>(m_termination_style);
	p.arrowSize = m_arrow_size;
	p.maskBackground = m_mask_background;
	p.fontSize = fontSize();
	return p;
}

void DimensionItem::setProperties(const DimensionProperties &p)
{
	prepareGeometryChange();
	m_auto_value = p.isAutoValue;
	m_custom_text = p.customText;
	m_prefix = p.prefix;
	m_suffix = p.suffix;
	m_precision = p.precision;
	m_unit = p.unit;
	m_show_unit = p.showUnit;
	m_framed = p.isFramed;
	m_use_comma = p.useComma;
	m_text_distance = p.textDistance;
	m_text_center = p.textCenter;
	m_show_dual_unit = p.showDualUnit;
	m_suppress_zero = p.suppressZero;
	m_text_horizontal = p.isTextHorizontal;
	m_text_in_line = p.isTextInLine;
	m_line_width = p.lineWidth;
	m_pen.setWidthF(m_line_width);
	m_extension_overshoot = p.extensionOvershoot;
	m_extension_gap = p.extensionGap;
	m_offset = p.offset;
	m_termination_style = static_cast<TerminationStyle>(p.terminationStyle);
	m_arrow_size = p.arrowSize;
	m_mask_background = p.maskBackground;
	m_font.setPointSize(p.fontSize > 0 ? p.fontSize : 8);
	emit propertiesChanged();
	emit geometryChanged();
	update();
}

qreal DimensionItem::dimensionValueMM() const
{
	qreal dx = m_p2.x() - m_p1.x();
	qreal dy = m_p2.y() - m_p1.y();
	qreal dist_px = std::hypot(dx, dy);
	return dist_px / 2.0; // Strictly 2 px = 1.0 mm
}

QString DimensionItem::displayText() const
{
	QString core;
	if (m_auto_value) {
		core = QString::number(dimensionValueMM(), 'f', m_precision);
		if (m_use_comma) {
			core.replace(QLatin1Char('.'), QLatin1Char(','));
		}
		if (m_suppress_zero && m_precision > 0) {
			QString zeroEnding = m_use_comma ? QStringLiteral(",0") : QStringLiteral(".0");
			if (core.endsWith(zeroEnding)) {
				core.chop(2);
			}
		}
	} else {
		core = m_custom_text;
	}

	QString result = m_prefix;
	result += core;
	if (m_show_unit && !m_unit.isEmpty()) {
		result += QLatin1Char(' ') + m_unit;
	}
	if (m_show_dual_unit) {
		result += QStringLiteral(" [%1 in]").arg(QString::number(dimensionValueMM() / 25.4, 'f', 2));
	}
	result += m_suffix;
	return result;
}

QList<DimensionItem *> DimensionItem::chainSegments() const
{
	if (m_mode != Chain || !diagram()) {
		return { const_cast<DimensionItem *>(this) };
	}

	QList<DimensionItem *> chain;
	if (!m_chain_id.isNull()) {
		for (QGraphicsItem *qgi : diagram()->items()) {
			if (qgi->type() == DimensionItem::Type) {
				auto *it = static_cast<DimensionItem *>(qgi);
				if (it->mode() == Chain && it->chainId() == m_chain_id) {
					chain.append(it);
				}
			}
		}
	}

	if (chain.isEmpty()) {
		QList<DimensionItem *> candidates;
		for (QGraphicsItem *qgi : diagram()->items()) {
			if (qgi->type() == DimensionItem::Type) {
				auto *it = static_cast<DimensionItem *>(qgi);
				if (it->mode() == Chain && std::abs(it->offset() - m_offset) < 2.0) {
					candidates.append(it);
				}
			}
		}

		DimensionItem *curr = const_cast<DimensionItem *>(this);
		QList<DimensionItem *> backward;
		while (curr) {
			DimensionItem *prev = nullptr;
			QPointF curr_p1 = curr->mapToScene(curr->p1());
			for (DimensionItem *it : candidates) {
				if (it != curr && !backward.contains(it) && it != this) {
					QPointF it_p2 = it->mapToScene(it->p2());
					if (std::hypot(it_p2.x() - curr_p1.x(), it_p2.y() - curr_p1.y()) < 2.0) {
						prev = it;
						break;
					}
				}
			}
			if (prev) {
				backward.prepend(prev);
				curr = prev;
			} else {
				break;
			}
		}

		chain.append(backward);
		chain.append(const_cast<DimensionItem *>(this));

		curr = const_cast<DimensionItem *>(this);
		while (curr) {
			DimensionItem *next = nullptr;
			QPointF curr_p2 = curr->mapToScene(curr->p2());
			for (DimensionItem *it : candidates) {
				if (it != curr && !chain.contains(it)) {
					QPointF it_p1 = it->mapToScene(it->p1());
					if (std::hypot(curr_p2.x() - it_p1.x(), curr_p2.y() - it_p1.y()) < 2.0) {
						next = it;
						break;
					}
				}
			}
			if (next) {
				chain.append(next);
				curr = next;
			} else {
				break;
			}
		}
		return chain;
	}

	// Sort segments in order P0 -> P1 -> P2 ... Pn
	QList<DimensionItem *> sorted;
	DimensionItem *first = nullptr;
	for (DimensionItem *it : chain) {
		QPointF p1_scene = it->mapToScene(it->p1());
		bool has_prev = false;
		for (DimensionItem *other : chain) {
			if (other != it) {
				QPointF other_p2 = other->mapToScene(other->p2());
				if (std::hypot(p1_scene.x() - other_p2.x(), p1_scene.y() - other_p2.y()) < 2.0) {
					has_prev = true;
					break;
				}
			}
		}
		if (!has_prev) {
			first = it;
			break;
		}
	}

	if (!first) {
		first = chain.first();
	}

	sorted.append(first);
	DimensionItem *curr = first;
	while (sorted.size() < chain.size()) {
		DimensionItem *next = nullptr;
		QPointF curr_p2 = curr->mapToScene(curr->p2());
		for (DimensionItem *it : chain) {
			if (!sorted.contains(it)) {
				QPointF it_p1 = it->mapToScene(it->p1());
				if (std::hypot(curr_p2.x() - it_p1.x(), curr_p2.y() - it_p1.y()) < 2.0) {
					next = it;
					break;
				}
			}
		}
		if (next) {
			sorted.append(next);
			curr = next;
		} else {
			for (DimensionItem *it : chain) {
				if (!sorted.contains(it)) sorted.append(it);
			}
			break;
		}
	}

	return sorted;
}

QList<DimensionItem *> DimensionItem::findChainSegments() const
{
	return chainSegments();
}

bool DimensionItem::isVerticalChain() const
{
	QList<DimensionItem *> chain = chainSegments();
	DimensionItem *firstSeg = chain.isEmpty() ? const_cast<DimensionItem *>(this) : chain.first();
	if (!firstSeg) return false;
	QPointF diff = firstSeg->p2() - firstSeg->p1();
	return std::abs(diff.y()) > std::abs(diff.x());
}

void DimensionItem::calculateGeometry(QPointF &p1_dim, QPointF &p2_dim,
                                     QPointF &p1_ext_end, QPointF &p2_ext_end,
                                     QPointF &mid_dim, qreal &angle_deg,
                                     QPointF &unit_dir, QPointF &normal_dir) const
{
	QPointF diff = m_p2 - m_p1;
	qreal len = std::hypot(diff.x(), diff.y());

	if (len < 1e-4) {
		unit_dir = QPointF(1.0, 0.0);
		normal_dir = QPointF(0.0, -1.0);
	} else {
		unit_dir = QPointF(diff.x() / len, diff.y() / len);
		normal_dir = QPointF(-unit_dir.y(), unit_dir.x());
	}

	QPointF offset_vec = normal_dir * m_offset;
	p1_dim = m_p1 + offset_vec;
	p2_dim = m_p2 + offset_vec;

	qreal overshoot = (m_offset >= 0.0 ? m_extension_overshoot : -m_extension_overshoot);
	p1_ext_end = p1_dim + normal_dir * overshoot;
	p2_ext_end = p2_dim + normal_dir * overshoot;

	mid_dim = (p1_dim + p2_dim) / 2.0;

	if (m_text_horizontal) {
		angle_deg = 0.0;
	} else {
		qreal rad = std::atan2(unit_dir.y(), unit_dir.x());
		angle_deg = rad * 180.0 / 3.14159265358979323846;

		// Keep dimension text readable from bottom or right edge
		if (angle_deg > 90.0) {
			angle_deg -= 180.0;
		} else if (angle_deg < -90.0) {
			angle_deg += 180.0;
		}
	}
}

QRectF DimensionItem::boundingRect() const
{
	QPointF p1_dim, p2_dim, p1_ext_end, p2_ext_end, mid_dim, unit_dir, normal_dir;
	qreal angle;
	calculateGeometry(p1_dim, p2_dim, p1_ext_end, p2_ext_end, mid_dim, angle, unit_dir, normal_dir);

	QRectF r;
	r = r.united(QRectF(m_p1, QSizeF(1, 1)));
	r = r.united(QRectF(m_p2, QSizeF(1, 1)));
	r = r.united(QRectF(p1_dim, QSizeF(1, 1)));
	r = r.united(QRectF(p2_dim, QSizeF(1, 1)));
	r = r.united(QRectF(p1_ext_end, QSizeF(1, 1)));
	r = r.united(QRectF(p2_ext_end, QSizeF(1, 1)));

	// Account for text bounds
	QFontMetricsF fm(m_font);
	QRectF textRect = fm.boundingRect(displayText());
	qreal maxTextDim = std::hypot(textRect.width(), textRect.height());
	r = r.united(QRectF(mid_dim.x() - maxTextDim, mid_dim.y() - maxTextDim, maxTextDim * 2.0, maxTextDim * 2.0));

	return r.adjusted(-12, -12, 12, 12);
}

QPainterPath DimensionItem::shape() const
{
	QPointF p1_dim, p2_dim, p1_ext_end, p2_ext_end, mid_dim, unit_dir, normal_dir;
	qreal angle;
	calculateGeometry(p1_dim, p2_dim, p1_ext_end, p2_ext_end, mid_dim, angle, unit_dir, normal_dir);

	QPainterPath path;
	// Extension line 1
	path.moveTo(m_p1);
	path.lineTo(p1_ext_end);
	// Extension line 2
	path.moveTo(m_p2);
	path.lineTo(p2_ext_end);
	// Dimension line
	path.moveTo(p1_dim);
	path.lineTo(p2_dim);

	QPainterPathStroker stroker;
	stroker.setWidth(8.0);
	QPainterPath stroked = stroker.createStroke(path);

	// Add text region
	QFontMetricsF fm(m_font);
	QRectF textRect = fm.boundingRect(displayText());
	stroked.addRect(QRectF(mid_dim.x() - textRect.width() / 2.0 - 4.0,
	                       mid_dim.y() - textRect.height() / 2.0 - 4.0,
	                       textRect.width() + 8.0,
	                       textRect.height() + 8.0));
	return stroked;
}

void DimensionItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

	QPointF p1_dim, p2_dim, p1_ext_end, p2_ext_end, mid_dim, unit_dir, normal_dir;
	qreal angle_deg;
	calculateGeometry(p1_dim, p2_dim, p1_ext_end, p2_ext_end, mid_dim, angle_deg, unit_dir, normal_dir);

	qreal diff_len = std::hypot(m_p2.x() - m_p1.x(), m_p2.y() - m_p1.y());
	if (diff_len < 1e-4) return;

	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);

	m_pen.setWidthF(m_line_width);
	painter->setPen(m_pen);
	painter->setBrush(m_pen.color());

	// 1. Draw extension lines (Maßhilfslinien) with optional gap from measured points
	QPointF gap_vec = normal_dir * (m_offset >= 0.0 ? m_extension_gap : -m_extension_gap);
	painter->drawLine(m_p1 + gap_vec, p1_ext_end);
	painter->drawLine(m_p2 + gap_vec, p2_ext_end);

	// 2. Draw dimension line (Maßlinie)
	painter->drawLine(p1_dim, p2_dim);

	// 3. Draw terminations (Arrows or Slashes)
	const qreal arrow_len = m_arrow_size;
	const qreal arrow_half_w = m_arrow_size * 0.35;

	if (m_termination_style == Arrows) {
		if (diff_len >= arrow_len * 2.5) {
			// Arrows inside pointing outward towards extension lines
			QPointF base1 = p1_dim + unit_dir * arrow_len;
			QPolygonF arrow1;
			arrow1 << p1_dim << (base1 + normal_dir * arrow_half_w) << (base1 - normal_dir * arrow_half_w);
			painter->drawPolygon(arrow1);

			QPointF base2 = p2_dim - unit_dir * arrow_len;
			QPolygonF arrow2;
			arrow2 << p2_dim << (base2 + normal_dir * arrow_half_w) << (base2 - normal_dir * arrow_half_w);
			painter->drawPolygon(arrow2);
		} else {
			// Narrow segment: arrows placed outside pointing inward
			QPointF base1 = p1_dim - unit_dir * arrow_len;
			QPolygonF arrow1;
			arrow1 << p1_dim << (base1 + normal_dir * arrow_half_w) << (base1 - normal_dir * arrow_half_w);
			painter->drawPolygon(arrow1);

			QPointF base2 = p2_dim + unit_dir * arrow_len;
			QPolygonF arrow2;
			arrow2 << p2_dim << (base2 + normal_dir * arrow_half_w) << (base2 - normal_dir * arrow_half_w);
			painter->drawPolygon(arrow2);
		}
	} else {
		// Architectural slashes (45° tick marks)
		QPointF slash_vec = (unit_dir + normal_dir) * (m_arrow_size * 0.5);
		painter->drawLine(p1_dim - slash_vec, p1_dim + slash_vec);
		painter->drawLine(p2_dim - slash_vec, p2_dim + slash_vec);
	}

	// 4. Draw dimension text (Maßtext)
	QString text = displayText();
	if (!text.isEmpty()) {
		painter->save();
		painter->setFont(m_font);

		QFontMetricsF fm(m_font);
		QRectF text_bounds = fm.boundingRect(text);

		QPointF text_pos = mid_dim;
		if (!m_text_in_line) {
			// Text placed alongside the dimension line with distance
			qreal dist = m_text_distance > 0.0 ? m_text_distance * 2.0 : 4.0;
			text_pos += normal_dir * (m_offset >= 0.0 ? dist : -dist);
		}

		painter->translate(text_pos);
		painter->rotate(angle_deg);

		if (m_framed) {
			// Boxed / Framed text as seen in CAD panel drawings
			QRectF boxRect = text_bounds.adjusted(-4, -2, 4, 2);
			if (m_mask_background && diagram()) {
				painter->fillRect(boxRect, diagram()->backgroundBrush());
			}
			painter->setPen(m_pen);
			painter->setBrush(Qt::NoBrush);
			painter->drawRect(boxRect);
		} else if (m_mask_background && m_text_in_line && diagram()) {
			// Mask background behind text so dimension line does not cut through
			QRectF maskRect = text_bounds.adjusted(-2, -1, 2, 1);
			painter->fillRect(maskRect, diagram()->backgroundBrush());
		}

		painter->setPen(m_pen);
		painter->drawText(QPointF(-text_bounds.width() / 2.0, text_bounds.height() / 4.0), text);
		painter->restore();
	}

	painter->restore();
}

QVariant DimensionItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemSelectedHasChanged) {
		bool is_sel = value.toBool();
		static bool s_syncing_selection = false;
		if (m_mode == Chain && !s_syncing_selection && diagram()) {
			s_syncing_selection = true;
			for (DimensionItem *sibling : chainSegments()) {
				if (sibling != this && sibling->isSelected() != is_sel) {
					sibling->setSelected(is_sel);
				}
			}
			s_syncing_selection = false;
		}

		if (is_sel) {
			rebuildHandles();
		} else {
			clearHandles();
		}
	} else if (change == ItemPositionHasChanged) {
		emit geometryChanged();
		if (m_mode == Chain) {
			QList<DimensionItem *> chain = chainSegments();
			if (!chain.isEmpty() && chain.first()) {
				chain.first()->updateHandlePositions();
			}
		} else {
			updateHandlePositions();
		}
	} else if (change == ItemSceneChange) {
		clearHandles();
	}
	return QetGraphicsItem::itemChange(change, value);
}

void DimensionItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_mode == Chain && event->button() == Qt::LeftButton) {
		if (!isSelected()) {
			if (!(event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier))) {
				if (scene()) {
					scene()->clearSelection();
				}
			}
			for (DimensionItem *sibling : chainSegments()) {
				if (sibling) {
					sibling->setSelected(true);
				}
			}
		}
	}
	QetGraphicsItem::mousePressEvent(event);
}

void DimensionItem::rebuildHandles()
{
	clearHandles();
	if (!scene() || !isSelected()) return;

	QList<DimensionItem *> chain = chainSegments();
	if (m_mode == Chain && !chain.isEmpty() && chain.first() != this) {
		// In chain mode, only the first segment acts as coordinator for all handles
		return;
	}

	if (chain.isEmpty()) {
		chain.append(this);
	}

	// 1. Create ReferencePoint handles (one handle for each unique point along the chain)
	// Index 0: Start of first segment
	auto *hStart = new DimensionHandler(this, DimensionHandler::ReferencePoint, 0);
	scene()->addItem(hStart);
	m_handlers.append(hStart);

	// Index 1 .. N-1: Intermediate points (p1 of segment k, connecting segment k-1 and segment k)
	for (int k = 1; k < chain.size(); ++k) {
		auto *hInter = new DimensionHandler(this, DimensionHandler::ReferencePoint, k);
		scene()->addItem(hInter);
		m_handlers.append(hInter);
	}

	// Index N: End of last segment
	auto *hEnd = new DimensionHandler(this, DimensionHandler::ReferencePoint, chain.size());
	scene()->addItem(hEnd);
	m_handlers.append(hEnd);

	// 2. Create OffsetPoint handles (on the dimension lines for adjusting height)
	for (int k = 0; k < chain.size(); ++k) {
		auto *hDim1 = new DimensionHandler(this, DimensionHandler::OffsetPoint, k * 2);
		scene()->addItem(hDim1);
		m_handlers.append(hDim1);

		if (k == chain.size() - 1) {
			auto *hDim2 = new DimensionHandler(this, DimensionHandler::OffsetPoint, k * 2 + 1);
			scene()->addItem(hDim2);
			m_handlers.append(hDim2);
		}
	}

	updateHandlePositions();
}

void DimensionItem::updateHandlePositions()
{
	if (m_mode == Chain) {
		QList<DimensionItem *> chain = chainSegments();
		if (!chain.isEmpty() && chain.first() != this) {
			if (chain.first()) chain.first()->updateHandlePositions();
			return;
		}
	}

	if (m_handlers.isEmpty() || !scene()) return;

	QList<DimensionItem *> chain = chainSegments();
	if (chain.isEmpty()) chain.append(this);

	int handleIdx = 0;

	// Handle 0: Start point of first segment
	if (handleIdx < m_handlers.size() && chain.first()) {
		m_handlers[handleIdx]->setPos(chain.first()->mapToScene(chain.first()->p1()));
		handleIdx++;
	}

	// Intermediate handles 1 .. N-1
	for (int k = 1; k < chain.size(); ++k) {
		if (handleIdx < m_handlers.size() && chain.at(k)) {
			m_handlers[handleIdx]->setPos(chain.at(k)->mapToScene(chain.at(k)->p1()));
			handleIdx++;
		}
	}

	// Handle N: End point of last segment
	if (handleIdx < m_handlers.size() && chain.last()) {
		m_handlers[handleIdx]->setPos(chain.last()->mapToScene(chain.last()->p2()));
		handleIdx++;
	}

	// Offset handles on the dimension lines
	for (int k = 0; k < chain.size(); ++k) {
		DimensionItem *seg = chain.at(k);
		if (!seg) continue;

		QPointF p1_dim, p2_dim, p1_ext, p2_ext, mid, udir, ndir;
		qreal ang;
		seg->calculateGeometry(p1_dim, p2_dim, p1_ext, p2_ext, mid, ang, udir, ndir);

		if (handleIdx < m_handlers.size()) {
			m_handlers[handleIdx]->setPos(seg->mapToScene(p1_dim));
			handleIdx++;
		}
		if (k == chain.size() - 1 && handleIdx < m_handlers.size()) {
			m_handlers[handleIdx]->setPos(seg->mapToScene(p2_dim));
			handleIdx++;
		}
	}
}

void DimensionItem::clearHandles()
{
	for (DimensionHandler *h : m_handlers) {
		if (h) {
			if (h->scene()) {
				h->scene()->removeItem(h);
			}
			delete h;
		}
	}
	m_handlers.clear();
}

QDomElement DimensionItem::toXml(QDomDocument &document) const
{
	QDomElement xml = document.createElement(QStringLiteral("dimension"));
	xml.setAttribute(QStringLiteral("uuid"), m_uuid.toString());
	if (!m_chain_id.isNull()) {
		xml.setAttribute(QStringLiteral("chain_id"), m_chain_id.toString());
	}
	xml.setAttribute(QStringLiteral("x1"), QString::number(m_p1.x() + pos().x(), 'f', 2));
	xml.setAttribute(QStringLiteral("y1"), QString::number(m_p1.y() + pos().y(), 'f', 2));
	xml.setAttribute(QStringLiteral("x2"), QString::number(m_p2.x() + pos().x(), 'f', 2));
	xml.setAttribute(QStringLiteral("y2"), QString::number(m_p2.y() + pos().y(), 'f', 2));
	xml.setAttribute(QStringLiteral("offset"), QString::number(m_offset, 'f', 2));
	xml.setAttribute(QStringLiteral("auto"), m_auto_value ? QStringLiteral("true") : QStringLiteral("false"));
	xml.setAttribute(QStringLiteral("text"), m_custom_text);
	xml.setAttribute(QStringLiteral("prefix"), m_prefix);
	xml.setAttribute(QStringLiteral("suffix"), m_suffix);
	xml.setAttribute(QStringLiteral("precision"), QString::number(m_precision));
	xml.setAttribute(QStringLiteral("unit"), m_unit);
	xml.setAttribute(QStringLiteral("show_unit"), m_show_unit ? QStringLiteral("true") : QStringLiteral("false"));
	xml.setAttribute(QStringLiteral("framed"), m_framed ? QStringLiteral("true") : QStringLiteral("false"));
	xml.setAttribute(QStringLiteral("comma"), m_use_comma ? QStringLiteral("true") : QStringLiteral("false"));
	xml.setAttribute(QStringLiteral("distance"), QString::number(m_text_distance, 'f', 2));
	xml.setAttribute(QStringLiteral("center"), m_text_center ? QStringLiteral("true") : QStringLiteral("false"));
	xml.setAttribute(QStringLiteral("dual_unit"), m_show_dual_unit ? QStringLiteral("true") : QStringLiteral("false"));
	xml.setAttribute(QStringLiteral("horizontal"), m_text_horizontal ? QStringLiteral("true") : QStringLiteral("false"));
	xml.setAttribute(QStringLiteral("mid_line"), m_text_in_line ? QStringLiteral("true") : QStringLiteral("false"));
	xml.setAttribute(QStringLiteral("suppress_zero"), m_suppress_zero ? QStringLiteral("true") : QStringLiteral("false"));
	xml.setAttribute(QStringLiteral("line_width"), QString::number(m_line_width, 'f', 2));
	xml.setAttribute(QStringLiteral("overshoot"), QString::number(m_extension_overshoot, 'f', 2));
	xml.setAttribute(QStringLiteral("gap"), QString::number(m_extension_gap, 'f', 2));
	xml.setAttribute(QStringLiteral("arrow_size"), QString::number(m_arrow_size, 'f', 2));
	xml.setAttribute(QStringLiteral("mask"), m_mask_background ? QStringLiteral("true") : QStringLiteral("false"));
	xml.setAttribute(QStringLiteral("font_size"), QString::number(fontSize()));
	xml.setAttribute(QStringLiteral("mode"), m_mode == Chain ? QStringLiteral("chain") : QStringLiteral("linear"));
	xml.setAttribute(QStringLiteral("style"), m_termination_style == Slashes ? QStringLiteral("slashes") : QStringLiteral("arrows"));
	return xml;
}

void DimensionItem::fromXml(const QDomElement &element)
{
	setPos(0, 0);
	qreal x1 = element.attribute(QStringLiteral("x1"), QStringLiteral("0")).toDouble();
	qreal y1 = element.attribute(QStringLiteral("y1"), QStringLiteral("0")).toDouble();
	qreal x2 = element.attribute(QStringLiteral("x2"), QStringLiteral("0")).toDouble();
	qreal y2 = element.attribute(QStringLiteral("y2"), QStringLiteral("0")).toDouble();

	m_p1 = QPointF(x1, y1);
	m_p2 = QPointF(x2, y2);
	m_offset = element.attribute(QStringLiteral("offset"), QStringLiteral("20")).toDouble();
	m_auto_value = (element.attribute(QStringLiteral("auto"), QStringLiteral("true")) == QStringLiteral("true"));
	m_custom_text = element.attribute(QStringLiteral("text"));
	m_prefix = element.attribute(QStringLiteral("prefix"));
	m_suffix = element.attribute(QStringLiteral("suffix"));
	m_precision = element.attribute(QStringLiteral("precision"), QStringLiteral("1")).toInt();
	m_unit = element.attribute(QStringLiteral("unit"), QStringLiteral("mm"));
	m_show_unit = (element.attribute(QStringLiteral("show_unit"), QStringLiteral("false")) == QStringLiteral("true"));
	m_framed = (element.attribute(QStringLiteral("framed"), QStringLiteral("false")) == QStringLiteral("true"));
	m_use_comma = (element.attribute(QStringLiteral("comma"), QStringLiteral("true")) == QStringLiteral("true"));
	m_text_distance = element.attribute(QStringLiteral("distance"), QStringLiteral("1.0")).toDouble();
	m_text_center = (element.attribute(QStringLiteral("center"), QStringLiteral("true")) == QStringLiteral("true"));
	m_show_dual_unit = (element.attribute(QStringLiteral("dual_unit"), QStringLiteral("false")) == QStringLiteral("true"));
	m_text_horizontal = (element.attribute(QStringLiteral("horizontal"), QStringLiteral("false")) == QStringLiteral("true"));
	m_text_in_line = (element.attribute(QStringLiteral("mid_line"), QStringLiteral("false")) == QStringLiteral("true"));
	m_suppress_zero = (element.attribute(QStringLiteral("suppress_zero"), QStringLiteral("false")) == QStringLiteral("true"));
	m_line_width = element.attribute(QStringLiteral("line_width"), QStringLiteral("1.2")).toDouble();
	m_pen.setWidthF(m_line_width);
	m_extension_overshoot = element.attribute(QStringLiteral("overshoot"), QStringLiteral("5.0")).toDouble();
	m_extension_gap = element.attribute(QStringLiteral("gap"), QStringLiteral("0.0")).toDouble();
	m_arrow_size = element.attribute(QStringLiteral("arrow_size"), QStringLiteral("8.0")).toDouble();
	m_mask_background = (element.attribute(QStringLiteral("mask"), QStringLiteral("false")) == QStringLiteral("true"));
	int fsize = element.attribute(QStringLiteral("font_size"), QStringLiteral("8")).toInt();
	m_font.setPointSize(fsize > 0 ? fsize : 8);

	QString modeStr = element.attribute(QStringLiteral("mode"), QStringLiteral("linear"));
	m_mode = (modeStr == QStringLiteral("chain")) ? Chain : Linear;

	QString styleStr = element.attribute(QStringLiteral("style"), QStringLiteral("arrows"));
	m_termination_style = (styleStr == QStringLiteral("slashes")) ? Slashes : Arrows;

	if (element.hasAttribute(QStringLiteral("uuid"))) {
		m_uuid = QUuid(element.attribute(QStringLiteral("uuid")));
	}
	if (element.hasAttribute(QStringLiteral("chain_id"))) {
		m_chain_id = QUuid(element.attribute(QStringLiteral("chain_id")));
	}
	prepareGeometryChange();
}

DimensionProperties DimensionFormatPattern::s_current_pattern;
bool DimensionFormatPattern::s_has_pattern = false;

bool DimensionFormatPattern::hasPattern()
{
	return s_has_pattern;
}

const DimensionProperties &DimensionFormatPattern::currentPattern()
{
	return s_current_pattern;
}

void DimensionFormatPattern::setCurrentPattern(const DimensionProperties &p)
{
	s_current_pattern = p;
	s_has_pattern = true;
}

void DimensionFormatPattern::clearPattern()
{
	s_has_pattern = false;
}

ApplyDimensionFormatCommand::ApplyDimensionFormatCommand(const QList<DimensionItem *> &items,
                                                         const DimensionProperties &pattern,
                                                         QUndoCommand *parent) :
	QUndoCommand(parent)
{
	setText(QObject::tr("Apply dimension format"));

	for (DimensionItem *it : items) {
		if (!it) continue;
		ItemChange change;
		change.item = it;
		change.oldProps = it->properties();

		DimensionProperties newP = change.oldProps;
		newP.prefix = pattern.prefix;
		newP.suffix = pattern.suffix;
		newP.unit = pattern.unit;
		newP.showUnit = pattern.showUnit;
		newP.showDualUnit = pattern.showDualUnit;
		newP.precision = pattern.precision;
		newP.useComma = pattern.useComma;
		newP.suppressZero = pattern.suppressZero;
		newP.textDistance = pattern.textDistance;
		newP.textCenter = pattern.textCenter;
		newP.isTextHorizontal = pattern.isTextHorizontal;
		newP.isTextInLine = pattern.isTextInLine;
		newP.lineWidth = pattern.lineWidth;
		newP.extensionOvershoot = pattern.extensionOvershoot;
		newP.extensionGap = pattern.extensionGap;
		newP.terminationStyle = pattern.terminationStyle;
		newP.arrowSize = pattern.arrowSize;
		newP.isFramed = pattern.isFramed;
		newP.maskBackground = pattern.maskBackground;
		newP.fontSize = pattern.fontSize;

		change.newProps = newP;
		m_changes.append(change);
	}
}

void ApplyDimensionFormatCommand::undo()
{
	for (const auto &c : m_changes) {
		if (c.item) {
			c.item->setProperties(c.oldProps);
		}
	}
}

void ApplyDimensionFormatCommand::redo()
{
	for (const auto &c : m_changes) {
		if (c.item) {
			c.item->setProperties(c.newProps);
		}
	}
}

DimensionHandler::DimensionHandler(DimensionItem *coordinator, Role role, int index, QGraphicsItem *parent) :
	QGraphicsObject(parent),
	m_coordinator(coordinator),
	m_role(role),
	m_index(index)
{
	setAcceptHoverEvents(true);
	setFlag(ItemIgnoresTransformations, true);
	setZValue(1000);
}

DimensionHandler::~DimensionHandler()
{
}

QRectF DimensionHandler::boundingRect() const
{
	return QRectF(-4.5, -4.5, 9.0, 9.0);
}

QPainterPath DimensionHandler::shape() const
{
	QPainterPath path;
	path.addRect(QRectF(-4.5, -4.5, 9.0, 9.0));
	return path;
}

void DimensionHandler::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, false);

	QPen pen(Qt::black, 1.0);
	pen.setCosmetic(true);
	painter->setPen(pen);
	painter->setBrush(m_hovered ? QBrush(QColor(255, 60, 60)) : QBrush(Qt::red));
	painter->drawRect(QRectF(-3.5, -3.5, 7.0, 7.0));

	painter->restore();
}

void DimensionHandler::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	Q_UNUSED(event)
	m_hovered = true;
	setCursor(Qt::SizeAllCursor);
	update();
}

void DimensionHandler::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	Q_UNUSED(event)
	m_hovered = false;
	unsetCursor();
	update();
}

void DimensionHandler::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() != Qt::LeftButton || !m_coordinator) {
		event->ignore();
		return;
	}

	m_dragging = true;
	m_press_pos = event->scenePos();

	m_saved_states.clear();
	QList<DimensionItem *> chain = m_coordinator->chainSegments();
	if (chain.isEmpty()) chain.append(m_coordinator.data());
	for (DimensionItem *seg : chain) {
		if (seg) {
			m_saved_states.append({ seg, seg->p1(), seg->p2(), seg->offset() });
		}
	}

	event->accept();
}

void DimensionHandler::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_dragging || !m_coordinator) {
		event->ignore();
		return;
	}

	QPointF scenePos = Diagram::snapToGrid(event->scenePos());
	QList<DimensionItem *> chain = m_coordinator->chainSegments();
	if (chain.isEmpty()) chain.append(m_coordinator.data());

	if (m_role == ReferencePoint) {
		if (m_coordinator->mode() == DimensionItem::Chain || chain.size() > 1) {
			bool isVert = m_coordinator->isVerticalChain();
			if (isVert) {
				qreal refX = chain.first()->mapToScene(chain.first()->p1()).x();
				scenePos.setX(refX);
			} else {
				qreal refY = chain.first()->mapToScene(chain.first()->p1()).y();
				scenePos.setY(refY);
			}
		}

		if (m_index == 0) {
			DimensionItem *s0 = chain.first();
			if (s0) {
				s0->setP1(s0->mapFromScene(scenePos));
			}
		} else if (m_index == chain.size()) {
			DimensionItem *sLast = chain.last();
			if (sLast) {
				sLast->setP2(sLast->mapFromScene(scenePos));
			}
		} else if (m_index > 0 && m_index < chain.size()) {
			DimensionItem *prevSeg = chain.at(m_index - 1);
			DimensionItem *nextSeg = chain.at(m_index);
			if (prevSeg) {
				prevSeg->setP2(prevSeg->mapFromScene(scenePos));
			}
			if (nextSeg) {
				nextSeg->setP1(nextSeg->mapFromScene(scenePos));
			}
		}
	} else if (m_role == OffsetPoint) {
		DimensionItem *firstSeg = chain.first();
		if (firstSeg) {
			QPointF p1_scene = firstSeg->mapToScene(firstSeg->p1());
			QPointF p2_scene = firstSeg->mapToScene(firstSeg->p2());
			QPointF diff = p2_scene - p1_scene;
			qreal len = std::hypot(diff.x(), diff.y());
			if (len > 1e-4) {
				QPointF normal_dir(-diff.y() / len, diff.x() / len);
				QPointF toCursor = event->scenePos() - p1_scene;
				qreal newOffset = toCursor.x() * normal_dir.x() + toCursor.y() * normal_dir.y();
				newOffset = std::round(newOffset / 5.0) * 5.0;
				if (std::abs(newOffset) < 5.0) {
					newOffset = (newOffset >= 0.0 ? 5.0 : -5.0);
				}

				for (DimensionItem *seg : chain) {
					if (seg) seg->setOffset(newOffset);
				}
			}
		}
	}

	m_coordinator->updateHandlePositions();
	event->accept();
}

void DimensionHandler::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_dragging || !m_coordinator) {
		event->ignore();
		return;
	}
	m_dragging = false;

	if (!m_coordinator->diagram()) {
		event->accept();
		return;
	}

	bool changed = false;
	QList<MoveDimensionPointCommand::PointChange> pointChanges;
	for (const auto &state : m_saved_states) {
		if (state.item) {
			if (state.item->p1() != state.oldP1 || state.item->p2() != state.oldP2) {
				changed = true;
				pointChanges.append({ state.item, state.oldP1, state.oldP2, state.item->p1(), state.item->p2() });
			}
			if (!qFuzzyCompare(state.item->offset(), state.oldOffset)) {
				changed = true;
			}
		}
	}

	if (!changed) {
		event->accept();
		return;
	}

	if (m_role == ReferencePoint && !pointChanges.isEmpty()) {
		m_coordinator->diagram()->undoStack().push(new MoveDimensionPointCommand(pointChanges));
	} else if (m_role == OffsetPoint && !m_saved_states.isEmpty()) {
		QList<DimensionItem *> items;
		qreal oldOff = m_saved_states.first().oldOffset;
		qreal newOff = m_saved_states.first().item ? m_saved_states.first().item->offset() : oldOff;
		for (const auto &s : m_saved_states) {
			if (s.item) items.append(s.item.data());
		}
		m_coordinator->diagram()->undoStack().push(new ChangeDimensionOffsetCommand(items, oldOff, newOff));
	}

	event->accept();
}

MoveDimensionPointCommand::MoveDimensionPointCommand(const QList<PointChange> &changes, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_changes(changes)
{
	setText(QObject::tr("Move dimension point"));
}

void MoveDimensionPointCommand::undo()
{
	for (const auto &c : m_changes) {
		if (c.item) {
			c.item->setP1(c.oldP1);
			c.item->setP2(c.oldP2);
		}
	}
}

void MoveDimensionPointCommand::redo()
{
	for (const auto &c : m_changes) {
		if (c.item) {
			c.item->setP1(c.newP1);
			c.item->setP2(c.newP2);
		}
	}
}

ChangeDimensionOffsetCommand::ChangeDimensionOffsetCommand(const QList<DimensionItem *> &items, qreal oldOffset, qreal newOffset, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_old_offset(oldOffset),
	m_new_offset(newOffset)
{
	setText(QObject::tr("Change dimension offset"));
	for (DimensionItem *it : items) {
		if (it) m_items.append({ it });
	}
}

void ChangeDimensionOffsetCommand::undo()
{
	for (const auto &it : m_items) {
		if (it.item) {
			it.item->setOffset(m_old_offset);
		}
	}
}

void ChangeDimensionOffsetCommand::redo()
{
	for (const auto &it : m_items) {
		if (it.item) {
			it.item->setOffset(m_new_offset);
		}
	}
}
