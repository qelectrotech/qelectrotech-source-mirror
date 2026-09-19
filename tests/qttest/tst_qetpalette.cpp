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
#include <QtTest>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QPainter>
#include <QRadioButton>
#include <QStandardItemModel>
#include <QTreeView>
#include <QStyleFactory>
#include <QToolBar>

#include "inkcontrast.h"
#include "ElementsCollection/elementpreviewdelegate.h"
#include "qetpalette.h"

using QET::Palette::contrastRatio;
using QET::Test::grab;
using QET::Test::inkContrast;

/**
	Contrast checks for the palettes QET installs under the Fusion style.

	Two levels. The role tests check the numbers directly: every
	foreground/background pair Fusion draws text with must read at 4.5:1
	(WCAG AA for normal text), 3:1 when disabled. The render tests paint
	real widgets with Fusion on the offscreen platform and measure the ink
	against its background, which is what a user sees and what broke on
	macOS: the combo box text went black on a dark combo as soon as the
	window lost focus, and radio indicators disappeared into the window.
*/
class tst_qetpalette : public QObject
{
	Q_OBJECT

	private slots:
		void textRolesAreReadable_data();
		void textRolesAreReadable();
		void controlsStandOutFromWindow_data();
		void controlsStandOutFromWindow();
		void inactiveEqualsActive_data();
		void inactiveEqualsActive();
		void platformAccentKeptOnlyWhenReadable();
		void forFusionPicksVariantByLightness();
		void styleIsFusionMatchesObjectName();
		void renderedWidgetsAreReadable_data();
		void renderedWidgetsAreReadable();
		void lineArtRuleSeparatesInkFromColor();
		void invertedLightnessKeepsHueAndAlpha();
		void elementPreviewReadsOnBothPalettes();
		void previewDelegateAdaptsLineArtOnly();
		void invertLightnessMapsSheetAndInk();
		void invertedViewReadsOnDarkSheet();
		void invertLightnessSpeed();

	private:
		static void addPaletteRows();
};

namespace {
	const double kTextRatio     = 4.5; ///< WCAG AA, normal text
	const double kDisabledRatio = 3.0; ///< disabled text
	/**
		An unchecked Fusion radio button is a disc filled with Base and
		outlined with Window darkened by 40%: on a dark palette only the
		fill is left to see it by, and 3:1 is out of reach for any Fusion
		dark theme. The dark palette here reaches 1.36 (light: 2.4); the
		macOS platform palette gave 1.06, which is the invisible case.
	*/
	const double kIndicatorRatio = 1.3;

	struct TextPair {
		const char *name;
		QPalette::ColorRole fg;
		QPalette::ColorRole bg;
	};
	const TextPair kTextPairs[] = {
		{"WindowText/Window",         QPalette::WindowText,      QPalette::Window},
		{"Text/Base",                 QPalette::Text,            QPalette::Base},
		{"Text/AlternateBase",        QPalette::Text,            QPalette::AlternateBase},
		{"ButtonText/Button",         QPalette::ButtonText,      QPalette::Button},
		{"ButtonText/Window",         QPalette::ButtonText,      QPalette::Window},
		{"HighlightedText/Highlight", QPalette::HighlightedText, QPalette::Highlight},
		{"ToolTipText/ToolTipBase",   QPalette::ToolTipText,     QPalette::ToolTipBase},
		{"PlaceholderText/Base",      QPalette::PlaceholderText, QPalette::Base},
		{"Link/Base",                 QPalette::Link,            QPalette::Base},
		{"Link/Window",               QPalette::Link,            QPalette::Window},
	};

}

void tst_qetpalette::addPaletteRows()
{
	QTest::addColumn<QPalette>("palette");
	QTest::newRow("light") << QET::Palette::fusionLight();
	QTest::newRow("dark")  << QET::Palette::fusionDark();
}

void tst_qetpalette::textRolesAreReadable_data()
{
	addPaletteRows();
}

void tst_qetpalette::textRolesAreReadable()
{
	QFETCH(QPalette, palette);
	for (const TextPair &pair : kTextPairs)
	{
		const double active = contrastRatio(
			palette.color(QPalette::Active, pair.fg),
			palette.color(QPalette::Active, pair.bg));
		const double disabled = contrastRatio(
			palette.color(QPalette::Disabled, pair.fg),
			palette.color(QPalette::Disabled, pair.bg));
		QVERIFY2(active >= kTextRatio,
		         qPrintable(QString("%1 active: %2 < %3")
		                    .arg(pair.name).arg(active).arg(kTextRatio)));
		QVERIFY2(disabled >= kDisabledRatio,
		         qPrintable(QString("%1 disabled: %2 < %3")
		                    .arg(pair.name).arg(disabled).arg(kDisabledRatio)));
	}
}

void tst_qetpalette::controlsStandOutFromWindow_data()
{
	addPaletteRows();
}

/**
	Fusion fills fields with Base and buttons with Button, and outlines
	both with Window darkened by 40%. The macOS platform palette set Base,
	Button and Window equal, so fields had no edge at all. In a dark
	palette the darkened outline is nearly black on near-black and the
	Button fill has to carry the separation on its own; in a light palette
	the outline does that job, as in Fusion's own standard palette where
	Button equals Window.
*/
void tst_qetpalette::controlsStandOutFromWindow()
{
	QFETCH(QPalette, palette);
	const QColor window = palette.color(QPalette::Active, QPalette::Window);
	const QColor button = palette.color(QPalette::Active, QPalette::Button);
	const QColor base   = palette.color(QPalette::Active, QPalette::Base);
	// White fields on Fusion's standard light grey (239) panel are 1.15:1;
	// the macOS platform palette had them equal.
	QVERIFY2(contrastRatio(window, base) >= 1.1, "Base blends into Window");
	if (QET::Palette::isDark(palette))
		QVERIFY2(contrastRatio(window, button) >= 1.1, "Button blends into Window");
	else
		QVERIFY2(contrastRatio(window, window.darker(140)) >= 1.5, "Fusion's outline blends into Window");
	QVERIFY2(contrastRatio(palette.color(QPalette::Light), palette.color(QPalette::Dark)) >= 1.5,
	         "Light and Dark too close for Fusion's bevels");
	QVERIFY2(palette.color(QPalette::Light).lightness() > palette.color(QPalette::Dark).lightness(),
	         "Light must be lighter than Dark");
}

void tst_qetpalette::inactiveEqualsActive_data()
{
	addPaletteRows();
}

/**
	The macOS platform palette sets Inactive ButtonText to black in dark
	mode, because native inactive buttons are drawn light. Fusion never
	draws them light, so combo box and button text vanished when the
	window lost focus. Our palettes keep every role identical between the
	two groups.
*/
void tst_qetpalette::inactiveEqualsActive()
{
	QFETCH(QPalette, palette);
	for (int role = 0; role < QPalette::NColorRoles; ++role)
	{
		const auto r = static_cast<QPalette::ColorRole>(role);
		QCOMPARE(palette.color(QPalette::Inactive, r),
		         palette.color(QPalette::Active, r));
	}
}

void tst_qetpalette::platformAccentKeptOnlyWhenReadable()
{
	QPalette readable;
	readable.setColor(QPalette::Active, QPalette::Highlight, QColor(74, 100, 66));
	readable.setColor(QPalette::Active, QPalette::HighlightedText, Qt::white);
	QPalette kept = QET::Palette::withPlatformAccent(QET::Palette::fusionDark(), readable);
	QCOMPARE(kept.color(QPalette::Active, QPalette::Highlight), QColor(74, 100, 66));
	QCOMPARE(kept.color(QPalette::Inactive, QPalette::Highlight), QColor(74, 100, 66));
	QCOMPARE(kept.color(QPalette::Active, QPalette::HighlightedText), QColor(Qt::white));

	QPalette faint;
	faint.setColor(QPalette::Active, QPalette::Highlight, QColor(200, 200, 200));
	faint.setColor(QPalette::Active, QPalette::HighlightedText, Qt::white);
	QPalette ours = QET::Palette::fusionDark();
	QPalette rejected = QET::Palette::withPlatformAccent(ours, faint);
	QCOMPARE(rejected.color(QPalette::Active, QPalette::Highlight),
	         ours.color(QPalette::Active, QPalette::Highlight));
}

void tst_qetpalette::forFusionPicksVariantByLightness()
{
	QPalette dark_platform;
	dark_platform.setColor(QPalette::Active, QPalette::Window, QColor(30, 30, 30));
	QVERIFY(QET::Palette::isDark(dark_platform));
	QVERIFY(QET::Palette::isDark(QET::Palette::forFusion(dark_platform)));

	QPalette light_platform;
	light_platform.setColor(QPalette::Active, QPalette::Window, Qt::white);
	QVERIFY(!QET::Palette::isDark(light_platform));
	QVERIFY(!QET::Palette::isDark(QET::Palette::forFusion(light_platform)));
}

void tst_qetpalette::styleIsFusionMatchesObjectName()
{
	QScopedPointer<QStyle> fusion(QStyleFactory::create("Fusion"));
	QVERIFY(!fusion.isNull());
	QVERIFY(QET::Palette::styleIsFusion(fusion.data()));
	QScopedPointer<QStyle> windows(QStyleFactory::create("Windows"));
	if (!windows.isNull())
		QVERIFY(!QET::Palette::styleIsFusion(windows.data()));
	QVERIFY(!QET::Palette::styleIsFusion(nullptr));
}

void tst_qetpalette::renderedWidgetsAreReadable_data()
{
	addPaletteRows();
}

/**
	Paint the widgets that broke on macOS with Fusion and measure the ink.
	The combo box sits in a toolbar exactly like the "Handles" size
	selector in the diagram editor; the radio buttons have no text, like
	the ones in the text alignment dialog.
*/
void tst_qetpalette::renderedWidgetsAreReadable()
{
	QFETCH(QPalette, palette);
	QApplication::setStyle(QStyleFactory::create("Fusion"));
	QApplication::setPalette(palette);

	QMainWindow window;
	QToolBar *toolbar = window.addToolBar("view");
	auto *host = new QWidget;
	auto *host_layout = new QHBoxLayout(host);
	host_layout->addWidget(new QLabel("Handles :"));
	auto *combo = new QComboBox;
	combo->addItems({"x 1", "x 2", "x 3"});
	host_layout->addWidget(combo);
	toolbar->addWidget(host);

	auto *central = new QWidget;
	auto *layout = new QHBoxLayout(central);
	auto *radio_off = new QRadioButton;
	auto *radio_on = new QRadioButton;
	radio_on->setChecked(true);
	auto *check = new QCheckBox("check");
	auto *button = new QPushButton("Button");
	auto *disabled = new QPushButton("Disabled");
	disabled->setEnabled(false);
	auto *edit = new QLineEdit("field");
	for (QWidget *w : {static_cast<QWidget *>(radio_off), static_cast<QWidget *>(radio_on),
	                   static_cast<QWidget *>(check), static_cast<QWidget *>(button),
	                   static_cast<QWidget *>(disabled), static_cast<QWidget *>(edit)})
		layout->addWidget(w);
	window.setCentralWidget(central);
	window.resize(520, 140);
	window.show();
	QVERIFY(QTest::qWaitForWindowExposed(&window));

	// Text area of the combo: skip the frame and the arrow on the right.
	const QImage combo_image = grab(combo, "combo");
	const QRect combo_text = combo->rect().adjusted(6, 4, -22, -4);
	const double combo_contrast = inkContrast(combo_image, combo_text);
	QVERIFY2(combo_contrast >= kTextRatio,
	         qPrintable(QString("combo box text: %1").arg(combo_contrast)));

	// Radio indicators: the whole widget is the indicator, no text. The
	// checked one has a dot to read by, the unchecked one only its disc.
	const double radio_on_contrast = inkContrast(grab(radio_on, "radio-checked"), radio_on->rect());
	QVERIFY2(radio_on_contrast >= kDisabledRatio,
	         qPrintable(QString("radio (checked): %1").arg(radio_on_contrast)));
	const double radio_off_contrast = inkContrast(grab(radio_off, "radio-unchecked"), radio_off->rect());
	QVERIFY2(radio_off_contrast >= kIndicatorRatio,
	         qPrintable(QString("radio (unchecked): %1").arg(radio_off_contrast)));

	const QImage button_image = grab(button, "button");
	const double button_contrast = inkContrast(button_image, button->rect().adjusted(6, 4, -6, -4));
	QVERIFY2(button_contrast >= kTextRatio,
	         qPrintable(QString("push button text: %1").arg(button_contrast)));

	const QImage disabled_image = grab(disabled, "button-disabled");
	const double disabled_contrast = inkContrast(disabled_image, disabled->rect().adjusted(6, 4, -6, -4));
	QVERIFY2(disabled_contrast >= kDisabledRatio,
	         qPrintable(QString("disabled button text: %1").arg(disabled_contrast)));

	const QImage edit_image = grab(edit, "lineedit");
	const double edit_contrast = inkContrast(edit_image, edit->rect().adjusted(4, 3, -4, -3));
	QVERIFY2(edit_contrast >= kTextRatio,
	         qPrintable(QString("line edit text: %1").arg(edit_contrast)));
}

namespace {
	/// A 40 x 40 transparent picture with a 3 px stroke square in color, as
	/// an element preview is drawn for the white sheet.
	QImage strokeSquare(const QColor &color)
	{
		QImage image(40, 40, QImage::Format_ARGB32);
		image.fill(Qt::transparent);
		QPainter painter(&image);
		painter.setPen(QPen(color, 3));
		painter.drawRect(6, 6, 27, 27);
		return image;
	}

	/// The most frequent color of a rendering: its background.
	QRgb dominant(const QImage &image)
	{
		QHash<QRgb, int> histogram;
		for (int y = 0; y < image.height(); ++y)
			for (int x = 0; x < image.width(); ++x)
				++histogram[image.pixel(x, y)];
		QRgb best = 0;
		int count = -1;
		for (auto it = histogram.cbegin(); it != histogram.cend(); ++it)
			if (it.value() > count) { count = it.value(); best = it.key(); }
		return best;
	}
}

void tst_qetpalette::lineArtRuleSeparatesInkFromColor()
{
	QVERIFY(QET::Palette::isLineArt(strokeSquare(Qt::black)));
	QVERIFY(QET::Palette::isLineArt(strokeSquare(QColor(80, 80, 80))));
	QVERIFY(!QET::Palette::isLineArt(strokeSquare(Qt::red)));
	QVERIFY(!QET::Palette::isLineArt(strokeSquare(QColor(30, 96, 176))));
	QVERIFY(!QET::Palette::isLineArt(QImage()));
}

/**
	Black ink becomes the dark palette's light gray, a colored stroke keeps
	its hue, and transparency is untouched.
*/
void tst_qetpalette::invertedLightnessKeepsHueAndAlpha()
{
	const QImage black = QET::Palette::invertedLightness(strokeSquare(Qt::black));
	QCOMPARE(black.pixelColor(6, 20).alpha(), 255);
	QVERIFY2(black.pixelColor(6, 20).lightnessF() > 0.8, "black ink did not become light");
	QCOMPARE(black.pixelColor(20, 20).alpha(), 0);

	const QImage red = QET::Palette::invertedLightness(strokeSquare(Qt::red));
	const QColor stroke = red.pixelColor(6, 20);
	QVERIFY2(qAbs(stroke.hslHueF() - QColor(Qt::red).hslHueF()) < 0.02, "hue changed");
	QVERIFY(stroke.hslSaturationF() > 0.9);
}

/**
	An element preview drawn for the white sheet must read at 3:1 on the
	Base color of both palettes: unchanged on the light one, inverted on
	the dark one.
*/
void tst_qetpalette::elementPreviewReadsOnBothPalettes()
{
	const QPixmap preview = QPixmap::fromImage(strokeSquare(Qt::black));
	for (const QPalette &palette : {QET::Palette::fusionLight(), QET::Palette::fusionDark()})
	{
		const QColor base = palette.color(QPalette::Active, QPalette::Base);
		const QPixmap shown = QET::Palette::forPalette(preview, palette);
		QImage row(shown.size(), QImage::Format_ARGB32);
		row.fill(base);
		QPainter painter(&row);
		painter.drawPixmap(0, 0, shown);
		painter.end();
		const double contrast = QET::Test::inkContrast(row, row.rect());
		QVERIFY2(contrast >= 3.0, qPrintable(QString("preview reads %1:1 on Base %2").arg(contrast).arg(base.name())));
	}
	// A light palette hands the picture back untouched.
	QCOMPARE(QET::Palette::forPalette(preview, QET::Palette::fusionLight()).cacheKey(), preview.cacheKey());
}

/**
	In a tree on the dark palette, the delegate inverts a line-art icon so
	it reads on the row, and leaves a colored icon (a folder) as it is.
*/
void tst_qetpalette::previewDelegateAdaptsLineArtOnly()
{
	QApplication::setStyle(QStyleFactory::create("Fusion"));
	QApplication::setPalette(QET::Palette::fusionDark());

	QStandardItemModel model;
	auto *element = new QStandardItem(QIcon(QPixmap::fromImage(strokeSquare(Qt::black))), "element");
	auto *folder = new QStandardItem(QIcon(QPixmap::fromImage(strokeSquare(QColor(30, 96, 176)))), "folder");
	model.appendRow(element);
	model.appendRow(folder);

	QTreeView view;
	view.setModel(&model);
	view.setIconSize(QSize(40, 40));
	view.setItemDelegate(new ElementPreviewDelegate(&view));
	view.resize(300, 200);
	view.show();
	QVERIFY(QTest::qWaitForWindowExposed(&view));

	const QImage image = view.viewport()->grab().toImage();
	const QRect element_icon(view.visualRect(element->index()).topLeft(), QSize(40, 40));
	const QRect folder_icon(view.visualRect(folder->index()).topLeft(), QSize(40, 40));
	const QColor base = QET::Palette::fusionDark().color(QPalette::Active, QPalette::Base);
	QCOMPARE(QColor(dominant(image)), base);

	const double element_contrast = QET::Test::inkContrast(image, element_icon);
	QVERIFY2(element_contrast >= 3.0, qPrintable(QString("element preview reads %1:1 on the dark row").arg(element_contrast)));

	// The folder icon keeps its blue: some pixel in its slot is still saturated blue.
	bool blue = false;
	for (int y = folder_icon.top(); y <= folder_icon.bottom() && !blue; ++y)
		for (int x = folder_icon.left(); x <= folder_icon.right() && !blue; ++x)
		{
			const QColor c = image.pixelColor(x, y);
			blue = c.hslSaturationF() > 0.5 && c.blue() > c.red() + 60;
		}
	QVERIFY2(blue, "the colored icon lost its color");
}

/**
	The lightness inversion sends white to the sheet color and black to
	the ink color, lands a mid gray between the two, and keeps the hue of
	a colored line. Without sheet and ink it is a plain inversion.
*/
void tst_qetpalette::invertLightnessMapsSheetAndInk()
{
	const QColor sheet(30, 30, 30);
	const QColor ink(220, 220, 220);
	QImage image(4, 1, QImage::Format_ARGB32);
	image.setPixelColor(0, 0, Qt::white);
	image.setPixelColor(1, 0, Qt::black);
	image.setPixelColor(2, 0, QColor(128, 128, 128));
	image.setPixelColor(3, 0, QColor(200, 0, 0));
	QImage plain = image;

	QET::Palette::invertLightness(image, sheet, ink);
	QCOMPARE(image.format(), QImage::Format_RGB32);
	QCOMPARE(image.pixelColor(0, 0), sheet);
	QCOMPARE(image.pixelColor(1, 0), ink);
	const int middle = (sheet.red() + ink.red()) / 2;
	QVERIFY(qAbs(image.pixelColor(2, 0).red() - middle) <= 2);
	const QColor red = image.pixelColor(3, 0);
	QCOMPARE(red.hslHue(), 0);
	QVERIFY2(red.hslSaturationF() > 0.5, qPrintable(red.name()));
	QVERIFY2(red.lightness() > QColor(200, 0, 0).lightness(), qPrintable(red.name()));

	QET::Palette::invertLightness(plain);
	QCOMPARE(plain.pixelColor(0, 0), QColor(Qt::black));
	QCOMPARE(plain.pixelColor(1, 0), QColor(Qt::white));
	QCOMPARE(plain.pixelColor(3, 0), QColor(255, 55, 55));
}

/**
	A view rendered the way DiagramView does it on a dark palette: a part
	of the viewport goes into an image, which is inverted between the
	palette's Base and Text. The white sheet comes out as Base, black
	lines read at text contrast, and a blue box is still blue.
*/
void tst_qetpalette::invertedViewReadsOnDarkSheet()
{
	QGraphicsScene scene(0, 0, 200, 120);
	scene.setBackgroundBrush(Qt::white);
	scene.addLine(10, 60, 190, 60, QPen(Qt::black, 2));
	scene.addRect(20, 20, 40, 20, QPen(Qt::NoPen), QBrush(QColor(30, 96, 176)));

	QGraphicsView view(&scene);
	view.setFrameShape(QFrame::NoFrame);
	view.setAlignment(Qt::AlignLeft | Qt::AlignTop);
	view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	view.resize(200, 120);
	view.show();
	QVERIFY(QTest::qWaitForWindowExposed(&view));

	// A part of the viewport that is not at its origin, as a partial
	// repaint after a scroll would be.
	const QRect area(10, 10, 100, 60);
	QImage buffer(area.size(), QImage::Format_RGB32);
	QPainter painter(&buffer);
	view.render(&painter, QRectF(QPointF(0, 0), QSizeF(area.size())), area);
	painter.end();

	const QPalette dark = QET::Palette::fusionDark();
	const QColor base = dark.color(QPalette::Active, QPalette::Base);
	const QColor text = dark.color(QPalette::Active, QPalette::Text);
	QET::Palette::invertLightness(buffer, base, text);

	QHash<QRgb, int> histogram;
	for (int y = 0; y < buffer.height(); ++y)
		for (int x = 0; x < buffer.width(); ++x)
			++histogram[buffer.pixel(x, y)];
	QRgb dominant = 0;
	for (auto it = histogram.cbegin(); it != histogram.cend(); ++it)
		if (it.value() > histogram.value(dominant)) dominant = it.key();
	QCOMPARE(QColor(dominant), base);

	const double contrast = inkContrast(buffer, buffer.rect());
	QVERIFY2(contrast >= QET::Palette::contrastRatio(base, text) - 0.5,
	         qPrintable(QString("ink reads %1:1 on the dark sheet").arg(contrast)));

	// The box at scene (20..60, 20..40) sits at (10..50, 10..30) in the buffer.
	const QColor box = buffer.pixelColor(30, 20);
	QVERIFY2(box.hslSaturationF() > 0.3 && box.blue() > box.red() + 60,
	         qPrintable(QString("the blue box became %1").arg(box.name())));
}

/**
	The inversion runs on every repaint of the folio, so a 4K viewport
	has to cost a few milliseconds. Reported, not asserted: the bound
	depends on the build box.
*/
void tst_qetpalette::invertLightnessSpeed()
{
	QImage image(3840, 2000, QImage::Format_RGB32);
	image.fill(Qt::white);
	QBENCHMARK {
		QET::Palette::invertLightness(image, QColor(30, 30, 30), QColor(220, 220, 220));
	}
}

int main(int argc, char **argv)
{
	// Widgets are painted for real, on Qt's offscreen platform so the test
	// runs the same on a build box and on a desktop.
	if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
		qputenv("QT_QPA_PLATFORM", "offscreen");
	QApplication app(argc, argv);
	tst_qetpalette test;
	QTEST_SET_MAIN_SOURCE_PATH
	return QTest::qExec(&test, argc, argv);
}

#include "tst_qetpalette.moc"
