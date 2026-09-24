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
#ifndef SPACEMOUSEMOTION_H
#define SPACEMOUSEMOTION_H

#include <QtGlobal>

/**
	@brief The SpaceMouseSample struct
	One raw motion sample from a SpaceMouseBackend: the device's current
	deflection on its six axes, in whatever units the backend's driver
	reports. With spacenavd's default configuration x is left/right, y is
	forward/back, z is push/pull, and rz is the twist of the cap.
*/
struct SpaceMouseSample
{
	int x = 0, y = 0, z = 0;
	int rx = 0, ry = 0, rz = 0;
};

/**
	@brief The SpaceMouseSettings struct
	The user's 3D mouse motion preferences (Configuration > Souris 3D).
	The defaults reproduce the behaviour before these settings existed.
*/
struct SpaceMouseSettings
{
	enum class ZoomAxis { PushPull, Twist };

	int pan_speed = 100;	///< percent
	int zoom_speed = 100;	///< percent
	int dead_zone = 0;		///< raw device units, applied to every axis used
	bool invert_pan_x = false;
	bool invert_pan_y = false;
	bool invert_zoom = false;
	ZoomAxis zoom_axis = ZoomAxis::PushPull;

	static SpaceMouseSettings load();
	void save() const;

	bool operator==(const SpaceMouseSettings &other) const;
};

/**
	@brief The SpaceMouseViewMotion struct
	What one sample does to a view: a scrollbar delta in pixels, and a
	multiplicative zoom factor (1.0 = no zoom).
*/
struct SpaceMouseViewMotion
{
	qreal scroll_x = 0;
	qreal scroll_y = 0;
	qreal zoom_factor = 1.0;
};

/**
	Pure translation from device samples to view motion, kept free of any
	widget or device so it can be unit-tested (tests/qttest/tst_spacemousemotion).
*/
namespace SpaceMouseMotion
{
		/// Sample period the speeds are calibrated for: about what
		/// spacenavd delivers. A device sampled twice as often moves the
		/// view by half as much per sample, so the speed on screen does not
		/// depend on the event rate of the platform or driver.
	constexpr qreal NOMINAL_PERIOD_MS = 16.0;

		/// A gap longer than this means the device was at rest before this
		/// sample, not that one sample covers the whole gap.
	constexpr qint64 MAX_PERIOD_MS = 100;

		/// @return how many nominal periods a sample covers, given the
		/// milliseconds since the previous one (negative = no previous one)
	qreal stepFor(qint64 elapsed_ms);

		/// @return \a value with \a dead_zone removed: 0 inside it, and
		/// shifted towards 0 outside it so motion starts smoothly at the edge
	int applyDeadZone(int value, int dead_zone);

		/// @return what \a sample does to the view, \a elapsed_ms after the
		/// previous sample, under \a settings
	SpaceMouseViewMotion map(const SpaceMouseSample &sample,
				 qint64 elapsed_ms,
				 const SpaceMouseSettings &settings);
}

#endif // SPACEMOUSEMOTION_H
