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
#include "texteditor.h"
#include "ui_texteditor.h"
#include "parttext.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include <cassert>

/**
	@brief TextEditor::TextEditor
	Default constructor
	@param editor : the element editor who use this editor
	@param text : the text to edit
	@param parent : the parent widget
*/
TextEditor::TextEditor(QETElementEditor *editor,  PartText *text, QWidget *parent) :
	ElementItemEditor(editor, parent),
	ui(new Ui::TextEditor) {
	ui -> setupUi(this);
	setUpEditConnection();
	if (text) {
		setPart(text);
		updateForm();
	}
}

/**
	@brief TextEditor::~TextEditor
*/
TextEditor::~TextEditor()
{
	delete ui;
}

/**
	@brief TextEditor::updateForm
	Update the gui
*/
void TextEditor::updateForm()
{
	if (m_text.isNull()) {
		return;
	}

	disconnectEditConnection();

	ui -> m_line_edit -> setText(m_text -> toPlainText());
	ui -> m_x_sb -> setValue(m_text -> pos().x());
	ui -> m_y_sb -> setValue(m_text -> pos().y());
	ui -> m_rotation_sb -> setValue(m_text -> rotation());
	ui -> m_size_sb -> setValue(m_text -> font().pointSize());
	ui -> m_font_pb -> setText(m_text -> font().family());
	ui -> m_color_pb -> setColor(m_text -> defaultTextColor());

	setUpEditConnection();
}

void TextEditor::setUpChangeConnection(QPointer<PartText> part) {
	assert(m_change_connection.isEmpty());
	m_change_connection << connect(part, &PartText::plainTextChanged, this, &TextEditor::updateForm);
	m_change_connection << connect(part, &PartText::xChanged,         this, &TextEditor::updateForm);
	m_change_connection << connect(part, &PartText::yChanged,         this, &TextEditor::updateForm);
	m_change_connection << connect(part, &PartText::rotationChanged,  this, &TextEditor::updateForm);
	m_change_connection << connect(part, &PartText::fontChanged,      this, &TextEditor::updateForm);
	m_change_connection << connect(part, &PartText::colorChanged,     this, &TextEditor::updateForm);
}

void TextEditor::disconnectChangeConnection()
{
	for (QMetaObject::Connection c : m_change_connection) {
		disconnect(c);
	}
	m_change_connection.clear();
}

void TextEditor::disconnectEditConnection()
{
	for (QMetaObject::Connection c : m_edit_connection) {
	disconnect(c);
	}
	m_edit_connection.clear();
}

/**
	@brief TextEditor::setPart
	Set the current text to edit.
	Set part to nullptr to clear the current text.
	@param part : part to edit
	@return if part is a partext or nullptr, else return false
*/
bool TextEditor::setPart(CustomElementPart *part) {
	if (!part) {
		m_text = nullptr;
		disconnectChangeConnection();
		return true;
	}
	
	if (PartText *part_text = static_cast<PartText *>(part)) {
		if (part_text == m_text) {
			return true;
		}
		m_text = part_text;
		setUpChangeConnection(m_text);
		updateForm();
		return true;
	}
	return false;
}

bool TextEditor::setParts(QList <CustomElementPart *> parts) {
	if (parts.isEmpty()) {
		m_parts.clear();
		if (m_text) {
			disconnectChangeConnection();
		}
		m_text = nullptr;
		return true;
	}

	if (PartText *part = static_cast<PartText *>(parts.first())) {
		if (m_text) {
			disconnectChangeConnection();
		}
		m_text = part;
		m_parts.clear();
		m_parts.append(part);
		for (int i=1; i < parts.length(); i++) {
			m_parts.append(static_cast<PartText*>(parts[i]));
		}
		setUpChangeConnection(m_text);
		updateForm();
		return true;
	}
	return(false);
}

/**
	@brief TextEditor::currentPart
	@return The current part
*/
CustomElementPart *TextEditor::currentPart() const
{
	return m_text;
}

QList<CustomElementPart*> TextEditor::currentParts() const
{
	QList<CustomElementPart*> parts;
	for (auto part: m_parts) {
		parts.append(static_cast<CustomElementPart*>(part));
	}
	return parts;
}

/**
	@brief TextEditor::setUpEditConnection
	Setup the connection between the widgets of this editor and the undo command
	use to apply the change to the edited text.
*/
void TextEditor::setUpEditConnection()
{
	disconnectEditConnection();

	m_edit_connection << connect(ui -> m_line_edit, &QLineEdit::editingFinished, [this]() {
		QString text_ = ui -> m_line_edit -> text();
		for (int i=0; i < m_parts.length(); i++) {
			PartText* partText = m_parts[i];
			if (text_ != partText -> toPlainText()) {
				QPropertyUndoCommand *undo = new QPropertyUndoCommand(partText, "text", partText -> toPlainText(), text_);
				undo -> setText(tr("Modifier le contenu d'un champ texte"));
				undoStack().push(undo);
			}
		}
	});
	m_edit_connection << connect(ui->m_x_sb, QOverload<int>::of(&QSpinBox::valueChanged), [this]() {
		QPointF pos(ui -> m_x_sb -> value(), 0);
		for (int i=0; i < m_parts.length(); i++) {
			PartText* partText = m_parts[i];
			pos.setY(partText -> pos().y());
			if (pos != partText -> pos()) {
				QPropertyUndoCommand *undo = new QPropertyUndoCommand(partText, "pos", partText -> pos(), pos);
				undo -> setText(tr("Déplacer un champ texte"));
				undo -> setAnimated(true, false);
				undoStack().push(undo);
			}
		}
	});
	m_edit_connection << connect(ui -> m_y_sb, QOverload<int>::of(&QSpinBox::valueChanged), [this]() {
		QPointF pos(0, ui -> m_y_sb -> value());
		for (int i=0; i < m_parts.length(); i++) {
			PartText* partText = m_parts[i];
			pos.setX(partText -> pos().x());
			if (pos != partText -> pos()) {
				QPropertyUndoCommand *undo = new QPropertyUndoCommand(partText, "pos", partText -> pos(), pos);
				undo -> setText(tr("Déplacer un champ texte"));
				undo -> setAnimated(true, false);
				undoStack().push(undo);
			}
		}
	});
	m_edit_connection << connect(ui -> m_rotation_sb, QOverload<int>::of(&QSpinBox::valueChanged), [this]() {
		for (int i=0; i < m_parts.length(); i++) {
			PartText* partText = m_parts[i];
			if (ui -> m_rotation_sb -> value() != partText -> rotation()) {
				QPropertyUndoCommand *undo = new QPropertyUndoCommand(
					partText, "rotation", partText -> rotation(), ui -> m_rotation_sb -> value());
				undo -> setText(tr("Pivoter un champ texte"));
				undo -> setAnimated(true, false);
				undoStack().push(undo);
			}
		}
	});
	m_edit_connection << connect(ui->m_size_sb, QOverload<int>::of(&QSpinBox::valueChanged), [this]() {
		for (int i=0; i < m_parts.length(); i++) {
			PartText* partText = m_parts[i];
			if (partText -> font().pointSize() != ui -> m_size_sb -> value()) {
				QFont font_ = partText -> font();
				font_.setPointSize(ui -> m_size_sb -> value());
				QPropertyUndoCommand *undo = new QPropertyUndoCommand(partText, "font", partText -> font(), font_);
				undo -> setText(tr("Modifier la police d'un texte"));
				undoStack().push(undo);
			}
		}
	});
}

/**
	@brief TextEditor::on_m_font_pb_clicked
*/
void TextEditor::on_m_font_pb_clicked()
{
	bool ok;
	QFont font_ = QFontDialog::getFont(&ok, m_text -> font(), this);

	if (ok && font_ != m_text -> font()) {
		ui -> m_size_sb -> blockSignals(true);
		ui -> m_size_sb -> setValue(font_.pointSize());
		ui -> m_size_sb -> blockSignals(false);

		ui -> m_font_pb -> setText(font_.family());
	}

	for (int i=0; i < m_parts.length(); i++) {
		PartText* partText = m_parts[i];
		if (ok && font_ != partText -> font()) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(partText, "font", partText -> font(), font_);
			undo -> setText(tr("Modifier la police d'un texte"));
			undoStack().push(undo);
		}
	}
}

/**
	@brief TextEditor::on_m_color_pb_changed
	@param newColor
*/
void TextEditor::on_m_color_pb_changed(const QColor &newColor) {
	for (int i=0; i < m_parts.length(); i++) {
		PartText* partText = m_parts[i];
		if (newColor != partText -> defaultTextColor()) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(
				partText, "color", partText -> defaultTextColor(), newColor);
			undo -> setText(tr("Modifier la couleur d'un texte"));
			undoStack().push(undo);
		}
	}
}
