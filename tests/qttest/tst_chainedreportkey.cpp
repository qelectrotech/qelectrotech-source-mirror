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
#include "undocommand/reportlinkkeycommand.h"

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
			QVERIFY(!pair.first->isFree());
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
			QVERIFY(pair.first->isFree());
			QVERIFY(pair.second->isFree());
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
			QVERIFY(pair.first->isFree());
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
			QVERIFY(!pair.first->isFree());
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
			QVERIFY(pair.first->isFree());
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
			QVERIFY(pair.first->isFree());
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
			QVERIFY(pair.first->isFree());
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
			QVERIFY(pair.first->isFree());
			QVERIFY(wireReport(pair.first, QStringLiteral("K1")));
			QVERIFY(wireReport(pair.second, QStringLiteral("K1")));
			QTest::qWait(50);
			QVERIFY(!pair.first->isFree());
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
			QVERIFY(!pair.first->isFree());
			const QStringList before = links(project);

			project.undoStack()->push(new ChangeReportLinkKeyCommand(
					&project, QETProject::ReportLinkKey::Function));
			QCOMPARE(project.chainedReportsKey(), QETProject::ReportLinkKey::Function);
			QVERIFY(pair.first->isFree());

			project.undoStack()->undo();
			QCOMPARE(project.chainedReportsKey(), QETProject::ReportLinkKey::Label);
			QCOMPARE(links(project), before);
		}
};

QET_APP_TEST_MAIN(tst_chainedreportkey)
#include "tst_chainedreportkey.moc"
