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

#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "diagram.h"
#include "diagramcontext.h"
#include "duplicatelabels.h"
#include "factory/elementfactory.h"
#include "projectcheck.h"
#include "projectcheck/checksettings.h"
#include "projectcheck/clearancecheck.h"
#include "qetgraphicsitem/conductor.h"
#include "conductorsegment.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"

/**
	Step 8 of docs/mm-scale.md: the clearance checks of IEC 61082-1 in the
	project check. Wires closer than 1 M side by side, closer than 2 M with
	a text between them (7.1.3.4), and a text over the wire of another
	(7.1.3.5). Both are on by default in a project drawn to scale only.
*/
class tst_clearancecheck : public QObject
{
	Q_OBJECT

	using Kind = ProjectCheck::Kind;

		/// A symbol 20 px wide with a connection on each side, its origin
		/// on the left one, so that two of them in a row are joined by a
		/// straight wire. Symbols snap to the grid: the connections are
		/// @p offset px below the origin to put a wire off it.
	static QString pass(int offset, const QString &kind = QStringLiteral("simple"))
	{
		return QStringLiteral(
			"<definition type=\"element\" link_type=\"%2\" width=\"20\" height=\"20\" "
			"hotspot_x=\"0\" hotspot_y=\"10\"><names><name lang=\"en\">pass</name></names>"
			"<description><rect x=\"0\" y=\"0\" width=\"20\" height=\"20\" "
			"style=\"line-style:normal;line-weight:normal;filling:none;color:black\"/>"
			"<terminal x=\"0\" y=\"%1\" orientation=\"w\"/>"
			"<terminal x=\"20\" y=\"%1\" orientation=\"e\"/>"
			"</description></definition>").arg(10 + offset).arg(kind);
	}

	static Element *place(QETProject &project, Diagram *diagram, const QPointF &pos,
						  int offset = 0, const QString &kind = QStringLiteral("simple"))
	{
		auto *collection = project.embeddedElementCollection();
		const QString name = QStringLiteral("pass%1%2").arg(offset).arg(kind);
		if (!collection->exist(QStringLiteral("import/%1.elmt").arg(name))) {
			QDomDocument document;
			document.setContent(pass(offset, kind));
			collection->addElementDefinition(QStringLiteral("import"), name,
											 document.documentElement());
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name), &project),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		element->setPos(pos);
		return element;
	}

		/// A wire from x = 100 to x = 400 at height @p y (a whole number of
		/// px), with the text @p number (none when empty)
	static Conductor *wire(QETProject &project, Diagram *diagram, int y,
						   const QString &number = QString())
	{
		const int offset = y % 10;
		Element *left = place(project, diagram, QPointF(100, y - offset), offset);
		Element *right = place(project, diagram, QPointF(400, y - offset), offset);
		if (!left || !right || left->terminals().size() < 2 || right->terminals().size() < 2)
			return nullptr;
		Terminal *from = left->terminals().at(0)->scenePos().x() > left->terminals().at(1)->scenePos().x()
			? left->terminals().at(0) : left->terminals().at(1);
		Terminal *to = right->terminals().at(0)->scenePos().x() < right->terminals().at(1)->scenePos().x()
			? right->terminals().at(0) : right->terminals().at(1);
		auto *conductor = new Conductor(from, to);
		diagram->addItem(conductor);
		ConductorProperties properties = conductor->properties();
		properties.text = number;
		conductor->setProperties(properties);
		return conductor;
	}

		/// Like wire(), from height @p left_y on the left to @p right_y
		/// on the right: a wire with a step in the middle
	static Conductor *steppedWire(QETProject &project, Diagram *diagram,
								  int left_y, int right_y)
	{
		Element *left = place(project, diagram, QPointF(100, left_y - left_y % 10), left_y % 10);
		Element *right = place(project, diagram, QPointF(400, right_y - right_y % 10), right_y % 10);
		if (!left || !right || left->terminals().size() < 2 || right->terminals().size() < 2)
			return nullptr;
		Terminal *from = left->terminals().at(0)->scenePos().x() > left->terminals().at(1)->scenePos().x()
			? left->terminals().at(0) : left->terminals().at(1);
		Terminal *to = right->terminals().at(0)->scenePos().x() < right->terminals().at(1)->scenePos().x()
			? right->terminals().at(0) : right->terminals().at(1);
		auto *conductor = new Conductor(from, to);
		diagram->addItem(conductor);
		return conductor;
	}

	static IndependentTextItem *freeText(Diagram *diagram, const QPointF &pos)
	{
		auto *text = new IndependentTextItem(QStringLiteral("TEXT"));
		diagram->addItem(text);
		text->setPos(pos);
		return text;
	}

	static ProjectCheck::Result run(QETProject &project, Kind kind)
	{
		return ProjectCheck::run(&project, kind);
	}

private slots:
	void initTestCase()
	{
		QETProject::setBackupEnabled(false);
	}

		/// The two wires of the example are at least 1 M apart: nothing
	void tenPixelsApartIsFine()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		QVERIFY(wire(project, diagram, 100));
		QVERIFY(wire(project, diagram, 110));
		QVERIFY(ClearanceCheck::clearances(diagram).isEmpty());
		QVERIFY(run(project, Kind::ConductorClearance).findings.isEmpty());
		QVERIFY(run(project, Kind::TextOnLines).findings.isEmpty());
	}

		/// What mapToScene rounds away does not make wires 1 M apart closer
	void tenPixelsLessARoundingIsFine()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		QVERIFY(wire(project, diagram, 100));
		Conductor *b = wire(project, diagram, 110);
		QVERIFY(b);
		b->setPos(0, -0.0001);
		QVERIFY(ClearanceCheck::clearances(diagram).isEmpty());
		b->setPos(0, -0.5);
		QCOMPARE(ClearanceCheck::clearances(diagram).size(), 1);
	}

	void fivePixelsApartIsReported()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Conductor *a = wire(project, diagram, 100, QStringLiteral("W1"));
		Conductor *b = wire(project, diagram, 105, QStringLiteral("W2"));
		QVERIFY(a && b);
		const auto found = ClearanceCheck::clearances(diagram);
		QCOMPARE(found.size(), 1);
		QCOMPARE(found.first().distance, 5.0);
		QVERIFY(!found.first().text);
		QCOMPARE(QSet<Conductor *>({found.first().first, found.first().second}),
				 QSet<Conductor *>({a, b}));

		const auto result = run(project, Kind::ConductorClearance);
		QCOMPARE(result.findings.size(), 1);
		const auto &finding = result.findings.first();
		QVERIFY(finding.text.contains(QLatin1String("W1")));
		QVERIFY(finding.text.contains(QLatin1String("W2")));
		QCOMPARE(finding.places.size(), 2);
		QVERIFY(!finding.places.first().folio.isEmpty());
		QVERIFY(finding.places.first().canShow());
	}

		/// The limits are 1 M and 2 M, 10 and 20 px whatever M is, and the
		/// finding says them in mm of the project's M
	void findingsNameTheModule_data()
	{
		QTest::addColumn<qreal>("module");
		QTest::newRow("M 1.8") << 1.8;
		QTest::newRow("M 2.5") << 2.5;
		QTest::newRow("M 5.0") << 5.0;
	}

	void findingsNameTheModule()
	{
		QFETCH(qreal, module);
		const QString mm = QString::number(module, 'f', 1);
		const QString twice = QString::number(2 * module, 'f', 1);
		QETProject project;
		project.setScale(QETProject::Scale::Mm);
		project.setModuleMm(module);
		Diagram *diagram = project.addNewDiagram();
		QVERIFY(wire(project, diagram, 100, QStringLiteral("W1")));
		QVERIFY(wire(project, diagram, 105, QStringLiteral("W2")));
		QVERIFY(wire(project, diagram, 200, QStringLiteral("W3")));
		QVERIFY(wire(project, diagram, 215, QStringLiteral("W4")));
		QVERIFY(wire(project, diagram, 300));
		QVERIFY(wire(project, diagram, 310));
		freeText(diagram, QPointF(250, 207));
		const auto result = run(project, Kind::ConductorClearance);
		QCOMPARE(result.findings.size(), 2); //10 px is 1 M: fine
		bool plain = false, with_text = false;
		for (const auto &finding : result.findings) {
			if (finding.text.contains(QLatin1String("W1"))) {
				plain = true;
				QVERIFY2(finding.text.contains(QStringLiteral("less than 1 M (%1 mm)").arg(mm)),
					 qPrintable(finding.text));
				QVERIFY2(finding.text.contains(QString::number(5 * module / 10, 'f', 1) + QLatin1String(" mm apart")),
					 qPrintable(finding.text));
			} else {
				with_text = true;
				QVERIFY2(finding.text.contains(QStringLiteral("less than 2 M (%1 mm)").arg(twice)),
					 qPrintable(finding.text));
			}
		}
		QVERIFY(plain && with_text);
	}

		/// Vertical wires are checked as the horizontal ones are
	void verticalWiresToo()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		for (int x : {100, 105}) {
				// Turned a quarter, the connections' offset is a shift in x
			const int offset = (x % 10) ? 10 - x % 10 : 0;
			Element *top = place(project, diagram, QPointF(x + offset, 100), offset);
			Element *bottom = place(project, diagram, QPointF(x + offset, 400), offset);
			QVERIFY(top && bottom);
			top->setRotation(90);
			bottom->setRotation(90);
			Terminal *from = top->terminals().at(0)->scenePos().y() > top->terminals().at(1)->scenePos().y()
				? top->terminals().at(0) : top->terminals().at(1);
			Terminal *to = bottom->terminals().at(0)->scenePos().y() < bottom->terminals().at(1)->scenePos().y()
				? bottom->terminals().at(0) : bottom->terminals().at(1);
			diagram->addItem(new Conductor(from, to));
		}
		const auto found = ClearanceCheck::clearances(diagram);
		QCOMPARE(found.size(), 1);
		QCOMPARE(found.first().distance, 5.0);
	}

		/// A wire pair is reported once, with its worst place: the 5 px
		/// stretch is not hidden by the 15 px one with a text between,
		/// which the sweep meets first
	void theWorstPlaceOfAPairIsKept()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		QVERIFY(wire(project, diagram, 100));
		Conductor *b = steppedWire(project, diagram, 85, 95);
		QVERIFY(b);
		freeText(diagram, QPointF(150, 86));
		const auto found = ClearanceCheck::clearances(diagram);
		QCOMPARE(found.size(), 1);
		QCOMPARE(found.first().distance, 5.0);
		QVERIFY(!found.first().text);
	}

		/// 15 px apart is enough alone, not with a text between the wires
	void fifteenPixelsWithATextBetween()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		QVERIFY(wire(project, diagram, 100));
		QVERIFY(wire(project, diagram, 115));
		QVERIFY(ClearanceCheck::clearances(diagram).isEmpty());
			// A text on one side of the wires is not between them
		freeText(diagram, QPointF(200, 60));
		QVERIFY(ClearanceCheck::clearances(diagram).isEmpty());
		freeText(diagram, QPointF(250, 100));
		const auto found = ClearanceCheck::clearances(diagram);
		QCOMPARE(found.size(), 1);
		QVERIFY(found.first().text);
		QCOMPARE(found.first().distance, 15.0);
		QCOMPARE(run(project, Kind::ConductorClearance).findings.size(), 1);
	}

		/// 25 px apart with a text between them is more than 2 M
	void twentyFivePixelsIsFine()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		QVERIFY(wire(project, diagram, 100));
		QVERIFY(wire(project, diagram, 125));
		freeText(diagram, QPointF(250, 105));
		QVERIFY(ClearanceCheck::clearances(diagram).isEmpty());
	}

		/// Two wires on one terminal run side by side where they leave it
	void wiresOnOneTerminalAreNotReported()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Conductor *a = wire(project, diagram, 100);
		Element *other = place(project, diagram, QPointF(400, 100), 5);
		QVERIFY(a && other);
		Terminal *far = other->terminals().at(0)->scenePos().x() < other->terminals().at(1)->scenePos().x()
			? other->terminals().at(0) : other->terminals().at(1);
		diagram->addItem(new Conductor(a->terminal1, far));
		QVERIFY(ClearanceCheck::clearances(diagram).isEmpty());
	}

	void aTextOnAnotherWire()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Conductor *a = wire(project, diagram, 100, QStringLiteral("W1"));
		Conductor *b = wire(project, diagram, 140, QStringLiteral("W2"));
		QVERIFY(a && b);
			// Their own texts sit on their own wires
		QVERIFY(ClearanceCheck::textsOnLines(diagram).isEmpty());
		auto *text = freeText(diagram, QPointF(250, 132));
		const auto found = ClearanceCheck::textsOnLines(diagram);
		QCOMPARE(found.size(), 1);
		QCOMPARE(found.first().conductor, b);
		QCOMPARE(found.first().text, static_cast<DiagramTextItem *>(text));

		const auto result = run(project, Kind::TextOnLines);
		QCOMPARE(result.findings.size(), 1);
		QVERIFY(result.findings.first().text.contains(QLatin1String("TEXT")));
		QVERIFY(result.findings.first().text.contains(QLatin1String("W2")));
		QVERIFY(run(project, Kind::ConductorClearance).findings.isEmpty());

			// Moved clear of the wire
		text->setPos(250, 160);
		QVERIFY(ClearanceCheck::textsOnLines(diagram).isEmpty());
	}

		/// The number of a wire over another wire of its own potential is
		/// no finding, over a wire of another potential it is
	void aTextOverAnotherPotentialIsReported()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Conductor *a = wire(project, diagram, 100, QStringLiteral("W1"));
		Conductor *b = wire(project, diagram, 160, QStringLiteral("W2"));
		QVERIFY(a && b);
		a->textItem()->setPos(320, 162);
		auto found = ClearanceCheck::textsOnLines(diagram);
		QCOMPARE(found.size(), 1);
		QCOMPARE(found.first().text, static_cast<DiagramTextItem *>(a->textItem()));
		QCOMPARE(found.first().conductor, b);
	}

	void aTextOverItsOwnPotentialIsFineSamePotential()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Conductor *a = wire(project, diagram, 100, QStringLiteral("W1"));
		Element *other = place(project, diagram, QPointF(400, 160), 0);
		QVERIFY(a && other);
		Terminal *far = other->terminals().at(0)->scenePos().x() < other->terminals().at(1)->scenePos().x()
			? other->terminals().at(0) : other->terminals().at(1);
			// Leaves the same terminal: one potential, and a stretch at 170
		auto *c = new Conductor(a->terminal1, far);
		diagram->addItem(c);
		QVERIFY(a->relatedPotentialConductors().contains(c));
		bool low = false;
		for (ConductorSegment *segment : c->segmentsList())
			if (!segment->isPoint() && qAbs(c->mapToScene(segment->firstPoint()).y() - 170) < 0.5
				&& qAbs(c->mapToScene(segment->secondPoint()).y() - 170) < 0.5
				&& qMax(c->mapToScene(segment->firstPoint()).x(), c->mapToScene(segment->secondPoint()).x()) > 340)
				low = true;
		QVERIFY(low);
		a->textItem()->setPos(320, 162);
		QVERIFY(ClearanceCheck::textsOnLines(diagram).isEmpty());
	}

		/// A text over two wires of one potential is one finding, over
		/// wires of two potentials it is two
	void aTextOverAPotentialIsListedOnce()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Conductor *a = wire(project, diagram, 100);
		Element *other = place(project, diagram, QPointF(400, 160), 0);
		QVERIFY(a && other);
		Terminal *far = other->terminals().at(0)->scenePos().x() < other->terminals().at(1)->scenePos().x()
			? other->terminals().at(0) : other->terminals().at(1);
			// Leaves the same terminal: one potential with a stretch at 170
		auto *c = new Conductor(a->terminal1, far);
		diagram->addItem(c);
		QVERIFY(a->relatedPotentialConductors().contains(c));
			// Both wires leave the terminal side by side, under the text
		auto *text = freeText(diagram, QPointF(110, 100));
		QCOMPARE(ClearanceCheck::textsOnLines(diagram).size(), 1);
		QCOMPARE(run(project, Kind::TextOnLines).findings.size(), 1);

			// A wire of another potential under the same text: a second one
		Conductor *b = wire(project, diagram, 104);
		QVERIFY(b);
		QVERIFY(!a->relatedPotentialConductors().contains(b));
		QList<ClearanceCheck::TextOnLine> two;
		for (const auto &line : ClearanceCheck::textsOnLines(diagram))
			if (line.text == text) two << line;
		QCOMPARE(two.size(), 2);
		QVERIFY(two.at(0).conductor != two.at(1).conductor);
			// Each is listed with an id of its own, for accepting it
		QSet<QString> ids;
		for (const auto &finding : run(project, Kind::TextOnLines).findings)
			if (finding.text.contains(QLatin1String("TEXT")))
				ids << finding.ids.join(QLatin1Char(','));
		QCOMPARE(ids.size(), 2);
	}

		/// An accepted finding stays accepted when another wire of the
		/// potential is the one the text is met over first: the id is
		/// the text and the potential, not the wire found
	void anAcceptedTextOnLineSurvivesAnotherWireMetFirst()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Conductor *a = wire(project, diagram, 100);
		Element *other = place(project, diagram, QPointF(400, 160), 0);
		QVERIFY(a && other);
		Terminal *far = other->terminals().at(0)->scenePos().x() < other->terminals().at(1)->scenePos().x()
			? other->terminals().at(0) : other->terminals().at(1);
			// Leaves the same terminal: one potential, a stretch at 170
		auto *c = new Conductor(a->terminal1, far);
		diagram->addItem(c);
		QVERIFY(a->relatedPotentialConductors().contains(c));
		auto *text = freeText(diagram, QPointF(110, 100));

		auto textFinding = [&]() {
			ProjectCheck::Finding found;
			for (const auto &finding : run(project, Kind::TextOnLines).findings)
				if (finding.text.contains(QLatin1String("TEXT")))
					found = finding;
			return found;
		};
		const auto first = textFinding();
		QVERIFY(!first.text.isEmpty());
		const QString id = CheckSettings::findingId(Kind::TextOnLines, first);
		CheckSettings::acceptFinding(&project, id, QStringLiteral("ok"));

			// Over the stretch of the other wire only: it is met first
		text->setPos(415, 162);
		const auto second = textFinding();
		QVERIFY(!second.text.isEmpty());
		QVERIFY2(first.places.at(1).item != second.places.at(1).item,
				 "the same wire was met");
		QCOMPARE(CheckSettings::findingId(Kind::TextOnLines, second), id);
		QVERIFY(CheckSettings::acceptedFindings(&project).contains(
			CheckSettings::findingId(Kind::TextOnLines, second)));
	}

		/// A potential over two folios by a folio report, the wire with
		/// the lowest uuid on the other folio than the text: the finding
		/// is about the wire under the text, on its folio
	void aTextOverAReportedPotentialIsPlacedOnItsFolio()
	{
		QETProject project;
		Diagram *first = project.addNewDiagram();
		Diagram *second = project.addNewDiagram();
		Conductor *a = wire(project, first, 100);
		Conductor *b = wire(project, second, 100);
		QVERIFY(a && b);
		Element *out = place(project, first, QPointF(500, 90), 0, QStringLiteral("next_report"));
		Element *in = place(project, second, QPointF(500, 90), 0, QStringLiteral("previous_report"));
		QVERIFY(out && in);
			//Each wire goes on to its report from the free end of its symbol
		auto rightmost = [](Conductor *w) {
			const auto terminals = w->terminal2->parentElement()->terminals();
			return terminals.at(0)->scenePos().x() > terminals.at(1)->scenePos().x()
				? terminals.at(0) : terminals.at(1);
		};
		auto leftmost = [](Element *report) {
			const auto terminals = report->terminals();
			return terminals.at(0)->scenePos().x() < terminals.at(1)->scenePos().x()
				? terminals.at(0) : terminals.at(1);
		};
		first->addItem(new Conductor(rightmost(a), leftmost(out)));
		second->addItem(new Conductor(rightmost(b), leftmost(in)));
		out->linkToElement(in);
		QVERIFY(a->relatedPotentialConductors().contains(b));

			//The text is on the folio that does not hold the lowest uuid
		QUuid lowest_first = a->uuid(), lowest_second = b->uuid();
		for (Conductor *w : a->relatedPotentialConductors()) {
			QUuid &lowest = w->diagram() == first ? lowest_first : lowest_second;
			if (w->uuid() < lowest) lowest = w->uuid();
		}
		Diagram *text_folio = lowest_first < lowest_second ? second : first;
		Conductor *under = text_folio == first ? a : b;
		freeText(text_folio, QPointF(250, 92));
			//A wire of another potential the text crosses too would be
			//a second finding: none here
		const auto found = ClearanceCheck::textsOnLines(text_folio);
		QCOMPARE(found.size(), 1);
		QCOMPARE(found.first().conductor, under);
		QCOMPARE(found.first().conductor->diagram(), text_folio);

		const auto result = run(project, Kind::TextOnLines);
		QCOMPARE(result.findings.size(), 1);
		const auto &places = result.findings.first().places;
		QCOMPARE(places.size(), 2);
		QCOMPARE(DuplicateLabels::diagramOf(places.at(1).item.data()), text_folio);
		QCOMPARE(places.at(1).item.data(), static_cast<QGraphicsObject *>(under));
	}

		/// Off in a project in pixels, on in one drawn to scale; either
		/// can be changed, and only a change is stored
	void switchedOnOnlyForAScale()
	{
		for (Kind kind : {Kind::ConductorClearance, Kind::TextOnLines}) {
			QVERIFY(ProjectCheck::kinds().contains(kind));
			QVERIFY(ProjectCheck::onlyForScale(kind));
		}
		QVERIFY(!ProjectCheck::onlyForScale(Kind::OpenWireEnds));

		QETProject project;
		project.addNewDiagram();
		const QString before = project.toXml().toString();
		for (Kind kind : ProjectCheck::kinds())
			QCOMPARE(CheckSettings::ruleEnabled(&project, kind), !ProjectCheck::onlyForScale(kind));
		QVERIFY(!project.projectProperties().contains(QStringLiteral("check_rules_on")));
		QVERIFY(!project.projectProperties().contains(QStringLiteral("check_rules_off")));
			// Following the defaults stores nothing
		project.setProjectProperties(CheckSettings::withRulesOff(
			project.projectProperties(),
			{Kind::ConductorClearance, Kind::TextOnLines}));
		QCOMPARE(project.toXml().toString(), before);

		CheckSettings::setRuleEnabled(&project, Kind::TextOnLines, true);
		QVERIFY(CheckSettings::ruleEnabled(&project, Kind::TextOnLines));
		QVERIFY(!CheckSettings::ruleEnabled(&project, Kind::ConductorClearance));
		QCOMPARE(project.projectProperties().value(QStringLiteral("check_rules_on")).toString(),
				 ProjectCheck::id(Kind::TextOnLines));
		QVERIFY(!project.projectProperties().contains(QStringLiteral("check_rules_off")));
		CheckSettings::setRuleEnabled(&project, Kind::TextOnLines, false);
		QVERIFY(!project.projectProperties().contains(QStringLiteral("check_rules_on")));

		project.setScale(QETProject::Scale::Mm);
		for (Kind kind : ProjectCheck::kinds())
			QVERIFY(CheckSettings::ruleEnabled(&project, kind));
		QVERIFY(!project.projectProperties().contains(QStringLiteral("check_rules_off")));
		CheckSettings::setRuleEnabled(&project, Kind::ConductorClearance, false);
		QVERIFY(!CheckSettings::ruleEnabled(&project, Kind::ConductorClearance));
		QVERIFY(CheckSettings::ruleEnabled(&project, Kind::TextOnLines));
		QCOMPARE(project.projectProperties().value(QStringLiteral("check_rules_off")).toString(),
				 ProjectCheck::id(Kind::ConductorClearance));
		CheckSettings::setRuleEnabled(&project, Kind::ConductorClearance, true);
		QVERIFY(!project.projectProperties().contains(QStringLiteral("check_rules_off")));
	}

		/// Many wires side by side: the sweep does not look at every pair
	void manyWiresStayBounded()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		for (int i = 0 ; i < 150 ; ++i)
			QVERIFY(wire(project, diagram, 100 + i * 30));
		QElapsedTimer timer;
		timer.start();
		QVERIFY(ClearanceCheck::clearances(diagram).isEmpty());
		QVERIFY(ClearanceCheck::textsOnLines(diagram).isEmpty());
		QVERIFY2(timer.elapsed() < 1000, qPrintable(QString::number(timer.elapsed())));
	}

		/// The cap keeps the findings from the top of the folio and counts
		/// the rest, in both directions
	void theCapCountsWhatItLeavesOut()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		for (int k = 0 ; k < 6 ; ++k) {
			QVERIFY(wire(project, diagram, 100 + 40 * k));
			QVERIFY(wire(project, diagram, 105 + 40 * k));
		}
		int omitted = -1;
		QCOMPARE(ClearanceCheck::clearances(diagram, &omitted).size(), 6);
		QCOMPARE(omitted, 0);
		const auto found = ClearanceCheck::clearances(diagram, &omitted, 4);
		QCOMPARE(found.size(), 4);
		QCOMPARE(omitted, 2);
		for (int i = 1 ; i < found.size() ; ++i)
			QVERIFY(found.at(i - 1).where.y() <= found.at(i).where.y());
		QCOMPARE(ClearanceCheck::clearances(diagram, nullptr, 0).size(), 0);
	}

		/// A folio with more findings than the cap says so, once
	void aFolioOverTheLimitSaysSo()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
			// 5 px apart one after the other: each neighbours the next
		const int wires = ClearanceCheck::limit + 2;
		for (int i = 0 ; i < wires ; ++i)
			QVERIFY(wire(project, diagram, 100 + 5 * i));
		int omitted = 0;
		QCOMPARE(ClearanceCheck::clearances(diagram, &omitted).size(), ClearanceCheck::limit);
		QCOMPARE(omitted, 1);
		const auto result = run(project, Kind::ConductorClearance);
		QCOMPARE(result.findings.size(), ClearanceCheck::limit + 1);
		const auto &last = result.findings.last();
		QVERIFY(last.text.contains(QLatin1String("1 more")));
		QCOMPARE(last.places.size(), 1);
		QVERIFY(!last.places.first().folio.isEmpty());
	}
};

QET_APP_TEST_MAIN(tst_clearancecheck)
#include "tst_clearancecheck.moc"
