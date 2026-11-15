// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "diagramevent/diagrameventaddelement.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

#include <QDomDocument>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QRegularExpression>

/**
	A symbol which cannot be built (here: no width, height or hotspot,
	load error 5) must leave nothing behind when it is picked for
	placing. The placing tool kept a pointer to the symbol it had just
	deleted, and every mouse move over the folio wrote into freed
	memory until QElectroTech went down while painting.
*/
class tst_brokenelementplacement : public QObject
{
	Q_OBJECT

	static QDomElement definition(QDomDocument &doc, bool with_size)
	{
		doc.setContent(QStringLiteral(
			"<definition type=\"element\" link_type=\"simple\" version=\"0.80\"%1>"
			"<uuid uuid=\"{0b6f1c9e-3c7e-4f51-9d0e-1f2a3b4c5d6e}\"/>"
			"<names><name lang=\"en\">probe</name></names>"
			"<description>"
			"<line x1=\"-5\" y1=\"0\" x2=\"5\" y2=\"0\""
			" style=\"line-style:normal;line-weight:normal;filling:none;color:black\""
			" antialias=\"false\" end1=\"none\" end2=\"none\" length1=\"1.5\" length2=\"1.5\"/>"
			"<terminal x=\"0\" y=\"0\" orientation=\"n\"/>"
			"</description></definition>")
			.arg(with_size ? QStringLiteral(" width=\"20\" height=\"20\" hotspot_x=\"10\" hotspot_y=\"10\"")
				       : QString()));
		return doc.documentElement();
	}

	private slots:
		void brokenSymbolLeavesNoPendingElement()
		{
			Diagram *diagram = QetAppTest::diagram();
			QVERIFY(diagram);
			QETProject *project = diagram->project();
			QVERIFY(project);

			QDomDocument doc;
			QVERIFY(project->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("broken_probe"),
				definition(doc, false)));
			ElementsLocation broken(QStringLiteral("embed://import/broken_probe.elmt"), project);
			QVERIFY(broken.exist());

			int state = 0;
			delete ElementFactory::Instance()->createElement(broken, nullptr, &state);
			QCOMPARE(state, 5);

			const int before = diagram->elements().size();
			{
				DiagramEventAddElement tool(broken, diagram);
				QVERIFY(!tool.isRunning());
				QVERIFY2(!tool.pendingElement(),
					"the placing tool still points at the symbol it deleted");

					//What the folio sends while the tool is still set
				QGraphicsSceneMouseEvent move(QEvent::GraphicsSceneMouseMove);
				move.setScenePos(QPointF(50, 50));
				tool.mouseMoveEvent(&move);
				QKeyEvent space(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
				tool.keyPressEvent(&space);
				QGraphicsSceneMouseEvent release(QEvent::GraphicsSceneMouseRelease);
				release.setButton(Qt::LeftButton);
				release.setScenePos(QPointF(50, 50));
				tool.mouseReleaseEvent(&release);
			}
			QCOMPARE(diagram->elements().size(), before);
		}

		void aSymbolGoesDownWithoutANullDisconnect()
		{
			Diagram *diagram = QetAppTest::diagram();
			QVERIFY(diagram);
			QETProject *project = diagram->project();
			QVERIFY(project);

			QDomDocument doc;
			QVERIFY(project->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("good_probe"),
				definition(doc, true)));
			ElementsLocation good(QStringLiteral("embed://import/good_probe.elmt"), project);

			QTest::failOnWarning(QRegularExpression(QStringLiteral("Unexpected nullptr parameter")));
			int state = -1;
			Element *element = ElementFactory::Instance()->createElement(good, nullptr, &state);
			QCOMPARE(state, 0);
			QVERIFY(element);
			delete element;
		}
};

QET_APP_TEST_MAIN(tst_brokenelementplacement)
#include "tst_brokenelementplacement.moc"
