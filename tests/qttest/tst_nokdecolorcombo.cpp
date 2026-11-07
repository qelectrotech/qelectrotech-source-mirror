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
#include "kcolorcombo.h"

#include <QApplication>
#include <QIcon>
#include <QImage>
#include <QStyle>
#include <QStyleFactory>
#include <QtTest>

#include <memory>
#include <utility>

/*
	Built without KDE Frameworks, QElectroTech has a KColorCombo of its
	own (sources/ui/nokde), used for the colours of bridges in the
	terminal strip editor. Each colour in the list shows a square of the
	colour beside its name, and the closed list shows the chosen one.
*/
class tst_nokdecolorcombo : public QObject
{
	Q_OBJECT

		//The bridge colours of the terminal strip editor, and a colour
		//of no style
	static QList<QColor> colors()
	{
		return {Qt::red, Qt::blue, Qt::white, Qt::darkGray, Qt::black,
			QColor(0x12, 0x9a, 0x3c)};
	}

		//How many pixels of @p image are @p color
	static int pixelsOf(const QImage &image, const QColor &color)
	{
		int count = 0;
		for (int y = 0; y < image.height(); ++y)
			for (int x = 0; x < image.width(); ++x)
				if (QColor(image.pixel(x, y)) == color)
					++count;
		return count;
	}

		//The middle of @p icon, @p size large at @p scale
	static QColor iconMiddle(const QIcon &icon, const QSize &size,
				 qreal scale, QIcon::Mode mode = QIcon::Normal,
				 QIcon::State state = QIcon::Off)
	{
		const QImage image
			= icon.pixmap(size, scale, mode, state).toImage();
		return QColor(image.pixel(image.width() / 2,
					  image.height() / 2));
	}

	private slots:
		void eachColourHasItsSquare()
		{
			KColorCombo combo;
			combo.setColors(colors());
			QCOMPARE(combo.count(), colors().size());
			for (int i = 0; i < combo.count(); ++i) {
				const QColor color = colors().at(i);
				QCOMPARE(combo.itemText(i), color.name());
				QCOMPARE(combo.color(i), color);
				const QIcon icon = combo.itemIcon(i);
				QVERIFY(!icon.isNull());
				const QSize size = combo.iconSize();
				QCOMPARE(iconMiddle(icon, size, 1), color);
				QCOMPARE(iconMiddle(icon, size, 2), color);
			}
		}

			//Pointed at, chosen, or the current colour in the open
			//list, which styles draw as checked: a colour keeps its
			//square as it is, in every style
		void chosenColourKeepsItsSquare_data()
		{
			QTest::addColumn<QString>("style");
			QTest::addColumn<int>("mode");
			QTest::addColumn<int>("state");
			const QList<std::pair<const char *, QIcon::Mode>> modes{
				{"pointed at", QIcon::Active},
				{"chosen", QIcon::Selected}};
			const QList<std::pair<const char *, QIcon::State>> states{
				{"", QIcon::Off}, {", current", QIcon::On}};
			for (const QString &style : QStyleFactory::keys())
				for (const auto &mode : modes)
					for (const auto &state : states)
						QTest::addRow("%s %s%s",
							qPrintable(style),
							mode.first, state.first)
							<< style << int(mode.second)
							<< int(state.second);
		}
		void chosenColourKeepsItsSquare()
		{
			QFETCH(QString, style);
			QFETCH(int, mode);
			QFETCH(int, state);
				//Icons are drawn in the application's style
			QApplication::setStyle(QStyleFactory::create(style));
			KColorCombo combo;
			combo.setColors(colors());
			for (int i = 0; i < combo.count(); ++i)
				QCOMPARE(iconMiddle(combo.itemIcon(i),
					combo.iconSize(), 1, QIcon::Mode(mode),
					QIcon::State(state)),
					colors().at(i));
		}

			//The edge stands out from both white and black
		void squareHasAnEdge_data()
		{
			QTest::addColumn<QColor>("color");
			QTest::newRow("white") << QColor(Qt::white);
			QTest::newRow("black") << QColor(Qt::black);
		}
		void squareHasAnEdge()
		{
			QFETCH(QColor, color);
			KColorCombo combo;
			combo.setColors({color});
			QVERIFY(!combo.itemIcon(0).isNull());
			const QImage image = combo.itemIcon(0)
				.pixmap(combo.iconSize(), 1).toImage();
			QVERIFY(QColor(image.pixel(0, 0)) != color);
			QVERIFY(QColor(image.pixel(image.width() - 1,
				image.height() - 1)) != color);
		}

		void colourNotInTheListIsAddedWithItsSquare()
		{
			KColorCombo combo;
			combo.setColors(colors());
			const QColor other(0xff, 0x80, 0x00);
			combo.setColor(other);
			QCOMPARE(combo.count(), colors().size() + 1);
			QCOMPARE(combo.color(), other);
			const QIcon icon = combo.itemIcon(combo.currentIndex());
			QCOMPARE(iconMiddle(icon, combo.iconSize(), 1), other);

			combo.setColor(Qt::blue);
			QCOMPARE(combo.count(), colors().size() + 1);
			QCOMPARE(combo.color(), QColor(Qt::blue));
		}

			//Closed, the list shows the chosen colour in every
			//style
		void closedListShowsTheColour_data()
		{
			QTest::addColumn<QString>("style");
			QTest::addColumn<QColor>("color");
				//Colours no style draws a list with
			const QList<QColor> shown{Qt::red, Qt::blue,
				QColor(0x12, 0x9a, 0x3c)};
			for (const QString &style : QStyleFactory::keys())
				for (const QColor &color : shown)
					QTest::addRow("%s %s",
						qPrintable(style),
						qPrintable(color.name()))
						<< style << color;
		}
		void closedListShowsTheColour()
		{
			QFETCH(QString, style);
			QFETCH(QColor, color);
			std::unique_ptr<QStyle> owner(
				QStyleFactory::create(style));
			KColorCombo combo;
			combo.setStyle(owner.get());
			combo.setColors(colors());
			combo.setColor(color);
			combo.resize(160, combo.sizeHint().height());
			const QImage image = combo.grab().toImage()
				.convertToFormat(QImage::Format_RGB32);
				//The square without its edge
			const QSize inner = combo.iconSize() - QSize(2, 2);
			const int shown = pixelsOf(image, color);
			QVERIFY2(shown >= inner.width() * inner.height(),
				 qPrintable(QString::number(shown)));
		}
};

QTEST_MAIN(tst_nokdecolorcombo)

#include "tst_nokdecolorcombo.moc"
