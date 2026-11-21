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
#include "plantslocationsdialog.h"

#include "../reports/projectreports.h"

#include <qet.h>
#include <qetproject.h>

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

namespace
{
	enum Column {PlantColumn, LocationColumn, DesignationColumn,
				 SymbolsColumn, FoliosColumn, FirstFolioColumn};
}

PlantsLocationsDialog::PlantsLocationsDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	m_project(project),
	m_summary(new QLabel(this)),
	m_table(new QTableWidget(this))
{
	setWindowTitle(tr("Plants and locations"));
	resize(760, 460);
	auto *layout = new QVBoxLayout(this);

	m_summary->setObjectName(QStringLiteral("placesSummary"));
	layout->addWidget(m_summary);

	m_table->setObjectName(QStringLiteral("placesTable"));
	m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_table->setAlternatingRowColors(true);
	m_table->verticalHeader()->setVisible(false);
	m_table->setToolTip(tr("Double-click a row to go to its first folio"));
	layout->addWidget(m_table);
	connect(m_table, &QTableWidget::itemDoubleClicked, this, &PlantsLocationsDialog::rowActivated);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	auto *refresh_button = buttons->addButton(tr("Refresh"), QDialogButtonBox::ActionRole);
	connect(refresh_button, &QPushButton::clicked, this, &PlantsLocationsDialog::refresh);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);
	if (project) connect(project, &QObject::destroyed, this, &QDialog::close);
	QET::trackDialogGeometry(this);

	refresh();
}

/**
	@brief PlantsLocationsDialog::refresh
	Reads the project database again.
*/
void PlantsLocationsDialog::refresh()
{
	const ProjectReports::Table shown = ProjectReports::locations(m_project);
	m_table->clear();
	m_table->setColumnCount(int(shown.headers.size()));
	m_table->setRowCount(int(shown.rows.size()));
	m_table->setHorizontalHeaderLabels(shown.headers);
	for (int r = 0 ; r < shown.rows.size() ; ++r) {
		const QStringList &row = shown.rows.at(r);
		for (int c = 0 ; c < row.size() ; ++c) {
			QString text = row.at(c);
			if ((c == PlantColumn || c == LocationColumn) && text.trimmed().isEmpty())
				text = tr("(none)");
			auto *item = new QTableWidgetItem(text);
			if (c == SymbolsColumn || c == FoliosColumn || c == FirstFolioColumn) {
				item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
			}
			if (c == PlantColumn)
				item->setData(Qt::UserRole, row.at(FirstFolioColumn).toInt());
			m_table->setItem(r, c, item);
		}
	}
	m_table->resizeColumnsToContents();
	m_table->horizontalHeader()->setSectionResizeMode(DesignationColumn, QHeaderView::Stretch);
	if (shown.error.isEmpty())
		m_summary->setText(tr("%n plant and location row(s).", nullptr, int(shown.rows.size())));
	else
		m_summary->setText(shown.error);
}

void PlantsLocationsDialog::rowActivated(QTableWidgetItem *item)
{
	if (!item) return;
	const QTableWidgetItem *first = m_table->item(item->row(), PlantColumn);
	const int position = first ? first->data(Qt::UserRole).toInt() : 0;
	if (position > 0)
		emit showFolio(position);
}
