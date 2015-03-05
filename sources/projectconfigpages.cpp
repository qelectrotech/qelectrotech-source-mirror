/*
	Copyright 2006-2015 The QElectroTech Team
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
	title_information_ = new QLabel(tr("Ce titre sera disponible pour tous les schémas de ce projet en tant que %projecttitle.", "informative label"));
	project_variables_label_ = new QLabel(
		tr(
			"Vous pouvez définir ci-dessous des propriétés personnalisées qui seront disponibles pour tous les schémas de ce projet (typiquement pour les cartouches).",
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
	m_label = new QLabel(tr("Numérotations disponibles :", "availables numerotations"), this);

	m_context_cb = new QComboBox(this);
	m_context_cb->setEditable(true);
	m_context_cb->lineEdit()->setClearButtonEnabled(true);
	m_context_cb->addItem(tr("Nom de la nouvelle numérotation"));

	m_remove_pb = new QPushButton(QET::Icons::EditDelete, QString(), this);
	m_remove_pb -> setToolTip(tr("Supprimer la numérotation"));

	m_saw = new SelectAutonumW(this);
}

/**
 * @brief ProjectAutoNumConfigPage::initLayout
 * Init the layout of this page
 */
void ProjectAutoNumConfigPage::initLayout() {
	QHBoxLayout *context_layout = new QHBoxLayout();
	context_layout -> addWidget (m_label);
	context_layout -> addWidget (m_context_cb);
	context_layout -> addWidget (m_remove_pb);

	QVBoxLayout *main_layout = new QVBoxLayout(this);
	this        -> setLayout (main_layout);
	main_layout -> addLayout (context_layout);
	main_layout -> addWidget (m_saw);
}

/**
 * @brief ProjectAutoNumConfigPage::readValuesFromProject
 * Read value stored on project, and update display
 */
void ProjectAutoNumConfigPage::readValuesFromProject() {
	QList <QString> keys = project_->conductorAutoNum().keys();
	if (keys.isEmpty()) return;
	foreach (QString str, keys) { m_context_cb -> addItem(str); }
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
	connect (m_context_cb, SIGNAL (currentIndexChanged(QString)), this, SLOT (updateContext(QString)));
	connect (m_saw,        SIGNAL (applyPressed()),               this, SLOT (saveContext()));
	connect (m_remove_pb,  SIGNAL(clicked()),                     this, SLOT(removeContext()));
}

/**
 * @brief ProjectAutoNumConfigPage::updateContext
 * Display the current selected context
 * @param str, key of context stored in project
 */
void ProjectAutoNumConfigPage::updateContext(QString str) {
	if (str == tr("Nom de la nouvelle numérotation")) m_saw -> setContext(NumerotationContext());
	else m_saw ->setContext(project_->conductorAutoNum(str));
}

/**
 * @brief ProjectAutoNumConfigPage::saveContext
 * Save the current displayed context in project
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
 * @brief ProjectAutoNumConfigPage::removeContext
 * Remove from project the current numerotation context
 */
void ProjectAutoNumConfigPage::removeContext() {
	//if default text, return
	if ( m_context_cb -> currentText() == tr("Nom de la nouvelle numérotation") ) return;
	project_     -> removeConductorAutonum (m_context_cb -> currentText() );
	m_context_cb -> removeItem             (m_context_cb -> currentIndex() );
}
