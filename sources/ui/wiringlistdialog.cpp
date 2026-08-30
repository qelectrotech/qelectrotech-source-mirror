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

#include "../dataBase/projectdatabase.h"
#include "../qetproject.h"

#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QSqlQueryModel>
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
	setWindowTitle(tr("Liste de câblage", "window title"));
	resize(900, 500);

	auto *layout = new QVBoxLayout(this);

		//The wiring list reads the database rather than the diagrams, and a
		//conductor's row is only as fresh as the last thing that touched it.
		//Refresh before querying so the dialog cannot show a wire number that
		//was edited earlier in the session.
	m_project->dataBase()->updateDB();

	auto *model = new QSqlQueryModel(this);
	model->setQuery(QStringLiteral(
				"SELECT wire_number, from_element_label, from_terminal,"
				" to_element_label, to_terminal, diagram_position"
				" FROM wiring_list_view"
				" ORDER BY diagram_position, wire_number"),
			m_project->dataBase()->database());

	model->setHeaderData(0, Qt::Horizontal, tr("Fil", "column title"));
	model->setHeaderData(1, Qt::Horizontal, tr("Composant 1", "column title"));
	model->setHeaderData(2, Qt::Horizontal, tr("Borne 1", "column title"));
	model->setHeaderData(3, Qt::Horizontal, tr("Composant 2", "column title"));
	model->setHeaderData(4, Qt::Horizontal, tr("Borne 2", "column title"));
	model->setHeaderData(5, Qt::Horizontal, tr("Folio", "column title"));

	const int excluded = m_project->dataBase()->excludedConductorCount();

		//QSqlQueryModel fetches lazily, so rowCount() straight after
		//setQuery() reports the first batch (256) rather than the query's
		//size. Draining it first is what makes the count below true for a
		//project with more wires than that.
	while (model->canFetchMore()) {
		model->fetchMore();
	}
	const int listed = model->rowCount();

	auto *summary = new QLabel(this);
	summary->setWordWrap(true);
	if (excluded > 0)
	{
			//Rare now that Terminal::stableUuid() gives every terminal an
			//identity: what is left is a conductor whose endpoint has no
			//parent element at all. Still worth saying out loud rather than
			//presenting a short list as if it were complete.
		summary->setText(tr("%n conducteur(s) listé(s).", "wiring list summary", listed)
				 % QStringLiteral(" ")
				 % tr("%n conducteur(s) exclu(s) : une extrémité n'est rattachée"
				      " à aucun élément.",
				      "wiring list exclusion warning", excluded));
	}
	else {
		summary->setText(tr("%n conducteur(s) listé(s).", "wiring list summary", listed));
	}
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
}
