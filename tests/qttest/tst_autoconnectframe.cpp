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
	//Automatic connections: only the frame of a symbol stops a wire, not
	//the pixel the line width adds around its drawing
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "autoconnect.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"

class tst_autoconnectframe : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;

		Element *place(const QString &name, const QPointF &pos)
		{
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name), m_diagram->project()),
				nullptr, &state);
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

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
			const QList<QPair<QString, QString>> definitions {
					//A T-junction whose line runs from its top terminal to
					//its bottom one, the branch to the right
				{QStringLiteral("tee"), QStringLiteral(
					R"(<definition type="element" link_type="terminal" width="10" height="20" hotspot_x="0" hotspot_y="10">
<names><name lang="en">tee</name></names><description>
<line x1="0" y1="-10" x2="0" y2="10" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<line x1="0" y1="0" x2="10" y2="0" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="0" y="-10" orientation="n" name="1"/><terminal x="0" y="10" orientation="s" name="2"/>
<terminal x="10" y="0" orientation="e" name="3"/></description></definition>)")},
				{QStringLiteral("target"), QStringLiteral(
					R"(<definition type="element" link_type="simple" width="20" height="20" hotspot_x="0" hotspot_y="10">
<names><name lang="en">target</name></names><description>
<rect x="0" y="-10" width="20" height="20" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="0" y="0" orientation="w" name="w"/></description></definition>)")}};
			for (const auto &d : definitions) {
				QDomDocument doc;
				doc.setContent(d.second);
				QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
					QStringLiteral("import"), d.first, doc.documentElement()));
			}
		}

			//A second T-junction one step down and two to the right: its top
			//terminal lies on the first one's branch line, on the edge of
			//its frame. The branch still finds the symbol beyond.
		void neighbourOneStepDown()
		{
			Element *upper = place(QStringLiteral("tee"), QPointF(100, 100));
			Element *lower = place(QStringLiteral("tee"), QPointF(120, 110));
			Element *target = place(QStringLiteral("target"), QPointF(200, 100));
			QVERIFY(upper && lower && target);
			QCOMPARE(named(lower, "1")->dockConductor(), QPointF(120, 100));
			QCOMPARE(AutoConnect::partner(named(upper, "3")), named(target, "w"));
		}

			//A symbol whose frame the line runs through still stops it
		void frameStillStops()
		{
			Element *upper = place(QStringLiteral("tee"), QPointF(100, 300));
			Element *across = place(QStringLiteral("tee"), QPointF(150, 300));
			Element *target = place(QStringLiteral("target"), QPointF(200, 300));
			QVERIFY(upper && across && target);
			QVERIFY(AutoConnect::partner(named(upper, "3")) != named(target, "w"));
		}
};

QET_APP_TEST_MAIN(tst_autoconnectframe)
#include "tst_autoconnectframe.moc"
