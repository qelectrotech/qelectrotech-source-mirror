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
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "autoNum/numerotationcontext.h"
#include "autoNum/renumberconductorscommand.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "titleblockproperties.h"

#include <QDomDocument>
#include <QUndoStack>
#include <QUuid>
#include <QtTest>

/**
	Renumber conductors: the wires of the chosen folios get new numbers
	with a conductor numbering, one per potential, in folio order and from
	the top left of each folio; frozen wires and potentials reaching a
	folio not chosen keep theirs, and their numbers are not given again;
	plant counters count each plant on its own; undo puts everything back.
*/
class tst_renumberwires : public QObject
{
	Q_OBJECT

	Element *symbol(Diagram *diagram, QPointF at)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		if (!collection->exist(QStringLiteral("import/simple.elmt"))) {
			QDomDocument definition;
			definition.setContent(QString(
				R"(<definition type="element" link_type="simple" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test symbol</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%2"/><terminal x="0" y="20" orientation="s" uuid="%3"/>
</description></definition>)").arg(QUuid::createUuid().toString(),
				QUuid::createUuid().toString(), QUuid::createUuid().toString()));
			if (!collection->addElementDefinition(QStringLiteral("import"), QStringLiteral("simple"),
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/simple.elmt"), diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		element->setPos(at);
		return element;
	}

		//A wire from the lower end of @p a to the upper end of @p b
	static Conductor *wire(Element *a, Element *b, const QString &text, bool frozen = false)
	{
		auto *c = new Conductor(a->terminals().last(), b->terminals().first());
		a->diagram()->addItem(c);
		auto p = c->properties();
		p.text = text;
		c->setProperties(p);
		c->setFreezeLabel(frozen);
		return c;
	}

		//A wire of its own at @p at
	Conductor *lone(Diagram *d, QPointF at, const QString &text, bool frozen = false)
	{
		return wire(symbol(d, at), symbol(d, at + QPointF(0, 60)), text, frozen);
	}

	static Diagram *folio(QETProject &project, const QString &plant = {})
	{
		Diagram *d = project.addNewDiagram();
		TitleBlockProperties tb = d->border_and_titleblock.exportTitleBlock();
		tb.plant = plant;
		d->border_and_titleblock.importTitleBlock(tb);
		return d;
	}

	static void numbering(QETProject &project, const QString &counter = QStringLiteral("unit"))
	{
		NumerotationContext rule;
		rule.addValue(QStringLiteral("string"), QStringLiteral("W"));
		rule.addValue(counter, 1, 1, 1);
		project.addConductorAutoNum(QStringLiteral("wires"), rule);
	}

	static void renumber(QETProject &project, const QList<Diagram *> &folios)
	{
		auto plan = RenumberConductorsCommand::plan(&project, QStringLiteral("wires"), folios);
		project.undoStack()->push(new RenumberConductorsCommand(
				&project, QStringLiteral("wires"), std::move(plan), QStringLiteral("renumber")));
	}

	static QString text(Conductor *c) {return c->properties().text;}

private slots:
	void folioThenTopLeftOrder()
	{
		QETProject project;
		numbering(project);
		Diagram *first = folio(project), *second = folio(project);
		Conductor *low = lone(first, {100, 400}, QStringLiteral("x"));
		Conductor *high_right = lone(first, {400, 100}, QStringLiteral("y"));
		Conductor *high_left = lone(first, {100, 100}, QStringLiteral("z"));
		Conductor *next = lone(second, {100, 100}, QStringLiteral("q"));
		renumber(project, project.diagrams());
		QCOMPARE(text(high_left), QStringLiteral("W1"));
		QCOMPARE(text(high_right), QStringLiteral("W2"));
		QCOMPARE(text(low), QStringLiteral("W3"));
		QCOMPARE(text(next), QStringLiteral("W4"));
	}

	void onePotentialOneNumber()
	{
		QETProject project;
		numbering(project);
		Diagram *d = folio(project);
		Element *a = symbol(d, {100, 100}), *b = symbol(d, {100, 200}), *c = symbol(d, {100, 300});
			//Three wires meeting at a's lower and c's upper terminal: one
			//potential, numbered "5" and "6" before
		Conductor *w1 = wire(a, b, QStringLiteral("5"));
		Conductor *w2 = wire(b, c, QStringLiteral("6"));
		Conductor *w3 = new Conductor(a->terminals().last(), c->terminals().first());
		d->addItem(w3);
		renumber(project, project.diagrams());
		QCOMPARE(text(w1), QStringLiteral("W1"));
		QCOMPARE(text(w2), QStringLiteral("W1"));
		QCOMPARE(text(w3), QStringLiteral("W1"));
	}

	void frozenKeepsItsNumberAndItIsSkipped()
	{
		QETProject project;
		numbering(project);
		Diagram *d = folio(project);
		Conductor *first = lone(d, {100, 100}, QStringLiteral("a"));
		Conductor *manual = lone(d, {300, 100}, QStringLiteral("W2"), true);
		Conductor *third = lone(d, {500, 100}, QStringLiteral("c"));
		renumber(project, project.diagrams());
		QCOMPARE(text(first), QStringLiteral("W1"));
		QCOMPARE(text(manual), QStringLiteral("W2"));
		QCOMPARE(text(third), QStringLiteral("W3"));
	}

	void folioNotChosenIsLeftAlone()
	{
		QETProject project;
		numbering(project);
		Diagram *first = folio(project), *second = folio(project);
		Conductor *here = lone(first, {100, 100}, QStringLiteral("a"));
		Conductor *there = lone(second, {100, 100}, QStringLiteral("W1"));
		renumber(project, {first});
			//W1 is the unchosen folio's: not given again
		QCOMPARE(text(here), QStringLiteral("W2"));
		QCOMPARE(text(there), QStringLiteral("W1"));
	}

	void plantCounterPerPlant()
	{
		QETProject project;
		numbering(project, QStringLiteral("unitplant"));
		Diagram *factory = folio(project, QStringLiteral("FACTORY"));
		Diagram *silo = folio(project, QStringLiteral("SILO"));
		Diagram *factory2 = folio(project, QStringLiteral("FACTORY"));
		Conductor *f1 = lone(factory, {100, 100}, QStringLiteral("a"));
		Conductor *s1 = lone(silo, {100, 100}, QStringLiteral("b"));
		Conductor *f2 = lone(factory2, {100, 100}, QStringLiteral("c"));
		renumber(project, project.diagrams());
		QCOMPARE(text(f1), QStringLiteral("W1"));
		QCOMPARE(text(s1), QStringLiteral("W1"));
		QCOMPARE(text(f2), QStringLiteral("W2"));
			//New wires go on from there
		QCOMPARE(project.conductorPlantCounter(QStringLiteral("wires"), QStringLiteral("FACTORY")),
				 QStringList({QStringLiteral("2")}));
	}

	void undoPutsEverythingBack()
	{
		QETProject project;
		numbering(project, QStringLiteral("unitplant"));
		Diagram *d = folio(project, QStringLiteral("SILO"));
		Conductor *a = lone(d, {100, 100}, QStringLiteral("old"));
		const NumerotationContext before = project.conductorAutoNum(QStringLiteral("wires"));
		renumber(project, project.diagrams());
		QCOMPARE(text(a), QStringLiteral("W1"));
		project.undoStack()->undo();
		QCOMPARE(text(a), QStringLiteral("old"));
		QVERIFY(project.conductorPlantCounter(QStringLiteral("wires"), QStringLiteral("SILO")).isEmpty());
		const NumerotationContext after = project.conductorAutoNum(QStringLiteral("wires"));
		QCOMPARE(after.size(), before.size());
		for (int i = 0 ; i < before.size() ; ++i)
			QCOMPARE(after.itemAt(i), before.itemAt(i));
		project.undoStack()->redo();
		QCOMPARE(text(a), QStringLiteral("W1"));
	}
};

QET_APP_TEST_MAIN(tst_renumberwires)
#include "tst_renumberwires.moc"
