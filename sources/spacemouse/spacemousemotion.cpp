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
#include "spacemousemotion.h"

#include <QSettings>
#include <QtMath>

namespace {
		//A device deflection is an integer on roughly the same order of
		//magnitude as a QWheelEvent::angleDelta() tick (about +-120 per
		//detent, more under a hard push/twist -- exact range depends on the
		//backend and the driver's own sensitivity). DiagramView::wheelEvent()
		//turns such a tick into a zoom step of about 1 + value/1000, reused
		//here as a starting point.
		//
		//Neither constant has been calibrated against real hardware; the
		//user-facing speed settings scale both.
	constexpr qreal ZOOM_DIVISOR = 1000.0;
	constexpr qreal PAN_SCALE = 1.0;

	const QString GROUP = QStringLiteral("spacemouse/motion/");
}

/**
	@brief SpaceMouseSettings::load
	@return the settings saved by the configuration page, or the defaults
*/
SpaceMouseSettings SpaceMouseSettings::load()
{
	QSettings settings;
	SpaceMouseSettings s;
	s.pan_speed    = settings.value(GROUP + "pan_speed", s.pan_speed).toInt();
	s.zoom_speed   = settings.value(GROUP + "zoom_speed", s.zoom_speed).toInt();
	s.dead_zone    = settings.value(GROUP + "dead_zone", s.dead_zone).toInt();
	s.invert_pan_x = settings.value(GROUP + "invert_pan_x", s.invert_pan_x).toBool();
	s.invert_pan_y = settings.value(GROUP + "invert_pan_y", s.invert_pan_y).toBool();
	s.invert_zoom  = settings.value(GROUP + "invert_zoom", s.invert_zoom).toBool();
	s.zoom_axis = settings.value(GROUP + "zoom_axis").toString() == QLatin1String("twist")
			? ZoomAxis::Twist
			: ZoomAxis::PushPull;
	return s;
}

/**
	@brief SpaceMouseSettings::save
*/
void SpaceMouseSettings::save() const
{
	QSettings settings;
	settings.setValue(GROUP + "pan_speed", pan_speed);
	settings.setValue(GROUP + "zoom_speed", zoom_speed);
	settings.setValue(GROUP + "dead_zone", dead_zone);
	settings.setValue(GROUP + "invert_pan_x", invert_pan_x);
	settings.setValue(GROUP + "invert_pan_y", invert_pan_y);
	settings.setValue(GROUP + "invert_zoom", invert_zoom);
	settings.setValue(GROUP + "zoom_axis",
			  zoom_axis == ZoomAxis::Twist ? QStringLiteral("twist")
						       : QStringLiteral("push_pull"));
}

bool SpaceMouseSettings::operator==(const SpaceMouseSettings &other) const
{
	return pan_speed == other.pan_speed
		&& zoom_speed == other.zoom_speed
		&& dead_zone == other.dead_zone
		&& invert_pan_x == other.invert_pan_x
		&& invert_pan_y == other.invert_pan_y
		&& invert_zoom == other.invert_zoom
		&& zoom_axis == other.zoom_axis;
}

/**
	@brief SpaceMouseMotion::stepFor
	@param elapsed_ms
	@return see the declaration
*/
qreal SpaceMouseMotion::stepFor(qint64 elapsed_ms)
{
	if (elapsed_ms < 0 || elapsed_ms > MAX_PERIOD_MS) {
		return 1.0;
	}
	return elapsed_ms / NOMINAL_PERIOD_MS;
}

/**
	@brief SpaceMouseMotion::applyDeadZone
	@param value
	@param dead_zone
	@return see the declaration
*/
int SpaceMouseMotion::applyDeadZone(int value, int dead_zone)
{
	if (dead_zone <= 0) {
		return value;
	}
	if (qAbs(value) <= dead_zone) {
		return 0;
	}
	return value > 0 ? value - dead_zone : value + dead_zone;
}

/**
	@brief SpaceMouseMotion::map
	Pan is a scrollbar delta: pushing the cap right moves the drawing right,
	so the scrollbar goes the other way, as in DiagramView::wheelEvent().
	Zoom is exponential in the deflection, so the factor is always positive
	and an equal push and pull cancel out exactly.
	@param sample
	@param elapsed_ms
	@param settings
	@return see the declaration
*/
SpaceMouseViewMotion SpaceMouseMotion::map(const SpaceMouseSample &sample,
					   qint64 elapsed_ms,
					   const SpaceMouseSettings &settings)
{
	const qreal step = stepFor(elapsed_ms);
	const int x = applyDeadZone(sample.x, settings.dead_zone);
	const int y = applyDeadZone(sample.y, settings.dead_zone);
	const int zoom_value = applyDeadZone(
		settings.zoom_axis == SpaceMouseSettings::ZoomAxis::Twist ? sample.rz : sample.z,
		settings.dead_zone);

	const qreal pan = PAN_SCALE * settings.pan_speed / 100.0 * step;
	const qreal zoom = settings.zoom_speed / 100.0 / ZOOM_DIVISOR * step;

	SpaceMouseViewMotion motion;
	motion.scroll_x = -x * pan * (settings.invert_pan_x ? -1 : 1);
	motion.scroll_y = -y * pan * (settings.invert_pan_y ? -1 : 1);
	motion.zoom_factor = qExp(zoom_value * zoom * (settings.invert_zoom ? -1 : 1));
	return motion;
}
