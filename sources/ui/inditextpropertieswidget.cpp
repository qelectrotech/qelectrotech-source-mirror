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
 * @brief IndiTextPropertiesWidget::IndiTextPropertiesWidget
 * @param text_list : a list of texts to edit
 * @param parent : the parent widget of this widget
 */
IndiTextPropertiesWidget::IndiTextPropertiesWidget(QList<IndependentTextItem *> text_list, QWidget *parent) :
	PropertiesEditorWidget (parent),
	ui(new Ui::IndiTextPropertiesWidget)
{
	ui->setupUi(this);
	setText(text_list);
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
	m_connect_list << connect(m_text.data(), &IndependentTextItem::fontChanged, this, &IndiTextPropertiesWidget::updateUi);
	m_connect_list << connect(m_text.data(), &IndependentTextItem::textEdited, this, &IndiTextPropertiesWidget::updateUi);

	updateUi();
}

void IndiTextPropertiesWidget::setText(QList<IndependentTextItem *> text_list)
{
	for (QMetaObject::Connection c : m_connect_list) {
		disconnect(c);
	}
	m_connect_list.clear();
	m_text_list.clear();
	m_text = nullptr;
	
	if (text_list.size() == 0) {
		updateUi();
	}
	else if (text_list.size() == 1) 
	{
		setText(text_list.first());
		m_text_list.clear();
	}
	else
	{
		for (IndependentTextItem *iti : text_list) {
			m_text_list.append(QPointer<IndependentTextItem>(iti));
		}
		updateUi();
	}
}

/**
 * @brief IndiTextPropertiesWidget::apply
 * Apply the current edition through a QUndoCommand pushed
 * to the undo stack of text's diagram.
 */
void IndiTextPropertiesWidget::apply()
{
	Diagram *d = nullptr;
	
	if (m_text && m_text->diagram()) {
		d = m_text->diagram();
	} else if (!m_text_list.isEmpty()) {
		for (QPointer<IndependentTextItem> piti : m_text_list) {
			if (piti->diagram()) {
				d = piti->diagram();
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
			//One text is edited
		if (m_text_list.isEmpty())
		{
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
			if (ui->m_size_sb->value() != m_text->font().pointSize()) {
				QFont font = m_text->font();
				font.setPointSize(ui->m_size_sb->value());
				undo = new QPropertyUndoCommand(m_text.data(), "font", m_text->font(), font);
				undo->setText(tr("Modifier la taille d'un champ texte"));
			}
			
			return undo;
		}
		else //several text are edited, only size and rotation is available for edition
		{
			QUndoCommand *parent_undo = nullptr;
			bool size_equal = true;
			bool angle_equal = true;
			qreal rotation_ = m_text_list.first()->rotation();
			int size_ = m_text_list.first()->font().pointSize();
			for (QPointer<IndependentTextItem> piti : m_text_list)
			{
				if (piti->rotation() != rotation_) {
					angle_equal = false;
				}
				if (piti->font().pointSize() != size_) {
					size_equal = false;
				}
			}
				
			if ((angle_equal && (ui->m_angle_sb->value() != rotation_)) ||
				(!angle_equal && (ui->m_angle_sb->value() != ui->m_angle_sb->minimum())))
			{
				for (QPointer<IndependentTextItem> piti : m_text_list)
				{
					if (piti)
					{
						if (!parent_undo) {
							parent_undo = new QUndoCommand(tr("Pivoter plusieurs champs texte"));
						}
						QPropertyUndoCommand *qpuc = new QPropertyUndoCommand(piti.data(), "rotation", QVariant(piti->rotation()), QVariant(ui->m_angle_sb->value()), parent_undo);
						qpuc->setAnimated(true, false);
					}
				}
			}
			else if ((size_equal && (ui->m_size_sb->value() != size_)) ||
					 (!size_equal && (ui->m_size_sb->value() != ui->m_size_sb->minimum())))
			{
				for (QPointer<IndependentTextItem> piti : m_text_list)
				{
					if (piti)
					{
						if (!parent_undo) {
							parent_undo = new QUndoCommand(tr("Modifier la taille de plusieurs champs texte"));
						}
						QFont font = piti->font();
						font.setPointSize(ui->m_size_sb->value());
						QPropertyUndoCommand *qpuc = new QPropertyUndoCommand(piti.data(), "font", QVariant(piti->font()), QVariant(font), parent_undo);
						qpuc->setAnimated(true, false);
					}
				}
			}
			return parent_undo;
		}
	}
		//In mode not live edit, only one text can be edited
	else if (m_text_list.isEmpty())
	{
		QUndoCommand *undo = new QUndoCommand(tr("Modifier les propriétés d'un texte"));
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
		if (ui->m_size_sb->value() != m_text->font().pointSize())
		{
			QFont font = m_text->font();
			font.setPointSize(ui->m_size_sb->value());
			new QPropertyUndoCommand(m_text.data(), "font", m_text->font(), font, undo);
		}
		
		if (undo->childCount()) {
			return undo;
		} else {
			return nullptr;
		}
	}
	else {
		return nullptr;
	}
}

/**
 * @brief IndiTextPropertiesWidget::setUpEditConnection
 * Disconnect the previous connection, and reconnect the connection between the editors widgets and void IndiTextPropertiesWidget::apply function
 */
void IndiTextPropertiesWidget::setUpEditConnection()
{
	for (QMetaObject::Connection c : m_edit_connection) {
		disconnect(c);
	}
	m_edit_connection.clear();
	
	if (m_text_list.isEmpty())
	{
		m_edit_connection << connect(ui->m_x_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &IndiTextPropertiesWidget::apply);
		m_edit_connection << connect(ui->m_y_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &IndiTextPropertiesWidget::apply);
		m_edit_connection << connect(ui->m_line_edit, &QLineEdit::textEdited, this, &IndiTextPropertiesWidget::apply);
	}
	m_edit_connection << connect(ui->m_angle_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &IndiTextPropertiesWidget::apply);
	m_edit_connection << connect(ui->m_size_sb, QOverload<int>::of(&QSpinBox::valueChanged), this, &IndiTextPropertiesWidget::apply);
}

/**
 * @brief IndiTextPropertiesWidget::updateUi
 */
void IndiTextPropertiesWidget::updateUi()
{
	if (!m_text && m_text_list.isEmpty()) {
		return;
	}

		//Disconnect every connections of editor widgets
		//to avoid an unwanted edition (QSpinBox emit valueChanged no matter if changer by user or by program)
	for (QMetaObject::Connection c : m_edit_connection) {
		disconnect(c);
	}
	m_edit_connection.clear();
	
	ui->m_x_sb->setEnabled(m_text_list.isEmpty() ? true : false);
	ui->m_y_sb->setEnabled(m_text_list.isEmpty() ? true : false);
	ui->m_line_edit->setEnabled(m_text_list.isEmpty() ? true : false);
	ui->m_advanced_editor_pb->setEnabled(m_text_list.isEmpty() ? true : false);
	
	if (m_text_list.isEmpty())
	{
		ui->m_x_sb->setValue(m_text->pos().x());
		ui->m_y_sb->setValue(m_text->pos().y());
		ui->m_line_edit->setText(m_text->toPlainText());
		ui->m_angle_sb->setValue(m_text->rotation());
		ui->m_size_sb->setValue(m_text->font().pointSize());
		
		ui->m_line_edit->setDisabled(m_text->isHtml() ? true : false);
		ui->m_size_sb->setDisabled(m_text->isHtml() ? true : false);
		ui->m_label->setVisible(m_text->isHtml() ? true : false);
		ui->m_break_html_pb->setVisible(m_text->isHtml() ? true : false);
	}
	else
	{
		bool size_equal = true;
		bool angle_equal = true;
		qreal rotation_ = m_text_list.first()->rotation();
		int size_ = m_text_list.first()->font().pointSize();
		for (QPointer<IndependentTextItem> piti : m_text_list)
		{
			if (piti->rotation() != rotation_) {
				angle_equal = false;
			}
			if (piti->font().pointSize() != size_) {
				size_equal = false;
			}
		}
		ui->m_angle_sb->setValue(angle_equal ? rotation_ : 0);
		
		bool valid_ = true;
		for (QPointer<IndependentTextItem> piti : m_text_list) {
			if (piti->isHtml()) {
				valid_ = false;
			}
		}
		ui->m_size_sb->setEnabled(valid_);
		ui->m_size_sb->setValue(size_equal ? size_ : 0);
		ui->m_label->setVisible(false);
		ui->m_break_html_pb->setVisible(true);
	}

	
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

void IndiTextPropertiesWidget::on_m_break_html_pb_clicked()
{
    if (m_text) {
		m_text->setPlainText(m_text->toPlainText());
	}
	for (QPointer<IndependentTextItem> piti : m_text_list) {
		piti->setPlainText(piti->toPlainText());
	}
	
	updateUi();
}
