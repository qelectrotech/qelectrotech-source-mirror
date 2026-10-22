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
#include "TerminalStrip/physicalterminal.h"
#include "TerminalStrip/realterminal.h"
#include "TerminalStrip/terminalstrip.h"
#include "TerminalStrip/ui/terminalstripeditor.h"
#include "TerminalStrip/ui/terminalstripmodel.h"
#include "cable/cable.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetinformation.h"
#include "qetproject.h"

#include <QAbstractItemView>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QSignalSpy>
#include <QTableView>
#include <QTextCursor>

/**
	The Cable and Color / wire number columns of the terminal strip editor
	are worked out from the cables of the project: a terminal with a cable
	wired to it shows that cable and its core, the cell is read only, and
	nothing is written into the element -- so a cable renamed or removed
	cannot leave a stale value behind. A terminal no cable is wired to
	keeps the value typed in by hand, cell and all.

	The Target column follows the same rule: it names where the cable
	lands past this strip (Installation, Location and Label), the very
	three fields the strip drawing writes in its box. Worked out from a
	cable it is read only -- only the drawing decides it -- and with no
	cable landing anywhere it is there to be typed in, just like the
	Cable cell, the text then stored in the element and kept until a
	cable works a target out again. A text typed in is written one line
	per field, Installation, Location, Label, the way the column shows
	them: only a line break breaks it into fields, so a "/" written
	inside a field (a name like "ASP/01") is part of that field and the
	box draws one line containing it.

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

	static QList<Conductor *> conductorsOf(const QSharedPointer<RealTerminal> &real_t)
	{
		QList<Conductor *> conductors;
		Element *element = real_t ? real_t->element() : nullptr;
		if (!element) return conductors;
		for (Terminal *terminal : element->terminals()) {
			if (!terminal) continue;
			for (Conductor *conductor : terminal->conductors()) {
				if (conductor && !conductors.contains(conductor))
					conductors.append(conductor);
			}
		}
		return conductors;
	}

	static Conductor *conductorOf(const QSharedPointer<RealTerminal> &real_t)
	{
		const QList<Conductor *> conductors = conductorsOf(real_t);
		return conductors.isEmpty() ? nullptr : conductors.first();
	}

	// The wire which leaves the strip: one of its ends belongs to a piece
	// outside the strip, while the wire joining two terminals of this very
	// strip never leaves it -- and never has a target to name.
	static Conductor *outgoingConductor(const QSharedPointer<RealTerminal> &real_t)
	{
		TerminalStrip *strip = real_t ? real_t->parentStrip() : nullptr;
		if (!strip) return nullptr;

		QSet<QUuid> own;
		for (const QSharedPointer<PhysicalTerminal> &physical
			 : strip->physicalTerminal())
		{
			if (!physical) continue;
			for (const QSharedPointer<RealTerminal> &real : physical->realTerminals())
				if (real && real->element())
					own.insert(real->element()->uuid());
		}

		for (Conductor *conductor : conductorsOf(real_t))
		{
			if (!conductor) continue;
			Element *ends[2] = {
				conductor->terminal1 ? conductor->terminal1->parentElement() : nullptr,
				conductor->terminal2 ? conductor->terminal2->parentElement() : nullptr };
			for (Element *end : ends)
				if (end && !own.contains(end->uuid()))
					return conductor;
		}
		return nullptr;
	}

	// The cable the create dialog hands over: owned by the project, four
	// cores, one core wired to the terminal's conductor (or to @p on).
	static Cable *wireCable(QETProject &project,
							const QSharedPointer<RealTerminal> &real_t,
							int core = 1,
							const QString &designation = QStringLiteral("15W1"),
							Conductor *on = nullptr)
	{
		if (!on) on = conductorOf(real_t);
		Cable *cable = project.newCable();
		cable->setDesignation(designation);
		cable->setCoreCount(4);
		cable->setCoreColors({QStringLiteral("bn"), QStringLiteral("bl"),
							  QStringLiteral("gn/ye"), QStringLiteral("gy")});
		CableCore assignment;
		assignment.core = core;
		assignment.conductor = on->uuid();
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

	// The window stays open while the user wires the drawing: the cells
	// follow a cable wired, renamed or taken away without any reload by
	// hand.
	void openEditorFollowsCableChanges()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		const auto real_t = wiredTerminal(project);
		QVERIFY(!real_t.isNull());

		TerminalStripEditor editor(&project);
		editor.setCurrentStrip(project.terminalStrip().first());
		const QAbstractItemView *table = editor.findChild<QAbstractItemView *>(
			QStringLiteral("m_table_widget"));
		QVERIFY2(table && table->model(), "the editor has its table");
		const QAbstractItemModel *model = table->model();
		const QModelIndex cable_cell = model->index(0, TerminalStripModel::Cable);
		QVERIFY(model->data(cable_cell).toString().isEmpty());

		//A cable wired to the terminal fills the cell in
		Cable *cable = wireCable(project, real_t);
		QCOMPARE(model->data(cable_cell).toString(), QStringLiteral("15W1"));

		//Renaming it follows at once, the drawing is not stale behind
		cable->setDesignation(QStringLiteral("16W1"));
		QCOMPARE(model->data(cable_cell).toString(), QStringLiteral("16W1"));

		//Taking it away empties the cell: nothing stale stays behind
		QVERIFY(project.removeCable(cable));
		QVERIFY(model->data(cable_cell).toString().isEmpty());
	}

	// A terminal wired to two cables shows them both, comma separated.
	void twoCablesOnOneTerminalAreBothShown()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		const auto real_t = wiredTerminal(project);
		QVERIFY(!real_t.isNull());
		const QList<Conductor *> conductors = conductorsOf(real_t);
		QVERIFY2(conductors.size() >= 2,
				 "the fixture terminal carries two wires");

		wireCable(project, real_t, 1, QStringLiteral("15W1"), conductors.at(0));
		wireCable(project, real_t, 1, QStringLiteral("16W1"), conductors.at(1));

		QCOMPARE(real_t->cable(), QStringLiteral("15W1, 16W1"));
		QCOMPARE(real_t->cableWire(), QStringLiteral("bl, bl"));
		QVERIFY(real_t->cableIsAutomatic());
	}

	// The Target column names where the cable lands, past this strip: the
	// three fields the strip drawing writes in the box under the cable,
	// joined in one line. Worked out from a cable it is read only, there
	// is nothing to name without one, and the cell empties again once the
	// cable goes.
	void targetColumnNamesWhereTheCableLands()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		const auto real_t = wiredTerminal(project);
		QVERIFY(!real_t.isNull());
		Conductor *outgoing = outgoingConductor(real_t);
		QVERIFY2(outgoing, "the fixture terminal has a wire leaving the strip");

		Element *target = (outgoing->terminal1
						   && outgoing->terminal1->parentElement() == real_t->element())
						  ? (outgoing->terminal2 ? outgoing->terminal2->parentElement() : nullptr)
						  : (outgoing->terminal1 ? outgoing->terminal1->parentElement() : nullptr);
		QVERIFY(target);

		//The piece the cable lands on carries an Installation, a Location
		//and a Label, the way an element of a real drawing does
		ElementData named = target->elementData();
		named.m_informations.addValue(QETInformation::ELMT_PLANT,
									  QStringLiteral("Plant 7"));
		named.m_informations.addValue(QETInformation::ELMT_LOCATION,
									  QStringLiteral("Cabinet 7"));
		named.m_informations.addValue(QETInformation::ELMT_LABEL,
									  QStringLiteral("-S7"));
		target->setElementData(named);

		//No cable yet : nothing to name, so the cell is there to be
		//filled in by hand -- the very rule of the Cable cell
		TerminalStripModel model_before(project.terminalStrip().first());
		const QModelIndex before_cell = model_before.index(
			0, TerminalStripModel::CableTarget);
		QVERIFY(model_before.data(before_cell).toString().isEmpty());
		QVERIFY(model_before.flags(before_cell) & Qt::ItemIsEditable);

		Cable *cable = wireCable(project, real_t, 1, QStringLiteral("15W1"),
								 outgoing);

		//Row zero is the terminal the cable is wired to
		TerminalStripModel model(project.terminalStrip().first());
		QCOMPARE(model.data(model.index(0, TerminalStripModel::Cable))
				 .toString(), QStringLiteral("15W1"));

		//The three fields, one on its own line as the box draws them
		const QModelIndex target_cell = model.index(0,
													TerminalStripModel::CableTarget);
		QCOMPARE(model.data(target_cell).toString(),
				 QStringLiteral("Plant 7\nCabinet 7\n-S7"));
		QVERIFY(!(model.flags(target_cell) & Qt::ItemIsEditable));
		QCOMPARE(model.headerData(TerminalStripModel::CableTarget,
								  Qt::Horizontal).toString(),
				 QStringLiteral("Target"));

		//The cable taken away : the cell is empty again, nothing stale
		//behind, and it takes an edit once more
		QVERIFY(cable);
		cable->removeCore(1);
		TerminalStripModel reloaded(project.terminalStrip().first());
		const QModelIndex reloaded_cell = reloaded.index(
			0, TerminalStripModel::CableTarget);
		QVERIFY(reloaded.data(reloaded_cell).toString().isEmpty());
		QVERIFY(reloaded.flags(reloaded_cell) & Qt::ItemIsEditable);
	}

	// A target the drawing cannot work out is typed in by hand, exactly
	// like a cable designation: the cell takes the edit, Apply stores the
	// text in the element, a fresh model shows it again. A cable landing
	// on a named piece then gives the read only target of the drawing and
	// the text typed in by hand waits, untouched, until that cable goes.
	void targetTypedByHandIsStoredAndKept()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		const auto real_t = wiredTerminal(project);
		QVERIFY(!real_t.isNull());
		Element *element = real_t->element();
		QVERIFY(element);
		Conductor *outgoing = outgoingConductor(real_t);
		QVERIFY2(outgoing, "the fixture terminal has a wire leaving the strip");

		//The editor takes a target typed in by hand : no cable is wired,
		//so there is nothing the drawing could name and the cell is open
		TerminalStripEditor editor(&project);
		editor.setCurrentStrip(project.terminalStrip().first());
		QAbstractItemView *table = editor.findChild<QAbstractItemView *>(
			QStringLiteral("m_table_widget"));
		QVERIFY2(table && table->model(), "the editor has its table");
		QAbstractItemModel *model = table->model();
		const QModelIndex cell = model->index(0, TerminalStripModel::CableTarget);
		QVERIFY(model->data(cell).toString().isEmpty());
		QVERIFY(model->flags(cell) & Qt::ItemIsEditable);

		const QString typed{QStringLiteral("Boiler room\nCabinet 3\n-K3")};
		QVERIFY(model->setData(cell, typed, Qt::EditRole));
		QCOMPARE(model->data(cell, Qt::EditRole).toString(), typed);
		QCOMPARE(model->data(cell).toString(), typed);

		//Apply writes it in the element, and nowhere else
		editor.apply();
		QCOMPARE(element->elementData().m_informations
				 .value(RealTerminal::cableTargetInfoKey()).toString(), typed);
		//Three lines, each field on its own: the box draws them one under
		//the other, without any separator
		QCOMPARE(real_t->cableTarget(),
				 (QVector<QString>{QStringLiteral("Boiler room"),
								   QStringLiteral("Cabinet 3"),
								   QStringLiteral("-K3")}));
		QVERIFY(!real_t->cableTargetIsAutomatic());

		//A fresh model shows the text and still lets it be edited
		TerminalStripModel model_after(project.terminalStrip().first());
		const QModelIndex after_cell = model_after.index(
			0, TerminalStripModel::CableTarget);
		QCOMPARE(model_after.data(after_cell).toString(), typed);
		QVERIFY(model_after.flags(after_cell) & Qt::ItemIsEditable);

		//The piece the cable lands on is named now : once the cable is
		//wired the drawing gives the target of its own, and that one is
		//read only while the text typed in by hand stays in the element
		Element *target = (outgoing->terminal1
						   && outgoing->terminal1->parentElement() == real_t->element())
						  ? (outgoing->terminal2 ? outgoing->terminal2->parentElement() : nullptr)
						  : (outgoing->terminal1 ? outgoing->terminal1->parentElement() : nullptr);
		QVERIFY(target);
		ElementData named = target->elementData();
		named.m_informations.addValue(QETInformation::ELMT_PLANT,
									  QStringLiteral("Plant 9"));
		named.m_informations.addValue(QETInformation::ELMT_LOCATION,
									  QStringLiteral("Cabinet 9"));
		named.m_informations.addValue(QETInformation::ELMT_LABEL,
									  QStringLiteral("-S9"));
		target->setElementData(named);

		Cable *cable = wireCable(project, real_t, 1, QStringLiteral("15W1"),
								 outgoing);
		QVERIFY(cable);
		QVERIFY(real_t->cableTargetIsAutomatic());
		QCOMPARE(real_t->cableTarget(),
				 (QVector<QString>{QStringLiteral("Plant 9"),
								   QStringLiteral("Cabinet 9"),
								   QStringLiteral("-S9")}));
		QCOMPARE(element->elementData().m_informations
				 .value(RealTerminal::cableTargetInfoKey()).toString(), typed);

		TerminalStripModel model_cable(project.terminalStrip().first());
		const QModelIndex cable_cell = model_cable.index(
			0, TerminalStripModel::CableTarget);
		QCOMPARE(model_cable.data(cable_cell).toString(),
				 QStringLiteral("Plant 9\nCabinet 9\n-S9"));
		QVERIFY(!(model_cable.flags(cable_cell) & Qt::ItemIsEditable));
		//And the automatic target was never written in the element
		QCOMPARE(element->elementData().m_informations
				 .value(RealTerminal::cableTargetInfoKey()).toString(), typed);

		//The cable taken away : the text typed in by hand is back, cell
		//and all, no stale Plant 9 anywhere
		cable->removeCore(1);
		QVERIFY(!real_t->cableTargetIsAutomatic());
		QCOMPARE(real_t->cableTarget(),
				 (QVector<QString>{QStringLiteral("Boiler room"),
								   QStringLiteral("Cabinet 3"),
								   QStringLiteral("-K3")}));
		TerminalStripModel model_back(project.terminalStrip().first());
		const QModelIndex back_cell = model_back.index(
			0, TerminalStripModel::CableTarget);
		QCOMPARE(model_back.data(back_cell).toString(), typed);
		QVERIFY(model_back.flags(back_cell) & Qt::ItemIsEditable);
	}

	// A text typed in by hand is written one line per field, and only a
	// line break breaks it into fields: the box under the cable draws
	// one line per field, exactly like a target worked out from the
	// project, while a "/" written inside a field is part of it and
	// gives one single line holding it.
	void typedTargetGivesOneLinePerField()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		const auto real_t = wiredTerminal(project);
		QVERIFY(!real_t.isNull());
		Element *element = real_t->element();
		QVERIFY(element);

		//One field only: one single line in the box
		ElementData typed = element->elementData();
		typed.m_informations.addValue(RealTerminal::cableTargetInfoKey(),
									  QStringLiteral("Boiler room"));
		element->setElementData(typed);
		QCOMPARE(real_t->cableTarget(),
				 QVector<QString>{QStringLiteral("Boiler room")});

		//Three fields, written one per line: three lines, and no
		//separator anywhere in them
		typed = element->elementData();
		typed.m_informations.addValue(RealTerminal::cableTargetInfoKey(),
									  QStringLiteral("Plant 9\nCabinet 9\n-S9"));
		element->setElementData(typed);
		QCOMPARE(real_t->cableTarget(),
				 (QVector<QString>{QStringLiteral("Plant 9"),
								   QStringLiteral("Cabinet 9"),
								   QStringLiteral("-S9")}));
		//A typed text is never a target of the drawing
		QVERIFY(!real_t->cableTargetIsAutomatic());

		//The column shows the text back the way it was written
		TerminalStripModel model(project.terminalStrip().first());
		QCOMPARE(model.data(model.index(0, TerminalStripModel::CableTarget))
				 .toString(), QStringLiteral("Plant 9\nCabinet 9\n-S9"));

		//A slash inside a field, and one typed between two words of a
		//field kept in one line: both stay part of the field, the cell
		//shows them untouched and the box holds one single line each
		typed = element->elementData();
		typed.m_informations.addValue(RealTerminal::cableTargetInfoKey(),
									  QStringLiteral("Betrieb 1/2"));
		element->setElementData(typed);
		QCOMPARE(real_t->cableTarget(),
				 QVector<QString>{QStringLiteral("Betrieb 1/2")});

		typed = element->elementData();
		typed.m_informations.addValue(RealTerminal::cableTargetInfoKey(),
									  QStringLiteral("Betrieb 1/2 / Keller"));
		element->setElementData(typed);
		QCOMPARE(real_t->cableTarget(),
				 QVector<QString>{QStringLiteral("Betrieb 1/2 / Keller")});
		TerminalStripModel slashed(project.terminalStrip().first());
		QCOMPARE(slashed.data(slashed.index(0, TerminalStripModel::CableTarget))
				 .toString(), QStringLiteral("Betrieb 1/2 / Keller"));

		//The line breaks written by hand are the fields again, whatever
		//the fields themselves contain
		typed = element->elementData();
		typed.m_informations.addValue(RealTerminal::cableTargetInfoKey(),
									  QStringLiteral("Betrieb 1/2\nKeller\n-K4"));
		element->setElementData(typed);
		QCOMPARE(real_t->cableTarget(),
				 (QVector<QString>{QStringLiteral("Betrieb 1/2"),
								   QStringLiteral("Keller"),
								   QStringLiteral("-K4")}));
	}

	// The Target cell is edited line by line: its editor holds one line
	// per field and hands the text back exactly as the cell holds it.
	// Return writes a new line (the cell is multi line), Ctrl+Return
	// gives the cell back without waiting for the focus to go somewhere
	// else, and the row of the table is as tall as the lines the cell
	// shows, so none of them is cut off.
	void targetCellIsEditedLineByLine()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		TerminalStripEditor editor(&project);
		editor.setCurrentStrip(project.terminalStrip().first());
		QTableView *table = editor.findChild<QTableView *>(
			QStringLiteral("m_table_widget"));
		QVERIFY2(table && table->model(), "the editor has its table");
		QAbstractItemModel *model = table->model();
		QVERIFY(model->rowCount() > 1);
		const QModelIndex cell = model->index(0, TerminalStripModel::CableTarget);
		QVERIFY(model->flags(cell) & Qt::ItemIsEditable);

		const QString typed{QStringLiteral("Betrieb 1/2\nKeller\n-K4")};
		QVERIFY(model->setData(cell, typed, Qt::EditRole));
		QCOMPARE(model->data(cell).toString(), typed);

			//The editor of the table is the one taking a cell of several
			//lines, and one of its own does the very same thing for the
			//keys, without a view in between
		QVERIFY(qobject_cast<TerminalStripModelDelegate *>(
					table->itemDelegate()));
		TerminalStripModelDelegate delegate;

		QStyleOptionViewItem option;
		option.rect = QRect(0, 0, qMax(
			50, table->columnWidth(TerminalStripModel::CableTarget)), 400);
		option.font = table->font();
		option.fontMetrics = QFontMetrics(option.font);

		QWidget *cell_editor = delegate.createEditor(table, option, cell);
		QVERIFY(cell_editor);
		auto *text_edit = qobject_cast<QPlainTextEdit *>(cell_editor);
		QVERIFY2(text_edit, "the Target cell takes a multi line editor");
		QCOMPARE(text_edit->objectName(),
				 QStringLiteral("terminal_cable_target"));

			//The editor gives the lines back exactly as the cell holds
			//them: nothing joined, nothing split again
		delegate.setEditorData(cell_editor, cell);
		QCOMPARE(text_edit->toPlainText(), typed);
		delegate.setModelData(cell_editor, model, cell);
		QCOMPARE(model->data(cell).toString(), typed);

			//Return writes a new line in the cell, and hands nothing over
			//yet: Ctrl+Return is the key giving the cell back
		QSignalSpy committed(&delegate, &TerminalStripModelDelegate::commitData);
		QSignalSpy closed(&delegate, &TerminalStripModelDelegate::closeEditor);
		text_edit->moveCursor(QTextCursor::End);

		QKeyEvent return_key{QEvent::KeyPress, Qt::Key_Return,
							 Qt::NoModifier};
		QCoreApplication::sendEvent(cell_editor, &return_key);
		QCOMPARE(text_edit->toPlainText(), typed + QLatin1Char('\n'));
		QCOMPARE(committed.count(), 0);
		QCOMPARE(closed.count(), 0);

		QKeyEvent ctrl_return_key{QEvent::KeyPress, Qt::Key_Return,
								  Qt::ControlModifier};
		QCoreApplication::sendEvent(cell_editor, &ctrl_return_key);
		QCOMPARE(committed.count(), 1);
		QCOMPARE(closed.count(), 1);
		delete cell_editor;
	}

	// The row of the table follows the lines its Target cell holds: the
	// cell is never cut off, however many lines it takes, the row grows
	// to hold them all and shrinks back afterwards, while the rows
	// around keep the height they have always had.
	void targetRowGrowsWithTheLinesOfItsCell()
	{
		const QString fixture = QFINDTESTDATA("fixtures/every_feature.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		TerminalStripEditor editor(&project);
		editor.setCurrentStrip(project.terminalStrip().first());
		QTableView *table = editor.findChild<QTableView *>(
			QStringLiteral("m_table_widget"));
		QVERIFY2(table && table->model(), "the editor has its table");
		QAbstractItemModel *model = table->model();
		QVERIFY(model->rowCount() > 1);
		const QModelIndex cell = model->index(0, TerminalStripModel::CableTarget);
		QVERIFY(model->flags(cell) & Qt::ItemIsEditable);
		QVERIFY2(model->data(cell).toString().isEmpty(),
				 "no target to start with, the row is the usual one");

		const QModelIndex other_cell = model->index(
			1, TerminalStripModel::CableTarget);
		const int base_row = table->rowHeight(cell.row());
		const int base_other_row = table->rowHeight(other_cell.row());
		QVERIFY(base_row > 0);

			//Three lines in the cell: the row holds them all, at the
			//font of the table -- the row was already taller than that
			//for what else the terminal draws, so it may stay as it is
		const QString typed{QStringLiteral("Betrieb 1/2\nKeller\n-K4")};
		QVERIFY(model->setData(cell, typed, Qt::EditRole));
		QCOMPARE(model->data(cell).toString(), typed);

		const auto heightOfLines = [table](int lines_) {
			return lines_ * table->fontMetrics().height();
		};
		const int three_line_row = table->rowHeight(cell.row());
		QVERIFY2(three_line_row >= qMax(base_row, heightOfLines(3)),
				 "the row must be as tall as both the lines the cell "
				 "shows and what it held before, or something is cut off");
		QVERIFY2(table->rowHeight(other_cell.row()) >= base_other_row,
				 "the rows around are not pushed down");

			//From here on the rows have been worked out once: what the
			//other rows hold is the height they keep
		const int other_row = table->rowHeight(other_cell.row());

			//Many more lines than anything else in the row: the row grows
			//to hold them, and the rows around stay where they are
		const QString many{QStringLiteral(
			"Plant 6\nCabinet 6\n-K6\nZone 6\nLevel 6\nEnd 6")};
		QVERIFY(model->setData(cell, many, Qt::EditRole));
		const int six_line_row = table->rowHeight(cell.row());
		QVERIFY2(six_line_row >= heightOfLines(6),
				 "the row must be as tall as the lines the cell shows, "
				 "or the fields below the first one are cut off");
		QVERIFY2(six_line_row > qMax(base_row, three_line_row),
				 "a cell which took six lines must take a taller row "
				 "than the one it had before");
		QVERIFY2(table->rowHeight(other_cell.row()) == other_row,
				 "the rows around keep their own height");

			//Back to one single line: the row follows again
		QVERIFY(model->setData(cell, QStringLiteral("Keller"), Qt::EditRole));
		QVERIFY2(table->rowHeight(cell.row()) < six_line_row,
				 "the row follows the cell back to a single line");
		QVERIFY2(table->rowHeight(cell.row())
					 <= base_row + table->fontMetrics().height(),
				 "the row is back to the height it had before");
		QVERIFY2(table->rowHeight(cell.row()) >= table->fontMetrics().height(),
				 "the single line left is still shown");
	}
};

QET_APP_TEST_MAIN(tst_terminalcableauto)

#include "tst_terminalcableauto.moc"
