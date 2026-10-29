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

#include <cmath>
#include <QDomDocument>
#include <QDomElement>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPathStroker>
#include <QStyleOptionGraphicsItem>

/**
	@brief DimensionItem::DimensionItem
	Constructor
*/
DimensionItem::DimensionItem(DimensionMode mode, QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	m_p1(0, 0),
	m_p2(0, 0),
	m_offset(20.0),
	m_mode(mode),
	m_uuid(QUuid::createUuid()),
	m_pen(QColor(40, 40, 40), 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin),
	m_font(QStringLiteral("Arial"), 8)
{
	setFlags(ItemIsSelectable | ItemIsMovable);
	setAcceptHoverEvents(true);
	setZValue(15);
}

/**
	@brief DimensionItem::DimensionItem
	Constructor with geometry
*/
DimensionItem::DimensionItem(const QPointF &p1, const QPointF &p2, qreal offset,
                             DimensionMode mode, QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	m_p1(p1),
	m_p2(p2),
	m_offset(offset),
	m_mode(mode),
	m_uuid(QUuid::createUuid()),
	m_pen(QColor(40, 40, 40), 1.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin),
	m_font(QStringLiteral("Arial"), 8)
{
	setFlags(ItemIsSelectable | ItemIsMovable);
	setAcceptHoverEvents(true);
	setZValue(15);
}

DimensionItem::~DimensionItem()
{
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

qreal DimensionItem::dimensionValueMM() const
{
	qreal dx = m_p2.x() - m_p1.x();
	qreal dy = m_p2.y() - m_p1.y();
	qreal dist_px = std::hypot(dx, dy);
	return dist_px / 2.0; // Strictly 2 px = 1.0 mm
}

QString DimensionItem::displayText() const
{
	if (m_auto_value) {
		return QString::number(dimensionValueMM(), 'f', 1);
	}
	return m_custom_text;
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

	qreal overshoot = (m_offset >= 0.0 ? 5.0 : -5.0);
	p1_ext_end = p1_dim + normal_dir * overshoot;
	p2_ext_end = p2_dim + normal_dir * overshoot;

	mid_dim = (p1_dim + p2_dim) / 2.0;

	qreal rad = std::atan2(unit_dir.y(), unit_dir.x());
	angle_deg = rad * 180.0 / 3.14159265358979323846;

	// Keep dimension text readable from bottom or right edge
	if (angle_deg > 90.0) {
		angle_deg -= 180.0;
	} else if (angle_deg < -90.0) {
		angle_deg += 180.0;
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

	return r.adjusted(-10, -10, 10, 10);
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

	// Selection highlight or hover
	if (isSelected()) {
		painter->save();
		QPen selPen(QColor(0, 120, 215, 120), 4.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
		painter->setPen(selPen);
		painter->setBrush(Qt::NoBrush);
		painter->drawPath(shape());
		painter->restore();
	}

	painter->setPen(m_pen);
	painter->setBrush(m_pen.color());

	// 1. Draw extension lines (Maßhilfslinien)
	painter->drawLine(m_p1, p1_ext_end);
	painter->drawLine(m_p2, p2_ext_end);

	// 2. Draw dimension line (Maßlinie)
	painter->drawLine(p1_dim, p2_dim);

	// 3. Draw terminations (Arrows or Slashes)
	const qreal arrow_len = 8.0;
	const qreal arrow_half_w = 2.5;

	if (m_termination_style == Arrows) {
		if (diff_len >= 20.0) {
			// Arrows inside pointing outward towards extension lines
			// Arrow 1 at p1_dim pointing towards p1_dim
			QPointF base1 = p1_dim + unit_dir * arrow_len;
			QPolygonF arrow1;
			arrow1 << p1_dim << (base1 + normal_dir * arrow_half_w) << (base1 - normal_dir * arrow_half_w);
			painter->drawPolygon(arrow1);

			// Arrow 2 at p2_dim pointing towards p2_dim
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
		QPointF slash_vec = (unit_dir + normal_dir) * 4.0;
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

		painter->translate(mid_dim);
		painter->rotate(angle_deg);

		// Mask background behind text so dimension line does not cut through
		QRectF maskRect = text_bounds.adjusted(-2, -1, 2, 1);
		painter->fillRect(maskRect, QBrush(Qt::white));

		painter->drawText(QPointF(-text_bounds.width() / 2.0, text_bounds.height() / 4.0), text);
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

	QString modeStr = element.attribute(QStringLiteral("mode"), QStringLiteral("linear"));
	m_mode = (modeStr == QStringLiteral("chain")) ? Chain : Linear;

	QString styleStr = element.attribute(QStringLiteral("style"), QStringLiteral("arrows"));
	m_termination_style = (styleStr == QStringLiteral("slashes")) ? Slashes : Arrows;

	if (element.hasAttribute(QStringLiteral("uuid"))) {
		m_uuid = QUuid(element.attribute(QStringLiteral("uuid")));
	}
	prepareGeometryChange();
}

