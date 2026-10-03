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
#include "wiringruleslistdialog.h"

#include "../dataBase/projectdatabase.h"
#include "../qetproject.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QSqlQuery>
#include <QTableWidget>
#include <QVBoxLayout>

/**
	@brief WiringRulesListDialog::WiringRulesListDialog
	@param project : the project to check
	@param rules : the rules to check against; the ones shown in the
	project properties, which may not be applied yet
	@param parent
*/
WiringRulesListDialog::WiringRulesListDialog(QETProject *project,
											 const WiringRules::Settings &rules,
											 QWidget *parent) :
	QDialog(parent)
{
	setWindowTitle(tr("Bornes au-delà de la limite", "window title"));

	auto table = new QTableWidget(0, 6, this);
	table->setHorizontalHeaderLabels({tr("Folio"),
									  tr("Position"),
									  tr("Élément"),
									  tr("Borne"),
									  tr("Conducteurs"),
									  tr("Limite")});
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->setSelectionBehavior(QAbstractItemView::SelectRows);
	table->verticalHeader()->hide();
	table->horizontalHeader()->setStretchLastSection(true);

	if (project && project->dataBase())
	{
		QSqlQuery query = project->dataBase()->newQuery(QStringLiteral(
				"SELECT folio, pos, element_label, element_type, terminal_name, "
				"terminal_index, wires FROM terminal_wires_view "
				"ORDER BY folio, pos, element_label, terminal_index"));
		if (query.exec())
		{
			while (query.next())
			{
				const int wires = query.value(6).toInt();
				const int limit = WiringRules::limit(rules, true,
													 WiringRules::isReportType(query.value(3).toString()));
				if (limit <= 0 || wires <= limit) {
					continue;
				}
				const QString terminal = query.value(4).toString().isEmpty()
						? QString::number(query.value(5).toInt() + 1)
						: query.value(4).toString();
				const int row = table->rowCount();
				table->insertRow(row);
				table->setItem(row, 0, new QTableWidgetItem(query.value(0).toString()));
				table->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
				table->setItem(row, 2, new QTableWidgetItem(query.value(2).toString()));
				table->setItem(row, 3, new QTableWidgetItem(terminal));
				table->setItem(row, 4, new QTableWidgetItem(QString::number(wires)));
				table->setItem(row, 5, new QTableWidgetItem(QString::number(limit)));
			}
		}
	}
	table->resizeColumnsToContents();

	auto summary = new QLabel(table->rowCount()
							  ? tr("%n borne(s) ont plus de conducteurs que la limite. "
								   "Les conducteurs déjà dessinés ne sont jamais retirés : "
								   "ajoutez une borne pour en déplacer.",
								   "wires-per-terminal check", table->rowCount())
							  : tr("Aucune borne n'a plus de conducteurs que la limite.",
								   "wires-per-terminal check"),
							  this);
	summary->setWordWrap(true);

	auto buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto layout = new QVBoxLayout(this);
	layout->addWidget(summary);
	layout->addWidget(table);
	layout->addWidget(buttons);
	resize(640, 420);
}
