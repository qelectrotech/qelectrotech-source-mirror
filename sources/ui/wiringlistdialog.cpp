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
#include "wiringlistdialog.h"
#include <qet.h>

#include <dataBase/projectdatabase.h>
#include <qetproject.h>
#include <wiringlistdb.h>

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QStandardItemModel>
#include <QTableView>
#include <QVBoxLayout>

/**
	@brief WiringListDialog::WiringListDialog
	@param project : project whose wiring list is shown
	@param parent : parent widget
*/
WiringListDialog::WiringListDialog(QETProject *project, QWidget *parent) :
	QDialog(parent),
	m_project(project)
{
	setWindowTitle(tr("Wiring list", "window title"));
	resize(900, 500);

	auto *layout = new QVBoxLayout(this);

		//The wiring list reads the database rather than the diagrams, and a
		//conductor's row is only as fresh as the last thing that touched it.
		//wiringListRows() refreshes before querying, so the dialog cannot
		//show a wire number that was edited earlier in the session. A wire
		//ending on a junction names where the junction's chain leads rather
		//than the junction itself, the way the CSV export names it (#1500).
	int unassigned_junctions = 0;
	QString error;
	const QVector<WiringListRow> rows =
		wiringListRows(m_project, &unassigned_junctions, &error);

	auto *model = new QStandardItemModel(int(rows.size()), 6, this);
	model->setHorizontalHeaderLabels({tr("Wire", "column title"),
					  tr("Component 1", "column title"),
					  tr("Terminal 1", "column title"),
					  tr("Component 2", "column title"),
					  tr("Terminal 2", "column title"),
					  tr("Sheet", "column title")});
	for (int r = 0 ; r < rows.size() ; ++r)
	{
		const WiringListRow &row = rows.at(r);
		const QStringList cells{row.wire_number,
					row.from_element_label,
					row.from_terminal,
					row.to_element_label,
					row.to_terminal,
					row.diagram_position};
		for (int c = 0 ; c < cells.size() ; ++c) {
			model->setItem(r, c, new QStandardItem(cells.at(c)));
		}
	}
	const int listed = rows.size();

	const int excluded = m_project->dataBase()->excludedConductorCount();

	auto *summary = new QLabel(this);
	summary->setWordWrap(true);
	QStringList said;
	if (!error.isEmpty())
	{
			//An empty list which says nothing would read as a project
			//without a single wire rather than as a list which could
			//not be read
		said << tr("The wiring list could not be read: %1",
			   "wiring list read error").arg(error);
	}
	said << tr("%n conductors listed.", "wiring list summary", listed);
	if (excluded > 0)
	{
			//Rare now that Terminal::stableUuid() gives every terminal an
			//identity: what is left is a conductor whose endpoint has no
			//parent element at all. Still worth saying out loud rather than
			//presenting a short list as if it were complete.
		said << tr("%n conductors excluded: one end is not attached to any "
			   "element.",
			   "wiring list exclusion warning", excluded);
	}
	if (unassigned_junctions > 0)
	{
			//Those rows end at the junction, because nothing says which
			//way the wire runs through it: said rather than listed as if
			//the way through were known (#1500)
		said << tr("%n junction(s) have no source connection set, so their rows "
			   "still end at the junction.",
			   "wiring list junction warning", unassigned_junctions);
	}
	summary->setText(said.join(QLatin1Char(' ')));
	layout->addWidget(summary);

	auto *view = new QTableView(this);
	view->setModel(model);
	view->setEditTriggers(QAbstractItemView::NoEditTriggers);
	view->setSelectionBehavior(QAbstractItemView::SelectRows);
	view->setAlternatingRowColors(true);
	view->verticalHeader()->setVisible(false);
	view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	layout->addWidget(view);

	auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	layout->addWidget(buttons);

	QET::trackDialogGeometry(this);
}
