#include "dialogconductorautonum.h"
#include "ui_dialogconductorautonum.h"

#include "conductorautonumerotation.h"
#include "qetmessagebox.h"

DialogConductorAutoNum::DialogConductorAutoNum(Diagram *dg, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DialogConductorAutoNum)
{
	ui->setupUi(this);
	dg_ = dg;
}

DialogConductorAutoNum::~DialogConductorAutoNum()
{
	delete ui;
}

/**
 * @brief DialogConductorAutoNum::on_pushButton_delete_clicked
 */
void DialogConductorAutoNum::on_pushButton_delete_clicked() {
	// TODO check if project or diagram
	// ...
	// Ask if user is sure to delete the conductor numerotation
	QMessageBox::StandardButton answer = QET::MessageBox::critical(
		this,
		tr("Suppression des annotations conducteurs", "Attention"),
		QString(
			tr("Voulez vraiment supprimer les annotations conducteurs de\n%1 ?")
		).arg(dg_ -> title()),
		QMessageBox::Yes | QMessageBox::No,
		QMessageBox::No
	);
	
	// if yes remove all
	if( answer ==  QMessageBox::Yes) {
		ConductorAutoNumerotation ConductorNum;
		ConductorNum.removeNum_ofDiagram( dg_ );
	}
}

/**
 * @brief Close the dialog
 */
void DialogConductorAutoNum::on_pushButton_close_clicked() {
	close();
}

