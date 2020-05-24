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
#include "nomenclaturemodel.h"
#include "elementquerywidget.h"
#include "diagram.h"
#include "qetgraphicsheaderitem.h"
#include "addtabledialog.h"
#include "qetutils.h"

#include <QDialog>

QetGraphicsTableFactory::QetGraphicsTableFactory()
{

}

/**
 * @brief QetGraphicsTableFactory::createAndAddNomenclature
 * Create a nomenclature table, open a dialog for ask to user the config of the table,
 * and add it to diagram @diagram;
 * @param diagram
 */
void QetGraphicsTableFactory::createAndAddNomenclature(Diagram *diagram)
{
	QScopedPointer<AddTableDialog> d(new AddTableDialog(diagram->views().first()));
	d->setWindowTitle(QObject::tr("Ajouter une nomenclature"));

	if (d->exec())
	{
		auto table_ = newTable(diagram, d.data());
		if (d->adjustTableToFolio()) {
			AdjustTableToFolio(table_);
		}


		if (d->addNewTableToNewDiagram() && table_->model()->rowCount() > table_->displayNRow())
		{
			auto already_displayed_rows = table_->displayNRow();
			auto project_ = diagram->project();
			auto actual_diagram = diagram;
			auto previous_table = table_;

			int i = 2;
			while (already_displayed_rows < table_->model()->rowCount())
			{
					//Add a new diagram after the current one
				actual_diagram = project_->addNewDiagram(project_->folioIndex(actual_diagram)+1);
				table_ = newTable(actual_diagram, d.data());
				table_->setTableName(d->tableName() + QString(" %1").arg(i));
					//Set the previous folio of table
				table_->setPreviousTable(previous_table);
					//Adjust table
				if (d->adjustTableToFolio()) {
					AdjustTableToFolio(table_);
				}
					//Update some variable for the next loop
				already_displayed_rows += table_->displayNRow();
				previous_table = table_;
				++i;
			}
		}
	}
}

QetGraphicsTableItem *QetGraphicsTableFactory::newTable(Diagram *diagram, AddTableDialog *dialog)
{
	auto model = new NomenclatureModel(diagram->project(), diagram->project());
	model->query(dialog->queryStr());
	model->autoHeaders();
	model->setData(model->index(0,0), int(dialog->tableAlignment()), Qt::TextAlignmentRole);
	model->setData(model->index(0,0), dialog->tableFont(), Qt::FontRole);
	model->setData(model->index(0,0), QETUtils::marginsToString(dialog->headerMargins()), Qt::UserRole+1);
	model->setHeaderData(0, Qt::Horizontal, int(dialog->headerAlignment()), Qt::TextAlignmentRole);
	model->setHeaderData(0, Qt::Horizontal, dialog->headerFont(), Qt::FontRole);
	model->setHeaderData(0, Qt::Horizontal, QETUtils::marginsToString(dialog->headerMargins()), Qt::UserRole+1);

	auto table = new QetGraphicsTableItem();
	table->setTableName(dialog->tableName());
	table->setModel(model);
	diagram->addItem(table);
	table->setPos(50,50);

	return table;
}

void QetGraphicsTableFactory::AdjustTableToFolio(QetGraphicsTableItem *table)
{
	auto drawable_rect = table->diagram()->border_and_titleblock.insideBorderRect();
	table->setPos(drawable_rect.topLeft().x() + 20, drawable_rect.topLeft().y() + 20 + table->headerItem()->rect().height());

	auto size_ = table->size();
	size_.setWidth(int(drawable_rect.width() - 40));
		//Size must be a multiple of 10, because the table adjust itself by step of 10.
	while (size_.width()%10) {
		--size_.rwidth();    }
	table->setSize(size_);

		//Calcul the maximum row to display to fit the nomenclature into diagram
	auto available_height = drawable_rect.height() - table->pos().y();
	auto min_row_height = table->minimumRowHeigth();
	table->setDisplayNRow(int(floor(available_height/min_row_height))); //Convert a double to int, but max_row_to_display is already rounded an integer so we assume everything is ok
}
