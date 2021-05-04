/*
	Copyright 2006-2021 The QElectroTech Team
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
#include "terminalstripeditor.h"
#include "ui_terminalstripeditor.h"
#include "terminalstripcreatordialog.h"
#include "../../qetproject.h"
#include "../terminalstrip.h"
#include "../elementprovider.h"
#include "../qetgraphicsitem/terminalelement.h"
#include "../UndoCommand/addterminalstripcommand.h"
#include "../UndoCommand/addterminaltostripcommand.h"
#include "terminalstriptreewidget.h"
#include "../../qeticons.h"

#include <QTreeWidgetItem>

/**
 * @brief TerminalStripEditor::TerminalStripEditor
 * @param project : Project to manage the terminal strip
 * @param parent : paent widget
 */
TerminalStripEditor::TerminalStripEditor(QETProject *project, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::TerminalStripEditor),
	m_project(project)
{
	ui->setupUi(this);
	buildTree();
	ui->m_terminal_strip_tw->expandRecursively(ui->m_terminal_strip_tw->rootIndex());
	setUpUndoConnections();
}

/**
 * @brief TerminalStripEditor::~TerminalStripEditor
 */
TerminalStripEditor::~TerminalStripEditor() {
	delete ui;
}

void TerminalStripEditor::setUpUndoConnections()
{
	connect(ui->m_terminal_strip_tw, &TerminalStripTreeWidget::terminalAddedToStrip,
			[this](QUuid terminal_uuid, QUuid strip_uuid)
	{
		auto terminal = m_uuid_terminal_H.value(terminal_uuid);
		auto strip = m_uuid_strip_H.value(strip_uuid);

		if (!terminal || !strip) {
			return;
		}

		auto undo = new AddTerminalToStripCommand(terminal, strip);
		m_project->undoStack()->push(undo);
	});

	connect(ui->m_terminal_strip_tw, &TerminalStripTreeWidget::terminalMovedFromStripToStrip,
			[this] (QUuid terminal_uuid, QUuid old_strip_uuid, QUuid new_strip_uuid)
	{
		auto terminal  = m_uuid_terminal_H.value(terminal_uuid);
		auto old_strip = m_uuid_strip_H.value(old_strip_uuid);
		auto new_strip = m_uuid_strip_H.value(new_strip_uuid);

		if (!terminal || !old_strip || !new_strip) {
			return;
		}

		auto undo = new AddTerminalToStripCommand(terminal, old_strip, new_strip);
		m_project->undoStack()->push(undo);
	});

	connect(ui->m_terminal_strip_tw, &TerminalStripTreeWidget::terminalRemovedFromStrip,
			[this] (QUuid terminal_uuid, QUuid old_strip_uuid)
	{
		auto terminal_ = m_uuid_terminal_H.value(terminal_uuid);
		auto strip_ = m_uuid_strip_H.value(old_strip_uuid);

		if (!terminal_ || !strip_) {
			return;
		}

		auto undo = new RemoveTerminalFromStripCommand(terminal_, strip_);
		m_project->undoStack()->push(undo);
	});
}

/**
 * @brief TerminalStripEditor::buildTree
 * Build the tree widget use to explore terminal strip
 */
void TerminalStripEditor::buildTree()
{
	ui->m_terminal_strip_tw->clear();

	auto title = m_project->title();
	if (title.isEmpty()) {
		title = tr("Projet sans titre");
	}

	QStringList strl{title};
	new QTreeWidgetItem(ui->m_terminal_strip_tw, strl, TerminalStripTreeWidget::Root);

	QStringList ftstrl(tr("Bornes indépendante"));
	new QTreeWidgetItem(ui->m_terminal_strip_tw, ftstrl, TerminalStripTreeWidget::FreeTerminal);

	auto ts_vector = m_project->terminalStrip();
	std::sort(ts_vector.begin(), ts_vector.end(), [](TerminalStrip *a, TerminalStrip *b) {
		return a->name() < b->name();});

	for (const auto ts : qAsConst(ts_vector)) {
		addTerminalStrip(ts);
	}
	addFreeTerminal();
}

/**
 * @brief TerminalStripEditor::addTerminalStrip
 * Add a new terminal strip to the list of displayed terminal strip
 * in the tree widget
 * @param terminal_strip
 * @return the QTreeWidgetItem who represent the terminal strip
 * both if created or already exist
 */
QTreeWidgetItem* TerminalStripEditor::addTerminalStrip(TerminalStrip *terminal_strip)
{
	if (auto item = m_item_strip_H.key(terminal_strip)) {
		return item;
	}

	auto root_item = ui->m_terminal_strip_tw->topLevelItem(0);

		//Check if installation already exist
		//if not create a new one
	auto installation_str = terminal_strip->installation();
	QTreeWidgetItem *inst_qtwi = nullptr;
	for (int i = 0 ; i<root_item->childCount() ; ++i) {
		auto child_inst = root_item->child(i);
		if (child_inst->data(0, Qt::DisplayRole).toString() == installation_str) {
			inst_qtwi = child_inst;
			break;
		}
	}
	if (!inst_qtwi) {
		QStringList inst_strl{installation_str};
		inst_qtwi = new QTreeWidgetItem(root_item, inst_strl, TerminalStripTreeWidget::Installation);
	}

		//Check if location already exist
		//if not create a new one
	auto location_str = terminal_strip->location();
	QTreeWidgetItem *loc_qtwi = nullptr;
	for (int i = 0 ; i<inst_qtwi->childCount() ; ++i) {
		auto child_loc = inst_qtwi->child(i);
		if (child_loc->data(0, Qt::DisplayRole).toString() == location_str) {
			loc_qtwi = child_loc;
			break;
		}
	}
	if (!loc_qtwi) {
		QStringList loc_strl{location_str};
		loc_qtwi = new QTreeWidgetItem(inst_qtwi, loc_strl, TerminalStripTreeWidget::Location);
	}

		//Add the terminal strip
	QStringList name{terminal_strip->name()};
	auto strip_item = new QTreeWidgetItem(loc_qtwi, name, TerminalStripTreeWidget::Strip);
	strip_item->setData(0, TerminalStripTreeWidget::UUID_USER_ROLE, terminal_strip->uuid());
	strip_item->setIcon(0, QET::Icons::TerminalStrip);

		//Add child terminal of the strip
	for (auto i=0 ; i<terminal_strip->physicalTerminalCount() ; ++i)
	{
		auto index = terminal_strip->index(i);
		auto term_item = new QTreeWidgetItem(strip_item, QStringList(index.label()), TerminalStripTreeWidget::Terminal);
		term_item->setData(0, TerminalStripTreeWidget::UUID_USER_ROLE, index.uuid().toString());
		term_item->setIcon(0, QET::Icons::ElementTerminal);

		if (index.isElement()) {
			m_uuid_terminal_H.insert(index.uuid(), index.element());
		}
	}

	m_item_strip_H.insert(strip_item, terminal_strip);
	m_uuid_strip_H.insert(terminal_strip->uuid(), terminal_strip);
	return strip_item;
}

/**
 * @brief TerminalStripEditor::addFreeTerminal
 * Add free terminal (aka terminal which not belong to a terminal strip)
 * in the tree widget
 */
void TerminalStripEditor::addFreeTerminal()
{
	ElementProvider ep(m_project);
	auto vector_ = ep.freeTerminal();

	if (vector_.isEmpty()) {
		return;
	}

		//Sort the terminal element by label
	std::sort(vector_.begin(), vector_.end(), [](TerminalElement *a, TerminalElement *b) {
		return a->actualLabel() < b->actualLabel();
	});

	auto free_terminal_item = ui->m_terminal_strip_tw->topLevelItem(1);

	for (const auto terminal : qAsConst(vector_))
	{
		QUuid uuid_ = terminal->uuid();
		QStringList strl{terminal->actualLabel()};
		auto item = new QTreeWidgetItem(free_terminal_item, strl, TerminalStripTreeWidget::Terminal);
		item->setData(0, TerminalStripTreeWidget::UUID_USER_ROLE, uuid_.toString());
		item->setIcon(0, QET::Icons::ElementTerminal);

		m_uuid_terminal_H.insert(uuid_, terminal);
	}
}

/**
 * @brief TerminalStripEditor::on_m_add_terminal_strip_pb_clicked
 * Action when user click on add terminal strip button
 */
void TerminalStripEditor::on_m_add_terminal_strip_pb_clicked()
{
	QScopedPointer<TerminalStripCreatorDialog> dialog(new TerminalStripCreatorDialog(m_project, this));

	if (auto item = ui->m_terminal_strip_tw->currentItem())
	{
		if (item->type() == TerminalStripTreeWidget::Strip) {
			item = item->parent();
		}
		if (item->type() == TerminalStripTreeWidget::Location) {
			dialog->setLocation(item->data(0, Qt::DisplayRole).toString());
			item = item->parent();
		}
		if (item->type() == TerminalStripTreeWidget::Installation) {
			dialog->setInstallation(item->data(0, Qt::DisplayRole).toString());
		}
	}

	if (dialog->exec() == QDialog::Accepted)
	{
		auto ts = dialog->generatedTerminalStrip();
		m_project->undoStack()->push(new AddTerminalStripCommand(ts, m_project));

		auto item = addTerminalStrip(ts);
		ui->m_terminal_strip_tw->setCurrentItem(item);
	}
}

/**
 * @brief TerminalStripEditor::on_m_remove_terminal_strip_pb_clicked
 * Action when user click on remove terminal strip button
 */
void TerminalStripEditor::on_m_remove_terminal_strip_pb_clicked()
{
	auto item = ui->m_terminal_strip_tw->currentItem();
	if (auto strip = m_item_strip_H.value(item))
	{
		m_item_strip_H.remove(item);
		m_uuid_strip_H.remove(strip->uuid());
		delete item;

		m_project->undoStack()->push(new RemoveTerminalStripCommand(strip, m_project));
	}

	on_m_reload_pb_clicked();
}

void TerminalStripEditor::on_m_reload_pb_clicked()
{
   ui->m_terminal_strip_tw->clear();
   m_item_strip_H.clear();
   m_uuid_terminal_H.clear();
   m_uuid_strip_H.clear();

   qDeleteAll(m_item_strip_H.keys());

   buildTree();
	ui->m_terminal_strip_tw->expandRecursively(ui->m_terminal_strip_tw->rootIndex());
}
