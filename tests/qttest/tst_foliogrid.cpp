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
#include "foliogrid.h"

#include <QtTest>
#include <QSettings>

/**
	FolioGrid::step() -- the folio grid step as read from the settings.
	Every reader of diagrameditor/Xgrid and Ygrid goes through it, so a
	settings file holding 0 (a division by zero in Diagram::snapToGrid()
	and an endless "% 0" in drawBackground()), a negative number (an
	endless loop) or text can no longer reach the code that uses the step.
*/
class tst_foliogrid : public QObject
{
	Q_OBJECT

private slots:
	void usableValuesAreKept_data()
	{
		QTest::addColumn<QVariant>("value");
		QTest::addColumn<int>("expected");
		QTest::newRow("one")          << QVariant(1) << 1;
		QTest::newRow("default")      << QVariant(10) << 10;
		QTest::newRow("text number")  << QVariant(QStringLiteral("7")) << 7;
		QTest::newRow("large")        << QVariant(100000) << 100000;
		QTest::newRow("int max")      << QVariant(2147483647) << 2147483647;
	}

	void usableValuesAreKept()
	{
		QFETCH(QVariant, value);
		QFETCH(int, expected);
		QCOMPARE(FolioGrid::step(value, 10), expected);
	}

	void unusableValuesFallBack_data()
	{
		QTest::addColumn<QVariant>("value");
		QTest::newRow("missing")   << QVariant();
		QTest::newRow("zero")      << QVariant(0);
		QTest::newRow("negative")  << QVariant(-5);
		QTest::newRow("text")      << QVariant(QStringLiteral("ten"));
		QTest::newRow("empty")     << QVariant(QString());
		QTest::newRow("nan")       << QVariant(QStringLiteral("nan"));
		QTest::newRow("too large") << QVariant(QStringLiteral("99999999999"));
		QTest::newRow("too large number") << QVariant(qlonglong(99999999999));
		QTest::newRow("int max + 1") << QVariant(qlonglong(2147483648));
	}

	void unusableValuesFallBack()
	{
		QFETCH(QVariant, value);
		QCOMPARE(FolioGrid::step(value, 10), 10);
		QCOMPARE(FolioGrid::step(value, 20), 20);
	}

	// A decimal in the file is rounded to a whole number, as the plain
	// QVariant::toInt() read did before.
	void decimalIsRoundedLikeBefore()
	{
		QCOMPARE(FolioGrid::step(QVariant(7.9), 10), 8);
		QCOMPARE(FolioGrid::step(QVariant(0.4), 10), 10);
	}

	// Through a real settings file, in a scope of this test's own.
	void readsTheSettingsKeys()
	{
		QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech-tst_foliogrid"));
		QCoreApplication::setApplicationName(QStringLiteral("tst_foliogrid"));
		QSettings settings;
		settings.clear();
		QCOMPARE(FolioGrid::step(settings.value(FolioGrid::x_key), 10), 10);
		settings.setValue(FolioGrid::x_key, 0);
		settings.setValue(FolioGrid::y_key, 15);
		QCOMPARE(FolioGrid::step(settings.value(FolioGrid::x_key), 10), 10);
		QCOMPARE(FolioGrid::step(settings.value(FolioGrid::y_key), 10), 15);
		settings.clear();
	}
};

QTEST_GUILESS_MAIN(tst_foliogrid)
#include "tst_foliogrid.moc"
