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
#include "terminalstriptreedockwidget.h"
#include "ui_terminalstriptreedockwidget.h"

#include <TerminalStrip/UndoCommand/addterminaltostripcommand.h>
#include <elementprovider.h>
#include <TerminalStrip/physicalterminal.h>
#include <qeticons.h>
#include <qetproject.h>
#include <TerminalStrip/realterminal.h>
#include <qetgraphicsitem/terminalelement.h>
#include <TerminalStrip/terminalstrip.h>
#include <qetinformation.h>
#include <utils/qetutils.h>
#include "freeterminalmodel.h"

#include <QApplication>
#include <QDrag>
#include <QDropEvent>
#include "../../diagram.h"
#include <QBoxLayout>
#include <QMimeData>
#include <QToolButton>
#include <QMouseEvent>
#include <QUuid>

TerminalStripTreeDockWidget::TerminalStripTreeDockWidget(QETProject *project, QWidget *parent) :
	QDockWidget(parent),
    ui(new Ui::TerminalStripTreeDockWidget)
{
	ui->setupUi(this);
    setProject(project);

	ui->m_tree_view->expandRecursively(ui->m_tree_view->rootIndex());

		//Above the tree: fold or unfold it all at once, with the icons of
		//the theme, or the words where it has none
	auto buttons_ = new QHBoxLayout();
	const auto addButton = [this, buttons_](const QString &icon, const QString &text,
											 const QString &tip, void (TerminalStripTreeDockWidget::*slot)()) {
		auto button = new QToolButton(ui->dockWidgetContents);
		const QIcon themed = QIcon::fromTheme(icon);
		if (themed.isNull()) {
			button->setText(text);
		} else {
			button->setIcon(themed);
			button->setAutoRaise(true);
		}
		button->setToolTip(tip);
		button->setAccessibleName(text);
		connect(button, &QToolButton::clicked, this, slot);
		buttons_->addWidget(button);
	};
	addButton(QStringLiteral("expand-all"), tr("Expand all"),
			  tr("Unfold every plant, location and terminal strip of the tree"),
			  &TerminalStripTreeDockWidget::expandAll);
	addButton(QStringLiteral("collapse-all"), tr("Collapse all"),
			  tr("Fold every plant, location and terminal strip of the tree"),
			  &TerminalStripTreeDockWidget::collapseAll);
	buttons_->addStretch();
	if (auto box = qobject_cast<QBoxLayout *>(ui->dockWidgetContents->layout()))
		box->insertLayout(0, buttons_);

		//Free terminals are dragged from this tree or from the free terminal
		//table and dropped on a strip. The tree items themselves never move,
		//the tree is rebuilt from the project after the drop.
	ui->m_tree_view->viewport()->setAcceptDrops(true);
	ui->m_tree_view->viewport()->installEventFilter(this);
}

TerminalStripTreeDockWidget::~TerminalStripTreeDockWidget()
{
	delete ui;
}

/**
 * @brief TerminalStripTreeDockWidget::setProject
 * Set @project as project handled by this tree dock.
 * If a previous project was setted, everything is clear.
 * This function track the destruction of the project,
 * that  mean if the project pointer is deleted
 * no need to call this function with a nullptr,
 * everything is made inside this class.
 * @param project
 */
void TerminalStripTreeDockWidget::setProject(QETProject *project)
{
    if(m_project && m_project_destroy_connection) {
        disconnect(m_project_destroy_connection);
    }
    m_project = project;
    if (m_project) {
            //`this` as context: this dock can be deleted before the project
            //(with the editor window that owns it), and the connection must go with it
        m_project_destroy_connection = connect(m_project, &QObject::destroyed, this, [this](){
            this->m_current_strip.clear();
            this->reload();
        });
    }
    m_current_strip.clear();
    reload();
}

/**
 * @brief TerminalStripTreeDockWidget::reload
 */
void TerminalStripTreeDockWidget::reload()
{
	auto current_ = m_current_strip;
	const bool free_current = m_current_is_free_terminal;

		//What was folded stays folded: a move into a strip does not unfold
		//every strip again
	if (m_built_once) {
		m_expanded.clear();
		QList<QTreeWidgetItem *> stack;
		for (int i = 0; i < ui->m_tree_view->topLevelItemCount(); ++i)
			stack << ui->m_tree_view->topLevelItem(i);
		while (!stack.isEmpty()) {
			QTreeWidgetItem *item = stack.takeLast();
			if (item->isExpanded())
				m_expanded << itemKey(item);
			for (int i = 0; i < item->childCount(); ++i)
				stack << item->child(i);
		}
	}

	ui->m_tree_view->clear();
	m_item_strip_H.clear();
	m_uuid_terminal_H.clear();
	m_uuid_strip_H.clear();

	for (const auto &connection_ : std::as_const(m_strip_changed_connection)) {
		disconnect(connection_);
	}
	m_strip_changed_connection.clear();


	buildTree();

	if (!m_built_once) {
		ui->m_tree_view->expandRecursively(ui->m_tree_view->rootIndex());
		m_built_once = m_project;
	} else {
		QList<QTreeWidgetItem *> stack;
		for (int i = 0; i < ui->m_tree_view->topLevelItemCount(); ++i)
			stack << ui->m_tree_view->topLevelItem(i);
		while (!stack.isEmpty()) {
			QTreeWidgetItem *item = stack.takeLast();
			item->setExpanded(m_expanded.contains(itemKey(item)));
			for (int i = 0; i < item->childCount(); ++i)
				stack << item->child(i);
		}
	}

		//Reselect the tree widget item of the current edited strip, or the
		//independent terminals
   auto item = m_item_strip_H.key(current_);
   if (item) {
	   ui->m_tree_view->setCurrentItem(item);
   } else if (free_current && ui->m_tree_view->topLevelItemCount() > 1) {
	   ui->m_tree_view->setCurrentItem(ui->m_tree_view->topLevelItem(1));
   }
}

/**
 * @brief TerminalStripTreeDockWidget::itemKey
 * @return what tells @a item apart over a reload of the tree: its kind
 * and the texts of the items from the top down to it, or the uuid of a
 * strip
 */
QString TerminalStripTreeDockWidget::itemKey(const QTreeWidgetItem *item) const
{
	if (item->type() == Strip)
		return QStringLiteral("strip:") + item->data(0, UUID_USER_ROLE).toString();
	QStringList path;
	for (const QTreeWidgetItem *i = item; i; i = i->parent())
		path.prepend(i->text(0));
	return QString::number(item->type()) + QLatin1Char(':') + path.join(QChar(0x1f));
}

void TerminalStripTreeDockWidget::expandAll() {
	ui->m_tree_view->expandAll();
}

void TerminalStripTreeDockWidget::collapseAll()
{
		//The project and the independent terminals stay open: everything
		//below them folds
	ui->m_tree_view->collapseAll();
	for (int i = 0; i < ui->m_tree_view->topLevelItemCount(); ++i)
		ui->m_tree_view->topLevelItem(i)->setExpanded(i == 0);
}

/**
 * @brief TerminalStripTreeDockWidget::currentIsFreeTerminals
 * @return true when the independent terminals, or one of them, are
 * selected: the table of the independent terminals shows
 */
bool TerminalStripTreeDockWidget::currentIsFreeTerminals() const {
	return m_current_is_free_terminal;
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
		m_current_is_free_terminal = false;
		setCurrentStrip(nullptr);
		return;
	}

	TerminalStrip *strip_ = nullptr;
	bool current_is_free{false};
	const auto current_type{current->type()};
	if (current_type == Strip) {
		strip_ = m_item_strip_H.value(current);
	}
	else if (current_type == FreeTerminal) {
		current_is_free = true;
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

		//The flag must follow every selection change, or a reload of the tree
		//(e.g. after moving a free terminal) leaves it stale and the next
		//click on a free terminal shows nothing (#1306)
	if (strip_ != m_current_strip || current_is_free != m_current_is_free_terminal) {
		m_current_is_free_terminal = current_is_free;
		setCurrentStrip(strip_);
	}
}

/**
 * @brief TerminalStripTreeDockWidget::buildTree
 */
void TerminalStripTreeDockWidget::buildTree()
{
    if(!m_project) {
        return;
    }

	auto title_ = m_project->title();
	if (title_.isEmpty()) {
		title_ = tr("Untitled project");
	}

	QStringList strl{title_};
	new QTreeWidgetItem(ui->m_tree_view, strl, Root);

	QStringList ftstrl(tr("Independent terminals"));
	new QTreeWidgetItem(ui->m_tree_view, ftstrl, FreeTerminal);

	auto ts_vector = m_project->terminalStrip();
	std::sort(ts_vector.begin(), ts_vector.end(), [](TerminalStrip *a, TerminalStrip *b) {
		return QETUtils::naturalLessThan(a->name(), b->name());
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
	auto vector_ = ep.freeTerminalBlocks();

	if (vector_.isEmpty()) {
		return;
	}

		//Sort the terminal element by label
	std::sort(vector_.begin(), vector_.end(), [](TerminalElement *a, TerminalElement *b)
	{
		return QETUtils::naturalLessThan(
					a->elementData().m_informations.value(QETInformation::ELMT_LABEL).toString(),
					b->elementData().m_informations.value(QETInformation::ELMT_LABEL).toString());
	});

	auto free_terminal_item = ui->m_tree_view->topLevelItem(1);

	for (const auto terminal : std::as_const(vector_))
	{
		QUuid uuid_ = terminal->uuid();
			//One without a label says what and where it is
		QString text_ = terminal->actualLabel();
		if (text_.isEmpty()) {
			const int folio_ = terminal->diagram() ? terminal->diagram()->folioIndex() + 1 : 0;
			text_ = tr("(no label) %1, folio %2").arg(terminal->name()).arg(folio_);
		}
		QStringList strl{text_};
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

/**
 * @brief TerminalStripTreeDockWidget::setDropCheck
 * @param check : called before accepting a drop of free terminals,
 * a drop is refused when it returns false.
 */
void TerminalStripTreeDockWidget::setDropCheck(std::function<bool ()> check) {
	m_drop_check = check;
}

/**
 * @brief TerminalStripTreeDockWidget::eventFilter
 * Drag a free terminal of the tree, and drop free terminals
 * (from the tree or from the free terminal table) on a strip.
 */
bool TerminalStripTreeDockWidget::eventFilter(QObject *watched, QEvent *event)
{
	if (watched != ui->m_tree_view->viewport()) {
		return QDockWidget::eventFilter(watched, event);
	}

	switch (event->type())
	{
		case QEvent::MouseButtonPress:
		{
			auto me = static_cast<QMouseEvent *>(event);
			m_drag_uuid = QUuid();
			auto item = ui->m_tree_view->itemAt(me->pos());
			if (me->button() == Qt::LeftButton
				&& item
				&& item->type() == Terminal
				&& item->parent()
				&& item->parent()->type() == FreeTerminal)
			{
				m_drag_start_pos = me->pos();
				m_drag_uuid = item->data(0, UUID_USER_ROLE).toUuid();
			}
			break;
		}
		case QEvent::MouseMove:
		{
			auto me = static_cast<QMouseEvent *>(event);
			if (!m_drag_uuid.isNull()
				&& (me->buttons() & Qt::LeftButton)
				&& (me->pos() - m_drag_start_pos).manhattanLength() >= QApplication::startDragDistance())
			{
				auto drag = new QDrag(ui->m_tree_view);
				drag->setMimeData(FreeTerminalModel::mimeDataForUuids({m_drag_uuid}));
				drag->setPixmap(QET::Icons::ElementTerminal.pixmap(16, 16));
				m_drag_uuid = QUuid();
				drag->exec(Qt::CopyAction);
				return true;
			}
			break;
		}
		case QEvent::DragEnter:
		{
				//Accepted wherever it enters, or no DragMove follows to reach a strip
			auto de = static_cast<QDragEnterEvent *>(event);
			if (!freeTerminals(de->mimeData()).isEmpty()) {
				de->acceptProposedAction();
			} else {
				de->ignore();
			}
			return true;
		}
		case QEvent::DragMove:
		{
			auto de = static_cast<QDragMoveEvent *>(event);
			if (dropAllowed(de->mimeData(), de->position().toPoint())) {
				de->acceptProposedAction();
			} else {
				de->ignore();
			}
			return true;
		}
		case QEvent::Drop:
		{
			auto de = static_cast<QDropEvent *>(event);
			const auto pos = de->position().toPoint();
			if (!dropAllowed(de->mimeData(), pos)) {
				de->ignore();
				return true;
			}
			auto strip = stripAt(pos);
			const auto terminals = freeTerminals(de->mimeData());
			de->acceptProposedAction();

				//The command rebuilds the tree (TerminalStrip::orderChanged)
			m_project->undoStack()->push(new AddTerminalToStripCommand(terminals, strip));
			setSelectedStrip(strip);
			return true;
		}
		default:
			break;
	}

	return QDockWidget::eventFilter(watched, event);
}

/**
 * @brief TerminalStripTreeDockWidget::stripAt
 * @param pos : position in the viewport of the tree
 * @return the strip of the item at @a pos, the item being the strip
 * or one of its terminals, or nullptr.
 */
TerminalStrip *TerminalStripTreeDockWidget::stripAt(const QPoint &pos) const
{
	auto item = ui->m_tree_view->itemAt(pos);
	if (item && item->type() == Terminal) {
		item = item->parent();
	}
	if (item && item->type() == Strip) {
		return m_item_strip_H.value(item);
	}
	return nullptr;
}

/**
 * @brief TerminalStripTreeDockWidget::freeTerminals
 * @param mime_data
 * @return the free terminals carried by @a mime_data
 * which are still free in the project.
 */
QVector<QSharedPointer<RealTerminal>> TerminalStripTreeDockWidget::freeTerminals(const QMimeData *mime_data) const
{
	QVector<QSharedPointer<RealTerminal>> terminals;
	for (const auto &uuid : FreeTerminalModel::uuidsFromMimeData(mime_data))
	{
		const auto real_t = m_uuid_terminal_H.value(uuid);
		if (real_t && !real_t->parentStrip() && !terminals.contains(real_t)) {
			terminals.append(real_t);
		}
	}
	return terminals;
}

/**
 * @brief TerminalStripTreeDockWidget::dropAllowed
 * @return true if @a mime_data carries free terminals
 * and there is a strip at @a pos to drop them in.
 */
bool TerminalStripTreeDockWidget::dropAllowed(const QMimeData *mime_data, const QPoint &pos) const
{
	return m_project
			&& stripAt(pos)
			&& !freeTerminals(mime_data).isEmpty()
			&& (!m_drop_check || m_drop_check());
}
