// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef CONDUCTORCOLORCODES_H
#define CONDUCTORCOLORCODES_H

#include <QColor>
#include <QIcon>
#include <QList>
#include <QString>

class ConductorProperties;

/**
	@brief The ConductorColorCodes namespace
	The wire colour codes of IEC 60757 (BK, BN, RD, ... and GNYE for
	protective earth), each with the colour it is drawn in.

	A code goes in the wire's existing colour text (conductor_color), the
	field the wiring list exports, so nothing new is stored and an older
	QElectroTech keeps it. Anything else typed there stays free text.
*/
namespace ConductorColorCodes
{
	struct Code
	{
			/// The code as written on the drawing, in upper case
		QString code;
			/// English name, translated in the ConductorColorToolButton context
		const char *name;
			/// The colour the wire is drawn in
		QColor color;
			/// For a two-colour code, the stripes; invalid otherwise
		QColor color_2;

		bool isTwoColour() const { return color_2.isValid(); }
	};

	const QList<Code> &all();
	const Code *find(const QString &text);
	QString displayName(const Code &code);
	QIcon swatch(const Code &code);
	void apply(const Code &code, ConductorProperties &properties);
}

#endif // CONDUCTORCOLORCODES_H
