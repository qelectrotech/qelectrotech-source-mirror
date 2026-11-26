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
#include "ui/checkpaneldockwidget.h"
#include "ui/configpage/projectconfigpages.h"
#include "ui/diagramcontextwidget.h"
#include "wiringrules.h"

#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QAction>
#include <QApplication>
#include <QTimer>
#include <QMenu>
#include <QSet>
#include <QTemporaryDir>
#include <QTreeWidget>
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

		/// The ids of the findings of every check, by check
	static QHash<ProjectCheck::Kind, QStringList> findingIds(QETProject &project)
	{
		project.loadFolios();
		QHash<ProjectCheck::Kind, QStringList> ids;
		for (auto kind : ProjectCheck::kinds()) {
			QStringList found;
			for (const auto &finding : ProjectCheck::run(&project, kind).findings)
				found << CheckSettings::findingId(kind, finding);
			found.sort();
			ids.insert(kind, found);
		}
		return ids;
	}

		/// The open (not accepted) findings listed under a check's row
	static int openRows(QTreeWidgetItem *root)
	{
		int open = 0;
		for (int i = 0 ; i < root->childCount() ; ++i)
			open += !root->child(i)->isHidden();
		return open;
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

		/// B8: every check has an id, its name, that reads back
	void everyCheckHasAnId()
	{
		QSet<QString> ids;
		for (auto kind : ProjectCheck::kinds()) {
			const QString id = ProjectCheck::id(kind);
			QVERIFY(!id.isEmpty());
			QVERIFY(!ids.contains(id));
			ids << id;
			ProjectCheck::Kind back = ProjectCheck::Kind::DuplicateLabels;
			QVERIFY(ProjectCheck::kindFromId(id, &back));
			QCOMPARE(back, kind);
		}
		QCOMPARE(ProjectCheck::id(ProjectCheck::Kind::OpenWireEnds), QStringLiteral("OpenWireEnds"));
		QVERIFY(!ProjectCheck::kindFromId(QStringLiteral("NoSuchCheck"), nullptr));
	}

		/// B8: a finding's id is the same after the project is saved and
		/// opened again, in every format, and no two findings of a check
		/// share one. The fixtures have wires with no uuid of their own,
		/// which get a new one each time they are opened.
	void findingIdsLast_data()
	{
		QTest::addColumn<QString>("fixture");
		QTest::addColumn<QString>("ext");
		for (const char *name : {"every_feature.qet", "device_types.qet", "structure_box.qet",
								 "linked_reports.qet", "strip_labels.qet", "shared_symbol_uuid_0200.qet"})
			for (const char *ext : {"qet", "qetdb"})
				QTest::newRow(qPrintable(QStringLiteral("%1 %2").arg(QLatin1String(name), QLatin1String(ext))))
						<< QString::fromLatin1(name) << QString::fromLatin1(ext);
	}
	void findingIdsLast()
	{
		QFETCH(QString, fixture);
		QFETCH(QString, ext);
		const QString path = copy(fixture);
		QVERIFY(!path.isEmpty());
		const QString saved = m_dir.filePath(QStringLiteral("ids.") + ext);
		QHash<ProjectCheck::Kind, QStringList> before;
		int total = 0;
		{
			QETProject project(path);
			before = findingIds(project);
			project.setFilePath(saved);
			QVERIFY(project.write().isOk());
		}
		for (auto kind : ProjectCheck::kinds()) {
			const QStringList ids = before.value(kind);
			total += ids.size();
			QCOMPARE(QSet<QString>(ids.cbegin(), ids.cend()).size(), ids.size());
		}
		qInfo("%s: %d findings", qPrintable(fixture), total);
		QETProject again(saved);
		QCOMPARE(findingIds(again), before);
	}

		/// The same on the examples QElectroTech ships, which have far more
		/// findings, and wires and terminals with no uuid of their own
	void findingIdsLastOnTheExamples_data()
	{
		QTest::addColumn<QString>("example");
		const QDir examples(QStringLiteral(QET_EXAMPLES_DIR));
			// Between them every check with findings on the examples, wires
			// with no uuid, and the most findings (m_000: 89 duplicates)
		for (const char *name : {"industrial.qet", "tableau_domestique.qet", "Projet_vierge.qet",
								 "m_000.qet", "ShellyParts.qet", "2612_ats_singlephase.qet"})
			QTest::newRow(name) << examples.filePath(QString::fromLatin1(name));
	}
	void findingIdsLastOnTheExamples()
	{
		QFETCH(QString, example);
		const QString path = m_dir.filePath(QStringLiteral("example.qet"));
		QFile::remove(path);
		QVERIFY(QFile::copy(example, path));
		QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
		const QString saved = m_dir.filePath(QStringLiteral("example_saved.qet"));
		QHash<ProjectCheck::Kind, QStringList> before;
		{
			QETProject project(path);
			before = findingIds(project);
			project.setFilePath(saved);
			QVERIFY(project.write().isOk());
		}
		QStringList counts;
		for (auto kind : ProjectCheck::kinds()) {
			const QStringList ids = before.value(kind);
			counts << QStringLiteral("%1 %2").arg(ProjectCheck::id(kind)).arg(ids.size());
			QCOMPARE(QSet<QString>(ids.cbegin(), ids.cend()).size(), ids.size());
		}
		qInfo("%s", qPrintable(counts.join(QStringLiteral(", "))));
		QETProject again(saved);
		QCOMPARE(findingIds(again), before);
	}

		/// Two terminals of one symbol over the wire limit are two findings
		/// with an id each
	void wiresPerTerminalFindingsHaveTheirOwnIds()
	{
		const bool master = WiringRules::masterEnabled();
		const auto restore = qScopeGuard([master]() { WiringRules::setMasterEnabled(master); });
		WiringRules::setMasterEnabled(true);
		const QString path = m_dir.filePath(QStringLiteral("wires.qet"));
		QFile::remove(path);
		QVERIFY(QFile::copy(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), path));
		QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
		QETProject project(path);
		project.loadFolios();
		WiringRules::Settings one;
		one.own = true;
		one.max_wires = 1;
		project.setWiringRules(one);
		const auto kind = ProjectCheck::Kind::WiresPerTerminal;
		const auto result = ProjectCheck::run(&project, kind);
		QSet<QString> ids;
		QSet<QUuid> symbols;
		for (const auto &finding : result.findings) {
			ids << CheckSettings::findingId(kind, finding);
			symbols << finding.places.first().element;
		}
		qInfo("%d findings on %d symbols", int(result.findings.size()), int(symbols.size()));
		QVERIFY2(symbols.size() < result.findings.size(),
				 "no symbol has two terminals over the limit: the test cannot tell");
		QCOMPARE(ids.size(), int(result.findings.size()));
	}

		/// B8: a check switched off is kept by the project, in every
		/// format, and the panel says so instead of running it; on again
		/// stores nothing. The id of a check this QElectroTech does not
		/// know is kept.
	void aCheckSwitchedOffIsKept_data()
	{
		QTest::addColumn<QString>("file");
		for (const char *ext : {"qet", "qetz", "qetdb"})
			QTest::newRow(ext) << QStringLiteral("rules_off.") + QLatin1String(ext);
	}
	void aCheckSwitchedOffIsKept()
	{
		QFETCH(QString, file);
		const QString fixture = copy(QStringLiteral("every_feature.qet"));
		const QString path = m_dir.filePath(file);
		const auto kind = ProjectCheck::Kind::TerminalNames;
		{
			QETProject project(fixture);
			QVERIFY(CheckSettings::ruleEnabled(&project, kind));
			CheckSettings::setRuleEnabled(&project, kind, false);
			QVERIFY(!CheckSettings::ruleEnabled(&project, kind));
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QETProject project(path);
		QVERIFY(!CheckSettings::ruleEnabled(&project, kind));
		for (auto other : ProjectCheck::kinds())
			if (other != kind) QVERIFY(CheckSettings::ruleEnabled(&project, other));

		CheckPanelDockWidget panel;
		panel.setProject(&project);
		panel.run();
		QTreeWidgetItem *root = panel.checkRow(kind);
		QVERIFY(root);
		QCOMPARE(root->childCount(), 0);
		QVERIFY(root->text(0).contains(QLatin1String("switched off")));

			// Switched on again from the row's right-click menu
		bool triggered = false;
		QTimer::singleShot(0, this, [&triggered]() {
			if (auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget())) {
				for (QAction *action : menu->actions())
					if (action->text().contains(QLatin1String("on for this project"))) {
						action->trigger();
						triggered = true;
					}
				menu->close();
			}
		});
		panel.show();
		panel.tree()->scrollToItem(root);
		emit panel.tree()->customContextMenuRequested(panel.tree()->visualItemRect(root).center());
		QVERIFY(triggered);
		root = panel.checkRow(kind);
		QVERIFY(CheckSettings::ruleEnabled(&project, kind));
		QVERIFY(!root->text(0).contains(QLatin1String("switched off")));
		QVERIFY(!project.toXml().toString().contains(QLatin1String("check_rules_off")));

			// A newer QElectroTech's check is kept
		DiagramContext properties = project.projectProperties();
		properties.addValue(QStringLiteral("check_rules_off"), QStringLiteral("FutureCheck"));
		project.setProjectProperties(properties);
		CheckSettings::setRuleEnabled(&project, kind, false);
		QCOMPARE(project.projectProperties().value(QStringLiteral("check_rules_off")).toString(),
				 QStringLiteral("FutureCheck,") + ProjectCheck::id(kind));
		CheckSettings::setRuleEnabled(&project, kind, true);
		QCOMPARE(project.projectProperties().value(QStringLiteral("check_rules_off")).toString(),
				 QStringLiteral("FutureCheck"));
	}

		/// B8's "done when": a finding accepted in the panel stays hidden
		/// after the project is saved and opened again, shows with its
		/// note on request, and can be listed again
	void anAcceptedFindingStaysHidden_data()
	{
		QTest::addColumn<QString>("file");
		for (const char *ext : {"qet", "qetz", "qetdb"})
			QTest::newRow(ext) << QStringLiteral("accepted.") + QLatin1String(ext);
	}
	void anAcceptedFindingStaysHidden()
	{
		QFETCH(QString, file);
			// Options which find something on the fixture
		setMachine(false);
		const QString fixture = copy(QStringLiteral("every_feature.qet"));
		const QString path = m_dir.filePath(file);
		ProjectCheck::Kind kind = ProjectCheck::Kind::DuplicateLabels;
		int found = 0;
		QString text;
		{
			QETProject project(fixture);
			CheckPanelDockWidget panel;
			panel.setProject(&project);
			panel.run();
			found = panel.findingCount();
			QVERIFY2(found > 0, "the fixture has no finding to accept");
			QTreeWidgetItem *row = nullptr;
			for (auto candidate : ProjectCheck::kinds()) {
				QTreeWidgetItem *root = panel.checkRow(candidate);
				if (root && root->childCount()) {
					kind = candidate;
					row = root->child(0);
					break;
				}
			}
			QVERIFY(row);
			text = row->text(0);
			const int listed = openRows(row->parent());
			QVERIFY(panel.setAccepted(row, true, QStringLiteral("spare, wired on site")));
			QCOMPARE(panel.findingCount(), found - 1);
			QCOMPARE(openRows(panel.checkRow(kind)), listed - 1);
			QVERIFY(project.toXml().toString().contains(QLatin1String("spare, wired on site")));
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QETProject project(path);
		CheckPanelDockWidget panel;
		panel.setProject(&project);
		panel.run();
		QCOMPARE(panel.findingCount(), found - 1);
		QTreeWidgetItem *root = panel.checkRow(kind);
		QVERIFY(root->text(0).contains(QLatin1String("accepted")));
		QTreeWidgetItem *accepted = nullptr;
		for (int i = 0 ; i < root->childCount() ; ++i)
			if (root->child(i)->isHidden()) accepted = root->child(i);
		QVERIFY(accepted);

			// Shown on request, with its note
		QAction *show = panel.optionsMenu()->findChild<QAction *>(QStringLiteral("checkPanelShowAccepted"));
		QVERIFY(show);
		show->setChecked(true);
		QVERIFY(!accepted->isHidden());
		QVERIFY(accepted->text(0).contains(QLatin1String("spare, wired on site")));
		QVERIFY(accepted->text(0).startsWith(text));
		show->setChecked(false);
		QVERIFY(accepted->isHidden());

			// Listed again: nothing stored
		QVERIFY(panel.setAccepted(accepted, false));
		QVERIFY(!accepted->isHidden());
		QCOMPARE(panel.findingCount(), found);
		QVERIFY(!project.toXml().toString().contains(QLatin1String("check_accepted")));
	}

		/// B8 on Project properties > General: a box per check, switched
		/// off there and kept; the accepted findings are kept by the page
	void theGeneralPageSwitchesChecks()
	{
		QETProject project;
		project.addNewDiagram();
		CheckSettings::acceptFinding(&project, QStringLiteral("OpenWireEnds:abc"), QStringLiteral("ok"));
		ProjectMainConfigPage page(&project);
		for (auto kind : ProjectCheck::kinds()) {
			auto *box = page.findChild<QCheckBox *>(QStringLiteral("checkRule") + ProjectCheck::id(kind));
			QVERIFY(box);
			QVERIFY(box->isChecked());
		}
		auto *names = page.findChild<QCheckBox *>(
			QStringLiteral("checkRule") + ProjectCheck::id(ProjectCheck::Kind::TerminalNames));
		names->setChecked(false);
		page.applyProjectConf();
		QVERIFY(!CheckSettings::ruleEnabled(&project, ProjectCheck::Kind::TerminalNames));
		QVERIFY(CheckSettings::ruleEnabled(&project, ProjectCheck::Kind::OpenWireEnds));
		QCOMPARE(CheckSettings::acceptedFindings(&project).value(QStringLiteral("OpenWireEnds:abc")),
				 QStringLiteral("ok"));
		auto *variables = page.findChild<DiagramContextWidget *>();
		QVERIFY(!variables->context().contains(QStringLiteral("check_accepted")));
		names->setChecked(true);
		page.applyProjectConf();
		QVERIFY(!project.projectProperties().contains(QStringLiteral("check_rules_off")));
		QVERIFY(project.projectProperties().contains(QStringLiteral("check_accepted")));
	}
};

QET_APP_TEST_MAIN(tst_checksettings)
#include "tst_checksettings.moc"
