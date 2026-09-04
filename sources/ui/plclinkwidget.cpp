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
#include "plclinkwidget.h"

#include "../qetgraphicsitem/masterelement.h"
#include "../qetgraphicsitem/element.h"
#include "../elementprovider.h"
#include "../undocommand/linkelementcommand.h"
#include "../diagram.h"
#include "../qetproject.h"
#include "../properties/elementdata.h"

#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMenu>
#include <QAction>
#include <QFont>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMenu>
#include <QAction>

PlcLinkWidget::PlcLinkWidget(Element *elmt, QWidget *parent)
	: AbstractElementPropertiesEditorWidget(parent)
{
	auto *main_layout = new QGridLayout(this);

	// Row 0: Status label + buttons (shown when already linked)
	m_label = new QLabel(tr("Cet élément est déjà lié"), this);
	m_unlink_pb = new QPushButton(tr("Délier"), this);
	m_show_this_pb = new QPushButton(tr("Voir cet élément"), this);

	main_layout->addWidget(m_label, 0, 0);
	main_layout->addWidget(m_unlink_pb, 0, 1);
	main_layout->addWidget(m_show_this_pb, 0, 2);

	// Row 1: Search field
	m_search_field = new QLineEdit(this);
	m_search_field->setPlaceholderText(tr("Recherche"));
	main_layout->addWidget(m_search_field, 1, 0, 1, 3);

	// Row 2: Tree widget
	m_tree_widget = new QTreeWidget(this);
	m_tree_widget->setHeaderLabels({
		tr("Label"), tr("Type"), tr("Adresse"),
		tr("Fonction"), tr("Commentaire"), tr("Bornes")
	});
	m_tree_widget->setRootIsDecorated(true);
	m_tree_widget->setIndentation(20);
	m_tree_widget->setSelectionMode(QAbstractItemView::SingleSelection);
	m_tree_widget->setContextMenuPolicy(Qt::CustomContextMenu);
	m_tree_widget->header()->setStretchLastSection(false);
	m_tree_widget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	m_tree_widget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
	m_tree_widget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	m_tree_widget->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
	m_tree_widget->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
	m_tree_widget->header()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
	main_layout->addWidget(m_tree_widget, 2, 0, 1, 3);

	// Row 3: Hidden masters note
	m_hidden_masters_label = new QLabel(
		tr("Remarque : les éléments maîtres ayant atteint leur nombre maximal "
		   "d'esclaves sont masqués."), this);
	m_hidden_masters_label->setWordWrap(true);
	QFont italic_font = m_hidden_masters_label->font();
	italic_font.setItalic(true);
	m_hidden_masters_label->setFont(italic_font);
	m_hidden_masters_label->hide();
	main_layout->addWidget(m_hidden_masters_label, 3, 0, 1, 3);

	main_layout->setRowStretch(2, 1);

	setMinimumWidth(500);

	// Connections
	connect(m_unlink_pb, &QPushButton::clicked,
		this, &PlcLinkWidget::on_m_unlink_pb_clicked);
	connect(m_show_this_pb, &QPushButton::clicked,
		this, &PlcLinkWidget::on_m_show_this_pb_clicked);
	connect(m_search_field, &QLineEdit::textEdited,
		this, &PlcLinkWidget::on_m_search_field_textEdited);
	connect(m_tree_widget, &QTreeWidget::customContextMenuRequested,
		this, &PlcLinkWidget::on_m_tree_widget_customContextMenuRequested);

	if (elmt)
		setElement(elmt);
}

void PlcLinkWidget::setElement(Element *element)
{
	if (m_element == element)
		return;

	m_element = element;
	updateUi();
}

void PlcLinkWidget::apply()
{
	QUndoCommand *undo = associatedUndo();
	if (undo)
		m_element->diagram()->undoStack().push(undo);

	m_element_to_link = nullptr;
	m_pending_io_index = -1;
}

QUndoCommand *PlcLinkWidget::associatedUndo() const
{
	if (!m_element_to_link || m_pending_io_index < 0)
		return nullptr;

	LinkElementCommand *undo = new LinkElementCommand(m_element);
	undo->setLink(m_element_to_link);
	undo->setGroupIndex(m_pending_io_index);
	return undo;
}

QString PlcLinkWidget::title() const
{
	return tr("Automate (PLC)");
}

void PlcLinkWidget::updateUi()
{
	if (m_element->isFree())
		hideButtons();
	else
		showButtons();

	buildPlcTree();
}

/**
	@brief PlcLinkWidget::buildPlcTree
	Build the tree showing all PLC-Masters with their IO entries.
	IO entries already linked to a slave are shown as struck-through and greyed out.
*/
void PlcLinkWidget::buildPlcTree()
{
	m_tree_widget->clear();
	m_io_entry_hash.clear();

	if (!m_element || !m_element->diagram() || !m_element->diagram()->project())
		return;

	ElementProvider ep(m_element->diagram()->project());
	QVector<QPointer<Element>> masters = ep.find(ElementData::Master);

	QSet<int> used_io_indices;

	for (Element *elmt : masters) {
		if (!elmt || elmt->elementData().m_master_type != ElementData::PLC)
			continue;

		const auto &plc_data = elmt->elementData().plcMasterData();
		if (plc_data.ios.isEmpty())
			continue;

		// Skip full masters
		MasterElement *me = static_cast<MasterElement*>(elmt);
		if (me->isFull()) {
			m_hidden_masters_label->show();
			continue;
		}

		// Collect used IO indices for this master
		used_io_indices.clear();
		for (Element *linked : elmt->linkedElements()) {
			int idx = elmt->groupIndexForElement(linked);
			if (idx >= 0)
				used_io_indices.insert(idx);
		}

		// Create parent item for this PLC-Master
		auto *parent_item = new QTreeWidgetItem(m_tree_widget);

		// Show master label + diagram info
		QString master_label = elmt->actualLabel();
		if (master_label.isEmpty())
			master_label = elmt->name();

		QString folio_info;
		if (elmt->diagram()) {
			folio_info = QString::number(elmt->diagram()->folioIndex() + 1);
		}
		parent_item->setText(0, QString("%1 (%2)")
			.arg(master_label)
			.arg(folio_info));
		QFont bold_font = parent_item->font(0);
		bold_font.setBold(true);
		parent_item->setFont(0, bold_font);
		parent_item->setExpanded(false);

		// Add child items for each IO entry
		for (int i = 0; i < plc_data.ios.size(); ++i) {
			const auto &io = plc_data.ios.at(i);
			auto *child_item = new QTreeWidgetItem(parent_item);

			child_item->setText(1, ElementData::translatedPlcIOType(io.type));
			child_item->setText(2, io.address);
			child_item->setText(3, io.functionText);
			child_item->setText(4, io.comment);

			// Terminal names
			child_item->setText(5, io.terminals.join(QStringLiteral(", ")));

			PlcIoEntry entry;
			entry.master = elmt;
			entry.ioIndex = i;
			m_io_entry_hash.insert(child_item, entry);

			// If this IO is already linked to a slave, grey it out and strike through
			if (used_io_indices.contains(i)) {
				QFont strike_font = child_item->font(0);
				strike_font.setStrikeOut(true);
				child_item->setFont(0, strike_font);
				child_item->setFont(1, strike_font);
				child_item->setFont(2, strike_font);
				child_item->setFont(3, strike_font);
				child_item->setFont(4, strike_font);
				child_item->setFont(5, strike_font);

				QBrush grey_brush(Qt::gray);
				for (int col = 0; col < 6; ++col)
					child_item->setForeground(col, grey_brush);

				// Show which slave is linked
				for (Element *linked : elmt->linkedElements()) {
					if (elmt->groupIndexForElement(linked) == i) {
						child_item->setToolTip(0,
							tr("Lié à: %1").arg(linked->actualLabel()));
						break;
					}
				}

				child_item->setFlags(child_item->flags() & ~Qt::ItemIsSelectable);
			}
		}
	}
}

void PlcLinkWidget::hideButtons()
{
	m_label->hide();
	m_unlink_pb->hide();
	m_show_this_pb->hide();
	m_search_field->show();
}

void PlcLinkWidget::showButtons()
{
	m_label->show();
	m_unlink_pb->show();
	m_show_this_pb->show();
	m_search_field->hide();
}

void PlcLinkWidget::on_m_search_field_textEdited(const QString &text)
{
	for (int i = 0; i < m_tree_widget->topLevelItemCount(); ++i) {
		QTreeWidgetItem *parent = m_tree_widget->topLevelItem(i);
		bool any_child_visible = false;

		for (int j = 0; j < parent->childCount(); ++j) {
			QTreeWidgetItem *child = parent->child(j);
			bool match = text.isEmpty();
			if (!match) {
				for (int col = 0; col < child->columnCount(); ++col) {
					if (child->text(col).contains(text, Qt::CaseInsensitive)) {
						match = true;
						break;
					}
				}
			}
			child->setHidden(!match);
			if (match) any_child_visible = true;
		}

		// Also check if parent label matches
		if (!text.isEmpty() && parent->text(0).contains(text, Qt::CaseInsensitive)) {
			any_child_visible = true;
			for (int j = 0; j < parent->childCount(); ++j)
				parent->child(j)->setHidden(false);
		}

		parent->setHidden(!any_child_visible);
	}
}

void PlcLinkWidget::on_m_tree_widget_customContextMenuRequested(const QPoint &pos)
{
	QTreeWidgetItem *item = m_tree_widget->itemAt(pos);
	if (!item)
		return;

	// Only show context menu for IO child items (not parent master items)
	if (!m_io_entry_hash.contains(item))
		return;

	const PlcIoEntry &entry = m_io_entry_hash.value(item);

	// Check if this IO is already linked
	Element *master = entry.master;
	if (!master)
		return;

	QSet<int> used_indices;
	for (Element *linked : master->linkedElements()) {
		int idx = master->groupIndexForElement(linked);
		if (idx >= 0)
			used_indices.insert(idx);
	}

	if (used_indices.contains(entry.ioIndex))
		return; // Already linked, no actions

	QMenu menu;
	QAction *link_action = menu.addAction(tr("Connecter"));

	QAction *selected_action = menu.exec(m_tree_widget->viewport()->mapToGlobal(pos));
	if (selected_action == link_action) {
		m_element_to_link = master;
		m_pending_io_index = entry.ioIndex;
		apply();
	}
}

void PlcLinkWidget::on_m_unlink_pb_clicked()
{
	if (!m_element || m_element->isFree())
		return;

	LinkElementCommand *undo = new LinkElementCommand(m_element);
	undo->unlinkAll();
	m_element->diagram()->undoStack().push(undo);

	updateUi();
}

void PlcLinkWidget::on_m_show_this_pb_clicked()
{
	if (!m_element || !m_element->diagram())
		return;

	m_element->diagram()->showMe();
	m_element->setHighlighted(true);
}
