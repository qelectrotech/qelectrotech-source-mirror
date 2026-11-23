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
#include "parttypesdialog.h"

#include "../devicetype/devicetype.h"
#include "../qetproject.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTableWidget>
#include <QUndoCommand>
#include <QUndoStack>
#include <QVBoxLayout>

namespace
{
		/// The device type cell: the known types, by list or by typing
	class TypeDelegate : public QStyledItemDelegate
	{
		public:
			TypeDelegate(const QStringList &types, QObject *parent) :
				QStyledItemDelegate(parent), m_types(types) {}

			QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &,
								  const QModelIndex &) const override
			{
				auto *combo = new QComboBox(parent);
				combo->setEditable(true);
				combo->setInsertPolicy(QComboBox::NoInsert);
				combo->addItem(QString());
				combo->addItems(m_types);
				combo->completer()->setCompletionMode(QCompleter::PopupCompletion);
				combo->completer()->setFilterMode(Qt::MatchContains);
				combo->completer()->setCaseSensitivity(Qt::CaseInsensitive);
				return combo;
			}

			void setEditorData(QWidget *editor, const QModelIndex &index) const override
			{
				static_cast<QComboBox *>(editor)->setCurrentText(index.data().toString());
			}

			void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
			{
				model->setData(index, static_cast<QComboBox *>(editor)->currentText().trimmed());
			}

		private:
			QStringList m_types;
	};
}

PartTypesDialog::PartTypesDialog(QETProject *project, const QString &material_path, QWidget *parent) :
	QDialog(parent),
	m_project(project),
	m_path(material_path)
{
	setWindowTitle(tr("Parts without a device type"));
	resize(900, 620);

	QString error;
	const bool read = QFileInfo::exists(m_path) && MaterialList::load(m_path, &m_data, &error);

	m_known_types = DeviceTypeLibrary::names();
	if (m_project)
		for (const DeviceType &type : m_project->deviceTypes())
			if (!m_known_types.contains(type.name)) m_known_types << type.name;
	m_known_types.sort(Qt::CaseInsensitive);

	m_rows = PartTypes::partsWithoutType(m_data, PartTypes::usageIn(m_project.data()));

	auto *layout = new QVBoxLayout(this);
	m_summary = new QLabel(this);
	m_summary->setObjectName(QStringLiteral("summary"));
	m_summary->setWordWrap(true);
	layout->addWidget(m_summary);

	auto *top = new QHBoxLayout;
	m_filter = new QLineEdit(this);
	m_filter->setPlaceholderText(tr("Search part, description or manufacturer..."));
	m_only_used = new QCheckBox(tr("Only the parts this project uses"), this);
	m_suggestions = new QPushButton(tr("Use the suggestions"), this);
	m_suggestions->setToolTip(tr("Takes the device type of the parts whose devices in this project all have the same one."));
	top->addWidget(m_filter, 1);
	top->addWidget(m_only_used);
	top->addWidget(m_suggestions);
	layout->addLayout(top);

	m_table = new QTableWidget(this);
	m_table->setItemDelegateForColumn(Type, new TypeDelegate(m_known_types, m_table));
	layout->addWidget(m_table, 1);
	fillTable();

	m_give_to_symbols = new QCheckBox(tr("Also give each type to the symbols of this project with that part and no device type"), this);
	m_give_to_symbols->setChecked(m_project && !m_project->isReadOnly());
	m_give_to_symbols->setEnabled(m_project && !m_project->isReadOnly());
	layout->addWidget(m_give_to_symbols);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
	layout->addWidget(buttons);
	connect(buttons, &QDialogButtonBox::accepted, this, &PartTypesDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_filter, &QLineEdit::textChanged, this, &PartTypesDialog::applyFilter);
	connect(m_only_used, &QCheckBox::toggled, this, &PartTypesDialog::applyFilter);
	connect(m_suggestions, &QPushButton::clicked, this, &PartTypesDialog::useSuggestions);

	const bool any_used = std::any_of(m_rows.cbegin(), m_rows.cend(),
									  [](const PartTypes::Row &row) {return row.used > 0;});
	m_only_used->setEnabled(any_used);
	m_only_used->setChecked(any_used);
	m_suggestions->setEnabled(std::any_of(m_rows.cbegin(), m_rows.cend(),
										  [](const PartTypes::Row &row) {return row.certain;}));
	applyFilter();

	if (!read) {
		m_summary->setText(error.isEmpty() ? tr("The material list %1 does not exist yet.").arg(m_path) : error);
		buttons->button(QDialogButtonBox::Save)->setEnabled(false);
	}
}

void PartTypesDialog::fillTable()
{
	m_table->setColumnCount(ColumnCount);
	m_table->setHorizontalHeaderLabels({tr("Part number"), tr("Description"), tr("Manufacturer"),
										tr("Devices here"), tr("Suggested type"), tr("Device type")});
	m_table->setRowCount(int(m_rows.size()));
	for (int r = 0 ; r < m_rows.size() ; ++r) {
		const PartTypes::Row &row = m_rows.at(r);
		const QStringList cells{row.part, row.description, row.manufacturer,
								row.used ? QString::number(row.used) : QString(),
								row.certain || row.suggestion.isEmpty()
								? row.suggestion
								: tr("%1 (others too)", "a suggested device type some devices do not share").arg(row.suggestion)};
		for (int c = 0 ; c < cells.size() ; ++c) {
			auto *item = new QTableWidgetItem(cells.at(c));
			item->setFlags(item->flags() & ~Qt::ItemIsEditable);
			m_table->setItem(r, c, item);
		}
		m_table->setItem(r, Type, new QTableWidgetItem);
	}
	m_table->resizeColumnsToContents();
	m_table->horizontalHeader()->setSectionResizeMode(Description, QHeaderView::Stretch);
}

void PartTypesDialog::applyFilter()
{
	const QString text = m_filter->text().trimmed();
	int shown = 0;
	for (int r = 0 ; r < m_rows.size() ; ++r) {
		const PartTypes::Row &row = m_rows.at(r);
		const bool hit = (!m_only_used->isChecked() || row.used > 0)
				&& (text.isEmpty() || row.part.contains(text, Qt::CaseInsensitive)
					|| row.description.contains(text, Qt::CaseInsensitive)
					|| row.manufacturer.contains(text, Qt::CaseInsensitive));
		m_table->setRowHidden(r, !hit);
		if (hit) ++shown;
	}
	m_summary->setText(tr("%n part(s) of the material list have no device type:", nullptr, int(m_rows.size()))
					   + QLatin1Char(' ') + tr("%n shown.", nullptr, shown));
}

/**
	@return how many rows took their suggestion: the rows shown, not yet
	filled in, whose devices all have one type
*/
int PartTypesDialog::useSuggestions()
{
	int count = 0;
	for (int r = 0 ; r < m_rows.size() ; ++r) {
		if (!m_rows.at(r).certain || m_table->isRowHidden(r)
				|| !m_table->item(r, Type)->text().isEmpty())
			continue;
		m_table->item(r, Type)->setText(m_rows.at(r).suggestion);
		++count;
	}
	return count;
}

void PartTypesDialog::accept()
{
	QHash<int, QString> types;
	QHash<QString, QString> part_types;
	QStringList unknown;
	for (int r = 0 ; r < m_rows.size() ; ++r) {
		const QString type = m_table->item(r, Type)->text().trimmed();
		if (type.isEmpty()) continue;
		if (!m_known_types.contains(type)) {
			unknown << type;
			continue;
		}
		types.insert(m_rows.at(r).record, type);
		part_types.insert(m_rows.at(r).part, type);
	}
	if (!unknown.isEmpty()) {
		unknown.removeDuplicates();
		QMessageBox::warning(this, windowTitle(),
							 tr("There is no device type called: %1").arg(unknown.mid(0, 10).join(QStringLiteral(", "))));
		return;
	}
	if (types.isEmpty()) {
		QDialog::reject();
		return;
	}
	QString error;
	if (!PartTypes::saveTypes(m_path, m_data, types, &error)) {
		QMessageBox::warning(this, windowTitle(), error);
		return;
	}
	int symbols = 0;
	if (m_project && m_give_to_symbols->isChecked()) {
		if (QUndoCommand *undo = PartTypes::giveToSymbols(m_project.data(), part_types, &symbols))
			m_project->undoStack()->push(undo);
	}
	QDialog::accept();
}
