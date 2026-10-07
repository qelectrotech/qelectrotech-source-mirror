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
#include "conductorproperties.h"
#include "qetapp.h"

#include <QtTest>
#include <QSettings>

// qet.cpp, compiled in for ConductorProperties, refers to this static.
QString QETApp::m_interface_language;

/**
	ConductorProperties::toSettings()/fromSettings() keep the conductor
	width (cond_size) as it was saved. The width is a decimal, written with
	QString::number(); reading it with toInt() gave 0 for every value that
	was not a whole number, so a default width of 1.4 came back as 0 after
	a restart.

	This test owns its own QSettings scope (organization + application
	name), so it cannot read or write the configuration of whoever runs it.
*/
class tst_conductorsizesetting : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase()
	{
		QCoreApplication::setOrganizationName(
					QStringLiteral("QElectroTech-tst_conductorsizesetting"));
		QCoreApplication::setApplicationName(QStringLiteral("tst_conductorsizesetting"));
		QSettings settings;
		settings.clear();
	}

	void init()
	{
		QSettings settings;
		settings.clear();
		settings.sync();
	}

	void widthSurvivesARoundTrip_data()
	{
		QTest::addColumn<qreal>("width");
		QTest::newRow("whole number") << 2.0;
		QTest::newRow("decimal")      << 1.4;
		QTest::newRow("minimum")      << 0.4;
		QTest::newRow("rounds down")  << 61.3;
		QTest::newRow("rounds up")    << 61.7;
	}

	void widthSurvivesARoundTrip()
	{
		QFETCH(qreal, width);
		QSettings settings;
		ConductorProperties saved;
		saved.cond_size = width;
		saved.toSettings(settings, QStringLiteral("test/"));

		ConductorProperties loaded;
		loaded.fromSettings(settings, QStringLiteral("test/"));
		QCOMPARE(loaded.cond_size, width);
	}

	// A hand-edited or truncated settings file must not give a width of 0.
	void unusableValueFallsBackToOne_data()
	{
		QTest::addColumn<QString>("value");
		QTest::newRow("text")     << QStringLiteral("wide");
		QTest::newRow("zero")     << QStringLiteral("0");
		QTest::newRow("negative") << QStringLiteral("-1");
		QTest::newRow("nan")      << QStringLiteral("nan");
		QTest::newRow("inf")      << QStringLiteral("inf");
	}

	void unusableValueFallsBackToOne()
	{
		QFETCH(QString, value);
		QSettings settings;
		settings.setValue(QStringLiteral("test/size"), value);

		ConductorProperties loaded;
		loaded.fromSettings(settings, QStringLiteral("test/"));
		QCOMPARE(loaded.cond_size, 1.0);
	}

	void missingValueIsOne()
	{
		QSettings settings;
		ConductorProperties loaded;
		loaded.fromSettings(settings, QStringLiteral("test/"));
		QCOMPARE(loaded.cond_size, 1.0);
	}
};

QTEST_GUILESS_MAIN(tst_conductorsizesetting)
#include "tst_conductorsizesetting.moc"
