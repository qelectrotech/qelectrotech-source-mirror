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
#include "dynamictextfieldeditor.h"
#include "ui_dynamictextfieldeditor.h"
#include "customelementpart.h"
#include "partdynamictextfield.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "qetelementeditor.h"
#include "qetapp.h"
#include "compositetexteditdialog.h"
#include "alignmenttextdialog.h"
#include <assert.h>

#include <QPointer>
#include <QGraphicsItem>
#include <QColorDialog>

DynamicTextFieldEditor::DynamicTextFieldEditor(
	QETElementEditor *editor, PartDynamicTextField *text_field, QWidget *parent) :
	ElementItemEditor(editor, parent),
	ui(new Ui::DynamicTextFieldEditor) {
	ui -> setupUi(this);
	ui -> m_composite_text_pb -> setDisabled(true);
	ui -> m_elmt_info_cb -> setDisabled(true);
	if(text_field) {
		setPart(text_field);
	}
	fillInfoComboBox();
}

DynamicTextFieldEditor::~DynamicTextFieldEditor()
{
	delete ui;
	if(!m_connection_list.isEmpty()) {
		for(const QMetaObject::Connection& con : m_connection_list) {
			disconnect(con);
		}
	}
}

/**
	@brief DynamicTextFieldEditor::setPart
	Set part as current edited part of this widget.
	@param part
	@return true if part can be edited by this widget
*/
bool DynamicTextFieldEditor::setPart(CustomElementPart *part) {
	disconnectConnections();

	QGraphicsItem *qgi = part -> toItem();
	if(!qgi) {
		return false;
	}
	else if (qgi -> type() != PartDynamicTextField::Type) {
		return false;
	}

	m_text_field = static_cast<PartDynamicTextField *>(qgi);
	updateForm();
	setUpConnections();
	return true;
}

bool DynamicTextFieldEditor::setParts(QList <CustomElementPart *> parts) {
	if (parts.isEmpty()) {
		m_parts.clear();
		if (m_text_field) {
			disconnectConnections();
		}
		m_text_field = nullptr;
		return true;
	}

	if (PartDynamicTextField *part= static_cast<PartDynamicTextField *>(parts.first())) {
		if (m_text_field) {
			disconnectConnections();
		}

		m_text_field = part;
		m_parts.clear();
		m_parts.append(part);
		for (int i=1; i < parts.length(); i++)
			m_parts.append(static_cast<PartDynamicTextField*>(parts[i]));

		setUpConnections();
		updateForm();
		return true;
	}
	return(false);
}

/**
	@brief DynamicTextFieldEditor::currentPart
	@return The current edited part, note they can return nullptr if
	there is not a currently edited part.
*/
CustomElementPart *DynamicTextFieldEditor::currentPart() const
{
	return m_text_field.data();
}

QList<CustomElementPart*> DynamicTextFieldEditor::currentParts() const
{
	QList<CustomElementPart*> parts;
	for (auto part: m_parts) {
		parts.append(static_cast<CustomElementPart*>(part));
	}
	return parts;
}

void DynamicTextFieldEditor::updateForm()
{
	if(m_text_field) {
		ui -> m_x_sb -> setValue(m_text_field.data() -> x());
		ui -> m_y_sb -> setValue(m_text_field.data() ->y ());
		ui -> m_rotation_sb -> setValue(QET::correctAngle(m_text_field.data() -> rotation()));
		ui -> m_frame_cb -> setChecked(m_text_field.data() -> frame());
		ui -> m_user_text_le -> setText(m_text_field.data() -> text());
		ui -> m_size_sb -> setValue(m_text_field.data() -> font().pointSize());
		ui -> m_color_kpb -> setColor(m_text_field.data() -> color());
		ui -> m_width_sb -> setValue(m_text_field.data() -> textWidth());
		ui -> m_font_pb -> setText(m_text_field -> font().family());

		switch (m_text_field.data() -> textFrom()) {
			case DynamicElementTextItem::UserText: {
				ui -> m_text_from_cb -> setCurrentIndex(0);
				break;
			}
			case DynamicElementTextItem::ElementInfo: {
				ui -> m_text_from_cb -> setCurrentIndex(1);
				ui -> m_elmt_info_cb -> setCurrentIndex(
					ui -> m_elmt_info_cb -> findData(m_text_field.data() -> infoName()));
				break;
			}
			case DynamicElementTextItem::CompositeText: {
				ui -> m_text_from_cb -> setCurrentIndex(2);
			}
		}

		on_m_text_from_cb_activated(ui -> m_text_from_cb -> currentIndex()); //For enable the good widget
	}
}

void DynamicTextFieldEditor::setUpConnections()
{
	assert(m_connection_list.isEmpty());
	//Setup the connection
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::colorChanged,
		[this](){this -> updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::fontChanged,
		[this](){this -> updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::taggChanged,
		[this](){this -> updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::textFromChanged,
		[this](){this -> updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::textChanged,
		[this](){this -> updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::infoNameChanged,
		[this](){this -> updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::rotationChanged,
		[this](){this -> updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::frameChanged,
		[this](){this -> updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::textWidthChanged,
		[this](){this -> updateForm();});
	m_connection_list << connect(m_text_field.data(), &PartDynamicTextField::compositeTextChanged,
		[this](){this -> updateForm();});
}

void DynamicTextFieldEditor::disconnectConnections()
{
	//Remove previous connection
	if(!m_connection_list.isEmpty())
		for(const QMetaObject::Connection& con : m_connection_list) {
			disconnect(con);
		}
	m_connection_list.clear();
}

/**
	@brief DynamicTextFieldEditor::fillInfoComboBox
	Fill the combo box "element information"
*/
void DynamicTextFieldEditor::fillInfoComboBox()
{
	ui -> m_elmt_info_cb -> clear();

	QStringList strl;
	QString type = elementEditor() -> elementScene() -> elementType();

	if(type.contains("report")) {
		strl << "function" << "tension_protocol" << "conductor_color" << "conductor_section";
	}
	else {
		strl = QETApp::elementInfoKeys();
	}
		//We use a QMap because the keys of the map are sorted, then no matter the curent local,
		//the value of the combo box are always alphabetically sorted
	QMap <QString, QString> info_map;
	for(const QString& str : strl)
		info_map.insert(QETApp::elementTranslatedInfoKey(str), str);

	for (const QString& key : info_map.keys())
		ui -> m_elmt_info_cb -> addItem(key, info_map.value(key));
}

void DynamicTextFieldEditor::on_m_x_sb_editingFinished()
{
	double value = ui -> m_x_sb -> value();
	for (int i = 0; i < m_parts.length(); i++) {
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "x", m_parts[i] -> x(), value);
		undo -> setText(tr("Déplacer un champ texte"));
		undo -> enableAnimation(true);
		undoStack().push(undo);
	}
}

void DynamicTextFieldEditor::on_m_y_sb_editingFinished()
{
	double value = ui -> m_y_sb -> value();
	for (int i = 0; i < m_parts.length(); i++) {
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "y", m_parts[i] -> y(), value);
		undo -> setText(tr("Déplacer un champ texte"));
		undo -> enableAnimation(true);
		undoStack().push(undo);
	}
}

void DynamicTextFieldEditor::on_m_rotation_sb_editingFinished()
{
	int value = ui -> m_rotation_sb -> value();
	for (int i = 0; i < m_parts.length(); i++) {
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "rotation", m_parts[i] -> rotation(), value);
		undo -> setText(tr("Pivoter un champ texte"));
		undo -> enableAnimation(true);
		undoStack().push(undo);
	}
}

void DynamicTextFieldEditor::on_m_user_text_le_editingFinished()
{
	QString text = ui -> m_user_text_le -> text();
	for (int i = 0; i < m_parts.length(); i++) {
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "text", m_parts[i] -> text(), text);
		undo -> setText(tr("Modifier le texte d'un champ texte"));
		undoStack().push(undo);
	}
}

void DynamicTextFieldEditor::on_m_size_sb_editingFinished()
{
	QFont font_ = m_text_field -> font();
	font_.setPointSize(ui -> m_size_sb -> value());
	for (int i = 0; i < m_parts.length(); i++) {
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "font", m_parts[i] -> font(), font_);
		undo -> setText(tr("Modifier la police d'un champ texte"));
		undoStack().push(undo);
	}
}

void DynamicTextFieldEditor::on_m_frame_cb_clicked()
{
	bool frame = ui -> m_frame_cb -> isChecked();

	for (int i = 0; i < m_parts.length(); i++) {
		if(frame != m_parts[i] -> frame()) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "frame", m_parts[i] -> frame(), frame);
			undo -> setText(tr("Modifier le cadre d'un champ texte"));
			undoStack().push(undo);
		}
	}
}

void DynamicTextFieldEditor::on_m_width_sb_editingFinished()
{
	qreal width = (qreal)ui -> m_width_sb -> value();

	for (int i = 0; i < m_parts.length(); i++) {
		if(width != m_parts[i] -> textWidth()) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "textWidth", m_parts[i] -> textWidth(), width);
			undo -> setText(tr("Modifier la largeur d'un texte"));
			undoStack().push(undo);
		}
	}
}

void DynamicTextFieldEditor::on_m_elmt_info_cb_activated(const QString &arg1) {
	Q_UNUSED(arg1)

	QString info = ui -> m_elmt_info_cb -> currentData().toString();
	for (int i = 0; i < m_parts.length(); i++) {
		if(info != m_parts[i] -> infoName()) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "infoName", m_parts[i] -> infoName(), info);
			undo->setText(tr("Modifier l'information d'un texte"));
			undoStack().push(undo);
			m_parts[i] -> setPlainText(
				elementEditor() -> elementScene() -> elementInformation().value(m_parts[i] -> infoName()).toString());
		}
	}
}

void DynamicTextFieldEditor::on_m_text_from_cb_activated(int index) {
	ui -> m_user_text_le -> setDisabled(true);
	ui -> m_elmt_info_cb -> setDisabled(true);
	ui -> m_composite_text_pb -> setDisabled(true);

	if(index == 0) {
		ui->m_user_text_le->setEnabled(true);
	}
	else if (index == 1) {
		ui->m_elmt_info_cb->setEnabled(true);
	}
	else {
		ui->m_composite_text_pb->setEnabled(true);
	}

	DynamicElementTextItem::TextFrom tf;
	if(index == 0) {
		tf = DynamicElementTextItem::UserText;
	}
	else if(index == 1) {
		tf = DynamicElementTextItem::ElementInfo;
	}
	else {
		tf = DynamicElementTextItem::CompositeText;
	}

	for (int i = 0; i < m_parts.length(); i++) {
		if(tf != m_parts[i] -> textFrom()) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "textFrom", m_parts[i] -> textFrom(), tf);
			undo -> setText(tr("Modifier la source de texte, d'un texte"));
			undoStack().push(undo);
		}
	}
}

void DynamicTextFieldEditor::on_m_composite_text_pb_clicked()
{
	CompositeTextEditDialog ctd(m_text_field.data() -> compositeText(), this);
	if(ctd.exec()) {
		QString ct = ctd.plainText();
		for (int i = 0; i < m_parts.length(); i++) {
			if(ct != m_parts[i] -> compositeText()) {
				QPropertyUndoCommand *undo =\
					new QPropertyUndoCommand(m_parts[i], "compositeText", m_parts[i] -> compositeText(), ctd.plainText());
				undoStack().push(undo);
			}
		}
	}
}

void DynamicTextFieldEditor::on_m_alignment_pb_clicked()
{
	AlignmentTextDialog atd(m_text_field.data() -> alignment(), this);
	atd.exec();

	for (int i = 0; i < m_parts.length(); i++) {
		if(atd.alignment() != m_parts[i] -> alignment()) {
			QPropertyUndoCommand *undo =\
				new QPropertyUndoCommand(
					m_parts[i], "alignment", QVariant(m_parts[i] -> alignment()), QVariant(atd.alignment()));
			undo -> setText(tr("Modifier l'alignement d'un champ texte"));
			undoStack().push(undo);
		}
	}
}

void DynamicTextFieldEditor::on_m_font_pb_clicked()
{
	bool ok;
	QFont font_ = QFontDialog::getFont(&ok, m_text_field -> font(), this);
	if (ok && font_ != this -> font()) {
		ui -> m_font_pb -> setText(font_.family());
		ui -> m_size_sb -> setValue(font_.pointSize());

		for (int i = 0; i < m_parts.length(); i++) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "font", m_parts[i] -> font(), font_);
			undo -> setText(tr("Modifier la police d'un champ texte"));
			undoStack().push(undo);
		}
	}
}

void DynamicTextFieldEditor::on_m_color_kpb_changed(const QColor &newColor) {
	if (!newColor.isValid()) {
		return;
	}

	for (int i = 0; i < m_parts.length(); i++) {
		if(newColor != m_parts[i] -> color()) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "color", m_parts[i] -> color(), newColor);
			undo -> setText(tr("Modifier la couleur d'un champ texte"));
			undoStack().push(undo);
		}
	}
}
