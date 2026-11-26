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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"
#include "projectcheck.h"
#include "projectcheck/checksettings.h"
#include "qetproject.h"
#include "qetresult.h"
#include "ui/configpage/projectconfigpages.h"
#include "ui/diagramcontextwidget.h"

#include <QCheckBox>
#include <QFile>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QtTest>

/**
	Items B12 and B8 of #1878: the choices of the checks kept in the
	project, so that two machines which open one project get the same
	check results.
*/
class tst_checksettings : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	QHash<CheckSettings::Option, bool> m_saved;

	QString copy(const QString &fixture)
	{
		const QString path = m_dir.filePath(fixture);
		QFile::remove(path);
		if (!QFile::copy(QFINDTESTDATA(QStringLiteral("fixtures/") + fixture), path)) return {};
		QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
		return path;
	}

		/// A machine whose application settings are all @p value
	static void setMachine(bool value)
	{
		for (auto option : CheckSettings::options())
			CheckSettings::setApplicationValue(option, value);
	}

		/// What the checks the options change find, as text
	static QStringList results(QETProject &project)
	{
		project.loadFolios();
		QStringList found;
		for (auto kind : {ProjectCheck::Kind::DuplicateLabels, ProjectCheck::Kind::UnwiredSymbols})
			for (const auto &finding : ProjectCheck::run(&project, kind).findings)
				found << finding.text;
		found.sort();
		return found;
	}

	static QByteArray readAll(const QString &path)
	{
		QFile file(path);
		return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QETProject::setBackupEnabled(false);
		for (auto option : CheckSettings::options())
			m_saved.insert(option, CheckSettings::applicationValue(option));
	}

	void cleanupTestCase()
	{
		for (auto it = m_saved.cbegin() ; it != m_saved.cend() ; ++it)
			CheckSettings::setApplicationValue(it.key(), it.value());
	}

		/// A project follows the application until it has its own, and
		/// stores nothing while it does
	void followsTheApplicationByDefault()
	{
		QETProject project;
		project.addNewDiagram();
		QVERIFY(!CheckSettings::hasOwnOptions(&project));
		for (bool machine : {false, true}) {
			setMachine(machine);
			for (auto option : CheckSettings::options())
				QCOMPARE(CheckSettings::value(&project, option), machine);
		}
		QVERIFY(!project.toXml().toString().contains(QLatin1String("check_")));

			// Changing an option changes the application's
		setMachine(false);
		CheckSettings::setValue(&project, CheckSettings::Option::UnwiredSkipContacts, true);
		QVERIFY(CheckSettings::applicationValue(CheckSettings::Option::UnwiredSkipContacts));
		QVERIFY(!CheckSettings::hasOwnOptions(&project));
	}

		/// Its own options: every one stored, the application's ignored;
		/// changing one changes the project's, not the application's
	void ownOptionsWinOverTheApplication()
	{
		QETProject project;
		project.addNewDiagram();
		QHash<CheckSettings::Option, bool> own;
		for (auto option : CheckSettings::options())
			own.insert(option, option == CheckSettings::Option::DuplicatesByDesignation);
		project.setProjectProperties(CheckSettings::withOptions(project.projectProperties(), own));
		QVERIFY(CheckSettings::hasOwnOptions(&project));
		for (bool machine : {false, true}) {
			setMachine(machine);
			for (auto option : CheckSettings::options())
				QCOMPARE(CheckSettings::value(&project, option), own.value(option));
		}
		setMachine(false);
		CheckSettings::setValue(&project, CheckSettings::Option::UnwiredSkipContacts, true);
		QVERIFY(CheckSettings::value(&project, CheckSettings::Option::UnwiredSkipContacts));
		QVERIFY(!CheckSettings::applicationValue(CheckSettings::Option::UnwiredSkipContacts));

			// Back to the application's: no property left
		project.setProjectProperties(CheckSettings::withOptions(project.projectProperties(), {}));
		QVERIFY(!CheckSettings::hasOwnOptions(&project));
		QVERIFY(!project.toXml().toString().contains(QLatin1String("check_")));
	}

		/// The item's "done when": one project, two machines with opposite
		/// settings, the same results once the project has its own. Without
		/// them the two machines differ, which shows the test can fail.
	void twoMachinesFindTheSame_data()
	{
		QTest::addColumn<QString>("file");
		for (const char *ext : {"qet", "qetz", "qetdb"})
			QTest::newRow(ext) << QStringLiteral("two_machines.") + QLatin1String(ext);
	}
	void twoMachinesFindTheSame()
	{
		QFETCH(QString, file);
		const QString fixture = copy(QStringLiteral("every_feature.qet"));
		QVERIFY(!fixture.isEmpty());
		{
			QETProject plain(fixture);
			setMachine(false);
			const QStringList first = results(plain);
			setMachine(true);
			QVERIFY2(results(plain) != first,
					 "the fixture finds the same with every option: it cannot tell");
		}

		const QString path = m_dir.filePath(file);
		QStringList expected;
		{
			QETProject project(fixture);
			QHash<CheckSettings::Option, bool> own;
			for (auto option : CheckSettings::options())
				own.insert(option, false);
			project.setProjectProperties(CheckSettings::withOptions(project.projectProperties(), own));
			setMachine(false);
			expected = results(project);
			project.setFilePath(path);
			const QETResult written = project.write();
			QVERIFY2(written.isOk(), qPrintable(written.errorMessage()));
		}
		for (bool machine : {false, true}) {
			setMachine(machine);
			QETProject again(path);
			QVERIFY(CheckSettings::hasOwnOptions(&again));
			QCOMPARE(results(again), expected);
		}
	}

		/// A project that follows the application saves the same bytes as
		/// before; giving it options and taking them away again too
	void followingSavesTheSameBytes()
	{
		const QString fixture = copy(QStringLiteral("every_feature.qet"));
		const QString path = m_dir.filePath(QStringLiteral("same_bytes.qet"));
		{
			QETProject project(fixture);
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		const QByteArray before = readAll(path);
		QVERIFY(!before.isEmpty());
		QVERIFY(!before.contains("check_"));
		{
			QETProject project(path);
			QHash<CheckSettings::Option, bool> own;
			own.insert(CheckSettings::Option::DuplicatesCaseSensitive, true);
			project.setProjectProperties(CheckSettings::withOptions(project.projectProperties(), own));
			QVERIFY(project.toXml().toString().contains(QLatin1String("check_duplicates_case_sensitive")));
			project.setProjectProperties(CheckSettings::withOptions(project.projectProperties(), {}));
			QVERIFY(project.write().isOk());
		}
		QCOMPARE(readAll(path), before);
	}

		/// Project properties > General: the options are not rows of the
		/// variables, "Use the application settings" stores none, unticked
		/// stores every one; the other variables stay
	void theGeneralPageEditsThem()
	{
		setMachine(false);
		QETProject project;
		project.addNewDiagram();
		DiagramContext properties = project.projectProperties();
		properties.addValue(QStringLiteral("customer"), QStringLiteral("ACME"));
		project.setProjectProperties(properties);

		ProjectMainConfigPage page(&project);
		auto *application = page.findChild<QCheckBox *>(QStringLiteral("checkOptionsApplication"));
		auto *contacts = page.findChild<QCheckBox *>(
			CheckSettings::key(CheckSettings::Option::UnwiredSkipContacts));
		auto *variables = page.findChild<DiagramContextWidget *>();
		QVERIFY(application && contacts && variables);
		QVERIFY(application->isChecked());
		QVERIFY(!contacts->isEnabled());

		page.applyProjectConf();
		QVERIFY(!CheckSettings::hasOwnOptions(&project));

		application->setChecked(false);
		QVERIFY(contacts->isEnabled());
		contacts->setChecked(true);
		page.applyProjectConf();
		QVERIFY(CheckSettings::hasOwnOptions(&project));
		QVERIFY(CheckSettings::value(&project, CheckSettings::Option::UnwiredSkipContacts));
		QVERIFY(!CheckSettings::applicationValue(CheckSettings::Option::UnwiredSkipContacts));
		for (auto option : CheckSettings::options())
			QVERIFY(project.projectProperties().contains(CheckSettings::key(option)));
		QCOMPARE(project.projectProperties().value(QStringLiteral("customer")).toString(),
				 QStringLiteral("ACME"));

			// Read again: ticked off, not a row of the variables
		ProjectMainConfigPage again(&project);
		auto *variables_again = again.findChild<DiagramContextWidget *>();
		QVERIFY(!again.findChild<QCheckBox *>(QStringLiteral("checkOptionsApplication"))->isChecked());
		for (auto option : CheckSettings::options())
			QVERIFY(!variables_again->context().contains(CheckSettings::key(option)));
		again.applyProjectConf();
		QVERIFY(CheckSettings::value(&project, CheckSettings::Option::UnwiredSkipContacts));

		application->setChecked(true);
		page.applyProjectConf();
		QVERIFY(!CheckSettings::hasOwnOptions(&project));
		QCOMPARE(project.projectProperties().value(QStringLiteral("customer")).toString(),
				 QStringLiteral("ACME"));
	}
};

QET_APP_TEST_MAIN(tst_checksettings)
#include "tst_checksettings.moc"
