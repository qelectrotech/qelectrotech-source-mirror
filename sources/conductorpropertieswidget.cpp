/*
	Copyright 2006-2009 Xavier Guerrin
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
#include <QtGui>
#include "conductor.h"
#include "qeticons.h"

/**
	Constructeur
	@param parent QWidget parent
*/
ConductorPropertiesWidget::ConductorPropertiesWidget(QWidget *parent) :
	QWidget(parent)
{
	buildInterface();
}

/**
	Constructeur
	@param parent QWidget parent
	@param cp Proprietes a editer
*/
ConductorPropertiesWidget::ConductorPropertiesWidget(const ConductorProperties &cp, QWidget *parent) :
	QWidget(parent)
{
	buildInterface();
	setConductorProperties(cp);
}

/// construit l'interface du widget
void ConductorPropertiesWidget::buildInterface() {
	
	setMinimumSize(380, 280);
	
	QVBoxLayout *main_layout = new QVBoxLayout(this);
	main_layout -> setContentsMargins(0, 0, 0, 0);
	
	QGroupBox *groupbox = new QGroupBox(tr("Type de conducteur"));
	main_layout -> addWidget(groupbox);
	
	QVBoxLayout *groupbox_layout = new QVBoxLayout();
	groupbox -> setLayout(groupbox_layout);
	
	simple = new QRadioButton(tr("Simple"));
	multiline = new QRadioButton(tr("Multifilaire"));
	
	QHBoxLayout *multiline_layout = new QHBoxLayout();
	QLabel *text = new QLabel(tr("Texte :"));
	text_field = new QLineEdit();
	multiline_layout -> addWidget(text);
	multiline_layout -> addWidget(text_field);
	
	singleline = new QRadioButton(tr("Unifilaire"));
	
	QHBoxLayout *singleline_layout3 = new QHBoxLayout();
	phase_checkbox = new QCheckBox(tr("phase"));
	phase_checkbox -> setIcon(QET::Icons::Phase);
	phase_slider = new QSlider(Qt::Horizontal);
	phase_slider -> setRange(1, 3);
	phase_spinbox = new QSpinBox();
	phase_spinbox -> setRange(1, 3);
	singleline_layout3 -> addWidget(phase_checkbox);
	singleline_layout3 -> addWidget(phase_slider);
	singleline_layout3 -> addWidget(phase_spinbox);
	
	QVBoxLayout *singleline_layout2 = new QVBoxLayout();
	ground_checkbox = new QCheckBox(tr("terre"));
	ground_checkbox -> setIcon(QET::Icons::Ground);
	neutral_checkbox = new QCheckBox(tr("neutre"));
	neutral_checkbox -> setIcon(QET::Icons::Neutral);
	singleline_layout2 -> addWidget(ground_checkbox);
	singleline_layout2 -> addWidget(neutral_checkbox);
	singleline_layout2 -> addLayout(singleline_layout3);
	
	QHBoxLayout *singleline_layout1 = new QHBoxLayout();
	preview = new QLabel();
	preview -> setFixedSize(96, 96);
	singleline_layout1 -> addWidget(preview);
	singleline_layout1 -> addLayout(singleline_layout2);
	
	groupbox_layout -> addWidget(simple);
	groupbox_layout -> addWidget(multiline);
	groupbox_layout -> addLayout(multiline_layout);
	groupbox_layout -> addWidget(singleline);
	groupbox_layout -> addLayout(singleline_layout1);
	
	radio_buttons = new QButtonGroup(this);
	radio_buttons -> addButton(simple,     ConductorProperties::Simple);
	radio_buttons -> addButton(multiline,  ConductorProperties::Multi);
	radio_buttons -> addButton(singleline, ConductorProperties::Single);
	
	buildConnections();
	setConductorType(ConductorProperties::Multi);
}

/// Met en place les connexions signaux/slots
void ConductorPropertiesWidget::buildConnections() {
	connect(phase_slider,      SIGNAL(valueChanged(int)),            phase_spinbox, SLOT(setValue(int)));
	connect(phase_spinbox,     SIGNAL(valueChanged(int)),            phase_slider,  SLOT(setValue(int)));
	connect(ground_checkbox,   SIGNAL(toggled(bool)),                this,          SLOT(updateConfig()));
	connect(neutral_checkbox,  SIGNAL(toggled(bool)),                this,          SLOT(updateConfig()));
	connect(phase_checkbox,    SIGNAL(toggled(bool)),                this,          SLOT(updateConfig()));
	connect(phase_slider,      SIGNAL(valueChanged(int)),            this,          SLOT(updateConfig()));
	connect(radio_buttons,     SIGNAL(buttonClicked(int)),           this,          SLOT(updateConfig()));
	connect(text_field,        SIGNAL(textChanged(const QString &)), this,          SLOT(updateConfig()));
}

/// Enleve les connexions signaux/slots
void ConductorPropertiesWidget::destroyConnections() {
	disconnect(phase_slider,      SIGNAL(valueChanged(int)),            phase_spinbox, SLOT(setValue(int)));
	disconnect(phase_spinbox,     SIGNAL(valueChanged(int)),            phase_slider,  SLOT(setValue(int)));
	disconnect(ground_checkbox,   SIGNAL(toggled(bool)),                this,          SLOT(updateConfig()));
	disconnect(neutral_checkbox,  SIGNAL(toggled(bool)),                this,          SLOT(updateConfig()));
	disconnect(phase_checkbox,    SIGNAL(toggled(bool)),                this,          SLOT(updateConfig()));
	disconnect(phase_slider,      SIGNAL(valueChanged(int)),            this,          SLOT(updateConfig()));
	disconnect(radio_buttons,     SIGNAL(buttonClicked(int)),           this,          SLOT(updateConfig()));
	disconnect(text_field,        SIGNAL(textChanged(const QString &)), this,          SLOT(updateConfig()));
}

/// Destructeur
ConductorPropertiesWidget::~ConductorPropertiesWidget() {
}

/// Met a jour les proprietes
void ConductorPropertiesWidget::updateConfig() {
	properties_.type = static_cast<ConductorProperties::ConductorType>(radio_buttons -> checkedId());
	properties_.text = text_field -> text();
	properties_.singleLineProperties.hasGround = ground_checkbox -> isChecked();
	properties_.singleLineProperties.hasNeutral = neutral_checkbox -> isChecked();
	properties_.singleLineProperties.setPhasesCount(phase_checkbox -> isChecked() ? phase_spinbox -> value() : 0);
	
	updateDisplay();
}

/// Met a jour l'affichage des proprietes
void ConductorPropertiesWidget::updateDisplay() {
	destroyConnections();
	
	setConductorType(properties_.type);
	text_field -> setText(properties_.text);
	ground_checkbox -> setChecked(properties_.singleLineProperties.hasGround);
	neutral_checkbox -> setChecked(properties_.singleLineProperties.hasNeutral);
	phase_spinbox -> setValue(properties_.singleLineProperties.phasesCount());
	phase_slider -> setValue(properties_.singleLineProperties.phasesCount());
	phase_checkbox -> setChecked(properties_.singleLineProperties.phasesCount());
	
	buildConnections();
	updatePreview();
}

/// Met a jour la previsualisation des attributs unifilaires
void ConductorPropertiesWidget::updatePreview() {
	const QRect pixmap_rect(0, 0, 96, 96);
	QPixmap pixmap(pixmap_rect.width(), pixmap_rect.height());
	QPainter painter;
	painter.begin(&pixmap);
	painter.eraseRect(pixmap_rect);
	painter.drawRect(pixmap_rect.adjusted(0,0,-1,-1));
	painter.drawLine(QLineF(0, pixmap_rect.height() / 2, pixmap_rect.width(), pixmap_rect.height() / 2));
	properties_.singleLineProperties.draw(&painter, QET::Horizontal, pixmap_rect);
	painter.end();
	preview -> setPixmap(pixmap);
}

/**
	Passe le widget en mode simple, unifilaire ou multifilaire
	@param t le type de conducteur
*/
void ConductorPropertiesWidget::setConductorType(ConductorProperties::ConductorType t) {
	
	// widgets lies au simple
	simple        -> setChecked(t == ConductorProperties::Simple);
	
	// widgets lies au mode multifilaire
	multiline        -> setChecked(t == ConductorProperties::Multi);
	text_field       -> setEnabled(t == ConductorProperties::Multi);
	
	// widgets lies au mode unifilaire
	bool sl = (t == ConductorProperties::Single);
	singleline       -> setChecked(sl);
	preview          -> setEnabled(sl);
	phase_checkbox   -> setEnabled(sl);
	phase_slider     -> setEnabled(sl);
	phase_spinbox    -> setEnabled(sl);
	ground_checkbox  -> setEnabled(sl);
	neutral_checkbox -> setEnabled(sl);
}

/// @param p les nouvelles proprietes
void ConductorPropertiesWidget::setConductorProperties(const ConductorProperties &p) {
	properties_ = p;
	updateDisplay();
}

/// @return les proprietes editees
ConductorProperties ConductorPropertiesWidget::conductorProperties() const {
	return(properties_);
}
