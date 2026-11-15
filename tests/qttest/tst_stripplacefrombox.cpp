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
	The terminal strips see a terminal where its label shows it: inside a
	structure box, at the box's location when the project lets %{plant}
	and %{location} take the box's or folio's (#1845); else at its own
	location only, as before.
*/
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "TerminalStrip/terminalstripautoimport.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/structureboxitem.h"
#include "qetproject.h"

#include <QDomDocument>
#include <QUuid>

class tst_stripplacefrombox : public QObject
{
	Q_OBJECT

	static Element *terminal(Diagram *diagram, const QString &label, const QPointF &pos)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		if (!collection->exist(QStringLiteral("import/terminal.elmt")))
		{
			QDomDocument definition;
			definition.setContent(
				QString(R"(<definition type="element" link_type="terminal" uuid="%1" width="20" height="20" hotspot_x="10" hotspot_y="10">
<names><name lang="en">Terminal</name></names>
<description><ellipse x="-2.5" y="-2.5" width="5" height="5"/>
<terminal x="0" y="-10" orientation="n" uuid="%2"/><terminal x="0" y="10" orientation="s" uuid="%3"/>
</description></definition>)")
					.arg(QUuid::createUuid().toString(),
						 QUuid::createUuid().toString(),
						 QUuid::createUuid().toString()));
			if (!collection->addElementDefinition(
					QStringLiteral("import"), QStringLiteral("terminal"),
					definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/terminal.elmt"),
							 diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		element->setPos(pos);
		DiagramContext info = element->elementInformations();
		info.addValue(QStringLiteral("label"), label);
		element->setElementInformations(info);
		return element;
	}

		/// Two terminals inside a box at +VD1, one outside, all of strip X1
	static void draw(QETProject &project)
	{
		Diagram *diagram = project.addNewDiagram();
		auto *b = new StructureBoxItem(QPointF(100, 100), QPointF(300, 300));
		b->setLocation(QStringLiteral("VD1"));
		diagram->addItem(b);
		QVERIFY(terminal(diagram, QStringLiteral("X1:1"), QPointF(150, 200)));
		QVERIFY(terminal(diagram, QStringLiteral("X1:2"), QPointF(200, 200)));
		QVERIFY(terminal(diagram, QStringLiteral("X1:3"), QPointF(600, 200)));
	}

		/// "location:count" of every strip, sorted
	static QStringList strips(const QETProject &project)
	{
		QStringList result;
		for (TerminalStrip *strip : project.terminalStrip())
			result << strip->location() + QLatin1Char(':')
					  + QString::number(strip->physicalTerminalCount());
		result.sort();
		return result;
	}

	private slots:
		void boxGivesTheStripsLocation()
		{
			QETProject project;
			project.setPlantAndLocationInherited(true);
			draw(project);
			TerminalStripAutoImport::importFreeTerminals(&project);
			QCOMPARE(strips(project), QStringList({QStringLiteral(":1"), QStringLiteral("VD1:2")}));
		}

		void withoutTheSettingAsBefore()
		{
			QETProject project;
			draw(project);
			TerminalStripAutoImport::importFreeTerminals(&project);
			QCOMPARE(strips(project), QStringList({QStringLiteral(":3")}));
		}

		void placeOfTerminal()
		{
			QETProject project;
			project.setPlantAndLocationInherited(true);
			Diagram *diagram = project.addNewDiagram();
			auto *b = new StructureBoxItem(QPointF(100, 100), QPointF(300, 300));
			b->setPlant(QStringLiteral("GARAGE"));
			b->setLocation(QStringLiteral("VD1"));
			diagram->addItem(b);
			Element *inside = terminal(diagram, QStringLiteral("X1:1"), QPointF(150, 200));
			QVERIFY(inside);
			QCOMPARE(RealTerminal::placeOf(inside),
					 qMakePair(QStringLiteral("GARAGE"), QStringLiteral("VD1")));
				//Its own value wins
			DiagramContext info = inside->elementInformations();
			info.addValue(QStringLiteral("location"), QStringLiteral("VD9"));
			inside->setElementInformations(info);
			QCOMPARE(RealTerminal::placeOf(inside).second, QStringLiteral("VD9"));
			project.setPlantAndLocationInherited(false);
			QCOMPARE(RealTerminal::placeOf(inside),
					 qMakePair(QString(), QStringLiteral("VD9")));
		}
};

QET_APP_TEST_MAIN(tst_stripplacefrombox)
#include "tst_stripplacefrombox.moc"
