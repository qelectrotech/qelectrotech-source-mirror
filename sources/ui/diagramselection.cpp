/*
		Copyright 2006-2014 The QElectroTech Team
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
#include "diagramselection.h"
#include "ui_diagramselection.h"

diagramselection::diagramselection(QETProject *prj, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::diagramselection),
	prj_ (prj),
	list_diagram_ (prj -> diagrams())
{
	ui -> setupUi(this);

	QString project_title = prj_ -> title();
	if (project_title.isEmpty()) project_title = tr("Projet sans titre");

	ui -> label_prj -> setText( tr("Projet : ") + project_title );
	load_TableDiagram();
}

diagramselection::~diagramselection() {
	delete ui;
}

/**
 * @brief load all Diagrams of project in table
 */
void diagramselection::load_TableDiagram() {
	// Clear all items
	ui -> tableDiagram -> clear();
	for (int i=ui -> tableDiagram -> rowCount()-1; i >= 0; --i) {
		ui -> tableDiagram->removeRow(i);
	}
	for (int i=ui -> tableDiagram -> columnCount()-1; i>=0; --i) {
		ui -> tableDiagram->removeColumn(i);
	}

	// Set the setting of table
	ui -> tableDiagram -> setColumnCount(2);
	ui -> tableDiagram -> setSelectionBehavior	(QAbstractItemView::SelectRows);
	ui -> tableDiagram -> setSelectionMode		(QAbstractItemView::SingleSelection);
	ui -> tableDiagram -> setEditTriggers		(QAbstractItemView::NoEditTriggers);
	QStringList titles;
	titles.clear();
	titles << tr("S\351lection") << tr("Nom");
	ui-> tableDiagram -> setHorizontalHeaderLabels( titles );
	
	// List Diagrams
	for(int i=0,j=0; i<list_diagram_.count(); i++,j++){
		QTableWidgetItem *item_Name  = new QTableWidgetItem();
		QTableWidgetItem *item_State = new QTableWidgetItem();

		QString diagram_title = list_diagram_.at(i) -> title();
		if (diagram_title.isEmpty()) diagram_title = tr("Sch\351ma sans titre");

		item_Name  -> setData(Qt::DisplayRole, diagram_title);
		item_State -> setData(Qt::CheckStateRole, Qt::Checked);

		ui -> tableDiagram -> setRowCount(j+1);
		ui -> tableDiagram -> setItem(j, 0, item_State);
		ui -> tableDiagram -> setItem(j, 1, item_Name);

	}
	ui -> tableDiagram -> horizontalHeader() -> setStretchLastSection(true);
}

/**
 * @brief get list of Diagrams is selected
 * @return this list of Diagrams
 */
QList<Diagram *> diagramselection::list_of_DiagramSelected() {
	QList<Diagram *> listDiag;
	for(int i=0; i<ui -> tableDiagram -> rowCount();i++){
		if(ui -> tableDiagram -> item(i, 0)->checkState()){
			listDiag.push_back( list_diagram_[i] );
		}
	}
	return listDiag;
}

/**
 * @brief contextMenuRequested
 * @param pos
 */
void diagramselection::on_tableDiagram_customContextMenuRequested(const QPoint &pos){
	QMenu menu(this);
	QAction *desl = menu.addAction( tr("D\351s\351lectionner tout") );
	QAction *sel  = menu.addAction(QIcon(":/ico/16x16/dialog-ok.png"), tr("S\351lectionner tout") );
	
	// Exec Menu
	QAction *ret = menu.exec(ui -> tableDiagram -> viewport() -> mapToGlobal(pos));	
	if (ret == desl)	{
		for(int i=0; i<ui -> tableDiagram -> rowCount();i++)
			ui -> tableDiagram -> item(i, 0)->setCheckState(Qt::Unchecked);
	}
	else{
		for(int i=0; i<ui -> tableDiagram -> rowCount();i++)
			ui -> tableDiagram -> item(i, 0)->setCheckState(Qt::Checked);
	}
}

