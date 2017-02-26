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
#include "formulaautonumberingw.h"
#include "autonumberingmanagementw.h"
#include "ui_autonumberingmanagementw.h"

/**
	Constructor
	@param project Project this page is editing.
	@param parent Parent QWidget
*/
ProjectConfigPage::ProjectConfigPage(QETProject *project, QWidget *parent) :
	ConfigPage(parent),
	m_project(project)
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
	return(m_project);
}

/**
	Set \a new_project as the project being edited by this page.
	@param read_values True to read values from the project into widgets before setting them read only accordingly, false otherwise. Defaults to true.
	@return the former project
*/
QETProject *ProjectConfigPage::setProject(QETProject *new_project, bool read_values) {
	if (new_project == m_project) return(m_project);
	
	QETProject *former_project = m_project;
	m_project = new_project;
	if (m_project && read_values) {
		readValuesFromProject();
		adjustReadOnly();
	}
	return(former_project);
}

/**
	Apply the configuration after user input
*/
void ProjectConfigPage::applyConf() {
	if (!m_project || m_project -> isReadOnly()) return;
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
	if (m_project) {
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
	if (m_project -> title() != new_title) {
		m_project -> setTitle(new_title);
		modified_project = true;
	}
	
	DiagramContext new_properties = project_variables_ -> context();
	if (m_project -> projectProperties() != new_properties) {
		m_project -> setProjectProperties(new_properties);
		modified_project = true;
	}
	if (modified_project) {
		m_project -> setModified(true);
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
	this -> setMinimumWidth(780);

}

/**
	Read properties from the edited project then fill widgets with them.
*/
void ProjectMainConfigPage::readValuesFromProject() {
	title_value_ -> setText(m_project -> title());
	project_variables_ -> setContext(m_project -> projectProperties());
}

/**
	Set the content of this page read only if the project is read only,
	editable if the project is editable.
 */
void ProjectMainConfigPage::adjustReadOnly() {
	bool is_read_only = m_project -> isReadOnly();
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
void ProjectAutoNumConfigPage::initWidgets()
{
	QTabWidget *tab_widget = new QTabWidget(this);
	
		//Management tab
	m_amw = new AutoNumberingManagementW(project());
	tab_widget->addTab(m_amw, tr("Management"));
	
		//Conductor tab
	QWidget *conductor_widget = new QWidget();
	QVBoxLayout *conductor_layout = new QVBoxLayout(conductor_widget);
	QHBoxLayout *bp_conductor_layout = new QHBoxLayout();
	
	QLabel *label_conductor = new QLabel(tr("Numérotations disponibles :", "availables numerotations"));
	m_context_cb_conductor= new QComboBox();
	m_context_cb_conductor->setEditable(true);
	m_context_cb_conductor->lineEdit()->setClearButtonEnabled(true);
	m_context_cb_conductor->addItem(tr("Nom de la nouvelle numérotation"));
	
	m_remove_pb_conductor= new QPushButton(QET::Icons::EditDelete, QString());
	m_remove_pb_conductor-> setToolTip(tr("Supprimer la numérotation"));
	
	m_saw_conductor = new SelectAutonumW(1);
	
	bp_conductor_layout->addWidget(label_conductor);
	bp_conductor_layout->addStretch();
	bp_conductor_layout->addWidget(m_context_cb_conductor);
	bp_conductor_layout->addWidget(m_remove_pb_conductor);
	
	conductor_layout->addLayout(bp_conductor_layout);
	conductor_layout->addWidget(m_saw_conductor);
	
	tab_widget->addTab(conductor_widget, tr("Conducteur"));
	
		//Element tab
	QWidget *element_widget = new QWidget();
	QVBoxLayout *element_layout = new QVBoxLayout(element_widget);
	QHBoxLayout *bp_element_layout = new QHBoxLayout();
	
	QLabel *label_element = new QLabel(tr("Numérotations disponibles :", "availables numerotations"));
	
	m_context_cb_element = new QComboBox();
	m_context_cb_element->setEditable(true);
	m_context_cb_element->lineEdit()->setClearButtonEnabled(true);
	m_context_cb_element->addItem(tr("Nom de la nouvelle numérotation"));
	
	m_remove_pb_element = new QPushButton(QET::Icons::EditDelete, QString());
	m_remove_pb_element -> setToolTip(tr("Supprimer la numérotation"));
	
	m_saw_element = new SelectAutonumW(0);
	
	bp_element_layout->addWidget(label_element);
	bp_element_layout->addStretch();
	bp_element_layout->addWidget(m_context_cb_element);
	bp_element_layout->addWidget(m_remove_pb_element);
	
	element_layout->addLayout(bp_element_layout);
	element_layout->addWidget(m_saw_element);
	
	tab_widget->addTab(element_widget, tr("Element"));
	
		//Folio Tab
	QWidget *folio_widget = new QWidget();
	QVBoxLayout *folio_layout = new QVBoxLayout(folio_widget);
	QHBoxLayout *bp_folio_layout = new QHBoxLayout();
	
	QLabel *label_folio = new QLabel(tr("Numérotations disponibles :", "availables numerotations"));
	
	m_context_cb_folio = new QComboBox();
	m_context_cb_folio->setEditable(true);
	m_context_cb_folio->lineEdit()->setClearButtonEnabled(true);
	m_context_cb_folio->addItem(tr("Nom de la nouvelle numérotation"));
	
	m_remove_pb_folio = new QPushButton(QET::Icons::EditDelete, QString());
	m_remove_pb_folio -> setToolTip(tr("Supprimer la numérotation"));
	
	m_saw_folio = new SelectAutonumW(2);
	
	bp_folio_layout->addWidget(label_folio);
	bp_folio_layout->addStretch();
	bp_folio_layout->addWidget(m_context_cb_folio);
	bp_folio_layout->addWidget(m_remove_pb_folio);
	
	folio_layout->addLayout(bp_folio_layout);
	folio_layout->addWidget(m_saw_folio);
	
	tab_widget->addTab(folio_widget, tr("Folio"));
	
		//AutoNumbering Tab
	m_faw = new FolioAutonumberingW(project());
	tab_widget->addTab(m_faw, tr("Folio autonumérotation"));
	
	m_main_layout = new QHBoxLayout();
	m_main_layout->addWidget(tab_widget);
	setLayout(m_main_layout);
}

/**
 * @brief ProjectAutoNumConfigPage::readValuesFromProject
 * Read value stored on project, and update display
 */
void ProjectAutoNumConfigPage::readValuesFromProject() {
	//Conductor Tab
	QList <QString> keys_conductor = m_project->conductorAutoNum().keys();
	if (!keys_conductor.isEmpty()){
	foreach (QString str, keys_conductor) { m_context_cb_conductor-> addItem(str); }
	}

	//Element Tab
	QList <QString> keys_element = m_project->elementAutoNum().keys();
	if (!keys_element.isEmpty()){
	foreach (QString str, keys_element) { m_context_cb_element -> addItem(str);}
	}

	//Folio Tab
	QList <QString> keys_folio = m_project->folioAutoNum().keys();
	if (!keys_folio.isEmpty()){
	foreach (QString str, keys_folio) { m_context_cb_folio -> addItem(str);}
	}

	//Folio AutoNumbering Tab
	m_faw->setContext(keys_folio);
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
	
	//connect(m_tab_widget,SIGNAL(currentChanged(int)),this,SLOT(tabChanged(int)));

	//Management Tab
	connect (m_amw, SIGNAL(applyPressed()), this, SLOT(applyManagement()));

	//Conductor Tab
	connect (m_context_cb_conductor, SIGNAL (currentTextChanged(QString)),  m_saw_conductor, SLOT (applyEnableOnContextChanged(QString)));
	connect (m_context_cb_conductor, SIGNAL (currentIndexChanged(QString)), this, SLOT (updateContext_conductor(QString)));
	connect (m_saw_conductor,        SIGNAL (applyPressed()),               this, SLOT (saveContext_conductor()));
	connect (m_remove_pb_conductor,  SIGNAL (clicked()),                    this, SLOT (removeContext_conductor()));

		//Element Tab
	connect (m_context_cb_element, SIGNAL (currentTextChanged(QString)),  m_saw_element, SLOT(applyEnableOnContextChanged(QString)));
	connect (m_context_cb_element, SIGNAL (currentIndexChanged(QString)), this, SLOT (updateContextElement(QString)));
	connect (m_saw_element,        SIGNAL (applyPressed()),               this, SLOT (saveContextElement()));
	connect (m_remove_pb_element,  SIGNAL (clicked()),                    this, SLOT (removeContextElement()));

	//Folio Tab
	connect (m_context_cb_folio, SIGNAL (currentTextChanged(QString)),  m_saw_folio, SLOT(applyEnableOnContextChanged(QString)));
	connect (m_context_cb_folio, SIGNAL (currentIndexChanged(QString)), this, SLOT (updateContext_folio(QString)));
	connect (m_saw_folio,        SIGNAL (applyPressed()),               this, SLOT (saveContext_folio()));
	connect (m_remove_pb_folio,  SIGNAL (clicked()),                    this, SLOT (removeContext_folio()));

	//	Auto Folio Numbering
	connect (m_faw, SIGNAL (applyPressed()),				 this, SLOT (applyAutoNum()));
}

/**
 * @brief ProjectAutoNumConfigPage::updateContext_conductor
 * Display the current selected context for conductor
 * @param str, key of context stored in project
 */
void ProjectAutoNumConfigPage::updateContext_conductor(QString str) {
	if (str == tr("Nom de la nouvelle numérotation")) m_saw_conductor -> setContext(NumerotationContext());
	else m_saw_conductor ->setContext(m_project->conductorAutoNum(str));
}

/**
 * @brief ProjectAutoNumConfigPage::updateContext_folio
 * Display the current selected context for folio
 * @param str, key of context stored in project
 */
void ProjectAutoNumConfigPage::updateContext_folio(QString str) {
	if (str == tr("Nom de la nouvelle numérotation")) m_saw_folio -> setContext(NumerotationContext());
	else m_saw_folio ->setContext(m_project->folioAutoNum(str));
}

/**
 * @brief ProjectAutoNumConfigPage::updateContextElement
 * Display the current selected context for element
 * @param str, key of context stored in project
 */
void ProjectAutoNumConfigPage::updateContextElement(QString str)
{
	if (str == tr("Nom de la nouvelle numérotation"))
	{
		m_saw_element->setContext(NumerotationContext());
	}
	else
	{
		m_saw_element->setContext(m_project->elementAutoNum(str));
	}
}

/**
 * @brief ProjectAutoNumConfigPage::saveContextElement
 * Save the current displayed Element formula in project
 */
void ProjectAutoNumConfigPage::saveContextElement()
{
		// If the text is the default text "Name of new numerotation" save the edited context
		// With the the name "No name"
	if (m_context_cb_element->currentText() == tr("Nom de la nouvelle numérotation"))
	{
		QString title(tr("Sans nom"));

		m_project->addElementAutoNum (title, m_saw_element -> toNumContext());
		m_project->setCurrrentElementAutonum(title);
		m_context_cb_element->addItem(tr("Sans nom"));
	}
		// If the text isn't yet to the autonum of the project, add this new item to the combo box.
	else if ( !m_project -> elementAutoNum().keys().contains( m_context_cb_element->currentText()))
	{
		m_project->addElementAutoNum(m_context_cb_element->currentText(), m_saw_element->toNumContext());
		m_project->setCurrrentElementAutonum(m_context_cb_element->currentText());
		m_context_cb_element->addItem(m_context_cb_element->currentText());
	}
		// Else, the text already exist in the autonum of the project, just update the context
	else
	{
		m_project->addElementAutoNum (m_context_cb_element -> currentText(), m_saw_element -> toNumContext());
		m_project->setCurrrentElementAutonum(m_context_cb_element->currentText());
	}
}

/**
 * @brief ProjectAutoNumConfigPage::removeContextElement
 * Remove from project the current element numerotation context
 */
void ProjectAutoNumConfigPage::removeContextElement()
{
		//if default text, return
	if (m_context_cb_element->currentText() == tr("Nom de la nouvelle numérotation"))
		return;
	m_project->removeElementAutoNum (m_context_cb_element->currentText());
	m_context_cb_element->removeItem (m_context_cb_element->currentIndex());
}

/**
 * @brief ProjectAutoNumConfigPage::saveContext_conductor
 * Save the current displayed conductor context in project
 */
void ProjectAutoNumConfigPage::saveContext_conductor() {
	// If the text is the default text "Name of new numerotation" save the edited context
	// With the the name "No name"
	if (m_context_cb_conductor-> currentText() == tr("Nom de la nouvelle numérotation"))
	{
		m_project->addConductorAutoNum (tr("Sans nom"), m_saw_conductor -> toNumContext());
		project()->setCurrentConductorAutoNum(tr("Sans nom"));
		m_context_cb_conductor-> addItem(tr("Sans nom"));
	}
	// If the text isn't yet to the autonum of the project, add this new item to the combo box.
	else if ( !m_project -> conductorAutoNum().keys().contains( m_context_cb_conductor->currentText()))
	{
		project()->addConductorAutoNum(m_context_cb_conductor->currentText(), m_saw_conductor->toNumContext());
		project()->setCurrentConductorAutoNum(m_context_cb_conductor->currentText());
		m_context_cb_conductor-> addItem(m_context_cb_conductor->currentText());
	}
	// Else, the text already exist in the autonum of the project, just update the context
	else
	{
		project()->setCurrentConductorAutoNum(m_context_cb_conductor->currentText());
		m_project->addConductorAutoNum (m_context_cb_conductor-> currentText(), m_saw_conductor -> toNumContext());
	}
	project()->conductorAutoNumAdded();
}

/**
 * @brief ProjectAutoNumConfigPage::saveContext_folio
 * Save the current displayed folio context in project
 */
void ProjectAutoNumConfigPage::saveContext_folio() {
	// If the text is the default text "Name of new numerotation" save the edited context
	// With the the name "No name"
	if (m_context_cb_folio -> currentText() == tr("Nom de la nouvelle numérotation")) {
		m_project->addFolioAutoNum (tr("Sans nom"), m_saw_folio -> toNumContext());
		m_context_cb_folio -> addItem(tr("Sans nom"));
	}
	// If the text isn't yet to the autonum of the project, add this new item to the combo box.
	else if ( !m_project -> folioAutoNum().keys().contains( m_context_cb_folio->currentText())) {
		project()->addFolioAutoNum(m_context_cb_folio->currentText(), m_saw_folio->toNumContext());
		m_context_cb_folio -> addItem(m_context_cb_folio->currentText());
	}
	// Else, the text already exist in the autonum of the project, just update the context
	else {
		m_project->addFolioAutoNum (m_context_cb_folio -> currentText(), m_saw_folio -> toNumContext());
	}
	project()->folioAutoNumAdded();
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
			project()->autoFolioNumberingNewFolios();
			foliosRemaining = foliosRemaining-1;
		}
		emit (loadSavedTbp());
	}
	else{
		QString autoNum   = m_faw->autoNumSelected();
		int fromFolio = m_faw->fromFolio();
		int toFolio   = m_faw->toFolio();
		m_project->autoFolioNumberingSelectedFolios(fromFolio,toFolio,autoNum);
	}
}

/**
 * @brief ProjectAutoNumConfigPage::applyAutoManagement
 * Apply Management Options in Selected Folios
 */
void ProjectAutoNumConfigPage::applyManagement() {
	int from;
	int to;
	//Apply to Entire Project
	if (m_amw->ui->m_apply_project_rb->isChecked()) {
		from = 0;
		to = project()->diagrams().size() - 1;
	}
	//Apply to selected Folios
	else {
		from = m_amw->ui->m_from_folios_cb->itemData(m_amw->ui->m_from_folios_cb->currentIndex()).toInt();
		to = m_amw->ui->m_to_folios_cb->itemData(m_amw->ui->m_to_folios_cb->currentIndex()).toInt();
	}

	//Conductor Autonumbering Update Policy
	//Allow Both Existent and New Conductors
	if (m_amw->ui->m_both_conductor_rb->isChecked()) {
		//Unfreeze Existent and New Conductors
		project()->freezeExistentConductorLabel(false, from,to);
		project()->freezeNewConductorLabel(false, from,to);
		project()->setFreezeNewConductors(false);
	}
	//Allow Only New
	else if (m_amw->ui->m_new_conductor_rb->isChecked()) {
		//Freeze Existent and Unfreeze New Conductors
		project()->freezeExistentConductorLabel(true, from,to);
		project()->freezeNewConductorLabel(false, from,to);
		project()->setFreezeNewConductors(false);
	}
	//Allow Only Existent
	else if (m_amw->ui->m_existent_conductor_rb->isChecked()) {
		//Freeze Existent and Unfreeze New Conductors
		project()->freezeExistentConductorLabel(false, from,to);
		project()->freezeNewConductorLabel(true, from,to);
		project()->setFreezeNewConductors(true);
	}
	//Disable
	else if (m_amw->ui->m_disable_conductor_rb->isChecked()) {
		//Freeze Existent and New Elements, Set Freeze Element Project Wide
		project()->freezeExistentConductorLabel(true, from,to);
		project()->freezeNewConductorLabel(true, from,to);
		project()->setFreezeNewConductors(true);
	}

	//Element Autonumbering Update Policy
	//Allow Both Existent and New Elements
	if (m_amw->ui->m_both_element_rb->isChecked()) {
		//Unfreeze Existent and New Elements
		project()->freezeExistentElementLabel(false, from,to);
		project()->freezeNewElementLabel(false, from,to);
		project()->setFreezeNewElements(false);
	}
	//Allow Only New
	else if (m_amw->ui->m_new_element_rb->isChecked()) {
		//Freeze Existent and Unfreeze New Elements
		project()->freezeExistentElementLabel(true, from,to);
		project()->freezeNewElementLabel(false, from,to);
		project()->setFreezeNewElements(false);
	}
	//Allow Only Existent
	else if (m_amw->ui->m_existent_element_rb->isChecked()) {
		//Freeze New and Unfreeze Existent Elements, Set Freeze Element Project Wide
		project()->freezeExistentElementLabel(false, from,to);
		project()->freezeNewElementLabel(true, from,to);
		project()->setFreezeNewElements(true);
	}
	//Disable
	else if (m_amw->ui->m_disable_element_rb->isChecked()) {
		//Freeze Existent and New Elements, Set Freeze Element Project Wide
		project()->freezeExistentElementLabel(true, from,to);
		project()->freezeNewElementLabel(true, from,to);
		project()->setFreezeNewElements(true);
	}

	//Folio Autonumbering Status
	if (m_amw->ui->m_both_folio_rb->isChecked()) {

	}
	else if (m_amw->ui->m_new_folio_rb->isChecked()) {

	}
	else if (m_amw->ui->m_existent_folio_rb->isChecked()) {

	}
	else if (m_amw->ui->m_disable_folio_rb->isChecked()) {

	}

}

/**
 * @brief ProjectAutoNumConfigPage::removeContext
 * Remove from project the current conductor numerotation context
 */
void ProjectAutoNumConfigPage::removeContext_conductor() {
	//if default text, return
	if ( m_context_cb_conductor-> currentText() == tr("Nom de la nouvelle numérotation") ) return;
	m_project -> removeConductorAutoNum (m_context_cb_conductor-> currentText() );
	m_context_cb_conductor-> removeItem (m_context_cb_conductor-> currentIndex() );
	project()->conductorAutoNumRemoved();
}

/**
 * @brief ProjectAutoNumConfigPage::removeContext_folio
 * Remove from project the current folio numerotation context
 */
void ProjectAutoNumConfigPage::removeContext_folio() {
	//if default text, return
	if ( m_context_cb_folio -> currentText() == tr("Nom de la nouvelle numérotation") ) return;
	m_project -> removeFolioAutoNum (m_context_cb_folio -> currentText() );
	m_context_cb_folio -> removeItem (m_context_cb_folio -> currentIndex() );
	project()->folioAutoNumRemoved();
}

/**
 * @brief ProjectAutoNumConfigPage::changeToTab
 * @param tab index
 * Change to Selected Tab
 */
void ProjectAutoNumConfigPage::changeToTab(int i)
{
	Q_UNUSED(i);
}
