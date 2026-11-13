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
#include "conductorproperties.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "ui/potentialselectordialog.h"
#include <QDomDocument>
#include <QRadioButton>
#include <QtTest>

/**
	A wire that joins two potentials asks which one to keep. The question
	offered two more choices, "Add to cable" and "Add to bus", that were
	always disabled: only the two potentials are offered now.
*/
class tst_potentialchoices : public QObject
{
	Q_OBJECT

		/// A symbol with two terminals, top and bottom
	static Element *symbol(Diagram *diagram, const QPointF &pos)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		const QString name = QStringLiteral("test_simple");
		if (!collection->exist(QStringLiteral("import/%1.elmt").arg(name))) {
			QDomDocument definition;
			definition.setContent(
				QString(R"(<definition type="element" link_type="simple" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%2"/><terminal x="0" y="20" orientation="s" uuid="%3"/>
</description></definition>)")
					.arg(QUuid::createUuid().toString(), QUuid::createUuid().toString(),
						 QUuid::createUuid().toString()));
			if (!collection->addElementDefinition(QStringLiteral("import"), name,
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name),
							 diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		element->setPos(pos);
		return element;
	}

	static Terminal *top(Element *element)
	{
		Terminal *found = element->terminals().value(0);
		for (Terminal *t : element->terminals())
			if (t->scenePos().y() < found->scenePos().y()) found = t;
		return found;
	}

	static Conductor *wire(Diagram *diagram, Terminal *a, Terminal *b, const QString &number)
	{
		auto *conductor = new Conductor(a, b);
		diagram->addItem(conductor);
		ConductorProperties properties = conductor->properties();
		properties.text = number;
		conductor->setProperties(properties);
		return conductor;
	}

private slots:
	void initTestCase() { QETProject::setBackupEnabled(false); }

	void onlyTheTwoPotentialsAreOffered()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Element *k1 = symbol(diagram, {100, 100});
		Element *k2 = symbol(diagram, {200, 100});
		Element *k3 = symbol(diagram, {100, 300});
		Element *k4 = symbol(diagram, {200, 300});
		QVERIFY(k1 && k2 && k3 && k4);
		wire(diagram, top(k1), top(k2), QStringLiteral("1"));
		wire(diagram, top(k3), top(k4), QStringLiteral("2"));
			//The wire joining potential 1 and potential 2
		Conductor *joining = wire(diagram, top(k1), top(k3), QString());

		PotentialSelectorDialog dialog(joining);
		const QList<QRadioButton *> choices = dialog.findChildren<QRadioButton *>();
		QCOMPARE(choices.size(), 2);
		for (QRadioButton *choice : choices) {
			QVERIFY2(!choice->text().startsWith(QStringLiteral("Add to cable")), qPrintable(choice->text()));
			QVERIFY2(!choice->text().startsWith(QStringLiteral("Add to bus")), qPrintable(choice->text()));
			QVERIFY(choice->isEnabled());
		}
	}
};

QET_APP_TEST_MAIN(tst_potentialchoices)
#include "tst_potentialchoices.moc"
