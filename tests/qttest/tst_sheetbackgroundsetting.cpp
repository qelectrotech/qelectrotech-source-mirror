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

/*
	QetSettings::sheetBackground() -- the sheet (folio) background the
	diagram editor remembers between runs, so the user does not have to
	pick it again on every start.

	Two properties are pinned here, and both are why this is a pair of
	values rather than a single colour:

	* The custom flag. "#ffffff" picked explicitly ("always white, never
	  inverted on a dark palette") and "#ffffff" left to follow the
	  system (where the views invert the lightness) are one colour and
	  two behaviours. Storing only the colour would silently turn one
	  into the other on the next start -- which is exactly the reported
	  bug, one step removed.
	* The alpha being dropped. The SVG export gives
	  Diagram::background_color an alpha of 0 to render a transparent
	  background, and that transient value must not be persisted as a
	  permanently transparent sheet.

	This test owns its own QSettings scope (organization + application
	name), so it cannot read or write the configuration of whoever runs
	it.
*/

#include "utils/qetsettings.h"

#include <QtTest>
#include <QSettings>

class TstSheetBackgroundSetting : public QObject
{
		Q_OBJECT

	private slots:
		void initTestCase();
		void init();

		void defaultsToWhiteFollowingTheSystem();
		void colourAndCustomFlagAreStoredTogether();
		void sameColourStillDistinguishesTheTwoModes();
		void alphaIsNotStored();
		void unreadableColourFallsBackToWhite();
		void recentColorsDefaultToEmpty();
		void recentColorsKeepTheirOrder();
};

void TstSheetBackgroundSetting::initTestCase()
{
	// A scope of this test's own: whatever this writes must not land in
	// the configuration of the account running the suite.
	QCoreApplication::setOrganizationName(
				QStringLiteral("QElectroTech-tst_sheetbackgroundsetting"));
	QCoreApplication::setApplicationName(
				QStringLiteral("tst_sheetbackgroundsetting"));
	QSettings settings;
	settings.clear();
}

void TstSheetBackgroundSetting::init()
{
	QSettings settings;
	settings.remove(QStringLiteral("diagrameditor/sheet_background_color"));
	settings.remove(QStringLiteral("diagrameditor/sheet_background_custom"));
	settings.remove(QStringLiteral("diagrameditor/sheet_background_recent"));
	settings.sync();
}

void TstSheetBackgroundSetting::defaultsToWhiteFollowingTheSystem()
{
	// An untouched installation, i.e. every existing one after an upgrade:
	// white sheet, following the system so the views may still invert it.
	const auto background = QetSettings::sheetBackground();
	QVERIFY(background.color.isValid());
	QCOMPARE(background.color, QColor(Qt::white));
	QVERIFY(!background.custom);
}

void TstSheetBackgroundSetting::colourAndCustomFlagAreStoredTogether()
{
	QetSettings::setSheetBackground(QetSettings::SheetBackground{QColor("#404040"), true});

	const auto background = QetSettings::sheetBackground();
	QCOMPARE(background.color, QColor("#404040"));
	QVERIFY(background.custom);
}

void TstSheetBackgroundSetting::sameColourStillDistinguishesTheTwoModes()
{
	// The reported bug in miniature: white is white either way, yet one
	// of the two must survive a restart as an explicit choice and the
	// other as "follow the system".
	QetSettings::setSheetBackground(QetSettings::SheetBackground{QColor("#ffffff"), true});
	QVERIFY(QetSettings::sheetBackground().custom);

	QetSettings::setSheetBackground(QetSettings::SheetBackground{QColor("#ffffff"), false});
	QVERIFY(!QetSettings::sheetBackground().custom);
}

void TstSheetBackgroundSetting::alphaIsNotStored()
{
	// What the SVG export leaves on Diagram::background_color once it has
	// asked for a transparent background.
	QColor transparent(QStringLiteral("#12ab34"));
	transparent.setAlpha(0);
	QetSettings::setSheetBackground(QetSettings::SheetBackground{transparent, true});

	const auto background = QetSettings::sheetBackground();
	QCOMPARE(background.color.name(), QStringLiteral("#12ab34"));
	QCOMPARE(background.color.alpha(), 255);
}

void TstSheetBackgroundSetting::unreadableColourFallsBackToWhite()
{
	// A hand-edited or truncated settings file must not blank the sheet.
	QSettings settings;
	settings.setValue(QStringLiteral("diagrameditor/sheet_background_color"),
			  QStringLiteral("not a colour"));
	settings.sync();

	const auto background = QetSettings::sheetBackground();
	QCOMPARE(background.color, QColor(Qt::white));
}

void TstSheetBackgroundSetting::recentColorsDefaultToEmpty()
{
	QVERIFY(QetSettings::sheetBackgroundRecentColors().isEmpty());
}

void TstSheetBackgroundSetting::recentColorsKeepTheirOrder()
{
	// Most recent first, as the menu lists them.
	QetSettings::setSheetBackgroundRecentColors({QStringLiteral("#111111"),
						     QStringLiteral("#222222")});
	QCOMPARE(QetSettings::sheetBackgroundRecentColors(),
		 QStringList({QStringLiteral("#111111"), QStringLiteral("#222222")}));

	QetSettings::setSheetBackgroundRecentColors(QStringList());
	QVERIFY(QetSettings::sheetBackgroundRecentColors().isEmpty());
}

QTEST_MAIN(TstSheetBackgroundSetting)
#include "tst_sheetbackgroundsetting.moc"
