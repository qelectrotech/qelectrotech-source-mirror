/*
		Copyright 2006-2016 The QElectroTech Team
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
#include "dialogautonum.h"
#include "ui_dialogautonum.h"

//#include "conductorautonumerotation.h"
#include "qetmessagebox.h"
#include "ui/selectautonumw.h"

/**
 * @brief DialogAutoNum::DialogAutoNum
 * @param dg
 * @param parent
 */
DialogAutoNum::DialogAutoNum(Diagram *dg, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DialogAutoNum),
	dg_ (dg)
{
	ui -> setupUi(this);
	
	ui -> configuration_layout -> addWidget (new SelectAutonumW());

	dgselect_ = new diagramselection( dg_ -> project(), ui -> annotation_tab);
	ui -> verticalLayout_Selection -> addWidget(dgselect_);
}

/**
 * @brief Destructor
 */
DialogAutoNum::~DialogAutoNum(){
	delete ui;
}

/**
 * @brief DialogAutoNum::on_pushButton_delete_clicked
 */
void DialogAutoNum::on_pushButton_delete_clicked() {
	// get list of diagrams selected
	QList<Diagram *>listDiag = dgselect_ -> list_of_DiagramSelected();
	if(listDiag.count()<=0) return;
	
	QString diagramsTitle;
	for(int i=0; i<listDiag.count(); i++){
		diagramsTitle += listDiag.at(i) -> title();
		if(i+1 < listDiag.count()) diagramsTitle += ", ";
	}
	// Ask if user is sure to delete the conductor numerotation
	QMessageBox::StandardButton answer = QET::QetMessageBox::critical(
		this,
		tr("Suppression des annotations conducteurs", "Attention"),
		QString(
			tr("Voulez-vous vraiment supprimer les annotations conducteurs de :\n\n%1 ?")
		).arg(diagramsTitle),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);
	
	// if yes remove all
	if( answer ==  QMessageBox::Yes) {
		for(int i=0; i<listDiag.count(); i++){
			/*ConductorAutoNumerotation can(listDiag.at(i));
			can.removeNumOfDiagram();*/
		}
	}
}

/**
 * @brief set the autonum to all diagram selected
 */
void DialogAutoNum::on_pushButton_annotation_clicked(){
	// Get list of diagrams selected
	QList<Diagram *>listDiag = dgselect_ -> list_of_DiagramSelected();
	if(listDiag.count()<=0) return;
	
	QString diagramsTitle;
	for(int i=0; i<listDiag.count(); i++){
		diagramsTitle += listDiag.at(i) -> title();
		if(i+1 < listDiag.count()) diagramsTitle += ", ";
	}
	// Ask if user is sure to numerate the conductor
	QET::QetMessageBox::warning(
		this,
		tr("Annotation des conducteurs", "Attention"),
		QString(
			tr("Voulez-vous vraiment annoter les conducteurs de :\n\n%1 ?")
		).arg(diagramsTitle),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);
}

/**
 * @brief Close the dialog
 */
void DialogAutoNum::on_pushButton_close_clicked() {
	close();
}
