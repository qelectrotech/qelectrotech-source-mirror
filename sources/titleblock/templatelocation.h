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
#ifndef TITLEBLOCK_SLASH_TEMPLATE_LOCATION_H
#define TITLEBLOCK_SLASH_TEMPLATE_LOCATION_H
#include <QtCore>
#include <QDomElement>
class QETProject;
class TitleBlockTemplate;
class TitleBlockTemplatesCollection;

/**
	This class represents the location of a title block template.
*/
class TitleBlockTemplateLocation {
	// constructor, destructor
	public:
	TitleBlockTemplateLocation(const QString & = QString(), TitleBlockTemplatesCollection * = 0);
	virtual ~TitleBlockTemplateLocation();
	
	// static methods
	public:
	TitleBlockTemplateLocation locationFromString(const QString &);
	
	// methods
	public:
	TitleBlockTemplatesCollection *parentCollection() const;
	void setParentCollection(TitleBlockTemplatesCollection *);
	QString name() const;
	void setName(const QString &);
	bool isValid() const;
	void fromString(const QString &);
	QString toString() const;
	QETProject *parentProject() const;
	QString protocol() const;
	QDomElement getTemplateXmlDescription() const;
	TitleBlockTemplate *getTemplate() const;
	bool isReadOnly() const;
	bool operator==(const TitleBlockTemplateLocation &) const;
	
	// attributes
	private:
	TitleBlockTemplatesCollection *collection_; ///< Collection the template belongs to
	QString name_;                              ///< Name of the template
	
	public:
	static int MetaTypeId; ///< Id of the corresponding Qt meta type
};
Q_DECLARE_METATYPE(TitleBlockTemplateLocation)
uint qHash(const TitleBlockTemplateLocation &);
#endif
