#include "templatelocationsaver.h"
#include "qetapp.h"
#include "qetproject.h"
#include "templatescollection.h"

/**
	Constructor
	@param location Initial location displayed by the widget
	@param widget Parent QWidget
*/
TitleBlockTemplateLocationSaver::TitleBlockTemplateLocationSaver(
	const TitleBlockTemplateLocation &location,
	QWidget *parent
) :
	TitleBlockTemplateLocationChooser(location, parent)
{
	init();
	setLocation(location);
}

/**
	Destructor
*/
TitleBlockTemplateLocationSaver::~TitleBlockTemplateLocationSaver() {
}

/**
	@return the currently selected/entered name
*/
QString TitleBlockTemplateLocationSaver::name() const {
	int template_index = templates_ -> currentIndex();
	return(template_index ? templates_ -> currentText() : new_name_ -> text());
}

/**
	Set the location displayed by this widget
	@param location to be displayed by this widget
*/
void TitleBlockTemplateLocationSaver::setLocation(const TitleBlockTemplateLocation &location) {
	// hack: if no suitable index was found, set it to 1, which is supposed to be the user collection
	int index = indexForCollection(location.parentCollection());
	if (index == -1 && collections_ -> count() > 1) index = 1;
	collections_ -> setCurrentIndex(index);
	
	if (!location.name().isEmpty()) {
		int template_index = templates_ -> findText(location.name());
		if (template_index != -1) {
			templates_ -> setCurrentIndex(template_index);
			return;
		}
	}
	templates_ -> setCurrentIndex(0);
}

/**
	Initialize this widget.
	@param location Initial location displayed by the widget
*/
void TitleBlockTemplateLocationSaver::init() {
	new_name_ = new QLineEdit();
	connect(templates_, SIGNAL(currentIndexChanged(int)), this, SLOT(updateNewName()));
	form_layout_ -> addRow(tr("ou nouveau nom",       "used in save as form"), new_name_);
	updateTemplates();
}

/**
	Update the templates list according to the selected collection.
*/
void TitleBlockTemplateLocationSaver::updateTemplates() {
	TitleBlockTemplatesCollection *current_collection = collection();
	if (!current_collection) return;
	
	TitleBlockTemplateLocationChooser::updateTemplates();
	templates_ -> insertItem(0, tr("Nouveau mod\350le (entrez son nom)", "combox box entry"), QVariant(false));
	templates_ -> insertSeparator(1);
	
	updateNewName();
}

/**
	Enable or diable the "new name" text field depending of the selected
	template.
*/
void TitleBlockTemplateLocationSaver::updateNewName() {
	int template_index = templates_ -> currentIndex();
	new_name_ -> setEnabled(!template_index);
}
