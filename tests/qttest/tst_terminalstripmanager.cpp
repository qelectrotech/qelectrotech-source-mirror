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
	The terminal block manager: the independent terminals open their
	table from their own entry, one without a label says what it is, the
	tree keeps what was folded over a reload and folds or unfolds at once,
	the table of the independent terminals sorts by a column, and OK
	applies and closes.
*/
#include "qetapptest.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "factory/elementfactory.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminalelement.h"
#include "qetproject.h"
#include "TerminalStrip/terminalstrip.h"
#include "TerminalStrip/ui/freeterminaleditor.h"
#include "TerminalStrip/ui/terminalstripeditorwindow.h"
#include "TerminalStrip/ui/terminalstriptreedockwidget.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QStackedWidget>
#include <QTableView>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>

class tst_terminalstripmanager : public QObject
{
	Q_OBJECT

	private:
		Diagram *m_diagram = nullptr;
		TerminalStripEditorWindow *m_window = nullptr;
		QTreeWidget *m_tree = nullptr;

		Element *place(const QString &label, const QPointF &pos)
		{
			int state = 0;
			Element *element = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/clamp.elmt"), m_diagram->project()),
				nullptr, &state);
			if (state || !element)
				return nullptr;
			m_diagram->addItem(element);
			element->setPos(pos);
			if (!label.isEmpty()) {
				DiagramContext info = element->elementInformations();
				info.addValue(QStringLiteral("label"), label);
				element->setElementInformations(info);
			}
			return element;
		}

		QTreeWidgetItem *find(const QString &text) const
		{
			const auto found = m_tree->findItems(text, Qt::MatchStartsWith | Qt::MatchRecursive);
			return found.isEmpty() ? nullptr : found.first();
		}

	private slots:
		void initTestCase()
		{
			m_diagram = QetAppTest::diagram();
			QVERIFY(m_diagram);
			QDomDocument definition;
			definition.setContent(QStringLiteral(
				R"(<definition type="element" link_type="terminal" width="10" height="20" hotspot_x="0" hotspot_y="0">
<names><name lang="en">clamp</name></names><kindInformations><kindInformation name="type">terminal</kindInformation></kindInformations>
<description><rect x="0" y="0" width="10" height="20" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="5" y="0" orientation="n"/><terminal x="5" y="20" orientation="s"/></description></definition>)"));
			QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("clamp"), definition.documentElement()));
				//A T-junction: of the terminal kind, but no terminal block
			QDomDocument tee;
			tee.setContent(QStringLiteral(
				R"(<definition type="element" link_type="terminal" width="10" height="20" hotspot_x="0" hotspot_y="10">
<names><name lang="en">tee</name></names><description>
<line x1="0" y1="-10" x2="0" y2="10" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal x="0" y="-10" orientation="n"/><terminal x="0" y="10" orientation="s"/><terminal x="10" y="0" orientation="e"/></description></definition>)"));
			QVERIFY(m_diagram->project()->embeddedElementCollection()->addElementDefinition(
				QStringLiteral("import"), QStringLiteral("tee"), tee.documentElement()));
			int state = 0;
			Element *junction = ElementFactory::Instance()->createElement(
				ElementsLocation(QStringLiteral("embed://import/tee.elmt"), m_diagram->project()), nullptr, &state);
			QVERIFY(!state && junction);
			m_diagram->addItem(junction);
			junction->setPos(300, 300);
			QVERIFY(place(QStringLiteral("X1:10"), QPointF(100, 100)));
			QVERIFY(place(QStringLiteral("X1:2"), QPointF(120, 100)));
			QVERIFY(place(QString(), QPointF(140, 100)));
			Element *strip_terminal = place(QStringLiteral("X2:1"), QPointF(200, 100));
			QVERIFY(strip_terminal);
			auto *strip = new TerminalStrip(QStringLiteral("="), QStringLiteral("UV1"),
											QStringLiteral("X2"), m_diagram->project());
			m_diagram->project()->addTerminalStrip(strip);
			QVERIFY(strip->addTerminal(static_cast<TerminalElement *>(strip_terminal)));

			m_window = TerminalStripEditorWindow::instance(m_diagram->project(), nullptr);
			QVERIFY(m_window);
			m_window->show();
			m_tree = m_window->findChild<QTreeWidget *>();
			QVERIFY(m_tree);
		}

		void unlabelledSaysWhatItIs()
		{
			QVERIFY(find(QStringLiteral("(no label) clamp, folio 1")));
		}

			//Corners and T-junctions join wires only: not listed
		void noJunctions()
		{
			QVERIFY(!find(QStringLiteral("(no label) tee")));
		}

		void foldButtonsAboveTree()
		{
			auto *dock = m_window->findChild<TerminalStripTreeDockWidget *>();
			int buttons = 0;
			for (auto *b : dock->findChildren<QToolButton *>())
				if (b->accessibleName() == QStringLiteral("Expand all")
					|| b->accessibleName() == QStringLiteral("Collapse all"))
					++buttons;
			QCOMPARE(buttons, 2);
		}

		void independentTerminalsOpenTheirTable()
		{
			QTreeWidgetItem *free_ = m_tree->topLevelItem(1);
			QVERIFY(free_);
			m_tree->setCurrentItem(free_);
			auto *stack = m_window->findChild<QStackedWidget *>();
			QVERIFY(stack);
			QVERIFY(qobject_cast<FreeTerminalEditor *>(stack->currentWidget()));
		}

		void freeTableSorts()
		{
			auto *editor = m_window->findChild<FreeTerminalEditor *>();
			auto *table = editor->findChild<QTableView *>();
			QVERIFY(table && table->isSortingEnabled());
			table->sortByColumn(0, Qt::AscendingOrder);
			QStringList labels;
			for (int row = 0; row < table->model()->rowCount(); ++row)
				labels << table->model()->index(row, 0).data().toString();
			const int two = labels.indexOf(QStringLiteral("X1:2"));
			const int ten = labels.indexOf(QStringLiteral("X1:10"));
			QVERIFY(two >= 0 && ten >= 0);
			QVERIFY2(two < ten, qPrintable(labels.join(QStringLiteral(", "))));
			table->sortByColumn(0, Qt::DescendingOrder);
			QVERIFY(table->model()->index(0, 0).data().toString() != QStringLiteral("X1:2"));
		}

		void foldingKeptOverReload()
		{
			QTreeWidgetItem *strip = find(QStringLiteral("X2"));
			QVERIFY(strip);
			strip->setExpanded(false);
			auto *dock = m_window->findChild<TerminalStripTreeDockWidget *>();
			dock->reload();
			strip = find(QStringLiteral("X2"));
			QVERIFY(strip && !strip->isExpanded());
			QVERIFY(strip->parent()->isExpanded());

			dock->expandAll();
			QVERIFY(find(QStringLiteral("X2"))->isExpanded());
			dock->collapseAll();
			QVERIFY(!find(QStringLiteral("X2"))->isExpanded());
			QVERIFY(m_tree->topLevelItem(0)->isExpanded());
		}

		void okClosesTheWindow()
		{
			auto *box = m_window->findChild<QDialogButtonBox *>();
			QVERIFY(box);
			m_tree->setCurrentItem(find(QStringLiteral("X2")));
			QVERIFY(box->isVisible());
			QAbstractButton *ok = box->button(QDialogButtonBox::Ok);
			QVERIFY(ok && box->button(QDialogButtonBox::Apply));
			ok->click();
			QVERIFY(!m_window->isVisible());
		}
};

QET_APP_TEST_MAIN(tst_terminalstripmanager)
#include "tst_terminalstripmanager.moc"
