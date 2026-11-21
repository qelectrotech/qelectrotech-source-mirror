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
#include "diagram.h"
#include "elementspanel.h"
#include "elementspanelwidget.h"
#include "projectview.h"
#include "qetapptest.h"
#include "qetdiagrameditor.h"
#include "qetproject.h"
#include "titleblockproperties.h"
#include "utils/qetsettings.h"

#include <QPointer>
#include <QTemporaryDir>
#include <QtTest>

/**
	The project panel lists a project's folios as a tree by folio group
	and sub-group (discussion #1878, C9), when the setting is on (off by
	default: flat, as before). A level only shows where it
	splits folios apart, so a project without groups, or with every folio
	in the same place, is listed flat as before. A folio given another
	group moves to its row at once; reordering, clicking and closing work
	on folios under group rows.
*/
class tst_foliotree : public QObject
{
	Q_OBJECT

	static void setFolio(Diagram *diagram, const QString &title,
						 const QString &group, const QString &subgroup = {},
						 const QString &plant = {}, const QString &location = {})
	{
		TitleBlockProperties tb = diagram->border_and_titleblock.exportTitleBlock();
		tb.title = title;
		tb.folio_group = group;
		tb.folio_subgroup = subgroup;
		tb.plant = plant;
		tb.locmach = location;
		diagram->border_and_titleblock.importTitleBlock(tb);
	}

		//The paths of @p folios, as "g=A/s+B" per folio
	static QStringList paths(const QList<Diagram *> &folios)
	{
		QStringList out;
		for (const QStringList &path : GenericPanel::folioGroupPaths(folios))
			out << path.join(QLatin1Char('/'));
		return out;
	}

		//The rows under @p item, one line each, indented by depth; folio
		//rows show their title only
	static QStringList rows(const QTreeWidgetItem *item, int depth = 0)
	{
		QStringList out;
		for (int i = 0; i < item->childCount(); ++i) {
			const QTreeWidgetItem *child = item->child(i);
			if (child->type() != QET::Diagram && child->type() != QET::FolioGroup)
				continue;
			QString text = child->text(0);
			if (child->type() == QET::Diagram)
				text = text.section(QStringLiteral(" - "), 1);
			out << QString(depth * 2, QLatin1Char(' ')) + text;
			out << rows(child, depth + 1);
		}
		return out;
	}

	ElementsPanel *m_panel = nullptr;
	ElementsPanelWidget *m_panel_widget = nullptr;

		//A new project in the editor, with folios titled @p titles
	QETProject *newProject(const QStringList &titles)
	{
		QETDiagramEditor *editor = QetAppTest::editor();
		if (!editor || !editor->newProject()) return nullptr;
		QETProject *project = editor->currentProjectView()->project();
			//newProject() makes one folio
		while (project->diagrams().size() < titles.size())
			project->addNewDiagram();
		for (int i = 0; i < titles.size(); ++i)
			setFolio(project->diagrams().at(i), titles.at(i), QString());
		return project;
	}

private slots:
	void initTestCase()
	{
		QETDiagramEditor *editor = QetAppTest::editor();
		QVERIFY(editor);
		m_panel_widget = editor->findChild<ElementsPanelWidget *>();
		QVERIFY(m_panel_widget);
		m_panel = &m_panel_widget->elementsPanel();
		m_panel->reload();
		QetSettings::setFoliosListedByGroup(true);
	}

	void cleanupTestCase()
	{
		QetSettings::setFoliosListedByGroup(false);
	}

	void offByDefaultIsFlat()
	{
		QetSettings::setFoliosListedByGroup(false);
		QPointer<QETProject> project = newProject({"One", "Two"});
		QVERIFY(project);
		setFolio(project->diagrams().at(0), "One", "A");
		setFolio(project->diagrams().at(1), "Two", "B");
		QTreeWidgetItem *project_item = m_panel->itemForProject(project);
		QCoreApplication::processEvents();
		QCOMPARE(rows(project_item), QStringList({"One", "Two"}));

			//turned on: the panel is reloaded after the settings dialog
		QetSettings::setFoliosListedByGroup(true);
		m_panel->reload();
		QCOMPARE(rows(project_item), QStringList({"A", "  One", "B", "  Two"}));
		project->setModified(false);
		m_panel->projectWasClosed(project);
	}

	void anotherFolioMoves()
	{
			//a change to one folio can move another one: here the
			//sub-group level of A goes, and Two leaves its "+1" row
		QPointer<QETProject> project = newProject({"One", "Two", "Three"});
		QVERIFY(project);
		const QList<Diagram *> f = project->diagrams();
		setFolio(f.at(0), "One", "A");
		setFolio(f.at(1), "Two", "A", "+1");
		setFolio(f.at(2), "Three", "B");
		QTreeWidgetItem *project_item = m_panel->itemForProject(project);
		QTRY_COMPARE(rows(project_item), QStringList({
			"A", "  One", "  +1", "    Two", "B", "  Three"}));
		setFolio(f.at(0), "One", "A", "+1");
		QTRY_COMPARE(rows(project_item), QStringList({
			"A", "  One", "  Two", "B", "  Three"}));

			//and a whole level: one group for all, the group rows go; the
			//sub-groups ("+1", "+1", none) still split
		setFolio(f.at(2), "Three", "A");
		QTRY_COMPARE(rows(project_item), QStringList({
			"+1", "  One", "  Two", "Three"}));
		project->setModified(false);
		m_panel->projectWasClosed(project);
	}

	void selectionIsKept()
	{
			//several folios selected; a group change moves rows, the
			//selection stays on the same folios
		QPointer<QETProject> project = newProject({"One", "Two", "Three"});
		QVERIFY(project);
		const QList<Diagram *> f = project->diagrams();
		setFolio(f.at(0), "One", "A");
		setFolio(f.at(1), "Two", "B");
		setFolio(f.at(2), "Three", "A");
		QTreeWidgetItem *project_item = m_panel->itemForProject(project);
		QTRY_COMPARE(rows(project_item), QStringList({
			"A", "  One", "  Three", "B", "  Two"}));
		m_panel->clearSelection();
		m_panel->setCurrentItem(m_panel->getItemForDiagram(f.at(2)));
		m_panel->getItemForDiagram(f.at(0))->setSelected(true);
		m_panel->getItemForDiagram(f.at(2))->setSelected(true);
		setFolio(f.at(1), "Two", "A");
		setFolio(f.at(2), "Three", "B");
		QTRY_COMPARE(rows(project_item), QStringList({
			"A", "  One", "  Two", "B", "  Three"}));
		QCOMPARE(m_panel->selectedDiagrams().size(), 2);
		QVERIFY(m_panel->selectedDiagrams().contains(f.at(0)));
		QVERIFY(m_panel->selectedDiagrams().contains(f.at(2)));
		QCOMPARE(m_panel->currentItem(), m_panel->getItemForDiagram(f.at(2)));
		project->setModified(false);
		m_panel->projectWasClosed(project);
	}

	void aLevelShowsOnlyWhereItSplits()
	{
		QETProject project;
		Diagram *a = project.addNewDiagram();
		Diagram *b = project.addNewDiagram();
		Diagram *c = project.addNewDiagram();
		const QList<Diagram *> folios{a, b, c};

			//no group anywhere: flat
		QCOMPARE(paths(folios), QStringList({"", "", ""}));

			//every folio at the same location (+IGC, as examples/
			//industrial.qet): still flat
		for (Diagram *d : folios)
			setFolio(d, QStringLiteral("x"), {}, {}, {}, QStringLiteral("IGC"));
		QCOMPARE(paths(folios), QStringList({"", "", ""}));

			//two plants: a group level; the location is the same within
			//each, so no sub-group level
		setFolio(c, QStringLiteral("x"), {}, {}, QStringLiteral("SILO"),
				 QStringLiteral("IGC"));
		QCOMPARE(paths(folios), QStringList({"", "", "gSILO"}));

			//a named group wins over the plant, and two sub-groups inside
			//"Power" split it; "Lists" has one sub-group: no level there
		setFolio(a, QStringLiteral("x"), QStringLiteral("Power"), QStringLiteral("+A"));
		setFolio(b, QStringLiteral("x"), QStringLiteral("Power"), QStringLiteral("+B"));
		setFolio(c, QStringLiteral("x"), QStringLiteral("Lists"), QStringLiteral("+A"));
		QCOMPARE(paths(folios),
				 QStringList({"gPower/s+A", "gPower/s+B", "gLists"}));

			//one group for all, sub-groups split: sub-group rows only;
			//a folio with no sub-group sits at the top
		setFolio(c, QStringLiteral("x"), QStringLiteral("Power"), {});
		QCOMPARE(paths(folios), QStringList({"s+A", "s+B", ""}));
	}

	void panelShowsTheTree()
	{
		QPointer<QETProject> project = newProject(
			{"Cover", "Supply", "Motor", "PLC", "Parts list"});
		QVERIFY(project);
		const QList<Diagram *> f = project->diagrams();
		setFolio(f.at(1), "Supply", "Power", "+CP1");
		setFolio(f.at(2), "Motor", "Power", "+CP2");
		setFolio(f.at(3), "PLC", "Control", "+CP1");
		setFolio(f.at(4), "Parts list", "Lists");

		QTreeWidgetItem *project_item = m_panel->itemForProject(project);
		QVERIFY(project_item);
		QTRY_COMPARE(rows(project_item), QStringList({
			"Cover",
			"Power",
			"  +CP1",
			"    Supply",
			"  +CP2",
			"    Motor",
			"Control",
			"  PLC",
			"Lists",
			"  Parts list"}));

			//a click on a folio under a group row opens that folio
		QTreeWidgetItem *motor = m_panel->getItemForDiagram(f.at(2));
		QVERIFY(motor);
		QCOMPARE(motor->parent()->parent()->text(0), QStringLiteral("Power"));
		QCOMPARE(m_panel->projectForItem(project_item), project.data());

			//a new group moves the folio at once; a folded row stays folded
			//(the current folio's row is kept in view: work on "Cover")
		m_panel->setCurrentItem(m_panel->getItemForDiagram(f.at(0)));
		QTreeWidgetItem *control = m_panel->getItemForDiagram(f.at(3))->parent();
		control->setExpanded(false);
		setFolio(f.at(4), "Parts list", "Control");
		QTRY_COMPARE(rows(project_item), QStringList({
			"Cover",
			"Power",
			"  +CP1",
			"    Supply",
			"  +CP2",
			"    Motor",
			"Control",
			"  +CP1",
			"    PLC",
			"  Parts list"}));
		control = m_panel->getItemForDiagram(f.at(3))->parent()->parent();
		QCOMPARE(control->text(0), QStringLiteral("Control"));
		QVERIFY(!control->isExpanded());

			//no group left: flat again, no group row anywhere
		for (Diagram *d : f)
			setFolio(d, d->title(), QString());
		QTRY_COMPARE(rows(project_item), QStringList({
			"Cover", "Supply", "Motor", "PLC", "Parts list"}));

		project->setModified(false);
		m_panel->projectWasClosed(project);
	}

	void groupsOpenWhenTheProjectOpens()
	{
			//a project saved with groups: every group row starts open
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const QString file = dir.filePath(QStringLiteral("grouped.qet"));
		{
			QETProject project;
			Diagram *a = project.addNewDiagram();
			Diagram *b = project.addNewDiagram();
			Diagram *c = project.addNewDiagram();
			setFolio(a, "One", "A", "+1");
			setFolio(b, "Two", "A", "+2");
			setFolio(c, "Three", "B");
			project.setFilePath(file);
			QVERIFY(project.write().isOk());
		}
			//read from the file, then shown in the panel, as opening a
			//project does (without its backup-copy question)
		QETProject *project = new QETProject(file);
		m_panel->projectWasOpened(project);
		QTreeWidgetItem *project_item = m_panel->itemForProject(project);
		QVERIFY(project_item);
		QCOMPARE(rows(project_item), QStringList({
			"A", "  +1", "    One", "  +2", "    Two", "B", "  Three"}));
		const auto groups = project_item->treeWidget()
			->findItems(QStringLiteral("*"), Qt::MatchWildcard | Qt::MatchRecursive);
		int group_rows = 0;
		for (QTreeWidgetItem *item : groups) {
			if (item->type() != QET::FolioGroup) continue;
			QTreeWidgetItem *top = item;
			while (top->parent()) top = top->parent();
			if (top != project_item) continue; //another test's project
			++group_rows;
			QVERIFY2(item->isExpanded(), qPrintable(item->text(0)));
		}
		QCOMPARE(group_rows, 4);
		m_panel->projectWasClosed(project);
		delete project;
	}

	void reorderingAFlatList()
	{
			//no group: the list stays flat and follows the new order
		QPointer<QETProject> project = newProject({"One", "Two", "Three"});
		QVERIFY(project);
		const QList<Diagram *> f = project->diagrams();
		QTreeWidgetItem *project_item = m_panel->itemForProject(project);
		QCOMPARE(rows(project_item), QStringList({"One", "Two", "Three"}));
		project->diagramOrderChanged(2, 0);
		QCOMPARE(rows(project_item), QStringList({"Three", "One", "Two"}));
		project->diagramOrderChanged(0, 1);
		QCOMPARE(rows(project_item), QStringList({"One", "Three", "Two"}));
		QCOMPARE(m_panel->currentItem(), m_panel->getItemForDiagram(f.at(2)));
		project->setModified(false);
		m_panel->projectWasClosed(project);
	}

	void reorderingKeepsTheGroups()
	{
		QPointer<QETProject> project = newProject({"One", "Two", "Three"});
		QVERIFY(project);
		const QList<Diagram *> f = project->diagrams();
		setFolio(f.at(0), "One", "A");
		setFolio(f.at(1), "Two", "B");
		setFolio(f.at(2), "Three", "A");
		QTreeWidgetItem *project_item = m_panel->itemForProject(project);
		QTRY_COMPARE(rows(project_item), QStringList({
			"A", "  One", "  Three", "B", "  Two"}));

			//"Three" to the top: group A still first, Three before One
		project->diagramOrderChanged(2, 0);
		QCOMPARE(rows(project_item), QStringList({
			"A", "  Three", "  One", "B", "  Two"}));

			//"Two" to the top: group B now comes first
		project->diagramOrderChanged(2, 0);
		QCOMPARE(rows(project_item), QStringList({
			"B", "  Two", "A", "  Three", "  One"}));
		QCOMPARE(m_panel->currentItem(), m_panel->getItemForDiagram(f.at(1)));

			//each folio row still shows its folio number in the new order
		QVERIFY2(m_panel->getItemForDiagram(f.at(1))->text(0).startsWith(QStringLiteral("1/")),
				 qPrintable(m_panel->getItemForDiagram(f.at(1))->text(0)));

			//closing the project removes every row, folios under groups too
		project->setModified(false);
		m_panel->projectWasClosed(project);
		QVERIFY(!m_panel->itemForProject(project));
	}
};

QET_APP_TEST_MAIN(tst_foliotree)

#include "tst_foliotree.moc"
