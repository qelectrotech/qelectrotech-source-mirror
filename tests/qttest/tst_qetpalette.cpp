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
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QRadioButton>
#include <QStyleFactory>
#include <QToolBar>

#include "inkcontrast.h"
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
