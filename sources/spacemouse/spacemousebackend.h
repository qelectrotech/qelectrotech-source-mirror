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

#include "spacemousemotion.h"

#include <QObject>

/**
	@brief The SpaceMouseBackend class
	Platform seam for discussion #599's 3D mouse support. A backend owns
	one platform's connection to the actual 6-DOF device driver -- opening
	it, pumping whatever event source that platform uses, closing it -- and
	reports motion through the signals below. Everything platform-
	independent (which view to apply motion to, the pan/zoom primitives to
	call, the sample-to-motion mapping) lives in SpaceMouseListener and
	SpaceMouseMotion instead, once, so it does not have to be duplicated
	or re-verified per backend.

	Two implementations, chosen at build time (cmake/find_spacemouse.cmake):
	SpnavBackend (Linux, through spacenavd/libspnav) and HidBackend (any
	platform, directly over USB through hidapi, with no 3Dconnexion driver
	or SDK). On macOS a third, ConnexionBackend, reads through 3DxWare and
	is tried first at run time. All report the same values for the same
	movement.
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
			/// One raw device sample, all six axes. Units and range are
			/// whatever the backend's own driver reports --
			/// SpaceMouseMotion::map() and the user's settings are what
			/// turn them into pixels and a zoom factor, not this signal.
		void motion(const SpaceMouseSample &sample);

			/// One device button was pressed. \a button is whatever index
			/// the backend's own driver numbers it as -- there is no
			/// portable numbering across device models, which is exactly
			/// why the binding from a button to a QET action
			/// (SpaceMouseButtonMap) is user-configurable rather than
			/// hardcoded. Emitted on press only; release is not reported,
			/// since nothing here has a use for it.
		void buttonPressed(int button);
};

#endif // SPACEMOUSEBACKEND_H
