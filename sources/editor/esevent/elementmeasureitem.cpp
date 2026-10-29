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
#include "elementmeasureitem.h"

#include <QPainter>
#include <QFontMetricsF>
#include <QtMath>
#include <QLineF>

ElementMeasureItem::ElementMeasureItem(const QPointF &p1, const QPointF &p2, QGraphicsItem *parent) :
	QGraphicsItem(parent),
	m_p1(p1),
	m_p2(p2)
{
	setZValue(1000.0); // Keep on top of element graphics
}

void ElementMeasureItem::setP1(const QPointF &p1)
{
	if (m_p1 == p1) return;
	prepareGeometryChange();
	m_p1 = p1;
	update();
}

void ElementMeasureItem::setP2(const QPointF &p2)
{
	if (m_p2 == p2) return;
	prepareGeometryChange();
	m_p2 = p2;
	update();
}

qreal ElementMeasureItem::lengthMM() const
{
	return QLineF(m_p1, m_p2).length() / 2.0;
}

qreal ElementMeasureItem::deltaXMM() const
{
	return qAbs(m_p2.x() - m_p1.x()) / 2.0;
}

qreal ElementMeasureItem::deltaYMM() const
{
	return qAbs(m_p2.y() - m_p1.y()) / 2.0;
}

qreal ElementMeasureItem::angleDeg() const
{
	return QLineF(m_p1, m_p2).angle();
}

void ElementMeasureItem::setFinished(bool finished)
{
	m_finished = finished;
	update();
}

QRectF ElementMeasureItem::boundingRect() const
{
	qreal minX = qMin(m_p1.x(), m_p2.x());
	qreal maxX = qMax(m_p1.x(), m_p2.x());
	qreal minY = qMin(m_p1.y(), m_p2.y());
	qreal maxY = qMax(m_p1.y(), m_p2.y());

	qreal margin = 50.0;
	return QRectF(minX - margin, minY - margin, (maxX - minX) + 2 * margin, (maxY - minY) + 2 * margin);
}

void ElementMeasureItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);

	QColor cadBlue(0, 120, 215);
	QColor projColor(100, 140, 180, 180);

	// 1. Draw dashed projection lines if diagonal
	qreal dx = qAbs(m_p2.x() - m_p1.x());
	qreal dy = qAbs(m_p2.y() - m_p1.y());
	if (dx > 4.0 && dy > 4.0) {
		QPen projPen(projColor, 1.0, Qt::DashLine);
		projPen.setCosmetic(true);
		painter->setPen(projPen);

		QPointF corner(m_p2.x(), m_p1.y());
		painter->drawLine(m_p1, corner);
		painter->drawLine(corner, m_p2);

		// Small projection labels
		QFont projFont = painter->font();
		projFont.setPointSize(7);
		painter->setFont(projFont);
		painter->setPen(projColor);

		QString dxStr = QStringLiteral("ΔX: %1 mm").arg(QString::number(deltaXMM(), 'f', 1).replace('.', ','));
		QPointF dxMid = (m_p1 + corner) / 2.0;
		painter->drawText(QRectF(dxMid.x() - 40, dxMid.y() - 15, 80, 14), Qt::AlignCenter, dxStr);

		QString dyStr = QStringLiteral("ΔY: %1 mm").arg(QString::number(deltaYMM(), 'f', 1).replace('.', ','));
		QPointF dyMid = (corner + m_p2) / 2.0;
		painter->drawText(QRectF(dyMid.x() + 5, dyMid.y() - 7, 80, 14), Qt::AlignLeft | Qt::AlignVCenter, dyStr);
	}

	// 2. Draw main dimension line
	QPen mainPen(cadBlue, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
	mainPen.setCosmetic(true);
	painter->setPen(mainPen);
	painter->drawLine(m_p1, m_p2);

	// 3. Draw endpoint ticks perpendicular to line
	QLineF line(m_p1, m_p2);
	qreal angle = line.angle();
	qreal radPerp = qDegreesToRadians(angle + 90.0);
	qreal tickLen = 6.0;
	QPointF perpOffset(tickLen * qCos(radPerp), -tickLen * qSin(radPerp));

	painter->drawLine(m_p1 - perpOffset, m_p1 + perpOffset);
	painter->drawLine(m_p2 - perpOffset, m_p2 + perpOffset);

	// Small endpoint anchor markers
	painter->setBrush(cadBlue);
	painter->drawEllipse(m_p1, 2.0, 2.0);
	painter->drawEllipse(m_p2, 2.0, 2.0);

	// 4. Draw dimension label badge at midpoint
	QString labelText = QStringLiteral("%1 mm").arg(QString::number(lengthMM(), 'f', 1).replace('.', ','));
	QFont badgeFont = painter->font();
	badgeFont.setPointSize(9);
	badgeFont.setBold(true);
	painter->setFont(badgeFont);

	QFontMetricsF fm(badgeFont);
	QRectF textRect = fm.boundingRect(labelText);
	qreal padH = 6.0;
	qreal padV = 3.0;
	QRectF badgeRect(0, 0, textRect.width() + 2 * padH, textRect.height() + 2 * padV);

	QPointF midPoint = (m_p1 + m_p2) / 2.0;
	badgeRect.moveCenter(midPoint);

	// Draw rounded badge box
	painter->setPen(QPen(cadBlue, 1.0));
	painter->setBrush(QColor(255, 255, 255, 235));
	painter->drawRoundedRect(badgeRect, 3.0, 3.0);

	// Draw badge text
	painter->setPen(QColor(20, 30, 45));
	painter->drawText(badgeRect, Qt::AlignCenter, labelText);

	painter->restore();
}
