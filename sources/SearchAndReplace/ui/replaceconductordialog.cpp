/*
	Copyright 2006-2018 The QElectroTech Team
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
#include "replaceconductordialog.h"
#include "ui_replaceconductordialog.h"
#include "searchandreplaceworker.h"

#include <QPainter>
#include <QColorDialog>

typedef SearchAndReplaceWorker sarw;

/**
 * @brief ReplaceConductorDialog::ReplaceConductorDialog
 * @param parent
 */
ReplaceConductorDialog::ReplaceConductorDialog(const ConductorProperties &properties, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ReplaceConductorDialog)
{
	ui->setupUi(this);
	initWidget();
	setProperties(properties);
}

ReplaceConductorDialog::~ReplaceConductorDialog()
{
	delete ui;
}

/**
 * @brief ReplaceConductorDialog::updatePreview
 * Update the preview for single line
 * @param b true: update from the value displayed by this widget
 *          false: update from the properties given at the constructor of this widget
 */
void ReplaceConductorDialog::updatePreview(bool b)
{
	const QRect pixmap_rect(0, 0, 96, 96);
	QPixmap pixmap(pixmap_rect.width(), pixmap_rect.height());
	QPainter painter;
	painter.begin(&pixmap);
	painter.eraseRect(pixmap_rect);
	painter.drawRect(pixmap_rect.adjusted(0,0,-1,-1));
	painter.drawLine(QLineF(0, pixmap_rect.height() / 2, pixmap_rect.width(), pixmap_rect.height() / 2));

	if (b) {
		properties().singleLineProperties.draw(&painter, QET::Horizontal, pixmap_rect);
	} else {
		m_properties.singleLineProperties.draw(&painter, QET::Horizontal, pixmap_rect);
	}

	painter.end();
	ui -> m_preview_label -> setPixmap(pixmap);
}

void ReplaceConductorDialog::setProperties(const ConductorProperties &properties)
{
	m_properties = properties;
	
	if (m_properties.type == ConductorProperties::Single) {
		ui->m_singlewire_gb->setChecked(true);
	} else {
		ui->m_multi_gb->setChecked(true);
	}
	ui->m_text_size_sb->setValue(m_properties.text_size);
	ui->m_show_text->setChecked(m_properties.m_show_text);
	sarw::setupLineEdit(ui->m_formula_le, ui->m_erase_formula_cb, m_properties.m_formula);
	sarw::setupLineEdit(ui->m_text_le, ui->m_erase_text_cb, m_properties.text);
	sarw::setupLineEdit(ui->m_function_le, ui->m_erase_function_cb, m_properties.m_function);
	sarw::setupLineEdit(ui->m_tension_protocol_le, ui->m_erase_tension_protocol_cb, m_properties.m_tension_protocol);
	switch (m_properties.m_vertical_alignment) {
		case Qt::AlignLeft: ui->m_vertical_align_cb->setCurrentIndex(1);break;
		case Qt::AlignRight: ui->m_vertical_align_cb->setCurrentIndex(2);break;
		default: ui->m_vertical_align_cb->setCurrentIndex(0); break;
	}
	switch (m_properties.m_horizontal_alignment) {
		case Qt::AlignTop: ui->m_horizontal_align_cb->setCurrentIndex(1); break;
		case Qt::AlignBottom: ui->m_horizontal_align_cb->setCurrentIndex(2); break;
		default: ui->m_horizontal_align_cb->setCurrentIndex(0); break;
	}
	ui->m_vertical_angle_sb->setValue(m_properties.verti_rotate_text);
	ui->m_horizontal_angle_sb->setValue(m_properties.horiz_rotate_text);
	if (m_properties.color.isValid())
	{
		setColorButton(m_properties.color);
		ui->m_mod_color_cb->setChecked(true);
	}
	int index = ui->m_line_style_cb->findData(QPen(m_properties.style));
	if (index != -1) {
		ui->m_line_style_cb->setCurrentIndex(index);
	}
	ui->m_second_color_gb->setChecked(m_properties.m_bicolor);
	if (m_properties.m_color_2.isValid())
	{
		setColorButton2(m_properties.m_color_2);
		ui->m_mod_color_2_cb->setChecked(true);
	}
	ui->m_color_2_dash_size_sb->setValue(m_properties.m_dash_size);
	
	ui->m_earth_cb    ->setChecked (m_properties.singleLineProperties.hasGround);
	ui->m_neutral_cb  ->setChecked (m_properties.singleLineProperties.hasNeutral);
	ui->m_pen_cb      ->setChecked (m_properties.singleLineProperties.isPen());
	ui->m_phase_cb    ->setChecked (m_properties.singleLineProperties.phasesCount());
	ui->m_phase_slider->setValue   (m_properties.singleLineProperties.phasesCount());
	
	updatePreview(false);
}

/**
 * @brief ReplaceConductorDialog::properties
 * @return the properties edited by this widget
 */
ConductorProperties ReplaceConductorDialog::properties() const
{
	ConductorProperties properties_;
	if (ui->m_multi_gb->isChecked()) {
		properties_.type = ConductorProperties::Multi;
	} else {
		properties_.type = ConductorProperties::Single;
	}
	
	properties_.text_size = ui->m_text_size_sb->value();
	properties_.m_formula = ui->m_formula_le->text();
	properties_.text = ui->m_text_le->text();
	properties_.m_show_text = ui->m_show_text->isChecked();
	properties_.m_function = ui->m_function_le->text();
	properties_.m_tension_protocol = ui->m_tension_protocol_le->text();
	switch (ui->m_vertical_align_cb->currentIndex()) {
		case 0: properties_.m_vertical_alignment = Qt::AlignAbsolute; break;
		case 1: properties_.m_vertical_alignment = Qt::AlignLeft;     break;
		case 2: properties_.m_vertical_alignment = Qt::AlignRight;    break;
		default:break;
	}
	switch (ui->m_horizontal_align_cb->currentIndex()) {
		case 0: properties_.m_horizontal_alignment = Qt::AlignAbsolute; break;
		case 1: properties_.m_horizontal_alignment = Qt::AlignTop; break;
		case 2: properties_.m_horizontal_alignment = Qt::AlignBottom; break;
		default: break;
	}
	properties_.verti_rotate_text = ui->m_vertical_angle_sb->value();
	properties_.horiz_rotate_text = ui->m_horizontal_angle_sb->value();
	properties_.color = ui->m_mod_color_cb->isChecked() ? ui->m_color_pb->palette().color(QPalette::Button) : QColor();
	properties_.style = ui->m_line_style_cb->itemData(ui->m_line_style_cb->currentIndex()).value<QPen>().style();
	properties_.cond_size = ui->m_cond_size_sb->value();
	properties_.m_bicolor = ui->m_second_color_gb->isChecked();
	properties_.m_color_2 = ui->m_mod_color_2_cb->isChecked() ? ui->m_color_2_pb->palette().color(QPalette::Button) : QColor();
	properties_.m_dash_size = ui->m_color_2_dash_size_sb->value();
	
	properties_.singleLineProperties.hasGround  = ui->m_earth_cb->isChecked();
	properties_.singleLineProperties.hasNeutral = ui->m_neutral_cb->isChecked();
	properties_.singleLineProperties.is_pen     = ui->m_pen_cb->isChecked();
	properties_.singleLineProperties.setPhasesCount(ui->m_phase_cb->isChecked() ? ui->m_phase_sb->value() : 0);
	
	return properties_;
}

void ReplaceConductorDialog::initWidget()
{
	connect(ui->m_button_box, &QDialogButtonBox::clicked, [this](QAbstractButton *button_) {
		this->done(ui->m_button_box->buttonRole(button_));
	});
	
	ui->m_update_preview_pb->setHidden(true);
	
	ui->m_line_style_cb->addItem(tr("Ne pas modifier"), QPen(Qt::NoPen));
	ui->m_line_style_cb->addItem(tr("Trait plein", "conductor style: solid line"), QPen(Qt::SolidLine));
	ui->m_line_style_cb->addItem(tr("Trait en pointillés", "conductor style: dashed line"), QPen(Qt::DashLine));
	ui->m_line_style_cb->addItem(tr("Traits et points", "conductor style: dashed and dotted line"), QPen(Qt::DashDotLine));
	
	connect(ui->m_multi_gb,      &QGroupBox::toggled,     [this](bool toggle)  {this->ui->m_singlewire_gb->setChecked(!toggle);});
	connect(ui->m_singlewire_gb, &QGroupBox::toggled,     [this](bool toggle)  {this->ui->m_multi_gb->setChecked(!toggle);});
	connect(ui->m_formula_le,    &QLineEdit::textChanged, [this](QString text) {this->ui->m_text_le->setEnabled(text.isEmpty());});
	ui->m_multi_gb->setChecked(true);
	ui->m_singlewire_gb->setChecked(true);
}

void ReplaceConductorDialog::setColorButton(const QColor &color)
{
	QPalette palette;
	palette.setColor(QPalette::Button, color);
	ui->m_color_pb->setStyleSheet(QString("background-color: %1; min-height: 1.5em; border-style: outset; border-width: 2px; border-color: gray; border-radius: 4px;").arg(color.name()));
}

void ReplaceConductorDialog::setColorButton2(const QColor &color)
{
	QPalette palette;
	palette.setColor(QPalette::Button, color);
	ui->m_color_2_pb->setStyleSheet(QString("background-color: %1; min-height: 1.5em; border-style: outset; border-width: 2px; border-color: gray; border-radius: 4px;").arg(color.name()));
}

void ReplaceConductorDialog::on_m_erase_formula_cb_clicked()
{
    ui->m_formula_le->setText(ui->m_erase_formula_cb->isChecked() ? SearchAndReplaceWorker::eraseText() : QString());
	ui->m_formula_le->setDisabled(ui->m_erase_formula_cb->isChecked());
}

void ReplaceConductorDialog::on_m_erase_text_cb_clicked()
{
	ui->m_text_le->setText(ui->m_erase_text_cb->isChecked() ? SearchAndReplaceWorker::eraseText() : QString());
	ui->m_text_le->setDisabled(ui->m_erase_text_cb->isChecked());
}

void ReplaceConductorDialog::on_m_erase_function_cb_clicked()
{
	ui->m_function_le->setText(ui->m_erase_function_cb->isChecked() ? SearchAndReplaceWorker::eraseText() : QString());
	ui->m_function_le->setDisabled(ui->m_erase_function_cb->isChecked());
}

void ReplaceConductorDialog::on_m_erase_tension_protocol_cb_clicked()
{
	ui->m_tension_protocol_le->setText(ui->m_erase_tension_protocol_cb->isChecked() ? SearchAndReplaceWorker::eraseText() : QString());
	ui->m_tension_protocol_le->setDisabled(ui->m_erase_tension_protocol_cb->isChecked());
}

void ReplaceConductorDialog::on_m_earth_cb_toggled(bool checked)
{
	if (checked && ui->m_neutral_cb -> isChecked()) {
		 ui -> m_pen_cb -> setEnabled(true);
	} else {
		ui -> m_pen_cb -> setDisabled(true);
	}
}

void ReplaceConductorDialog::on_m_neutral_cb_toggled(bool checked)
{
	if (checked && ui->m_earth_cb->isChecked()) {
		ui->m_pen_cb->setEnabled(true);
	} else {
		ui->m_pen_cb->setDisabled(true);
	}
}

void ReplaceConductorDialog::on_m_update_preview_pb_clicked() {
    updatePreview();
}

void ReplaceConductorDialog::on_m_color_pb_clicked()
{
	QColor color = QColorDialog::getColor(m_properties.color, this);
	if (color.isValid()) {
		setColorButton(color);
	}
}

void ReplaceConductorDialog::on_m_color_2_pb_clicked()
{
	QColor color = QColorDialog::getColor(m_properties.m_color_2, this);
	if (color.isValid()) {
		setColorButton2(color);
	}
}
