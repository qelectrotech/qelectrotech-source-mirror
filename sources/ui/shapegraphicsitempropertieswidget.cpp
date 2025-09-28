/*
	Copyright 2006-2025 The QElectroTech Team
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

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
#include "../qetgraphicsitem/qetshapeitem.h"
#include "../ui_shapegraphicsitempropertieswidget.h"

#include <QHash>

/**
	@brief ShapeGraphicsItemPropertiesWidget::ShapeGraphicsItemPropertiesWidget
	Constructor
	@param item : shape to edit
	@param parent : parent widget
*/
ShapeGraphicsItemPropertiesWidget::ShapeGraphicsItemPropertiesWidget(QetShapeItem *item, QWidget *parent) :
	PropertiesEditorWidget(parent),
	ui(new Ui::ShapeGraphicsItemPropertiesWidget),
	m_shape(nullptr)
{
	ui->setupUi(this);
	setItem(item);
}

ShapeGraphicsItemPropertiesWidget::ShapeGraphicsItemPropertiesWidget(QList<QetShapeItem *> items_list, QWidget *parent) :
	PropertiesEditorWidget (parent),
	ui(new Ui::ShapeGraphicsItemPropertiesWidget)
{
	ui->setupUi(this);
	setItems(items_list);
}

/**
	@brief ShapeGraphicsItemPropertiesWidget::~ShapeGraphicsItemPropertiesWidget
	Destructor
*/
ShapeGraphicsItemPropertiesWidget::~ShapeGraphicsItemPropertiesWidget()
{
	delete ui;
}

/**
	@brief ShapeGraphicsItemPropertiesWidget::setItem
	Set shape as the current edited item
	@param shape
*/
void ShapeGraphicsItemPropertiesWidget::setItem(QetShapeItem *shape)
{
	if (m_shape != shape) {
		clearEditConnection();
	}
	if (!shape) {
		return;
	}

	m_shape = shape;
	ui->m_close_polygon->setVisible(m_shape->shapeType() == QetShapeItem::Polygon);
	ui->m_filling_gb->setHidden(m_shape->shapeType() == QetShapeItem::Line);

	updateUi();
	setUpEditConnection();
}

/**
	@brief ShapeGraphicsItemPropertiesWidget::setItems
	Set a list of shapes to be edited
	@param shapes_list
*/
void ShapeGraphicsItemPropertiesWidget::setItems(QList<QetShapeItem *> shapes_list)
{
	clearEditConnection();
	m_shapes_list.clear();
	m_shape = nullptr;

	if (shapes_list.isEmpty()) {
		updateUi();
	}
	else if (shapes_list.size() == 1)
	{
		setItem(shapes_list.first());
	}
	else
	{
		for (QetShapeItem *shape : shapes_list) {
			m_shapes_list.append(QPointer<QetShapeItem>(shape));
		}
		updateUi();
	}
	setUpEditConnection();
}

/**
	@brief ShapeGraphicsItemPropertiesWidget::apply
	Apply the current change, by pushing an undo command to the
	undo stack of the shape diagram.
*/
void ShapeGraphicsItemPropertiesWidget::apply()
{
	Diagram *d = nullptr;

	if (m_shape && m_shape->diagram()) {
		d = m_shape->diagram();
	}
	else if (!m_shapes_list.isEmpty())
	{
		for (QPointer<QetShapeItem> qsi : m_shapes_list)
		{
			if (qsi->diagram()) {
				d = qsi->diagram();
				break;
			}
		}
	}

	if (d)
	{
		QUndoCommand *undo = associatedUndo();
		if (undo) {
			d->undoStack().push(undo);
		}
	}
}

/**
	@brief ShapeGraphicsItemPropertiesWidget::reset
	Reset the change
*/
void ShapeGraphicsItemPropertiesWidget::reset()
{
	updateUi();
}

/**
	@brief ShapeGraphicsItemPropertiesWidget::associatedUndo
	@return an undo command that represent the change edited by this widget.
	The returned undo command is a QPropertyUndoCommand with the properties "pen".
	If there isn't change, return nullptr
*/
QUndoCommand* ShapeGraphicsItemPropertiesWidget::associatedUndo() const
{
	if (m_live_edit)
	{
			//One shape is edited
		if (m_shapes_list.isEmpty())
		{
			QPropertyUndoCommand *undo = nullptr;

			QPen old_pen = m_shape->pen();
			QPen new_pen = old_pen;

			new_pen.setStyle(Qt::PenStyle(ui->m_style_cb->currentIndex() + 1));
			new_pen.setWidthF(ui->m_size_dsb->value());

			if (ui->m_style_cb->currentIndex() ==5) {
				new_pen.setDashPattern( QVector<qreal>() << 10 << 10 );
				new_pen.setStyle( Qt::CustomDashLine );
			}
				//painter.setPen( new_pen );
			new_pen.setColor(ui->m_color_kpb->color());

			if (new_pen != old_pen)
			{
				undo = new QPropertyUndoCommand(m_shape, "pen", old_pen, new_pen);
				undo->setText(tr("Modifier le trait d'une forme"));
			}

			QBrush old_brush = m_shape->brush();
			QBrush new_brush = old_brush;
			new_brush.setStyle(Qt::BrushStyle(ui->m_brush_style_cb->currentIndex()));
			new_brush.setColor(ui->m_brush_color_kpb->color());

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
		else if (!m_shapes_list.isEmpty()) //seberal shapes are edited
		{
			QUndoCommand *parent_undo = nullptr;
			QetShapeItem *shape_ = m_shapes_list.first().data();

				//Pen
			QHash <QetShapeItem *, QPen> pen_H;

			if (ui->m_style_cb->currentIndex() != -1 &&
				Qt::PenStyle(ui->m_style_cb->currentIndex() + 1) != shape_->pen().style())
			{
				for (QPointer<QetShapeItem> qsi : m_shapes_list)
				{
					QPen pen = qsi->pen();

					if (ui->m_style_cb->currentIndex() ==5) {
						pen.setDashPattern( QVector<qreal>() << 10 << 10 );
						pen.setStyle( Qt::CustomDashLine );
					} else {
						pen.setStyle(Qt::PenStyle(ui->m_style_cb->currentIndex() + 1));
					}
					pen_H.insert(qsi, pen);
				}
			}

			if (ui->m_size_dsb->value() > 0 &&
				ui->m_size_dsb->value() != shape_->pen().widthF())
			{
				for (QPointer<QetShapeItem> qsi : m_shapes_list)
				{
					QPen pen = pen_H.contains(qsi) ? pen_H.value(qsi) : qsi->pen();
					pen.setWidthF(ui->m_size_dsb->value());
					pen_H.insert(qsi, pen);
				}
			}

			QColor c =ui->m_color_kpb->color();
			if (c != QPalette().color(QPalette::Button) && shape_->pen().color() != c)
			{
				for (QPointer<QetShapeItem> qsi : m_shapes_list)
				{
					QPen pen = pen_H.contains(qsi) ? pen_H.value(qsi) : qsi->pen();
					pen.setColor(c);
					pen_H.insert(qsi, pen);
				}
			}

			for (QPointer<QetShapeItem> qsi : pen_H.keys())
			{
				if (!parent_undo) {
					parent_undo = new QUndoCommand(tr("Modifier une forme simple"));
				}
				new QPropertyUndoCommand(qsi, "pen", qsi->pen(), pen_H.value(qsi), parent_undo);
			}

				//Brush
			QHash <QetShapeItem *, QBrush> brush_H;
			if (ui->m_brush_style_cb->currentIndex() != -1 &&
				shape_->brush().style() != Qt::BrushStyle(ui->m_brush_style_cb->currentIndex()))
			{
				for (QPointer<QetShapeItem> qsi : m_shapes_list)
				{
					QBrush brush = qsi->brush();
					brush.setStyle(Qt::BrushStyle(ui->m_brush_style_cb->currentIndex()));
					brush_H.insert(qsi, brush);
				}
			}

			c = ui->m_brush_color_kpb->color();
			if (c != QPalette().color(QPalette::Button) && shape_->brush().color() != c)
			{
				for (QPointer<QetShapeItem> qsi : m_shapes_list)
				{
					QBrush brush = brush_H.contains(qsi) ? brush_H.value(qsi) : qsi->brush();
					brush.setColor(c);
					brush_H.insert(qsi, brush);
				}
			}

			for (QPointer<QetShapeItem> qsi : brush_H.keys())
			{
				if (!parent_undo) {
					parent_undo = new QUndoCommand(tr("Modifier une forme simple"));
				}

				new QPropertyUndoCommand(qsi, "brush", qsi->brush(), brush_H.value(qsi), parent_undo);
			}

			return parent_undo;
		}
	}
		//In mode not live edit, only one shape can be edited
	else if (m_shapes_list.isEmpty())
	{
		QUndoCommand *undo = new QUndoCommand(tr("Modifier les propriétés d'une forme simple"));
		QPen old_pen = m_shape->pen();
		QPen new_pen = old_pen;

		new_pen.setStyle(Qt::PenStyle(ui->m_style_cb->currentIndex() + 1));
		new_pen.setWidthF(ui->m_size_dsb->value());

		if (ui->m_style_cb->currentIndex() ==5) {
			new_pen.setDashPattern( QVector<qreal>() << 10 << 10 );
			new_pen.setStyle( Qt::CustomDashLine );
		}
			//painter.setPen( new_pen );
		new_pen.setColor(ui->m_color_kpb->color());

		if (new_pen != old_pen) {
			new QPropertyUndoCommand(m_shape, "pen", old_pen, new_pen, undo);
		}

		QBrush old_brush = m_shape->brush();
		QBrush new_brush = old_brush;
		new_brush.setStyle(Qt::BrushStyle(ui->m_brush_style_cb->currentIndex()));
		new_brush.setColor(ui->m_brush_color_kpb->color());

		if (new_brush != old_brush) {
			new QPropertyUndoCommand(m_shape, "brush", old_brush, new_brush, undo);
		}

		if (ui->m_close_polygon->isChecked() != m_shape->isClosed()) {
			QPropertyUndoCommand(m_shape, "close", m_shape->isClosed(), ui->m_close_polygon->isChecked(), undo);
		}

		if (undo->childCount()) {
			return undo;
		} else {
			delete undo;
			return nullptr;
		}
	}
	return nullptr;
}

/**
	@brief ShapeGraphicsItemPropertiesWidget::updateUi
*/
void ShapeGraphicsItemPropertiesWidget::updateUi()
{
	if (!m_shape && m_shapes_list.isEmpty()) {
		return;
	}

		//Disconnect every connections of editor widgets
		//to avoid an unwanted edition (QSpinBox emit valueChanged no matter if changer by user or by program)
	clearEditConnection();

	if (m_shape)
	{
		//Pen
		ui->m_style_cb->setCurrentIndex(static_cast<int>(m_shape->pen().style()) - 1);
		ui->m_size_dsb ->setValue(m_shape->pen().widthF());
		ui->m_color_kpb->setColor(m_shape->pen().color());
		ui->m_color_kpb->setColor(m_shape->pen().color());

		//Brush
		if (m_shape->shapeType() == QetShapeItem::Polygon)
			ui->m_filling_gb->setVisible(m_shape->isClosed());

		ui->m_brush_style_cb->setCurrentIndex(static_cast<int>(m_shape->brush().style()));
		ui->m_brush_color_kpb->setColor(m_shape->brush().color());

		ui->m_lock_pos_cb->setChecked(!m_shape->isMovable());
		ui->m_close_polygon->setChecked(m_shape->isClosed());
	}
	else if (m_shapes_list.size() >= 2)
	{
		ui->m_close_polygon->setHidden(true);
		bool same = true;
			//Pen
		Qt::PenStyle ps = m_shapes_list.first()->pen().style();
		for (QetShapeItem *qsi : m_shapes_list) {
			if (qsi->pen().style() != ps) {
				same = false;
				break;
			}
		}
		ui->m_style_cb->setCurrentIndex(same ? static_cast<int>(ps) - 1 : -1);

		same = true;
		qreal pw = m_shapes_list.first()->pen().widthF();
		for (QetShapeItem *qsi : m_shapes_list) {
			if (qsi->pen().widthF() != pw) {
				same = false;
				break;
			}
		}
		ui->m_size_dsb->setValue(same ? pw : 0);

		same = true;
		QColor pc = m_shapes_list.first()->pen().color();
		for (QetShapeItem *qsi : m_shapes_list) {
			if (qsi->pen().color() != pc) {
				same = false;
				break;
			}
		}
		ui->m_color_kpb->setColor(same ? pc : QColor());

			//Brush
		ui->m_filling_gb->setVisible(true);

		same = true;
		Qt::BrushStyle bs = m_shapes_list.first()->brush().style();
		for (QetShapeItem *qsi : m_shapes_list) {
			if (qsi->brush().style() != bs) {
				same = false;
				break;
			}
		}
		ui->m_brush_style_cb->setCurrentIndex(same ? static_cast<int>(bs) : -1);

		same = true;
		QColor bc = m_shapes_list.first()->brush().color();
		for (QetShapeItem *qsi : m_shapes_list) {
			if (qsi->brush().color() != bc) {
				same = false;
				break;
			}
		}
		ui->m_brush_color_kpb->setColor(same ? bc : QColor());

		ui->m_lock_pos_cb->setChecked(false);
		ui->m_close_polygon->setChecked(false);
	}

	setUpEditConnection();
}

/**
	@brief ShapeGraphicsItemPropertiesWidget::setLiveEdit
	@param live_edit
	@return always true
*/
bool ShapeGraphicsItemPropertiesWidget::setLiveEdit(bool live_edit)
{
	if (live_edit == m_live_edit) {
		return true;
	}
	m_live_edit = live_edit;

	if (m_live_edit) {
		setUpEditConnection();
	} else {
		clearEditConnection();
	}
	return true;
}

/**
 * @brief ShapeGraphicsItemPropertiesWidget::setUpEditConnection
 * Disconnect the previous connection, and reconnect the connection between
 * the editors widgets and void ShapeGraphicsItemPropertiesWidget::apply function
*/
void ShapeGraphicsItemPropertiesWidget::setUpEditConnection()
{
	clearEditConnection();

	if (m_shape || !m_shapes_list.isEmpty())
	{
		m_edit_connection << connect (ui->m_style_cb, QOverload<int>::of(&QComboBox::activated),
									  this, &ShapeGraphicsItemPropertiesWidget::apply);

		m_edit_connection << connect (ui->m_size_dsb, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
									  this, &ShapeGraphicsItemPropertiesWidget::apply);

		m_edit_connection << connect (ui->m_color_kpb, &ColorButton::changed,
									  this, &ShapeGraphicsItemPropertiesWidget::apply);

		m_edit_connection << connect (ui->m_brush_style_cb, QOverload<int>::of(&QComboBox::activated),
									  this, &ShapeGraphicsItemPropertiesWidget::apply);

		m_edit_connection << connect (ui->m_brush_color_kpb, &ColorButton::changed,
									  this, &ShapeGraphicsItemPropertiesWidget::apply);

		m_edit_connection << connect (ui->m_close_polygon, &QCheckBox::clicked,
									  this, &ShapeGraphicsItemPropertiesWidget::apply);

		m_edit_connection << connect (m_shape, &QetShapeItem::penChanged,
									  this, &ShapeGraphicsItemPropertiesWidget::updateUi);

		m_edit_connection << connect (m_shape, &QetShapeItem::closeChanged,
									  this, &ShapeGraphicsItemPropertiesWidget::updateUi);

		m_edit_connection << connect (m_shape, &QetShapeItem::brushChanged,
									  this, &ShapeGraphicsItemPropertiesWidget::updateUi);

	}
}

void ShapeGraphicsItemPropertiesWidget::clearEditConnection()
{
	for (const auto &c : std::as_const(m_edit_connection)) {
		disconnect(c);
	}
	m_edit_connection.clear();
}

void ShapeGraphicsItemPropertiesWidget::on_m_lock_pos_cb_clicked()
{
	if (m_shape) {
		m_shape->setMovable(!ui->m_lock_pos_cb->isChecked());
	}
	else if (!m_shapes_list.isEmpty()) {
		for (QPointer<QetShapeItem> qsi : m_shapes_list) {
			qsi->setMovable(!ui->m_lock_pos_cb->isChecked());
		}
	}
}
