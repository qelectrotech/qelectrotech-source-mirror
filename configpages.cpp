#include "configpages.h"
#include "qetapp.h"

/**
	Constructeur
	@param parent QWidget parent
*/
NewDiagramPage::NewDiagramPage(QWidget *parent) : ConfigPage(parent) {
	
	// acces a la configuration de QElectroTech
	QSettings &settings = QETApp::settings();
	
	// recupere les dimensions du schema
	int columns_count_value  = settings.value("diagrameditor/defaultcols",       15).toInt();
	int columns_width_value  = qRound(settings.value("diagrameditor/defaultcolsize",  50.0).toDouble());
	int columns_height_value = qRound(settings.value("diagrameditor/defaultheight",  500.0).toDouble());
	
	QVBoxLayout *vlayout1 = new QVBoxLayout();
	
	QLabel *title = new QLabel(tr("Nouveau sch\351ma"));
	vlayout1 -> addWidget(title);
	
	QFrame *horiz_line = new QFrame();
	horiz_line -> setFrameShape(QFrame::HLine);
	vlayout1 -> addWidget(horiz_line);
	
	QHBoxLayout *hlayout1 = new QHBoxLayout();
	QVBoxLayout *vlayout2 = new QVBoxLayout();
	
	QGroupBox *diagram_size_box = new QGroupBox(tr("Dimensions du sch\351ma"));
	diagram_size_box -> setMinimumWidth(300);
	QGridLayout *diagram_size_box_layout = new QGridLayout(diagram_size_box);
	
	QLabel *ds1 = new QLabel(tr("Colonnes :"));
	
	columns_count = new QSpinBox(diagram_size_box);
	columns_count -> setMinimum(3); /// @todo methode statique pour recuperer ca
	columns_count -> setValue(columns_count_value);
	
	columns_width = new QSpinBox(diagram_size_box);
	columns_width -> setMinimum(1);
	columns_width -> setSingleStep(10);
	columns_width -> setValue(columns_width_value);
	columns_width -> setPrefix(tr("\327"));
	columns_width -> setSuffix(tr("px"));
	
	QLabel *ds2 = new QLabel(tr("Hauteur :"));
	
	columns_height = new QSpinBox(diagram_size_box);
	columns_height -> setRange(80, 10000); /// @todo methode statique pour recuperer ca
	columns_height -> setSingleStep(80);
	columns_height -> setValue(columns_height_value);
	
	diagram_size_box_layout -> addWidget(ds1,            0, 0);
	diagram_size_box_layout -> addWidget(columns_count,  0, 1);
	diagram_size_box_layout -> addWidget(columns_width,  0, 2);
	diagram_size_box_layout -> addWidget(ds2,            1, 0);
	diagram_size_box_layout -> addWidget(columns_height, 1, 1);
	
	QGroupBox *inset_infos = new QGroupBox(tr("Informations du cartouche"), this);
	inset_infos -> setMinimumWidth(300);
	
	inset_title = new QLineEdit(settings.value("diagrameditor/defaulttitle").toString(), this);
	inset_author = new QLineEdit(settings.value("diagrameditor/defaultauthor").toString(), this);
	
	QButtonGroup *date_policy_group = new QButtonGroup(this);
	inset_no_date = new QRadioButton(tr("Pas de date"), this);
	inset_current_date = new QRadioButton(tr("Date courante"), this);
	inset_fixed_date = new QRadioButton(tr("Date fixe : "), this);
	date_policy_group -> addButton(inset_no_date);
	date_policy_group -> addButton(inset_current_date);
	date_policy_group -> addButton(inset_fixed_date);
	QString settings_date = settings.value("diagrameditor/defaultdate").toString();
	QDate date_diagram;
	if (settings_date == "now") {
		date_diagram = QDate::currentDate();
		inset_current_date -> setChecked(true);
	} else if (settings_date.isEmpty() || settings_date == "null") {
		date_diagram = QDate();
		inset_no_date -> setChecked(true);
	} else {
		date_diagram = QDate::fromString(settings_date, "yyyyMMdd");
		inset_fixed_date -> setChecked(true);
	}
	inset_date = new QDateEdit(date_diagram, this);
	inset_date -> setEnabled(inset_fixed_date -> isChecked());
	connect(inset_fixed_date, SIGNAL(toggled(bool)), inset_date, SLOT(setEnabled(bool)));
	inset_date -> setCalendarPopup(true);
	
	QGridLayout *layout_date = new QGridLayout();
	layout_date -> addWidget(inset_no_date,      0, 0);
	layout_date -> addWidget(inset_current_date, 1, 0);
	layout_date -> addWidget(inset_fixed_date,   2, 0);
	layout_date -> addWidget(inset_date,         2, 1);
	
	inset_filename = new QLineEdit(settings.value("diagrameditor/defaultfilename").toString(), this);
	inset_folio = new QLineEdit(settings.value("diagrameditor/defaultfolio").toString(), this);
	QGridLayout *layout_champs = new QGridLayout(inset_infos);
	
	layout_champs -> addWidget(new QLabel(tr("Titre : ")),   0, 0);
	layout_champs -> addWidget(inset_title,                  0, 1);
	layout_champs -> addWidget(new QLabel(tr("Auteur : ")),  1, 0);
	layout_champs -> addWidget(inset_author,                 1, 1);
	layout_champs -> addWidget(new QLabel(tr("Date : ")),    2, 0);
	layout_champs -> addLayout(layout_date,                  3, 1);
	layout_champs -> addWidget(new QLabel(tr("Fichier : ")), 4, 0);
	layout_champs -> addWidget(inset_filename,               4, 1);
	layout_champs -> addWidget(new QLabel(tr("Folio : ")),   5, 0);
	layout_champs -> addWidget(inset_folio,                  5, 1);
	
	// proprietes par defaut des conducteurs
	ConductorProperties cp;
	cp.fromSettings(settings, "diagrameditor/defaultconductor");
	cpw = new ConductorPropertiesWidget(cp);
	
	vlayout2 -> addWidget(diagram_size_box);
	vlayout2 -> addWidget(inset_infos);
	hlayout1 -> addLayout(vlayout2);
	hlayout1 -> addWidget(cpw);
	vlayout1 -> addLayout(hlayout1);
	vlayout1 -> addStretch(1);
	hlayout1 -> setAlignment(cpw, Qt::AlignTop);
	setLayout(vlayout1);
}

/// Destructeur
NewDiagramPage::~NewDiagramPage() {
}

/**
	Applique la configuration de cette page
*/
void NewDiagramPage::applyConf() {
	QSettings &settings = QETApp::settings();
	
	// dimensions des nouveaux schemas
	settings.setValue("diagrameditor/defaultcols",    columns_count -> value());
	settings.setValue("diagrameditor/defaultcolsize", columns_width -> value());
	settings.setValue("diagrameditor/defaultheight",  columns_height -> value());
	
	// proprietes du cartouche
	settings.setValue("diagrameditor/defaulttitle",    inset_title -> text());
	settings.setValue("diagrameditor/defaultauthor",   inset_author -> text());
	settings.setValue("diagrameditor/defaultfilename", inset_filename -> text());
	settings.setValue("diagrameditor/defaultfolio",    inset_folio -> text());
	QString date_setting_value;
	if (inset_no_date -> isChecked()) date_setting_value = "null";
	else if (inset_current_date -> isChecked()) date_setting_value = "now";
	else date_setting_value = inset_date -> date().toString("yyyyMMdd");
	settings.setValue("diagrameditor/defaultdate", date_setting_value);
	
	// proprietes par defaut des conducteurs
	cpw -> conductorProperties().toSettings(settings, "diagrameditor/defaultconductor");
}

/// @return l'icone de cette page
QIcon NewDiagramPage::icon() const {
	return(QIcon(":/ico/conf_new_diagram.png"));
}

/// @return le titre de cette page
QString NewDiagramPage::title() const {
	return(tr("Nouveau sch\351ma"));
}
