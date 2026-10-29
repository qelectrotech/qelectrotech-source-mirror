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
#include <QDomDocument>
#include <QDomElement>
#include <QFontMetricsF>
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

bool DimensionItem::isFractionFormat() const
{
	return m_fraction_format;
}

void DimensionItem::setFractionFormat(bool f)
{
	if (m_fraction_format == f) return;
	prepareGeometryChange();
	m_fraction_format = f;
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
	p.fractionFormat = m_fraction_format;
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
	m_fraction_format = p.fractionFormat;
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

QList<DimensionItem *> DimensionItem::findChainSegments() const
{
	if (m_mode != Chain || !diagram()) {
		return { const_cast<DimensionItem *>(this) };
	}

	QList<DimensionItem *> candidates;
	for (QGraphicsItem *qgi : diagram()->items()) {
		if (qgi->type() == DimensionItem::Type) {
			auto *it = static_cast<DimensionItem *>(qgi);
			if (it->mode() == Chain && std::abs(it->offset() - m_offset) < 2.0) {
				candidates.append(it);
			}
		}
	}

	QList<DimensionItem *> chain;
	// Trace backward: it->p2() == curr->p1()
	DimensionItem *curr = const_cast<DimensionItem *>(this);
	QList<DimensionItem *> backward;
	while (curr) {
		DimensionItem *prev = nullptr;
		for (DimensionItem *it : candidates) {
			if (it != curr && !backward.contains(it) && it != this) {
				qreal d = std::hypot(it->p2().x() - curr->p1().x(), it->p2().y() - curr->p1().y());
				if (d < 2.0) {
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

	// Trace forward: curr->p2() == it->p1()
	curr = const_cast<DimensionItem *>(this);
	while (curr) {
		DimensionItem *next = nullptr;
		for (DimensionItem *it : candidates) {
			if (it != curr && !chain.contains(it)) {
				qreal d = std::hypot(curr->p2().x() - it->p1().x(), curr->p2().y() - it->p1().y());
				if (d < 2.0) {
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
			painter->fillRect(boxRect, QBrush(Qt::white));
			painter->setPen(m_pen);
			painter->setBrush(Qt::NoBrush);
			painter->drawRect(boxRect);
		} else if (m_mask_background && m_text_in_line) {
			// Mask background behind text so dimension line does not cut through
			QRectF maskRect = text_bounds.adjusted(-2, -1, 2, 1);
			painter->fillRect(maskRect, QBrush(Qt::white));
		}

		painter->setPen(m_pen);
		painter->drawText(QPointF(-text_bounds.width() / 2.0, text_bounds.height() / 4.0), text);
		painter->restore();
	}

	// 5. Draw red square handles when selected (as shown in panel layout drawings)
	if (isSelected()) {
		painter->save();
		painter->setPen(QPen(Qt::black, 1.0));
		painter->setBrush(QBrush(Qt::red));
		const qreal hs = 3.0;
		painter->drawRect(QRectF(m_p1.x() - hs, m_p1.y() - hs, hs * 2.0, hs * 2.0));
		painter->drawRect(QRectF(m_p2.x() - hs, m_p2.y() - hs, hs * 2.0, hs * 2.0));
		painter->drawRect(QRectF(p1_dim.x() - hs, p1_dim.y() - hs, hs * 2.0, hs * 2.0));
		painter->drawRect(QRectF(p2_dim.x() - hs, p2_dim.y() - hs, hs * 2.0, hs * 2.0));
		painter->restore();
	}

	painter->restore();
}

QVariant DimensionItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemPositionHasChanged) {
		emit geometryChanged();
	}
	return QetGraphicsItem::itemChange(change, value);
}

QDomElement DimensionItem::toXml(QDomDocument &document) const
{
	QDomElement xml = document.createElement(QStringLiteral("dimension"));
	xml.setAttribute(QStringLiteral("uuid"), m_uuid.toString());
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
	xml.setAttribute(QStringLiteral("fraction"), m_fraction_format ? QStringLiteral("true") : QStringLiteral("false"));
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
	m_framed = (element.attribute(QStringLiteral("framed"), QStringLiteral("true")) == QStringLiteral("true"));
	m_use_comma = (element.attribute(QStringLiteral("comma"), QStringLiteral("true")) == QStringLiteral("true"));
	m_text_distance = element.attribute(QStringLiteral("distance"), QStringLiteral("1.0")).toDouble();
	m_text_center = (element.attribute(QStringLiteral("center"), QStringLiteral("true")) == QStringLiteral("true"));
	m_show_dual_unit = (element.attribute(QStringLiteral("dual_unit"), QStringLiteral("false")) == QStringLiteral("true"));
	m_fraction_format = (element.attribute(QStringLiteral("fraction"), QStringLiteral("false")) == QStringLiteral("true"));
	m_text_horizontal = (element.attribute(QStringLiteral("horizontal"), QStringLiteral("false")) == QStringLiteral("true"));
	m_text_in_line = (element.attribute(QStringLiteral("mid_line"), QStringLiteral("true")) == QStringLiteral("true"));
	m_suppress_zero = (element.attribute(QStringLiteral("suppress_zero"), QStringLiteral("false")) == QStringLiteral("true"));
	m_line_width = element.attribute(QStringLiteral("line_width"), QStringLiteral("1.2")).toDouble();
	m_pen.setWidthF(m_line_width);
	m_extension_overshoot = element.attribute(QStringLiteral("overshoot"), QStringLiteral("5.0")).toDouble();
	m_extension_gap = element.attribute(QStringLiteral("gap"), QStringLiteral("0.0")).toDouble();
	m_arrow_size = element.attribute(QStringLiteral("arrow_size"), QStringLiteral("8.0")).toDouble();
	m_mask_background = (element.attribute(QStringLiteral("mask"), QStringLiteral("true")) == QStringLiteral("true"));
	int fsize = element.attribute(QStringLiteral("font_size"), QStringLiteral("8")).toInt();
	m_font.setPointSize(fsize > 0 ? fsize : 8);

	QString modeStr = element.attribute(QStringLiteral("mode"), QStringLiteral("linear"));
	m_mode = (modeStr == QStringLiteral("chain")) ? Chain : Linear;

	QString styleStr = element.attribute(QStringLiteral("style"), QStringLiteral("arrows"));
	m_termination_style = (styleStr == QStringLiteral("slashes")) ? Slashes : Arrows;

	if (element.hasAttribute(QStringLiteral("uuid"))) {
		m_uuid = QUuid(element.attribute(QStringLiteral("uuid")));
	}
	prepareGeometryChange();
}
