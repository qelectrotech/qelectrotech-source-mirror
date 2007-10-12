#include "conductorproperties.h"
#include "conductor.h"

/**
	Constructeur
	@param parent QWidget parent
*/
ConductorPropertiesWidget::ConductorPropertiesWidget(QWidget *parent) :
	QWidget(parent),
	type_(Conductor::Multi)
{
	buildInterface();
}

/// construit l'interface du widget
void ConductorPropertiesWidget::buildInterface() {
	
	setFixedSize(380, 270);
	
	QVBoxLayout *main_layout = new QVBoxLayout(this);
	
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
	phase_checkbox -> setIcon(QIcon(":/ico/phase.png"));
	phase_slider = new QSlider(Qt::Horizontal);
	phase_slider -> setRange(1, 3);
	phase_spinbox = new QSpinBox();
	phase_spinbox -> setRange(1, 3);
	singleline_layout3 -> addWidget(phase_checkbox);
	singleline_layout3 -> addWidget(phase_slider);
	singleline_layout3 -> addWidget(phase_spinbox);
	
	QVBoxLayout *singleline_layout2 = new QVBoxLayout();
	ground_checkbox = new QCheckBox(tr("terre"));
	ground_checkbox -> setIcon(QIcon(":/ico/ground.png"));
	neutral_checkbox = new QCheckBox(tr("neutre"));
	neutral_checkbox -> setIcon(QIcon(":/ico/neutral.png"));
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
	radio_buttons -> addButton(simple,     Conductor::Simple);
	radio_buttons -> addButton(multiline,  Conductor::Multi);
	radio_buttons -> addButton(singleline, Conductor::Single);
	
	buildConnections();
	setConductorType(Conductor::Multi);
}

/// Met en place les connexions signaux/slots
void ConductorPropertiesWidget::buildConnections() {
	connect(phase_slider,      SIGNAL(valueChanged(int)),  phase_spinbox, SLOT(setValue(int)));
	connect(phase_spinbox,     SIGNAL(valueChanged(int)),  phase_slider,  SLOT(setValue(int)));
	connect(ground_checkbox,   SIGNAL(toggled(bool)),      this,          SLOT(updateSingleLineConfig()));
	connect(neutral_checkbox,  SIGNAL(toggled(bool)),      this,          SLOT(updateSingleLineConfig()));
	connect(phase_checkbox,    SIGNAL(toggled(bool)),      this,          SLOT(updateSingleLineConfig()));
	connect(phase_slider,      SIGNAL(valueChanged(int)),  this,          SLOT(updateSingleLineConfig()));
	connect(radio_buttons,     SIGNAL(buttonClicked(int)), this,          SLOT(setConductorType(int)));
}

/// Enleve les connexions signaux/slots
void ConductorPropertiesWidget::destroyConnections() {
	disconnect(phase_slider,      SIGNAL(valueChanged(int)),  phase_spinbox, SLOT(setValue(int)));
	disconnect(phase_spinbox,     SIGNAL(valueChanged(int)),  phase_slider,  SLOT(setValue(int)));
	disconnect(ground_checkbox,   SIGNAL(toggled(bool)),      this,          SLOT(updateSingleLineConfig()));
	disconnect(neutral_checkbox,  SIGNAL(toggled(bool)),      this,          SLOT(updateSingleLineConfig()));
	disconnect(phase_checkbox,    SIGNAL(toggled(bool)),      this,          SLOT(updateSingleLineConfig()));
	disconnect(phase_slider,      SIGNAL(valueChanged(int)),  this,          SLOT(updateSingleLineConfig()));
	disconnect(radio_buttons,     SIGNAL(buttonClicked(int)), this,          SLOT(setConductorType(int)));
}

/// Destructeur
ConductorPropertiesWidget::~ConductorPropertiesWidget() {
}

/// Met a jour les proprietes unifilaires
void ConductorPropertiesWidget::updateSingleLineConfig() {
	slp.hasGround = ground_checkbox -> isChecked();
	slp.hasNeutral = neutral_checkbox -> isChecked();
	slp.setPhasesCount(phase_checkbox -> isChecked() ? phase_spinbox -> value() : 0);
	updatePreview();
}

/// Met a jour l'affichage des proprietes unifilaires
void ConductorPropertiesWidget::updateSingleLineDisplay() {
	destroyConnections();
	ground_checkbox -> setChecked(slp.hasGround);
	neutral_checkbox -> setChecked(slp.hasNeutral);
	phase_spinbox -> setValue(slp.phasesCount());
	phase_slider -> setValue(slp.phasesCount());
	phase_checkbox -> setChecked(slp.phasesCount());
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
	slp.draw(&painter, QET::Horizontal, pixmap_rect);
	painter.end();
	preview -> setPixmap(pixmap);
}

/// @return true si le widget est en mode unifilaire, false sinon
Conductor::ConductorType ConductorPropertiesWidget::conductorType() const {
	return(type_);
}

/**
	Passe le widget en mode simple, unifilaire ou multifilaire
	@param t le type de conducteur
*/
void ConductorPropertiesWidget::setConductorType(Conductor::ConductorType t) {
	type_ = t;
	// widgets lies au simple
	simple        -> setChecked(t == Conductor::Simple);
	
	// widgets lies au mode multifilaire
	multiline        -> setChecked(t == Conductor::Multi);
	text_field       -> setEnabled(t == Conductor::Multi);
	
	// widgets lies au mode unifilaire
	bool sl = (t == Conductor::Single);
	singleline       -> setChecked(sl);
	preview          -> setEnabled(sl);
	phase_checkbox   -> setEnabled(sl);
	phase_slider     -> setEnabled(sl);
	phase_spinbox    -> setEnabled(sl);
	ground_checkbox  -> setEnabled(sl);
	neutral_checkbox -> setEnabled(sl);
	updateSingleLineDisplay();
}

void ConductorPropertiesWidget::setConductorType(int t) {
	setConductorType(static_cast<Conductor::ConductorType>(t));
}

/// @param prop Les nouvelles proprietes unifilaires de ce conducteur
void ConductorPropertiesWidget::setSingleLineProperties(const SingleLineProperties &prop) {
	slp = prop;
	updateSingleLineDisplay();
}

/// @return les proprietes unifilaires de ce conducteur
SingleLineProperties ConductorPropertiesWidget::singleLineProperties() const {
	return(slp);
}

/// @param text Le texte de ce conducteur
void ConductorPropertiesWidget::setConductorText(const QString &text) {
	text_field -> setText(text);
}

/// @return Le texte de ce conducteur
QString ConductorPropertiesWidget::conductorText() const {
	return(text_field -> text());
}
