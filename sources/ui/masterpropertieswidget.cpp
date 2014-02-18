#include "masterpropertieswidget.h"
#include "ui_masterpropertieswidget.h"

MasterPropertiesWidget::MasterPropertiesWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::MasterPropertiesWidget)
{
	ui->setupUi(this);
}

MasterPropertiesWidget::~MasterPropertiesWidget()
{
	delete ui;
}
