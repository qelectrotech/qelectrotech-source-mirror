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
#include "diagramgestureoverlay.h"

#include <QAction>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QtMath>

namespace {
const int outer_radius = 84;
	/// Releasing closer than this to the centre cancels
const int inner_radius = 22;
const int icon_radius = 56;
const int icon_size = 22;
	/// Wider than the ring, so a long command name under it is not cut
const int overlay_width = 280;
}

/**
	@brief DiagramGestureOverlay::DiagramGestureOverlay
	@param viewport : the view's viewport, which it is drawn on. It lets the
	mouse through, so the view keeps receiving the drag.
*/
DiagramGestureOverlay::DiagramGestureOverlay(QWidget *viewport) :
	QWidget(viewport)
{
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setAttribute(Qt::WA_TranslucentBackground);
	setFixedSize(overlay_width, 2 * outer_radius + 2 + 24);
	hide();
}

/**
	@return whether right-drag gestures are on (a preference)
*/
bool DiagramGestureOverlay::isEnabled()
{
	return QSettings().value(QStringLiteral("diagrameditor/mouse_gestures"),
				 true).toBool();
}

/**
	@brief DiagramGestureOverlay::showAt
	Show the ring centred on @a center with @a actions, at most eight,
	clockwise from the top.
*/
void DiagramGestureOverlay::showAt(const QPoint &center,
				   const QList<QAction *> &actions)
{
	m_center = center;
	m_actions = actions.mid(0, sectors);
	m_active = -1;
	move(center - QPoint(overlay_width / 2, outer_radius + 1));
	show();
	raise();
}

/**
	@brief DiagramGestureOverlay::sectorAt
	@return the sector in the direction of @a viewport_pos from the centre,
	0 at the top then clockwise, or -1 within the cancel radius
*/
int DiagramGestureOverlay::sectorAt(const QPoint &viewport_pos) const
{
	const QPoint d = viewport_pos - m_center;
	if (qHypot(d.x(), d.y()) < inner_radius) {
		return -1;
	}
		//Angle clockwise from the top, in degrees
	qreal angle = qRadiansToDegrees(qAtan2(d.x(), -d.y()));
	if (angle < 0) {
		angle += 360;
	}
	return int(qRound(angle / (360.0 / sectors))) % sectors;
}

/**
	@return the command in the direction of @a viewport_pos, or nullptr when
	there is none there or it is disabled
*/
QAction *DiagramGestureOverlay::actionAt(const QPoint &viewport_pos) const
{
	const int sector = sectorAt(viewport_pos);
	if (sector < 0 || sector >= m_actions.count()) {
		return nullptr;
	}
	QAction *action = m_actions.at(sector);
	return action->isEnabled() ? action : nullptr;
}

void DiagramGestureOverlay::setPointer(const QPoint &viewport_pos)
{
	const int sector = sectorAt(viewport_pos);
	if (sector != m_active) {
		m_active = sector;
		update();
	}
}

void DiagramGestureOverlay::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	const QPointF c(overlay_width / 2, outer_radius + 1);
	const qreal step = 360.0 / sectors;

	QColor base = palette().color(QPalette::Window);
	base.setAlpha(225);
	painter.setPen(palette().color(QPalette::Mid));
	painter.setBrush(base);
	painter.drawEllipse(c, outer_radius, outer_radius);

		//Highlight the sector the mouse points at, if it holds a command
	if (m_active >= 0 && m_active < m_actions.count()
	    && m_actions.at(m_active)->isEnabled())
	{
		QPainterPath wedge;
		wedge.moveTo(c);
			//Qt angles run counter-clockwise from 3 o'clock
		const qreal start = 90 - m_active * step - step / 2;
		wedge.arcTo(QRectF(c.x() - outer_radius, c.y() - outer_radius,
				   2 * outer_radius, 2 * outer_radius), start, step);
		wedge.closeSubpath();
		painter.setPen(Qt::NoPen);
		painter.setBrush(palette().color(QPalette::Highlight));
		painter.drawPath(wedge);
	}

	painter.setPen(palette().color(QPalette::Mid));
	painter.setBrush(palette().color(QPalette::Window));
	painter.drawEllipse(c, inner_radius, inner_radius);

	for (int i = 0 ; i < m_actions.count() ; ++i)
	{
		QAction *action = m_actions.at(i);
		const qreal a = qDegreesToRadians(i * step);
		const QPointF p(c.x() + icon_radius * qSin(a), c.y() - icon_radius * qCos(a));
		const QRect r(int(p.x()) - icon_size / 2, int(p.y()) - icon_size / 2,
			      icon_size, icon_size);
		const QIcon::Mode mode = action->isEnabled() ? QIcon::Normal : QIcon::Disabled;
		if (!action->icon().isNull()) {
			action->icon().paint(&painter, r, Qt::AlignCenter, mode);
		} else {
			painter.setPen(palette().color(action->isEnabled() ? QPalette::Active : QPalette::Disabled,
						      QPalette::WindowText));
			painter.drawText(r.adjusted(-12, 0, 12, 0), Qt::AlignCenter,
					 action->text().remove(QLatin1Char('&')).left(3));
		}
	}

		//Name of the highlighted command, under the ring
	if (m_active >= 0 && m_active < m_actions.count())
	{
		const QString name = m_actions.at(m_active)->text().remove(QLatin1Char('&'));
		const QRect label(0, 2 * outer_radius + 4, width(), 20);
		QFont f = font();
		f.setBold(true);
		painter.setFont(f);
		const QRect text = painter.fontMetrics().boundingRect(label, Qt::AlignCenter, name)
				   .adjusted(-6, -2, 6, 2);
		painter.setPen(Qt::NoPen);
		painter.setBrush(base);
		painter.drawRoundedRect(text, 4, 4);
		painter.setPen(palette().color(QPalette::WindowText));
		painter.drawText(label, Qt::AlignCenter, name);
	}
}
