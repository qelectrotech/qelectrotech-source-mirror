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
#ifndef HIDBACKEND_H
#define HIDBACKEND_H

#include "spacemousebackend.h"
#include "spacemousehid.h"

class QTimer;
struct hid_device_;

/**
	@brief The HidBackend class
	SpaceMouseBackend that reads the device directly over USB through
	hidapi, with no 3Dconnexion driver or SDK: the backend for Windows and
	macOS, and for Linux without spacenavd. Only compiled in when
	QET_SPACEMOUSE_BACKEND_HID is defined (cmake/find_spacemouse.cmake).

	The raw reports are decoded by SpaceMouseHid, from the device's own
	report descriptor, into the same values spacenavd would give.

	hidapi has no event to wait on, so the device is polled from the main
	thread: fast while it is moving, slowly once it has been still for a
	moment. With no device, it looks for one every few seconds, so plugging
	one in works without restarting QElectroTech. As with every backend,
	no device is the normal case and is never reported as an error.
*/
class HidBackend : public SpaceMouseBackend
{
	Q_OBJECT

	public:
		explicit HidBackend(QObject *parent = nullptr);
		~HidBackend() override;

		bool isAvailable() const override { return m_device != nullptr; }

	private slots:
		void scan();
		void readReports();

	private:
		void close();

		bool m_hid_initialised = false;
		hid_device_ *m_device = nullptr;
		SpaceMouseHid::Decoder m_decoder;
		QTimer *m_scan_timer = nullptr;
		QTimer *m_read_timer = nullptr;
		int m_idle_polls = 0;
		bool m_reading = false;
};

#endif // HIDBACKEND_H
