/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "conductorpropertieswidget.h"
#include "ui_conductorpropertieswidget.h"
#include "conductorproperties.h"
#include "qetapp.h"
#include "qtextorientationspinboxwidget.h"

/**
 * @brief ConductorPropertiesWidget::ConductorPropertiesWidget
 * Constructor
 * @param parent, paretn widget
 */
ConductorPropertiesWidget::ConductorPropertiesWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ConductorPropertiesWidget)
{
	ui->setupUi(this);
	initWidget();
}

/**
 * @brief ConductorPropertiesWidget::ConductorPropertiesWidget
 * Constructor with properties
 * @param properties, properties to edit
 * @param parent, parent widget
 */
ConductorPropertiesWidget::ConductorPropertiesWidget(const ConductorProperties &properties, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ConductorPropertiesWidget)
{
	ui -> setupUi(this);
	initWidget();
	setProperties(properties);
}

/**
 * @brief ConductorPropertiesWidget::~ConductorPropertiesWidget
 * destructor
 */
ConductorPropertiesWidget::~ConductorPropertiesWidget()
{
	delete ui;
}

/**
 * @brief ConductorPropertiesWidget::setProperties
 * Set the properties
 * @param properties
 */
void ConductorPropertiesWidget::setProperties(const ConductorProperties &properties) {
	if (m_properties == properties) return;
	m_properties = properties;

	setColorButton(m_properties.color);
	int index = ui -> m_line_style_cb->findData(m_properties.style);
	if (index != -1) ui -> m_line_style_cb -> setCurrentIndex(index);
	ui -> m_text_le      -> setText    (m_properties.text);
	ui -> m_text_size_sb -> setValue   (m_properties.text_size);
	ui -> m_earth_cb     -> setChecked (m_properties.singleLineProperties.hasGround);
	ui -> m_neutral_cb   -> setChecked (m_properties.singleLineProperties.hasNeutral);
	ui -> m_pen_cb       -> setChecked (m_properties.singleLineProperties.isPen());
	ui -> m_phase_cb     -> setChecked (m_properties.singleLineProperties.phasesCount());
	ui -> m_phase_slider -> setValue   (m_properties.singleLineProperties.phasesCount());
	m_verti_select       -> setValue   (m_properties.verti_rotate_text);
	m_horiz_select       -> setValue   (m_properties.horiz_rotate_text);

	setConductorType(m_properties.type);
	updatePreview(false);
}

/**
 * @brief ConductorPropertiesWidget::properties
 * @return the edited properties
 */
ConductorProperties ConductorPropertiesWidget::properties() const {
	ConductorProperties properties_;
	if      (ui -> m_simple_rb -> isChecked()) properties_.type = ConductorProperties::Simple;
	else if (ui -> m_multi_rb  -> isChecked()) properties_.type = ConductorProperties::Multi;
	else if (ui -> m_single_rb -> isChecked()) properties_.type = ConductorProperties::Single;
	properties_.color = ui->m_color_pb->palette().color(QPalette::Button);
	properties_.style = static_cast<Qt::PenStyle>(ui->m_line_style_cb->itemData(ui->m_line_style_cb->currentIndex()).toInt());
	properties_.text = ui -> m_text_le -> text();
	properties_.text_size = ui -> m_text_size_sb -> value();
	properties_.verti_rotate_text = m_verti_select -> value();
	properties_.horiz_rotate_text = m_horiz_select -> value();
	properties_.singleLineProperties.hasGround = ui -> m_earth_cb -> isChecked();
	properties_.singleLineProperties.hasNeutral = ui -> m_neutral_cb -> isChecked();
	properties_.singleLineProperties.is_pen = ui -> m_pen_cb -> isChecked();
	properties_.singleLineProperties.setPhasesCount(ui -> m_phase_cb -> isChecked() ? ui -> m_phase_sb -> value() : 0);

	return properties_;
}

/**
 * @brief ConductorPropertiesWidget::setReadOnly
 * @param ro if true this widget is disabled
 */
void ConductorPropertiesWidget::setReadOnly(const bool &ro) {
	this->setDisabled(ro);
}

void ConductorPropertiesWidget::addAutonumWidget(QWidget *widget) {
	ui->m_autonum_layout->addWidget(widget);
}

/**
 * @brief ConductorPropertiesWidget::initWidget
 */
void ConductorPropertiesWidget::initWidget() {
	m_verti_select = QETApp::createTextOrientationSpinBoxWidget();
	ui -> m_text_angle_gl -> addWidget(m_verti_select, 2, 0,  Qt::AlignHCenter);
	m_horiz_select = QETApp::createTextOrientationSpinBoxWidget();
	ui -> m_text_angle_gl -> addWidget(m_horiz_select, 2, 1,  Qt::AlignHCenter);

	ui -> m_line_style_cb -> addItem(tr("Trait plein", "conductor style: solid line"), Qt::SolidLine);
	ui -> m_line_style_cb -> addItem(tr("Trait en pointill\351s", "conductor style: dashed line"), Qt::DashLine);
	ui -> m_line_style_cb -> addItem(tr("Traits et points", "conductor style: dashed and dotted line"), Qt::DashDotLine);

	ui -> m_update_preview_pb -> setHidden(true);
	//Check this checkbox for init the associated groupbox
	ui -> m_single_rb -> setChecked(true);
	ui -> m_multi_rb -> setChecked(true);
}

/**
 * @brief ConductorPropertiesWidget::setConductorType
 * Update this widget to the new conductor type
 * @param type
 */
void ConductorPropertiesWidget::setConductorType(ConductorProperties::ConductorType type) {
	switch (type) {
		case ConductorProperties::Simple:
			ui -> m_simple_rb -> setChecked(true);
			break;
		case ConductorProperties::Multi:
			ui -> m_multi_rb -> setChecked(true);
			break;
		case ConductorProperties::Single:
			ui -> m_single_rb -> setChecked(true);
			break;
		default:
			ui -> m_multi_rb -> setChecked(true);
			break;
	}
}

/**
 * @brief ConductorPropertiesWidget::updatePreview
 * Update the preview for single lien
 * @param b true: update from the value displayed by this widget
 *          false: update from the properties given at the constructor of this widget
 */
void ConductorPropertiesWidget::updatePreview(bool b) {
	const QRect pixmap_rect(0, 0, 96, 96);
	QPixmap pixmap(pixmap_rect.width(), pixmap_rect.height());
	QPainter painter;
	painter.begin(&pixmap);
	painter.eraseRect(pixmap_rect);
	painter.drawRect(pixmap_rect.adjusted(0,0,-1,-1));
	painter.drawLine(QLineF(0, pixmap_rect.height() / 2, pixmap_rect.width(), pixmap_rect.height() / 2));

	if (b) properties().singleLineProperties.draw(&painter, QET::Horizontal, pixmap_rect);
	else   m_properties.singleLineProperties.draw(&painter, QET::Horizontal, pixmap_rect);

	painter.end();
	ui -> m_preview_label -> setPixmap(pixmap);
}

/**
 * @brief ConductorPropertiesWidget::on_m_earth_cb_toggled
 * Update the widget when toggled
 * @param checked
 */
void ConductorPropertiesWidget::on_m_earth_cb_toggled(bool checked) {
	if (checked && ui -> m_neutral_cb -> isChecked())
		 ui -> m_pen_cb -> setEnabled(true);
	else ui -> m_pen_cb -> setDisabled(true);
}

/**
 * @brief ConductorPropertiesWidget::on_m_neutral_cb_toggled
 * Update the widget when toggled
 * @param checked
 */
void ConductorPropertiesWidget::on_m_neutral_cb_toggled(bool checked) {
	if (checked && ui -> m_earth_cb -> isChecked())
		ui -> m_pen_cb -> setEnabled(true);
   else ui -> m_pen_cb -> setDisabled(true);
}

/**
 * @brief ConductorPropertiesWidget::on_m_color_pb_clicked
 * Open a color dialog, for choose the color of conductor
 */
void ConductorPropertiesWidget::on_m_color_pb_clicked() {
	QColor color = QColorDialog::getColor(m_properties.color, this);
	if (color.isValid())
		setColorButton(color);
}

/**
 * @brief ConductorPropertiesWidget::setColorButton
 * Set m_color_pb to @color
 * @param color
 */
void ConductorPropertiesWidget::setColorButton(const QColor &color) {
	QPalette palette;
	palette.setColor(QPalette::Button, color);
	ui -> m_color_pb -> setStyleSheet(QString("background-color: %1").arg(color.name()));
}

/**
 * @brief ConductorPropertiesWidget::on_m_update_preview_pb_clicked
 * Update the preview of single line.
 * m_update_preview_pb is a no used button and hidden, his role is only
 * to centralize signal from various widget to edit single ligne properties,
 * for update the preview
 */
void ConductorPropertiesWidget::on_m_update_preview_pb_clicked() {
	updatePreview();
}
