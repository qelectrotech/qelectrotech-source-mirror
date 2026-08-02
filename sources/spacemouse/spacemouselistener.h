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

class SpaceMouseBackend;

/**
	@brief The SpaceMouseListener class
	https://github.com/qelectrotech/qelectrotech-source-mirror/discussions/599 :
	bridges a 3Dconnexion SpaceMouse/SpacePilot 6-DOF device to
	DiagramView's existing pan/zoom primitives (the same
	horizontalScrollBar()/verticalScrollBar()/zoom() calls
	DiagramView::wheelEvent() already uses for a physical wheel).

	Everything here is platform-independent: which DiagramView to apply
	motion to, the pan/zoom calls, and the Z-to-zoom-factor mapping.
	Talking to the actual device driver is a SpaceMouseBackend's job (see
	its class comment) -- this class owns one and applies whatever it
	reports, without knowing or caring which platform it came from.

	Only compiled in when QET_SPACEMOUSE_SUPPORT is defined. Even then,
	constructing one is always safe: if no backend is available for this
	platform, or the one that exists can't reach a driver/daemon (no
	device attached -- the expected state for the overwhelming majority of
	users, even of a build with the option on), this silently does nothing
	rather than failing or nagging the user. There is exactly one of
	these, owned by QETApp, because a physical 6-DOF device is a single
	ambient input source for the whole application, not something tied to
	one window.
*/
class SpaceMouseListener : public QObject
{
	Q_OBJECT

	public:
		explicit SpaceMouseListener(QObject *parent = nullptr);
		~SpaceMouseListener() override = default;

			/// True once the platform backend has a live connection to a
			/// driver/daemon. False is the common case, not an error -- see
			/// the class comment -- so callers should not warn the user
			/// when this is false.
		bool isAvailable() const;

			/// Pure translation from a device Z-axis delta to the
			/// multiplicative factor DiagramView::zoom() expects. A free
			/// function so the mapping can be unit-tested without a live
			/// backend connection or a real device.
		static qreal zoomFactorForZAxis(int z);

	private slots:
			/// Apply one motion sample -- from whichever backend is in use
			/// -- to whichever DiagramView is currently active.
		void applyMotion(int dx, int dy, int dz);

	private:
		SpaceMouseBackend *m_backend = nullptr;
};

#endif // SPACEMOUSELISTENER_H
