/*
		Copyright 2006-2013 The QElectroTech Team
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
#include "dialogconductorautonum.h"
#include "ui_dialogconductorautonum.h"

#include "conductorautonumerotation.h"
#include "qetmessagebox.h"


DialogConductorAutoNum::DialogConductorAutoNum(Diagram *dg, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DialogConductorAutoNum)
{
	ui -> setupUi(this);
	dg_ = dg;
	
	// create widget of diagram selection
	dgselect_ = new diagramselection( dg_ ->project(), this );
	ui -> verticalLayout_Selection -> addWidget( dgselect_ );
}

DialogConductorAutoNum::~DialogConductorAutoNum()
{
	delete ui;
}


/**
 * @brief DialogConductorAutoNum::on_pushButton_delete_clicked
 */
void DialogConductorAutoNum::on_pushButton_delete_clicked() {
	// get list of diagrams selected
	QList<Diagram *>listDiag = dgselect_ ->list_of_DiagramSelected();
	if(listDiag.count()<=0) return;
	
	QString diagramsTitle;
	for(int i=0; i<listDiag.count(); i++){
		diagramsTitle += listDiag.at(i)->title();
		if(i+1<listDiag.count()) diagramsTitle += ", ";
	}
	// Ask if user is sure to delete the conductor numerotation
	QMessageBox::StandardButton answer = QET::MessageBox::critical(
		this,
		tr("Suppression des annotations conducteurs", "Attention"),
		QString(
			tr("Voulez vraiment supprimer les annotations conducteurs de :\n\n%1 ?")
		).arg(diagramsTitle),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);
	
	// if yes remove all
	if( answer ==  QMessageBox::Yes) {
		for(int i=0; i<listDiag.count(); i++){
			ConductorAutoNumerotation can(listDiag.at(i));
			can.removeNum_ofDiagram();
		}
	}
}

/**
 * @brief Close the dialog
 */
void DialogConductorAutoNum::on_pushButton_close_clicked() {
	close();
}

/**
 * @brief set the autonum to all diagram selected
 */
void DialogConductorAutoNum::on_pushButton_annotation_clicked(){
	// Get list of diagrams selected
	QList<Diagram *>listDiag = dgselect_ ->list_of_DiagramSelected();
	if(listDiag.count()<=0) return;
	
	QString diagramsTitle;
	for(int i=0; i<listDiag.count(); i++){
		diagramsTitle += listDiag.at(i)->title();
		if(i+1<listDiag.count()) diagramsTitle += ", ";
	}
	// Ask if user is sure to numerate the conductor
	QMessageBox::StandardButton answer = QET::MessageBox::warning(
		this,
		tr("Annotation des conducteurs", "Attention"),
		QString(
			tr("Voulez vraiment annoter les conducteurs de :\n\n%1 ?")
		).arg(diagramsTitle),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);
	
	// if yes numerate all
	if( answer ==  QMessageBox::Yes) {
		NumerotationContext num;
		for(int i=0; i<listDiag.count(); i++){
			num.clear();
			num.addValue("ten",5);
			num.addValue("string","U");
			num.addValue("folio");
			listDiag.at(i)->setNumerotation(Diagram::Conductors, num);
			qDebug() << "ok";
		}
	}
}

