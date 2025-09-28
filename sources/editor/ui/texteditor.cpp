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
#include "texteditor.h"

#include "../../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../graphicspart/parttext.h"

#include <cassert>

/**
	@brief TextEditor::TextEditor
	Default constructor
	@param editor : the element editor who use this editor
	@param text : the text to edit
	@param parent : the parent widget
*/
TextEditor::TextEditor(QETElementEditor *editor,  PartText *text, QWidget *parent) :
	ElementItemEditor(editor, parent){
	setUpWidget(parent);
	setUpEditConnection();
	if (text) {
		setPart(text);
		updateForm();
	}
}

/**
	@brief TextEditor::~TextEditor
*/
TextEditor::~TextEditor() {}

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

	m_line_edit -> setText(m_text -> toPlainText());
	m_x_sb -> setValue(m_text -> pos().x());
	m_y_sb -> setValue(m_text -> pos().y());
	m_rotation_sb -> setValue(m_text -> rotation());
	m_size_sb -> setValue(m_text -> font().pointSize());
	m_font_pb -> setText(m_text -> font().family());
	m_color_pb -> setColor(m_text -> defaultTextColor());

	setUpEditConnection();
}

void TextEditor::setUpChangeConnection(QPointer<PartText> part)
{
	disconnectChangeConnection();

	m_change_connection << connect(part, &PartText::plainTextChanged, this, &TextEditor::updateForm);
	m_change_connection << connect(part, &PartText::xChanged,         this, &TextEditor::updateForm);
	m_change_connection << connect(part, &PartText::yChanged,         this, &TextEditor::updateForm);
	m_change_connection << connect(part, &PartText::rotationChanged,  this, &TextEditor::updateForm);
	m_change_connection << connect(part, &PartText::fontChanged,      this, &TextEditor::updateForm);
	m_change_connection << connect(part, &PartText::colorChanged,     this, &TextEditor::updateForm);
}

void TextEditor::disconnectChangeConnection()
{
	for (const auto &connection : std::as_const(m_change_connection)) {
		disconnect(connection);
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

bool TextEditor::setParts(QList <CustomElementPart *> parts)
{
	if (parts.isEmpty())
	{
		m_parts.clear();
		disconnectChangeConnection();
		m_text = nullptr;
		return true;
	}

	if (PartText *part = static_cast<PartText *>(parts.first()))
	{
		disconnectChangeConnection();
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

	m_edit_connection << connect(m_line_edit, &QLineEdit::editingFinished, [this]() {
		QString text_ = m_line_edit -> text();
		for (int i=0; i < m_parts.length(); i++) {
			PartText* partText = m_parts[i];
			if (text_ != partText -> toPlainText()) {
				QPropertyUndoCommand *undo = new QPropertyUndoCommand(partText, "text", partText -> toPlainText(), text_);
				undo -> setText(tr("Modifier le contenu d'un champ texte"));
				undoStack().push(undo);
			}
		}
	});

	m_edit_connection << connect(m_x_sb, QOverload<int>::of(&QSpinBox::valueChanged), [this]() {
		QPointF pos(m_x_sb -> value(), 0);
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
		m_x_sb->setFocus();
	});

	m_edit_connection << connect(m_y_sb, QOverload<int>::of(&QSpinBox::valueChanged), [this]() {
		QPointF pos(0, m_y_sb -> value());
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
		m_y_sb->setFocus();
	});

	m_edit_connection << connect(m_rotation_sb, QOverload<int>::of(&QSpinBox::valueChanged), [this]() {
		for (int i=0; i < m_parts.length(); i++) {
			PartText* partText = m_parts[i];
			if (m_rotation_sb -> value() != partText -> rotation()) {
				QPropertyUndoCommand *undo = new QPropertyUndoCommand(
					partText, "rotation", partText -> rotation(), m_rotation_sb -> value());
				undo -> setText(tr("Pivoter un champ texte"));
				undo -> setAnimated(true, false);
				undoStack().push(undo);
			}
		}
		m_rotation_sb->setFocus();
	});

	m_edit_connection << connect(m_size_sb, QOverload<int>::of(&QSpinBox::valueChanged), [this]() {
		for (int i=0; i < m_parts.length(); i++) {
			PartText* partText = m_parts[i];
			if (partText -> font().pointSize() !=m_size_sb -> value()) {
				QFont font_ = partText -> font();
				font_.setPointSize(m_size_sb -> value());
				QPropertyUndoCommand *undo = new QPropertyUndoCommand(partText, "font", partText -> font(), font_);
				undo -> setText(tr("Modifier la police d'un texte"));
				undoStack().push(undo);
			}
		}
		m_size_sb->setFocus();
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
		m_size_sb -> blockSignals(true);
		m_size_sb -> setValue(font_.pointSize());
		m_size_sb -> blockSignals(false);

		m_font_pb -> setText(font_.family());
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

void TextEditor::setUpWidget(QWidget *parent)
{
	setWindowTitle(tr("Form"));
	resize(378, 133);
	QGridLayout *gridLayout = new QGridLayout(parent);
	gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
	m_y_sb = new QSpinBox(parent);
	m_y_sb->setObjectName(QString::fromUtf8("m_y_sb"));
	m_y_sb->setMinimum(-10000);
	m_y_sb->setMaximum(10000);

	gridLayout->addWidget(m_y_sb, 1, 3, 1, 1);

	QLabel*label_2 = new QLabel(tr("Y :"),parent);

	gridLayout->addWidget(label_2, 1, 2, 1, 1);

	QLabel*label_4 = new QLabel(tr("Police :"),parent);

	gridLayout->addWidget(label_4, 2, 0, 1, 1);

	m_rotation_sb = new QSpinBox(parent);
	m_rotation_sb->setSuffix(tr("\302\260"));
	m_rotation_sb->setWrapping(true);
	m_rotation_sb->setMaximum(360);

	gridLayout->addWidget(m_rotation_sb, 1, 5, 1, 1);

	QLabel *label_3 = new QLabel(tr("Rotation :"),parent);

	gridLayout->addWidget(label_3, 1, 4, 1, 1);

	m_x_sb = new QSpinBox(parent);
	m_x_sb->setObjectName(QString::fromUtf8("m_x_sb"));
	m_x_sb->setMinimum(-10000);
	m_x_sb->setMaximum(10000);

	gridLayout->addWidget(m_x_sb, 1, 1, 1, 1);

	QLabel *label = new QLabel(tr("X :"),parent);

	gridLayout->addWidget(label, 1, 0, 1, 1);

	m_size_sb = new QSpinBox(parent);
	m_size_sb->setObjectName(QString::fromUtf8("m_size_sb"));

	gridLayout->addWidget(m_size_sb, 2, 1, 1, 1);

	m_line_edit = new QLineEdit(parent);
	m_line_edit->setObjectName(QString::fromUtf8("m_line_edit"));
	m_line_edit->setClearButtonEnabled(true);
	m_line_edit->setPlaceholderText(tr("Entrer votre texte ici"));

	gridLayout->addWidget(m_line_edit, 0, 0, 1, 6);
	m_color_pb = new ColorButton(parent);
	m_color_pb->setObjectName(QString::fromUtf8("m_color_pb"));

	connect(
		m_color_pb,
		&ColorButton::changed,
		this,
		&TextEditor::on_m_color_pb_changed);

	gridLayout->addWidget(m_color_pb, 2, 5, 1, 1);
	QLabel *label_5 = new QLabel(parent);
	label_5->setObjectName(QString::fromUtf8("label_5"));

	gridLayout->addWidget(label_5, 2, 4, 1, 1);

	m_font_pb = new QPushButton(tr("Couleur :"),parent);
	connect(m_font_pb,
		&QPushButton::pressed,
		this,
		&TextEditor::on_m_font_pb_clicked);

	gridLayout->addWidget(m_font_pb, 2, 2, 1, 2);

	QSpacerItem *verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

	gridLayout->addItem(verticalSpacer, 3, 2, 1, 1);
	setLayout(gridLayout);
}
