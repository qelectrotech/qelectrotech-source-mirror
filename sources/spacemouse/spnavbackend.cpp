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
#include "spnavbackend.h"

#include <QSocketNotifier>

#include <spnav.h>

/**
	@brief SpnavBackend::SpnavBackend
	Try to connect to spacenavd. Failure -- no daemon running, no device
	attached -- is left silent: it is the expected state for most users and
	must never surface as an error dialog or a log warning on every
	ordinary startup.
	@param parent
*/
SpnavBackend::SpnavBackend(QObject *parent) :
	SpaceMouseBackend(parent)
{
	if (spnav_open() == -1) {
		return;
	}

	m_available = true;
	m_notifier = new QSocketNotifier(spnav_fd(), QSocketNotifier::Read, this);
	connect(m_notifier, &QSocketNotifier::activated,
		this, &SpnavBackend::readEvents);
}

/**
	@brief SpnavBackend::~SpnavBackend
*/
SpnavBackend::~SpnavBackend()
{
	if (m_available) {
		spnav_close();
	}
}

/**
	@brief SpnavBackend::readEvents
	Called when the spacenavd socket has data available. Drains every event
	currently queued -- spnav_poll_event() returns 0 once the queue is
	empty -- rather than handling just one per activation, so events cannot
	silently back up if several arrive between two Qt event loop turns.
*/
void SpnavBackend::readEvents()
{
	spnav_event event;
	while (spnav_poll_event(&event))
	{
		if (event.type == SPNAV_EVENT_MOTION) {
				//A 6-DOF device also reports rotation (rx, ry, rz); QET's
				//view has nothing rotation maps to, so those three axes are
				//read by nothing here.
			emit motion(event.motion.x, event.motion.y, event.motion.z);
		}
		else if (event.type == SPNAV_EVENT_BUTTON && event.button.press) {
				//Release (press == 0) is not reported -- SpaceMouseListener
				//triggers a QET action on press, the same way a keyboard
				//shortcut triggers on key-down, and has no use for release.
			emit buttonPressed(event.button.bnum);
		}
	}
}
