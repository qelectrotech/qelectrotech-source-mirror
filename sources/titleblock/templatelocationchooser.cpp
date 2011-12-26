#include "templatelocationchooser.h"
#include "qetapp.h"
#include "qetproject.h"

/**
	Constructor
	@param location Initial location displayed by the widget
	@param widget Parent QWidget
*/
TitleBlockTemplateLocationChooser::TitleBlockTemplateLocationChooser(
	const TitleBlockTemplateLocation &location,
	QWidget *parent
) :
	QWidget(parent)
{
	init();
	setLocation(location);
}

/**
	Destructor
*/
TitleBlockTemplateLocationChooser::~TitleBlockTemplateLocationChooser() {
}

/**
	@return the current location
*/
TitleBlockTemplateLocation TitleBlockTemplateLocationChooser::location() const {
	return(TitleBlockTemplateLocation(project(), name()));
}

/**
	@return the currently selected project
*/
QETProject *TitleBlockTemplateLocationChooser::project() const {
	uint project_id = projects_ -> itemData(projects_ -> currentIndex()).toUInt();
	return(QETApp::project(project_id));
}

/**
	@return the currently selected/entered name
*/
QString TitleBlockTemplateLocationChooser::name() const {
	int template_index = templates_ -> currentIndex();
	return(template_index ? templates_ -> currentText() : new_name_ -> text());
}

/**
	Set the location displayed by this widget
	@param location to be displayed by this widget
*/
void TitleBlockTemplateLocationChooser::setLocation(const TitleBlockTemplateLocation &location) {
	// we need a project id
	int project_id = QETApp::projectId(location.project());
	if (project_id == -1) return;
	
	// attempt to find this project id in our project combo box
	int project_index = projects_ -> findData(QVariant(static_cast<uint>(project_id)));
	if (project_index == -1) return;
	
	projects_ -> setCurrentIndex(project_index);
	
	if (!location.name().isEmpty()) {
		int template_index = templates_ -> findText(location.name());
		if (template_index != -1) {
			templates_ -> setCurrentIndex(template_index);
		} else {
			templates_ -> setCurrentIndex(0);
		}
	}
}

/**
	Initialize this widget.
	@param location Initial location displayed by the widget
*/
void TitleBlockTemplateLocationChooser::init() {
	projects_ = new QComboBox();
	templates_ = new QComboBox();
	new_name_ = new QLineEdit();
	
	QMap<uint, QETProject *> projects = QETApp::registeredProjects();
	foreach (uint project_id, projects.keys()) {
		projects_ -> addItem(projects[project_id] -> title(), project_id);
	}
	updateTemplates();
	connect(projects_, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTemplates()));
	
	QFormLayout *form_layout = new QFormLayout();
	form_layout -> addRow(tr("Projet parent",   "used in save as form"), projects_);
	form_layout -> addRow(tr("Modèle existant", "used in save as form"), templates_);
	form_layout -> addRow(tr("ou nouveau nom",  "used in save as form"), new_name_);
	setLayout(form_layout);
}

/**
	Update the templates list according to the selected project.
*/
void TitleBlockTemplateLocationChooser::updateTemplates() {
	int current_index = projects_ -> currentIndex();
	if (current_index == -1) return;
	
	uint project_id = projects_ -> itemData(current_index).toUInt();
	QETProject *project = QETApp::project(project_id);
	if (!project) return;
	
	templates_ -> clear();
	templates_ -> addItem(tr("Nouveau modèle (entrez son nom)", "combox box entry"), QVariant(false));
	
	QStringList available_templates = project -> embeddedTitleBlockTemplates();
	if (available_templates.count()) {
		templates_ -> insertSeparator(1);
		foreach (QString template_name, available_templates) {
			templates_ -> addItem(template_name, QVariant(true));
		}
	}
}
