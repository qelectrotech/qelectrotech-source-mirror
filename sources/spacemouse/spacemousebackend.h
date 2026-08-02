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
#ifndef SPACEMOUSEBACKEND_H
#define SPACEMOUSEBACKEND_H

#include <QObject>

/**
	@brief The SpaceMouseBackend class
	Platform seam for discussion #599's 3D mouse support. A backend owns
	one platform's connection to the actual 6-DOF device driver -- opening
	it, pumping whatever event source that platform uses, closing it -- and
	reports motion through the one signal below. Everything platform-
	independent (which DiagramView to apply motion to, the pan/zoom
	primitives to call, the Z-to-zoom-factor mapping) lives in
	SpaceMouseListener instead, once, so it does not have to be duplicated
	or re-verified per backend.

	SpnavBackend (Linux, spacenavd/libspnav) is the only implementation so
	far -- built, linked, and its "no daemon/device present" path actually
	run in the environment this was written in. A Windows/macOS backend
	(3Dconnexion's proprietary 3DxWare SDK) would implement this same
	interface and be selected in SpaceMouseListener's constructor, without
	changing SpnavBackend or anything downstream of the motion signal.
	Deliberately not attempted here: this was written on Linux with no
	3DxWare SDK and no Windows/macOS toolchain available to compile,
	link, or run a single line of it against, and shipping platform code
	that has never even built would be a materially different, weaker
	thing than everything else in this feature.
*/
class SpaceMouseBackend : public QObject
{
	Q_OBJECT

	public:
		explicit SpaceMouseBackend(QObject *parent = nullptr) : QObject(parent) {}
		~SpaceMouseBackend() override = default;

			/// True once this backend actually has a live connection to a
			/// driver/daemon. False is the ordinary case -- no daemon
			/// running, no device attached -- not an error; see
			/// SpaceMouseListener's class comment for why that distinction
			/// matters.
		virtual bool isAvailable() const = 0;

	signals:
			/// One raw device sample. dx/dy are the two axes
			/// SpaceMouseListener maps to horizontal/vertical pan, dz the
			/// one it maps to zoom. Units and range are whatever the
			/// backend's own driver reports -- SpaceMouseListener's own
			/// scale/divisor constants are what turn them into pixels and a
			/// zoom factor, not this signal.
		void motion(int dx, int dy, int dz);
};

#endif // SPACEMOUSEBACKEND_H
