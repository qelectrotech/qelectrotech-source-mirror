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
#include "iec60757.h"

#include <QHash>
#include <QPainter>
#include <QPainterPath>

namespace {
	struct Entry { const char *name; const char *code; QColor color; };

	// IEC 60757 conductor colours. RGB values chosen to read well as swatches.
	const QVector<Entry> &table()
	{
		static const QVector<Entry> t = {
			{ "Black",        "BK",   QColor(0x1d, 0x1d, 0x1d) },
			{ "Brown",        "BN",   QColor(0x8b, 0x45, 0x13) },
			{ "Red",          "RD",   QColor(0xe6, 0x00, 0x00) },
			{ "Orange",       "OG",   QColor(0xff, 0x8c, 0x00) },
			{ "Yellow",       "YE",   QColor(0xff, 0xd5, 0x00) },
			{ "Green",        "GN",   QColor(0x00, 0xa6, 0x51) },
			{ "Blue",         "BU",   QColor(0x00, 0x66, 0xcc) },
			{ "Violet",       "VT",   QColor(0x7b, 0x2d, 0x8b) },
			{ "Grey",         "GY",   QColor(0x9e, 0x9e, 0x9e) },
			{ "White",        "WH",   QColor(0xf5, 0xf5, 0xf5) },
			{ "Pink",         "PK",   QColor(0xff, 0x80, 0xc0) },
			{ "Turquoise",    "TQ",   QColor(0x1b, 0xc9, 0xc9) },
			{ "Gold",         "GD",   QColor(0xd4, 0xaf, 0x37) },
			{ "Silver",       "SR",   QColor(0xc0, 0xc0, 0xc0) },
			{ "Green-Yellow", "GNYE", QColor(0x00, 0xa6, 0x51) }
		};
		return t;
	}

	QString normalise(const QString &s)
	{
		return s.trimmed().toUpper().remove(QLatin1Char('-')).remove(QLatin1Char(' '));
	}
}

namespace Iec60757 {

QStringList standardNames()
{
	QStringList out;
	for (const auto &e : table())
		out << QString::fromLatin1(e.name);
	return out;
}

bool isGreenYellow(const QString &name)
{
	const QString n = normalise(name);
	return n == QLatin1String("GNYE") || n == QLatin1String("GREENYELLOW");
}

QColor colorForName(const QString &name)
{
	const QString n = normalise(name);
	for (const auto &e : table()) {
		if (normalise(QString::fromLatin1(e.name)) == n
			|| QString::fromLatin1(e.code) == n)
			return e.color;
	}
	// Allow an explicit #rrggbb fallback so custom entries still render.
	QColor c(name.trimmed());
	return c.isValid() ? c : QColor();
}

QPixmap swatch(const QString &name, int size)
{
	QPixmap pm(size, size);
	pm.fill(Qt::transparent);

	QPainter p(&pm);
	p.setRenderHint(QPainter::Antialiasing, true);
	QPainterPath path;
	path.addRoundedRect(QRectF(0.5, 0.5, size - 1.0, size - 1.0), 3, 3);

	const QColor c = colorForName(name);
	if (!c.isValid()) {
		// Unknown colour: hollow swatch with a faint border.
		p.setPen(QColor(0xb0, 0xb0, 0xb0));
		p.setBrush(Qt::NoBrush);
		p.drawPath(path);
		return pm;
	}

	if (isGreenYellow(name)) {
		// Split diagonal green / yellow for the protective conductor.
		p.setClipPath(path);
		p.fillRect(pm.rect(), colorForName(QStringLiteral("Green")));
		QPainterPath tri;
		tri.moveTo(size, 0);
		tri.lineTo(size, size);
		tri.lineTo(0, size);
		tri.closeSubpath();
		p.fillPath(tri, colorForName(QStringLiteral("Yellow")));
		p.setClipping(false);
	} else {
		p.fillPath(path, c);
	}
	p.setPen(QColor(0x80, 0x80, 0x80));
	p.setBrush(Qt::NoBrush);
	p.drawPath(path);
	return pm;
}

QIcon icon(const QString &name, int size)
{
	return QIcon(swatch(name, size));
}

QPixmap swatchStrip(const QStringList &names, int size)
{
	if (names.isEmpty())
		return QPixmap();

	const int gap = 2;
	QPixmap pm(names.size() * size + (names.size() - 1) * gap, size);
	pm.fill(Qt::transparent);
	QPainter p(&pm);
	int x = 0;
	for (const QString &n : names) {
		p.drawPixmap(x, 0, swatch(n, size));
		x += size + gap;
	}
	return pm;
}

} // namespace Iec60757
