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
		inline QImage grab(QWidget *widget, const char *name)
		{
			const QImage image = widget->grab().toImage();
			const QByteArray dir = qgetenv("QET_TEST_DUMP_DIR");
			if (!dir.isEmpty())
				image.save(QString("%1/%2-%3.png").arg(QString::fromLocal8Bit(dir),
				                                     QTest::currentDataTag(), name));
			return image;
		}

		/**
			Contrast between the background of a rendered widget (its most
			frequent color) and the ink drawn on it (the pixel whose
			luminance differs most from the background), inside rect.
		*/
		inline double inkContrast(const QImage &image, const QRect &rect)
		{
			QHash<QRgb, int> histogram;
			for (int y = rect.top(); y <= rect.bottom(); ++y)
				for (int x = rect.left(); x <= rect.right(); ++x)
					++histogram[image.pixel(x, y)];

			QRgb background = 0;
			int best = -1;
			for (auto it = histogram.cbegin(); it != histogram.cend(); ++it)
				if (it.value() > best) { best = it.value(); background = it.key(); }

			double contrast = 1.0;
			for (auto it = histogram.cbegin(); it != histogram.cend(); ++it)
				contrast = qMax(contrast, QET::Palette::contrastRatio(QColor(background),
				                                                      QColor(it.key())));
			return contrast;
		}
	}
}

#endif
