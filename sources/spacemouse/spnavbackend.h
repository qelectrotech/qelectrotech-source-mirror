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
#ifndef SPNAVBACKEND_H
#define SPNAVBACKEND_H

#include "spacemousebackend.h"

class QSocketNotifier;

/**
	@brief The SpnavBackend class
	Linux SpaceMouseBackend, via the spacenavd daemon and libspnav. Only
	compiled in when QET_SPACEMOUSE_BACKEND_SPNAV is defined (set by
	cmake/find_spacemouse.cmake once it has actually found libspnav).

	Connecting is always safe even when no daemon is running or no device
	is attached -- the expected state for the overwhelming majority of
	users, even of a build with 3D mouse support compiled in -- this
	silently leaves isAvailable() false rather than failing loudly.
*/
class SpnavBackend : public SpaceMouseBackend
{
	Q_OBJECT

	public:
		explicit SpnavBackend(QObject *parent = nullptr);
		~SpnavBackend() override;

		bool isAvailable() const override {return m_available;}

	private slots:
			/// Drain and emit every event currently queued on the spacenavd
			/// socket. Connected to a QSocketNotifier on that socket's fd
			/// rather than polled on a timer, so this is idle-cost-free
			/// between events.
		void readEvents();

	private:
		bool m_available = false;
		QSocketNotifier *m_notifier = nullptr;
};

#endif // SPNAVBACKEND_H
