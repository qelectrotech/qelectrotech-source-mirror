/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "bordertitleblock.h"
#include "qeticons.h"
#include "exportpropertieswidget.h"
#include "ui/reportpropertiewidget.h"
#include "ui/xrefpropertieswidget.h"
#include "qetproject.h"
#include "reportproperties.h"
#include "qetapp.h"
#include <QFontDialog>
#include <QFont>
#include <QSizePolicy>
#include <utility>

/**
 * @brief NewDiagramPage::NewDiagramPage
 * Default constructor
 * @param project, If project, edit the propertie of Project
 * else edit the properties by default of QElectroTech
 * @param parent, parent widget
 */
NewDiagramPage::NewDiagramPage(QETProject *project, QWidget *parent, ProjectPropertiesDialog *ppd) :
	ConfigPage (parent),
	ppd_ (ppd),
	m_project  (project)
{
	//By default we set the global default properties

	// dimensions by default for diagram
	bpw = new BorderPropertiesWidget(BorderProperties::defaultProperties());
	// default titleblock properties
	QList <TitleBlockTemplatesCollection *> c;
	c << QETApp::commonTitleBlockTemplatesCollection() << QETApp::customTitleBlockTemplatesCollection();
	if (m_project) c << m_project->embeddedTitleBlockTemplatesCollection();
	ipw = new TitleBlockPropertiesWidget(c, TitleBlockProperties::defaultProperties(), true, project, parent);
	// default conductor properties
	m_cpw = new ConductorPropertiesWidget(ConductorProperties::defaultProperties());
	m_cpw->setHiddenAvailableAutonum(true);
	// default propertie of report label
	rpw = new ReportPropertieWidget(ReportProperties::defaultProperties());
	// default properties of xref
	xrefpw = new XRefPropertiesWidget(XRefProperties::defaultProperties(), this);

	//If there is a project, we edit his properties
	if (m_project) {
		bpw	   -> setProperties		  (m_project -> defaultBorderProperties());
		m_cpw	   -> setProperties       (m_project -> defaultConductorProperties());
		ipw    -> setProperties       (m_project -> defaultTitleBlockProperties());
		rpw	   -> setReportProperties (m_project -> defaultReportProperties());
		xrefpw -> setProperties		  (m_project -> defaultXRefProperties());
	}

	connect(ipw,SIGNAL(openAutoNumFolioEditor(QString)),this,SLOT(changeToAutoFolioTab()));

	// main tab widget
	QTabWidget *tab_widget      = new QTabWidget(this);
	QWidget *diagram_widget     = new QWidget();
	QVBoxLayout *diagram_layout = new QVBoxLayout(diagram_widget);

	diagram_layout -> addWidget(bpw);
	diagram_layout -> addWidget(ipw);
	tab_widget->setMinimumSize(800, 650);

	tab_widget -> addTab (diagram_widget, tr("Folio"));
	tab_widget -> addTab (m_cpw,            tr("Conducteur"));
	tab_widget -> addTab (rpw,            tr("Reports de folio"));
	tab_widget -> addTab (xrefpw,         tr("Références croisées"));

	QVBoxLayout *vlayout1 = new QVBoxLayout();
	vlayout1->addWidget(tab_widget);

	setLayout(vlayout1);
}

/**
 * @brief NewDiagramPage::~NewDiagramPage
 */
NewDiagramPage::~NewDiagramPage() {
	disconnect(ipw,SIGNAL(openAutoNumFolioEditor(QString)),this,SLOT(changeToAutoFolioTab()));
}

/**
 * @brief NewDiagramPage::applyConf
 * Apply conf for this page.
 * If there is a project, save in the project,
 * else save to the default conf of QElectroTech
 */
void NewDiagramPage::applyConf() {
	if (m_project) { //If project we save to the project
		if (m_project -> isReadOnly()) return;
		bool modified_project = false;

		BorderProperties new_border_prop = bpw -> properties();
		if (m_project -> defaultBorderProperties() != new_border_prop) {
			m_project -> setDefaultBorderProperties(bpw -> properties());
			modified_project = true;
		}

		TitleBlockProperties new_tbt_prop = ipw -> properties();
		if (m_project -> defaultTitleBlockProperties() != new_tbt_prop) {
			m_project -> setDefaultTitleBlockProperties(ipw -> properties());
			modified_project = true;
		}

		ConductorProperties new_conductor_prop = m_cpw -> properties();
		if (m_project -> defaultConductorProperties() != new_conductor_prop) {
			m_project -> setDefaultConductorProperties(m_cpw -> properties());
			modified_project = true;
		}

		QString new_report_prop = rpw -> ReportProperties();
		if (m_project -> defaultReportProperties() != new_report_prop) {
			m_project -> setDefaultReportProperties(new_report_prop);
			modified_project = true;
		}

		QHash<QString, XRefProperties> new_xref_properties = xrefpw -> properties();
		if (m_project -> defaultXRefProperties() != new_xref_properties) {
			m_project -> setDefaultXRefProperties(new_xref_properties);
			modified_project = true;
		}

		if (modified_project) {
			m_project -> setModified(modified_project);
		}

	} else { //Else we save to the default value
		QSettings settings;

		// dimensions des nouveaux schemas
		bpw -> properties().toSettings(settings, "diagrameditor/default");

		// proprietes du cartouche
		ipw-> properties().toSettings(settings, "diagrameditor/default");

		// proprietes par defaut des conducteurs
		m_cpw -> properties().toSettings(settings, "diagrameditor/defaultconductor");

		// default report propertie
		rpw->toSettings(settings, "diagrameditor/defaultreport");

		// default xref properties
		QHash <QString, XRefProperties> hash_xrp = xrefpw -> properties();
		foreach (QString key, hash_xrp.keys()) {
			XRefProperties xrp = hash_xrp[key];
			QString str("diagrameditor/defaultxref");
			xrp.toSettings(settings, str += key);
		}
	}

}

/**
 * @brief NewDiagramPage::icon
 * @return  icon of this page
 */
QIcon NewDiagramPage::icon() const {
	if (m_project) return(QET::Icons::NewDiagram);
	return(QET::Icons::Projects);
}

/**
 * @brief NewDiagramPage::title
 * @return title of this page
 */
QString NewDiagramPage::title() const {
	if (m_project) return(tr("Nouveau folio", "configuration page title"));
	return(tr("Nouveau projet", "configuration page title"));
}

/**
 * @brief NewDiagramPage::changeToAutoFolioTab
 * Set the current tab to Autonum
 */
void NewDiagramPage::changeToAutoFolioTab(){
	if (m_project){
		ppd_->setCurrentPage(ProjectPropertiesDialog::Autonum);
		ppd_->changeToFolio();
		ppd_->exec();
	}
}

/**
 * @brief NewDiagramPage::setFolioAutonum
 * Set temporary TBP to use in auto folio num
 */
void NewDiagramPage::setFolioAutonum(QString autoNum){
	TitleBlockProperties tbptemp = ipw->propertiesAutoNum(std::move(autoNum));
	ipw->setProperties(tbptemp);
	applyConf();
}

/**
 * @brief NewDiagramPage::saveCurrentTbp
 * Save current TBP to retrieve after auto folio	num
 */
void NewDiagramPage::saveCurrentTbp(){
	savedTbp = ipw->properties();
}

/**
 * @brief NewDiagramPage::loadSavedTbp
 * Retrieve saved auto folio num
 */
void NewDiagramPage::loadSavedTbp(){
	ipw->setProperties(savedTbp);
	applyConf();
}

/**
	Constructeur
	@param parent QWidget parent
*/
ExportConfigPage::ExportConfigPage(QWidget *parent) : ConfigPage(parent) {
	// epw contient les options d'export
	epw = new ExportPropertiesWidget(ExportProperties::defaultExportProperties());
	
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
void ExportConfigPage::applyConf()
{
	QSettings settings;
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
	epw = new ExportPropertiesWidget(ExportProperties::defaultPrintProperties());
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
 * @brief PrintConfigPage::applyConf
 * Apply the config of this page
 */
void PrintConfigPage::applyConf()
{
	QString prefix = "print/default";
	
	QSettings settings;
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

