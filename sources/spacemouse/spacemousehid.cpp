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
#include "spacemousehid.h"

#include <QHash>

#include <algorithm>

namespace {
	constexpr unsigned short PAGE_GENERIC_DESKTOP = 0x01;
	constexpr unsigned short PAGE_BUTTON = 0x09;
	constexpr unsigned short USAGE_MULTI_AXIS = 0x08;
	constexpr unsigned short USAGE_X = 0x30;	// X, Y, Z, Rx, Ry, Rz follow

		//Newer devices (SpaceMouse Enterprise, the wireless ones) report the
		//buttons held as a list of 16-bit button numbers in this report,
		//outside the Button usage page.
	constexpr int REPORT_BUTTON_LIST = 0x1c;

		//spacenavd rescales absolute axes to this range, so doing the same
		//makes both backends hand QET the same numbers.
	constexpr int SCALED_MIN = -500;
	constexpr int SCALED_MAX = 500;

		//Limits on what a descriptor can make the parser walk.
	constexpr quint32 MAX_FIELDS = 1024;
	constexpr qint64 MAX_BITS = 1 << 20;

		//3D mice sold under Logitech's vendor id, for the ones whose
		//interface does not report a usage.
	const QSet<unsigned short> LOGITECH_3D_MICE = {
		0xc603, 0xc605, 0xc606, 0xc621, 0xc623, 0xc625,
		0xc626, 0xc627, 0xc628, 0xc629, 0xc62b
	};

		/// Read \a size bits at \a offset, little-endian as HID packs them.
	bool extract(const QByteArray &payload, int offset, int size, bool is_signed, int *value)
	{
		if (size <= 0 || size > 32 || offset < 0
			|| (offset + size + 7) / 8 > payload.size()) {
			return false;
		}
		quint32 raw = 0;
		for (int b = 0; b < size; ++b) {
			const int bit = offset + b;
			if (static_cast<quint8>(payload.at(bit / 8)) & (1u << (bit % 8))) {
				raw |= 1u << b;
			}
		}
		if (is_signed && size < 32 && (raw & (1u << (size - 1)))) {
			raw |= ~((1u << size) - 1);
		}
		*value = static_cast<qint32>(raw);
		return true;
	}

	int scaled(const SpaceMouseHid::Field &field, int value)
	{
		if (field.relative || field.logical_max <= field.logical_min) {
			return value;
		}
		const qint64 range = qint64(field.logical_max) - field.logical_min;
		return static_cast<int>((qint64(value) - field.logical_min)
					* (SCALED_MAX - SCALED_MIN) / range + SCALED_MIN);
	}
}

bool SpaceMouseHid::isSpaceMouse(unsigned short vendor, unsigned short product,
				 unsigned short usage_page, unsigned short usage)
{
	const bool multi_axis = usage_page == PAGE_GENERIC_DESKTOP && usage == USAGE_MULTI_AXIS;
	const bool usage_unknown = usage_page == 0 && usage == 0;

	if (vendor == VENDOR_3DCONNEXION) {
		return multi_axis || usage_unknown;
	}
	if (vendor == VENDOR_LOGITECH) {
		return multi_axis || (usage_unknown && LOGITECH_3D_MICE.contains(product));
	}
	return false;
}

bool SpaceMouseHid::Layout::hasAxes() const
{
	for (const Field &f : axes) {
		if (f.isValid()) {
			return true;
		}
	}
	return false;
}

/**
	@brief SpaceMouseHid::parseDescriptor
	Walks the short items of a HID report descriptor (HID 1.11, 6.2.2),
	keeping only what the decoder needs: the Input fields carrying the six
	Generic Desktop axes and the Button page.
	@param descriptor
	@return see the declaration
*/
SpaceMouseHid::Layout SpaceMouseHid::parseDescriptor(const QByteArray &descriptor)
{
	struct Globals {
		quint32 usage_page = 0;
		qint32 logical_min = 0;
		qint32 logical_max = 0;
		quint32 report_size = 0;
		quint32 report_count = 0;
		quint32 report_id = 0;
	};

	Layout layout;
	Globals globals;
	QList<Globals> stack;
	QList<quint32> usages;			// full 32-bit usages (page << 16 | id)
	quint32 usage_min = 0, usage_max = 0;
	bool have_range = false;
	QHash<quint32, int> next_bit;	// per report id

	auto clearLocals = [&]() {
		usages.clear();
		have_range = false;
		usage_min = usage_max = 0;
	};
	auto fullUsage = [&](quint32 value, int size) {
		return size == 4 ? value : (globals.usage_page << 16) | value;
	};

	int i = 0;
	const int n = descriptor.size();
	while (i < n)
	{
		const quint8 prefix = static_cast<quint8>(descriptor.at(i));
		if (prefix == 0xfe) {			// long item: never used by these devices
			if (i + 1 >= n) break;
			i += 3 + static_cast<quint8>(descriptor.at(i + 1));
			continue;
		}
		const int size = (prefix & 0x03) == 3 ? 4 : (prefix & 0x03);
		const int type = (prefix >> 2) & 0x03;
		const int tag = prefix >> 4;
		if (i + 1 + size > n) break;

		quint32 uvalue = 0;
		for (int b = 0; b < size; ++b) {
			uvalue |= quint32(static_cast<quint8>(descriptor.at(i + 1 + b))) << (8 * b);
		}
		qint32 svalue = static_cast<qint32>(uvalue);
		if (size > 0 && size < 4 && (uvalue & (1u << (8 * size - 1)))) {
			svalue = static_cast<qint32>(uvalue | ~((1u << (8 * size)) - 1));
		}
		i += 1 + size;

		if (type == 1)					// global
		{
			switch (tag) {
				case 0: globals.usage_page = uvalue; break;
				case 1: globals.logical_min = svalue; break;
				case 2: globals.logical_max = svalue; break;
				case 7: globals.report_size = uvalue; break;
				case 8: globals.report_id = uvalue; layout.numbered_reports = true; break;
				case 9: globals.report_count = uvalue; break;
				case 10: stack.append(globals); break;
				case 11: if (!stack.isEmpty()) globals = stack.takeLast(); break;
			}
		}
		else if (type == 2)				// local
		{
			switch (tag) {
				case 0: usages.append(fullUsage(uvalue, size)); break;
				case 1: usage_min = fullUsage(uvalue, size); have_range = true; break;
				case 2: usage_max = fullUsage(uvalue, size); have_range = true; break;
			}
		}
		else if (type == 0)				// main
		{
			if (tag == 8)				// Input
			{
				const bool constant = uvalue & 0x01;
				const bool variable = uvalue & 0x02;
				const bool relative = uvalue & 0x04;
				int &bit = next_bit[globals.report_id];
					//A real report is at most a few hundred bytes; a count
					//beyond that is a broken (or hostile) descriptor, and
					//walking it field by field would stall the application.
				const quint32 count = qMin<quint32>(globals.report_count, MAX_FIELDS);

				for (quint32 k = 0; k < count; ++k)
				{
					quint32 usage = 0;
					if (have_range && usage_max >= usage_min) {
						usage = qMin(usage_min + k, usage_max);
					} else if (!usages.isEmpty()) {
						usage = usages.at(qMin<int>(k, usages.size() - 1));
					}

					Field field;
					field.report_id = static_cast<int>(globals.report_id);
					field.bit_offset = bit + static_cast<int>(k * globals.report_size);
					field.bit_size = static_cast<int>(globals.report_size);
					field.logical_min = globals.logical_min;
					field.logical_max = globals.logical_max;
					field.relative = relative;

					if (!constant && variable) {
						const quint32 page = usage >> 16, id = usage & 0xffff;
						if (page == PAGE_GENERIC_DESKTOP && id >= USAGE_X && id < USAGE_X + 6) {
							layout.axes[id - USAGE_X] = field;
						} else if (page == PAGE_BUTTON && id >= 1 && id <= 64) {
							if (layout.buttons.size() < int(id)) {
								layout.buttons.resize(id);
							}
							layout.buttons[id - 1] = field;
						}
					}
				}
				bit = static_cast<int>(qMin<qint64>(
					bit + qint64(count) * globals.report_size, MAX_BITS));
			}
			clearLocals();		// every main item (Input, Collection, ...) ends the locals
		}
	}
	return layout;
}

SpaceMouseHid::Layout SpaceMouseHid::fallbackLayout()
{
	Layout layout;
	layout.numbered_reports = true;
	for (int a = 0; a < 6; ++a) {
		Field &f = layout.axes[a];
		f.report_id = a < 3 ? 1 : 2;
		f.bit_offset = (a % 3) * 16;
		f.bit_size = 16;
		f.logical_min = -32768;		// signed, and relative: passed through as sent
		f.logical_max = 32767;
		f.relative = true;
	}
	layout.buttons.resize(32);
	for (int b = 0; b < 32; ++b) {
		Field &f = layout.buttons[b];
		f.report_id = 3;
		f.bit_offset = b;
		f.bit_size = 1;
		f.logical_max = 1;
	}
	return layout;
}

SpaceMouseHid::Decoder::Decoder(const Layout &layout) :
	m_layout(layout)
{}

/**
	@brief SpaceMouseHid::Decoder::feed
	@param report : one report as read from the device, report id first
	when the device numbers its reports
	@return see Result
*/
SpaceMouseHid::Decoder::Result SpaceMouseHid::Decoder::feed(const QByteArray &report)
{
	Result result;
	if (report.isEmpty()) {
		return result;
	}

	const int id = m_layout.numbered_reports ? static_cast<quint8>(report.at(0)) : 0;
	const QByteArray payload = m_layout.numbered_reports ? report.mid(1) : report;

	for (int a = 0; a < 6; ++a)
	{
		const Field &f = m_layout.axes[a];
		int value = 0;
		if (f.isValid() && f.report_id == id
			&& extract(payload, f.bit_offset, f.bit_size, f.logical_min < 0, &value)) {
			m_axes[a] = scaled(f, value);
			result.motion = true;
		}
	}

		//Some devices send rotation after translation in one longer report
		//1, while their layout (or the fallback, which has no descriptor)
		//puts rotation in a report of its own: read it from report 1 too.
	if (id == 1 && payload.size() >= 12
		&& m_layout.axes[3].isValid() && m_layout.axes[3].report_id != 1) {
		for (int a = 3; a < 6; ++a) {
			const Field &f = m_layout.axes[a];
			int value = 0;
			if (extract(payload, 48 + f.bit_offset, f.bit_size, f.logical_min < 0, &value)) {
				m_axes[a] = scaled(f, value);
			}
		}
	}

	QSet<int> down;
	bool buttons_in_report = false;
	for (int b = 0; b < m_layout.buttons.size(); ++b)
	{
		const Field &f = m_layout.buttons.at(b);
		int value = 0;
		if (f.isValid() && f.report_id == id
			&& extract(payload, f.bit_offset, f.bit_size, false, &value)) {
			buttons_in_report = true;
			if (value) {
				down.insert(b);
			}
		}
	}
	if (!buttons_in_report && m_layout.numbered_reports && id == REPORT_BUTTON_LIST)
	{
		buttons_in_report = true;
		for (int offset = 0; offset + 1 < payload.size(); offset += 2) {
			const int number = static_cast<quint8>(payload.at(offset))
					 | static_cast<quint8>(payload.at(offset + 1)) << 8;
				//0 means no button, so the numbers start at 1: count from
				//0 like the Button page and spacenavd do.
			if (number) {
				down.insert(number - 1);
			}
		}
	}
	if (buttons_in_report)
	{
		for (int b : down) {
			if (!m_down.contains(b)) {
				result.pressed.append(b);
			}
		}
		std::sort(result.pressed.begin(), result.pressed.end());
		m_down = down;
	}

	if (result.motion) {
		result.sample.x = m_axes[0];
		result.sample.y = m_axes[1];
		result.sample.z = m_axes[2];
		result.sample.rx = m_axes[3];
		result.sample.ry = m_axes[4];
		result.sample.rz = m_axes[5];
	}
	return result;
}
