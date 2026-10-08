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
#ifndef CONNEXIONBACKEND_H
#define CONNEXIONBACKEND_H

#include "spacemousebackend.h"

#include <QLibrary>

/**
	@brief The ConnexionBackend class
	SpaceMouseBackend that reads the device through 3Dconnexion's own macOS
	driver, 3DxWare, the way Blender does. Only compiled in on macOS
	(cmake/find_spacemouse.cmake).

	3DxWare installs a driver extension that takes the device over, so
	with 3DxWare installed HidBackend can open the device but receives
	nothing (discussion #599). This backend asks 3DxWare for the motion
	instead. SpaceMouseListener tries it first and falls back to HidBackend
	when 3DxWare is not installed or not running, so the device works
	either way.

	The client library, 3DconnexionClient.framework, is loaded at run time
	from where 3DxWare installs it: nothing is linked, bundled or needed to
	build. The few declarations below are written here, as Blender does,
	rather than taken from 3Dconnexion's SDK, whose headers may not be
	redistributed.

	3DxWare delivers its messages on a thread of its own; they are passed
	to the main thread before any signal is emitted.
*/
class ConnexionBackend : public SpaceMouseBackend
{
	Q_OBJECT

	public:
		static const char DEFAULT_LIBRARY[];

		explicit ConnexionBackend(QObject *parent = nullptr,
					  const QString &library = QString::fromLatin1(DEFAULT_LIBRARY));
		~ConnexionBackend() override;

		bool isAvailable() const override { return m_client != 0; }

			/// 3DxWare's six axes, in its own order and signs, as QET's
			/// raw USB convention (+x right, +y towards the user, +z down).
		static SpaceMouseSample sampleFromAxes(const qint16 axis[6]);
			/// The 0-based buttons set in \a now but not in \a before.
		static QList<int> newlyPressed(quint32 before, quint32 now);

	private:
		static void onMessage(quint32 connection, quint32 type, void *argument);
		void handleState(quint16 client, quint16 command,
				 const qint16 axis[6], quint32 buttons);
		void shutdown();

		QLibrary m_library;
		quint16 m_client = 0;
		bool m_handlers_installed = false;
		quint32 m_buttons = 0;

		void (*m_cleanup)() = nullptr;
		void (*m_unregister)(quint16) = nullptr;
};

#endif // CONNEXIONBACKEND_H
