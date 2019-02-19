/*
	Copyright 2006-2019 The QElectroTech Team
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
#include "inditextpropertieswidget.h"
#include "ui_inditextpropertieswidget.h"
#include "independenttextitem.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "diagram.h"
#include "diagramcommands.h"
#include <QtGlobal>
#include <QLineEdit>

/**
 * @brief IndiTextPropertiesWidget::IndiTextPropertiesWidget
 * @param text : the text to edit
 * @param parent : the parent widget of this widget
 */
IndiTextPropertiesWidget::IndiTextPropertiesWidget(IndependentTextItem *text, QWidget *parent) :
	PropertiesEditorWidget(parent),
	ui(new Ui::IndiTextPropertiesWidget)
{
	ui->setupUi(this);
	if (text) {
		setText(text);
	}
}

/**
 * @brief IndiTextPropertiesWidget::~IndiTextPropertiesWidget
 */
IndiTextPropertiesWidget::~IndiTextPropertiesWidget() {
	delete  ui;
}

/**
 * @brief IndiTextPropertiesWidget::setText
 * @param text : set @text as edited text
 */
void IndiTextPropertiesWidget::setText(IndependentTextItem *text)
{
	if (m_text) {
		for (QMetaObject::Connection c : m_connect_list) {
			disconnect(c);
		}
	}
	
	m_text = text;
	m_connect_list.clear();
	m_connect_list << connect(m_text.data(), &IndependentTextItem::xChanged, this, &IndiTextPropertiesWidget::updateUi);
	m_connect_list << connect(m_text.data(), &IndependentTextItem::yChanged, this, &IndiTextPropertiesWidget::updateUi);
	m_connect_list << connect(m_text.data(), &IndependentTextItem::rotationChanged, this, &IndiTextPropertiesWidget::updateUi);
	m_connect_list << connect(m_text.data(), &IndependentTextItem::fontSizeChanged, this, &IndiTextPropertiesWidget::updateUi);
	m_connect_list << connect(m_text.data(), &IndependentTextItem::textEdited, this, &IndiTextPropertiesWidget::updateUi);

	updateUi();
}

/**
 * @brief IndiTextPropertiesWidget::apply
 * Apply the current edition through a QUndoCommand pushed
 * to the undo stack of text's diagram.
 */
void IndiTextPropertiesWidget::apply()
{
	if (m_text && m_text->diagram())
	{
		QUndoCommand *undo = associatedUndo();
		if (undo) {
			m_text->diagram()->undoStack().push(undo);
		}
	}
}

/**
 * @brief IndiTextPropertiesWidget::setLiveEdit
 * @param live_edit
 * @return 
 */
bool IndiTextPropertiesWidget::setLiveEdit(bool live_edit)
{
	if (m_live_edit == live_edit) {
		return true;
	}
	m_live_edit = live_edit;
	
	if (m_live_edit) {
		setUpEditConnection();
	}
	else {
		for (QMetaObject::Connection c : m_edit_connection) {
			disconnect(c);
		}
		m_edit_connection.clear();
	}
	return true;
}

/**
 * @brief IndiTextPropertiesWidget::associatedUndo
 * @return 
 */
QUndoCommand *IndiTextPropertiesWidget::associatedUndo() const
{
	if (m_live_edit)
	{
		QPropertyUndoCommand *undo = nullptr;
		if(ui->m_x_sb->value() != m_text->pos().x()) {
			undo = new QPropertyUndoCommand(m_text.data(), "x", QVariant(m_text->pos().x()), QVariant(ui->m_x_sb->value()));
			undo->setAnimated(true, false);
			undo->setText(tr("Déplacer un champ texte"));
		}
		if(ui->m_y_sb->value() != m_text->pos().y()) {
			undo = new QPropertyUndoCommand(m_text.data(), "y", QVariant(m_text->pos().y()), QVariant(ui->m_y_sb->value()));
			undo->setAnimated(true, false);
			undo->setText(tr("Déplacer un champ texte"));
		}
		if(ui->m_angle_sb->value() != m_text->rotation()) {
			undo = new QPropertyUndoCommand(m_text.data(), "rotation", QVariant(m_text->rotation()), QVariant(ui->m_angle_sb->value()));
			undo->setAnimated(true, false);
			undo->setText(tr("Pivoter un champ texte"));
		}
		if (ui->m_line_edit->text() != m_text->toPlainText()) {
			undo = new QPropertyUndoCommand(m_text.data(), "plainText", m_text->toPlainText(), ui->m_line_edit->text());
			undo->setText(tr("Modifier un champ texte"));
		}
		if (ui->m_size_sb->value() != m_text->fontSize()) {
			undo = new QPropertyUndoCommand(m_text.data(), "fontSize", m_text->fontSize(), ui->m_size_sb->value());
			undo->setAnimated(true, false);
			undo->setText(tr("Modifier la taille d'un champ texte"));
		}
		
		return undo;
	}
	else
	{
		QUndoCommand *undo = new QUndoCommand(tr("Modifier les propriétées d'un texte"));
		if(ui->m_x_sb->value() != m_text->pos().x()) {
			new QPropertyUndoCommand(m_text.data(), "x", QVariant(m_text->pos().x()), QVariant(ui->m_x_sb->value()), undo);
		}
		if(ui->m_y_sb->value() != m_text->pos().y()) {
			new QPropertyUndoCommand(m_text.data(), "y", QVariant(m_text->pos().y()), QVariant(ui->m_y_sb->value()), undo);
		}
		if(ui->m_angle_sb->value() != m_text->rotation()) {
			new QPropertyUndoCommand(m_text.data(), "rotation", QVariant(m_text->rotation()), QVariant(ui->m_angle_sb->value()), undo);
		}
		if (ui->m_line_edit->text() != m_text->toPlainText()) {
			new ChangeDiagramTextCommand(m_text.data(), m_text->toHtml(), ui->m_line_edit->text(), undo);
		}
		if (ui->m_size_sb->value() != m_text->fontSize()) {
			new QPropertyUndoCommand(m_text.data(), "fontSize", m_text->fontSize(), ui->m_size_sb->value(), undo);
		}
		
		if (undo->childCount()) {
			return undo;
		} else {
			return nullptr;
		}
	}
}

/**
 * @brief IndiTextPropertiesWidget::setUpEditConnection
 * Disconnect the previous connection, and reconnect the connection between the editors widgets and apply function
 */
void IndiTextPropertiesWidget::setUpEditConnection()
{
	for (QMetaObject::Connection c : m_edit_connection) {
		disconnect(c);
	}
	m_edit_connection.clear();
	m_edit_connection << connect(ui->m_x_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &IndiTextPropertiesWidget::apply);
	m_edit_connection << connect(ui->m_y_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &IndiTextPropertiesWidget::apply);
	m_edit_connection << connect(ui->m_angle_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &IndiTextPropertiesWidget::apply);
	m_edit_connection << connect(ui->m_line_edit, &QLineEdit::textEdited, this, &IndiTextPropertiesWidget::apply);
	m_edit_connection << connect(ui->m_size_sb, QOverload<int>::of(&QSpinBox::valueChanged), this, &IndiTextPropertiesWidget::apply);
}

/**
 * @brief IndiTextPropertiesWidget::updateUi
 */
void IndiTextPropertiesWidget::updateUi()
{
	if (!m_text) {
		return;
	}

		//Disconnect every connections of editor widgets
		//to avoid an unwanted edition (QSpinBox emit valueChanged no matter if changer by user or by program)
	for (QMetaObject::Connection c : m_edit_connection) {
		disconnect(c);
	}
	m_edit_connection.clear();
	
	ui->m_x_sb->setValue(m_text->pos().x());
	ui->m_y_sb->setValue(m_text->pos().y());
	ui->m_angle_sb->setValue(m_text->rotation());
	ui->m_line_edit->setText(m_text->toPlainText());
	ui->m_size_sb->setValue(m_text->fontSize());
	
	ui->m_line_edit->setDisabled(m_text->isHtml() ? true : false);
	ui->m_size_sb->setDisabled(m_text->isHtml() ? true : false);
	
		//Set the connection now
	setUpEditConnection();
}

/**
 * @brief IndiTextPropertiesWidget::on_m_advanced_editor_pb_clicked
 */
void IndiTextPropertiesWidget::on_m_advanced_editor_pb_clicked() {
	if (m_text) {
		m_text->edit();
	}
}
