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
#include "../custom/wirecatalogue/wirecataloguemodel.h"
#include "../custom/wirecatalogue/wirefilterproxymodel.h"
#include "../custom/wirecatalogue/iec60757.h"

#include <QComboBox>
#include <QLabel>
#include <QGridLayout>
#include <QTableView>
#include <QHeaderView>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>

#include <algorithm>

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

	initAssignWiresTab();
}

/**
	@brief ConductorPropertiesWidget::initAssignWiresTab
	Custom feature (Trovo Tech): a dedicated "Assign wires" tab (next to
	Appearance), modelled on the SolidWorks Electrical "assign wires" workflow.
	Filter the catalogue by colour / cross-section, pick a wire or cable core,
	and assign it to THIS conductor only.
*/
void ConductorPropertiesWidget::initAssignWiresTab()
{
	m_wire_db = new WireCatalogueDb(this);
	if (!m_wire_db->open(WireCatalogueDb::defaultPath()))
		return; // catalogue unavailable: skip the tab

	auto *tab = new QWidget(this);
	auto *layout = new QVBoxLayout(tab);

	// --- Filters ---
	m_colour_filter  = new QComboBox(tab);
	m_section_filter = new QComboBox(tab);
	m_colour_filter->setIconSize(QSize(14, 14));
	auto *filter_row = new QHBoxLayout;
	filter_row->addWidget(new QLabel(tr("Colour:"), tab));
	filter_row->addWidget(m_colour_filter);
	filter_row->addSpacing(12);
	filter_row->addWidget(new QLabel(tr("Cross-section:"), tab));
	filter_row->addWidget(m_section_filter);
	filter_row->addStretch(1);
	layout->addLayout(filter_row);

	// --- Catalogue table (filtered) ---
	m_wire_model = new WireCatalogueModel(m_wire_db, this);
	m_wire_proxy = new WireFilterProxyModel(this);
	m_wire_proxy->setSourceModel(m_wire_model);

	m_wire_table = new QTableView(tab);
	m_wire_table->setModel(m_wire_proxy);
	m_wire_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_wire_table->setSelectionMode(QAbstractItemView::SingleSelection);
	m_wire_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_wire_table->verticalHeader()->setVisible(false);
	m_wire_table->setSortingEnabled(true);
	m_wire_table->horizontalHeader()->setStretchLastSection(true);
	layout->addWidget(m_wire_table, 1);

	// --- Core selector (cables) + Assign button ---
	m_core_label = new QLabel(tr("Cable core:"), tab);
	m_core_cb    = new QComboBox(tab);
	m_core_cb->setIconSize(QSize(14, 14));
	m_core_label->setVisible(false);
	m_core_cb->setVisible(false);
	m_assign_btn = new QPushButton(tr("Assign to this conductor"), tab);
	m_assign_btn->setEnabled(false);

	auto *action_row = new QHBoxLayout;
	action_row->addWidget(m_core_label);
	action_row->addWidget(m_core_cb);
	action_row->addStretch(1);
	action_row->addWidget(m_assign_btn);
	layout->addLayout(action_row);

	ui->tabWidget->addTab(tab, tr("Assign wires"));

	populateFilters();

	connect(m_colour_filter,  QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &ConductorPropertiesWidget::filtersChanged);
	connect(m_section_filter, QOverload<int>::of(&QComboBox::currentIndexChanged),
			this, &ConductorPropertiesWidget::filtersChanged);
	connect(m_wire_table->selectionModel(), &QItemSelectionModel::selectionChanged,
			this, &ConductorPropertiesWidget::wireSelectionChanged);
	connect(m_assign_btn, &QPushButton::clicked,
			this, &ConductorPropertiesWidget::assignSelectedWire);
}

/**
	@brief Fill the colour and cross-section filter combos from the catalogue.
*/
void ConductorPropertiesWidget::populateFilters()
{
	m_colour_filter->addItem(tr("Any"), QString());
	for (const QString &name : Iec60757::standardNames())
		m_colour_filter->addItem(Iec60757::icon(name, 14), name, name);

	m_section_filter->addItem(tr("Any"), -1.0);
	QList<double> sections;
	for (const WireSpec &w : m_wire_db->allWires())
		if (w.crossSectionMm2 > 0 && !sections.contains(w.crossSectionMm2))
			sections << w.crossSectionMm2;
	std::sort(sections.begin(), sections.end());
	for (double s : sections)
		m_section_filter->addItem(QStringLiteral("%1 mm²").arg(s), s);
}

void ConductorPropertiesWidget::filtersChanged()
{
	m_wire_proxy->setColourFilter(m_colour_filter->currentData().toString());
	m_wire_proxy->setSectionFilter(m_section_filter->currentData().toDouble());
}

/**
	@brief WireSpec for the currently selected table row (invalid if none).
*/
WireSpec ConductorPropertiesWidget::selectedWire() const
{
	const QModelIndexList sel = m_wire_table->selectionModel()->selectedRows();
	if (sel.isEmpty())
		return WireSpec();
	const QModelIndex src = m_wire_proxy->mapToSource(sel.first());
	return m_wire_model->wireAt(src.row());
}

// Sentinel core index used for "this conductor IS the cable shield".
static const int kShieldCore = -1;

/**
	@brief Reveal/populate the core selector when the selection changes.
	Shows one entry per cable core, plus a "Shield" entry for shielded
	wires/cables so the shield can be connected (terminated) to a terminal.
*/
void ConductorPropertiesWidget::wireSelectionChanged()
{
	const WireSpec w = selectedWire();
	m_assign_btn->setEnabled(w.isValid());

	// A selector is needed for multi-core cables and for anything shielded.
	if (!w.isValid() || (!w.isCable() && !w.hasShield)) {
		m_core_label->setVisible(false);
		m_core_cb->setVisible(false);
		return;
	}

	m_core_cb->blockSignals(true);
	m_core_cb->clear();
	for (int i = 0; i < w.coreColors.size(); ++i) {
		const QStringList core = w.coreColors.at(i);
		const QString text = tr("Core %1 — %2").arg(i + 1)
				.arg(core.isEmpty() ? tr("(no colour)") : core.join(QStringLiteral("/")));
		m_core_cb->addItem(Iec60757::icon(core.value(0), 14), text, i);
	}
	if (w.hasShield) {
		const QString sh = w.shieldType.isEmpty() ? tr("screen") : w.shieldType;
		m_core_cb->addItem(Iec60757::icon(QStringLiteral("Green-Yellow"), 14),
						   tr("Shield (%1)").arg(sh), kShieldCore);
	}
	m_core_cb->setCurrentIndex(0);
	m_core_cb->blockSignals(false);
	m_core_label->setVisible(true);
	m_core_cb->setVisible(true);
}

/**
	@brief Assign the selected wire (or chosen cable core) to THIS conductor.
	Emits wireAssigned() so the dialog scopes the change to this conductor only.
*/
void ConductorPropertiesWidget::assignSelectedWire()
{
	const WireSpec w = selectedWire();
	if (!w.isValid())
		return;

	const QString section = QStringLiteral("%1mm²")
			.arg(QString::number(w.crossSectionMm2));

	// What is selected: a cable core (0..N-1), the shield (kShieldCore), or
	// nothing special (plain single wire, selector hidden -> treat as the wire).
	const int sel = (m_core_cb->isVisible())
			? m_core_cb->currentData().toInt() : 0;

	if (sel == kShieldCore) {
		// Connect the cable's shield (screen) — green-yellow, dashed line.
		const QString colour = QStringLiteral("Green-Yellow");
		if (ui->m_line_style_cb->count() > 1)
			ui->m_line_style_cb->setCurrentIndex(1); // dashed
		applyWireAppearance(section, colour, w.wireId,
							QStringLiteral("%1:SH  Shield").arg(w.wireId));
	} else if (w.isCable()) {
		const QStringList core = w.coreColors.value(sel);
		const QString colour = core.value(0);
		const QString core_ref = QStringLiteral("%1:%2").arg(w.wireId).arg(sel + 1);
		applyWireAppearance(section, colour, w.wireId,
							QStringLiteral("%1  %2  %3").arg(core_ref, section, colour));
	} else {
		const QString colour = w.effectiveColor();
		applyWireAppearance(section, colour, QString(),
							QStringLiteral("%1  %2").arg(section, colour));
	}

	emit wireAssigned();
}

/**
	@brief ConductorPropertiesWidget::applyWireAppearance
	Shared helper: set conductor colour, wire metadata and the on-line label.
	@param cableId : when non-empty, recorded as the conductor's cable so the
		terminal strip / BOM treat the bundle as one cable.
*/
void ConductorPropertiesWidget::applyWireAppearance(const QString &section,
													const QString &colour,
													const QString &cableId,
													const QString &lineLabel)
{
	const QColor c = Iec60757::colorForName(colour);
	if (c.isValid())
		ui->m_color_kpb->setColor(c);

	ui->m_wire_section_le->setText(section);
	ui->m_wire_color_le->setText(colour);
	ui->m_cable_le->setText(cableId); // saved via properties().m_cable

	// Displayed text driven by the formula (literal text shown verbatim);
	// m_text_le is disabled while a formula is present.
	ui->m_formula_le->setText(lineLabel);
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
