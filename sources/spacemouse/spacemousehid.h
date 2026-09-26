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
#ifndef SPACEMOUSEHID_H
#define SPACEMOUSEHID_H

#include "spacemousemotion.h"

#include <QByteArray>
#include <QList>
#include <QSet>
#include <QVector>

/**
	Decoding of a 3D mouse's raw USB HID reports, for the backends that read
	the device directly (HidBackend) rather than through a driver that has
	already decoded them (spacenavd). No I/O here, so it is unit-tested from
	recorded bytes (tests/qttest/tst_spacemousehid).
*/
namespace SpaceMouseHid
{
		/// USB vendor ids 3Dconnexion devices use: Logitech's for the older
		/// ones, 3Dconnexion's own for everything since about 2011.
	constexpr unsigned short VENDOR_LOGITECH = 0x046d;
	constexpr unsigned short VENDOR_3DCONNEXION = 0x256f;

		/// @return whether a HID interface is a 3D mouse: a 3Dconnexion
		/// vendor id and the Generic Desktop "multi-axis controller" usage.
		/// Logitech devices that do not report a usage are matched by
		/// product id instead, from the list of their 3D mice.
	bool isSpaceMouse(unsigned short vendor, unsigned short product,
			  unsigned short usage_page, unsigned short usage);

		/// Where one value sits in the reports, per the report descriptor.
	struct Field
	{
		int report_id = 0;		///< 0 when the device does not number its reports
		int bit_offset = 0;		///< after the report id byte, if any
		int bit_size = 0;
		int logical_min = 0;
		int logical_max = 0;
		bool relative = false;

		bool isValid() const { return bit_size > 0; }
	};

		/// What the report descriptor says about the six axes and the buttons.
	struct Layout
	{
		Field axes[6];				///< x, y, z, rx, ry, rz
		QVector<Field> buttons;		///< button n (0-based) is buttons[n]
		bool numbered_reports = false;

		bool hasAxes() const;
	};

		/// @return the layout described by \a descriptor, or one where
		/// hasAxes() is false if it describes no axes (or is empty)
	Layout parseDescriptor(const QByteArray &descriptor);

		/// @return the fixed layout of the classic 3Dconnexion reports
		/// (1 = translation, 2 = rotation, 3 = button bitmask), used when
		/// the descriptor cannot be read
	Layout fallbackLayout();

	/**
		@brief The Decoder class
		Turns a stream of raw reports into samples and button presses.
		A device can send translation and rotation in separate reports, so
		the decoder keeps the last value of every axis and reports all six
		each time any of them changes.
	*/
	class Decoder
	{
		public:
			struct Result
			{
				bool motion = false;		///< sample is new
				SpaceMouseSample sample;
				QList<int> pressed;			///< buttons pressed since the previous report
			};

			explicit Decoder(const Layout &layout = fallbackLayout());
			Result feed(const QByteArray &report);

		private:
			Layout m_layout;
			int m_axes[6] = {0, 0, 0, 0, 0, 0};
			QSet<int> m_down;			///< buttons currently held
	};
}

#endif // SPACEMOUSEHID_H
