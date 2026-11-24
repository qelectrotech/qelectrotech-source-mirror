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
#include "print/printdrawing.h"

#include <QBuffer>
#include <QGraphicsScene>
#include <QImage>
#include <QSvgGenerator>
#include <QSvgRenderer>
#include <QApplication>
#include <QtTest>

/**
	A folio printed or exported to SVG fits its page whatever the screen's
	resolution: 96 dpi on most screens, 72 on macOS (#1977). ctest runs
	this test twice, with QT_FONT_DPI=96 and QT_FONT_DPI=72.
*/
class TestPrintDrawing : public QObject
{
	Q_OBJECT

	private:
		static constexpr int folio_w = 1020;
		static constexpr int folio_h = 710;

		/// A folio filled black from edge to edge.
		static void fillScene(QGraphicsScene &scene)
		{
			scene.setSceneRect(0, 0, folio_w, folio_h);
			scene.addRect(0, 0, folio_w, folio_h, Qt::NoPen, Qt::black);
		}

		static bool isBlack(const QImage &image, int x, int y)
		{
			return qGray(image.pixel(x, y)) < 64;
		}

	private slots:
		void screenDpiIsTheOneAskedFor()
		{
			const QByteArray asked = qgetenv("QT_FONT_DPI");
			if (asked.isEmpty())
				QSKIP("run by ctest with QT_FONT_DPI set");
			QCOMPARE(QPicture().logicalDpiX(), asked.toInt());
		}

		void renderFitsAPrinterPage()
		{
			QGraphicsScene scene;
			fillScene(scene);

				//A page at 300 dpi, wider than the folio's shape: the
				//folio fills its height, from x = 0 to 1000 * 1020 / 710.
			QImage page(2000, 1000, QImage::Format_RGB32);
			page.fill(Qt::white);
			page.setDotsPerMeterX(qRound(300 / 0.0254));
			page.setDotsPerMeterY(qRound(300 / 0.0254));
			{
				QPainter painter(&page);
				QetPrint::render(&scene, &painter,
						 QRectF(0, 0, page.width(), page.height()),
						 QRectF(0, 0, folio_w, folio_h));
			}
			const int right = 1000 * folio_w / folio_h;	// 1436
			QVERIFY(isBlack(page, 5, 5));
			QVERIFY(isBlack(page, right - 5, page.height() - 5));
			QVERIFY(!isBlack(page, right + 5, page.height() / 2));
		}

		void svgShowsTheWholeFolio()
		{
				//The same steps as ExportDialog::generateSvg()
				//Only the bottom right corner is black: it is the
				//part a too small view box cuts off.
			QPicture picture;
			{
				QGraphicsScene scene;
				scene.setSceneRect(0, 0, folio_w, folio_h);
				scene.addRect(folio_w - 60, folio_h - 60, 60, 60,
					      Qt::NoPen, Qt::black);
				QPainter painter(&picture);
				scene.render(&painter, QRectF(0, 0, folio_w, folio_h));
			}
			QBuffer svg;
			svg.open(QIODevice::WriteOnly);
			{
				QSvgGenerator generator;
				generator.setSize(QSize(folio_w, folio_h));
				generator.setViewBox(QetPrint::svgViewBox(
					QSize(folio_w, folio_h), generator.resolution()));
				generator.setOutputDevice(&svg);
				QPainter painter(&generator);
				picture.play(&painter);
			}

			QSvgRenderer renderer(svg.data());
			QVERIFY(renderer.isValid());
			QImage image(folio_w / 2, folio_h / 2, QImage::Format_RGB32);
			image.fill(Qt::white);
			{
				QPainter painter(&image);
				renderer.render(&painter);
			}
			QVERIFY(!isBlack(image, 2, 2));
			QVERIFY(isBlack(image, image.width() - 3, image.height() - 3));
			QVERIFY(!isBlack(image, image.width() - 40, image.height() - 40));
		}

		void svgViewBoxUnchangedOnA96DpiScreen()
		{
			if (QPicture().logicalDpiX() != 96)
				QSKIP("only on a 96 dpi screen");
				//What QET wrote before #1977: 0.75 of the size
			QCOMPARE(QetPrint::svgViewBox(QSize(folio_w, folio_h), 72),
				 QRect(0, 0, int(folio_w * 0.75), int(folio_h * 0.75)));
		}
};

	//Not QTEST_MAIN: it sets Qt::AA_Use96Dpi, which hides the screen's
	//resolution this test is about.
int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	TestPrintDrawing test;
	return QTest::qExec(&test, argc, argv);
}
#include "tst_printdrawing.moc"
