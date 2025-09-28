/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "terminalstriptreedockwidget.h"
#include "ui_terminalstriptreedockwidget.h"

#include "../UndoCommand/addterminaltostripcommand.h"
#include "../../elementprovider.h"
#include "../physicalterminal.h"
#include "../../qeticons.h"
#include "../../qetproject.h"
#include "../realterminal.h"
#include "../../qetgraphicsitem/terminalelement.h"
#include "../terminalstrip.h"
#include "../../qetinformation.h"

TerminalStripTreeDockWidget::TerminalStripTreeDockWidget(QETProject *project, QWidget *parent) :
	QDockWidget(parent),
	ui(new Ui::TerminalStripTreeDockWidget),
	m_project(project)
{
	ui->setupUi(this);
	buildTree();

#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
	ui->m_tree_view->expandRecursively(ui->m_tree_view->rootIndex());
#else
	ui->m_tree_view->expandAll();
#endif
}

TerminalStripTreeDockWidget::~TerminalStripTreeDockWidget()
{
	delete ui;
}

/**
 * @brief TerminalStripTreeDockWidget::reload
 */
void TerminalStripTreeDockWidget::reload()
{
	auto current_ = m_current_strip;

	ui->m_tree_view->clear();
	m_item_strip_H.clear();
	m_uuid_terminal_H.clear();
	m_uuid_strip_H.clear();

	for (const auto &connection_ : std::as_const(m_strip_changed_connection)) {
		disconnect(connection_);
	}
	m_strip_changed_connection.clear();


	buildTree();

#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
	ui->m_tree_view->expandRecursively(ui->m_tree_view->rootIndex());
#else
	ui->m_tree_view->expandAll();
#endif

		//Reselect the tree widget item of the current edited strip
   auto item = m_item_strip_H.key(current_);
   if (item) {
	   ui->m_tree_view->setCurrentItem(item);
   }
}

/**
 * @brief TerminalStripTreeDockWidget::currentIsStrip
 * @return true if the current selected item is a terminal strip.
 */
bool TerminalStripTreeDockWidget::currentIsStrip() const {
	return m_item_strip_H.contains(ui->m_tree_view->currentItem());
}

/**
 * @brief TerminalStripTreeDockWidget::currentStrip
 * @return The current selected strip or nullptr if there is
 * no strip selected;
 */
TerminalStrip *TerminalStripTreeDockWidget::currentStrip() const {
	return m_current_strip;
}

/**
 * @brief TerminalStripTreeDockWidget::currentInstallation
 * @return the installation according to the current selection
 */
QString TerminalStripTreeDockWidget::currentInstallation() const
{
	if (m_current_strip) {
		return m_current_strip->installation();
	}

	if (auto item = ui->m_tree_view->currentItem())
	{
		if (item->type() == Location) {
			item = item->parent();
		}
		if (item->type() == Installation) {
			return item->data(0, Qt::DisplayRole).toString();
		}
	}

	return QString();
}

/**
 * @brief TerminalStripTreeDockWidget::currentLocation
 * @return the location according to the current selection
 */
QString TerminalStripTreeDockWidget::currentLocation() const
{
	if (m_current_strip) {
		return m_current_strip->location();
	}

	if (auto item = ui->m_tree_view->currentItem()) {
		if (item->type() == Location) {
			return item->data(0, Qt::DisplayRole).toString();
		}
	}

	return QString();
}

/**
 * @brief TerminalStripTreeDockWidget::setSelectedStrip
 * @param strip
 */
void TerminalStripTreeDockWidget::setSelectedStrip(TerminalStrip *strip) {
	ui->m_tree_view->setCurrentItem(m_item_strip_H.key(strip));
}

/**
 * @brief TerminalStripTreeDockWidget::currentRealTerminal
 * @return the current real terminal or a null QSharedPointer.
 */
QSharedPointer<RealTerminal> TerminalStripTreeDockWidget::currentRealTerminal() const
{
	if (auto item = ui->m_tree_view->currentItem()) {
		if (item->type() == Terminal) {
			return m_uuid_terminal_H.value(item->data(0,UUID_USER_ROLE).toUuid());
		}
	}
	return QSharedPointer<RealTerminal>();
}

/**
 * @brief TerminalStripTreeDockWidget::on_m_tree_view_currentItemChanged
 * @param current
 * @param previous
 */
void TerminalStripTreeDockWidget::on_m_tree_view_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	Q_UNUSED(previous)

	if (!current) {
		setCurrentStrip(nullptr);
		return;
	}

	TerminalStrip *strip_ = nullptr;
	bool current_is_free{false};
	const auto current_type{current->type()};
	if (current_type == Strip) {
		strip_ = m_item_strip_H.value(current);
	}
	else if (current_type == Terminal && current->parent())
	{
		const auto parent_type{current->parent()->type()};
		if (parent_type == Strip) {
			strip_ = m_item_strip_H.value(current->parent());
		} else if (parent_type == FreeTerminal) {
			current_is_free = true;
		}
	}

	if (strip_ != m_current_strip) {
		setCurrentStrip(strip_);
	} else if (current_is_free != m_current_is_free_terminal) {
		m_current_is_free_terminal = current_is_free;
		emit currentStripChanged(nullptr);
	}
}

/**
 * @brief TerminalStripTreeDockWidget::buildTree
 */
void TerminalStripTreeDockWidget::buildTree()
{

	auto title_ = m_project->title();
	if (title_.isEmpty()) {
		title_ = tr("Projet sans titre");
	}

	QStringList strl{title_};
	new QTreeWidgetItem(ui->m_tree_view, strl, Root);

	QStringList ftstrl(tr("Bornes indépendante"));
	new QTreeWidgetItem(ui->m_tree_view, ftstrl, FreeTerminal);

	auto ts_vector = m_project->terminalStrip();
	std::sort(ts_vector.begin(), ts_vector.end(), [](TerminalStrip *a, TerminalStrip *b) {
		return a->name() < b->name();
	});

	for (const auto &ts : std::as_const(ts_vector)) {
		addTerminalStrip(ts);
	}
	addFreeTerminal();
}

QTreeWidgetItem* TerminalStripTreeDockWidget::addTerminalStrip(TerminalStrip *terminal_strip)
{
	if (auto item = m_item_strip_H.key(terminal_strip)) {
		return item;
	}

	auto root_item = ui->m_tree_view->topLevelItem(0);

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
		inst_qtwi = new QTreeWidgetItem(root_item, inst_strl, Installation);
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
		loc_qtwi = new QTreeWidgetItem(inst_qtwi, loc_strl, Location);
	}

		//Add the terminal strip
	QStringList name{terminal_strip->name()};
	auto strip_item = new QTreeWidgetItem(loc_qtwi, name, Strip);
	strip_item->setData(0, UUID_USER_ROLE, terminal_strip->uuid());
	strip_item->setIcon(0, QET::Icons::TerminalStrip);

		//Add child terminal of the strip
	for (auto i=0 ; i<terminal_strip->physicalTerminalCount() ; ++i)
	{
		auto phy_t = terminal_strip->physicalTerminal(i);
		if (phy_t->realTerminalCount())
		{
			QString text_;
			for (const auto &real_t : phy_t->realTerminals())
			{
				if (text_.isEmpty())
					text_ = real_t->label();
				else
					text_.append(QStringLiteral(", ")).append(real_t->label());
			}
			const auto real_t = phy_t->realTerminals().at(0);
			auto terminal_item = new QTreeWidgetItem(strip_item, QStringList(text_), Terminal);
			terminal_item->setData(0, UUID_USER_ROLE, phy_t->uuid());
			terminal_item->setIcon(0, QET::Icons::ElementTerminal);
		}
	}

	m_item_strip_H.insert(strip_item, terminal_strip);
	m_uuid_strip_H.insert(terminal_strip->uuid(), terminal_strip);

	m_strip_changed_connection.append(connect(terminal_strip, &TerminalStrip::orderChanged, this, &TerminalStripTreeDockWidget::reload));
	return strip_item;
}

/**
 * @brief TerminalStripTreeDockWidget::addFreeTerminal
 */
void TerminalStripTreeDockWidget::addFreeTerminal()
{
	ElementProvider ep(m_project);
	auto vector_ = ep.freeTerminal();

	if (vector_.isEmpty()) {
		return;
	}

		//Sort the terminal element by label
	std::sort(vector_.begin(), vector_.end(), [](TerminalElement *a, TerminalElement *b)
	{
		return a->elementData().m_informations.value(QETInformation::ELMT_LABEL).toString()
				<
				b->elementData().m_informations.value(QETInformation::ELMT_LABEL).toString();
	});

	auto free_terminal_item = ui->m_tree_view->topLevelItem(1);

	for (const auto terminal : std::as_const(vector_))
	{
		QUuid uuid_ = terminal->uuid();
		QStringList strl{terminal->actualLabel()};
		auto item = new QTreeWidgetItem(free_terminal_item, strl, Terminal);
		item->setData(0, UUID_USER_ROLE, uuid_.toString());
		item->setIcon(0, QET::Icons::ElementTerminal);

		m_uuid_terminal_H.insert(uuid_, terminal->realTerminal());
	}
}

void TerminalStripTreeDockWidget::setCurrentStrip(TerminalStrip *strip)
{
	m_current_strip = strip;
	emit currentStripChanged(strip);
}
