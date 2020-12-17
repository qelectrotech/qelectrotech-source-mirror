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

#include "../../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../../editor/graphicspart/customelementpart.h"
#include "../../editor/graphicspart/partdynamictextfield.h"
#include "../../qetapp.h"
#include "../../qetinformation.h"
#include "../../ui/alignmenttextdialog.h"
#include "../../ui/compositetexteditdialog.h"
#include "../qetelementeditor.h"

#include <QColorDialog>
#include <QFrame>
#include <QGraphicsItem>
#include <QGridLayout>
#include <QLabel>
#include <QPointer>
#include <QSpacerItem>
#include <assert.h>

DynamicTextFieldEditor::DynamicTextFieldEditor(
	QETElementEditor*	  editor,
	PartDynamicTextField* text_field,
	QWidget*			  parent) :
	ElementItemEditor(editor, parent)
{
	setUpWidget(parent);
	if (text_field) { setPart(text_field); }
}

DynamicTextFieldEditor::~DynamicTextFieldEditor()
{
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
	fillInfoComboBox();
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

	if (PartDynamicTextField *part = static_cast<PartDynamicTextField *>(parts.first())) {
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
		fillInfoComboBox();
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
		m_x_sb->setValue(m_text_field.data()->x());
		m_y_sb->setValue(m_text_field.data()->y());
		m_rotation_sb->setValue(
			QET::correctAngle(m_text_field.data()->rotation()));
		m_frame_cb->setChecked(m_text_field.data()->frame());
		m_user_text_le->setText(m_text_field.data()->text());
		m_size_sb->setValue(m_text_field.data()->font().pointSize());
#ifdef BUILD_WITHOUT_KF5
#else
		m_color_kpb->setColor(m_text_field.data()->color());
#endif
		m_width_sb->setValue(m_text_field.data()->textWidth());
		m_font_pb->setText(m_text_field->font().family());

		switch (m_text_field.data() -> textFrom()) {
			case DynamicElementTextItem::UserText: {
				m_text_from_cb->setCurrentIndex(0);
				break;
			}
			case DynamicElementTextItem::ElementInfo: {
				m_text_from_cb->setCurrentIndex(1);
				m_elmt_info_cb->setCurrentIndex(
					m_elmt_info_cb->findData(m_text_field.data()->infoName()));
				break;
			}
			case DynamicElementTextItem::CompositeText: {
				m_text_from_cb->setCurrentIndex(2);
			}
		}

		on_m_text_from_cb_activated(
			m_text_from_cb->currentIndex()); // For enable the good widget
	}
}

void DynamicTextFieldEditor::setUpWidget(QWidget* parent)
{
	resize(344, 285);
	setWindowTitle(tr("Form"));

	QGridLayout* gridLayout = new QGridLayout(parent);
	gridLayout->setObjectName(QString::fromUtf8("gridLayout"));

	m_text_from_cb = new QComboBox(parent);
	m_text_from_cb->addItem(QString());
	m_text_from_cb->addItem(QString());
	m_text_from_cb->addItem(QString());
	m_text_from_cb->setObjectName(QString::fromUtf8("m_text_from_cb"));
	m_text_from_cb->setItemText(0, tr("Texte utilisateur"));
	m_text_from_cb->setItemText(
		1,
		tr("Information de l'\303\251l\303\251ment"));
	m_text_from_cb->setItemText(2, tr("Texte compos\303\251"));

	connect(
		m_text_from_cb,
		&QComboBox::currentIndexChanged,
		this,
		&DynamicTextFieldEditor::on_m_text_from_cb_activated);

	gridLayout->addWidget(m_text_from_cb, 2, 1, 1, 2);

	QLabel* label_3 = new QLabel(tr("Couleur"), parent);
	label_3->setObjectName(QString::fromUtf8("label_3"));

	gridLayout->addWidget(label_3, 7, 0, 1, 1);

	QSpacerItem* verticalSpacer =
		new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

	gridLayout->addItem(verticalSpacer, 8, 1, 1, 1);

	m_elmt_info_cb = new QComboBox(parent);
	m_elmt_info_cb->setObjectName(QString::fromUtf8("m_elmt_info_cb"));
	m_elmt_info_cb->setDisabled(true);

	connect(
		m_elmt_info_cb,
		&QComboBox::currentTextChanged,
		this,
		&DynamicTextFieldEditor::on_m_elmt_info_cb_activated);

	gridLayout->addWidget(m_elmt_info_cb, 4, 1, 1, 2);

	m_user_text_le = new QLineEdit(parent);
	m_user_text_le->setObjectName(QString::fromUtf8("m_user_text_le"));

	connect(
		m_user_text_le,
		&QLineEdit::editingFinished,
		this,
		&DynamicTextFieldEditor::on_m_user_text_le_editingFinished);

	gridLayout->addWidget(m_user_text_le, 3, 1, 1, 2);

	m_composite_text_pb = new QPushButton(tr("Texte compos\303\251"), parent);
	m_composite_text_pb->setObjectName(
		QString::fromUtf8("m_composite_text_pb"));
	m_composite_text_pb->setDisabled(true);

	connect(
		m_composite_text_pb,
		&QPushButton::clicked,
		this,
		&DynamicTextFieldEditor::on_m_composite_text_pb_clicked);

	gridLayout->addWidget(m_composite_text_pb, 5, 1, 1, 2);

	QLabel* label = new QLabel(tr("Police"), parent);
	label->setObjectName(QString::fromUtf8("label"));

	gridLayout->addWidget(label, 6, 0, 1, 1);

	QLabel* label_4 = new QLabel(tr("Source du texte"), parent);
	label_4->setObjectName(QString::fromUtf8("label_4"));

	gridLayout->addWidget(label_4, 2, 0, 1, 1);

	QSpacerItem* verticalSpacer_2 =
		new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

	gridLayout->addItem(verticalSpacer_2, 1, 0, 1, 1);

	QFrame* frame = new QFrame(parent);
	frame->setObjectName(QString::fromUtf8("frame"));
	frame->setFrameShape(QFrame::NoFrame);
	frame->setFrameShadow(QFrame::Raised);

	QGridLayout* gridLayout_2 = new QGridLayout(frame);
	gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));

	m_x_sb = new QDoubleSpinBox(frame);
	m_x_sb->setObjectName(QString::fromUtf8("m_x_sb"));
	m_x_sb->setMinimum(-5000.000000000000000);
	m_x_sb->setMaximum(5000.000000000000000);

	connect(
		m_x_sb,
		&QDoubleSpinBox::editingFinished,
		this,
		&DynamicTextFieldEditor::on_m_x_sb_editingFinished);

	gridLayout_2->addWidget(m_x_sb, 0, 1, 1, 1);

	m_frame_cb = new QCheckBox(tr("cadre"), frame);
	m_frame_cb->setObjectName(QString::fromUtf8("m_frame_cb"));

	connect(
		m_frame_cb,
		&QCheckBox::clicked,
		this,
		&DynamicTextFieldEditor::on_m_frame_cb_clicked);

	gridLayout_2->addWidget(m_frame_cb, 0, 6, 1, 1);

	m_rotation_sb = new QSpinBox(frame);
	m_rotation_sb->setObjectName(QString::fromUtf8("m_rotation_sb"));
	m_rotation_sb->setMaximum(359);

	connect(
		m_rotation_sb,
		&QSpinBox::editingFinished,
		this,
		&DynamicTextFieldEditor::on_m_rotation_sb_editingFinished);

	gridLayout_2->addWidget(m_rotation_sb, 0, 5, 1, 1);

	QLabel* label_7 = new QLabel(tr("Rotation"), frame);
	label_7->setObjectName(QString::fromUtf8("label_7"));
	label_7->setAlignment(
		Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

	gridLayout_2->addWidget(label_7, 0, 4, 1, 1);

	QLabel* label_5 = new QLabel(tr("X"), frame);
	label_5->setObjectName(QString::fromUtf8("label_5"));
	label_5->setAlignment(
		Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

	gridLayout_2->addWidget(label_5, 0, 0, 1, 1);

	m_width_sb = new QSpinBox(frame);
	m_width_sb->setObjectName(QString::fromUtf8("m_width_sb"));
	m_width_sb->setMinimum(-1);
	m_width_sb->setMaximum(500);

	connect(
		m_width_sb,
		&QSpinBox::editingFinished,
		this,
		&DynamicTextFieldEditor::on_m_width_sb_editingFinished);

	gridLayout_2->addWidget(m_width_sb, 1, 5, 1, 1);

	QLabel* label_2 = new QLabel(tr("Largeur"), frame);
	label_2->setObjectName(QString::fromUtf8("label_2"));
	label_2->setAlignment(
		Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

	gridLayout_2->addWidget(label_2, 1, 4, 1, 1);

	QLabel* label_6 = new QLabel(tr("Y"), frame);
	label_6->setObjectName(QString::fromUtf8("label_6"));
	label_6->setAlignment(
		Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);

	gridLayout_2->addWidget(label_6, 1, 0, 1, 1);

	m_y_sb = new QDoubleSpinBox(frame);
	m_y_sb->setObjectName(QString::fromUtf8("m_y_sb"));
	m_y_sb->setMinimum(-5000.000000000000000);
	m_y_sb->setMaximum(5000.000000000000000);

	connect(
		m_y_sb,
		&QDoubleSpinBox::editingFinished,
		this,
		&DynamicTextFieldEditor::on_m_y_sb_editingFinished);

	gridLayout_2->addWidget(m_y_sb, 1, 1, 1, 1);

	QPushButton* m_alignment_pb = new QPushButton(tr("Alignement"), frame);
	m_alignment_pb->setObjectName(QString::fromUtf8("m_alignment_pb"));

	connect(
		m_alignment_pb,
		&QPushButton::clicked,
		this,
		&DynamicTextFieldEditor::on_m_alignment_pb_clicked);

	gridLayout_2->addWidget(m_alignment_pb, 1, 6, 1, 1);

	gridLayout->addWidget(frame, 0, 0, 1, 3);

	m_size_sb = new QSpinBox(parent);
	m_size_sb->setObjectName(QString::fromUtf8("m_size_sb"));

	connect(
		m_size_sb,
		&QSpinBox::editingFinished,
		this,
		&DynamicTextFieldEditor::on_m_size_sb_editingFinished);

	gridLayout->addWidget(m_size_sb, 6, 1, 1, 1);

	m_font_pb = new QPushButton(QString(), parent);
	m_font_pb->setObjectName(QString::fromUtf8("m_font_pb"));

	connect(
		m_font_pb,
		&QPushButton::clicked,
		this,
		&DynamicTextFieldEditor::on_m_font_pb_clicked);

	gridLayout->addWidget(m_font_pb, 6, 2, 1, 1);

#ifdef BUILD_WITHOUT_KF5
#else
	m_color_kpb = new KColorButton(parent);
	m_color_kpb->setObjectName(QString::fromUtf8("m_color_kpb"));

	connect(
		m_color_kpb,
		&KColorButton::changed,
		this,
		&DynamicTextFieldEditor::on_m_color_kpb_changed);

	gridLayout->addWidget(m_color_kpb, 7, 1, 1, 2);
#endif
	setLayout(gridLayout);
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
	m_elmt_info_cb->clear();

	QStringList strl;
	QString type = elementEditor() -> elementScene() -> elementType();

	if(type.contains("report")) {
		strl = QETInformation::folioReportInfoKeys();
	}
	else {
		strl = QETInformation::elementInfoKeys();
	}
		//We use a QMap because the keys of the map are sorted, then no matter the curent local,
		//the value of the combo box are always alphabetically sorted
	QMap <QString, QString> info_map;
	for(const QString& str : strl)
		info_map.insert(QETInformation::translatedInfoKey(str), str);


	for (const QString& key : info_map.keys())
		m_elmt_info_cb->addItem(key, info_map.value(key));
}

void DynamicTextFieldEditor::on_m_x_sb_editingFinished()
{
	double value = m_x_sb->value();
	for (int i = 0; i < m_parts.length(); i++) {
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "x", m_parts[i] -> x(), value);
		undo -> setText(tr("Déplacer un champ texte"));
		undo -> enableAnimation(true);
		undoStack().push(undo);
	}
}

void DynamicTextFieldEditor::on_m_y_sb_editingFinished()
{
	double value = m_y_sb->value();
	for (int i = 0; i < m_parts.length(); i++) {
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "y", m_parts[i] -> y(), value);
		undo -> setText(tr("Déplacer un champ texte"));
		undo -> enableAnimation(true);
		undoStack().push(undo);
	}
}

void DynamicTextFieldEditor::on_m_rotation_sb_editingFinished()
{
	int value = m_rotation_sb->value();
	for (int i = 0; i < m_parts.length(); i++) {
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "rotation", m_parts[i] -> rotation(), value);
		undo -> setText(tr("Pivoter un champ texte"));
		undo -> enableAnimation(true);
		undoStack().push(undo);
	}
}

void DynamicTextFieldEditor::on_m_user_text_le_editingFinished()
{
	QString text = m_user_text_le->text();
	for (int i = 0; i < m_parts.length(); i++) {
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "text", m_parts[i] -> text(), text);
		undo -> setText(tr("Modifier le texte d'un champ texte"));
		undoStack().push(undo);
	}
}

void DynamicTextFieldEditor::on_m_size_sb_editingFinished()
{
	QFont font_ = m_text_field -> font();
	font_.setPointSize(m_size_sb->value());
	for (int i = 0; i < m_parts.length(); i++) {
		QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "font", m_parts[i] -> font(), font_);
		undo -> setText(tr("Modifier la police d'un champ texte"));
		undoStack().push(undo);
	}
}

void DynamicTextFieldEditor::on_m_frame_cb_clicked()
{
	bool frame = m_frame_cb->isChecked();

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
	qreal width = (qreal) m_width_sb->value();

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

	QString info = m_elmt_info_cb->currentData().toString();
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
	m_user_text_le->setDisabled(true);
	m_elmt_info_cb->setDisabled(true);
	m_composite_text_pb->setDisabled(true);

	if (index == 0) { m_user_text_le->setEnabled(true); }
	else if (index == 1) {
		m_elmt_info_cb->setEnabled(true);
	}
	else {
		m_composite_text_pb->setEnabled(true);
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
		m_font_pb->setText(font_.family());
		m_size_sb->setValue(font_.pointSize());

		for (int i = 0; i < m_parts.length(); i++) {
			QPropertyUndoCommand *undo = new QPropertyUndoCommand(m_parts[i], "font", m_parts[i] -> font(), font_);
			undo -> setText(tr("Modifier la police d'un champ texte"));
			undoStack().push(undo);
		}
	}
}

#ifdef BUILD_WITHOUT_KF5
#else
void DynamicTextFieldEditor::on_m_color_kpb_changed(const QColor& newColor)
{
	if (! newColor.isValid()) { return; }

	for (int i = 0; i < m_parts.length(); i++)
	{
		if (newColor != m_parts[i]->color())
		{
			QPropertyUndoCommand* undo = new QPropertyUndoCommand(
				m_parts[i],
				"color",
				m_parts[i]->color(),
				newColor);
			undo->setText(tr("Modifier la couleur d'un champ texte"));
			undoStack().push(undo);
		}
	}
}
#endif
