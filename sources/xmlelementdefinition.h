/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef XML_ELEMENT_DEFINITION
#define XML_ELEMENT_DEFINITION
#include <QtXml>
#include "elementdefinition.h"
class XmlElementsCategory;
class XmlElementsCollection;
/**
	This class represents an element definition stored within an XML document
	(e.g. the embedded collection of a QET project).
*/
class XmlElementDefinition : public ElementDefinition {
	Q_OBJECT
	
	public:
	XmlElementDefinition(const QString &, XmlElementsCategory * = 0, XmlElementsCollection * = 0);
	XmlElementDefinition(const QDomElement &, XmlElementsCategory * = 0, XmlElementsCollection * = 0);
	virtual ~XmlElementDefinition();
	
	private:
	XmlElementDefinition(const XmlElementDefinition &);
	
	// methods
	public:
	virtual QDomElement xml();
	virtual bool setXml(const QDomElement &);
	virtual bool write();
	virtual bool isNull() const;
	virtual QString pathName() const;
	virtual QString virtualPath();
	virtual void reload();
	virtual bool exists();
	virtual bool isReadable();
	virtual bool isWritable();
	virtual bool remove();
	virtual bool hasFilePath();
	virtual QString filePath();
	virtual void setFilePath(const QString &);
	virtual QDateTime modificationTime() const;
	virtual QDomElement writeXml(QDomDocument &) const;
	
	signals:
	void written();
	void removed(const QString &);
	
	// attributes
	private:
	bool is_null_;
	QString name_;
	XmlElementsCategory *parent_category_;
	QDomDocument xml_element_;
	QDomElement element_definition_;
};
#endif
