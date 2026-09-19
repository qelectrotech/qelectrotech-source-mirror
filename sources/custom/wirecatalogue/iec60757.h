/*
	Copyright 2026 Trovo Tech Solutions
	This file is part of a custom feature set built on QElectroTech.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef IEC60757_H
#define IEC60757_H

#include <QString>
#include <QStringList>
#include <QColor>
#include <QIcon>

/**
	@brief IEC 60757 wire colour helpers.
	Maps standard conductor colour names (and their two-letter codes) to
	QColor, and builds little colour-swatch icons used throughout the wire
	catalogue UI. Lookup is case-insensitive and accepts full names ("Brown"),
	codes ("BN") and the bicolour green-yellow ("GN-YE", "GNYE").
*/
namespace Iec60757
{
	/// Ordered list of standard colour names offered in the pickers.
	QStringList standardNames();

	/// QColor for a name or code; invalid QColor() if unknown.
	QColor colorForName(const QString &name);

	/// True for the green-yellow protective-conductor bicolour.
	bool isGreenYellow(const QString &name);

	/// A rounded colour swatch pixmap (green-yellow drawn as a split swatch).
	QPixmap swatch(const QString &name, int size = 14);

	/// Convenience icon built from swatch().
	QIcon icon(const QString &name, int size = 14);

	/// Horizontal strip of swatches for a list of core colours.
	QPixmap swatchStrip(const QStringList &names, int size = 14);
}

#endif // IEC60757_H
