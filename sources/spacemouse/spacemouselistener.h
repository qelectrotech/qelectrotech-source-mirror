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
#ifndef SPACEMOUSELISTENER_H
#define SPACEMOUSELISTENER_H

#include <QObject>

class QSocketNotifier;
struct spnav_event_motion;

/**
	@brief The SpaceMouseListener class
	Phase 1 (Linux, libspnav) of
	https://github.com/qelectrotech/qelectrotech-source-mirror/discussions/599 :
	bridges a 3Dconnexion SpaceMouse/SpacePilot 6-DOF device, via the
	spacenavd daemon and libspnav, to DiagramView's existing pan/zoom
	primitives (the same horizontalScrollBar()/verticalScrollBar()/zoom()
	calls DiagramView::wheelEvent() already uses for a physical wheel).

	Only compiled in when the QET_ENABLE_SPACEMOUSE CMake option is on. Even
	then, constructing one is always safe: when no spacenavd is running or
	no device is attached -- the expected state for the overwhelming
	majority of users, even of a build with the option on -- it silently
	does nothing rather than failing or nagging the user. There is exactly
	one of these, owned by QETApp, because a physical 6-DOF device is a
	single ambient input source for the whole application, not something
	tied to one window; motion is applied to whichever DiagramView is
	currently active (see targetView()).
*/
class SpaceMouseListener : public QObject
{
	Q_OBJECT

	public:
		explicit SpaceMouseListener(QObject *parent = nullptr);
		~SpaceMouseListener() override;

			/// True once a connection to spacenavd was established.
			/// False is the common case, not an error -- see the class
			/// comment -- so callers should not warn the user when this
			/// is false.
		bool isAvailable() const {return m_available;}

			/// Pure translation from a device Z-axis delta to the
			/// multiplicative factor DiagramView::zoom() expects. A free
			/// function so the mapping can be unit-tested without a live
			/// spacenavd connection or a real device.
		static qreal zoomFactorForZAxis(int z);

	private slots:
			/// Drain and dispatch every event currently queued on the
			/// spacenavd socket. Connected to a QSocketNotifier on that
			/// socket's fd rather than polled on a timer, so this is
			/// idle-cost-free between events.
		void readEvents();

	private:
		void dispatchMotion(const spnav_event_motion &motion);

		bool m_available = false;
		QSocketNotifier *m_notifier = nullptr;
};

#endif // SPACEMOUSELISTENER_H
