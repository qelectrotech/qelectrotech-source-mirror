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
//The names of the terminals and how they are drawn belong to the copy of
//the symbol they are set on: three copies of one symbol can read
//differently, the definition keeps its own values, and taking the values
//back shows the definition's again
#include "qetapptest.h"
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "properties/terminallabel.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "ui/elementterminalswidget.h"

#include <QCheckBox>
#include <QDomDocument>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QUndoStack>

class tst_terminallabels : public QObject
{
	Q_OBJECT

	private:
		/// A symbol with two terminals, X1:1 west and X1:2 east, neither
		/// showing its name. The terminal uuids are the ones the
		/// definition gives: every copy of a symbol carries them, and a
		/// label saved with one copy is keyed on them
		void define(Diagram *diagram, const QString &name)
		{
			QDomDocument definition;
			definition.setContent(QStringLiteral(
				R"(<definition type="element" link_type="simple" width="40" height="40" hotspot_x="20" hotspot_y="20">
<names><name lang="en">%1</name></names>
<description><line x1="-10" y1="0" x2="10" y2="0"/>
<terminal x="-10" y="0" orientation="w" name="X1:1" uuid="00000000-0000-4000-8000-000000000001"/>
<terminal x="10" y="0" orientation="e" name="X1:2" uuid="00000000-0000-4000-8000-000000000002"/></description></definition>)")
				.arg(name));
			QVERIFY(diagram->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), name, definition.documentElement()));
		}

		Element *place(Diagram *diagram, const QString &name)
		{
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name),
								 diagram->project()),
				nullptr, &state);
			if (state || !element)
				return nullptr;
			diagram->addItem(element);
			return element;
		}

		static Terminal *named(Element *element, const QString &name)
		{
			for (Terminal *t : element->terminals())
				if (t->name() == name)
					return t;
			return nullptr;
		}

		static QLineEdit *nameEdit(ElementTerminalsWidget *tab, int row)
		{
			auto *table = tab->findChild<QTableWidget *>();
			return table ? qobject_cast<QLineEdit *>(table->cellWidget(row, 1)) : nullptr;
		}

	private slots:
		void initTestCase() {QETProject::setBackupEnabled(false);}

		//A symbol which was given nothing saves nothing: a project opened
		//and saved again keeps every element it had
		void nothingIsSavedWithoutAChange()
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			define(diagram, QStringLiteral("untouched"));
			Element *element = place(diagram, QStringLiteral("untouched"));
			QVERIFY(element);
			QCOMPARE(element->terminals().size(), 2);
			QVERIFY(element->terminalLabels().isEmpty());
			for (Terminal *t : element->terminals())
				QVERIFY(!t->labelIsOverridden());

			QDomDocument document;
			QHash<Terminal *, int> table;
			const QDomElement saved = element->toXml(document, table);
			QVERIFY(saved.firstChildElement(QStringLiteral("terminalLabels")).isNull());
		}

		//One copy of a symbol takes a name of its own; the other copy and
		//the definition keep what they had
		void oneCopyKeepsItsOwnName()
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			define(diagram, QStringLiteral("twocopies"));
			Element *first = place(diagram, QStringLiteral("twocopies"));
			Element *second = place(diagram, QStringLiteral("twocopies"));
			QVERIFY(first && second);

			Terminal *terminal = named(first, QStringLiteral("X1:1"));
			QVERIFY(terminal);
			TerminalLabel label = terminal->label();
			label.m_name = QStringLiteral("X1:99");
			label.m_show_name = true;
			first->setTerminalLabel(terminal->stableUuid(), label);

			QCOMPARE(terminal->name(), QStringLiteral("X1:99"));
			QVERIFY(terminal->labelIsOverridden());
			QVERIFY(terminal->label().m_show_name);
				//What the definition says is still there to go back to
			QCOMPARE(terminal->baseName(), QStringLiteral("X1:1"));
			QCOMPARE(terminal->definitionLabel().m_name, QStringLiteral("X1:1"));
			QVERIFY(!terminal->definitionLabel().m_show_name);

			Terminal *untouched = named(second, QStringLiteral("X1:1"));
			QVERIFY(untouched);
			QCOMPARE(untouched->name(), QStringLiteral("X1:1"));
			QVERIFY(!untouched->labelIsOverridden());
			QVERIFY(!untouched->label().m_show_name);
		}

		//What one copy was given is saved with it and read back, and the
		//definition behind it is not written over
		void savedAndReadBack()
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			define(diagram, QStringLiteral("roundtrip"));
			Element *element = place(diagram, QStringLiteral("roundtrip"));
			QVERIFY(element);

			Terminal *terminal = named(element, QStringLiteral("X1:1"));
			QVERIFY(terminal);
			TerminalLabel label = terminal->label();
			label.m_name = QStringLiteral("X1:42");
			label.m_show_name = true;
			label.m_label_pos = QPointF(2.0, -6.0);
			label.m_label_rotation = 90.0;
			label.m_label_frame = true;
			element->setTerminalLabel(terminal->stableUuid(), label);

			QDomDocument document;
			QHash<Terminal *, int> table;
			const QDomElement saved = element->toXml(document, table);
			const QDomElement labels = saved.firstChildElement(QStringLiteral("terminalLabels"));
			QVERIFY(!labels.isNull());
			QCOMPARE(int(labels.childNodes().count()), 1);

				//Read back in a project of its own, the way a project is
				//opened: the copy takes what the saved element was given
			QETProject other;
			Diagram *folio = other.addNewDiagram();
			define(folio, QStringLiteral("roundtrip"));
			Element *copy = place(folio, QStringLiteral("roundtrip"));
			QVERIFY(copy);
			QDomElement readable = saved.cloneNode(true).toElement();
			QHash<int, Terminal *> addresses;
			QVERIFY(copy->fromXml(readable, addresses));

			Terminal *copied = named(copy, QStringLiteral("X1:42"));
			QVERIFY(copied);
			QVERIFY(copied->labelIsOverridden());
			QVERIFY(copied->label().m_show_name);
			QCOMPARE(copied->label().m_label_pos, QPointF(2.0, -6.0));
			QCOMPARE(copied->label().m_label_rotation, 90.0);
			QVERIFY(copied->label().m_label_frame);
			QCOMPARE(copied->label().m_name, QStringLiteral("X1:42"));
				//The definition of the symbol still says X1:1
			QCOMPARE(copied->baseName(), QStringLiteral("X1:1"));
			QCOMPARE(copied->definitionLabel().m_name, QStringLiteral("X1:1"));
		}

		//Taking the values back shows the definition's again, and the
		//saved element holds nothing of the change any more
		void resetTakesTheDefinitionBack()
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			define(diagram, QStringLiteral("resetme"));
			Element *element = place(diagram, QStringLiteral("resetme"));
			QVERIFY(element);

			Terminal *terminal = named(element, QStringLiteral("X1:1"));
			QVERIFY(terminal);
			TerminalLabel label = terminal->label();
			label.m_name = QStringLiteral("X1:77");
			label.m_show_name = true;
			element->setTerminalLabel(terminal->stableUuid(), label);
			QVERIFY(terminal->labelIsOverridden());

			element->removeTerminalLabel(terminal->stableUuid());
			QVERIFY(!terminal->labelIsOverridden());
			QCOMPARE(terminal->name(), QStringLiteral("X1:1"));
			QVERIFY(!terminal->label().m_show_name);
			QVERIFY(element->terminalLabels().isEmpty());

			QDomDocument document;
			QHash<Terminal *, int> table;
			const QDomElement saved = element->toXml(document, table);
			QVERIFY(saved.firstChildElement(QStringLiteral("terminalLabels")).isNull());
		}

		//The "Terminals" tab lists every terminal, changes this copy only
		//and puts the change back with one undo step
		void theTabEditsThisCopyOnly()
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			define(diagram, QStringLiteral("tabbed"));
			Element *first = place(diagram, QStringLiteral("tabbed"));
			Element *second = place(diagram, QStringLiteral("tabbed"));
			QVERIFY(first && second);

			ElementTerminalsWidget tab(first);
			auto *table = tab.findChild<QTableWidget *>();
			QVERIFY(table);
			QCOMPARE(table->rowCount(), int(first->terminals().size()));
			QCOMPARE(table->columnCount(), 4);

			QLineEdit *edit = nameEdit(&tab, 0);
			QVERIFY(edit);
			edit->setText(QStringLiteral("X1:99"));
			QUndoCommand *undo = tab.associatedUndo();
			QVERIFY(undo);
			diagram->undoStack().push(undo);

			QVERIFY(named(first, QStringLiteral("X1:99")));
			QVERIFY(named(second, QStringLiteral("X1:1")));
			QVERIFY(!tab.associatedUndo());

			QCheckBox *show = qobject_cast<QCheckBox *>(table->cellWidget(0, 0));
			QVERIFY(show);
			show->setChecked(true);
			undo = tab.associatedUndo();
			QVERIFY(undo);
			diagram->undoStack().push(undo);
			QVERIFY(named(first, QStringLiteral("X1:99"))->label().m_show_name);

			QPushButton *reset = qobject_cast<QPushButton *>(table->cellWidget(0, 3));
			QVERIFY(reset);
			reset->click();
			undo = tab.associatedUndo();
			QVERIFY(undo);
			diagram->undoStack().push(undo);
				//The terminal reads its definition's name again
			Terminal *back = named(first, QStringLiteral("X1:1"));
			QVERIFY(back);
			QVERIFY(!back->labelIsOverridden());
			QVERIFY(!back->label().m_show_name);

				//The other copy was never touched
			Terminal *untouched = named(second, QStringLiteral("X1:1"));
			QVERIFY(untouched);
			QVERIFY(!untouched->labelIsOverridden());
		}

		//In the properties panel, where there is no Apply, a name is
		//applied as it is typed and the typing is one undo step
		void thePanelAppliesAsItIsTyped()
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			define(diagram, QStringLiteral("lived"));
			Element *element = place(diagram, QStringLiteral("lived"));
			QVERIFY(element);

			ElementTerminalsWidget tab(element);
			tab.setLiveEdit(true);
			QLineEdit *edit = nameEdit(&tab, 0);
			QVERIFY(edit);
				//Adding the folio itself was a step already
			const int before = diagram->undoStack().count();

			edit->setText(QStringLiteral("X1:7"));
			QVERIFY(named(element, QStringLiteral("X1:7")));
			QCOMPARE(diagram->undoStack().count(), before + 1);

			edit->setText(QStringLiteral("X1:77"));
			QCOMPARE(named(element, QStringLiteral("X1:77"))->name(),
					 QStringLiteral("X1:77"));
				//The typing stays one step: the second keystroke merges
				//into the first
			QCOMPARE(diagram->undoStack().count(), before + 1);

			diagram->undoStack().undo();
			QVERIFY(named(element, QStringLiteral("X1:1")));
			QVERIFY(!named(element, QStringLiteral("X1:1"))->labelIsOverridden());
		}
};

QET_APP_TEST_MAIN(tst_terminallabels)
#include "tst_terminallabels.moc"
