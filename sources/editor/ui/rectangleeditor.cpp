/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "rectangleeditor.h"
#include "ui_rectangleeditor.h"
#include "styleeditor.h"
#include "partrectangle.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "elementscene.h"
#include "qeticons.h"

/**
	@brief RectangleEditor::RectangleEditor
	@param editor
	@param rect
	@param parent
*/
RectangleEditor::RectangleEditor(QETElementEditor *editor, PartRectangle *rect, QWidget *parent) :
	ElementItemEditor(editor, parent),
	m_part(rect),
	ui(new Ui::RectangleEditor)
{
	ui->setupUi(this);
	m_style = new StyleEditor(editor);
	ui->verticalLayout->insertWidget(0, m_style);
}

/**
	@brief RectangleEditor::~RectangleEditor
*/
RectangleEditor::~RectangleEditor()
{
	delete ui;
}

void RectangleEditor::setUpChangeConnections()
{
	m_change_connections << connect(m_part, &PartRectangle::rectChanged, this, &RectangleEditor::updateForm);
	m_change_connections << connect(m_part, &PartRectangle::XRadiusChanged, this, &RectangleEditor::updateForm);
	m_change_connections << connect(m_part, &PartRectangle::YRadiusChanged, this, &RectangleEditor::updateForm);
	m_change_connections << connect(m_part, &PartRectangle::xChanged, this, &RectangleEditor::updateForm);
	m_change_connections << connect(m_part, &PartRectangle::yChanged, this, &RectangleEditor::updateForm);
}

void RectangleEditor::disconnectChangeConnections()
{
	for (QMetaObject::Connection c : m_change_connections) {
		disconnect(c);
	}
	m_change_connections.clear();
}

/**
	@brief RectangleEditor::setPart
	@param part
	@return 
*/
bool RectangleEditor::setPart(CustomElementPart *part)
{
	if (!part)
	{
		if (m_part)
		{
			disconnectChangeConnections();
		}
		m_part = nullptr;
		return(true);
	}
	
	if (PartRectangle *part_rectangle = dynamic_cast<PartRectangle *>(part))
	{
		if (m_part == part_rectangle) {
			return true;
		}
		if (m_part)
		{
			disconnectChangeConnections();
		}
		m_part = part_rectangle;
		updateForm();
		setUpChangeConnections();
		return(true);
	}
	
	return(false);
}

bool RectangleEditor::setParts(QList <CustomElementPart *> parts)
{
	if (parts.isEmpty())
		return false;
	
	if (!setPart(parts.first()))
		return false;
	return m_style->setParts(parts);
}

/**
	@brief RectangleEditor::currentPart
	@return 
*/
CustomElementPart *RectangleEditor::currentPart() const
{
	return m_part;
}

QList<CustomElementPart*> RectangleEditor::currentParts() const
{
	return m_style->currentParts();
}

/**
	@brief RectangleEditor::topLeft
	@return The edited topLeft already mapped to part coordinate
*/
QPointF RectangleEditor::editedTopLeft() const
{
	return m_part->mapFromScene(ui->m_x_sb->value(), ui->m_y_sb->value());
}

/**
	@brief RectangleEditor::updateForm
*/
void RectangleEditor::updateForm()
{
	if (!m_part) {
		return;
	}
	activeConnections(false);
	
	QRectF rect = m_part->property("rect").toRectF();
	QPointF p = m_part->mapToScene(rect.topLeft());
	ui->m_x_sb->setValue(p.x());
	ui->m_y_sb->setValue(p.y());
	ui->m_width_sb->setValue(rect.width());
	ui->m_height_sb->setValue(rect.height());
	ui->m_rx_sb->setValue(m_part->XRadius());
	ui->m_rx_sb->setMaximum(rect.width()/2);
	ui->m_ry_sb->setValue(m_part->YRadius());
	ui->m_ry_sb->setMaximum(rect.height()/2);
	
	activeConnections(true);
}

/**
	@brief RectangleEditor::editingFinished
	Slot called when a editor widget is finish to be edited
	Update the geometry of the rectangle according to value of editing widget.
*/
void RectangleEditor::editingFinished()
{
	if (m_locked) {
		return;
	}
	m_locked = true;
	
	QUndoCommand *undo = new QUndoCommand();
	undo->setText(tr("Modifier un rectangle"));
	
	QRectF rect(editedTopLeft(), QSizeF(ui->m_width_sb->value(), ui->m_height_sb->value()));
	if (m_part->rect() != rect)
	{
		QPropertyUndoCommand *u = new QPropertyUndoCommand(m_part, "rect", m_part->rect(), rect, undo);
		u->enableAnimation();
	}
	if (m_part->XRadius() != ui->m_rx_sb->value())
	{
		QPropertyUndoCommand *u = new QPropertyUndoCommand(m_part, "xRadius", m_part->XRadius(), ui->m_rx_sb->value(), undo);
		u->setAnimated();
	}
	if (m_part->YRadius() != ui->m_ry_sb->value())
	{
		QPropertyUndoCommand *u = new QPropertyUndoCommand(m_part, "yRadius", m_part->YRadius(), ui->m_ry_sb->value(), undo);
		u->setAnimated();
	}
	elementScene()->undoStack().push(undo);
	
	m_locked = false;
}

void RectangleEditor::xPosChanged()
{
	if (m_locked) {
		return;
	}
	m_locked = true;
	
	for (auto part: m_style->currentParts()) {
		
		PartRectangle* rec = static_cast<PartRectangle*>(part);
		
		QRectF rect = rec->rect();
		
		QPointF p = rec->mapFromScene(ui->m_x_sb->value(), ui->m_y_sb->value());
		
		if (rect.x() != p.x()) {
			rect.moveLeft(p.x());
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(rec, "rect", rec->rect(), rect);
			undo->enableAnimation();
			elementScene()->undoStack().push(undo);
		}
	}
	
	m_locked = false;
}

void RectangleEditor::yPosChanged()
{
	if (m_locked) {
		return;
	}
	m_locked = true;
	
	for (auto part: m_style->currentParts()) {
		
		PartRectangle* rec = static_cast<PartRectangle*>(part);
		
		QRectF rect = rec->rect();
		
		QPointF p = rec->mapFromScene(ui->m_x_sb->value(), ui->m_y_sb->value());
		
		if (rect.y() != p.y()) {
			rect.moveTop(p.y());
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(rec, "rect", rec->rect(), rect);
			undo->enableAnimation();
			elementScene()->undoStack().push(undo);
		}
	}
	
	m_locked = false;
}

void RectangleEditor::widthChanged()
{
	if (m_locked) {
		return;
	}
	m_locked = true;
	
	double width = ui->m_width_sb->value();
	
	for (auto part: m_style->currentParts()) {
		
		PartRectangle* rec = static_cast<PartRectangle*>(part);
		
		QRectF rect = rec->rect();
		
		if (rect.width() != width) {
			rect.setWidth(width);
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(rec, "rect", rec->rect(), rect);
			undo->enableAnimation();
			elementScene()->undoStack().push(undo);
		}
	}
	
	m_locked = false;
}

void RectangleEditor::heightChanged()
{
	if (m_locked) {
		return;
	}
	m_locked = true;
	
	double height = ui->m_height_sb->value();
	
	for (auto part: m_style->currentParts()) {
		
		PartRectangle* rec = static_cast<PartRectangle*>(part);
		
		QRectF rect = rec->rect();
		
		if (rect.height() != height) {
			rect.setHeight(height);
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(rec, "rect", rec->rect(), rect);
			undo->enableAnimation();
			elementScene()->undoStack().push(undo);
		}
	}
	
	m_locked = false;
}

void RectangleEditor::xRadiusChanged()
{
	if (m_locked) {
		return;
	}
	m_locked = true;
	
	double radius = ui->m_rx_sb->value();
	
	for (auto part: m_style->currentParts()) {
		
		PartRectangle* rec = static_cast<PartRectangle*>(part);
		
		if (rec->XRadius() != radius) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(rec, "xRadius", rec->XRadius(), radius);
			undo->setAnimated();
			elementScene()->undoStack().push(undo);
		}
	}
	
	m_locked = false;
}

void RectangleEditor::yRadiusChanged()
{
	if (m_locked) {
		return;
	}
	m_locked = true;
	
	double radius = ui->m_ry_sb->value();
	
	for (auto part: m_style->currentParts()) {
		
		PartRectangle* rec = static_cast<PartRectangle*>(part);
		
		if (rec->YRadius() != radius) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(rec, "yRadius", rec->YRadius(), radius);
			undo->setAnimated();
			elementScene()->undoStack().push(undo);
		}
	}
	
	m_locked = false;
}


/**
	@brief RectangleEditor::activeConnections
	Enable/disable connection between editor widget and slot editingFinished
	True == enable | false == disable
	@param active
*/
void RectangleEditor::activeConnections(bool active)
{
	if (active)
	{
		connect(ui->m_x_sb,      &QDoubleSpinBox::editingFinished, this, &RectangleEditor::xPosChanged);
		connect(ui->m_y_sb,      &QDoubleSpinBox::editingFinished, this, &RectangleEditor::yPosChanged);
		connect(ui->m_width_sb,  &QDoubleSpinBox::editingFinished, this, &RectangleEditor::widthChanged);
		connect(ui->m_height_sb, &QDoubleSpinBox::editingFinished, this, &RectangleEditor::heightChanged);
		connect(ui->m_rx_sb,     &QDoubleSpinBox::editingFinished, this, &RectangleEditor::xRadiusChanged);
		connect(ui->m_ry_sb,     &QDoubleSpinBox::editingFinished, this, &RectangleEditor::yRadiusChanged);
	}
	else
	{
		disconnect(ui->m_x_sb,      &QDoubleSpinBox::editingFinished, this, &RectangleEditor::xPosChanged);
		disconnect(ui->m_y_sb,      &QDoubleSpinBox::editingFinished, this, &RectangleEditor::yPosChanged);
		disconnect(ui->m_width_sb,  &QDoubleSpinBox::editingFinished, this, &RectangleEditor::widthChanged);
		disconnect(ui->m_height_sb, &QDoubleSpinBox::editingFinished, this, &RectangleEditor::heightChanged);
		disconnect(ui->m_rx_sb,     &QDoubleSpinBox::editingFinished, this, &RectangleEditor::xRadiusChanged);
		disconnect(ui->m_ry_sb,     &QDoubleSpinBox::editingFinished, this, &RectangleEditor::yRadiusChanged);
	}
}
