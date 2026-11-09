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
	//A T-junction can be drawn as its lines and a dot where they meet; the
	//choice is kept with the project and undone in one step
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

#include <QImage>
#include <QPainter>

class tst_junctiondot : public QObject
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

			//Whether the folio is drawn dark just right and below the
			//middle of @a element: on the dot, off the T's own lines
		bool darkBesideMiddle(Element *element)
		{
			const QPointF at = element->mapToScene(QPointF(1, 1));
			QImage image(40, 40, QImage::Format_ARGB32);
			image.fill(Qt::white);
			QPainter painter(&image);
			m_diagram->render(&painter, QRectF(0, 0, 40, 40), QRectF(at - QPointF(20, 20), QSizeF(40, 40)));
			painter.end();
			return qGray(image.pixel(20, 20)) < 220;
		}

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
			const QList<QPair<QString, QString>> definitions {
				{QStringLiteral("tee"), QStringLiteral(
					R"(<definition type="element" link_type="terminal" width="10" height="20" hotspot_x="0" hotspot_y="10">
<names><name lang="en">tee</name></names><description>
<line x1="0" y1="-10" x2="0" y2="10" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<line x1="0" y1="0" x2="10" y2="0" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="0" y="-10" orientation="n" name="1"/><terminal x="0" y="10" orientation="s" name="2"/>
<terminal x="10" y="0" orientation="e" name="3"/></description></definition>)")},
					//The branch nearer one end of the line than the
					//other: the two ends are not the farthest apart
				{QStringLiteral("shortTee"), QStringLiteral(
					R"(<definition type="element" link_type="terminal" width="30" height="30" hotspot_x="18" hotspot_y="10">
<names><name lang="en">short tee</name></names><description>
<polygon x1="-10" y1="0" x2="4" y2="0" closed="false" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="0" y="10" orientation="s" name="3"/><terminal x="-10" y="0" orientation="w" name="1"/>
<terminal x="4" y="0" orientation="e" name="2"/></description></definition>)")},
				{QStringLiteral("corner"), QStringLiteral(
					R"(<definition type="element" link_type="terminal" width="10" height="10" hotspot_x="0" hotspot_y="10">
<names><name lang="en">corner</name></names><description>
<polygon x1="0" y1="-10" x2="0" y2="0" x3="10" y3="0" closed="false" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="0" y="-10" orientation="n" name="1"/><terminal x="10" y="0" orientation="e" name="2"/></description></definition>)")}};
			for (const auto &d : definitions) {
				QDomDocument doc;
				doc.setContent(d.second);
				QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
					QStringLiteral("import"), d.first, doc.documentElement()));
			}
		}

		void onlyATeeHasADot()
		{
			Element *tee = place(QStringLiteral("tee"), QPointF(100, 100));
			Element *corner = place(QStringLiteral("corner"), QPointF(200, 100));
			QVERIFY(tee && corner);
			QCOMPARE(tee->dotPoint().value_or(QPointF(-1, -1)), QPointF(0, 0));
			QVERIFY(!corner->dotPoint());
			Element *short_tee = place(QStringLiteral("shortTee"), QPointF(300, 100));
			QVERIFY(short_tee);
			QCOMPARE(short_tee->dotPoint().value_or(QPointF(-1, -1)), QPointF(0, 0));
		}

		void drawnWithADot()
		{
			Element *tee = place(QStringLiteral("tee"), QPointF(100, 300));
			QVERIFY(tee);
			QVERIFY(!darkBesideMiddle(tee));
			tee->setShownAsDot(true);
			QVERIFY(darkBesideMiddle(tee));
		}

		void keptInTheFile()
		{
			Element *tee = place(QStringLiteral("tee"), QPointF(100, 500));
			QVERIFY(tee);
			QDomDocument doc;
			QHash<Terminal *, int> ids;
			QDomElement plain = tee->toXml(doc, ids);
			QVERIFY(!plain.hasAttribute(QStringLiteral("shownAsDot")));
			tee->setShownAsDot(true);
			QDomElement saved = tee->toXml(doc, ids);
			QCOMPARE(saved.attribute(QStringLiteral("shownAsDot")), QStringLiteral("true"));
			Element *again = place(QStringLiteral("tee"), QPointF(200, 500));
			QHash<int, Terminal *> terminals;
			QVERIFY(again->fromXml(saved, terminals));
			QVERIFY(again->isShownAsDot());
		}

		void undone()
		{
			Element *tee = place(QStringLiteral("tee"), QPointF(100, 700));
			QVERIFY(tee);
			m_diagram->undoStack().push(new QPropertyUndoCommand(tee, "shownAsDot", false, true));
			QVERIFY(tee->isShownAsDot());
			m_diagram->undoStack().undo();
			QVERIFY(!tee->isShownAsDot());
		}
};

QET_APP_TEST_MAIN(tst_junctiondot)
#include "tst_junctiondot.moc"
