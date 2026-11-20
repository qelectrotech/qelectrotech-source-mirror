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
#include "diagram.h"
#include "diagramcontext.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "undocommand/dissolvereportlinkscommand.h"
#include "undocommand/reportlinkkeycommand.h"
#include <QPropertyUndoCommand/qpropertyundocommand.h>

#include <QFile>
#include <QTemporaryDir>

/**
	Chained folio reports: what two reports must share to be chained
	together is the project's choice (QETProject::ReportLinkKey) -- the
	label each of them carries, or the function of the wire its terminal
	holds. A report without a value of its own is never chained, and a
	report never follows the other key than the one the project chose.
	The fixture holds one report going to the next folio and one coming
	from the previous, each on a folio of its own beside a terminal the
	test wires it to.
*/
class tst_chainedreportkey : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	/// The fixture: one report going to the next folio and one coming
	/// from the previous, each beside a terminal to wire it to
	static QString fixture()
	{
		return QFINDTESTDATA("fixtures/chained_report_keys.qet");
	}

	/// The report of the fixture which goes to the next folio and the one
	/// which comes from the previous
	QPair<Element *, Element *> reports(QETProject &project)
	{
		Element *go = nullptr, *come = nullptr;
		for (Diagram *diagram : project.diagrams())
			for (Element *element : diagram->elements()) {
				if (element->linkType() == Element::NextReport) go = element;
				else if (element->linkType() == Element::PreviousReport)
					come = element;
			}
		return qMakePair(go, come);
	}

	/// The symbol with this uuid
	static Element *elementByUuid(QETProject &project, const QString &uuid)
	{
		for (Diagram *diagram : project.diagrams())
			for (Element *element : diagram->elements())
				if (element->uuid().toString() == uuid)
					return element;
		return nullptr;
	}

	/// Give @p element the label the reports are chained by
	void setLabel(Element *element, const QString &label)
	{
		DiagramContext infos = element->elementInformations();
		infos.addValue(QStringLiteral("label"), label);
		element->setElementInformations(infos);
	}

	/// Draw a wire from @p report to the terminal beside it, carrying
	/// @p function as the wire the report reads its texts from
	Conductor *wireReport(Element *report, const QString &function)
	{
		Diagram *diagram = report->diagram();
		if (!diagram) return nullptr;
		Terminal *second = nullptr;
		for (Element *element : diagram->elements()) {
			if (element == report) continue;
			for (Terminal *terminal : element->terminals()) {
				if (terminal->conductors().isEmpty()) {
					second = terminal;
					break;
				}
			}
			if (second) break;
		}
		if (!second) return nullptr;
		auto *conductor = new Conductor(report->terminals().first(), second);
		diagram->addItem(conductor);
		ConductorProperties properties = conductor->properties();
		properties.m_function = function;
		conductor->setProperties(properties);
		return conductor;
	}

	/// Every report link of this project, as "uuid of the one -> uuid of
	/// the other", so a test can say what chains the project has
	QStringList links(QETProject &project)
	{
		QStringList found;
		for (Diagram *diagram : project.diagrams())
			for (Element *element : diagram->elements())
				if (!element->isFree())
					found << element->uuid().toString()
							 + QStringLiteral(" -> ")
							 + element->linkedElements().first()->uuid().toString();
		found.sort();
		return found;
	}

	private slots:
		/// The label of the reports is what chains them, and reports with
		/// the same one are chained across their folios
		void labelKeyChainsMatchingLabels()
		{
			QETProject project(fixture());
			QCOMPARE(project.state(), QETProject::Ok);
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			setLabel(pair.first, QStringLiteral("K1"));
			setLabel(pair.second, QStringLiteral("K1"));
			project.setChainedReportsKey(QETProject::ReportLinkKey::Label);
			project.setChainedReports(true);
			QTRY_VERIFY(!pair.first->isFree());
			QCOMPARE(pair.first->linkedElements().value(0), pair.second);
		}

		/// Reports whose labels differ are left alone
		void labelKeyLeavesDifferentLabelsAlone()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			setLabel(pair.first, QStringLiteral("K1"));
			setLabel(pair.second, QStringLiteral("K2"));
			project.setChainedReportsKey(QETProject::ReportLinkKey::Label);
			project.setChainedReports(true);
			QTRY_VERIFY(pair.first->isFree());
			QTRY_VERIFY(pair.second->isFree());
		}

		/// The wire's function is not what chains the reports when the
		/// project chains by the label
		void labelKeyIgnoresTheWireFunction()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			QVERIFY(wireReport(pair.first, QStringLiteral("K1")));
			QVERIFY(wireReport(pair.second, QStringLiteral("K1")));
			project.setChainedReportsKey(QETProject::ReportLinkKey::Label);
			project.setChainedReports(true);
			QTRY_VERIFY(pair.first->isFree());
		}

		/// The function of the wire a report's terminal holds is what
		/// chains it, and two wires carrying the same one chain the
		/// reports across their folios
		void functionKeyChainsMatchingFunctions()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			QVERIFY(wireReport(pair.first, QStringLiteral("K1")));
			QVERIFY(wireReport(pair.second, QStringLiteral("K1")));
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			project.setChainedReports(true);
			QTRY_VERIFY(!pair.first->isFree());
			QCOMPARE(pair.first->linkedElements().value(0), pair.second);
		}

		/// Wires whose functions differ leave the reports alone
		void functionKeyLeavesDifferentFunctionsAlone()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			QVERIFY(wireReport(pair.first, QStringLiteral("K1")));
			QVERIFY(wireReport(pair.second, QStringLiteral("K2")));
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			project.setChainedReports(true);
			QTRY_VERIFY(pair.first->isFree());
		}

		/// A report without a wire of its own has no function to chain by
		void functionKeyNeedsAWireOnBothReports()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			QVERIFY(wireReport(pair.first, QStringLiteral("K1")));
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			project.setChainedReports(true);
			QTRY_VERIFY(pair.first->isFree());
		}

		/// The label is not what chains the reports when the project
		/// chains by the wire's function
		void functionKeyIgnoresTheLabel()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			setLabel(pair.first, QStringLiteral("K1"));
			setLabel(pair.second, QStringLiteral("K1"));
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			project.setChainedReports(true);
			QTRY_VERIFY(pair.first->isFree());
		}

		/// A wire drawn and typed after the chains were made is seen to:
		/// the function of a wire is the key of this project
		void functionKeyFollowsAFunctionSetLater()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			project.setChainedReports(true);
			QTRY_VERIFY(pair.first->isFree());
			QVERIFY(wireReport(pair.first, QStringLiteral("K1")));
			QVERIFY(wireReport(pair.second, QStringLiteral("K1")));
			QTest::qWait(50);
			QTRY_VERIFY(!pair.first->isFree());
			QCOMPARE(pair.first->linkedElements().value(0), pair.second);
		}

		/// A wire given another function takes its report's chain with it:
		/// the link the two had does not describe the drawing any more and
		/// is made again from the new value
		void functionKeyChainsAgainWhenAFunctionChanges()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			QVERIFY(wireReport(pair.first, QStringLiteral("K1")));
			Conductor *second_wire = wireReport(pair.second, QStringLiteral("K1"));
			QVERIFY(second_wire);
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			project.setChainedReports(true);
			QTRY_VERIFY(!pair.first->isFree());

			ConductorProperties properties = second_wire->properties();
			properties.m_function = QStringLiteral("K2");
			second_wire->setProperties(properties);
			QTest::qWait(50);
			QTRY_VERIFY(pair.first->isFree());
			QTRY_VERIFY(pair.second->isFree());

				//... and back again: the chain is made anew
			properties.m_function = QStringLiteral("K1");
			second_wire->setProperties(properties);
			QTest::qWait(50);
			QTRY_VERIFY(!pair.first->isFree());
			QCOMPARE(pair.first->linkedElements().value(0), pair.second);
		}

		/// The wire carries the chain: taken away, the link it held goes
		void functionKeyDropsTheLinkWhenTheWireGoes()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			Conductor *first_wire = wireReport(pair.first, QStringLiteral("K1"));
			QVERIFY(first_wire);
			QVERIFY(wireReport(pair.second, QStringLiteral("K1")));
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			project.setChainedReports(true);
			QTRY_VERIFY(!pair.first->isFree());

				//The way the editor deletes a wire
			pair.first->diagram()->removeItem(first_wire);
			delete first_wire;
			QTest::qWait(50);
			QTRY_VERIFY(pair.first->isFree());
			QTRY_VERIFY(pair.second->isFree());
		}

		/// A name written over a potential stays on the folio it is
		/// written on: the folio report link goes before the name is
		/// written (the wires it joins need not carry the same name), and
		/// the chaining looks for a link again afterwards
		void functionRenameStaysOnItsOwnFolio()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			Conductor *first_wire = wireReport(pair.first, QStringLiteral("K2"));
			Conductor *second_wire = wireReport(pair.second, QStringLiteral("K2"));
			QVERIFY(first_wire && second_wire);
				//The user links the two by hand
			pair.first->linkToElement(pair.second);
			QTRY_VERIFY(!pair.first->isFree());
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			project.setChainedReports(true);

				//What the conductor dialog does when the user ticks
				//"apply to all conductors of this potential"
			ConductorProperties properties = first_wire->properties();
			properties.m_function = QStringLiteral("K3");
			first_wire->setPropertyToPotential(properties);
			QTest::qWait(50);

			QCOMPARE(first_wire->properties().m_function, QStringLiteral("K3"));
			QCOMPARE(second_wire->properties().m_function, QStringLiteral("K2"));
			QTRY_VERIFY(pair.first->isFree());
			QTRY_VERIFY(pair.second->isFree());
		}

		/// The same for a link the chaining made: a name written over a
		/// potential stays on its folio whatever the link came from, and
		/// the chaining makes the links again from the names
		void functionRenameStaysOnItsOwnFolioWithAChainedLink()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			Conductor *first_wire = wireReport(pair.first, QStringLiteral("K2"));
			Conductor *second_wire = wireReport(pair.second, QStringLiteral("K2"));
			QVERIFY(first_wire && second_wire);
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			project.setChainedReports(true);
			QTRY_VERIFY(!pair.first->isFree());

			ConductorProperties properties = first_wire->properties();
			properties.m_function = QStringLiteral("K3");
			first_wire->setPropertyToPotential(properties);
			QTest::qWait(50);

			QCOMPARE(first_wire->properties().m_function, QStringLiteral("K3"));
			QCOMPARE(second_wire->properties().m_function, QStringLiteral("K2"));
			QTRY_VERIFY(pair.first->isFree());
			QTRY_VERIFY(pair.second->isFree());
		}

		/// A report that takes the name of a chain and sits between two
		/// of its reports makes the chain go around it: the links of that
		/// name are made again from the order of the folios
		void functionRenameRearrangesTheChain()
		{
			QETProject project(QFINDTESTDATA("fixtures/chained_report_chain.qet"));
			QCOMPARE(project.state(), QETProject::Ok);
				//The report going to the next folio (folio 1) and the two
				//coming from the previous one (folios 2 and 3)
			Element *go = elementByUuid(project,
					QStringLiteral("{139e1748-290b-4bfa-aa31-2d092f4aeb14}"));
			Element *near = elementByUuid(project,
					QStringLiteral("{f85d9f0e-0068-460e-a241-8da7ef1ae4e9}"));
			Element *far = elementByUuid(project,
					QStringLiteral("{c0a1e1de-0002-4000-8000-000000000001}"));
			QVERIFY(go && near && far);

			QVERIFY(wireReport(go, QStringLiteral("K1")));
			Conductor *middle_wire = wireReport(near, QStringLiteral("K2"));
			QVERIFY(middle_wire);
			QVERIFY(wireReport(far, QStringLiteral("K1")));
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			project.setChainedReports(true);

				//The chain runs from folio 1 to folio 3: the report in
				//between is called something else
			QVERIFY(!go->isFree());
			QCOMPARE(go->linkedElements().value(0), far);
			QTRY_VERIFY(near->isFree());

				//The report in between takes the name of the chain
			ConductorProperties properties = middle_wire->properties();
			properties.m_function = QStringLiteral("K1");
			middle_wire->setPropertyToPotential(properties);
			QTest::qWait(50);

				//The chain goes around it now: folio 1 to folio 2
			QCOMPARE(go->linkedElements().value(0), near);
			QTRY_VERIFY(far->isFree());
		}

		/// A project chaining by the label of its reports keeps its links
		/// when a name is written over a potential: nothing in such a
		/// project would make them again
		void labelModeKeepsTheLinksOnARename()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			setLabel(pair.first, QStringLiteral("K1"));
			setLabel(pair.second, QStringLiteral("K1"));
			Conductor *first_wire = wireReport(pair.first, QStringLiteral("L1"));
			QVERIFY(first_wire);
			project.setChainedReportsKey(QETProject::ReportLinkKey::Label);
			project.setChainedReports(true);
			QTRY_VERIFY(!pair.first->isFree());

			ConductorProperties properties = first_wire->properties();
			properties.m_function = QStringLiteral("L2");
			first_wire->setPropertyToPotential(properties);
			QTest::qWait(50);
			QTRY_VERIFY(!pair.first->isFree());
		}

		/// The same in a project that chains nothing at all: the links of
		/// such a project are made by hand and are nobody's to take away
		void noChainingKeepsTheLinksOnARename()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			Conductor *first_wire = wireReport(pair.first, QStringLiteral("L1"));
			QVERIFY(first_wire);
			pair.first->linkToElement(pair.second);
			QTRY_VERIFY(!pair.first->isFree());

			ConductorProperties properties = first_wire->properties();
			properties.m_function = QStringLiteral("L2");
			first_wire->setPropertyToPotential(properties);
			QTest::qWait(50);
			QTRY_VERIFY(!pair.first->isFree());
		}

		/// The links a rename takes away come back with one undo step, as
		/// the first thing the edit undoes and the last it brings back
		void functionRenameDissolveIsUndone()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			Conductor *first_wire = wireReport(pair.first, QStringLiteral("K2"));
			QVERIFY(first_wire);
			pair.first->linkToElement(pair.second);
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			project.setChainedReports(true);
			QTRY_VERIFY(!pair.first->isFree());

				//What the conductor dialog builds: the links go first and
				//the names after them
			auto *undo = new QUndoCommand();
			new DissolveReportLinksCommand({first_wire}, undo);
			QVariant old_properties, new_properties;
			old_properties.setValue(first_wire->properties());
			ConductorProperties renamed = first_wire->properties();
			renamed.m_function = QStringLiteral("K3");
			new_properties.setValue(renamed);
			new QPropertyUndoCommand(first_wire, "properties",
									 old_properties, new_properties, undo);
			first_wire->diagram()->undoStack().push(undo);
			QTest::qWait(50);
			QCOMPARE(first_wire->properties().m_function, QStringLiteral("K3"));
			QTRY_VERIFY(pair.first->isFree());

			first_wire->diagram()->undoStack().undo();
			QTest::qWait(50);
			QCOMPARE(first_wire->properties().m_function, QStringLiteral("K2"));
			QTRY_VERIFY(!pair.first->isFree());
			QCOMPARE(pair.first->linkedElements().value(0), pair.second);
		}

		/// The links a file has are kept as it saved them, whoever made
		/// them: opening a project changes nothing until the user edits a
		/// name, and a link joining two different names is the user's to
		/// keep
		void linksAreKeptAsTheFileHasThem()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			QVERIFY(wireReport(pair.first, QStringLiteral("K2")));
			QVERIFY(wireReport(pair.second, QStringLiteral("K5")));
				//A link the user made between two different names
			pair.first->linkToElement(pair.second);
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			project.setChainedReports(true);
			QTRY_VERIFY(!pair.first->isFree());

			const QString path = m_dir.filePath(QStringLiteral("kept.qet"));
			QFile file(path);
			QVERIFY(file.open(QIODevice::WriteOnly));
			file.write(project.toXml().toByteArray());
			file.close();
			QETProject reopened(path);
			QCOMPARE(reopened.state(), QETProject::Ok);
			const auto reopened_pair = reports(reopened);
			QVERIFY(reopened_pair.first && reopened_pair.second);
			QTRY_VERIFY(!reopened_pair.first->isFree());
		}

		/// A chain of folio reports is a row of pairs: a report keeps one
		/// partner, and the key change takes the pairs away and brings
		/// them back with one undo
		void changingTheKeyRestoresEveryPair()
		{
			QETProject project(QFINDTESTDATA("fixtures/chained_report_chain.qet"));
			QCOMPARE(project.state(), QETProject::Ok);
			Element *go = elementByUuid(project,
					QStringLiteral("{139e1748-290b-4bfa-aa31-2d092f4aeb14}"));
			Element *near = elementByUuid(project,
					QStringLiteral("{f85d9f0e-0068-460e-a241-8da7ef1ae4e9}"));
			Element *far = elementByUuid(project,
					QStringLiteral("{c0a1e1de-0002-4000-8000-000000000001}"));
			QVERIFY(go && near && far);
			QVERIFY(wireReport(go, QStringLiteral("K1")));
			QVERIFY(wireReport(near, QStringLiteral("K2")));
			QVERIFY(wireReport(far, QStringLiteral("K1")));
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			project.setChainedReports(true);
			QTRY_VERIFY(!go->isFree());
			QCOMPARE(go->linkedElements().value(0), far);
			QTRY_VERIFY(near->isFree());

			project.undoStack()->push(new ChangeReportLinkKeyCommand(
					&project, QETProject::ReportLinkKey::Label));
			QTRY_VERIFY(go->isFree());
			QTRY_VERIFY(near->isFree());
			QTRY_VERIFY(far->isFree());

			project.undoStack()->undo();
			QTRY_VERIFY(!go->isFree());
			QCOMPARE(go->linkedElements().value(0), far);
			QTRY_VERIFY(near->isFree());
		}

		/// The wire's function is not what holds a chain together when the
		/// project chains by the label: changing it leaves the link alone
		void labelKeyKeepsTheLinkWhenAFunctionChanges()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			setLabel(pair.first, QStringLiteral("K1"));
			setLabel(pair.second, QStringLiteral("K1"));
			Conductor *first_wire = wireReport(pair.first, QStringLiteral("L1"));
			QVERIFY(first_wire);
			project.setChainedReportsKey(QETProject::ReportLinkKey::Label);
			project.setChainedReports(true);
			QTRY_VERIFY(!pair.first->isFree());

			ConductorProperties properties = first_wire->properties();
			properties.m_function = QStringLiteral("L2");
			first_wire->setProperties(properties);
			QTest::qWait(50);
			QTRY_VERIFY(!pair.first->isFree());
			QCOMPARE(pair.first->linkedElements().value(0), pair.second);
		}

		/// The choice is stored in the project: a project which chains by
		/// the wire's function says so, and one which chains by the label
		/// keeps the element it always had
		void keyIsKeptWhenSavedAndOpenedAgain()
		{
			QETProject project(fixture());
			QVERIFY(project.toXml().documentElement()
					.firstChildElement(QStringLiteral("chained_reports")).isNull());

			project.setChainedReports(true);
			project.setChainedReportsKey(QETProject::ReportLinkKey::Function);
			const QDomElement saved = project.toXml().documentElement()
					.firstChildElement(QStringLiteral("chained_reports"));
			QCOMPARE(saved.attribute(QStringLiteral("enabled")), QStringLiteral("true"));
			QCOMPARE(saved.attribute(QStringLiteral("key")), QStringLiteral("function"));

			const QString path = m_dir.filePath(QStringLiteral("keys.qet"));
			QFile file(path);
			QVERIFY(file.open(QIODevice::WriteOnly));
			file.write(project.toXml().toByteArray());
			file.close();
			QETProject reopened(path);
			QCOMPARE(reopened.state(), QETProject::Ok);
			QCOMPARE(reopened.chainedReportsKey(), QETProject::ReportLinkKey::Function);

			reopened.setChainedReportsKey(QETProject::ReportLinkKey::Label);
			const QDomElement labelled = reopened.toXml().documentElement()
					.firstChildElement(QStringLiteral("chained_reports"));
			QVERIFY(labelled.attribute(QStringLiteral("key")).isEmpty());
		}

		/// Changing the key takes the chains this project has away and
		/// makes them again from the new one: one undo step carries the
		/// choice and the links it made and took away
		void changingTheKeyDissolvesAndRemakesTheChains()
		{
			QETProject project(fixture());
			const auto pair = reports(project);
			QVERIFY(pair.first && pair.second);
			setLabel(pair.first, QStringLiteral("K1"));
			setLabel(pair.second, QStringLiteral("K1"));
			project.setChainedReportsKey(QETProject::ReportLinkKey::Label);
			project.setChainedReports(true);
			QTRY_VERIFY(!pair.first->isFree());
			const QStringList before = links(project);

			project.undoStack()->push(new ChangeReportLinkKeyCommand(
					&project, QETProject::ReportLinkKey::Function));
			QCOMPARE(project.chainedReportsKey(), QETProject::ReportLinkKey::Function);
			QTRY_VERIFY(pair.first->isFree());

			project.undoStack()->undo();
			QCOMPARE(project.chainedReportsKey(), QETProject::ReportLinkKey::Label);
			QCOMPARE(links(project), before);
		}
};

QET_APP_TEST_MAIN(tst_chainedreportkey)
#include "tst_chainedreportkey.moc"
