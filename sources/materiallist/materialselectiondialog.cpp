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
#include "materialselectiondialog.h"

#include "ui_materialselectiondialog.h"

#include <qetapp.h>
#include <qeticons.h>
#include <qetinformation.h>
#include <qetmessagebox.h>

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHeaderView>
#include <QLineEdit>
#include <QMap>
#include <QMenu>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QSet>
#include <QShortcut>
#include <QStyledItemDelegate>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int SourceRowRole = Qt::UserRole;
constexpr int ColumnKeyRole = Qt::UserRole;

/**
	@brief Delegate attaching QETInformation::NumericInfoValidator when editing
	numeric columns in the right-hand Field/Value table.
*/
class MaterialValueDelegate : public QStyledItemDelegate
{
	public:
		explicit MaterialValueDelegate(QObject *parent = nullptr) :
			QStyledItemDelegate(parent)
		{
		}

		QWidget *createEditor(QWidget *parent,
							  const QStyleOptionViewItem &option,
							  const QModelIndex &index) const override
		{
			QWidget *editor = QStyledItemDelegate::createEditor(parent, option, index);
			if (auto *line_edit = qobject_cast<QLineEdit *>(editor))
			{
				line_edit->setClearButtonEnabled(true);
				const QModelIndex field_index = index.sibling(index.row(), 0);
				const QString column = field_index.data(ColumnKeyRole).toString();
				if (QETInformation::isNumericInfoKey(column))
				{
					line_edit->setValidator(
						new QETInformation::NumericInfoValidator(column, line_edit));
					line_edit->setPlaceholderText(
						MaterialSelectionDialog::tr("e.g. 80.5"));
				}
			}
			return editor;
		}
};

bool recordMatchesTokens(const MaterialRecord &record,
						 const QStringList &columns,
						 const QStringList &tokens)
{
	if (tokens.isEmpty()) {
		return true;
	}

	for (const QString &token : tokens)
	{
		bool found = false;
		for (const QString &column : columns)
		{
			if (record.value(column).contains(token, Qt::CaseInsensitive))
			{
				found = true;
				break;
			}
		}
		if (!found) {
			return false;
		}
	}
	return true;
}

bool sameEntry(const MaterialRecord &lhs, const MaterialRecord &rhs)
{
	QStringList columns = lhs.values.keys();
	for (const QString &column : rhs.values.keys())
	{
		if (!columns.contains(column)) {
			columns.append(column);
		}
	}

	for (const QString &column : columns)
	{
		if (lhs.value(column).trimmed() != rhs.value(column).trimmed()) {
			return false;
		}
	}
	return true;
}

QString articleNumberLabel(const MaterialRecord &record)
{
	QString num = record.value(QStringLiteral("designation")).trimmed();
	if (num.isEmpty()) {
		num = record.value(QStringLiteral("manufacturer_reference")).trimmed();
	}
	if (num.isEmpty()) {
		num = record.value(QStringLiteral("machine_manufacturer_reference")).trimmed();
	}
	return num;
}

} // namespace

/**
	@brief MaterialSelectionDialog::MaterialSelectionDialog
	@param path material CSV file path
	@param parent parent widget
*/
MaterialSelectionDialog::MaterialSelectionDialog(const QString &path,
												 QWidget *parent) :
	QDialog(parent),
	ui(new Ui::MaterialSelectionDialog),
	m_path(path)
{
	ui->setupUi(this);

	ui->m_button_box->button(QDialogButtonBox::Ok)->setText(tr("Apply"));
	ui->m_button_box->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));

	ui->m_tree_widget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->m_tree_widget->header()->setStretchLastSection(true);
	ui->m_tree_widget->sortByColumn(0, Qt::AscendingOrder);
	ui->m_tree_widget->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->m_tree_widget, &QTreeWidget::customContextMenuRequested,
			this, &MaterialSelectionDialog::showTreeContextMenu);

	ui->m_new_entry_btn->setIcon(QET::Icons::Add);
	ui->m_edit_entry_btn->setIcon(QET::Icons::ElementEdit);
	ui->m_duplicate_entry_btn->setIcon(QET::Icons::EditCopy);
	ui->m_remove_entry_btn->setIcon(QET::Icons::EditDelete);
	ui->m_import_btn->setIcon(QET::Icons::DocumentImport);
	ui->m_export_btn->setIcon(QET::Icons::DocumentExport);
	ui->m_settings_btn->setIcon(QET::Icons::Configure);

	ui->m_details_table->setItemDelegateForColumn(1, new MaterialValueDelegate(ui->m_details_table));
	ui->m_details_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->m_details_table->horizontalHeader()->setStretchLastSection(true);
	ui->m_details_table->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(ui->m_details_table, &QTableWidget::customContextMenuRequested,
			this, &MaterialSelectionDialog::showDetailsContextMenu);

	auto *copy_shortcut = new QShortcut(QKeySequence::Copy, ui->m_details_table);
	copy_shortcut->setContext(Qt::WidgetWithChildrenShortcut);
	connect(copy_shortcut, &QShortcut::activated,
			this, &MaterialSelectionDialog::copyDetailsSelection);

	ui->m_edit_bar->setVisible(false);

	ui->m_group_combo->addItem(tr("None"), QString());
	ui->m_group_combo->addItem(tr("Manufacturer"), QStringLiteral("manufacturer"));
	ui->m_group_combo->addItem(tr("Category"), QStringLiteral("category"));
	ui->m_group_combo->setCurrentIndex(2);

	ui->m_filter_field_combo->addItem(tr("Off"), QString());

	connect(ui->m_search_edit, &QLineEdit::textChanged,
			this, &MaterialSelectionDialog::filterUpdated);
	connect(ui->m_group_combo, &QComboBox::currentIndexChanged,
			this, &MaterialSelectionDialog::filterUpdated);
	connect(ui->m_filter_field_combo, &QComboBox::currentIndexChanged,
			this, &MaterialSelectionDialog::filterFieldChanged);
	connect(ui->m_filter_value_combo, &QComboBox::currentIndexChanged,
			this, &MaterialSelectionDialog::filterUpdated);
	connect(ui->m_tree_widget, &QTreeWidget::itemSelectionChanged,
			this, &MaterialSelectionDialog::treeSelectionChanged);
	connect(ui->m_tree_widget, &QTreeWidget::itemDoubleClicked,
			this, &MaterialSelectionDialog::treeItemDoubleClicked);

	reload();

	if (const QSize saved_size = MaterialList::savedDialogSize(); saved_size.isValid()) {
		resize(saved_size);
	}
	ui->m_splitter->setSizes({440, 660});

	updateButtons();
}

/**
	@brief MaterialSelectionDialog::~MaterialSelectionDialog
*/
MaterialSelectionDialog::~MaterialSelectionDialog()
{
	delete ui;
}

void MaterialSelectionDialog::done(int result)
{
	MaterialList::saveDialogSize(size());
	QDialog::done(result);
}

QStringList MaterialSelectionDialog::effectiveColumns() const
{
	QStringList cols = m_data.columns;
	for (const QString &col : MaterialList::defaultColumns())
	{
		if (!cols.contains(col)) {
			cols.append(col);
		}
	}
	return cols;
}

void MaterialSelectionDialog::reload()
{
	QString error;
	MaterialListData data;
	if (!MaterialList::load(m_path, &data, &error))
	{
		QET::QetMessageBox::warning(this,
									tr("Cannot read"),
									tr("Cannot read the file:\n%1\n%2")
										.arg(m_path, error));
	}

	const QString current_filter_field = ui->m_filter_field_combo->currentData().toString();
	const QString current_filter_val = ui->m_filter_value_combo->currentData().toString();
	m_data = data;

	populateFilterFields(current_filter_field);
	populateFilterValues(current_filter_val);
	rebuildTree();
	populateDetails(selectedRecord());
	updateButtons();
}

void MaterialSelectionDialog::populateFilterFields(const QString &keep_field)
{
	const QSignalBlocker blocker(ui->m_filter_field_combo);
	ui->m_filter_field_combo->clear();
	ui->m_filter_field_combo->addItem(tr("Off"), QString());

	const QStringList cols = effectiveColumns();
	for (const QString &col : cols) {
		ui->m_filter_field_combo->addItem(MaterialList::translatedColumn(col), col);
	}

	const int idx = keep_field.isEmpty() ? 0 : ui->m_filter_field_combo->findData(keep_field);
	ui->m_filter_field_combo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void MaterialSelectionDialog::populateFilterValues(const QString &keep_value)
{
	const QSignalBlocker blocker(ui->m_filter_value_combo);
	ui->m_filter_value_combo->clear();

	const QString field = ui->m_filter_field_combo->currentData().toString();
	if (field.isEmpty())
	{
		ui->m_filter_value_combo->addItem(tr("All"), QString());
		ui->m_filter_value_combo->setEnabled(false);
		return;
	}

	ui->m_filter_value_combo->setEnabled(m_edit_mode == EditMode::View);
	ui->m_filter_value_combo->addItem(tr("All"), QString());

	QStringList values;
	bool has_empty = false;
	for (const MaterialRecord &record : std::as_const(m_data.records))
	{
		const QString val = record.value(field).trimmed();
		if (val.isEmpty()) {
			has_empty = true;
		} else if (!values.contains(val, Qt::CaseInsensitive)) {
			values.append(val);
		}
	}
	std::sort(values.begin(), values.end(), [](const QString &a, const QString &b) {
		return QString::compare(a, b, Qt::CaseInsensitive) < 0;
	});

	for (const QString &val : std::as_const(values)) {
		ui->m_filter_value_combo->addItem(val, val);
	}
	if (has_empty) {
		ui->m_filter_value_combo->addItem(tr("(Empty)"), QStringLiteral("__empty__"));
	}

	const int idx = ui->m_filter_value_combo->findData(keep_value);
	ui->m_filter_value_combo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void MaterialSelectionDialog::rebuildTree()
{
	const QSignalBlocker blocker(ui->m_tree_widget);

	QSet<QString> expanded_groups;
	for (int i = 0; i < ui->m_tree_widget->topLevelItemCount(); ++i) {
		QTreeWidgetItem *top = ui->m_tree_widget->topLevelItem(i);
		if (top && top->data(0, SourceRowRole).toInt() < 0 && top->isExpanded()) {
			expanded_groups.insert(top->text(0));
		}
	}
	const int scroll_value = ui->m_tree_widget->verticalScrollBar()
		? ui->m_tree_widget->verticalScrollBar()->value()
		: 0;

	ui->m_tree_widget->clear();

	const QStringList tokens = ui->m_search_edit->text()
			.simplified()
			.split(QLatin1Char(' '), Qt::SkipEmptyParts);
	const QString group_field = ui->m_group_combo->currentData().toString();
	const QString filter_field = ui->m_filter_field_combo->currentData().toString();
	const QString filter_val = ui->m_filter_value_combo->currentData().toString();
	const QStringList cols = effectiveColumns();

	ui->m_tree_widget->setRootIsDecorated(!group_field.isEmpty());

	QMap<QString, QList<int>> grouped_rows;
	QList<int> flat_rows;
	int visible_count = 0;

	for (int i = 0; i < m_data.records.size(); ++i)
	{
		const MaterialRecord &rec = m_data.records.at(i);

		if (!filter_field.isEmpty() && !filter_val.isEmpty())
		{
			const QString val = rec.value(filter_field).trimmed();
			if (filter_val == QStringLiteral("__empty__")) {
				if (!val.isEmpty()) continue;
			} else {
				if (QString::compare(val, filter_val, Qt::CaseInsensitive) != 0) continue;
			}
		}

		if (!recordMatchesTokens(rec, cols, tokens)) {
			continue;
		}

		if (group_field.isEmpty()) {
			flat_rows.append(i);
		} else {
			const QString gval = rec.value(group_field).trimmed();
			const QString group_key = gval.isEmpty() ? tr("(Empty)") : gval;
			grouped_rows[group_key].append(i);
		}
		++visible_count;
	}

	if (group_field.isEmpty())
	{
		for (int row : std::as_const(flat_rows))
		{
			const MaterialRecord &rec = m_data.records.at(row);
			auto *item = new QTreeWidgetItem(ui->m_tree_widget);
			item->setText(0, articleNumberLabel(rec));
			item->setText(1, rec.value(QStringLiteral("description")));
			item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
			item->setData(0, SourceRowRole, row);
		}
	}
	else
	{
		QStringList group_names = grouped_rows.keys();
		std::sort(group_names.begin(), group_names.end(), [](const QString &a, const QString &b) {
			return QString::compare(a, b, Qt::CaseInsensitive) < 0;
		});

		for (const QString &group : std::as_const(group_names))
		{
			auto *group_item = new QTreeWidgetItem(ui->m_tree_widget);
			group_item->setText(0, group);
			group_item->setFirstColumnSpanned(true);
			group_item->setFlags(Qt::ItemIsEnabled);
			group_item->setData(0, SourceRowRole, -1);

			QFont bold_font = group_item->font(0);
			bold_font.setBold(true);
			group_item->setFont(0, bold_font);

			for (int row : grouped_rows.value(group))
			{
				const MaterialRecord &rec = m_data.records.at(row);
				auto *child = new QTreeWidgetItem(group_item);
				child->setText(0, articleNumberLabel(rec));
				child->setText(1, rec.value(QStringLiteral("description")));
				child->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
				child->setData(0, SourceRowRole, row);
			}
		}

		if (!tokens.isEmpty() || !filter_val.isEmpty() || group_names.size() <= 3) {
			ui->m_tree_widget->expandAll();
		} else if (!expanded_groups.isEmpty()) {
			for (int i = 0; i < ui->m_tree_widget->topLevelItemCount(); ++i) {
				QTreeWidgetItem *top = ui->m_tree_widget->topLevelItem(i);
				if (top && expanded_groups.contains(top->text(0))) {
					top->setExpanded(true);
				}
			}
		}
	}

	if (QScrollBar *vbar = ui->m_tree_widget->verticalScrollBar()) {
		vbar->setValue(scroll_value);
	}

	updateCount(visible_count);
}

void MaterialSelectionDialog::updateCount(int visible_count)
{
	const int total = m_data.records.size();
	if (visible_count == total) {
		ui->m_count_label->setText(tr("Entries: %1").arg(total));
	} else {
		ui->m_count_label->setText(tr("Entries: %1 of %2").arg(visible_count).arg(total));
	}
}

void MaterialSelectionDialog::populateDetails(const MaterialRecord &record)
{
	ui->m_details_table->setRowCount(0);
	const QStringList cols = effectiveColumns();
	ui->m_details_table->setRowCount(cols.size());

	const bool editable = (m_edit_mode != EditMode::View);
	ui->m_details_table->setColumnCount(editable ? 3 : 2);
	if (editable)
	{
		ui->m_details_table->setHorizontalHeaderItem(2, new QTableWidgetItem(QString()));
		ui->m_details_table->horizontalHeader()->setStretchLastSection(false);
		ui->m_details_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
		ui->m_details_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
		ui->m_details_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
	}
	else
	{
		ui->m_details_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
		ui->m_details_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
		ui->m_details_table->horizontalHeader()->setStretchLastSection(true);
	}

	for (int i = 0; i < cols.size(); ++i)
	{
		const QString &column = cols.at(i);

		auto *field_item = new QTableWidgetItem(MaterialList::translatedColumn(column));
		field_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		field_item->setData(ColumnKeyRole, column);
		ui->m_details_table->setItem(i, 0, field_item);

		auto *value_item = new QTableWidgetItem(record.value(column));
		Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		if (editable) {
			flags |= Qt::ItemIsEditable;
		}
		value_item->setFlags(flags);
		ui->m_details_table->setItem(i, 1, value_item);

		if (editable)
		{
			auto *btn_item = new QTableWidgetItem();
			btn_item->setFlags(Qt::ItemIsEnabled);
			ui->m_details_table->setItem(i, 2, btn_item);

			if (!QETInformation::isNumericInfoKey(column))
			{
				auto *multiline_btn = new QToolButton(ui->m_details_table);
				multiline_btn->setIcon(QET::Icons::EditText);
				multiline_btn->setAutoRaise(true);
				multiline_btn->setFocusPolicy(Qt::NoFocus);
				multiline_btn->setToolTip(tr("Edit on several lines"));
				const QString field_label = field_item->text();
				connect(multiline_btn, &QToolButton::clicked, this, [this, i, field_label]() {
					QTableWidgetItem *item = ui->m_details_table->item(i, 1);
					if (!item) {
						return;
					}
					QDialog dialog(this);
					dialog.setWindowTitle(field_label);

					auto *edit = new QPlainTextEdit(&dialog);
					edit->setPlainText(item->text());
					auto *buttons = new QDialogButtonBox(
						QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
					connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
					connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

					auto *layout = new QVBoxLayout(&dialog);
					layout->addWidget(edit);
					layout->addWidget(buttons);
					edit->setFocus();

					if (dialog.exec() == QDialog::Accepted) {
						item->setText(edit->toPlainText());
					}
				});
				ui->m_details_table->setCellWidget(i, 2, multiline_btn);
			}
		}
	}
}

void MaterialSelectionDialog::setEditMode(EditMode mode)
{
	m_edit_mode = mode;
	const bool editing = (mode != EditMode::View);

	ui->m_edit_bar->setVisible(editing);
	ui->m_button_box->setVisible(!editing);
	if (mode == EditMode::New) {
		ui->m_edit_mode_label->setText(tr("New entry"));
	} else if (mode == EditMode::Edit) {
		ui->m_edit_mode_label->setText(tr("Editing entry"));
	} else {
		ui->m_edit_mode_label->clear();
	}

	ui->m_tree_widget->setEnabled(!editing);
	ui->m_search_edit->setEnabled(!editing);
	ui->m_group_combo->setEnabled(!editing);
	ui->m_filter_field_combo->setEnabled(!editing);
	ui->m_filter_value_combo->setEnabled(
		!editing && !ui->m_filter_field_combo->currentData().toString().isEmpty());

	if (mode == EditMode::New) {
		populateDetails(MaterialRecord());
	} else if (mode == EditMode::Edit) {
		populateDetails(m_editing_original);
	} else {
		populateDetails(selectedRecord());
	}

	if (editing && ui->m_details_table->rowCount() > 0)
	{
		ui->m_details_table->setCurrentCell(0, 1);
		ui->m_details_table->setFocus();
	}

	updateButtons();
}

MaterialRecord MaterialSelectionDialog::detailsRecord() const
{
	MaterialRecord record;
	if (m_edit_mode == EditMode::Edit) {
		record.extra = m_editing_original.extra;
	}

	const int rows = ui->m_details_table->rowCount();
	for (int i = 0; i < rows; ++i)
	{
		const QTableWidgetItem *field_item = ui->m_details_table->item(i, 0);
		const QTableWidgetItem *value_item = ui->m_details_table->item(i, 1);
		if (!field_item || !value_item) continue;

		const QString column = field_item->data(ColumnKeyRole).toString();
		QString value = value_item->text();
		value.remove(QLatin1Char('\r'));
		value = value.trimmed();
		if (!value.isEmpty()) {
			record.setValue(column, value);
		}
	}

	return record;
}

void MaterialSelectionDialog::updateButtons()
{
	const bool in_view = (m_edit_mode == EditMode::View);
	const bool has_selection = (selectedSourceRow() >= 0);
	const bool has_any_selection = !selectedSourceRows().isEmpty();

	if (QPushButton *ok_btn = ui->m_button_box->button(QDialogButtonBox::Ok)) {
		ok_btn->setEnabled(in_view && has_selection);
	}
	ui->m_new_entry_btn->setEnabled(in_view);
	ui->m_edit_entry_btn->setEnabled(in_view && has_selection);
	ui->m_duplicate_entry_btn->setEnabled(in_view && has_selection);
	ui->m_remove_entry_btn->setEnabled(in_view && has_any_selection);
	ui->m_import_btn->setEnabled(in_view);
	ui->m_export_btn->setEnabled(in_view && !m_data.records.isEmpty());
	ui->m_settings_btn->setEnabled(in_view);
}

void MaterialSelectionDialog::filterFieldChanged()
{
	populateFilterValues(QString());
	filterUpdated();
}

void MaterialSelectionDialog::filterUpdated()
{
	const MaterialRecord current = selectedRecord();
	rebuildTree();
	if (!current.values.isEmpty()) {
		selectRecord(current);
	}
	populateDetails(selectedRecord());
	updateButtons();
}

void MaterialSelectionDialog::treeSelectionChanged()
{
	if (m_edit_mode != EditMode::View) {
		return;
	}
	populateDetails(selectedRecord());
	updateButtons();
}

void MaterialSelectionDialog::treeItemDoubleClicked(QTreeWidgetItem *item, int)
{
	if (m_edit_mode != EditMode::View || !item) {
		return;
	}
	const QVariant row_var = item->data(0, SourceRowRole);
	if (row_var.isValid() && row_var.toInt() >= 0) {
		accept();
	}
}

int MaterialSelectionDialog::selectedSourceRow() const
{
	const QList<QTreeWidgetItem *> items = ui->m_tree_widget->selectedItems();
	if (items.size() != 1) {
		return -1;
	}
	const QVariant row_var = items.first()->data(0, SourceRowRole);
	if (!row_var.isValid()) {
		return -1;
	}
	const int row = row_var.toInt();
	return (row >= 0 && row < m_data.records.size()) ? row : -1;
}

QList<int> MaterialSelectionDialog::selectedSourceRows() const
{
	QList<int> rows;
	const QList<QTreeWidgetItem *> items = ui->m_tree_widget->selectedItems();
	for (QTreeWidgetItem *item : items) {
		const QVariant row_var = item->data(0, SourceRowRole);
		if (!row_var.isValid()) {
			continue;
		}
		const int row = row_var.toInt();
		if (row >= 0 && row < m_data.records.size() && !rows.contains(row)) {
			rows.append(row);
		}
	}
	std::sort(rows.begin(), rows.end());
	return rows;
}

MaterialRecord MaterialSelectionDialog::selectedRecord() const
{
	const int row = selectedSourceRow();
	return row >= 0 ? m_data.records.at(row) : MaterialRecord();
}

bool MaterialSelectionDialog::selectRecord(const MaterialRecord &record)
{
	int target_row = -1;
	for (int i = m_data.records.size() - 1; i >= 0; --i)
	{
		if (sameEntry(m_data.records.at(i), record))
		{
			target_row = i;
			break;
		}
	}
	if (target_row < 0) {
		return false;
	}

	for (int i = 0; i < ui->m_tree_widget->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem *top = ui->m_tree_widget->topLevelItem(i);
		if (top->data(0, SourceRowRole).toInt() == target_row)
		{
			ui->m_tree_widget->setCurrentItem(top);
			ui->m_tree_widget->scrollToItem(top);
			updateButtons();
			return true;
		}
		for (int c = 0; c < top->childCount(); ++c)
		{
			QTreeWidgetItem *child = top->child(c);
			if (child->data(0, SourceRowRole).toInt() == target_row)
			{
				top->setExpanded(true);
				ui->m_tree_widget->setCurrentItem(child);
				ui->m_tree_widget->scrollToItem(child);
				updateButtons();
				return true;
			}
		}
	}
	return false;
}

void MaterialSelectionDialog::accept()
{
	if (m_edit_mode != EditMode::View) {
		return;
	}
	if (selectedRecord().values.isEmpty())
	{
		QET::QetMessageBox::information(this,
										tr("Nothing selected"),
										tr("Select an item in the list first."));
		return;
	}
	QDialog::accept();
}

void MaterialSelectionDialog::on_m_new_entry_btn_clicked()
{
	m_editing_row = -1;
	m_editing_original = MaterialRecord();
	setEditMode(EditMode::New);
}

void MaterialSelectionDialog::on_m_edit_entry_btn_clicked()
{
	const int row = selectedSourceRow();
	if (row < 0) {
		return;
	}
	m_editing_row = row;
	m_editing_original = m_data.records.at(row);
	setEditMode(EditMode::Edit);
}

void MaterialSelectionDialog::on_m_save_entry_btn_clicked()
{
	if (m_edit_mode == EditMode::View) {
		return;
	}

		//Close any active inline cell editor so its text is committed.
	ui->m_details_table->setCurrentItem(nullptr);

	const int rows = ui->m_details_table->rowCount();
	for (int i = 0; i < rows; ++i)
	{
		const QTableWidgetItem *field_item = ui->m_details_table->item(i, 0);
		QTableWidgetItem *value_item = ui->m_details_table->item(i, 1);
		if (!field_item || !value_item) continue;

		const QString column = field_item->data(ColumnKeyRole).toString();
		const QString text = value_item->text().trimmed();
		if (!text.isEmpty() && QETInformation::isNumericInfoKey(column))
		{
			QETInformation::NumericInfoValidator validator(column);
			QString copy = text;
			int pos = 0;
			if (validator.validate(copy, pos) != QValidator::Acceptable)
			{
				QET::QetMessageBox::warning(this,
											tr("Unfinished number"),
											tr("Finish the number or empty the field."));
				ui->m_details_table->setCurrentCell(i, 1);
				ui->m_details_table->editItem(value_item);
				return;
			}
		}
	}

	const MaterialRecord record = detailsRecord();
	if (record.values.isEmpty())
	{
		QET::QetMessageBox::warning(this,
									tr("Nothing filled in"),
									tr("Fill in at least one field to create an entry."));
		return;
	}

	QString error;
	const bool ok = (m_edit_mode == EditMode::New)
			? MaterialList::appendRecord(m_path, record, &error)
			: MaterialList::updateRecord(m_path, m_editing_row, m_editing_original, record, &error);

	if (!ok)
	{
		QET::QetMessageBox::critical(this,
									 tr("Cannot write"),
									 tr("Cannot write to the file:\n%1\n%2")
										.arg(m_path, error));
		return;
	}

	m_editing_row = -1;
	m_editing_original = MaterialRecord();
	setEditMode(EditMode::View);
	reload();

	if (!selectRecord(record))
	{
		ui->m_search_edit->clear();
		ui->m_filter_field_combo->setCurrentIndex(0);
		selectRecord(record);
	}
	populateDetails(selectedRecord());
}

void MaterialSelectionDialog::on_m_cancel_entry_btn_clicked()
{
	m_editing_row = -1;
	m_editing_original = MaterialRecord();
	setEditMode(EditMode::View);
}

void MaterialSelectionDialog::on_m_settings_btn_clicked()
{
	if (QETApp *app = QETApp::instance()) {
		app->configureQETMaterialsList();
	}

	const QString configured = MaterialList::configuredPath();
	if (!configured.isEmpty()) {
		m_path = configured;
	}
	reload();
}

QList<MaterialRecord> MaterialSelectionDialog::selectedRecordsForExport() const
{
	QList<MaterialRecord> result;
	QSet<int> seen;

	auto add_row = [&](const QTreeWidgetItem *item) {
		if (!item) {
			return;
		}
		const QVariant row_var = item->data(0, SourceRowRole);
		if (!row_var.isValid()) {
			return;
		}
		const int row = row_var.toInt();
		if (row >= 0 && row < m_data.records.size() && !seen.contains(row)) {
			seen.insert(row);
			result.append(m_data.records.at(row));
		}
	};

	const QList<QTreeWidgetItem *> selected = ui->m_tree_widget->selectedItems();
	for (const QTreeWidgetItem *item : selected)
	{
		if (item->data(0, SourceRowRole).isValid()) {
			add_row(item);
		} else {
			for (int i = 0; i < item->childCount(); ++i) {
				add_row(item->child(i));
			}
		}
	}
	return result;
}

QList<MaterialRecord> MaterialSelectionDialog::visibleRecordsForExport() const
{
	QList<MaterialRecord> result;
	QSet<int> seen;

	auto collect = [&](auto &self, const QTreeWidgetItem *item) -> void {
		if (!item) {
			return;
		}
		const QVariant row_var = item->data(0, SourceRowRole);
		if (row_var.isValid())
		{
			const int row = row_var.toInt();
			if (row >= 0 && row < m_data.records.size() && !seen.contains(row)) {
				seen.insert(row);
				result.append(m_data.records.at(row));
			}
		}
		for (int i = 0; i < item->childCount(); ++i) {
			self(self, item->child(i));
		}
	};

	for (int i = 0; i < ui->m_tree_widget->topLevelItemCount(); ++i) {
		collect(collect, ui->m_tree_widget->topLevelItem(i));
	}
	return result;
}

void MaterialSelectionDialog::exportRecordsToCsv(const QList<MaterialRecord> &records)
{
	if (records.isEmpty()) {
		return;
	}

	const QString default_dir = m_path.isEmpty()
		? QFileInfo(MaterialList::defaultPath()).absolutePath()
		: QFileInfo(m_path).absolutePath();
	const QString dest_path = QFileDialog::getSaveFileName(
		this,
		tr("Export Materials List (%n item(s) selected for export)", "", records.size()),
		default_dir + QStringLiteral("/materials_export.csv"),
		tr("CSV files (*.csv);;All files (*)"));
	if (dest_path.isEmpty()) {
		return;
	}

	MaterialListData export_data;
	export_data.columns = effectiveColumns();
	export_data.separator = m_data.separator;
	export_data.records = records;

	QString error;
	if (!MaterialList::writeFile(dest_path, export_data, &error)) {
		QET::QetMessageBox::warning(
			this,
			tr("Export Materials List"),
			error.isEmpty() ? tr("Unable to write the export file.") : error);
		return;
	}

	QET::QetMessageBox::information(
		this,
		tr("Export Materials List"),
		tr("Exported %n entry(s) to %1.", "", records.size()).arg(dest_path));
}

void MaterialSelectionDialog::on_m_export_btn_clicked()
{
	const QList<MaterialRecord> selected_records = selectedRecordsForExport();
	if (!selected_records.isEmpty()) {
		exportRecordsToCsv(selected_records);
		return;
	}

	const QList<MaterialRecord> visible_records = visibleRecordsForExport();

	QMenu menu(this);
	QAction *export_visible = menu.addAction(
		tr("Export Visible (%1)...").arg(visible_records.size()));
	export_visible->setEnabled(!visible_records.isEmpty());

	QAction *export_all = menu.addAction(
		tr("Export All (%1)...").arg(m_data.records.size()));
	export_all->setEnabled(!m_data.records.isEmpty());

	QAction *chosen = menu.exec(
		ui->m_export_btn->mapToGlobal(QPoint(0, ui->m_export_btn->height())));
	if (!chosen) {
		return;
	}
	if (chosen == export_visible) {
		exportRecordsToCsv(visible_records);
	} else if (chosen == export_all) {
		exportRecordsToCsv(m_data.records);
	}
}

void MaterialSelectionDialog::on_m_import_btn_clicked()
{
	const QString default_dir = m_path.isEmpty()
		? QFileInfo(MaterialList::defaultPath()).absolutePath()
		: QFileInfo(m_path).absolutePath();
	const QString source_path = QFileDialog::getOpenFileName(
		this,
		tr("Import Materials List"),
		default_dir,
		tr("CSV files (*.csv);;All files (*)"));
	if (source_path.isEmpty()) {
		return;
	}

	MaterialListData incoming;
	QString error;
	if (!MaterialList::load(source_path, &incoming, &error)) {
		QET::QetMessageBox::warning(
			this,
			tr("Import Materials List"),
			error.isEmpty() ? tr("Unable to read the import file.") : error);
		return;
	}
	if (incoming.records.isEmpty()) {
		QET::QetMessageBox::information(
			this,
			tr("Import Materials List"),
			tr("The selected file contains no entries."));
		return;
	}

	auto policy = MaterialList::ImportConflictPolicy::SkipExisting;
	const int conflicts = MaterialList::countImportConflicts(m_data, incoming);
	if (conflicts > 0)
	{
		QMessageBox box(
			QMessageBox::Question,
			tr("Import Materials List"),
			tr("%n imported entry(s) share an existing item number with different values.", "", conflicts),
			QMessageBox::Cancel,
			this);
		QPushButton *update_btn = box.addButton(tr("Update existing"), QMessageBox::AcceptRole);
		QPushButton *skip_btn = box.addButton(tr("Skip existing"), QMessageBox::RejectRole);
		QPushButton *keep_btn = box.addButton(tr("Keep both"), QMessageBox::ActionRole);
		box.setDefaultButton(update_btn);
		box.exec();

		if (box.clickedButton() == update_btn) {
			policy = MaterialList::ImportConflictPolicy::UpdateExisting;
		} else if (box.clickedButton() == skip_btn) {
			policy = MaterialList::ImportConflictPolicy::SkipExisting;
		} else if (box.clickedButton() == keep_btn) {
			policy = MaterialList::ImportConflictPolicy::KeepBoth;
		} else {
			return;
		}
	}

	MaterialList::ImportResult result;
	if (!MaterialList::importFromFile(m_path, source_path, policy, &result, &error)) {
		QET::QetMessageBox::warning(
			this,
			tr("Import Materials List"),
			error.isEmpty() ? tr("Unable to save the imported entries.") : error);
		return;
	}

	reload();
	QET::QetMessageBox::information(
		this,
		tr("Import Materials List"),
		tr("Import complete.\nAdded: %1\nUpdated: %2\nSkipped: %3")
			.arg(result.added)
			.arg(result.updated)
			.arg(result.skipped));
}

void MaterialSelectionDialog::copyDetailsSelection()
{
	if (auto *line_edit = qobject_cast<QLineEdit *>(QApplication::focusWidget())) {
		line_edit->copy();
		return;
	}

	QModelIndexList selected_rows = ui->m_details_table->selectionModel()->selectedRows();
	if (selected_rows.isEmpty()) {
		return;
	}

	std::sort(selected_rows.begin(), selected_rows.end(),
			  [](const QModelIndex &a, const QModelIndex &b) {
		return a.row() < b.row();
	});

	auto escape_tsv = [](QString text) {
		if (text.contains(QLatin1Char('\t'))
			|| text.contains(QLatin1Char('\n'))
			|| text.contains(QLatin1Char('\r'))
			|| text.contains(QLatin1Char('"')))
		{
			text.replace(QLatin1Char('"'), QStringLiteral("\"\""));
			return QStringLiteral("\"%1\"").arg(text);
		}
		return text;
	};

	QStringList tsv_lines;
	QString html = QStringLiteral("<table><tbody>");
	for (const QModelIndex &idx : std::as_const(selected_rows))
	{
		const int row = idx.row();
		const QTableWidgetItem *field_item = ui->m_details_table->item(row, 0);
		const QTableWidgetItem *value_item = ui->m_details_table->item(row, 1);
		const QString field = field_item ? field_item->text() : QString();
		const QString value = value_item ? value_item->text() : QString();

		tsv_lines.append(escape_tsv(field) + QLatin1Char('\t') + escape_tsv(value));
		html += QStringLiteral("<tr><td>%1</td><td>%2</td></tr>")
					.arg(field.toHtmlEscaped(), value.toHtmlEscaped());
	}
	html += QStringLiteral("</tbody></table>");

	auto *mime = new QMimeData();
	mime->setText(tsv_lines.join(QLatin1Char('\n')));
	mime->setHtml(html);
	QApplication::clipboard()->setMimeData(mime);
}

void MaterialSelectionDialog::showDetailsContextMenu(const QPoint &pos)
{
	QMenu menu(this);
	QAction *copy_action = menu.addAction(tr("Copy"), QKeySequence::Copy,
										  this, &MaterialSelectionDialog::copyDetailsSelection);
	copy_action->setEnabled(!ui->m_details_table->selectionModel()->selectedRows().isEmpty());

	QAction *select_all_action = menu.addAction(tr("Select All"), QKeySequence::SelectAll,
												ui->m_details_table, &QTableWidget::selectAll);
	select_all_action->setEnabled(ui->m_details_table->rowCount() > 0);

	menu.exec(ui->m_details_table->viewport()->mapToGlobal(pos));
}

void MaterialSelectionDialog::on_m_duplicate_entry_btn_clicked()
{
	const int row = selectedSourceRow();
	if (row < 0 || row >= m_data.records.size()) {
		return;
	}

	MaterialRecord copy = m_data.records.at(row);
	const QString new_designation = MaterialList::nextDuplicateDesignation(
		m_data.records,
		copy.value(QStringLiteral("designation")));
	copy.setValue(QStringLiteral("designation"), new_designation);

	QString error;
	if (!MaterialList::appendRecord(m_path, copy, &error)) {
		QET::QetMessageBox::warning(
			this,
			tr("Duplicate material entry"),
			error.isEmpty() ? tr("Unable to duplicate selected entry in the material list.") : error);
		return;
	}

	reload();
	selectRecord(copy);
}

void MaterialSelectionDialog::on_m_remove_entry_btn_clicked()
{
	const QList<int> rows = selectedSourceRows();
	if (rows.isEmpty()) {
		return;
	}

	QString prompt;
	if (rows.size() == 1) {
		const MaterialRecord &rec = m_data.records.at(rows.first());
		const QString article = rec.value(QStringLiteral("designation"));
		const QString desc = rec.value(QStringLiteral("description"));
		const QString label = article.isEmpty() ? desc : article;
		prompt = tr("Are you sure you want to remove \"%1\" from the material list?")
					 .arg(label.isEmpty() ? tr("selected item") : label);
	} else {
		prompt = tr("Are you sure you want to remove %1 selected items from the material list?")
					 .arg(rows.size());
	}

	const QMessageBox::StandardButton answer = QET::QetMessageBox::question(
		this,
		tr("Remove material entry"),
		prompt,
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No);
	if (answer != QMessageBox::Yes) {
		return;
	}

	QList<MaterialRecord> originals;
	originals.reserve(rows.size());
	for (int row : rows) {
		originals.append(m_data.records.at(row));
	}

	QString error;
	if (!MaterialList::removeRecords(m_path, rows, originals, &error)) {
		QET::QetMessageBox::warning(
			this,
			tr("Remove material entry"),
			error.isEmpty() ? tr("Unable to remove selected entry from the material list.") : error);
		return;
	}

	reload();
}

void MaterialSelectionDialog::showTreeContextMenu(const QPoint &pos)
{
	if (m_edit_mode != EditMode::View) {
		return;
	}

	QMenu menu(this);
	QAction *new_action = menu.addAction(QET::Icons::Add, tr("New"),
										 this, &MaterialSelectionDialog::on_m_new_entry_btn_clicked);
	new_action->setEnabled(true);

	QAction *edit_action = menu.addAction(QET::Icons::ElementEdit, tr("Edit"),
										  this, &MaterialSelectionDialog::on_m_edit_entry_btn_clicked);
	edit_action->setEnabled(selectedSourceRow() >= 0);

	QAction *duplicate_action = menu.addAction(QET::Icons::EditCopy, tr("Duplicate"),
											   this, &MaterialSelectionDialog::on_m_duplicate_entry_btn_clicked);
	duplicate_action->setEnabled(selectedSourceRow() >= 0);

	QAction *remove_action = menu.addAction(QET::Icons::EditDelete, tr("Remove"),
											this, &MaterialSelectionDialog::on_m_remove_entry_btn_clicked);
	remove_action->setEnabled(!selectedSourceRows().isEmpty());

	menu.addSeparator();
	QAction *export_action = menu.addAction(QET::Icons::DocumentExport, tr("Export..."),
											this, &MaterialSelectionDialog::on_m_export_btn_clicked);
	export_action->setEnabled(!m_data.records.isEmpty());

	menu.exec(ui->m_tree_widget->viewport()->mapToGlobal(pos));
}
