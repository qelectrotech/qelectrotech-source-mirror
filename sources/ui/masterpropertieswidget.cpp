/*
 *	Copyright 2006-2026 The QElectroTech Team
 *	This file is part of QElectroTech.
 *
 *	QElectroTech is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	QElectroTech is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "masterpropertieswidget.h"
#include "contactgroupselectiondialog.h"
#include "../qetproject.h"
#include "../diagram.h"
#include "../diagramposition.h"
#include "../elementprovider.h"
#include "../qetgraphicsitem/element.h"
#include "../undocommand/linkelementcommand.h"
#include "ui_masterpropertieswidget.h"
#include "../properties/elementdata.h"

#include <QListWidgetItem>
#include <QMessageBox>
#include <QTableWidget>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QApplication>
#include <QShortcut>
#include <QPushButton>


/**
 *	@brief MasterPropertiesWidget::MasterPropertiesWidget
 *	Default constructor
 *	@param elmt
 *	@param parent
 */
MasterPropertiesWidget::MasterPropertiesWidget(Element *elmt, QWidget *parent) :
AbstractElementPropertiesEditorWidget(parent),
ui(new Ui::MasterPropertiesWidget),
m_project(nullptr)
{
	ui->setupUi(this);

	ui->m_free_tree_widget->setContextMenuPolicy(Qt::CustomContextMenu);
	ui->m_link_tree_widget->setContextMenuPolicy(Qt::CustomContextMenu);

	QStringList list;
	QSettings settings;
	if (settings.value("genericpanel/folio", false).toBool()) {
		list << tr("Vignette")
		<< tr("Label de folio")
		<< tr("Titre de folio")
		<< tr("Position");
	}
	else {
		list << tr("Vignette")
		<< tr("N° de folio")
		<< tr("Titre de folio")
		<< tr("Position");
	}
	ui->m_free_tree_widget->setHeaderLabels(list);
	ui->m_link_tree_widget->setHeaderLabels(list);

	m_context_menu  = new QMenu(this);
	m_link_action   = new QAction(tr("Lier l'élément"), this);
	m_unlink_action = new QAction(tr("Délier l'élément"), this);
	m_show_qtwi     = new QAction(tr("Montrer l'élément"), this);
	m_show_element  = new QAction(tr("Montrer l'élément maître"), this);
	m_save_header_state = new QAction(tr("Enregistrer la disposition"), this);

	connect(ui->m_free_tree_widget, &QTreeWidget::itemDoubleClicked,
			this, &MasterPropertiesWidget::showElementFromTWI);
	connect(ui->m_link_tree_widget, &QTreeWidget::itemDoubleClicked,
			this, &MasterPropertiesWidget::showElementFromTWI);

	connect(ui->m_free_tree_widget, &QTreeWidget::customContextMenuRequested,
			[this](QPoint point) {this->customContextMenu(point, 1);});
	connect(ui->m_link_tree_widget, &QTreeWidget::customContextMenuRequested,
			[this](QPoint point) {this->customContextMenu(point, 2);});

	connect(m_link_action,   &QAction::triggered,
			this, &MasterPropertiesWidget::on_link_button_clicked);
	connect(m_unlink_action, &QAction::triggered,
			this, &MasterPropertiesWidget::on_unlink_button_clicked);
	connect(m_show_qtwi,     &QAction::triggered,
			[this]() {this->showElementFromTWI(this->m_qtwi_at_context_menu,0);});

	connect(m_show_element,  &QAction::triggered, [this]()
	{
		this->m_element->diagram()->showMe();
		this->m_element->setHighlighted(true);
		if(this->m_showed_element)
			m_showed_element->setHighlighted(false);
	});

	QHeaderView *qhv = ui->m_free_tree_widget->header();
	qhv->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(qhv, &QHeaderView::customContextMenuRequested,
			this, &MasterPropertiesWidget::headerCustomContextMenuRequested);
	connect(m_save_header_state, &QAction::triggered, [qhv]()
	{
		QByteArray qba = qhv->saveState();
		QSettings settings;
		settings.setValue("link-element-widget/master-state", qba);
	});

	setElement(elmt);
}

/**
 *	@brief MasterPropertiesWidget::~MasterPropertiesWidget
 *	Destructor
 */
MasterPropertiesWidget::~MasterPropertiesWidget()
{
	if (m_showed_element)
		m_showed_element->setHighlighted(false);

	if(m_element)
		m_element->setHighlighted(false);

	delete ui;
}

/**
 *	@brief MasterPropertiesWidget::setElement
 *	Set the element to be edited
 *	@param element
 */
void MasterPropertiesWidget::setElement(Element *element)
{
	if (m_element == element)
		return;

	if (m_showed_element)
	{
		m_showed_element->setHighlighted(false);
		m_showed_element = nullptr;
	}
	if (m_element)
		m_element->setHighlighted(false);

	if (m_project)
		disconnect(m_project, &QETProject::diagramRemoved, this, &MasterPropertiesWidget::diagramWasdeletedFromProject);

	if(Q_LIKELY(element->diagram() && element->diagram()->project()))
		{
			m_project = element->diagram()->project();
			connect(m_project, &QETProject::diagramRemoved, this, &MasterPropertiesWidget::diagramWasdeletedFromProject);
		}
		else
			m_project = nullptr;

	//Keep up to date this widget when the linked elements of m_element change
	if (m_element)
		disconnect(m_element.data(), &Element::linkedElementChanged,
				   this, &MasterPropertiesWidget::updateUi);

	m_element = element;
	connect(m_element.data(), &Element::linkedElementChanged,
			this, &MasterPropertiesWidget::updateUi);

	updateUi();
}

/**
 *	@brief MasterPropertiesWidget::apply
 *	If link between edited element and other change,
 *	apply the change with a QUndoCommand (got with method associatedUndo)
 *	pushed to the stack of element project.
 *	Return true if link change, else false
 *	@note is void no Return ???
 */
void MasterPropertiesWidget::apply()
{
	if (QUndoCommand *undo = associatedUndo())
		m_element -> diagram() -> undoStack().push(undo);

	m_pending_group_indices.clear();
}

/**
 *	@brief MasterPropertiesWidget::reset
 *	Reset current widget, clear eveything and rebuild widget.
 */
void MasterPropertiesWidget::reset()
{
	foreach (QTreeWidgetItem *qtwi, m_qtwi_hash.keys())
		delete qtwi;

	m_qtwi_hash.clear();
	updateUi();
}

/**
 *	@brief MasterPropertiesWidget::associatedUndo
 *	If link between the edited element and other change,
 *	return a QUndoCommand with this change.
 *	If no change return nullptr.
 *	@return
 */
QUndoCommand* MasterPropertiesWidget::associatedUndo() const
{
	// PLC masters manage their slave links via the IO table (setElementData),
	// not via the link tree widget. The link tree is always empty for PLC
	// masters, so we must not create an unlinkAll command.
	if (m_element &&
		m_element->elementData().m_type == ElementData::Master &&
		m_element->elementData().m_master_type == ElementData::PLC)
	{
		return nullptr;
	}

	QList <Element *> to_link;
	QList <Element *> linked_ = m_element->linkedElements();

	for (int i=0; i<ui->m_link_tree_widget->topLevelItemCount(); i++)
		to_link << m_qtwi_hash[ui->m_link_tree_widget->topLevelItem(i)];

	//The two list contain the same element, there is no change
	if (to_link.size() == linked_.size())
	{
		bool equal = true;

		foreach(Element *elmt, to_link)
			if (!linked_.contains(elmt))
				equal = false;

		if(equal)
			return nullptr;
	}

	LinkElementCommand *undo = new LinkElementCommand(m_element);

	if (to_link.isEmpty())
		undo->unlinkAll();
	else
	{
		undo->setLink(to_link);

		//Pass group indices for newly linked slaves
		if (!m_pending_group_indices.isEmpty())
		{
			QMap<Element*, int> indices;
			for (Element *slave : to_link)
			{
				if (m_pending_group_indices.contains(slave))
					indices[slave] = m_pending_group_indices.value(slave);
			}
			if (!indices.isEmpty())
				undo->setGroupIndices(indices);
		}
	}

	return undo;
}

/**
 *	@brief MasterPropertiesWidget::setLiveEdit
 *	@param live_edit = true : live edit is enable
 *	else false : live edit is disable.
 *	@return always true because live edit is handled by this editor widget
 */
bool MasterPropertiesWidget::setLiveEdit(bool live_edit)
{
	m_live_edit = live_edit;
	return true;
}



/**
 *	@brief MasterPropertiesWidget::headerCustomContextMenuRequested
 *	@param pos
 */
void MasterPropertiesWidget::headerCustomContextMenuRequested(const QPoint &pos)
{
	m_context_menu->clear();
	m_context_menu->addAction(m_save_header_state);
	m_context_menu->popup(ui->m_free_tree_widget->header()->mapToGlobal(pos));
}

/**
 * @brief MasterPropertiesWidget::on_link_button_clicked
 * Moves the current item from the free_list to the linked_list,
 * provided the master's slave limit has not been reached.
 */
void MasterPropertiesWidget::on_link_button_clicked()
{
	// Get the maximum number of allowed slaves from the element's information
	QVariant max_slaves_variant = m_element->kindInformations().value("max_slaves");

	if (max_slaves_variant.isValid() && !max_slaves_variant.toString().isEmpty()) {
		int max_slaves = max_slaves_variant.toInt();
		int current_slaves = ui->m_link_tree_widget->topLevelItemCount();

		// If a limit is set and reached
		if (max_slaves != -1 && current_slaves >= max_slaves) {


			// Show a message box with the actual window as parent to ensure it's on top
			QMessageBox::warning(this->window(),
								 tr("Nombre maximal d'esclaves atteint."),
								 tr("Cet élément maître ne peut plus accepter aucun nouveau contact esclave, la limite fixée a été atteinte (Limite: %1).").arg(max_slaves));
			return;
		}
	}

	// Move current item from free_list to linked_list
	QTreeWidgetItem *qtwi = ui->m_free_tree_widget->currentItem();
	if (qtwi)
	{
		Element *slave_elmt = m_qtwi_hash.value(qtwi);

		//If master has contact groups, show group selection dialog
		const auto &groups = m_element->elementData().m_slave_contact_groups;
		if (!groups.isEmpty() && slave_elmt)
		{
			// Collect already-used group indices from the master
			QSet<int> used_indices;
			for (Element *linked : m_element->linkedElements()) {
				int idx = m_element->groupIndexForElement(linked);
				if (idx >= 0) {
					used_indices.insert(idx);
				}
			}

			// Don't mark the current slave as used (it might be relinked)
			if (slave_elmt->linkedElements().contains(m_element)) {
				int current_idx = m_element->groupIndexForElement(slave_elmt);
				if (current_idx >= 0) {
					used_indices.remove(current_idx);
				}
			}

			ContactGroupSelectionDialog dlg(groups, used_indices,
				slave_elmt->elementData(), this);
			if (dlg.exec() == QDialog::Accepted && dlg.selectedIndex() >= 0) {
				m_pending_group_indices[slave_elmt] = dlg.selectedIndex();
			} else {
				return;
			}
		}

		ui->m_free_tree_widget->takeTopLevelItem(
			ui->m_free_tree_widget->indexOfTopLevelItem(qtwi));
		ui->m_link_tree_widget->insertTopLevelItem(0, qtwi);

		if(m_live_edit)
			apply();
	}
}


/**
 *	@brief MasterPropertiesWidget::on_unlink_button_clicked
 *	move current item in linked_list to free_list
 */
void MasterPropertiesWidget::on_unlink_button_clicked()
{
	//take the current item from linked_list and push it to free_list
	QTreeWidgetItem *qtwi = ui->m_link_tree_widget->currentItem();
	if(qtwi)
	{
		ui->m_link_tree_widget->takeTopLevelItem(
			ui->m_link_tree_widget->indexOfTopLevelItem(qtwi));
		ui->m_free_tree_widget->insertTopLevelItem(0, qtwi);

		if(m_live_edit)
			apply();
	}
}

/**
 *	@brief MasterPropertiesWidget::showElementFromTWI
 *	Show the element corresponding to the given QTreeWidgetItem
 *	@param qtwi
 *	@param column
 */
void MasterPropertiesWidget::showElementFromTWI(QTreeWidgetItem *qtwi, int column)
{
	Q_UNUSED(column);
	if (m_showed_element)
	{
		disconnect(m_showed_element, &QObject::destroyed, this, &MasterPropertiesWidget::showedElementWasDeleted);
		m_showed_element -> setHighlighted(false);
	}
	if (m_element)
		m_element->setHighlighted(false);

	m_showed_element = m_qtwi_hash[qtwi];
	m_showed_element->diagram()->showMe();
	m_showed_element->setHighlighted(true);
	connect(m_showed_element, &QObject::destroyed, this, &MasterPropertiesWidget::showedElementWasDeleted);
}

/**
 *	@brief MasterPropertiesWidget::showedElementWasDeleted
 *	Set to nullptr the current showed element when he was deleted
 */
void MasterPropertiesWidget::showedElementWasDeleted()
{
	m_showed_element = nullptr;
}

/**
 *	@brief MasterPropertiesWidget::diagramWasdeletedFromProject
 *	This slot is called when a diagram is removed from the parent project
 *	of edited element to update the content of this widget
 */
void MasterPropertiesWidget::diagramWasdeletedFromProject()
{
	// We use a timer because if the removed diagram
	// contains slave element linked to the edited element
	// we must wait for this elements be unlinked,
	// or else the linked list provides deleted elements.
	QTimer::singleShot(10, this, &MasterPropertiesWidget::updateUi);
}

/**
 *	@brief MasterPropertiesWidget::customContextMenu
 *	Display a context menu
 *	@param pos
 *	@param i : the tree widget where the context menu was requested.
 */
void MasterPropertiesWidget::customContextMenu(const QPoint &pos, int i)
{
	// add the size of the header to display the topleft of the QMenu
	// at the position of the mouse.
	// See doc about QWidget::customContextMenuRequested
	// section related to QAbstractScrollArea
	QPoint point = pos;
	point.ry()+=ui->m_free_tree_widget->header()->height();

	m_context_menu->clear();

	if (i == 1)
	{
		point = ui->m_free_tree_widget->mapToGlobal(point);

		//Context at for free tree widget
		if (ui->m_free_tree_widget->currentItem())
		{
			m_qtwi_at_context_menu = ui->m_free_tree_widget->currentItem();
			m_context_menu->addAction(m_link_action);
			m_context_menu->addAction(m_show_qtwi);
		}
	}
	else
	{
		point = ui->m_link_tree_widget->mapToGlobal(point);

		//context at for link tre widget
		if (ui->m_link_tree_widget->currentItem())
		{
			m_qtwi_at_context_menu = ui->m_link_tree_widget->currentItem();
			m_context_menu->addAction(m_unlink_action);
			m_context_menu->addAction(m_show_qtwi);
		}
	}

	m_context_menu->addAction(m_show_element);
	m_context_menu->popup(point);
}

/**
 * @brief MasterPropertiesWidget::updateUi
 * Build the interface of the widget
 */
void MasterPropertiesWidget::updateUi()
{
	ui->m_free_tree_widget->clear();
	ui->m_link_tree_widget->clear();
	m_qtwi_hash.clear();

	// Check if this is a PLC master
	bool is_plc = m_element &&
		m_element->elementData().m_type == ElementData::Master &&
		m_element->elementData().m_master_type == ElementData::PLC;

	// Show/hide normal master UI and PLC UI
	ui->m_free_tree_widget->setVisible(!is_plc);
	ui->m_link_tree_widget->setVisible(!is_plc);
	ui->label->setVisible(!is_plc);
	ui->label_2->setVisible(!is_plc);
	ui->link_button->setVisible(!is_plc);
	ui->unlink_button->setVisible(!is_plc);

	// In PLC mode, make hidden widgets take no space in the grid layout
	if (is_plc) {
		for (QWidget *w : {static_cast<QWidget*>(ui->m_free_tree_widget),
				   static_cast<QWidget*>(ui->m_link_tree_widget),
				   static_cast<QWidget*>(ui->label),
				   static_cast<QWidget*>(ui->label_2),
				   static_cast<QWidget*>(ui->link_button),
				   static_cast<QWidget*>(ui->unlink_button)}) {
			w->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
			w->setMinimumSize(0, 0);
			w->setMaximumSize(0, 0);
		}
		for (int col = 0; col < ui->gridLayout->columnCount(); ++col)
			ui->gridLayout->setColumnStretch(col, col < 2 ? 1 : 0);
	}

	if (is_plc) {
		// Create PLC widget if not yet created
		if (!m_plc_widget) {
			m_plc_widget = new QWidget(ui->gridLayout->parentWidget());

			auto *plc_layout = new QVBoxLayout(m_plc_widget);
			plc_layout->setContentsMargins(0, 0, 0, 0);

			// Table
			m_plc_table = new QTableWidget(m_plc_widget);
			m_plc_table->setColumnCount(6);
			m_plc_table->setHorizontalHeaderLabels({
				tr("Type"), tr("Adresse"), tr("Fonction"),
				tr("Commentaire"), tr("Réf. croisée"), tr("Bornes")
			});
			m_plc_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
			m_plc_table->setSelectionBehavior(QAbstractItemView::SelectItems);
			m_plc_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
			m_plc_table->setMinimumHeight(200);

			plc_layout->addWidget(m_plc_table);

			connect(m_plc_table, &QTableWidget::cellChanged, this, &MasterPropertiesWidget::plcIOCellChanged);

			// Context menu for the PLC table
			m_plc_table->setContextMenuPolicy(Qt::CustomContextMenu);
			connect(m_plc_table, &QTableWidget::customContextMenuRequested,
					this, &MasterPropertiesWidget::plcShowTableContextMenu);

			// Ctrl+V shortcut for paste
			auto *paste_shortcut = new QShortcut(QKeySequence::Paste, m_plc_table);
			connect(paste_shortcut, &QShortcut::activated, this, &MasterPropertiesWidget::plcPasteFromClipboard);
		}

		ui->gridLayout->addWidget(m_plc_widget, 0, 0, 1, 5);
		m_plc_widget->setVisible(true);

		// Load PLC data into table (block signals to prevent cellChanged during population)
		m_plc_updating = true;
		m_plc_table->blockSignals(true);

		// Clear existing rows
		m_plc_table->setRowCount(0);

		ElementData::PlcMasterData plc_data = m_element->elementData().plcMasterData();

		m_plc_table->setRowCount(plc_data.ios.size());
		for (int row = 0; row < plc_data.ios.size(); ++row) {
			const ElementData::PlcIO &io = plc_data.ios.at(row);

			// Type combo (read-only — type is defined in the editor)
			auto *type_cb = new QComboBox(m_plc_table);
			type_cb->setEnabled(false);
			type_cb->setStyleSheet("QComboBox { background-color: #f0f0f0; }");
			QStringList plc_types = ElementData::plcIOTypeList();
			for (int t = 0; t < plc_types.size(); ++t) {
				type_cb->addItem(plc_types.at(t), t);
			}
			type_cb->setCurrentIndex(static_cast<int>(io.type));
			m_plc_table->setCellWidget(row, 0, type_cb);

			// Address
			auto *addr_item = new QTableWidgetItem(io.address);
			m_plc_table->setItem(row, 1, addr_item);

			// Function text
			auto *func_item = new QTableWidgetItem(io.functionText);
			m_plc_table->setItem(row, 2, func_item);

			// Comment
			auto *comment_item = new QTableWidgetItem(io.comment);
			m_plc_table->setItem(row, 3, comment_item);

			// CrossRef (read-only)
			auto *crossref_item = new QTableWidgetItem(io.crossRef);
			crossref_item->setFlags(crossref_item->flags() & ~Qt::ItemIsEditable);
			m_plc_table->setItem(row, 4, crossref_item);

			// Anschlüsse (read-only)
			auto *terminals_item = new QTableWidgetItem(io.terminals.join(QStringLiteral(", ")));
			terminals_item->setFlags(terminals_item->flags() & ~Qt::ItemIsEditable);
			m_plc_table->setItem(row, 5, terminals_item);
		}

		m_plc_table->blockSignals(false);
		m_plc_updating = false;
	} else {
		// Hide PLC widget if it was shown before
		if (m_plc_widget)
			m_plc_widget->setVisible(false);

		// Restore normal widget size policies
		for (QWidget *w : {static_cast<QWidget*>(ui->m_free_tree_widget),
				   static_cast<QWidget*>(ui->m_link_tree_widget),
				   static_cast<QWidget*>(ui->label),
				   static_cast<QWidget*>(ui->label_2),
				   static_cast<QWidget*>(ui->link_button),
				   static_cast<QWidget*>(ui->unlink_button)}) {
			w->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
			w->setMinimumSize(0, 0);
			w->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
		}
		for (int col = 0; col < ui->gridLayout->columnCount(); ++col)
			ui->gridLayout->setColumnStretch(col, 0);

		if (Q_UNLIKELY(!m_project))
			return;

		ElementProvider elmt_prov(m_project);
		QSettings settings;

		//Build the list of free available element
		QList <QTreeWidgetItem *> items_list;
		for(const auto &elmt : elmt_prov.freeElement(ElementData::Slave))
		{
			// Filter out PLC-Slave elements — they should only link to PLC-Master
			if (elmt->elementData().m_slave_type == ElementData::PLCSlave)
				continue;

			QTreeWidgetItem *qtwi = new QTreeWidgetItem(ui->m_free_tree_widget);
			qtwi->setIcon(0, elmt->pixmap());

			if(settings.value("genericpanel/folio", false).toBool())
			{
				autonum::sequentialNumbers seq;
				QString F =autonum::AssignVariables::formulaToLabel(
					elmt->diagram()->border_and_titleblock.folio(),
																	seq,
														   elmt->diagram(),
																	elmt);
				qtwi->setText(1, F);
			}
			else
			{
				qtwi->setText(1, QString::number(
					elmt->diagram()->folioIndex()
					+ 1));
			}


			qtwi->setText(2, elmt->diagram()->title());
			qtwi->setText(4, elmt->diagram()->convertPosition(
				elmt->scenePos()).toString());
			items_list.append(qtwi);
			m_qtwi_hash.insert(qtwi, elmt);
		}

		ui->m_free_tree_widget->addTopLevelItems(items_list);
		items_list.clear();

		//Build the list of already linked element
		const QList<Element *> link_list = m_element->linkedElements();
		for(Element *elmt : link_list)
		{
			QTreeWidgetItem *qtwi = new QTreeWidgetItem(ui->m_link_tree_widget);
			qtwi->setIcon(0, elmt->pixmap());

			if(settings.value("genericpanel/folio", false).toBool())
			{
				autonum::sequentialNumbers seq;
				QString F =autonum::AssignVariables::formulaToLabel(
					elmt->diagram()->border_and_titleblock.folio(),
																	seq,
														   elmt->diagram(),
																	elmt);
				qtwi->setText(1, F);
			}
			else
			{
				qtwi->setText(1, QString::number(
					elmt->diagram()->folioIndex()
					+ 1));
			}

			qtwi->setText(2, elmt->diagram()->title());
			qtwi->setText(3, elmt->diagram()->convertPosition(
				elmt->scenePos()).toString());
			items_list.append(qtwi);
			m_qtwi_hash.insert(qtwi, elmt);
		}

		if(items_list.count())
			ui->m_link_tree_widget->addTopLevelItems(items_list);

		QVariant v = settings.value("link-element-widget/master-state");
		if(!v.isNull())
		{
			ui->m_free_tree_widget->header()->restoreState(v.toByteArray());
			ui->m_link_tree_widget->header()->restoreState(v.toByteArray());
		}
	}
}

// ============================================================================
// PLC IO Table methods
// ============================================================================

/**
 * @brief MasterPropertiesWidget::plcPasteFromClipboard
 * Paste IO data from clipboard.
 *
 * Two modes:
 * 1. Single-column data (no tabs, e.g. copied vertically from A1:A6):
 *    Values are pasted vertically down the current column.
 * 2. Multi-column data (tab-separated, e.g. copied from a row in Excel):
 *    Each line becomes a separate IO row.
 */
void MasterPropertiesWidget::plcPasteFromClipboard()
{
	if (!m_plc_table || !m_element)
		return;

	QString clipboard_text = QApplication::clipboard()->text();
	if (clipboard_text.isEmpty())
		return;

	QStringList lines = clipboard_text.split('\n', Qt::SkipEmptyParts);
	if (lines.isEmpty())
		return;

	bool has_tabs = false;
	for (const QString &line : lines) {
		if (line.contains('\t')) {
			has_tabs = true;
			break;
		}
	}

	m_plc_updating = true;

	if (!has_tabs) {
		// Vertical paste: values go down the same column, within existing rows only
		int target_col = m_plc_table->currentColumn();
		if (target_col < 0) target_col = 0;
		int target_row = m_plc_table->currentRow();
		if (target_row < 0) target_row = 0;
		int max_rows = m_plc_table->rowCount();

		for (int i = 0; i < lines.size(); ++i) {
			int row = target_row + i;
			if (row >= max_rows) break;
			setCellFromValue(row, target_col, lines.at(i).trimmed());
		}
	} else {
		// Horizontal paste: each line is a separate IO row, within existing rows only
		int target_row = m_plc_table->currentRow();
		if (target_row < 0) target_row = 0;
		int max_rows = m_plc_table->rowCount();

		for (int i = 0; i < lines.size(); ++i) {
			int row = target_row + i;
			if (row >= max_rows) break;
			QStringList cells = lines.at(i).split('\t');
			for (int c = 0; c < cells.size(); ++c) {
				if (c > 6) break;
				setCellFromValue(row, c, cells.at(c).trimmed());
			}
		}
	}

	m_plc_updating = false;
	plcUpdateDisplaySettings();
}

/**
 * @brief MasterPropertiesWidget::setCellFromValue
 * Set a single table cell value, respecting the column widget type.
 */
void MasterPropertiesWidget::setCellFromValue(int row, int col, const QString &val)
{
	if (!m_plc_table || row < 0 || col < 0 || col > 6)
		return;

	if (col == 0) {
		// Type combo
		auto *type_cb = qobject_cast<QComboBox*>(m_plc_table->cellWidget(row, col));
		if (!type_cb) {
			type_cb = new QComboBox(m_plc_table);
			QStringList plc_types = ElementData::plcIOTypeList();
			for (int t = 0; t < plc_types.size(); ++t)
				type_cb->addItem(plc_types.at(t), t);
			m_plc_table->setCellWidget(row, col, type_cb);
			connect(type_cb, QOverload<int>::of(&QComboBox::currentIndexChanged),
				this, [this, row](int) { plcIOCellChanged(row, 0); });
		}
		if (!val.isEmpty()) {
			QStringList plc_types = ElementData::plcIOTypeList();
			for (int t = 0; t < plc_types.size(); ++t) {
				if (plc_types.at(t).compare(val, Qt::CaseInsensitive) == 0) {
					type_cb->setCurrentIndex(t);
					return;
				}
			}
		}
	}
	else if (col == 5) {
		// Anschlüsse (read-only)
		auto *item = new QTableWidgetItem(val);
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		m_plc_table->setItem(row, col, item);
	}
	else {
		// Text columns: Address, Function, Comment
		m_plc_table->setItem(row, col, new QTableWidgetItem(val));
	}
}

/**
 * @brief MasterPropertiesWidget::plcAddRow
 * Add a new empty row to the PLC IO table
 */
void MasterPropertiesWidget::plcAddRow()
{
	if (!m_plc_table)
		return;

	int row = m_plc_table->rowCount();
	m_plc_table->insertRow(row);

	// Type combo
	auto *type_cb = new QComboBox(m_plc_table);
	QStringList plc_types = ElementData::plcIOTypeList();
	for (int t = 0; t < plc_types.size(); ++t) {
		type_cb->addItem(plc_types.at(t), t);
	}
	m_plc_table->setCellWidget(row, 0, type_cb);
	connect(type_cb, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, [this, row](int) { plcIOCellChanged(row, 0); });

	m_plc_table->setItem(row, 1, new QTableWidgetItem());
	m_plc_table->setItem(row, 2, new QTableWidgetItem());
	m_plc_table->setItem(row, 3, new QTableWidgetItem());
	m_plc_table->setItem(row, 4, new QTableWidgetItem());

	// Anschlüsse (read-only)
	auto *terminals_item = new QTableWidgetItem();
	terminals_item->setFlags(terminals_item->flags() & ~Qt::ItemIsEditable);
	m_plc_table->setItem(row, 5, terminals_item);
}

/**
 * @brief MasterPropertiesWidget::plcRemoveRow
 * Remove selected rows from the PLC IO table
 */
void MasterPropertiesWidget::plcRemoveRow()
{
	if (!m_plc_table)
		return;

	QModelIndexList selected = m_plc_table->selectionModel()->selectedRows();
	if (selected.isEmpty())
		return;

	m_plc_updating = true;

	// Remove from bottom to top to preserve indices
	std::sort(selected.begin(), selected.end(),
		[](const QModelIndex &a, const QModelIndex &b) { return a.row() > b.row(); });

	for (const QModelIndex &idx : selected) {
		m_plc_table->removeRow(idx.row());
	}

	m_plc_updating = false;
	plcUpdateDisplaySettings();
}

/**
 * @brief MasterPropertiesWidget::plcMoveRowUp
 * Move selected row up
 */
void MasterPropertiesWidget::plcMoveRowUp()
{
	if (!m_plc_table)
		return;

	int row = m_plc_table->currentRow();
	if (row <= 0)
		return;

	m_plc_updating = true;

	// Swap with row above
	for (int col = 0; col < m_plc_table->columnCount(); ++col) {
		QWidget *w1 = m_plc_table->cellWidget(row, col);
		QWidget *w2 = m_plc_table->cellWidget(row - 1, col);
		m_plc_table->setCellWidget(row, col, w2);
		m_plc_table->setCellWidget(row - 1, col, w1);

		QTableWidgetItem *i1 = m_plc_table->item(row, col);
		QTableWidgetItem *i2 = m_plc_table->item(row - 1, col);
		m_plc_table->setItem(row, col, i2);
		m_plc_table->setItem(row - 1, col, i1);
	}

	m_plc_table->setCurrentCell(row - 1, m_plc_table->currentColumn());
	m_plc_updating = false;
	plcUpdateDisplaySettings();
}

/**
 * @brief MasterPropertiesWidget::plcMoveRowDown
 * Move selected row down
 */
void MasterPropertiesWidget::plcMoveRowDown()
{
	if (!m_plc_table)
		return;

	int row = m_plc_table->currentRow();
	if (row < 0 || row >= m_plc_table->rowCount() - 1)
		return;

	m_plc_updating = true;

	// Swap with row below
	for (int col = 0; col < m_plc_table->columnCount(); ++col) {
		QWidget *w1 = m_plc_table->cellWidget(row, col);
		QWidget *w2 = m_plc_table->cellWidget(row + 1, col);
		m_plc_table->setCellWidget(row, col, w2);
		m_plc_table->setCellWidget(row + 1, col, w1);

		QTableWidgetItem *i1 = m_plc_table->item(row, col);
		QTableWidgetItem *i2 = m_plc_table->item(row + 1, col);
		m_plc_table->setItem(row, col, i2);
		m_plc_table->setItem(row + 1, col, i1);
	}

	m_plc_table->setCurrentCell(row + 1, m_plc_table->currentColumn());
	m_plc_updating = false;
	plcUpdateDisplaySettings();
}

/**
 * @brief MasterPropertiesWidget::plcIOCellChanged
 * Called when a cell in the PLC IO table changes
 */
void MasterPropertiesWidget::plcIOCellChanged(int row, int column)
{
	Q_UNUSED(row)
	Q_UNUSED(column)
	if (m_plc_updating || !m_element || !m_plc_table)
		return;

	// Save immediately
	plcUpdateDisplaySettings();
}

/**
 * @brief MasterPropertiesWidget::plcUpdateDisplaySettings
 * Update the IO data from the table (display settings are managed by the editor only)
 */
void MasterPropertiesWidget::plcUpdateDisplaySettings()
{
	if (m_plc_updating || !m_element || !m_plc_table)
		return;

	m_plc_updating = true;

	// Preserve existing display settings
	ElementData ed = m_element->elementData();
	const auto orig_plc = ed.plcMasterData();
	ElementData::PlcMasterData plc_data = orig_plc;
	plc_data.ios.clear();

	// Read IOs from table, preserving terminal data from original IOs by row index
	for (int row = 0; row < m_plc_table->rowCount(); ++row) {
		ElementData::PlcIO io;

		auto *type_cb = qobject_cast<QComboBox *>(m_plc_table->cellWidget(row, 0));
		if (type_cb)
			io.type = static_cast<ElementData::PlcIOType>(type_cb->currentData().toInt());

		auto *addr_item = m_plc_table->item(row, 1);
		if (addr_item)
			io.address = addr_item->text();

		auto *func_item = m_plc_table->item(row, 2);
		if (func_item)
			io.functionText = func_item->text();

		auto *comment_item = m_plc_table->item(row, 3);
		if (comment_item)
			io.comment = comment_item->text();

		auto *crossref_item = m_plc_table->item(row, 4);
		if (crossref_item)
			io.crossRef = crossref_item->text();

		// Preserve terminal data by matching row index directly.
		// This avoids address-based lookup which fails when all addresses
		// are empty (common in PLC masters) — a hash collision would cause
		// only the last IO's terminals to be used for all rows.
		if (row < orig_plc.ios.size()) {
			const auto &orig_io = orig_plc.ios.at(row);
			io.terminalCount = orig_io.terminalCount;
			io.terminals = orig_io.terminals;
		}

		plc_data.ios.append(io);
	}

	ed.setPlcMasterData(plc_data);
	m_element->setElementData(ed);

	// Trigger update of the cross ref item
	if (m_element->scene())
		m_element->update();

	m_plc_updating = false;
}

/**
 * @brief MasterPropertiesWidget::plcShowTableContextMenu
 * Show context menu for the PLC IO table
 */
void MasterPropertiesWidget::plcShowTableContextMenu(const QPoint &pos)
{
	Q_UNUSED(pos)
	if (!m_plc_table)
		return;

	QMenu menu;
	menu.addAction(tr("Coller depuis le presse-papiers"), this, &MasterPropertiesWidget::plcPasteFromClipboard);

	menu.exec(m_plc_table->mapToGlobal(pos));
}
