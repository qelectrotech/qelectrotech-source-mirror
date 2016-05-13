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
#include "shapegraphicsitempropertieswidget.h"
#include "ui_shapegraphicsitempropertieswidget.h"
#include "qetshapeitem.h"
#include "diagram.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"

/**
 * @brief ShapeGraphicsItemPropertiesWidget::ShapeGraphicsItemPropertiesWidget
 * Constructor
 * @param item : shape to edit
 * @param parent : parent widget
 */
ShapeGraphicsItemPropertiesWidget::ShapeGraphicsItemPropertiesWidget(QetShapeItem *item, QWidget *parent) :
	PropertiesEditorWidget(parent),
	ui(new Ui::ShapeGraphicsItemPropertiesWidget),
	m_shape(nullptr)
{
	ui->setupUi(this);
	setItem(item);
}

/**
 * @brief ShapeGraphicsItemPropertiesWidget::~ShapeGraphicsItemPropertiesWidget
 * Destructor
 */
ShapeGraphicsItemPropertiesWidget::~ShapeGraphicsItemPropertiesWidget()
{
	delete ui;
}

/**
 * @brief ShapeGraphicsItemPropertiesWidget::setItem
 * Set @shape as the current edited item
 * @param shape
 */
void ShapeGraphicsItemPropertiesWidget::setItem(QetShapeItem *shape)
{
	if (!shape || shape == m_shape) return;

	if (m_shape && m_live_edit)
	{
		disconnect(m_shape, &QetShapeItem::penChanged, this, &ShapeGraphicsItemPropertiesWidget::updateUi);
		disconnect(m_shape, &QetShapeItem::brushChanged, this, &ShapeGraphicsItemPropertiesWidget::updateUi);
		disconnect(m_shape, &QetShapeItem::closeChanged, this, &ShapeGraphicsItemPropertiesWidget::updateUi);
	}

	m_shape = shape;
	ui->m_close_polygon->setVisible(m_shape->shapeType() == QetShapeItem::Polygon);
	ui->m_filling_gb->setHidden(m_shape->shapeType() == QetShapeItem::Line);

	if (m_live_edit)
	{
		connect(m_shape, &QetShapeItem::penChanged, this, &ShapeGraphicsItemPropertiesWidget::updateUi);
		connect(m_shape, &QetShapeItem::brushChanged, this, &ShapeGraphicsItemPropertiesWidget::updateUi);
		connect(m_shape, &QetShapeItem::closeChanged, this, &ShapeGraphicsItemPropertiesWidget::updateUi);
	}

	updateUi();
}

/**
 * @brief ShapeGraphicsItemPropertiesWidget::apply
 * Apply the current change, by pushing an undo command to the
 * undo stack of the shape diagram.
 */
void ShapeGraphicsItemPropertiesWidget::apply()
{	
	if (m_shape->diagram())
		if (QUndoCommand *undo = associatedUndo())
			m_shape->diagram()->undoStack().push(undo);
}

/**
 * @brief ShapeGraphicsItemPropertiesWidget::reset
 * Reset the change
 */
void ShapeGraphicsItemPropertiesWidget::reset() {
	updateUi();
}

/**
 * @brief ShapeGraphicsItemPropertiesWidget::associatedUndo
 * @return an undo command that represent the change edited by this widget.
 * The returned undo command is a QPropertyUndoCommand with the properties "pen".
 * If there isn't change, return nullptr
 */
QUndoCommand* ShapeGraphicsItemPropertiesWidget::associatedUndo() const
{
	QPropertyUndoCommand *undo = nullptr;
	QPen old_pen = m_shape->pen();
	QPen new_pen = old_pen;

	new_pen.setStyle(Qt::PenStyle(ui->m_style_cb->currentIndex() + 1));
	new_pen.setWidthF(ui->m_size_dsb->value());
	new_pen.setColor(ui->m_color_pb->palette().color(QPalette::Button));

	if (new_pen != old_pen)
	{
		undo = new QPropertyUndoCommand(m_shape, "pen", old_pen, new_pen);
		undo->setText(tr("Modifier le trait d'une forme"));
	}

	QBrush old_brush = m_shape->brush();
	QBrush new_brush = old_brush;
	new_brush.setStyle(Qt::BrushStyle(ui->m_brush_style_cb->currentIndex()));
	new_brush.setColor(ui->m_brush_color_pb->palette().color(QPalette::Button));

	if (new_brush != old_brush)
	{
		if (undo)
			new QPropertyUndoCommand(m_shape, "brush", old_brush, new_brush, undo);
		else
		{
			undo = new QPropertyUndoCommand(m_shape, "brush", old_brush, new_brush);
			undo->setText(tr("Modifier le remplissage d'une forme"));
		}
	}

	if (ui->m_close_polygon->isChecked() != m_shape->isClosed())
	{
		if (undo)
			new QPropertyUndoCommand(m_shape, "close", m_shape->isClosed(), ui->m_close_polygon->isChecked(), undo);
		else
		{
			undo = new QPropertyUndoCommand(m_shape, "close", m_shape->isClosed(), ui->m_close_polygon->isChecked(), undo);
			undo->setText(tr("Fermer le polygone"));
		}
	}

	return undo;
}

/**
 * @brief ShapeGraphicsItemPropertiesWidget::updateUi
 */
void ShapeGraphicsItemPropertiesWidget::updateUi()
{
	bool le = m_live_edit;
	setLiveEdit(false); //Disable temporally live edit mode to avoid weird behavior
		//Pen
	ui->m_style_cb->setCurrentIndex(static_cast<int>(m_shape->pen().style()) - 1);
	ui->m_size_dsb ->setValue(m_shape->pen().widthF());
	setPenColorButton(m_shape->pen().color());

		//Brush
	if (m_shape->shapeType() == QetShapeItem::Polygon)
		ui->m_filling_gb->setVisible(m_shape->isClosed());

	ui->m_brush_style_cb->setCurrentIndex(static_cast<int>(m_shape->brush().style()));
	setBrushColorButton(m_shape->brush().color());

	ui->m_lock_pos_cb->setChecked(!m_shape->isMovable());
	ui->m_close_polygon->setChecked(m_shape->isClosed());
	setLiveEdit(le);
}

/**
 * @brief ShapeGraphicsItemPropertiesWidget::setLiveEdit
 * @param live_edit
 * @return always true
 */
bool ShapeGraphicsItemPropertiesWidget::setLiveEdit(bool live_edit)
{
	if (live_edit == m_live_edit) return true;
	m_live_edit = live_edit;

	if (m_live_edit)
	{
		connect (ui->m_style_cb, SIGNAL(activated(int)), this, SLOT(apply()));
		connect (ui->m_size_dsb, SIGNAL(valueChanged(double)), this, SLOT(apply()));
		connect (ui->m_brush_style_cb, SIGNAL(activated(int)), this, SLOT(apply()));
		connect (ui->m_close_polygon, &QCheckBox::clicked, this, &ShapeGraphicsItemPropertiesWidget::apply);
		connect (m_shape, &QetShapeItem::penChanged, this, &ShapeGraphicsItemPropertiesWidget::updateUi);
		connect (m_shape, &QetShapeItem::closeChanged, this, &ShapeGraphicsItemPropertiesWidget::updateUi);
	}
	else
	{
		disconnect (ui->m_style_cb, SIGNAL(activated(int)), this, SLOT(apply()));
		disconnect (ui->m_size_dsb, SIGNAL(valueChanged(double)), this, SLOT(apply()));
		disconnect (ui->m_brush_style_cb, SIGNAL(activated(int)), this, SLOT(apply()));
		disconnect (ui->m_close_polygon, &QCheckBox::clicked, this, &ShapeGraphicsItemPropertiesWidget::apply);
		disconnect (m_shape, &QetShapeItem::penChanged, this, &ShapeGraphicsItemPropertiesWidget::updateUi);
		disconnect (m_shape, &QetShapeItem::closeChanged, this, &ShapeGraphicsItemPropertiesWidget::updateUi);
	}
	return true;
}

/**
 * @brief ShapeGraphicsItemPropertiesWidget::setPenColorButton
 * Set the color of pen push button to the current color of the shape pen
 * @param color
 */
void ShapeGraphicsItemPropertiesWidget::setPenColorButton(const QColor &color)
{
	QPalette palette;
	palette.setColor(QPalette::Button, color);
	ui -> m_color_pb -> setStyleSheet(QString("background-color: %1; min-height: 1.5em; border-style: outset; border-width: 2px; border-color: gray; border-radius: 4px;").arg(color.name()));
}

/**
 * @brief ShapeGraphicsItemPropertiesWidget::setBrushColorButton
 * Set the color of brush push button to the current color of shape brush
 * @param color
 */
void ShapeGraphicsItemPropertiesWidget::setBrushColorButton(const QColor &color)
{
	QPalette palette;
	palette.setColor(QPalette::Button, color);
	ui->m_brush_color_pb->setStyleSheet(QString("background-color: %1; min-height: 1.5em; border-style: outset; border-width: 2px; border-color: gray; border-radius: 4px;").arg(color.name()));
}

void ShapeGraphicsItemPropertiesWidget::on_m_lock_pos_cb_clicked() {
	m_shape->setMovable(!ui->m_lock_pos_cb->isChecked());
}

/**
 * @brief ShapeGraphicsItemPropertiesWidget::on_m_color_pb_clicked
 * Pen color button was clicked, we open a QColorDialog for select the color to apply to the shape pen.
 */
void ShapeGraphicsItemPropertiesWidget::on_m_color_pb_clicked()
{
	QColor color = QColorDialog::getColor(m_shape->pen().color(), this);
	if (color.isValid())
		setPenColorButton(color);
	if (m_live_edit)
		apply();
}

/**
 * @brief ShapeGraphicsItemPropertiesWidget::on_m_brush_color_pb_clicked
 * Brush color button was clicked, we open a QColorDialog for select the color to apply to the shape brush.
 */
void ShapeGraphicsItemPropertiesWidget::on_m_brush_color_pb_clicked()
{
	QColor color = QColorDialog::getColor(m_shape->brush().color(), this);
	if (color.isValid())
		setBrushColorButton(color);
	if (m_live_edit)
		apply();
}
