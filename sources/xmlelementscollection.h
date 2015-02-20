/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef XML_ELEMENTS_COLLECTION
#define XML_ELEMENTS_COLLECTION
#include <QtXml>
#include "elementscollection.h"
class XmlElementsCategory;
/**
	This class represents an elements collection stored within an XML document
	(e.g. the embedded collection of a QET project).
*/
class XmlElementsCollection : public ElementsCollection {
	Q_OBJECT
	public:
	// constructors, destructor
	XmlElementsCollection(ElementsCollectionItem * = 0);
	XmlElementsCollection(const QDomElement &, ElementsCollectionItem * = 0);
	virtual ~XmlElementsCollection();
	
	private:
	XmlElementsCollection(const XmlElementsCollection &);
	
	// methods
	public:
	virtual QString title() const;
	virtual ElementsCategory *rootCategory();
	virtual bool hasFilePath();
	virtual QString filePath();
	virtual void setFilePath(const QString &);
	virtual void reload();
	virtual bool exists();
	virtual bool isReadable();
	virtual bool isWritable();
	virtual bool write();
	virtual bool isCacheable() const;
	
	virtual QDomElement writeXml(QDomDocument &) const;
	
	public slots:
	void componentWritten();
	
	signals:
	void written();
	
	private:
	void deleteContent();
	
	// attributes
	private:
	XmlElementsCategory *root;
};
#endif
