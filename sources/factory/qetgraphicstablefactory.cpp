/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "qetgraphicstablefactory.h"
#include "qetgraphicstableitem.h"
#include "projectdbmodel.h"
#include "elementquerywidget.h"
#include "diagram.h"
#include "qetgraphicsheaderitem.h"
#include "addtabledialog.h"
#include "qetutils.h"
#include "elementquerywidget.h"
#include "summaryquerywidget.h"

#include <QDialog>

QetGraphicsTableFactory::QetGraphicsTableFactory()
{

}

/**
	@brief QetGraphicsTableFactory::createAndAddNomenclature
	Open a dialog for ask user the config of the table,
	create a nomenclature table and add it to diagram
	@param diagram
*/
void QetGraphicsTableFactory::createAndAddNomenclature(Diagram *diagram)
{
	QScopedPointer<AddTableDialog> d(
				new AddTableDialog(
					new ElementQueryWidget(),
					diagram->views().first()));
	d->setWindowTitle(QObject::tr("Ajouter une nomenclature"));

	if (d->exec()) {
		create(diagram, d.data());
	}
}

/**
	@brief QetGraphicsTableFactory::createAndAddSummary
	Open a dialog for ask user the config of the table,
	create a summary table and add it to diagram
	@param diagram
*/
void QetGraphicsTableFactory::createAndAddSummary(Diagram *diagram)
{
	QScopedPointer<AddTableDialog> d(
				new AddTableDialog(
					new SummaryQueryWidget(),
					diagram->views().first()));
	d->setWindowTitle(QObject::tr("Ajouter un sommaire"));

	if (d->exec()) {
		create(diagram, d.data());
	}
}

void QetGraphicsTableFactory::create(Diagram *diagram, AddTableDialog *dialog)
{
	auto table_ = newTable(diagram, dialog);
	if (dialog->adjustTableToFolio()) {
		QetGraphicsTableItem::adjustTableToFolio(table_);
	}

		//Add new table if needed and option checked
	dialog->addNewTableToNewDiagram();
	table_->model()->rowCount();
	table_->displayNRow();
	if (dialog->addNewTableToNewDiagram() && table_->model()->rowCount() > table_->displayNRow())
	{
		auto already_displayed_rows = table_->displayNRow();
		auto project_ = diagram->project();
		auto actual_diagram = diagram;
		auto previous_table = table_;

		table_->setTableName(dialog->tableName() + QString(" 1"));
		int table_number = 2;
		while (already_displayed_rows < table_->model()->rowCount())
		{
				//Add a new diagram after the current one
			actual_diagram = project_->addNewDiagram(project_->folioIndex(actual_diagram)+1);
			table_ = newTable(actual_diagram, dialog, previous_table);
			table_->setTableName(dialog->tableName() + QString(" %1").arg(table_number));
				//Adjust table
			if (dialog->adjustTableToFolio()) {
				QetGraphicsTableItem::adjustTableToFolio(table_);
			}
				//Update some variable for the next loop
			already_displayed_rows += table_->displayNRow();
			previous_table = table_;
			++table_number;
		}
	}
}

/**
	@brief QetGraphicsTableFactory::newTable
	Create a new table .
	@param diagram : Diagram where we must add the new table.
	@param dialog : dialog conf, it's used to setup the model.
	@param previous_table : If you know that the new table will have a previous table and you already now the previous table,
	set it now they will improve time needed for creating the new table by avoiding to create a new model.
	@return the new table
*/
QetGraphicsTableItem *QetGraphicsTableFactory::newTable(Diagram *diagram, AddTableDialog *dialog, QetGraphicsTableItem *previous_table)
{
	auto table = new QetGraphicsTableItem();
	table->setTableName(dialog->tableName());

	if (!previous_table)
	{
		QString identifier_;
		QString query_;

		if (auto query_widget = dynamic_cast<ElementQueryWidget *>(dialog->contentWidget())) {
			identifier_ = query_widget->modelIdentifier();
			query_ = query_widget->queryStr();
		} else if (auto query_widget = dynamic_cast<SummaryQueryWidget *>(dialog->contentWidget())) {
			identifier_ = query_widget->modelIdentifier();
			query_ = query_widget->queryStr();
		}

		auto model = new ProjectDBModel(diagram->project(), diagram->project());
		model->setIdentifier(identifier_);
		model->setQuery(query_);
		model->setData(model->index(0,0), int(dialog->tableAlignment()), Qt::TextAlignmentRole);
		model->setData(model->index(0,0), dialog->tableFont(), Qt::FontRole);
		model->setData(model->index(0,0), QETUtils::marginsToString(dialog->headerMargins()), Qt::UserRole+1);
		model->setHeaderData(0, Qt::Horizontal, int(dialog->headerAlignment()), Qt::TextAlignmentRole);
		model->setHeaderData(0, Qt::Horizontal, dialog->headerFont(), Qt::FontRole);
		model->setHeaderData(0, Qt::Horizontal, QETUtils::marginsToString(dialog->headerMargins()), Qt::UserRole+1);
		table->setModel(model);
	}
	else {
		table->setPreviousTable(previous_table);
	}

	diagram->addItem(table);
	table->setPos(50,50);

	return table;
}
