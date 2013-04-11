#include "dialogconductorautonum.h"
#include "ui_dialogconductorautonum.h"

#include "conductorautonumerotation.h"

DialogConductorAutoNum::DialogConductorAutoNum(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DialogConductorAutoNum)
{
	ui->setupUi(this);
}

DialogConductorAutoNum::~DialogConductorAutoNum()
{
	delete ui;
}
