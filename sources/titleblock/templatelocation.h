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
#ifndef TITLEBLOCK_SLASH_TEMPLATE_LOCATION_H
#define TITLEBLOCK_SLASH_TEMPLATE_LOCATION_H
#include <QtCore>
class QETProject;

/**
	This class represents the location of a title block template.
*/
class TitleBlockTemplateLocation {
	// constructor, destructor
	public:
	TitleBlockTemplateLocation(QETProject * = 0, const QString & = QString());
	virtual ~TitleBlockTemplateLocation();
	
	// methods
	public:
	QETProject *project() const;
	void setProject(QETProject *);
	QString name() const;
	void setName(const QString &);
	bool isValid() const;
	
	// attributes
	private:
	QETProject *project_; ///< Parent project of the template, if any
	QString name_;        ///< Name of the template
};
#endif
