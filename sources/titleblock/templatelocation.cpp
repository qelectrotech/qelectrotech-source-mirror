/*
	Copyright 2006-2012 Xavier Guerrin
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
#include "templatelocation.h"
#include "templatescollection.h"
#include "qetapp.h"

/**
	Constructor
	@param collection Parent collection of the title block template
	@param name Name of the title block template within its parent project or collection
*/
TitleBlockTemplateLocation::TitleBlockTemplateLocation(const QString &name, TitleBlockTemplatesCollection *collection) :
	collection_(collection),
	name_(name)
{
}

/**
	Destructor
*/
TitleBlockTemplateLocation::~TitleBlockTemplateLocation() {
}

/**
	@return the parent collection of the template, or 0 if none was defined
*/
TitleBlockTemplatesCollection *TitleBlockTemplateLocation::parentCollection() const {
	return(collection_);
}

/**
	@param project The new parent collection of the template, or 0 if none
	applies.
*/
void TitleBlockTemplateLocation::setParentCollection(TitleBlockTemplatesCollection *collection) {
	collection_ = collection;
}

/**
	@return the name of this template within its parent project or collection.
*/
QString TitleBlockTemplateLocation::name() const {
	return(name_);
}

/**
	@param name The new name of this template.
*/
void TitleBlockTemplateLocation::setName(const QString &name) {
	name_ = name;
}

/**
	@return true if this location is null, false otherwise
*/
bool TitleBlockTemplateLocation::isValid() const {
	return(!name_.isEmpty());
}

/**
	@return A string representation of the location
*/
QString TitleBlockTemplateLocation::toString() const {
	return(protocol() + QString("://") + name_);
}

/**
	This is a convenience method equivalent to
	parentCollection() -> parentProject().
*/
QETProject *TitleBlockTemplateLocation::parentProject() const {
	if (collection_) {
		return(collection_ -> parentProject());
	}
	return(0);
}

/**
	This is a convenience method equivalent to
	parentCollection() -> protocol().
*/
QString TitleBlockTemplateLocation::protocol() const {
	if (collection_) {
		return(collection_ -> protocol());
	}
	return("unknown");
}

/**
	This is a convenience method equivalent to
	parentCollection() -> getTemplate(...).
*/
TitleBlockTemplate *TitleBlockTemplateLocation::getTemplate() const {
	if (!collection_ || name_.isEmpty()) return(0);
	return(collection_ -> getTemplate(name_));
}
