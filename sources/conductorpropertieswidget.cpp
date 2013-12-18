/*
	Copyright 2006-2013 The QElectroTech Team
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
#include "qetgraphicsitem/conductor.h"
#include "qeticons.h"
#include "qetapp.h"

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
	
	setFocusPolicy(Qt::StrongFocus);
	
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
	QLabel *size_text = new QLabel(tr("Taille"), this);
	text_size_sb = new QSpinBox(this);
	text_size_sb->setRange(5,9);
	multiline_layout -> addWidget(text);
	multiline_layout -> addWidget(text_field);
	multiline_layout -> addWidget(size_text);
	multiline_layout -> addWidget(text_size_sb);

	QGridLayout *rotate_text_layout = new QGridLayout;
	QLabel *rotate_label = new QLabel(tr("Rotation du texte sur conducteur :"));
	rotate_text_layout -> addWidget(rotate_label, 0, 0);

	QLabel *verti_text = new QLabel(tr("Vertical"));
	verti_select = QETApp::createTextOrientationSpinBoxWidget();
	rotate_text_layout -> addWidget(verti_text, 1, 0);
	rotate_text_layout -> setAlignment(verti_text, Qt::AlignCenter);
	rotate_text_layout -> addWidget(verti_select, 2, 0);

	QLabel *horiz_text = new QLabel(tr("Horizontal"));
	horiz_select = QETApp::createTextOrientationSpinBoxWidget();
	rotate_text_layout -> addWidget(horiz_text, 1, 1);
	rotate_text_layout -> setAlignment(horiz_text, Qt::AlignCenter);
	rotate_text_layout -> addWidget(horiz_select, 2, 1);
	
	singleline = new QRadioButton(tr("Unifilaire"));
	
	ground_checkbox = new QCheckBox(tr("terre"));
	ground_checkbox -> setIcon(QET::Icons::Ground);
	neutral_checkbox = new QCheckBox(tr("neutre"));
	neutral_checkbox -> setIcon(QET::Icons::Neutral);
	merge_checkbox = new QCheckBox(tr("PEN", "Protective Earth Neutral"));
	merge_checkbox -> setToolTip(tr("Protective Earth Neutral", "Tooltip displaying the meaning of the 'PEN' acronym"));
	
	QVBoxLayout *singleline_layout5 = new QVBoxLayout();
	singleline_layout5 -> addWidget(ground_checkbox);
	singleline_layout5 -> addWidget(neutral_checkbox);
	
	QHBoxLayout *singleline_layout4 = new QHBoxLayout();
	singleline_layout4 -> addLayout(singleline_layout5);
	singleline_layout4 -> addWidget(merge_checkbox, 4, Qt::AlignVCenter | Qt::AlignLeft);
	
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
	singleline_layout2 -> addLayout(singleline_layout4);
	singleline_layout2 -> addLayout(singleline_layout3);
	
	QHBoxLayout *singleline_layout1 = new QHBoxLayout();
	preview = new QLabel();
	preview -> setFixedSize(150, 150);
	singleline_layout1 -> addWidget(preview);
	singleline_layout1 -> addLayout(singleline_layout2);
	
	QGroupBox *groupbox2 = new QGroupBox(tr("Apparence du conducteur"));
	main_layout -> addWidget(groupbox2);
	
	QVBoxLayout *groupbox2_layout = new QVBoxLayout();
	groupbox2 -> setLayout(groupbox2_layout);
	
	QHBoxLayout *color_layout = new QHBoxLayout();
	QLabel *text1 = new QLabel(tr("Couleur :"));
	color_button = new QPushButton("");
	color_layout -> addWidget(text1);
	color_layout -> addWidget(color_button);
	
	QHBoxLayout *style_layout = new QHBoxLayout();
	QLabel *text2 = new QLabel(tr("Style :", "conductor line style"));
	line_style = new QComboBox();
	line_style -> addItem(tr("Trait plein", "conductor style: solid line"), Qt::SolidLine);
	line_style -> addItem(tr("Trait en pointill\351s", "conductor style: dashed line"), Qt::DashLine);
	line_style -> addItem(tr("Traits et points", "conductor style: dashed and dotted line"), Qt::DashDotLine);
	style_layout -> addWidget(text2);
	style_layout -> addWidget(line_style);
	
	setColorButton(properties_.color);
	int index = line_style -> findData(properties_.style);
	if (index != -1) line_style -> setCurrentIndex(index);
	
	groupbox_layout -> addWidget(simple);
	groupbox_layout -> addWidget(multiline);
	groupbox_layout -> addLayout(multiline_layout);
	groupbox_layout -> addLayout(rotate_text_layout);
	groupbox_layout -> addWidget(singleline);
	groupbox_layout -> addLayout(singleline_layout1);
	
	groupbox2_layout -> addLayout(color_layout);
	groupbox2_layout -> addLayout(style_layout);

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
	connect(merge_checkbox,    SIGNAL(toggled(bool)),                this,          SLOT(updateConfig()));
	connect(phase_checkbox,    SIGNAL(toggled(bool)),                this,          SLOT(updateConfig()));
	connect(phase_slider,      SIGNAL(valueChanged(int)),            this,          SLOT(updateConfig()));
	connect(radio_buttons,     SIGNAL(buttonClicked(int)),           this,          SLOT(updateConfig()));
	connect(text_field,        SIGNAL(textChanged(const QString &)), this,          SLOT(updateConfig()));
	connect(line_style,        SIGNAL(currentIndexChanged(int)),     this,          SLOT(updateConfig()));
	connect(color_button,      SIGNAL(clicked()),                    this,          SLOT(chooseColor()));
	connect(verti_select,      SIGNAL(editingFinished(double)),      this,          SLOT(updateConfig()));
	connect(horiz_select,      SIGNAL(editingFinished(double)),      this,          SLOT(updateConfig()));
	connect(text_size_sb,      SIGNAL(valueChanged(int)),            this,          SLOT(updateConfig()));
}

/**
	Demande a l'utilisateur de choisir une couleur via un dialogue approprie.
*/
void ConductorPropertiesWidget::chooseColor() {
	QColor user_chosen_color = QColorDialog::getColor(properties_.color);
	if (user_chosen_color.isValid()) {
		setColorButton(user_chosen_color);
		updateConfig();
	}
}

/**
	@return la couleur actuelle du bouton permettant de choisir la couleur du
	conducteur
*/
QColor ConductorPropertiesWidget::colorButton() const {
	return(color_button -> palette().color(QPalette::Button));
}

/**
	Change la couleur du bouton permettant de choisir la couleur du conducteur
	@param color Nouvelle couleur a afficher
*/
void ConductorPropertiesWidget::setColorButton(const QColor &color) {
	QPalette palette;
	palette.setColor(QPalette::Button, color);
	color_button -> setPalette(palette);
}

/// Enleve les connexions signaux/slots
void ConductorPropertiesWidget::destroyConnections() {
	disconnect(phase_slider,      SIGNAL(valueChanged(int)),            phase_spinbox, SLOT(setValue(int)));
	disconnect(phase_spinbox,     SIGNAL(valueChanged(int)),            phase_slider,  SLOT(setValue(int)));
	disconnect(ground_checkbox,   SIGNAL(toggled(bool)),                this,          SLOT(updateConfig()));
	disconnect(neutral_checkbox,  SIGNAL(toggled(bool)),                this,          SLOT(updateConfig()));
	disconnect(merge_checkbox,    SIGNAL(toggled(bool)),                this,          SLOT(updateConfig()));
	disconnect(phase_checkbox,    SIGNAL(toggled(bool)),                this,          SLOT(updateConfig()));
	disconnect(phase_slider,      SIGNAL(valueChanged(int)),            this,          SLOT(updateConfig()));
	disconnect(radio_buttons,     SIGNAL(buttonClicked(int)),           this,          SLOT(updateConfig()));
	disconnect(text_field,        SIGNAL(textChanged(const QString &)), this,          SLOT(updateConfig()));
	disconnect(color_button,      SIGNAL(clicked()),                    this,          SLOT(chooseColor()));
	disconnect(line_style,        SIGNAL(currentIndexChanged(int)),     this,          SLOT(updateConfig()));
	disconnect(verti_select,      SIGNAL(editingFinished(double)),      this,          SLOT(updateConfig()));
	disconnect(horiz_select,      SIGNAL(editingFinished(double)),      this,          SLOT(updateConfig()));
	disconnect(text_size_sb,      SIGNAL(valueChanged(int)),            this,          SLOT(updateConfig()));
}

/// Destructeur
ConductorPropertiesWidget::~ConductorPropertiesWidget() {
	delete verti_select;
	delete horiz_select;
}

/// Met a jour les proprietes
void ConductorPropertiesWidget::updateConfig() {
	properties_.type = static_cast<ConductorProperties::ConductorType>(radio_buttons -> checkedId());
	properties_.color = colorButton();
	properties_.style = static_cast<Qt::PenStyle>(line_style -> itemData(line_style -> currentIndex()).toInt());
	properties_.text = text_field -> text();
	properties_.text_size = text_size_sb->value();
	properties_.verti_rotate_text = verti_select -> value();
	properties_.horiz_rotate_text = horiz_select -> value();
	properties_.singleLineProperties.hasGround = ground_checkbox -> isChecked();
	properties_.singleLineProperties.hasNeutral = neutral_checkbox -> isChecked();
	properties_.singleLineProperties.is_pen = merge_checkbox -> isChecked();
	properties_.singleLineProperties.setPhasesCount(phase_checkbox -> isChecked() ? phase_spinbox -> value() : 0);
	
	updateDisplay();
}

/// Met a jour l'affichage des proprietes
void ConductorPropertiesWidget::updateDisplay() {
	destroyConnections();
	
	setConductorType(properties_.type);
	setColorButton(properties_.color);
	int index = line_style -> findData(properties_.style);
	if (index != -1) line_style -> setCurrentIndex(index);
	text_field -> setText(properties_.text);
	text_size_sb -> setValue(properties_.text_size);
	ground_checkbox -> setChecked(properties_.singleLineProperties.hasGround);
	neutral_checkbox -> setChecked(properties_.singleLineProperties.hasNeutral);
	merge_checkbox -> setChecked(properties_.singleLineProperties.is_pen);
	merge_checkbox -> setEnabled(!isReadOnly() && properties_.type == ConductorProperties::Single && ground_checkbox -> isChecked() && neutral_checkbox -> isChecked());
	phase_spinbox -> setValue(properties_.singleLineProperties.phasesCount());
	phase_slider -> setValue(properties_.singleLineProperties.phasesCount());
	phase_checkbox -> setChecked(properties_.singleLineProperties.phasesCount());
	verti_select -> setValue(properties_.verti_rotate_text);
	horiz_select -> setValue(properties_.horiz_rotate_text);
	
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
	merge_checkbox   -> setEnabled(sl);
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

/**
	@return true si ce widget est en lecture seule, false sinon
*/
bool ConductorPropertiesWidget::isReadOnly() const {
	return(text_field -> isReadOnly());
}

/**
	@param ro true pour passer ce widget en lecture seule, false sinon
*/
void ConductorPropertiesWidget::setReadOnly(bool ro) {
	// enable or disable all child widgets according to the read only state
	simple    -> setDisabled(ro);
	multiline -> setDisabled(ro);
	singleline -> setDisabled(ro);
	text_field -> setReadOnly(ro);
	text_size_sb -> setReadOnly(ro);
	phase_checkbox -> setDisabled(ro);
	phase_spinbox -> setReadOnly(ro);
	ground_checkbox -> setDisabled(ro);
	neutral_checkbox -> setDisabled(ro);
	merge_checkbox -> setDisabled(ro);
	color_button -> setDisabled(ro);
	line_style -> setDisabled(ro);
	// if the widget is not read-only, we still need to disable some widgets for consistency
	if (!ro) {
		updateDisplay();
	}
}

/**
	Gere le focus de ce widget
*/
void ConductorPropertiesWidget::focusInEvent(QFocusEvent *event) {
	if (properties_.type == ConductorProperties::Multi) {
		text_field -> setFocus(Qt::ActiveWindowFocusReason);
		text_field -> selectAll();
	} else if (properties_.type == ConductorProperties::Single) {
		phase_spinbox -> setFocus(Qt::ActiveWindowFocusReason);
		phase_spinbox -> selectAll();
	}
	QWidget::focusInEvent(event);
}
