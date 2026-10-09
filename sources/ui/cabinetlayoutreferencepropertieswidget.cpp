/*
	Copyright 2006-2026 The QElectroTech Team
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

#include "cabinetlayoutreferencepropertieswidget.h"
#include "../ui_cabinetlayoutreferencepropertieswidget.h"

#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../diagram.h"
#include "../qetgraphicsitem/cabinetlayoutreferenceitem.h"
#include "../qetinformation.h"

#include <QFontDialog>

CabinetLayoutReferencePropertiesWidget::CabinetLayoutReferencePropertiesWidget(
		CabinetLayoutReferenceItem *item, QWidget *parent) :
	PropertiesEditorWidget(parent),
	ui(new Ui::CabinetLayoutReferencePropertiesWidget),
	m_item(nullptr)
{
	ui->setupUi(this);
	ui->m_box_pen_style_cb->addItem(tr("Aucune"));
	ui->m_box_pen_style_cb->addItem(tr("Continu"));
	ui->m_box_pen_style_cb->addItem(tr("Tirets"));
	ui->m_box_pen_style_cb->addItem(tr("Pointillés"));
	ui->m_box_pen_style_cb->addItem(tr("Tirets-points"));
	ui->m_box_pen_style_cb->addItem(tr("Tirets-points-points"));
	ui->m_box_pen_style_cb->addItem(tr("Tirets personnalisés"));

	ui->m_box_width_dsb->setSuffix(tr(" px"));
	ui->m_box_height_dsb->setSuffix(tr(" px"));
	ui->m_box_width_dsb->setEnabled(false);
	ui->m_box_height_dsb->setEnabled(false);
	ui->m_box_rotation_sb->setSuffix(tr(" °"));
	ui->m_text_rotation_sb->setSuffix(tr(" °"));

	populateInfoCombo();
	setItem(item);
}

CabinetLayoutReferencePropertiesWidget::~CabinetLayoutReferencePropertiesWidget()
{
	delete ui;
}

/**
	@brief CabinetLayoutReferencePropertiesWidget::populateInfoCombo
	Fills m_elmt_info_cb with every available element info key,
	exactly mirroring the element editor's DynamicTextFieldEditor
	(@see dynamictextfieldeditor.cpp) -- same source list
	(QETInformation::elementInfoKeys()), same translated display text
	with the raw key stored as item data.
*/
void CabinetLayoutReferencePropertiesWidget::populateInfoCombo()
{
	ui->m_elmt_info_cb->clear();
	for (const QString &key : QETInformation::elementInfoKeys()) {
		ui->m_elmt_info_cb->addItem(QETInformation::translatedInfoKey(key), key);
	}
}

void CabinetLayoutReferencePropertiesWidget::setItem(CabinetLayoutReferenceItem *item)
{
	clearEditConnection();
	m_item = item;
	if (!m_item)
		return;

	updateUi();
	setUpEditConnection();
}

void CabinetLayoutReferencePropertiesWidget::updateUi()
{
	if (!m_item)
		return;

	clearEditConnection();

	ui->m_box_width_dsb->setValue(m_item->boxRect().width());
	ui->m_box_height_dsb->setValue(m_item->boxRect().height());

	ui->m_box_pen_style_cb->setCurrentIndex(static_cast<int>(m_item->pen().style()));
	ui->m_box_pen_width_dsb->setValue(m_item->pen().widthF());
	ui->m_box_pen_color_kpb->setColor(m_item->pen().color());
	const bool has_line = m_item->pen().style() != Qt::NoPen;
	ui->label_box_pen_width->setVisible(has_line);
	ui->m_box_pen_width_dsb->setVisible(has_line);
	ui->m_box_pen_color_kpb->setVisible(has_line);
	ui->label_box_pen_color->setVisible(has_line);

	ui->m_box_brush_color_kpb->setColor(m_item->brush().color());

	const bool has_picture = m_item->hasLayoutPicture();
	ui->m_box_brush_color_kpb->setVisible(!has_picture);
	ui->label_box_fill_color->setVisible(!has_picture);

	ui->m_box_rotation_sb->setValue(int(m_item->rotation()));

	const int idx = ui->m_elmt_info_cb->findData(m_item->displayedInfoKey());
	ui->m_elmt_info_cb->setCurrentIndex(idx >= 0 ? idx : 0);

	ui->m_text_rotation_sb->setValue(int(m_item->textRotation()));
	m_current_font = m_item->font();
	ui->m_font_pb->setText(m_current_font.family());

	setUpEditConnection();
}

/**
	@brief CabinetLayoutReferencePropertiesWidget::on_m_font_pb_clicked
	Mirrors DynamicTextFieldEditor::on_m_font_pb_clicked() exactly --
	a single native font dialog rather than separate family/size/bold
	controls, since QFontDialog already covers all of that (including
	italic/bold selection) in one step.
*/
void CabinetLayoutReferencePropertiesWidget::on_m_font_pb_clicked()
{
	bool ok = false;
	QFont f = QFontDialog::getFont(&ok, m_current_font, this);
	if (ok) {
		m_current_font = f;
		ui->m_font_pb->setText(f.family());
		apply();
	}
}

QUndoCommand* CabinetLayoutReferencePropertiesWidget::associatedUndo() const
{
	if (!m_item)
		return nullptr;

	QPropertyUndoCommand *undo = nullptr;

	QPen old_pen = m_item->pen();
	QPen new_pen = old_pen;
	new_pen.setStyle(Qt::PenStyle(ui->m_box_pen_style_cb->currentIndex()));
	new_pen.setWidthF(ui->m_box_pen_width_dsb->value());
	new_pen.setColor(ui->m_box_pen_color_kpb->color());
	if (new_pen != old_pen)
	{
		undo = new QPropertyUndoCommand(m_item, "pen", old_pen, new_pen);
		undo->setText(tr("Modifier le trait d'une référence de disposition"));
	}

	//Used brush instead of color to later implement picture fill instead
	//of color -- QBrush already natively supports an image texture via
	//setTextureImage(), so this needs no redesign later, just an
	//additional, optional source/path in paint().
	QBrush old_brush = m_item->brush();
	QBrush new_brush = old_brush;
	new_brush.setColor(ui->m_box_brush_color_kpb->color());
	if (new_brush != old_brush)
	{
		if (undo)
			new QPropertyUndoCommand(m_item, "brush", old_brush, new_brush, undo);
		else {
			undo = new QPropertyUndoCommand(m_item, "brush", old_brush, new_brush);
			undo->setText(tr("Modifier le remplissage d'une référence de disposition"));
		}
	}

	const qreal new_box_rotation = ui->m_box_rotation_sb->value();
	if (!qFuzzyCompare(new_box_rotation, m_item->rotation()))
	{
		if (undo)
			new QPropertyUndoCommand(m_item, "rotation", m_item->rotation(), new_box_rotation, undo);
		else {
			undo = new QPropertyUndoCommand(m_item, "rotation", m_item->rotation(), new_box_rotation);
			undo->setText(tr("Modifier la rotation d'une référence de disposition"));
		}
	}

	const QString new_key = ui->m_elmt_info_cb->currentData().toString();
	if (new_key != m_item->displayedInfoKey())
	{
		if (undo)
			new QPropertyUndoCommand(m_item, "displayedInfoKey", m_item->displayedInfoKey(), new_key, undo);
		else {
			undo = new QPropertyUndoCommand(m_item, "displayedInfoKey", m_item->displayedInfoKey(), new_key);
			undo->setText(tr("Modifier la source de l'étiquette"));
		}
	}

	const qreal new_rotation = ui->m_text_rotation_sb->value();
	if (!qFuzzyCompare(new_rotation, m_item->textRotation()))
	{
		if (undo)
			new QPropertyUndoCommand(m_item, "textRotation", m_item->textRotation(), new_rotation, undo);
		else {
			undo = new QPropertyUndoCommand(m_item, "textRotation", m_item->textRotation(), new_rotation);
			undo->setText(tr("Modifier la rotation de l'étiquette"));
		}
	}

	if (m_current_font != m_item->font())
	{
		if (undo)
			new QPropertyUndoCommand(m_item, "font", m_item->font(), m_current_font, undo);
		else {
			undo = new QPropertyUndoCommand(m_item, "font", m_item->font(), m_current_font);
			undo->setText(tr("Modifier la police de l'étiquette"));
		}
	}

	return undo;
}

void CabinetLayoutReferencePropertiesWidget::apply()
{
	if (!m_item || !m_item->diagram())
		return;

	QUndoCommand *undo = associatedUndo();
	if (undo)
		m_item->diagram()->undoStack().push(undo);

	updateUi();
}

void CabinetLayoutReferencePropertiesWidget::reset()
{
	updateUi();
}

bool CabinetLayoutReferencePropertiesWidget::setLiveEdit(bool live_edit)
{
	if (live_edit == m_live_edit)
		return true;
	m_live_edit = live_edit;
	if (m_live_edit)
		setUpEditConnection();
	else
		clearEditConnection();
	return true;
}

void CabinetLayoutReferencePropertiesWidget::setUpEditConnection()
{
	clearEditConnection();
	if (!m_item)
		return;

	m_edit_connection << connect(ui->m_box_pen_style_cb, QOverload<int>::of(&QComboBox::activated),
							  this, &CabinetLayoutReferencePropertiesWidget::apply);
	m_edit_connection << connect(ui->m_box_pen_width_dsb, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
								  this, &CabinetLayoutReferencePropertiesWidget::apply);
	m_edit_connection << connect(ui->m_box_pen_color_kpb, &KColorButton::changed,
								  this, &CabinetLayoutReferencePropertiesWidget::apply);
	m_edit_connection << connect(ui->m_box_pen_style_cb, QOverload<int>::of(&QComboBox::activated),
							  this, &CabinetLayoutReferencePropertiesWidget::apply);

	m_edit_connection << connect(ui->m_box_brush_color_kpb, &KColorButton::changed,
								  this, &CabinetLayoutReferencePropertiesWidget::apply);

	m_edit_connection << connect(ui->m_box_rotation_sb, QOverload<int>::of(&QSpinBox::valueChanged),
							  this, &CabinetLayoutReferencePropertiesWidget::apply);
	m_edit_connection << connect(ui->m_elmt_info_cb, QOverload<int>::of(&QComboBox::activated),
								  this, &CabinetLayoutReferencePropertiesWidget::apply);
	m_edit_connection << connect(ui->m_text_rotation_sb, QOverload<int>::of(&QSpinBox::valueChanged),
								  this, &CabinetLayoutReferencePropertiesWidget::apply);
}

void CabinetLayoutReferencePropertiesWidget::clearEditConnection()
{
	for (const auto &c : std::as_const(m_edit_connection))
		disconnect(c);
	m_edit_connection.clear();
}