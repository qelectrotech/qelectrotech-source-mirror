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
#include "conductorpropertieswidget.h"

#include "../conductorproperties.h"
#include "../qetapp.h"
#include "../qtextorientationspinboxwidget.h"
#include "ui_conductorpropertieswidget.h"

#include "../custom/wirecatalogue/wirecataloguedb.h"
#include "../custom/wirecatalogue/iec60757.h"

#include <QComboBox>
#include <QLabel>
#include <QGridLayout>

/**
	@brief ConductorPropertiesWidget::ConductorPropertiesWidget
	Constructor
	@param parent : paretn widget
*/
ConductorPropertiesWidget::ConductorPropertiesWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ConductorPropertiesWidget)
{
	ui->setupUi(this);
	initWidget();
}




/**
	@brief ConductorPropertiesWidget::ConductorPropertiesWidget
	Constructor with properties
	@param properties : properties to edit
	@param parent : parent widget
*/
ConductorPropertiesWidget::ConductorPropertiesWidget(
		const ConductorProperties &properties,
		QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ConductorPropertiesWidget)
{
	ui -> setupUi(this);
	initWidget();
	setProperties(properties);
}

/**
	@brief ConductorPropertiesWidget::~ConductorPropertiesWidget
	destructor
*/
ConductorPropertiesWidget::~ConductorPropertiesWidget()
{
	delete ui;
}

/**
	@brief ConductorPropertiesWidget::setProperties
	Set the properties
	@param properties
*/
void ConductorPropertiesWidget::setProperties(
		const ConductorProperties &properties)
{
	if (m_properties == properties)
		return;

	m_properties = properties;

	int index = ui -> m_line_style_cb -> findData(QPen(m_properties.style));
	if (index != -1) ui -> m_line_style_cb -> setCurrentIndex(index);

	ui->m_color_2_gb            -> setChecked  (m_properties.m_bicolor);
	ui->m_dash_size_sb          -> setValue    (m_properties.m_dash_size);
	ui->m_formula_le            -> setText    (m_properties.m_formula);
	ui->m_text_le               -> setText    (m_properties.text);
	ui->m_function_le           -> setText    (m_properties.m_function);
	ui->m_cable_le              -> setText    (m_properties.m_cable);
	ui->m_bus_le                -> setText    (m_properties.m_bus);
	ui->m_tension_protocol_le   -> setText    (m_properties.m_tension_protocol);
	ui->m_wire_color_le         -> setText    (m_properties.m_wire_color);
	ui->m_wire_section_le       -> setText    (m_properties.m_wire_section);
	ui->m_text_size_sb          -> setValue   (m_properties.text_size);
	ui->m_cond_size_sb          -> setValue   (m_properties.cond_size);
	ui->m_show_text_cb          -> setChecked (m_properties.m_show_text);
	ui->m_one_text_per_folio_cb -> setChecked (m_properties.m_one_text_per_folio);
	ui->m_earth_cb              -> setChecked (m_properties.singleLineProperties.hasGround);
	ui->m_neutral_cb            -> setChecked (m_properties.singleLineProperties.hasNeutral);
	ui->m_pen_cb                -> setChecked (m_properties.singleLineProperties.isPen());
	ui->m_phase_cb              -> setChecked (m_properties.singleLineProperties.phasesCount());
	ui->m_phase_slider          -> setValue   (m_properties.singleLineProperties.phasesCount());
	ui->m_color_kpb             -> setColor(m_properties.color);
	ui->m_color_2_kpb           -> setColor(m_properties.m_color_2);
	ui->m_text_color_kpb        -> setColor(m_properties.text_color);

	ui->m_horiz_cb->setCurrentIndex(m_properties.m_horizontal_alignment == Qt::AlignTop? 0 : 1);
	ui->m_verti_cb->setCurrentIndex(m_properties.m_vertical_alignment == Qt::AlignLeft? 0 : 1);
	m_verti_select -> setValue (m_properties.verti_rotate_text);
	m_horiz_select -> setValue (m_properties.horiz_rotate_text);

	setConductorType(m_properties.type);
	updatePreview(false);
}

/**
	@brief ConductorPropertiesWidget::properties
	@return the edited properties
*/
ConductorProperties ConductorPropertiesWidget::properties() const
{
	ConductorProperties properties_;
	if (ui -> m_multiwires_gb  -> isChecked()) properties_.type = ConductorProperties::Multi;
	else if (ui -> m_singlewire_gb -> isChecked()) properties_.type = ConductorProperties::Single;

	properties_.color                   = ui -> m_color_kpb->color();
	properties_.m_bicolor               = ui->m_color_2_gb->isChecked();
	properties_.m_color_2               = ui->m_color_2_kpb->color();
	properties_.m_dash_size             = ui->m_dash_size_sb->value();
	properties_.style                   = ui -> m_line_style_cb->itemData(ui->m_line_style_cb->currentIndex()).value<QPen>().style();
	properties_.m_formula               = ui->m_formula_le->text();
	properties_.text                    = ui -> m_text_le -> text();
	properties_.text_color              = ui -> m_text_color_kpb->color();
	properties_.m_function              = ui -> m_function_le->text();
	properties_.m_cable                 = ui -> m_cable_le->text();
	properties_.m_bus                   = ui -> m_bus_le->text();
	properties_.m_tension_protocol      = ui -> m_tension_protocol_le->text();
	properties_.m_wire_color            = ui -> m_wire_color_le->text();
	properties_.m_wire_section          = ui -> m_wire_section_le->text();
	properties_.text_size               = ui -> m_text_size_sb -> value();
	properties_.cond_size               = ui -> m_cond_size_sb -> value();
	properties_.m_show_text             = ui -> m_show_text_cb -> isChecked();
	properties_.m_one_text_per_folio    = ui -> m_one_text_per_folio_cb -> isChecked();
	properties_.verti_rotate_text       = m_verti_select -> value();
	properties_.horiz_rotate_text       = m_horiz_select -> value();
	properties_.m_vertical_alignment    = ui->m_verti_cb->currentIndex() == 0? Qt::AlignLeft : Qt::AlignRight;
	properties_.m_horizontal_alignment  = ui->m_horiz_cb->currentIndex() == 0? Qt::AlignTop : Qt::AlignBottom;

	properties_.singleLineProperties.hasGround  = ui -> m_earth_cb -> isChecked();
	properties_.singleLineProperties.hasNeutral = ui -> m_neutral_cb -> isChecked();
	properties_.singleLineProperties.is_pen     = ui -> m_pen_cb -> isChecked();
	properties_.singleLineProperties.setPhasesCount(ui -> m_phase_cb -> isChecked() ? ui -> m_phase_sb -> value() : 0);

	return properties_;
}

/**
	@brief ConductorPropertiesWidget::event
	@param event
	@return
*/
bool ConductorPropertiesWidget::event(QEvent *event)
{
	if (event->type() == QEvent::WindowActivate) {
		ui -> m_formula_le -> setFocus();
	}
	return(QWidget::event(event));
}


/**
	@brief ConductorPropertiesWidget::setReadOnly
	@param ro if true this widget is disabled
*/
void ConductorPropertiesWidget::setReadOnly(const bool &ro) {
	this->setDisabled(ro);
}

/**
	@brief ConductorPropertiesWidget::addAutonumWidget
	@param widget
*/
void ConductorPropertiesWidget::addAutonumWidget(QWidget *widget) {
	ui->m_autonum_layout->addWidget(widget);
}

/**
	@brief ConductorPropertiesWidget::setHiddenOneTextPerFolio
	@param hide
*/
void ConductorPropertiesWidget::setHiddenOneTextPerFolio(const bool &hide) {
	ui -> m_one_text_per_folio_cb -> setHidden(hide);
}

/**
	@brief ConductorPropertiesWidget::setDisabledShowText
	@param disable
*/
void ConductorPropertiesWidget::setDisabledShowText(const bool &disable) {
	ui->m_show_text_cb->setDisabled(disable==true? true : false);
}

/**
	@brief ConductorPropertiesWidget::setHiddenAvailableAutonum
	Hide the label, combo box and push button of available autonum
	@param hide
*/
void ConductorPropertiesWidget::setHiddenAvailableAutonum(const bool &hide)
{
	ui->m_autonum_label->setHidden(hide);
	ui->m_available_autonum_cb->setHidden(hide);
	ui->m_edit_autonum_pb->setHidden(hide);
}

QComboBox *ConductorPropertiesWidget::autonumComboBox() const
{
	return ui->m_available_autonum_cb;
}

QPushButton *ConductorPropertiesWidget::editAutonumPushButton() const
{
	return ui->m_edit_autonum_pb;
}

/**
	@brief ConductorPropertiesWidget::initWidget
*/
void ConductorPropertiesWidget::initWidget()
{
	m_verti_select = QETApp::createTextOrientationSpinBoxWidget();
	ui -> m_text_angle_gl -> addWidget(m_verti_select, 2, 0, Qt::AlignHCenter);
	m_horiz_select = QETApp::createTextOrientationSpinBoxWidget();
	ui -> m_text_angle_gl -> addWidget(m_horiz_select, 2, 1, Qt::AlignHCenter);

	ui -> m_line_style_cb -> addItem(tr("Trait plein", "conductor style: solid line"), QPen(Qt::SolidLine));
	ui -> m_line_style_cb -> addItem(tr("Trait en pointillés", "conductor style: dashed line"), QPen(Qt::DashLine));
	ui -> m_line_style_cb -> addItem(tr("Traits et points", "conductor style: dashed and dotted line"), QPen(Qt::DashDotLine));

	ui -> m_update_preview_pb -> setHidden(true);

	connect(ui->m_multiwires_gb, &QGroupBox::toggled, [this](bool toggle) {this->ui->m_singlewire_gb->setChecked(!toggle);});
	connect(ui->m_singlewire_gb, &QGroupBox::toggled, [this](bool toggle) {this->ui->m_multiwires_gb->setChecked(!toggle);});
	connect(ui->m_formula_le, &QLineEdit::textChanged, [this](QString text) {this->ui->m_text_le->setEnabled(text.isEmpty());});
	ui->m_multiwires_gb->setChecked(true);
	ui->m_singlewire_gb->setChecked(true);
#if TODO_LIST
#pragma message("@TODO Add Kabel and Bus to qet")
#else
	ui->m_cable_le->setDisabled(true);
	ui->m_bus_le->setDisabled(true);
#endif

	initWireCatalogue();
}

/**
	@brief ConductorPropertiesWidget::initWireCatalogue
	Custom feature (Trovo Tech): add a "From wire catalogue" picker to the
	Appearance tab. Choosing a catalogue wire sets the conductor colour from
	that wire's primary IEC 60757 colour.
*/
void ConductorPropertiesWidget::initWireCatalogue()
{
	m_wire_db = new WireCatalogueDb(this);
	if (!m_wire_db->open(WireCatalogueDb::defaultPath()))
		return; // catalogue unavailable: silently skip the picker

	m_wire_catalogue_cb = new QComboBox(this);
	m_wire_catalogue_cb->setIconSize(QSize(14, 14));
	m_wire_catalogue_cb->addItem(tr("(none)"));
	const QVector<WireSpec> wires = m_wire_db->allWires();
	for (const WireSpec &w : wires) {
		const QString label = QStringLiteral("%1 — %2").arg(w.wireId, w.specLabel());
		m_wire_catalogue_cb->addItem(Iec60757::icon(w.colorPrimary, 14), label, w.wireId);
	}

	auto *label = new QLabel(tr("From catalogue :"), this);
	label->setToolTip(tr("Pick a wire from the catalogue to set its colour"));

	// Append a row at the bottom of the Appearance tab grid.
	auto *grid = ui->appearance->findChild<QGridLayout*>();
	if (grid) {
		const int row = grid->rowCount();
		grid->addWidget(label, row, 0);
		grid->addWidget(m_wire_catalogue_cb, row, 1);
	}

	connect(m_wire_catalogue_cb, QOverload<int>::of(&QComboBox::activated),
			this, &ConductorPropertiesWidget::applyCatalogueWire);
}

/**
	@brief ConductorPropertiesWidget::applyCatalogueWire
	Apply the primary colour of the catalogue wire selected at @p index.
*/
void ConductorPropertiesWidget::applyCatalogueWire(int index)
{
	if (!m_wire_db || index <= 0) // 0 == "(none)"
		return;

	const QString wire_id = m_wire_catalogue_cb->itemData(index).toString();
	const WireSpec w = m_wire_db->wire(wire_id);
	if (!w.isValid())
		return;

	const QString section = QStringLiteral("%1mm²")
			.arg(QString::number(w.crossSectionMm2));

	// Drawing colour from the wire's primary colour.
	const QColor c = Iec60757::colorForName(w.colorPrimary);
	if (c.isValid())
		ui->m_color_kpb->setColor(c);

	// Dedicated wire metadata fields.
	ui->m_wire_section_le->setText(section);
	ui->m_wire_color_le->setText(w.colorPrimary);

	// Put cross-section + colour on the conductor line (Wire ID is not shown
	// on the line, only kept as catalogue metadata). The displayed text is
	// driven by the formula (literal text shown verbatim); m_text_le is
	// disabled while a formula is present.
	ui->m_formula_le->setText(QStringLiteral("%1  %2")
			.arg(section, w.colorPrimary));
	ui->m_show_text_cb->setChecked(true);

	updatePreview();
}

/**
	@brief ConductorPropertiesWidget::setConductorType
	Update this widget to the new conductor type
	@param type
*/
void ConductorPropertiesWidget::setConductorType(ConductorProperties::ConductorType type) {
	switch (type) {
		case ConductorProperties::Multi:
			ui -> m_multiwires_gb -> setChecked(true);
			break;
		case ConductorProperties::Single:
			ui -> m_singlewire_gb -> setChecked(true);
			break;
		default:
			ui -> m_multiwires_gb -> setChecked(true);
			break;
	}
}

/**
	@brief ConductorPropertiesWidget::updatePreview
	Update the preview for single lien
	@param b true: update from the value displayed by this widget
		 false: update from the properties given at the constructor of this widget
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
	@brief ConductorPropertiesWidget::on_m_earth_cb_toggled
	Update the widget when toggled
	@param checked
*/
void ConductorPropertiesWidget::on_m_earth_cb_toggled(bool checked) {
	if (checked && ui -> m_neutral_cb -> isChecked())
		 ui -> m_pen_cb -> setEnabled(true);
	else ui -> m_pen_cb -> setDisabled(true);
}

/**
	@brief ConductorPropertiesWidget::on_m_neutral_cb_toggled
	Update the widget when toggled
	@param checked
*/
void ConductorPropertiesWidget::on_m_neutral_cb_toggled(bool checked) {
	if (checked && ui -> m_earth_cb -> isChecked())
		ui -> m_pen_cb -> setEnabled(true);
	else ui -> m_pen_cb -> setDisabled(true);
}

/**
	@brief ConductorPropertiesWidget::on_m_update_preview_pb_clicked
	Update the preview of single line.
	m_update_preview_pb is a no used button and hidden, his role is only
	to centralize signal from various widget to edit single ligne properties,
	for update the preview
*/
void ConductorPropertiesWidget::on_m_update_preview_pb_clicked()
{
	updatePreview();
}
