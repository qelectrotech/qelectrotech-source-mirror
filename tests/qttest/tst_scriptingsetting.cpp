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
	QetSettings::scriptingEnabled() -- the switch that decides whether
	QElectroTech will run JavaScript at all.

	Its default is the whole point: a setting nobody has touched must read
	as off, because that is the state every existing installation is in
	after an upgrade. The other case worth pinning is the environment
	override, which exists so a headless run has a way in without a dialog
	-- and which must beat a stored "false", or a CI machine that once had
	the box unticked can never script again.

	This test owns its own QSettings scope (organization + application
	name), so it cannot read or write the real configuration of whoever
	runs it.
*/

#include "utils/qetsettings.h"

#include <QtTest>
#include <QSettings>

class TstScriptingSetting : public QObject
{
		Q_OBJECT

	private slots:
		void initTestCase();
		void init();

		void defaultsToOff();
		void storedValueIsHonoured();
		void environmentOverridesAStoredFalse();
		void environmentIsReportedSeparately();
		void writingDoesNotDependOnReading();

	private:
		void clearStoredValue();
};

void TstScriptingSetting::initTestCase()
{
	// A scope of this test's own: whatever this writes must not land in
	// the configuration of the account running the suite.
	QCoreApplication::setOrganizationName(
				QStringLiteral("QElectroTech-tst_scriptingsetting"));
	QCoreApplication::setApplicationName(
				QStringLiteral("tst_scriptingsetting"));
	QSettings settings;
	settings.clear();
}

void TstScriptingSetting::clearStoredValue()
{
	QSettings settings;
	settings.remove(QStringLiteral("scripting/enabled"));
	settings.sync();
}

void TstScriptingSetting::init()
{
	clearStoredValue();
	qunsetenv("QET_ENABLE_SCRIPTING");
}

void TstScriptingSetting::defaultsToOff()
{
	QVERIFY2(!QetSettings::scriptingEnabled(),
		 "an untouched installation must not run scripts");
}

void TstScriptingSetting::storedValueIsHonoured()
{
	QetSettings::setScriptingEnabled(true);
	QVERIFY(QetSettings::scriptingEnabled());

	QetSettings::setScriptingEnabled(false);
	QVERIFY(!QetSettings::scriptingEnabled());
}

void TstScriptingSetting::environmentOverridesAStoredFalse()
{
	// The headless case: no dialog to tick, and a stored false that must
	// not be able to lock a CI job out of --run forever.
	QetSettings::setScriptingEnabled(false);
	qputenv("QET_ENABLE_SCRIPTING", "1");
	QVERIFY(QetSettings::scriptingEnabled());

	// Only "1" counts. An empty or accidental value is not consent.
	for (const QByteArray &value : {QByteArray(""), QByteArray("0"),
					QByteArray("true"), QByteArray("yes")}) {
		qputenv("QET_ENABLE_SCRIPTING", value);
		QVERIFY2(!QetSettings::scriptingEnabled(),
			 qPrintable(QStringLiteral("accepted QET_ENABLE_SCRIPTING=%1")
				    .arg(QString::fromUtf8(value))));
	}
}

void TstScriptingSetting::environmentIsReportedSeparately()
{
	// The configuration dialog asks this to explain why its checkbox is
	// disabled, so it must answer about the environment alone and not be
	// confused by the stored value.
	QetSettings::setScriptingEnabled(true);
	QVERIFY(!QetSettings::scriptingForcedByEnvironment());

	qputenv("QET_ENABLE_SCRIPTING", "1");
	QVERIFY(QetSettings::scriptingForcedByEnvironment());
}

void TstScriptingSetting::writingDoesNotDependOnReading()
{
	// While the environment forces scripting on, scriptingEnabled() says
	// true whatever is stored -- so read the stored value directly to be
	// sure a write still lands. The configuration dialog relies on this:
	// it skips the write in that state on purpose, and would be silently
	// wrong if setScriptingEnabled() were a no-op instead.
	qputenv("QET_ENABLE_SCRIPTING", "1");
	QetSettings::setScriptingEnabled(false);
	QSettings settings;
	QCOMPARE(settings.value(QStringLiteral("scripting/enabled")).toBool(), false);

	QetSettings::setScriptingEnabled(true);
	QSettings other;
	QCOMPARE(other.value(QStringLiteral("scripting/enabled")).toBool(), true);
}

QTEST_MAIN(TstScriptingSetting)
#include "tst_scriptingsetting.moc"
