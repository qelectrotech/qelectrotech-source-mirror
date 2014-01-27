/*
	Copyright 2006-2013 The QElectroTech Team
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

// make this class usable with QVariant
int TitleBlockTemplateLocation::MetaTypeId = qRegisterMetaType<TitleBlockTemplateLocation>("TitleBlockTemplateLocation");

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
	@param loc_str String describing the location of a title block template.
*/
TitleBlockTemplateLocation TitleBlockTemplateLocation::locationFromString(const QString &loc_str) {
	TitleBlockTemplateLocation loc;
	loc.fromString(loc_str);
	return(loc);
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
	@param loc_str String describing the location of a title block template.
*/
void TitleBlockTemplateLocation::fromString(const QString &loc_str) {
	collection_ = QETApp::titleBlockTemplatesCollection(QUrl(loc_str).scheme());
	
	QRegExp name_from_url("^[^:]*:\\/\\/(.*)$");
	if (name_from_url.exactMatch(loc_str)) {
		name_ = name_from_url.capturedTexts().at(1);
	} else {
		name_ = QString();
	}
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
	parentCollection() -> getTemplateXmlDescription
*/
QDomElement TitleBlockTemplateLocation::getTemplateXmlDescription() const {
	if (!collection_ || name_.isEmpty()) return(QDomElement());
	return(collection_ -> getTemplateXmlDescription(name_));
}

/**
	This is a convenience method equivalent to
	parentCollection() -> getTemplate(...).
*/
TitleBlockTemplate *TitleBlockTemplateLocation::getTemplate() const {
	if (!collection_ || name_.isEmpty()) return(0);
	return(collection_ -> getTemplate(name_));
}

/**
	This is a convenience method equivalent to
	parentCollection() -> isReadOnly(name())
*/
bool TitleBlockTemplateLocation::isReadOnly() const {
	if (!collection_) return(false);
	return(collection_ -> isReadOnly(name_));
}

/**
	@param location other location that should be compared to this one
	@return true if locations are equal, false otherwise
*/
bool TitleBlockTemplateLocation::operator==(const TitleBlockTemplateLocation &location) const {
	return(location.collection_ == collection_ && location.name_ == name_);
}

/**
	@param location A standard title block template location
	@return a hash identifying this location
*/
uint qHash(const TitleBlockTemplateLocation &location) {
	return(qHash(location.toString()));
}
