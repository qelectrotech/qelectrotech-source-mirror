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
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "TerminalStrip/ui/terminalstripmodel.h"
#include "cable/cable.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"

/**
	The Cable and Color / wire number columns of the terminal strip editor
	are worked out from the cables of the project: a terminal with a cable
	wired to it shows that cable and its core, the cell is read only, and
	nothing is written into the element -- so a cable renamed or removed
	cannot leave a stale value behind. A terminal no cable is wired to
	keeps the value typed in by hand, cell and all.

	fixtures/every_feature.qet has the strip X1 whose two terminals are
	wired to conductors on the folio, and no cable at all.
*/
class tst_terminalcableauto : public QObject
{
	Q_OBJECT

	// The wired terminal of strip X1: its element carries a conductor.
	static QSharedPointer<RealTerminal> wiredTerminal(QETProject &project)
	{
		const QVector<TerminalStrip *> strips = project.terminalStrip();
		if (strips.isEmpty()) return {};
		for (const QSharedPointer<RealTerminal> &real_t
			 : strips.first()->realTerminals())
		{
			Element *element = real_t ? real_t->element() : nullptr;
			if (!element) continue;
			for (Terminal *terminal : element->terminals())
				if (terminal && !terminal->conductors().isEmpty())
					return real_t;
		}
		return {};
	}

	static Conductor *conductorOf(const QSharedPointer<RealTerminal> &real_t)
	{
		Element *element = real_t->element();
		if (!element) return nullptr;
		for (Terminal *terminal : element->terminals()) {
			if (!terminal) continue;
			const QList<Conductor *> conductors = terminal->conductors();
			if (!conductors.isEmpty()) return conductors.first();
		}
		return nullptr;
	}

	// The cable the create dialog hands over: owned by the project, four
	// cores, core 1 (blue) wired to the terminal's conductor.
	static Cable *wireCable(QETProject &project,
							const QSharedPointer<RealTerminal> &real_t,
							int core = 1)
	{
		Cable *cable = project.newCable();
		cable->setDesignation(QStringLiteral("15W1"));
		cable->setCoreCount(4);
		cable->setCoreColors({QStringLiteral("bn"), QStringLiteral("bl"),
							  QStringLiteral("gn/ye"), QStringLiteral("gy")});
		CableCore assignment;
		assignment.core = core;
		assignment.conductor = conductorOf(real_t)->uuid();
		cable->setCore(assignment);
		return cable;
	}

private slots:
	void wiredCableIsWorkedOutAndReadsOnly()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QVERIFY2(!fixture.isEmpty(), "fixture project not found");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		const auto real_t = wiredTerminal(project);
		QVERIFY2(!real_t.isNull(), "the fixture has a wired terminal");
		Element *element = real_t->element();

		//Without a cable, the cells show nothing and may be typed in
		QVERIFY(!real_t->cableIsAutomatic());
		TerminalStripModel model_before(project.terminalStrip().first());
		QVERIFY(model_before.rowCount() > 0);
		QVERIFY(model_before.flags(model_before.index(0, TerminalStripModel::Cable))
				& Qt::ItemIsEditable);

		wireCable(project, real_t);

		//The terminal itself reads the cable and its core
		QCOMPARE(real_t->cable(), QStringLiteral("15W1"));
		QVERIFY(real_t->cableIsAutomatic());
		QCOMPARE(real_t->cableWire(), QStringLiteral("bl"));

		//The automatic value is never written into the element: nothing
		//stays behind once the cable is renamed or removed
		QVERIFY(element);
		QVERIFY(element->elementData().m_informations
					.value(RealTerminal::cableInfoKey()).toString().isEmpty());

		//The model shows both cells and keeps them out of the edits
		TerminalStripModel model(project.terminalStrip().first());
		QCOMPARE(model.rowCount(), model_before.rowCount());
		const QModelIndex cable_cell = model.index(0, TerminalStripModel::Cable);
		const QModelIndex wire_cell = model.index(0, TerminalStripModel::CableWire);
		QCOMPARE(model.data(cable_cell).toString(), QStringLiteral("15W1"));
		QCOMPARE(model.data(wire_cell).toString(), QStringLiteral("bl"));
		QVERIFY(!(model.flags(cable_cell) & Qt::ItemIsEditable));
		QVERIFY(!(model.flags(wire_cell) & Qt::ItemIsEditable));
		//The other cells go on being editable
		QVERIFY(model.flags(model.index(0, TerminalStripModel::Label))
				& Qt::ItemIsEditable);
	}

	void coreWithoutColorIsShownAsItsNumber()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		const auto real_t = wiredTerminal(project);
		QVERIFY(!real_t.isNull());

		Cable *cable = project.newCable();
		cable->setDesignation(QStringLiteral("15W1"));
		cable->setCoreCount(4);
		//A type file which gives no colours: the number of the core is shown
		CableCore assignment;
		assignment.core = 1;
		assignment.conductor = conductorOf(real_t)->uuid();
		cable->setCore(assignment);

		QCOMPARE(real_t->cable(), QStringLiteral("15W1"));
		QCOMPARE(real_t->cableWire(), QStringLiteral("2"));
		QVERIFY(real_t->cableIsAutomatic());
	}

	void typedValueStaysWhenNoCableIsThere()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		const auto real_t = wiredTerminal(project);
		QVERIFY(!real_t.isNull());
		Element *element = real_t->element();
		QVERIFY(element);

		//A cable designation typed in by hand in the editor
		ElementData typed = element->elementData();
		typed.m_informations.addValue(RealTerminal::cableInfoKey(),
									  QStringLiteral("K9"));
		element->setElementData(typed);
		QCOMPARE(real_t->cable(), QStringLiteral("K9"));
		QVERIFY(!real_t->cableIsAutomatic());

		//Wired to a cable: the cable is shown, the typed value is set aside
		Cable *cable = wireCable(project, real_t);
		QCOMPARE(real_t->cable(), QStringLiteral("15W1"));
		QVERIFY(real_t->cableIsAutomatic());
		QCOMPARE(real_t->cableWire(), QStringLiteral("bl"));

		//Cable removed: the typed value is back, no stale 15W1 anywhere
		cable->removeCore(1);
		QCOMPARE(real_t->cable(), QStringLiteral("K9"));
		QVERIFY(!real_t->cableIsAutomatic());
		QVERIFY(element->elementData().m_informations
					.value(RealTerminal::cableInfoKey())
					.toString() == QStringLiteral("K9"));

		TerminalStripModel model(project.terminalStrip().first());
		QVERIFY(model.flags(model.index(0, TerminalStripModel::Cable))
				& Qt::ItemIsEditable);
	}
};

QET_APP_TEST_MAIN(tst_terminalcableauto)

#include "tst_terminalcableauto.moc"
