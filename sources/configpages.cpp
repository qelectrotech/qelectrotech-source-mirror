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
#include "configpages.h"
#include "borderpropertieswidget.h"
#include "conductorpropertieswidget.h"
#include "titleblockpropertieswidget.h"
#include "qetapp.h"
#include "qetdiagrameditor.h"
#include "bordertitleblock.h"
#include "qeticons.h"
#include "exportpropertieswidget.h"
#include "ui/reportpropertiewidget.h"

/**
	Constructeur
	@param parent QWidget parent
*/
NewDiagramPage::NewDiagramPage(QWidget *parent) : ConfigPage(parent) {
	// main tab widget
	QTabWidget *tab_widget = new QTabWidget(this);

	// dimensions by default for diagram
	bpw = new BorderPropertiesWidget(QETDiagramEditor::defaultBorderProperties());	
	// default titleblock properties
	ipw = new TitleBlockPropertiesWidget(QETDiagramEditor::defaultTitleBlockProperties(), true);
	QWidget *diagram_widget = new QWidget();
	QVBoxLayout *diagram_layout = new QVBoxLayout(diagram_widget);
	diagram_layout -> addWidget(bpw);
	diagram_layout -> addWidget(ipw);
	tab_widget->addTab(diagram_widget, tr("Sch\351ma"));
	
	// default conductor properties
	cpw = new ConductorPropertiesWidget(QETDiagramEditor::defaultConductorProperties());
	cpw -> setContentsMargins(0, 0, 0, 0);
	tab_widget->addTab(cpw, tr("Conducteur"));

	// default propertie of report label
	rpw = new ReportPropertieWidget(QETDiagramEditor::defaultReportProperties());
	tab_widget->addTab(rpw, tr("Report de folio"));
	
	QVBoxLayout *vlayout1 = new QVBoxLayout();
	vlayout1->addWidget(tab_widget);

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
	bpw -> borderProperties().toSettings(settings, "diagrameditor/default");
	
	// proprietes du cartouche
	ipw-> titleBlockProperties().toSettings(settings, "diagrameditor/default");
	
	// proprietes par defaut des conducteurs
	cpw -> conductorProperties().toSettings(settings, "diagrameditor/defaultconductor");

	// default report propertie
	rpw->toSettings(settings, "diagrameditor/defaultreport");
}

/// @return l'icone de cette page
QIcon NewDiagramPage::icon() const {
	return(QET::Icons::NewDiagram);
}

/// @return le titre de cette page
QString NewDiagramPage::title() const {
	return(tr("Nouveau sch\351ma", "configuration page title"));
}


/**
	Constructeur
	@param parent QWidget parent
*/
GeneralConfigurationPage::GeneralConfigurationPage(QWidget *parent) : ConfigPage(parent) {
	
	// acces a la configuration de QElectroTech
	QSettings &settings = QETApp::settings();
	bool use_system_colors = settings.value("usesystemcolors", "true").toBool();
	bool tabbed = settings.value("diagrameditor/viewmode", "tabbed") == "tabbed";
	bool integrate_elements = settings.value("diagrameditor/integrate-elements", true).toBool();
	bool highlight_integrated_elements = settings.value("diagrameditor/highlight-integrated-elements", true).toBool();
	QString default_element_informations = settings.value("elementeditor/default-informations", "").toString();
	
	appearance_ = new QGroupBox(tr("Apparence"), this);
	use_system_colors_ = new QCheckBox(tr("Utiliser les couleurs du syst\350me"), appearance_);
	
	projects_view_mode_ = new QGroupBox(tr("Projets"), this);
	windowed_mode_ = new QRadioButton(tr("Utiliser des fen\352tres"), projects_view_mode_);
	tabbed_mode_ = new QRadioButton(tr("Utiliser des onglets"), projects_view_mode_);
	warning_view_mode_ = new QLabel(tr("Ces param\350tres s'appliqueront d\350s la prochaine ouverture d'un \351diteur de sch\351mas."));
	
	elements_management_ = new QGroupBox(tr("Gestion des \351l\351ments"), this);
	integrate_elements_ = new QCheckBox(tr("Int\351grer automatiquement les \351l\351ments dans les projets (recommand\351)"));
	highlight_integrated_elements_ = new QCheckBox(tr("Mettre en valeur dans le panel les \351l\351ments fra\356chement int\351gr\351s", "configuration option"));
	default_element_infos_label_ = new QLabel(
		tr(
			"Chaque \351l\351ment embarque des informations sur ses auteurs, sa licence, ou tout autre renseignement que vous jugerez utile dans un champ libre. "
			"Vous pouvez sp\351cifier ici la valeur par d\351faut de ce champ pour les \351l\351ments que vous cr\351erez :"
		)
	);
	default_element_infos_label_ -> setWordWrap(true);
	default_element_infos_textfield_ = new QTextEdit();
	default_element_infos_textfield_ ->  setAcceptRichText(false);
	
	use_system_colors_ -> setChecked(use_system_colors);
	
	if (tabbed) {
		tabbed_mode_ -> setChecked(true);
	} else {
		windowed_mode_ -> setChecked(true);
	}
	
	integrate_elements_ -> setChecked(integrate_elements);
	highlight_integrated_elements_ -> setChecked(highlight_integrated_elements);
	default_element_infos_textfield_ -> setPlainText(default_element_informations);
	
	QVBoxLayout *appearance_layout = new QVBoxLayout();
	appearance_layout -> addWidget(use_system_colors_);
	appearance_ -> setLayout(appearance_layout);
	
	QVBoxLayout *projects_view_mode_layout = new QVBoxLayout();
	projects_view_mode_layout -> addWidget(windowed_mode_);
	projects_view_mode_layout -> addWidget(tabbed_mode_);
	projects_view_mode_layout -> addWidget(warning_view_mode_);
	projects_view_mode_ -> setLayout(projects_view_mode_layout);
	
	QVBoxLayout *elements_management_layout = new QVBoxLayout();
	elements_management_layout -> addWidget(integrate_elements_);
	elements_management_layout -> addWidget(highlight_integrated_elements_);
	elements_management_layout -> addWidget(default_element_infos_label_);
	elements_management_layout -> addWidget(default_element_infos_textfield_);
	elements_management_ -> setLayout(elements_management_layout);

	//setup lang combo box selection widget
	lang_group_box = new QGroupBox(tr("Langue"), this);
	QHBoxLayout *lang_layout = new QHBoxLayout(lang_group_box);
	lang_combo_box = new QComboBox();
	lang_label = new QLabel(tr("La nouvelle langue sera pris en compte au prochain lancement de QElectroTech."));
	lang_layout->addWidget(lang_combo_box);
	lang_layout->addWidget(lang_label);
	fillLang(settings);
	
	QVBoxLayout *vlayout1 = new QVBoxLayout();
	
	QLabel *title_label_ = new QLabel(title());
	vlayout1 -> addWidget(title_label_);
	
	QFrame *horiz_line_ = new QFrame();
	horiz_line_ -> setFrameShape(QFrame::HLine);
	vlayout1 -> addWidget(horiz_line_);
	
	vlayout1 -> addWidget(appearance_);
	vlayout1 -> addWidget(projects_view_mode_);
	vlayout1 -> addWidget(elements_management_);
	vlayout1 -> addWidget(lang_group_box);
	vlayout1 -> addStretch();
	
	setLayout(vlayout1);
}

/// Destructeur
GeneralConfigurationPage::~GeneralConfigurationPage() {
}

/**
 * @brief GeneralConfigurationPage::applyConf
 * Write all configuration in settings file
 */
void GeneralConfigurationPage::applyConf() {
	QSettings &settings = QETApp::settings();
	
	bool was_using_system_colors = settings.value("usesystemcolors", "true").toBool();
	bool must_use_system_colors  = use_system_colors_ -> isChecked();
	settings.setValue("usesystemcolors", must_use_system_colors);
	if (was_using_system_colors != must_use_system_colors) {
		QETApp::instance() -> useSystemPalette(must_use_system_colors);
	}

	settings.setValue("lang", lang_combo_box->itemData(lang_combo_box->currentIndex()).toString());
	
	QString view_mode = tabbed_mode_ -> isChecked() ? "tabbed" : "windowed";
	settings.setValue("diagrameditor/viewmode", view_mode) ;
	
	settings.setValue("diagrameditor/integrate-elements", integrate_elements_ -> isChecked());
	settings.setValue("diagrameditor/highlight-integrated-elements", highlight_integrated_elements_ -> isChecked());
	settings.setValue("elementeditor/default-informations", default_element_infos_textfield_ -> toPlainText());
}

/// @return l'icone de cette page
QIcon GeneralConfigurationPage::icon() const {
	return(QET::Icons::Settings);
}

/// @return le titre de cette page
QString GeneralConfigurationPage::title() const {
	return(tr("G\351n\351ral", "configuration page title"));
}

/**
 * @brief GeneralConfigurationPage::fillLang
 * fill all available lang in @lang_combo_box
 */
void GeneralConfigurationPage::fillLang(QSettings &settings) {
	lang_combo_box->addItem(QET::Icons::translation,  tr("Syst\350me"), "system");
	lang_combo_box->insertSeparator(1);

	// all lang available on lang directory
	lang_combo_box->addItem(QET::Icons::ar,           tr("Arabe"), "ar");
	lang_combo_box->addItem(QET::Icons::catalonia,    tr("Catalan"), "ca");
	lang_combo_box->addItem(QET::Icons::cs,           tr("Tch\350que"), "cs");
	lang_combo_box->addItem(QET::Icons::de,           tr("Allemand"), "de");
	lang_combo_box->addItem(QET::Icons::gr,           tr("Grec"), "el");
	lang_combo_box->addItem(QET::Icons::en,           tr("Anglais"), "en");
	lang_combo_box->addItem(QET::Icons::es,           tr("Espagnol"), "es");
	lang_combo_box->addItem(QET::Icons::fr,           tr("Fran\347ais"), "fr");
	lang_combo_box->addItem(QET::Icons::hr,           tr("Croate"), "hr");
	lang_combo_box->addItem(QET::Icons::it,           tr("Italien"), "it");
	lang_combo_box->addItem(QET::Icons::pl,           tr("Polonais"), "pl");
	lang_combo_box->addItem(QET::Icons::pt,           tr("Portugais"), "pt");
	lang_combo_box->addItem(QET::Icons::ro,           tr("Roumains"), "ro");
	lang_combo_box->addItem(QET::Icons::ru,           tr("Russe"), "ru");
	lang_combo_box->addItem(QET::Icons::sl,           tr("Slov\350ne"), "sl");

	//set curent index to the lang found in setting file
	//if lang doesn't exist set to system
	for (int i=0; i<lang_combo_box->count(); i++) {
		if (lang_combo_box->itemData(i).toString() == settings.value("lang").toString()) {
			lang_combo_box->setCurrentIndex(i);
			return;
		}
	}
	lang_combo_box->setCurrentIndex(0);
}

/**
	Constructeur
	@param parent QWidget parent
*/
ExportConfigPage::ExportConfigPage(QWidget *parent) : ConfigPage(parent) {
	// epw contient les options d'export
	epw = new ExportPropertiesWidget(QETDiagramEditor::defaultExportProperties());
	
	// layout vertical contenant le titre, une ligne horizontale et epw
	QVBoxLayout *vlayout1 = new QVBoxLayout();
	
	QLabel *title = new QLabel(this -> title());
	vlayout1 -> addWidget(title);
	
	QFrame *horiz_line = new QFrame();
	horiz_line -> setFrameShape(QFrame::HLine);
	vlayout1 -> addWidget(horiz_line);
	vlayout1 -> addWidget(epw);
	vlayout1 -> addStretch();

	// activation du layout
	setLayout(vlayout1);
}

/// Destructeur
ExportConfigPage::~ExportConfigPage() {
}

/**
	Applique la configuration de cette page
*/
void ExportConfigPage::applyConf() {
	QSettings &settings = QETApp::settings();
	epw -> exportProperties().toSettings(settings, "export/default");
}

/// @return l'icone de cette page
QIcon ExportConfigPage::icon() const {
	return(QET::Icons::DocumentExport);
}

/// @return le titre de cette page
QString ExportConfigPage::title() const {
	return(tr("Export", "configuration page title"));
}

/**
	Constructeur
	@param parent QWidget parent
*/
PrintConfigPage::PrintConfigPage(QWidget *parent) : ConfigPage(parent) {
	// epw contient les options d'export
	epw = new ExportPropertiesWidget(QETDiagramEditor::defaultPrintProperties());
	epw -> setPrintingMode(true);
	
	// layout vertical contenant le titre, une ligne horizontale et epw
	QVBoxLayout *vlayout1 = new QVBoxLayout();
	
	QLabel *title = new QLabel(this -> title());
	vlayout1 -> addWidget(title);
	
	QFrame *horiz_line = new QFrame();
	horiz_line -> setFrameShape(QFrame::HLine);
	vlayout1 -> addWidget(horiz_line);
	vlayout1 -> addWidget(epw);
	vlayout1 -> addStretch();

	// activation du layout
	setLayout(vlayout1);
}

/// Destructeur
PrintConfigPage::~PrintConfigPage() {
}

/**
	Applique la configuration de cette page
*/
void PrintConfigPage::applyConf() {
	QString prefix = "print/default";
	
	QSettings &settings = QETApp::settings();
	epw -> exportProperties().toSettings(settings, prefix);
	
	// annule l'enregistrement de certaines proprietes non pertinentes
	settings.remove(prefix + "path");
	settings.remove(prefix + "format");
	settings.remove(prefix + "area");
}

/// @return l'icone de cette page
QIcon PrintConfigPage::icon() const {
	return(QET::Icons::Printer);
}

/// @return le titre de cette page
QString PrintConfigPage::title() const {
	return(tr("Impression", "configuration page title"));
}
