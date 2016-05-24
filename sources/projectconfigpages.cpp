/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "projectconfigpages.h"
#include "qeticons.h"
#include "qetproject.h"
#include "borderpropertieswidget.h"
#include "conductorpropertieswidget.h"
#include "diagramcontextwidget.h"
#include "titleblockpropertieswidget.h"
#include <QtWidgets>
#include "ui/reportpropertiewidget.h"
#include "ui/xrefpropertieswidget.h"
#include "selectautonumw.h"
#include "numerotationcontext.h"
#include "folioautonumbering.h"
/**
	Constructor
	@param project Project this page is editing.
	@param parent Parent QWidget
*/
ProjectConfigPage::ProjectConfigPage(QETProject *project, QWidget *parent) :
	ConfigPage(parent),
	project_(project)
{
}

/**
	Destructor
*/
ProjectConfigPage::~ProjectConfigPage() {
}

/**
	@return the project being edited by this page
*/
QETProject *ProjectConfigPage::project() const {
	return(project_);
}

/**
	Set \a new_project as the project being edited by this page.
	@param read_values True to read values from the project into widgets before setting them read only accordingly, false otherwise. Defaults to true.
	@return the former project
*/
QETProject *ProjectConfigPage::setProject(QETProject *new_project, bool read_values) {
	if (new_project == project_) return(project_);
	
	QETProject *former_project = project_;
	project_ = new_project;
	if (project_ && read_values) {
		readValuesFromProject();
		adjustReadOnly();
	}
	return(former_project);
}

/**
	Apply the configuration after user input
*/
void ProjectConfigPage::applyConf() {
	if (!project_ || project_ -> isReadOnly()) return;
	applyProjectConf();
}

/**
	Initialize the page by calling initWidgets() and initLayout(). Also call
	readValuesFromProject() and adjustReadOnly() if a non-zero project has been
	set. Typically, you should call this function in your subclass constructor.
*/
void ProjectConfigPage::init() {
	initWidgets();
	initLayout();
	if (project_) {
		readValuesFromProject();
		adjustReadOnly();
	}
}

//######################################################################################//

/**
	Constructor
	@param project Project this page is editing.
	@param parent Parent QWidget
*/
ProjectMainConfigPage::ProjectMainConfigPage(QETProject *project, QWidget *parent) :
	ProjectConfigPage(project, parent)
{
	init();
}

/**
	Destructor
*/
ProjectMainConfigPage::~ProjectMainConfigPage() {
}

/**
	@return the title for this page
*/
QString ProjectMainConfigPage::title() const {
	return(tr("Général", "configuration page title"));
}

/**
	@return the icon for this page
*/
QIcon ProjectMainConfigPage::icon() const {
	return(QET::Icons::Settings);
}

/**
	Apply the configuration after user input
*/
void ProjectMainConfigPage::applyProjectConf() {
	bool modified_project = false;
	
	QString new_title = title_value_ -> text();
	if (project_ -> title() != new_title) {
		project_ -> setTitle(new_title);
		modified_project = true;
	}
	
	DiagramContext new_properties = project_variables_ -> context();
	if (project_ -> projectProperties() != new_properties) {
		project_ -> setProjectProperties(new_properties);
		modified_project = true;
	}
	if (modified_project) {
		project_ -> setModified(true);
	}
}

/**
	@return the project title entered by the user
*/
QString ProjectMainConfigPage::projectTitle() const {
	return(title_value_ -> text());
}

/**
	Initialize widgets displayed by the page.
*/
void ProjectMainConfigPage::initWidgets() {
	title_label_ = new QLabel(tr("Titre du projet :", "label when configuring"));
	title_value_ = new QLineEdit();
	title_information_ = new QLabel(tr("Ce titre sera disponible pour tous les folios de ce projet en tant que %projecttitle.", "informative label"));
	project_variables_label_ = new QLabel(
		tr(
			"Vous pouvez définir ci-dessous des propriétés personnalisées qui seront disponibles pour tous les folios de ce projet (typiquement pour les cartouches).",
			 "informative label"
		)
	);
	project_variables_label_ -> setWordWrap(true);
	project_variables_ = new DiagramContextWidget();
	project_variables_ -> setContext(DiagramContext());
}

/**
	Initialize the layout of this page.
*/
void ProjectMainConfigPage::initLayout() {
	QVBoxLayout *main_layout0 = new QVBoxLayout();
	QHBoxLayout *title_layout0 = new QHBoxLayout();
	title_layout0 -> addWidget(title_label_);
	title_layout0 -> addWidget(title_value_);
	main_layout0 -> addLayout(title_layout0);
	main_layout0 -> addWidget(title_information_);
	main_layout0 -> addSpacing(10);
	main_layout0 -> addWidget(project_variables_label_);
	main_layout0 -> addWidget(project_variables_);
	setLayout(main_layout0);
	this -> setMinimumWidth(630);

}

/**
	Read properties from the edited project then fill widgets with them.
*/
void ProjectMainConfigPage::readValuesFromProject() {
	title_value_ -> setText(project_ -> title());
	project_variables_ -> setContext(project_ -> projectProperties());
}

/**
	Set the content of this page read only if the project is read only,
	editable if the project is editable.
 */
void ProjectMainConfigPage::adjustReadOnly() {
	bool is_read_only = project_ -> isReadOnly();
	title_value_ -> setReadOnly(is_read_only);
}

//######################################################################################//

/**
 * @brief ProjectAutoNumConfigPage::ProjectAutoNumConfigPage
 * Default constructor
 * @param project, project to edit
 * @param parent, parent widget
 */
ProjectAutoNumConfigPage::ProjectAutoNumConfigPage (QETProject *project, QWidget *parent) :
	ProjectConfigPage(project, parent)
{
	initWidgets();
	initLayout();
	buildConnections();
	readValuesFromProject();
}

/**
 * @brief ProjectAutoNumConfigPage::title
 * Title of this config page
 * @return
 */
QString ProjectAutoNumConfigPage::title() const {
	return tr("Auto numerotation");
}

/**
 * @brief ProjectAutoNumConfigPage::icon
 * Icon of this config pafe
 * @return
 */
QIcon ProjectAutoNumConfigPage::icon() const {
	return QIcon (QET::Icons::AutoNum);
}

/**
 * @brief ProjectAutoNumConfigPage::applyProjectConf
 */
void ProjectAutoNumConfigPage::applyProjectConf() {}

/**
 * @brief ProjectAutoNumConfigPage::initWidgets
 * Init some widget of this page
 */
void ProjectAutoNumConfigPage::initWidgets() {

	tab_widget = new QTabWidget(this);

	//Conductor Tab
	conductor_tab_widget = new QWidget(this);

	m_label = new QLabel(tr("Numérotations disponibles :", "availables numerotations"), conductor_tab_widget);

	m_context_cb = new QComboBox(conductor_tab_widget);
	m_context_cb->setEditable(true);
	m_context_cb->lineEdit()->setClearButtonEnabled(true);
	m_context_cb->addItem(tr("Nom de la nouvelle numérotation"));

	m_remove_pb = new QPushButton(QET::Icons::EditDelete, QString(), conductor_tab_widget);
	m_remove_pb -> setToolTip(tr("Supprimer la numérotation"));

	m_saw = new SelectAutonumW(conductor_tab_widget);

	//Folio Tab
	folio_tab_widget = new QWidget(this);
	folio_tab_widget->setObjectName("FolioTab");

	m_label_2 = new QLabel(tr("Numérotations disponibles :", "availables numerotations"), folio_tab_widget);

	m_context_cb_2 = new QComboBox(folio_tab_widget);
	m_context_cb_2->setEditable(true);
	m_context_cb_2->lineEdit()->setClearButtonEnabled(true);
	m_context_cb_2->addItem(tr("Nom de la nouvelle numérotation"));

	m_remove_pb_2 = new QPushButton(QET::Icons::EditDelete, QString(), folio_tab_widget);
	m_remove_pb_2 -> setToolTip(tr("Supprimer la numérotation"));

	m_saw_2 = new SelectAutonumW(folio_tab_widget);

	//AutoNumbering Tab
	autoNumbering_tab_widget = new QWidget(this);
	m_faw = new FolioAutonumberingW(project(),autoNumbering_tab_widget);
}

/**
 * @brief ProjectAutoNumConfigPage::initLayout
 * Init the layout of this page
 */
void ProjectAutoNumConfigPage::initLayout() {

	//Conductor tab
	tab_widget->addTab(conductor_tab_widget, tr("Conductor"));

	QHBoxLayout *context_layout = new QHBoxLayout();
	context_layout -> addWidget (m_label);
	context_layout -> addWidget (m_context_cb);
	context_layout -> addWidget (m_remove_pb);

	QVBoxLayout *main_layout = new QVBoxLayout();
	QVBoxLayout *aux_layout = new QVBoxLayout();
	aux_layout->addLayout(context_layout);
	aux_layout->addWidget(m_saw);

	main_layout->addLayout(aux_layout);
	conductor_tab_widget -> setLayout (main_layout);

	// Folio Tab
	tab_widget->addTab(folio_tab_widget, tr("Folio"));

	QHBoxLayout *context_layout_2 = new QHBoxLayout();
	context_layout_2 -> addWidget (m_label_2);
	context_layout_2 -> addWidget (m_context_cb_2);
	context_layout_2 -> addWidget (m_remove_pb_2);

	QVBoxLayout *main_layout_2 = new QVBoxLayout();
	QVBoxLayout *aux_layout_2 = new QVBoxLayout();
	aux_layout_2->addLayout(context_layout_2);
	aux_layout_2->addWidget(m_saw_2);

	main_layout_2->addLayout(aux_layout_2);
	folio_tab_widget -> setLayout (main_layout_2);

	//Auto Numbering Tab
	tab_widget->addTab(autoNumbering_tab_widget,tr ("Folio Auto Numbering"));

	tab_widget->resize(455,590);
}

/**
 * @brief ProjectAutoNumConfigPage::readValuesFromProject
 * Read value stored on project, and update display
 */
void ProjectAutoNumConfigPage::readValuesFromProject() {
	//Conductor Tab
	QList <QString> keys = project_->conductorAutoNum().keys();
	if (!keys.isEmpty()){
	foreach (QString str, keys) { m_context_cb -> addItem(str); }
	}

	//Folio Tab
	QList <QString> keys_2 = project_->folioAutoNum().keys();
	if (!keys_2.isEmpty()){
	foreach (QString str, keys_2) { m_context_cb_2 -> addItem(str);}
	}

	//Folio AutoNumbering Tab
	m_faw->setContext(keys_2);
}

/**
 * @brief ProjectAutoNumConfigPage::adjustReadOnly
 * set this config page disable if project is read only
 */
void ProjectAutoNumConfigPage::adjustReadOnly() {
}

/**
 * @brief ProjectAutoNumConfigPage::buildConnections
 * setup some connections
 */
void ProjectAutoNumConfigPage::buildConnections() {
	
	connect(tab_widget,SIGNAL(currentChanged(int)),this,SLOT(tabChanged(int)));

	//Conductor Tab
	connect (m_context_cb, SIGNAL (currentIndexChanged(QString)), this, SLOT (updateContext(QString)));
	connect (m_saw, SIGNAL (applyPressed()), this, SLOT (saveContext()));
	connect (m_remove_pb, SIGNAL (clicked()), this, SLOT(removeContext()));

	//Folio Tab
	connect (m_context_cb_2, SIGNAL (currentIndexChanged(QString)), this, SLOT (updateContext_2(QString)));
	connect (m_saw_2, SIGNAL (applyPressed()), this, SLOT (saveContext_2()));
	connect (m_remove_pb_2, SIGNAL (clicked()), this, SLOT (removeContext_2()));

	//	Auto Folio Numbering
	connect (m_faw, SIGNAL (applyPressed()),				 this, SLOT (applyAutoNum()));
}

/**
 * @brief ProjectAutoNumConfigPage::updateContext
 * Display the current selected context for conductor
 * @param str, key of context stored in project
 */
void ProjectAutoNumConfigPage::updateContext(QString str) {
	if (str == tr("Nom de la nouvelle numérotation")) m_saw -> setContext(NumerotationContext());
	else m_saw ->setContext(project_->conductorAutoNum(str));
}

/**
 * @brief ProjectAutoNumConfigPage::updateContext_2
 * Display the current selected context for folio
 * @param str, key of context stored in project
 */
void ProjectAutoNumConfigPage::updateContext_2(QString str) {
	if (str == tr("Nom de la nouvelle numérotation")) m_saw_2 -> setContext(NumerotationContext());
	else m_saw_2 ->setContext(project_->folioAutoNum(str));
}

/**
 * @brief ProjectAutoNumConfigPage::saveContext
 * Save the current displayed conductor context in project
 */
void ProjectAutoNumConfigPage::saveContext() {
	// If the text is the default text "Name of new numerotation" save the edited context
	// With the the name "No name"
	if (m_context_cb -> currentText() == tr("Nom de la nouvelle numérotation")) {
		project_->addConductorAutoNum (tr("Sans nom"), m_saw -> toNumContext());
		m_context_cb -> addItem(tr("Sans nom"));
	}
	// If the text isn't yet to the autonum of the project, add this new item to the combo box.
	else if ( !project_ -> conductorAutoNum().keys().contains( m_context_cb->currentText())) {
		project()->addConductorAutoNum(m_context_cb->currentText(), m_saw->toNumContext());
		m_context_cb -> addItem(m_context_cb->currentText());
	}
	// Else, the text already exist in the autonum of the project, just update the context
	else {
		project_->addConductorAutoNum (m_context_cb -> currentText(), m_saw -> toNumContext());
	}
}

/**
 * @brief ProjectAutoNumConfigPage::saveContext_2
 * Save the current displayed folio context in project
 */
void ProjectAutoNumConfigPage::saveContext_2() {
	// If the text is the default text "Name of new numerotation" save the edited context
	// With the the name "No name"
	if (m_context_cb_2 -> currentText() == tr("Nom de la nouvelle numérotation")) {
		project_->addFolioAutoNum (tr("Sans nom"), m_saw_2 -> toNumContext());
		m_context_cb_2 -> addItem(tr("Sans nom"));
	}
	// If the text isn't yet to the autonum of the project, add this new item to the combo box.
	else if ( !project_ -> folioAutoNum().keys().contains( m_context_cb_2->currentText())) {
		project()->addFolioAutoNum(m_context_cb_2->currentText(), m_saw_2->toNumContext());
		m_context_cb_2 -> addItem(m_context_cb_2->currentText());
	}
	// Else, the text already exist in the autonum of the project, just update the context
	else {
		project_->addFolioAutoNum (m_context_cb_2 -> currentText(), m_saw_2 -> toNumContext());
	}
}

/**
 * @brief ProjectAutoNumConfigPage::applyAutoNum
 * Apply auto folio numbering, New Folios or Selected Folios
 */
void ProjectAutoNumConfigPage::applyAutoNum() {

	if (m_faw->newFolios){
		int foliosRemaining = m_faw->newFoliosNumber();
		emit (saveCurrentTbp());
		emit (setAutoNum(m_faw->autoNumSelected()));
		while (foliosRemaining > 0){
					qDebug() << foliosRemaining;
			project()->autoFolioNumberingNewFolios();
			foliosRemaining = foliosRemaining-1;
		}
		emit (loadSavedTbp());
	}
	else{
		QString autoNum   = m_faw->autoNumSelected();
		int fromFolio = m_faw->fromFolio();
		int toFolio   = m_faw->toFolio();
		project_->autoFolioNumberingSelectedFolios(fromFolio,toFolio,autoNum);		
	}
}

/**
 * @brief ProjectAutoNumConfigPage::removeContext
 * Remove from project the current conductor numerotation context
 */
void ProjectAutoNumConfigPage::removeContext() {
	//if default text, return
	if ( m_context_cb -> currentText() == tr("Nom de la nouvelle numérotation") ) return;
	project_	 -> removeConductorAutonum	(m_context_cb -> currentText() );
	m_context_cb -> removeItem				(m_context_cb -> currentIndex() );
}
/**
 * @brief ProjectAutoNumConfigPage::removeContext_2
 * Remove from project the current folio numerotation context
 */
void ProjectAutoNumConfigPage::removeContext_2() {
	//if default text, return
	if ( m_context_cb_2 -> currentText() == tr("Nom de la nouvelle numérotation") ) return;
	project_	   -> removeFolioAutoNum	 (m_context_cb_2 -> currentText() );
	m_context_cb_2 -> removeItem			 (m_context_cb_2 -> currentIndex() );
}
/**
 * @brief ProjectAutoNumConfigPage::changeToTab
 * @param tab index
 * Change to Selected Tab
 */
void ProjectAutoNumConfigPage::changeToTab(int i){
		tab_widget->setCurrentIndex(i);
}

/**
 * @brief ProjectAutoNumConfigPage::tabChanged
 * @param tab index
 * Used to resize window to correct size
 */
void ProjectAutoNumConfigPage::tabChanged(int i){
	if (i>0){
		if (tab_widget->currentIndex()==2){
			tab_widget->resize(470,tab_widget->height());
		}
		else {
			tab_widget->resize(455,tab_widget->height());
		}
	}
}
