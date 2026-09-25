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
#include "hidbackend.h"

#include <QTimer>

#include <hidapi.h>
#if defined(Q_OS_MACOS) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
#	include <hidapi_darwin.h>
#endif

namespace {
	constexpr int SCAN_INTERVAL_MS = 3000;
	constexpr int FAST_POLL_MS = 8;		// devices report at up to ~125 Hz
	constexpr int SLOW_POLL_MS = 50;
		//Fast polls in a row with nothing to read before slowing down.
	constexpr int IDLE_POLLS_BEFORE_SLOW = 125;
	constexpr int MAX_REPORT_SIZE = 64;
	constexpr int MAX_DESCRIPTOR_SIZE = 4096;
}

/**
	@brief HidBackend::HidBackend
	@param parent
*/
HidBackend::HidBackend(QObject *parent) :
	SpaceMouseBackend(parent)
{
	if (hid_init() != 0) {
		return;
	}
	m_hid_initialised = true;
#if defined(Q_OS_MACOS) && HID_API_VERSION >= HID_API_MAKE_VERSION(0, 12, 0)
		//hidapi opens devices exclusively on macOS by default, which fails
		//while 3DxWare has the device open (discussion #599).
	hid_darwin_set_open_exclusive(0);
#endif

	m_read_timer = new QTimer(this);
	connect(m_read_timer, &QTimer::timeout, this, &HidBackend::readReports);

	m_scan_timer = new QTimer(this);
	m_scan_timer->setInterval(SCAN_INTERVAL_MS);
	connect(m_scan_timer, &QTimer::timeout, this, &HidBackend::scan);

	scan();
}

/**
	@brief HidBackend::~HidBackend
*/
HidBackend::~HidBackend()
{
	close();
	if (m_hid_initialised) {
		hid_exit();
	}
}

/**
	@brief HidBackend::scan
	Open the first 3D mouse found, or keep looking every few seconds.
*/
void HidBackend::scan()
{
	if (m_device) {
		return;
	}

	hid_device_info *list = hid_enumerate(0, 0);
	for (hid_device_info *info = list; info && !m_device; info = info->next)
	{
		if (!SpaceMouseHid::isSpaceMouse(info->vendor_id, info->product_id,
						 info->usage_page, info->usage)) {
			continue;
		}
		m_device = hid_open_path(info->path);
	}
	hid_free_enumeration(list);

	if (!m_device) {
		m_scan_timer->start();
		return;
	}
	m_scan_timer->stop();
	hid_set_nonblocking(m_device, 1);

	SpaceMouseHid::Layout layout;
#if HID_API_VERSION >= HID_API_MAKE_VERSION(0, 14, 0)
	unsigned char descriptor[MAX_DESCRIPTOR_SIZE];
	const int size = hid_get_report_descriptor(m_device, descriptor, sizeof descriptor);
	if (size > 0) {
		layout = SpaceMouseHid::parseDescriptor(
			QByteArray(reinterpret_cast<const char *>(descriptor), size));
	}
#endif
	if (!layout.hasAxes()) {
		layout = SpaceMouseHid::fallbackLayout();
	}
	m_decoder = SpaceMouseHid::Decoder(layout);

	m_idle_polls = 0;
	m_read_timer->start(FAST_POLL_MS);
}

/**
	@brief HidBackend::readReports
	Drain every report waiting, emit what they mean, and adjust the polling
	rate. A read error means the device went away (unplugged, or the
	wireless receiver lost it): go back to looking for one.
*/
void HidBackend::readReports()
{
		//A button can trigger an action that opens a dialog, whose event
		//loop fires this timer again before the signal returns. That is
		//fine -- the device keeps working in the dialog -- because the
		//signals are emitted last, after every use of m_device below; this
		//guard only keeps two reads from ever overlapping.
	if (m_reading || !m_device) {
		return;
	}
	m_reading = true;

	unsigned char buffer[MAX_REPORT_SIZE];
	bool got_any = false;
		//Everything read in one poll is one moment: a device that sends
		//translation and rotation as two reports, or two samples that
		//queued up, gives one sample here -- the latest state -- so the
		//listener's time scaling sees one sample per real interval.
	bool moved = false;
	SpaceMouseSample latest;
	QList<int> pressed;

	for (;;)
	{
		const int size = hid_read(m_device, buffer, sizeof buffer);
		if (size == 0) {
			break;
		}
		if (size < 0) {
			close();
			m_scan_timer->start();
			break;
		}
		got_any = true;

		const SpaceMouseHid::Decoder::Result result = m_decoder.feed(
			QByteArray(reinterpret_cast<const char *>(buffer), size));
		if (result.motion) {
			moved = true;
			latest = result.sample;
		}
		pressed += result.pressed;
	}

	if (m_device) {
		if (got_any) {
			m_idle_polls = 0;
			if (m_read_timer->interval() != FAST_POLL_MS) {
				m_read_timer->setInterval(FAST_POLL_MS);
			}
		} else if (++m_idle_polls == IDLE_POLLS_BEFORE_SLOW) {
			m_read_timer->setInterval(SLOW_POLL_MS);
		}
	}

		//Signals last: whatever they trigger (even the device going away
		//during a dialog), nothing after them touches the device.
	m_reading = false;
	if (moved) {
		emit motion(latest);
	}
	for (int button : pressed) {
		emit buttonPressed(button);
	}
}

/**
	@brief HidBackend::close
*/
void HidBackend::close()
{
	if (m_read_timer) {
		m_read_timer->stop();
	}
	if (m_device) {
		hid_close(m_device);
		m_device = nullptr;
	}
}
