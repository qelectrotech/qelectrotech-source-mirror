/*
	Copyright 2006-2011 Xavier Guerrin
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

/**
	Constructor
	@param project Parent project of the title block template
	@param name Name of the title block template within its parent project or collection
*/
TitleBlockTemplateLocation::TitleBlockTemplateLocation(QETProject *project, const QString &name) :
	project_(project),
	name_(name)
{
}

/**
	Destructor
*/
TitleBlockTemplateLocation::~TitleBlockTemplateLocation() {
}

/**
	@return the parent project of the template, or 0 if none was defined
*/
QETProject *TitleBlockTemplateLocation::project() const {
	return(project_);
}

/**
	@param project The new parent project of the template, or 0 if none
	applies.
*/
void TitleBlockTemplateLocation::setProject(QETProject *project) {
	project_ = project;
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
	return(project_ && !name_.isEmpty());
}
