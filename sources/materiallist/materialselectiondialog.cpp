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
#include <qetinformation.h>
#include <qetmessagebox.h>

#include <QComboBox>
#include <QFont>
#include <QHeaderView>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTableWidgetItem>
#include <QTreeWidgetItem>

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

	ui->m_details_table->setItemDelegateForColumn(1, new MaterialValueDelegate(ui->m_details_table));
	ui->m_details_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
	ui->m_details_table->horizontalHeader()->setStretchLastSection(true);

	ui->m_edit_bar->setVisible(false);

	ui->m_group_combo->addItem(tr("None"), QString());
	ui->m_group_combo->addItem(tr("Manufacturer"), QStringLiteral("manufacturer"));
	ui->m_group_combo->addItem(tr("Category"), QStringLiteral("category"));
	ui->m_group_combo->setCurrentIndex(1);

	ui->m_filter_field_combo->addItem(tr("Off"), QString());
	ui->m_filter_field_combo->addItem(tr("Manufacturer"), QStringLiteral("manufacturer"));
	ui->m_filter_field_combo->addItem(tr("Category"), QStringLiteral("category"));

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
	return m_data.columns.isEmpty() ? MaterialList::defaultColumns() : m_data.columns;
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

	const QString current_filter_val = ui->m_filter_value_combo->currentData().toString();
	m_data = data;

	populateFilterValues(current_filter_val);
	rebuildTree();
	populateDetails(selectedRecord());
	updateButtons();
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
		}
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
	}
}

void MaterialSelectionDialog::setEditMode(EditMode mode)
{
	m_edit_mode = mode;
	const bool editing = (mode != EditMode::View);

	ui->m_edit_bar->setVisible(editing);
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
		value.remove(QLatin1Char('\n'));
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

	if (QPushButton *ok_btn = ui->m_button_box->button(QDialogButtonBox::Ok)) {
		ok_btn->setEnabled(in_view && has_selection);
	}
	ui->m_new_entry_btn->setEnabled(in_view);
	ui->m_edit_entry_btn->setEnabled(in_view && has_selection);
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
	const int row = item->data(0, SourceRowRole).toInt();
	if (row >= 0) {
		accept();
	}
}

int MaterialSelectionDialog::selectedSourceRow() const
{
	const QList<QTreeWidgetItem *> items = ui->m_tree_widget->selectedItems();
	if (items.isEmpty()) {
		return -1;
	}
	const int row = items.first()->data(0, SourceRowRole).toInt();
	return (row >= 0 && row < m_data.records.size()) ? row : -1;
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
		app->configureQET();
	}

	const QString configured = MaterialList::configuredPath();
	if (!configured.isEmpty()) {
		m_path = configured;
	}
	reload();
}
