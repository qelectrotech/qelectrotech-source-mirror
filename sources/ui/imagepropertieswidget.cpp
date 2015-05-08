/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "imagepropertieswidget.h"
#include "ui_imagepropertieswidget.h"
#include "diagramimageitem.h"
#include "diagramcommands.h"
#include <QUndoCommand>

/**
 * @brief ImagePropertiesWidget::ImagePropertiesWidget
 * Constructor
 * @param image : image to edit properties
 * @param parent : parent widget
 */
ImagePropertiesWidget::ImagePropertiesWidget(DiagramImageItem *image, QWidget *parent) :
	PropertiesEditorWidget(parent),
	ui(new Ui::ImagePropertiesWidget),
	m_image(nullptr)
{
	ui->setupUi(this);
	this->setDisabled(true);
	setImageItem(image);
}

/**
 * @brief ImagePropertiesWidget::~ImagePropertiesWidget
 * Destructor
 */
ImagePropertiesWidget::~ImagePropertiesWidget() {
	delete ui;
}

/**
 * @brief ImagePropertiesWidget::setImageItem
 * Set the image to edit properties
 * @param image : image to edit
 */
void ImagePropertiesWidget::setImageItem(DiagramImageItem *image)
{
	if(!image) return;
	this->setEnabled(true);
	if (m_image == image) return;
	if (m_image)
		disconnect(m_image, SIGNAL(destroyed()), this, SLOT(imageWasDeleted()));

	m_image = image;
	connect(m_image, SIGNAL(destroyed()), this, SLOT(imageWasDeleted()));
	m_movable = image->isMovable();
	m_scale = m_image->scale();
	updateUi();
}

/**
 * @brief ImagePropertiesWidget::apply
 * Apply the change
 */
void ImagePropertiesWidget::apply()
{
	if(!m_image) return;

	if (m_image->diagram())
		m_image->diagram()->undoStack().push(associatedUndo());

	m_scale = m_image->scale();
}

/**
 * @brief ImagePropertiesWidget::reset
 * Reset the change
 */
void ImagePropertiesWidget::reset()
{
	if(!m_image) return;

	m_image->setScale(m_scale);
	m_image->setMovable(m_movable);
	updateUi();
}

/**
 * @brief ImagePropertiesWidget::associatedUndo
 * @return the change in an undo command (ItemResizerCommand)
 */
QUndoCommand* ImagePropertiesWidget::associatedUndo()
{
	qreal value = ui->m_scale_slider->value();
	value /= 100;
	return new ItemResizerCommand(m_image, m_scale, value, tr("une image"));
}

/**
 * @brief ImagePropertiesWidget::updateUi
 * Udpdate the ui, notably when the image to edit change
 */
void ImagePropertiesWidget::updateUi()
{
	ui->m_scale_slider->setValue(m_scale * 100);
	ui->m_lock_pos_cb->setChecked(!m_movable);
}

/**
 * @brief ImagePropertiesWidget::on_m_scale_slider_valueChanged
 * Update the size of image when move slider.
 * @param value
 */
void ImagePropertiesWidget::on_m_scale_slider_valueChanged(int value)
{
	qreal scale = value;
	m_image->setScale(scale / 100);
}

void ImagePropertiesWidget::imageWasDeleted() {
	m_image = nullptr;
}

/**
 * @brief ImagePropertiesWidget::on_m_lock_pos_cb_clicked
 * Set movable or not the image according to corresponding check box
 */
void ImagePropertiesWidget::on_m_lock_pos_cb_clicked() {
	m_image->setMovable(!ui->m_lock_pos_cb->isChecked());
}
