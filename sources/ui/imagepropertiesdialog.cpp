#include "imagepropertiesdialog.h"
#include "ui_imagepropertiesdialog.h"
#include "imagepropertieswidget.h"
#include "diagramimageitem.h"

ImagePropertiesDialog::ImagePropertiesDialog(DiagramImageItem *image, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ImagePropertiesDialog)
{
	ui->setupUi(this);
	m_editor = new ImagePropertiesWidget(image, this);
	ui->verticalLayout->insertWidget(0, m_editor);
}

ImagePropertiesDialog::~ImagePropertiesDialog() {
	delete ui;
}

void ImagePropertiesDialog::setImageItem(DiagramImageItem *image) {
	m_editor->setImageItem(image);
}

void ImagePropertiesDialog::on_buttonBox_accepted() {
	m_editor->apply();
}

void ImagePropertiesDialog::on_buttonBox_rejected() {
	m_editor->reset();
}
