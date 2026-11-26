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
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include <QDomDocument>
#include <QtTest>

/**
	Bugtracker #349: terminals 10 apart, pointing south, with their names
	shown. The box of a name reached over the terminals beside it, and the
	terminal on top took their clicks, so some could not be reached.
*/
class tst_terminalnamehitarea : public QObject
{
	Q_OBJECT

		/// A symbol with three terminals 10 apart along X, pointing south,
		/// each showing a name centred on it and turned by rotation
	static Element *symbol(Diagram *diagram, const QString &name, int rotation)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		QString terminals;
		for (int x : {-10, 0, 10})
			terminals += QString(R"(<terminal x="%1" y="20" orientation="s" name="X1:%2" show_name="true" label_x="0" label_y="-4" label_halign="%3" label_valign="%4" label_rotation="%5" uuid="%6"/>)")
							 .arg(x).arg(x + 100)
							 .arg(int(Qt::AlignHCenter)).arg(int(Qt::AlignVCenter))
							 .arg(rotation).arg(QUuid::createUuid().toString());
		QDomDocument definition;
		definition.setContent(
			QString(R"(<definition type="element" link_type="simple" uuid="%1" width="40" height="40" hotspot_x="20" hotspot_y="20">
<names><name lang="en">Test</name></names>
<description><line x1="-10" y1="0" x2="10" y2="0"/>%2</description></definition>)")
				.arg(QUuid::createUuid().toString(), terminals));
		if (!collection->addElementDefinition(QStringLiteral("import"), name,
											  definition.documentElement()))
			return nullptr;
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name),
							 diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		element->setPos(100, 100);
		return element;
	}

		/// The terminal the mouse reaches at pos: the top one under it
	static Terminal *terminalAt(Diagram *diagram, const QPointF &pos)
	{
		for (QGraphicsItem *item : diagram->items(pos))
			if (auto *t = qgraphicsitem_cast<Terminal *>(item)) return t;
		return nullptr;
	}

private slots:
	void initTestCase() { QETProject::setBackupEnabled(false); }

	void eachTerminalTakesItsOwnClick()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Element *element = symbol(diagram, QStringLiteral("test_named"), 0);
		QVERIFY(element);
		QCOMPARE(element->terminals().size(), 3);
		for (Terminal *t : element->terminals()) {
				//On the terminal, a little in from where a wire ends
			const QPointF on_terminal = t->dockConductor() + QPointF(0, -3);
			QCOMPARE(terminalAt(diagram, on_terminal), t);
		}
	}

	void aTurnedNameIsInsideTheBoundingRect()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Element *element = symbol(diagram, QStringLiteral("test_named_turned"), 90);
		QVERIFY(element);
			//Turned upright, the name runs along Y: its box, and so the
			//terminal's, is taller than wide. Unturned it was wider.
		const QRectF br = element->terminals().value(0)->boundingRect();
		QVERIFY2(br.height() > br.width(),
				 qPrintable(QStringLiteral("%1 x %2").arg(br.width()).arg(br.height())));
	}
};

QET_APP_TEST_MAIN(tst_terminalnamehitarea)
#include "tst_terminalnamehitarea.moc"
