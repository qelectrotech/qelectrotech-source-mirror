/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "dynamictextfieldeditor.h"
#include "ui_dynamictextfieldeditor.h"
#include "customelementpart.h"
#include "partdynamictextfield.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"

#include <QPointer>
#include <QGraphicsItem>
#include <QColorDialog>

DynamicTextFieldEditor::DynamicTextFieldEditor(QETElementEditor *editor, PartDynamicTextField *text_field, QWidget *parent) :
	ElementItemEditor(editor, parent),
	ui(new Ui::DynamicTextFieldEditor)
{
	ui->setupUi(this);
	ui->m_composite_text_pb->setDisabled(true);
	ui->m_elmt_info_cb->setDisabled(true);
	if(text_field)
		setPart(text_field);
}

DynamicTextFieldEditor::~DynamicTextFieldEditor()
{
	delete ui;
	if(!m_connection_list.isEmpty())
		for(QMetaObject::Connection con : m_connection_list)
			disconnect(con);
}

/**
 * @brief DynamicTextFieldEditor::setPart
 * Set @part as current edited part of this widget.
 * @param part
 * @return true if @part can be edited by this widget
 */
bool DynamicTextFieldEditor::setPart(CustomElementPart *part)
{
		//Remove previous connection
	if(!m_connection_list.isEmpty())
		for(QMetaObject::Connection con : m_connection_list)
			disconnect(con);
	
	QGraphicsItem *qgi = part->toItem();
	if(!qgi)
		return false;
	else if (qgi->type() != PartDynamicTextField::Type)
		return false;
	
	m_text_field = static_cast<PartDynamicTextField *>(qgi);
	updateForm();
	
		//Setup the connection
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::colorChanged,    [this](){this->updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::fontSizeChanged, [this](){this->updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::taggChanged,     [this](){this->updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::textFromChanged, [this](){this->updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::textChanged,     [this](){this->updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::infoNameChanged, [this](){this->updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::rotationChanged, [this](){this->updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::frameChanged,    [this](){this->updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::compositeTextChanged, [this]() {this->updateForm();});
	
	return true;
}

/**
 * @brief DynamicTextFieldEditor::currentPart
 * @return The current edited part, note they can return nullptr if
 * there is not a currently edited part.
 */
CustomElementPart *DynamicTextFieldEditor::currentPart() const {
	return m_text_field.data();
}

void DynamicTextFieldEditor::updateForm()
{
	if(m_text_field)
	{
		ui->m_x_sb->setValue(m_text_field.data()->x());
		ui->m_y_sb->setValue(m_text_field.data()->y());
		ui->m_rotation_sb->setValue(QET::correctAngle(m_text_field.data()->rotation()));
		ui->m_frame_cb->setChecked(m_text_field.data()->frame());
		ui->m_user_text_le->setText(m_text_field.data()->text());
		ui->m_size_sb->setValue(m_text_field.data()->fontSize());
		ui->m_tagg_le->setText(m_text_field.data()->tagg());
		setColorPushButton(m_text_field.data()->color());
	}
}

void DynamicTextFieldEditor::setColorPushButton(QColor color)
{
	QPalette palette;
	palette.setColor(QPalette::Button, color);
	ui->m_color_pb->setStyleSheet(QString("background-color: %1; min-height: 1.5em; border-style: outset; border-width: 2px; border-color: gray; border-radius: 4px;").arg(color.name()));
}

void DynamicTextFieldEditor::on_m_x_sb_editingFinished()
{
    QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_text_field, "x", m_text_field.data()->x(), ui->m_x_sb->value());
	undo->setText(tr("Déplacer un champ texte"));
	undo->enableAnimation(true);
	undoStack().push(undo);
}

void DynamicTextFieldEditor::on_m_y_sb_editingFinished()
{
	QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_text_field, "y", m_text_field.data()->y(), ui->m_y_sb->value());
	undo->setText(tr("Déplacer un champ texte"));
	undo->enableAnimation(true);
	undoStack().push(undo);
}

void DynamicTextFieldEditor::on_m_rotation_sb_editingFinished()
{
	QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_text_field, "rotation", m_text_field.data()->rotation(), ui->m_rotation_sb->value());
	undo->setText(tr("Pivoter un champ texte"));
	undo->enableAnimation(true);
	undoStack().push(undo);
}

void DynamicTextFieldEditor::on_m_user_text_le_editingFinished()
{
	QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_text_field, "text", m_text_field.data()->text(), ui->m_user_text_le->text());
	undo->setText(tr("Modifier le texte d'un champ texte"));
	undoStack().push(undo);
}

void DynamicTextFieldEditor::on_m_size_sb_editingFinished()
{
	QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_text_field, "fontSize", m_text_field.data()->fontSize(), ui->m_size_sb->value());
	undo->setText(tr("Modifier la taille d'un champ texte"));
	undo->enableAnimation(true);
	undoStack().push(undo);
}

void DynamicTextFieldEditor::on_m_color_pb_clicked()
{
    QColor color = QColorDialog::getColor(m_text_field.data()->color(), this, tr("Couleur du texte"));
	
	if(color.isValid() && color != m_text_field.data()->color())
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_text_field, "color", m_text_field.data()->color(), color);
		undo->setText(tr("Modifier la couleur d'un champ texte"));
		undoStack().push(undo);
		setColorPushButton(m_text_field.data()->color());
	}
}

void DynamicTextFieldEditor::on_m_frame_cb_clicked()
{
    bool frame = ui->m_frame_cb->isChecked();
	
	if(frame != m_text_field.data()->frame())
	{
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_text_field, "frame", m_text_field.data()->frame(), frame);
		undo->setText(tr("Modifier le cadre d'un champ texte"));
		undoStack().push(undo);
	}
}
