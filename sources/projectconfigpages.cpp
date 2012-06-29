#include "projectconfigpages.h"
#include "qeticons.h"
#include "qetproject.h"
#include "borderpropertieswidget.h"
#include "conductorpropertieswidget.h"
#include "titleblockpropertieswidget.h"
#include <QtGui>

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
	return(tr("G\351n\351ral", "configuration page title"));
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
	project_ -> setTitle(title_value_ -> text());
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
	title_label_ = new QLabel(tr("Titre du projet\240:", "label when configuring"));
	title_value_ = new QLineEdit();
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
	main_layout0 -> addStretch();
	setLayout(main_layout0);
}

/**
	Read properties from the edited project then fill widgets with them.
*/
void ProjectMainConfigPage::readValuesFromProject() {
	title_value_ -> setText(project_ -> title());
}

/**
	Set the content of this page read only if the project is read only,
	editable if the project is editable.
 */
void ProjectMainConfigPage::adjustReadOnly() {
	bool is_read_only = project_ -> isReadOnly();
	title_value_ -> setReadOnly(is_read_only);
}

/**
	Constructor
	@param project Project this page is editing.
	@param parent Parent QWidget
*/
ProjectNewDiagramConfigPage::ProjectNewDiagramConfigPage(QETProject *project, QWidget *parent) :
	ProjectConfigPage(project, parent)
{
	init();
}

/**
	Destructor
*/
ProjectNewDiagramConfigPage::~ProjectNewDiagramConfigPage() {
}

/**
	@return the title for this page
*/
QString ProjectNewDiagramConfigPage::title() const {
	return(tr("Nouveau sch\351ma", "project configuration page title"));
}

/**
	@return the icon for this page
*/
QIcon ProjectNewDiagramConfigPage::icon() const {
	return(QET::Icons::NewDiagram);
}

/**
	Apply the configuration after user input
*/
void ProjectNewDiagramConfigPage::applyProjectConf() {
	project_ -> setDefaultBorderProperties(border_ -> borderProperties());
	project_ -> setDefaultTitleBlockProperties(titleblock_ -> titleBlockProperties());
	project_ -> setDefaultConductorProperties(conductor_ -> conductorProperties());
}

/**
	Initialize widgets displayed by the page.
*/
void ProjectNewDiagramConfigPage::initWidgets() {
	informative_label_ = new QLabel(
		tr(
			"Propri\351t\351s \340 utiliser lors de l'ajout d'un nouveau sch\351ma au projet :",
			"explicative label"
		)
	);
	border_ = new BorderPropertiesWidget(BorderProperties()); 
	titleblock_ = new TitleBlockPropertiesWidget(TitleBlockProperties(), true);
	conductor_ = new ConductorPropertiesWidget();
	conductor_ -> setContentsMargins(0, 0, 0, 0);
}

/**
	Initialize the layout of this page.
*/
void ProjectNewDiagramConfigPage::initLayout() {
	// put border properties above title block properties
	QVBoxLayout *vlayout2 = new QVBoxLayout();
	vlayout2 -> addWidget(border_);
	vlayout2 -> addWidget(titleblock_);
	vlayout2 -> setSpacing(5);
	
	// add conductor properties on the right
	QHBoxLayout *hlayout1 = new QHBoxLayout();
	hlayout1 -> addLayout(vlayout2);
	hlayout1 -> addWidget(conductor_);
	hlayout1 -> setAlignment(conductor_, Qt::AlignTop);
	
	// add the informative label above previous widgets
	QVBoxLayout *vlayout1 = new QVBoxLayout();
	vlayout1 -> addWidget(informative_label_);
	vlayout1 -> addLayout(hlayout1);
	vlayout1 -> addStretch();
	setLayout(vlayout1);
}

/**
	Read properties from the edited project then fill widgets with them.
*/
void ProjectNewDiagramConfigPage::readValuesFromProject() {
	border_ -> setEditedBorder(project_ -> defaultBorderProperties());
	conductor_ -> setConductorProperties(project_ -> defaultConductorProperties());
	titleblock_ -> setTitleBlockProperties(project_ -> defaultTitleBlockProperties());
}

/**
	Set the content of this page read only if the project is read only,
	editable if the project is editable.
 */
void ProjectNewDiagramConfigPage::adjustReadOnly() {
	bool is_read_only = project_ -> isReadOnly();
	border_ -> setReadOnly(is_read_only);
	titleblock_ -> setReadOnly(is_read_only);
	conductor_ -> setReadOnly(is_read_only);
}
