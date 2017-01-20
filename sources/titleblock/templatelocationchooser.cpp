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
#include "templatelocationchooser.h"
#include "qetapp.h"
#include "qetproject.h"
#include "templatescollection.h"

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
	return(TitleBlockTemplateLocation(name(), collection()));
}

/**
	@return the currently selected collection
*/
TitleBlockTemplatesCollection *TitleBlockTemplateLocationChooser::collection() const {
	return(collections_index_[collections_ -> currentIndex()]);
}

/**
	@return the currently selected/entered name
*/
QString TitleBlockTemplateLocationChooser::name() const {
	int template_index = templates_ -> currentIndex();
	return(template_index != -1 ? templates_ -> currentText() : QString());
}

/**
	Set the location displayed by this widget
	@param location to be displayed by this widget
*/
void TitleBlockTemplateLocationChooser::setLocation(const TitleBlockTemplateLocation &location) {
	int index = indexForCollection(location.parentCollection());
	collections_ -> setCurrentIndex(index);
	
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
	collections_ = new QComboBox();
	templates_ = new QComboBox();
	
	updateCollections();
	connect(collections_, SIGNAL(currentIndexChanged(int)), this, SLOT(updateTemplates()));
	
	form_layout_ = new QFormLayout();
	form_layout_ -> addRow(tr("Collection parente",   "used in save as form"), collections_);
	form_layout_ -> addRow(tr("Modèle existant",   "used in save as form"), templates_);
	setLayout(form_layout_);
}

/**
	@param coll A Title block templates collection which we want to know the index within the combo box of this dialog.
	@return -1 if the collection is unknown to this dialog, or the index of \a coll
*/
int TitleBlockTemplateLocationChooser::indexForCollection(TitleBlockTemplatesCollection *coll) const {
	QList<int> indexes = collections_index_.keys(coll);
	if (indexes.count()) return(indexes.first());
	return(-1);
}

/**
	Update the collections list
*/
void TitleBlockTemplateLocationChooser::updateCollections() {
	collections_ -> clear();
	collections_index_.clear();
	
	int index = 0;
	foreach(TitleBlockTemplatesCollection *collection, QETApp::availableTitleBlockTemplatesCollections()) {
		collections_ -> addItem(collection -> title());
		collections_index_.insert(index, collection);
		++ index;
	}
	
	updateTemplates();
}

/**
	Update the templates list according to the selected collection.
*/
void TitleBlockTemplateLocationChooser::updateTemplates() {
	TitleBlockTemplatesCollection *current_collection = collection();
	if (!current_collection) return;
	
	templates_ -> clear();
	
	QStringList available_templates = current_collection -> templates();
	if (available_templates.count()) {
		foreach (QString template_name, available_templates) {
			templates_ -> addItem(template_name, QVariant(true));
		}
	}
}
