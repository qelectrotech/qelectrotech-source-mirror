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
#include "kcolorbutton.h"

#include <QHBoxLayout>
#include <QImage>
#include <QPushButton>
#include <QStyle>
#include <QStyleFactory>
#include <QtTest>

#include <cmath>
#include <memory>

/*
	Built without KDE Frameworks, QElectroTech has a KColorButton of its
	own (sources/ui/nokde). In every style it shows its colour as it is,
	with the name of the colour readable on it: black on a black
	conductor's button too. Reached with Tab, it shows the keyboard focus.
*/
class tst_nokdecolorbutton : public QObject
{
	Q_OBJECT

	static qreal luminance(const QColor &color)
	{
		const auto linear = [](qreal c) {
			return c <= 0.04045 ? c / 12.92
					    : std::pow((c + 0.055) / 1.055, 2.4);
		};
		return 0.2126 * linear(color.redF())
			+ 0.7152 * linear(color.greenF())
			+ 0.0722 * linear(color.blueF());
	}

		//The contrast ratio of WCAG 2, from 1 to 21
	static qreal contrast(const QColor &first, const QColor &second)
	{
		const qreal a = luminance(first) + 0.05;
		const qreal b = luminance(second) + 0.05;
		return a > b ? a / b : b / a;
	}

	static QImage drawn(const QString &style_name, const QColor &color,
			    bool enabled, std::unique_ptr<QStyle> &style)
	{
		style.reset(QStyleFactory::create(style_name));
		auto *button = new KColorButton;
		std::unique_ptr<KColorButton> owner(button);
		button->setStyle(style.get());
		button->setColor(color);
		button->setEnabled(enabled);
		button->resize(120, button->sizeHint().height());
		return button->grab().toImage().convertToFormat(QImage::Format_RGB32);
	}

		//The most common colour of @p image
	static QColor mostCommon(const QImage &image)
	{
		QHash<QRgb, int> count;
		for (int y = 0; y < image.height(); ++y)
			for (int x = 0; x < image.width(); ++x)
				++count[image.pixel(x, y)];
		QRgb most = 0;
		int most_count = -1;
		for (auto it = count.constBegin(); it != count.constEnd(); ++it) {
			if (it.value() > most_count) {
				most = it.key();
				most_count = it.value();
			}
		}
		return QColor(most);
	}

		//The highest contrast with @p background in the middle of
		//@p image, where the name of the colour is
	static qreal textContrast(const QImage &image, const QColor &background)
	{
		qreal highest = 1;
		for (int y = image.height() / 3; y < image.height() * 2 / 3; ++y)
			for (int x = image.width() / 4; x < image.width() * 3 / 4; ++x)
				highest = qMax(highest,
					contrast(QColor(image.pixel(x, y)), background));
		return highest;
	}

	static void addRows()
	{
		QTest::addColumn<QString>("style");
		QTest::addColumn<QColor>("color");
		const QList<QColor> colors{Qt::black, Qt::white,
			QColor(0x80, 0x80, 0x80), Qt::red, Qt::yellow,
			QColor(0x00, 0x00, 0x80)};
		for (const QString &style : QStyleFactory::keys())
			for (const QColor &color : colors)
				QTest::addRow("%s %s", qPrintable(style),
					      qPrintable(color.name()))
					<< style << color;
	}

	private slots:
		void showsTheColourAsItIs_data() { addRows(); }
		void showsTheColourAsItIs()
		{
			QFETCH(QString, style);
			QFETCH(QColor, color);
			std::unique_ptr<QStyle> owner;
			const QImage image = drawn(style, color, true, owner);
			QCOMPARE(mostCommon(image), color);
		}

		void nameIsReadable_data() { addRows(); }
		void nameIsReadable()
		{
			QFETCH(QString, style);
			QFETCH(QColor, color);
			std::unique_ptr<QStyle> owner;
			const QImage image = drawn(style, color, true, owner);
				//4.5 is what WCAG asks of text
			QVERIFY2(textContrast(image, color) >= 4.5,
				 qPrintable(QString::number(textContrast(image, color))));
		}

		void disabledButtonIsDimmed_data() { addRows(); }
		void disabledButtonIsDimmed()
		{
			QFETCH(QString, style);
			QFETCH(QColor, color);
			std::unique_ptr<QStyle> owner;
			const QImage image = drawn(style, color, false, owner);
			const QColor fill = mostCommon(image);
			QVERIFY(fill != color);
			QVERIFY2(textContrast(image, fill) >= 2.5,
				 qPrintable(QString::number(textContrast(image, fill))));
		}

			//Reached with Tab, the button shows the focus as the style
			//draws it, and its colour as it is
		void focusIsShown_data() { addRows(); }
		void focusIsShown()
		{
			QFETCH(QString, style);
			QFETCH(QColor, color);
			std::unique_ptr<QStyle> owner(QStyleFactory::create(style));
			QWidget window;
			auto *layout = new QHBoxLayout(&window);
			auto *before = new QPushButton(QStringLiteral("Before"));
			auto *button = new KColorButton;
			before->setStyle(owner.get());
			button->setStyle(owner.get());
			button->setColor(color);
			button->setMinimumWidth(120);
			layout->addWidget(before);
			layout->addWidget(button);
			window.show();
			window.activateWindow();
			QVERIFY(QTest::qWaitForWindowActive(&window));
			before->setFocus();
			const QImage unfocused = button->grab().toImage();
			QTest::keyClick(before, Qt::Key_Tab);
			QVERIFY(button->hasFocus());
			const QImage focused = button->grab().toImage();
			QVERIFY(focused != unfocused);
			QCOMPARE(mostCommon(
				focused.convertToFormat(QImage::Format_RGB32)), color);
		}
};

QTEST_MAIN(tst_nokdecolorbutton)

#include "tst_nokdecolorbutton.moc"
