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
#include "qetapptest.h"
#include "cable/cable.h"
#include "diagram.h"
#include "devicetype/devicetypecheck.h"
#include "dataBase/projectdatabase.h"
#include "diagramevent/macrolabels.h"
#include "projectcheck.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "ui/checkpaneldockwidget.h"
#include "ui/openwireendsdialog.h"
#include "utils/qetsettings.h"
#include "wiringrules.h"

#include <QDomDocument>
#include <QFile>
#include <QScopeGuard>
#include <QSet>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>

/**
	The Check panel (#1878 B1): every check QElectroTech has, run at once,
	with one shape of finding each can be shown from.
*/
class tst_projectcheck : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	QString copy(const QString &fixture)
	{
		const QString path = m_dir.filePath(fixture);
		QFile::remove(path);
		if (!QFile::copy(QFINDTESTDATA(QStringLiteral("fixtures/") + fixture), path)) return {};
		QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
		return path;
	}

	static ProjectCheck::Result run(QETProject &project, ProjectCheck::Kind kind)
	{
		project.loadFolios();
		return ProjectCheck::run(&project, kind);
	}

	/// The terminals of the project's symbols with more than @p limit
	/// wires, counted on the folios, not in the database. A folio report
	/// has the same limit while one_wire_per_report is off
	static int overLimit(QETProject &project, int limit)
	{
		int count = 0;
		for (Diagram *diagram : project.diagrams())
			for (Element *element : diagram->elements())
				for (Terminal *terminal : element->terminals())
					if (terminal->conductors().size() > limit) ++count;
		return count;
	}

		/// The symbols with terminals but no wire, found on the folios,
		/// with the panel's options: what unwiredSymbols() must list
		static QSet<QUuid> unwired(QETProject &project, bool wireless, bool strip, bool contacts)
		{
			QSet<QUuid> found;
			for (Diagram *diagram : project.diagrams()) {
				if (wireless && diagram->conductors().isEmpty()) continue;
				for (Element *element : diagram->elements()) {
					if (element->terminals().isEmpty()) continue;
					if (element->linkType() & Element::AllReport) continue;
					if (strip && element->linkType() == Element::Terminale) continue;
					if (contacts && element->linkType() == Element::Slave) continue;
					bool wired = false;
					for (Terminal *terminal : element->terminals())
						wired |= !terminal->conductors().isEmpty();
					if (!wired) found << element->uuid();
				}
			}
			return found;
		}

		/// Set the three options of symbols with no wire
		static void setUnwiredOptions(bool wireless, bool strip, bool contacts)
		{
			QetSettings::setUnwiredSymbolsSkipWirelessFolios(wireless);
			QetSettings::setUnwiredSymbolsSkipStripTerminals(strip);
			QetSettings::setUnwiredSymbolsSkipContacts(contacts);
		}

		static QSet<QUuid> listed(const ProjectCheck::Result &result)
		{
			QSet<QUuid> uuids;
			for (const auto &finding : result.findings)
				for (const auto &place : finding.places) uuids << place.element;
			return uuids;
		}

	private slots:
		/// A label or text still holding a ? or a ?name is listed, once for
		/// each item, and answering it takes it off the list (#1878 B7)
		void unansweredPromptsFindsWhatAMacroAsks()
		{
			QVERIFY(MacroLabels::holdsPrompt(QStringLiteral("-K?")));
			QVERIFY(MacroLabels::holdsPrompt(QStringLiteral("?motor_kw kW")));
			QVERIFY(!MacroLabels::holdsPrompt(QStringLiteral("-K1")));

			const QString path = copy(QStringLiteral("every_feature.qet"));
			QVERIFY(!path.isEmpty());
			QETProject project(path);
			project.loadFolios();
			const QSet<QUuid> before = listed(run(project, ProjectCheck::Kind::UnansweredPrompts));

			QList<Element *> symbols;
			for (Diagram *diagram : project.diagrams())
				for (Element *e : diagram->elements())
					if (!before.contains(e->uuid()) && symbols.size() < 2) symbols << e;
			QCOMPARE(symbols.size(), 2);
			const auto setInfo = [](Element *e, const QString &key, const QString &value) {
				DiagramContext info = e->elementInformations();
					// A label typed by hand no longer follows a formula
				if (key == QLatin1String("label")) info.addValue(QStringLiteral("formula"), QString());
				info.addValue(key, value);
				e->setElementInformations(info);
			};
			setInfo(symbols.at(0), QStringLiteral("label"), QStringLiteral("-K?"));
				// Two texts of one symbol are one finding
			setInfo(symbols.at(1), QStringLiteral("comment"), QStringLiteral("?motor_kw kW"));
			setInfo(symbols.at(1), QStringLiteral("function"), QStringLiteral("?motor_kw"));

			auto result = ProjectCheck::run(&project, ProjectCheck::Kind::UnansweredPrompts);
				// A symbol linked to another may take its label along: the
				// two planted are there, whatever else is
			QSet<QUuid> found = listed(result);
			QVERIFY(found.contains(symbols.at(0)->uuid()));
			QVERIFY(found.contains(symbols.at(1)->uuid()));
			int own = 0;
			for (const auto &finding : result.findings) {
				QVERIFY(finding.places.first().canShow());
				if (finding.places.first().element == symbols.at(1)->uuid()) ++own;
			}
			QCOMPARE(own, 1);

			setInfo(symbols.at(0), QStringLiteral("label"), QStringLiteral("-K1"));
			found = listed(ProjectCheck::run(&project, ProjectCheck::Kind::UnansweredPrompts));
			QVERIFY(!found.contains(symbols.at(0)->uuid()));
			QVERIFY(found.contains(symbols.at(1)->uuid()));
		}

		/// The symbols with no part number are those the parts list would
		/// hold: not a report, not set to be left out, not a junction;
		/// once a part number is given, the symbol is not listed (#1878 B6)
		void noPartNumberListsWhatThePartsListHolds()
		{
			const QString path = copy(QStringLiteral("every_feature.qet"));
			QVERIFY(!path.isEmpty());
			QETProject project(path);
			project.loadFolios();

			const auto text = [](Element *e, const QString &key) {
				return e->elementInformations().value(key).toString().trimmed();
			};
				// What the parts list holds, found on the folios
			const auto expected = [&]() {
				QSet<QUuid> set;
				for (Diagram *diagram : project.diagrams())
					for (Element *e : diagram->elements()) {
						const auto type = e->linkType();
						if (!(type & (Element::Simple | Element::Master | Element::Slave | Element::Terminale)))
							continue;
						if (QET::infoFlagIsTrue(text(e, QStringLiteral("exclude_from_bom")))) continue;
						if (!text(e, QStringLiteral("manufacturer_reference")).isEmpty()) continue;
						if (type == Element::Terminale
							&& text(e, QStringLiteral("label")).isEmpty()
							&& text(e, QStringLiteral("designation")).isEmpty()
							&& text(e, QStringLiteral("manufacturer")).isEmpty()) continue;
						set << e->uuid();
					}
				return set;
			};
			const auto set = [](Element *e, const QString &key, const QString &value) {
				DiagramContext info = e->elementInformations();
				info.addValue(key, value);
				e->setElementInformations(info);
			};

			auto result = run(project, ProjectCheck::Kind::NoPartNumber);
			QSet<QUuid> before = expected();
			QVERIFY(before.size() >= 2);
			QCOMPARE(listed(result), before);

				// One given a part number, one set to be left out
			QList<Element *> two;
			for (Diagram *diagram : project.diagrams())
				for (Element *e : diagram->elements())
					if (before.contains(e->uuid()) && two.size() < 2) two << e;
			set(two.at(0), QStringLiteral("manufacturer_reference"), QStringLiteral("PN-1"));
			set(two.at(1), QStringLiteral("exclude_from_bom"), QStringLiteral("true"));
			result = ProjectCheck::run(&project, ProjectCheck::Kind::NoPartNumber);
			QSet<QUuid> after = before;
			after.remove(two.at(0)->uuid());
			after.remove(two.at(1)->uuid());
			QCOMPARE(expected(), after);
			QCOMPARE(listed(result), after);
		}

		/// A wire that is a core of two cables is listed once, with both
		/// cables, and a wire of one cable is not (#1878 B5)
		void wireInTwoCablesFindsTheSharedWire()
		{
			const QString path = copy(QStringLiteral("shared_symbol_uuid_0200.qet"));
			QVERIFY(!path.isEmpty());
			QETProject project(path);
			project.loadFolios();
			QList<Conductor *> wires;
			for (Diagram *diagram : project.diagrams())
				wires << diagram->conductors();
			QVERIFY(wires.size() >= 2);

			QVERIFY(ProjectCheck::run(&project, ProjectCheck::Kind::WireInTwoCables).findings.isEmpty());

			const auto makeCable = [&project](const QString &designation, const QList<Conductor *> &used) {
				auto *cable = new Cable(&project);
				cable->setDesignation(designation);
				cable->setCoreCount(3);
				project.addCable(cable);
				for (int i = 0 ; i < used.size() ; ++i) {
					CableCore core;
					core.core = i;
					core.conductor = used.at(i)->uuid();
					cable->setCore(core);
				}
			};
			makeCable(QStringLiteral("W1"), {wires.at(0), wires.at(1)});
			makeCable(QStringLiteral("W2"), {wires.at(0)});

			const auto result = ProjectCheck::run(&project, ProjectCheck::Kind::WireInTwoCables);
			QCOMPARE(result.findings.size(), 1);
			const auto &finding = result.findings.first();
			QVERIFY2(finding.text.contains(QStringLiteral("W1, W2")), qPrintable(finding.text));
			QCOMPARE(finding.places.size(), 1);
			QCOMPARE(finding.places.first().wires, QList<QUuid>{wires.at(0)->uuid()});
			QVERIFY(finding.places.first().canShow());
		}

		/// A limit of 1 lists exactly the terminals the folios show with
		/// two wires or more; no limit gives a note and no finding
		void wiresPerTerminalMatchesTheFolios()
		{
			const bool master = WiringRules::masterEnabled();
			const auto restore = qScopeGuard([master]() { WiringRules::setMasterEnabled(master); });
			const QString path = copy(QStringLiteral("shared_symbol_uuid_0200.qet"));
			QVERIFY(!path.isEmpty());
			QETProject project(path);
			project.loadFolios();

			WiringRules::setMasterEnabled(true);
			WiringRules::Settings none;
			none.own = true;
			project.setWiringRules(none);
			auto result = ProjectCheck::run(&project, ProjectCheck::Kind::WiresPerTerminal);
			QVERIFY(!result.note.isEmpty());
			QVERIFY(result.findings.isEmpty());

			WiringRules::Settings one = none;
			one.max_wires = 1;
			project.setWiringRules(one);
			result = ProjectCheck::run(&project, ProjectCheck::Kind::WiresPerTerminal);
			QVERIFY(result.note.isEmpty());
			const int expected = overLimit(project, 1);
			QVERIFY2(expected > 0, "the fixture has no terminal with two wires");
			QCOMPARE(int(result.findings.size()), expected);
			for (const auto &finding : result.findings) {
				QCOMPARE(finding.places.size(), 1);
				QVERIFY(!finding.places.first().element.isNull());
				QVERIFY(!finding.places.first().folio.isEmpty());
			}

			// The master switch off: no limit at all
			WiringRules::setMasterEnabled(false);
			result = ProjectCheck::run(&project, ProjectCheck::Kind::WiresPerTerminal);
			QVERIFY(!result.note.isEmpty());
		}

		/// A symbol whose definition names two terminals alike is found
		/// once, with every placed copy of it; the fixture as shipped has none
		void terminalNamesFindsARepeatedName()
		{
			const QString path = copy(QStringLiteral("every_feature.qet"));
			QVERIFY(!path.isEmpty());
			{
				QETProject project(path);
				QVERIFY(run(project, ProjectCheck::Kind::TerminalNames).findings.isEmpty());
			}

				// Name two terminals of the first embedded symbol which has two "X9"
			QFile file(path);
			QVERIFY(file.open(QIODevice::ReadOnly));
			QDomDocument document;
			QVERIFY(bool(document.setContent(&file)));
			file.close();
			QString definition_name;
			const QDomNodeList definitions = document.elementsByTagName(QStringLiteral("definition"));
			for (int i = 0 ; i < definitions.count() && definition_name.isEmpty() ; ++i) {
				QDomElement definition = definitions.at(i).toElement();
				if (definition.attribute(QStringLiteral("type")) != QLatin1String("element")) continue;
				const QDomNodeList terminals = definition.elementsByTagName(QStringLiteral("terminal"));
				if (terminals.count() < 2) continue;
				terminals.at(0).toElement().setAttribute(QStringLiteral("name"), QStringLiteral("X9"));
				terminals.at(1).toElement().setAttribute(QStringLiteral("name"), QStringLiteral("X9"));
				definition_name = definition.parentNode().toElement().attribute(QStringLiteral("name"));
			}
			QVERIFY(!definition_name.isEmpty());
			QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
			file.write(document.toByteArray());
			file.close();

			QETProject project(path);
			const auto result = run(project, ProjectCheck::Kind::TerminalNames);
			QCOMPARE(result.findings.size(), 1);
			QVERIFY2(result.findings.first().text.contains(QStringLiteral("X9")),
					 qPrintable(result.findings.first().text));
			int placed = 0;
			for (Diagram *diagram : project.diagrams())
				for (Element *element : diagram->elements())
					if (element->location().path().endsWith(definition_name)) ++placed;
			QCOMPARE(int(result.findings.first().places.size()), placed);
		}

		/// The device type findings are the devices the check finds not
		/// complete, or with too many contacts
		void deviceTypesMatchesTheCheck()
		{
			const QString path = copy(QStringLiteral("device_types.qet"));
			QVERIFY(!path.isEmpty());
			QETProject project(path);
			const auto result = run(project, ProjectCheck::Kind::DeviceTypes);
			QList<Element *> unlinked;
			int expected = 0;
			for (const auto &device : DeviceTypeCheck::check(&project, &unlinked))
				if (!device.complete() || DeviceTypeCheck::extraContacts(device)) ++expected;
			if (!unlinked.isEmpty()) ++expected;
			QCOMPARE(int(result.findings.size()), expected);
		}

		/// The open wire ends are B2's, worded as its window words them: a
		/// folio report unlinked here is found, and its row selects the
		/// report and its wire on its folio
		void openWireEndsFindsAnUnlinkedReport()
		{
			const QString path = copy(QStringLiteral("every_feature.qet"));
			QVERIFY(!path.isEmpty());
			QETProject project(path);
			QVERIFY(run(project, ProjectCheck::Kind::OpenWireEnds).findings.isEmpty());

			Element *report = nullptr;
			for (Diagram *diagram : project.diagrams())
				for (Element *element : diagram->elements())
					if (!report && (element->linkType() & Element::AllReport)
						&& !element->linkedElements().isEmpty()
						&& !element->terminals().isEmpty()
						&& !element->terminals().first()->conductors().isEmpty())
						report = element;
			QVERIFY2(report, "the fixture has no linked report with a wire");
			report->unlinkAllElements();

			const auto ends = project.dataBase()->openWireEnds();
			const auto result = ProjectCheck::run(&project, ProjectCheck::Kind::OpenWireEnds);
			QVERIFY(!result.findings.isEmpty());
			QCOMPARE(int(result.findings.size()), int(ends.size()));
			int row = -1;
			for (int i = 0 ; i < ends.size() ; ++i) {
				QVERIFY(result.findings.at(i).text.startsWith(OpenWireEndsDialog::problemText(ends.at(i))));
				QCOMPARE(result.findings.at(i).places.first().wires, ends.at(i).wires);
				if (ends.at(i).element == report->uuid()) row = i;
			}
			QVERIFY2(row >= 0, "the unlinked report is not found");

			CheckPanelDockWidget panel;
			panel.setProject(&project);
			panel.run();
			const int check = int(ProjectCheck::kinds().indexOf(ProjectCheck::Kind::OpenWireEnds));
			QTreeWidgetItem *finding = panel.tree()->topLevelItem(check)->child(row);
			QVERIFY(finding);
			for (Diagram *diagram : project.diagrams()) diagram->clearSelection();
			QVERIFY(panel.showRow(finding));
			QVERIFY(report->isSelected());
			QVERIFY(report->terminals().first()->conductors().first()->isSelected());
		}

		/// Symbols with no wire lists what the folios show, one finding
		/// each, with every combination of its three options
		void unwiredSymbolsMatchesTheFolios_data()
		{
			QTest::addColumn<QString>("fixture");
			for (const char *name : {"every_feature.qet", "device_types.qet", "structure_box.qet",
									 "linked_reports.qet", "strip_labels.qet", "bridge_tracks.qet"})
				QTest::newRow(name) << QString::fromLatin1(name);
		}
		void unwiredSymbolsMatchesTheFolios()
		{
			QFETCH(QString, fixture);
			const auto restore = qScopeGuard([]() { setUnwiredOptions(true, true, false); });
			const QString path = copy(fixture);
			QVERIFY(!path.isEmpty());
			QETProject project(path);
			project.loadFolios();
			for (int options = 0 ; options < 8 ; ++options) {
				const bool wireless = options & 1, strip = options & 2, contacts = options & 4;
				setUnwiredOptions(wireless, strip, contacts);
				const auto result = ProjectCheck::run(&project, ProjectCheck::Kind::UnwiredSymbols);
				const QSet<QUuid> expected = unwired(project, wireless, strip, contacts);
				qInfo("%s options %d: %d", qPrintable(fixture), options, int(expected.size()));
				QCOMPARE(int(result.findings.size()), int(expected.size()));
				QCOMPARE(listed(result), expected);
			}
		}

		/// A symbol whose wires are taken out of the file is found, its
		/// row selects it; on a folio with no wire left, it is left out
		/// until that option is off
		void unwiredSymbolsFindsAPlantedSymbol()
		{
			const auto restore = qScopeGuard([]() { setUnwiredOptions(true, true, false); });
			setUnwiredOptions(true, true, false);
			const QString path = copy(QStringLiteral("every_feature.qet"));
			QVERIFY(!path.isEmpty());
			QUuid planted;
			QUuid alone_folio;
			{
				QETProject project(path);
				project.loadFolios();
				QVERIFY(ProjectCheck::run(&project, ProjectCheck::Kind::UnwiredSymbols).findings.isEmpty());
					// A plain symbol with a wire, on a folio with a wire
					// it is not on: that wire stays
				for (Diagram *diagram : project.diagrams())
					for (Element *element : diagram->elements()) {
						if (!planted.isNull() || element->linkType() != Element::Simple) continue;
						bool own = false, other = false;
						for (Conductor *conductor : diagram->conductors()) {
							const bool mine = conductor->terminal1->parentElement() == element
											  || conductor->terminal2->parentElement() == element;
							(mine ? own : other) = true;
						}
						if (own && other) planted = element->uuid();
					}
			}
			QVERIFY2(!planted.isNull(), "the fixture has no plain symbol to plant");

			QFile file(path);
			QVERIFY(file.open(QIODevice::ReadOnly));
			QDomDocument document;
			QVERIFY(bool(document.setContent(&file)));
			file.close();
			const QDomNodeList conductors = document.elementsByTagName(QStringLiteral("conductor"));
			QList<QDomElement> gone;
			for (int i = 0 ; i < conductors.count() ; ++i) {
				const QDomElement conductor = conductors.at(i).toElement();
				if (QUuid(conductor.attribute(QStringLiteral("element1"))) == planted
					|| QUuid(conductor.attribute(QStringLiteral("element2"))) == planted)
					gone << conductor;
			}
			QVERIFY2(!gone.isEmpty(), "the planted symbol's wires do not name it");
			for (QDomElement &conductor : gone) conductor.parentNode().removeChild(conductor);
			QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
			file.write(document.toByteArray());
			file.close();

			QETProject project(path);
			project.loadFolios();
			const auto result = ProjectCheck::run(&project, ProjectCheck::Kind::UnwiredSymbols);
				// With the symbol at the other end of its wire, if that was its only one
			QCOMPARE(listed(result), unwired(project, true, true, false));
			QVERIFY(listed(result).contains(planted));
			int row = -1;
			for (int i = 0 ; i < result.findings.size() ; ++i)
				if (result.findings.at(i).places.first().element == planted) row = i;

			CheckPanelDockWidget panel;
			panel.setProject(&project);
			panel.run();
			const int check = int(ProjectCheck::kinds().indexOf(ProjectCheck::Kind::UnwiredSymbols));
			QTreeWidgetItem *finding = panel.tree()->topLevelItem(check)->child(row);
			QVERIFY(finding);
			QVERIFY(!finding->text(1).isEmpty());
			QVERIFY(panel.showRow(finding));
			Element *shown = nullptr;
			for (Diagram *diagram : project.diagrams())
				for (QGraphicsItem *item : diagram->selectedItems())
					if (auto *element = qgraphicsitem_cast<Element *>(item)) shown = element;
			QVERIFY(shown);
			QCOMPARE(shown->uuid(), planted);

				// The options menu saves its choice
			QAction *wireless = panel.optionsMenu()->findChild<QAction *>(QStringLiteral("unwiredSkipWirelessFolios"));
			QVERIFY(wireless);
			QVERIFY(wireless->isChecked());
			wireless->setChecked(false);
			QVERIFY(!QetSettings::unwiredSymbolsSkipWirelessFolios());
		}

		/// Every check runs on every fixture without a crash, and every
		/// place which says it can be shown names something to show
		void everyCheckRunsOnEveryFixture_data()
		{
			QTest::addColumn<QString>("fixture");
			for (const char *name : {"every_feature.qet", "device_types.qet", "structure_box.qet",
									 "linked_reports.qet", "strip_labels.qet"})
				QTest::newRow(name) << QString::fromLatin1(name);
		}
		void everyCheckRunsOnEveryFixture()
		{
			QFETCH(QString, fixture);
			const QString path = copy(fixture);
			QVERIFY(!path.isEmpty());
			QETProject project(path);
			for (auto kind : ProjectCheck::kinds()) {
				QVERIFY(!ProjectCheck::title(kind).isEmpty());
				const auto result = run(project, kind);
				QCOMPARE(result.kind, kind);
				for (const auto &finding : result.findings) {
					QVERIFY(!finding.text.isEmpty());
					for (const auto &place : finding.places)
						if (place.canShow())
							QVERIFY(place.item || !place.element.isNull() || !place.wires.isEmpty());
				}
			}
		}

		/// The panel lists one row per check, counts the findings, and
		/// a finding's row selects its symbol on its folio
		void panelRunsAndShows()
		{
			const bool master = WiringRules::masterEnabled();
			const auto restore = qScopeGuard([master]() { WiringRules::setMasterEnabled(master); });
			WiringRules::setMasterEnabled(true);
			const QString path = copy(QStringLiteral("shared_symbol_uuid_0200.qet"));
			QVERIFY(!path.isEmpty());
			QETProject project(path);
			WiringRules::Settings one;
			one.own = true;
			one.max_wires = 1;
			project.setWiringRules(one);

			CheckPanelDockWidget panel;
			panel.setProject(&project);
			QCOMPARE(panel.tree()->topLevelItemCount(), 0);
			panel.run();
			QCOMPARE(panel.tree()->topLevelItemCount(), int(ProjectCheck::kinds().size()));
			QVERIFY(panel.findingCount() >= overLimit(project, 1));

			const int wires_row = int(ProjectCheck::kinds().indexOf(ProjectCheck::Kind::WiresPerTerminal));
			QTreeWidgetItem *check = panel.tree()->topLevelItem(wires_row);
			QVERIFY(check->childCount() > 0);
			QTreeWidgetItem *finding = check->child(0);
			QVERIFY(!finding->text(1).isEmpty());
			QVERIFY(panel.showRow(finding));
			int selected = 0;
			for (Diagram *diagram : project.diagrams())
				selected += int(diagram->selectedItems().size());
			QCOMPARE(selected, 1);
			QVERIFY(!panel.showRow(check));     // a check's own row shows nothing

			// Another project clears the results
			panel.setProject(nullptr);
			QCOMPARE(panel.tree()->topLevelItemCount(), 0);
			QCOMPARE(panel.findingCount(), 0);
		}

		/// Another project made current while the folios are built (the
		/// progress dialog processes events) clears the panel; the old
		/// project's results are not listed
		void panelFollowsAProjectChangedWhileBuilding()
		{
			const QString path = copy(QStringLiteral("every_feature.qet"));
			QVERIFY(!path.isEmpty());
			const QString other_path = copy(QStringLiteral("device_types.qet"));
			QVERIFY(!other_path.isEmpty());
			QETProject project(path);
			QETProject other(other_path);
			QVERIFY2(project.unloadedFolioCount() > 0, "the folios are built already");
			CheckPanelDockWidget panel;
			panel.setProject(&project);
			QTimer::singleShot(0, &panel, [&panel, &other]() { panel.setProject(&other); });
			panel.run();
			QCOMPARE(panel.project(), &other);
			QCOMPARE(panel.tree()->topLevelItemCount(), 0);
			QCOMPARE(panel.findingCount(), 0);

				// The same, to no project (a copy whose folios are not built)
			QETProject again(path);
			panel.setProject(&again);
			QTimer::singleShot(0, &panel, [&panel]() { panel.setProject(nullptr); });
			panel.run();
			QCOMPARE(panel.project(), nullptr);
			QCOMPARE(panel.tree()->topLevelItemCount(), 0);
		}

		/// A potential-types finding, read from the database, selects its
		/// wires on their folio, found by uuid
		void panelShowsWiresOfAPotential()
		{
			const QString path = copy(QStringLiteral("every_feature.qet"));
			QVERIFY(!path.isEmpty());
			QETProject project(path);
			project.loadFolios();
			Conductor *wire = nullptr;
			for (Diagram *diagram : project.diagrams())
				for (Conductor *conductor : diagram->conductors())
					if (!wire && !conductor->relatedPotentialConductors().isEmpty())
						wire = conductor;
			QVERIFY(wire);
			Conductor *other = *wire->relatedPotentialConductors().constBegin();
			const auto typeOf = [](Conductor *w, const QString &type) {
				ConductorProperties p = w->properties();
				p.m_potential_type = type;
				w->setProperties(p);
			};
			typeOf(wire, QStringLiteral("N"));
			typeOf(other, QStringLiteral("PE"));

			CheckPanelDockWidget panel;
			panel.setProject(&project);
			panel.run();
			const int row = int(ProjectCheck::kinds().indexOf(ProjectCheck::Kind::PotentialTypes));
			QTreeWidgetItem *check = panel.tree()->topLevelItem(row);
			QCOMPARE(check->childCount(), 1);
			for (Diagram *diagram : project.diagrams()) diagram->clearSelection();
			QVERIFY(panel.showRow(check->child(0)));
			QVERIFY(wire->isSelected() || other->isSelected());
			QVERIFY(!wire->diagram()->selectedItems().isEmpty());
		}
};

QET_APP_TEST_MAIN(tst_projectcheck)
#include "tst_projectcheck.moc"
