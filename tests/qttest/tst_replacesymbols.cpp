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
	//Edit > Replace the symbols: the new symbol takes the old one's place,
	//label and wires, in one undo step
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "replacesymbols.h"

#include <QUndoCommand>

class tst_replacesymbols : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;

		void define(const QString &name, const QString &terminals)
		{
			QDomDocument definition;
			definition.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="40" height="40" hotspot_x="20" hotspot_y="20">
<names><name lang="en">%1</name></names>
<description><rect x="-10" y="-10" width="20" height="20" style="line-style:normal;line-weight:normal;filling:none;color:black"/>%2</description></definition>)")
				.arg(name, terminals));
			QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), name, definition.documentElement()));
		}

		ElementsLocation location(const QString &name) const
		{
			return ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name), m_diagram->project());
		}

		Element *place(const QString &name, const QPointF &pos)
		{
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(location(name), nullptr, &state);
			if (state || !element)
				return nullptr;
			m_diagram->addItem(element);
			element->setPos(pos);
			return element;
		}

		static Terminal *named(Element *element, const QString &name)
		{
			for (Terminal *t : element->terminals())
				if (t->name() == name)
					return t;
			return nullptr;
		}

		QList<Element *> elementsOf(const QString &name) const
		{
			QList<Element *> list;
			for (QGraphicsItem *item : m_diagram->items())
				if (auto *e = qgraphicsitem_cast<Element *>(item))
					if (e->location().toString().endsWith(name + QStringLiteral(".elmt")))
						list << e;
			return list;
		}

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
				//"two": 1 and 3 on top, 2 and 4 below; "three": 3 moved below
				//and no 4
			define(QStringLiteral("two"), QStringLiteral(
				R"(<terminal x="0" y="-10" orientation="n" name="1"/><terminal x="0" y="10" orientation="s" name="2"/>
<terminal x="10" y="-10" orientation="n" name="3"/><terminal x="10" y="10" orientation="s" name="4"/>)"));
			define(QStringLiteral("three"), QStringLiteral(
				R"(<terminal x="0" y="-10" orientation="n" name="1"/><terminal x="0" y="10" orientation="s" name="2"/>
<terminal x="10" y="10" orientation="s" name="3"/>)"));
			define(QStringLiteral("other"), QStringLiteral(
				R"(<terminal x="0" y="10" orientation="s" name="a"/><terminal x="10" y="10" orientation="s" name="b"/>
<terminal x="20" y="10" orientation="s" name="c"/>)"));
		}

		void replaceKeepsPlaceLabelAndWires()
		{
			Element *old_element = place(QStringLiteral("two"), QPointF(200, 200));
			Element *other = place(QStringLiteral("other"), QPointF(200, 100));
			QVERIFY(old_element && other);
			old_element->setRotation(0);
			DiagramContext info = old_element->elementInformations();
			info.addValue(QStringLiteral("label"), QStringLiteral("-X1"));
			old_element->setElementInformations(info);
			auto wire = [this](Terminal *a, Terminal *b) {
				auto *c = new Conductor(a, b);
				m_diagram->addItem(c);
				return c;
			};
			wire(named(other, "a"), named(old_element, "1"));
			wire(named(other, "b"), named(old_element, "3"));
			wire(named(other, "c"), named(old_element, "4"));

			const ReplaceSymbols::Result result = ReplaceSymbols::build(
				m_diagram, {old_element}, location(QStringLiteral("three")));
			QVERIFY(result.command);
				//The new 3 stands where the old 4 was and takes its wire;
				//the old 3's point is empty, and its name taken: lost
			QCOMPARE(result.lost_wires, 1);
			m_diagram->undoStack().push(result.command);

			QVERIFY(elementsOf(QStringLiteral("two")).isEmpty());
			const QList<Element *> replaced = elementsOf(QStringLiteral("three"));
			QCOMPARE(replaced.size(), 1);
			Element *element = replaced.first();
			QCOMPARE(element->pos(), QPointF(200, 200));
			QCOMPARE(element->elementInformations().value(QStringLiteral("label")).toString(),
					 QStringLiteral("-X1"));
			QCOMPARE(named(element, "1")->conductors().size(), 1);
			QCOMPARE(named(element, "3")->conductors().size(), 1);
			QCOMPARE(named(element, "2")->conductors().size(), 0);
			QCOMPARE(named(other, "c")->conductors().size(), 1);
			QCOMPARE(named(other, "b")->conductors().size(), 0);

				//One undo step brings the old symbol and its three wires back
			m_diagram->undoStack().undo();
			QVERIFY(elementsOf(QStringLiteral("three")).isEmpty());
			QCOMPARE(elementsOf(QStringLiteral("two")).size(), 1);
			QCOMPARE(old_element->conductors().size(), 3);
			m_diagram->undoStack().redo();
			QCOMPARE(elementsOf(QStringLiteral("three")).size(), 1);
		}

			//The same point comes before the same name
		void samePointFirst()
		{
			Element *old_element = place(QStringLiteral("three"), QPointF(400, 200));
			Element *other = place(QStringLiteral("other"), QPointF(400, 100));
			QVERIFY(old_element && other);
			auto *c = new Conductor(named(other, "a"), named(old_element, "3"));
			m_diagram->addItem(c);
			const ReplaceSymbols::Result result = ReplaceSymbols::build(
				m_diagram, {old_element}, location(QStringLiteral("two")));
			QVERIFY(result.command);
			QCOMPARE(result.lost_wires, 0);
			m_diagram->undoStack().push(result.command);
			Element *element = nullptr;
			for (Element *e : elementsOf(QStringLiteral("two")))
				if (e->pos() == QPointF(400, 200))
					element = e;
			QVERIFY(element);
				//The old 3 was below at (10, 10), where "two" has its 4
			QCOMPARE(named(element, "4")->conductors().size(), 1);
			QCOMPARE(named(element, "3")->conductors().size(), 0);
		}

			//A wired terminal whose point is empty on the new symbol finds
			//the one of the same name
		void byNameWhenMoved()
		{
			Element *old_element = place(QStringLiteral("two"), QPointF(600, 200));
			Element *other = place(QStringLiteral("other"), QPointF(600, 100));
			QVERIFY(old_element && other);
			auto *c = new Conductor(named(other, "b"), named(old_element, "3"));
			m_diagram->addItem(c);
			const ReplaceSymbols::Result result = ReplaceSymbols::build(
				m_diagram, {old_element}, location(QStringLiteral("three")));
			QVERIFY(result.command);
			QCOMPARE(result.lost_wires, 0);
			m_diagram->undoStack().push(result.command);
			Element *element = nullptr;
			for (Element *e : elementsOf(QStringLiteral("three")))
				if (e->pos() == QPointF(600, 200))
					element = e;
			QVERIFY(element);
			QCOMPARE(named(element, "3")->conductors().size(), 1);
		}
};

QET_APP_TEST_MAIN(tst_replacesymbols)
#include "tst_replacesymbols.moc"
