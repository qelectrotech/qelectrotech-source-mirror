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
#ifndef QET_TEST_INKCONTRAST_H
#define QET_TEST_INKCONTRAST_H

#include <QHash>
#include <QImage>
#include <QRect>
#include <QString>
#include <QTest>
#include <QWidget>

#include "qetpalette.h"

/**
	Shared helpers for the rendering tests: paint a widget on the offscreen
	platform and measure the ink against the background.
*/
namespace QET {
	namespace Test {
		/**
			Grab a widget for measuring. With QET_TEST_DUMP_DIR set, the
			image is also written there as <row>-<name>.png so a failure
			can be looked at.
		*/
		inline QImage dump(const QImage &image, const char *name)
		{
			const QByteArray dir = qgetenv("QET_TEST_DUMP_DIR");
			if (!dir.isEmpty())
				image.save(QString("%1/%2-%3.png").arg(QString::fromLocal8Bit(dir),
				                                     QTest::currentDataTag(), name));
			return image;
		}

		inline QImage grab(QWidget *widget, const char *name)
		{
			return dump(widget->grab().toImage(), name);
		}

		/**
			Contrast between the background of a rendered widget (its most
			frequent color) and the ink drawn on it (the pixel whose
			luminance differs most from the background), inside rect.
		*/
		inline QHash<QRgb, int> histogram(const QImage &image, const QRect &rect)
		{
			QHash<QRgb, int> counts;
			for (int y = rect.top(); y <= rect.bottom(); ++y)
				for (int x = rect.left(); x <= rect.right(); ++x)
					++counts[image.pixel(x, y)];
			return counts;
		}

		/// The most frequent color inside rect.
		inline QRgb background(const QImage &image, const QRect &rect)
		{
			const QHash<QRgb, int> counts = histogram(image, rect);
			QRgb color = 0;
			int best = -1;
			for (auto it = counts.cbegin(); it != counts.cend(); ++it)
				if (it.value() > best) { best = it.value(); color = it.key(); }
			return color;
		}

		/// The color inside rect that contrasts most with the background: the ink.
		inline QRgb ink(const QImage &image, const QRect &rect)
		{
			const QHash<QRgb, int> counts = histogram(image, rect);
			const QColor bg(background(image, rect));
			QRgb color = bg.rgb();
			double contrast = 1.0;
			for (auto it = counts.cbegin(); it != counts.cend(); ++it)
			{
				const double c = QET::Palette::contrastRatio(bg, QColor(it.key()));
				if (c > contrast) { contrast = c; color = it.key(); }
			}
			return color;
		}

		inline double inkContrast(const QImage &image, const QRect &rect)
		{
			return QET::Palette::contrastRatio(QColor(background(image, rect)),
			                                   QColor(ink(image, rect)));
		}
	}
}

#endif
