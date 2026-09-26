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
#ifndef DIAGRAMSORTKEYS_H
#define DIAGRAMSORTKEYS_H

#include <QPointF>
#include <QString>
#include <QtGlobal>

/// Sort-key helpers used by Diagram::toXml() to serialize elements and
/// conductors in a deterministic order. Pulled into their own header
/// (instead of an anonymous namespace in diagram.cpp) so they can be
/// unit-tested directly -- see tests/qttest/tst_diagramsortkeys.cpp.
namespace DiagramSortKeys {

	/// Format a coordinate as a string that sorts the same way the number
	/// does. Plain fixed-precision formatting ("%.4f") does NOT do this --
	/// e.g. "15.0000" sorts before "5.0000" as text even though 15 > 5 --
	/// so shift into a non-negative range and zero-pad to a fixed width
	/// before comparing.
	inline QString coordinateKey(double v)
	{
			//Diagram coordinates are nowhere near this range; the offset and
			//width just need to be big enough that shifted values are always
			//non-negative and always the same digit count.
		constexpr double offset = 1e9;
		qint64 scaled = qint64(qRound64((v + offset) * 10000.0));
		return QStringLiteral("%1").arg(scaled, 20, 10, QLatin1Char('0'));
	}

	inline QString positionKey(const QPointF &pos)
	{
		return coordinateKey(pos.x()) + QLatin1Char('|') + coordinateKey(pos.y());
	}

}

#endif
