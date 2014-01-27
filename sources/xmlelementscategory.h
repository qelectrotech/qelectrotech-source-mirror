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
#ifndef XML_ELEMENTS_CATEGORY
#define XML_ELEMENTS_CATEGORY
#include <QtXml>
#include "elementscategory.h"
class XmlElementsCollection;
class XmlElementDefinition;
/**
	This class represents an elements category stored within an XML document
	(e.g. the embedded collection of a QET project).
*/
class XmlElementsCategory : public ElementsCategory {
	Q_OBJECT
	
	// constructors, destructor
	public:
	XmlElementsCategory(XmlElementsCategory * = 0, XmlElementsCollection * = 0);
	XmlElementsCategory(const QDomElement &, XmlElementsCategory * = 0, XmlElementsCollection * = 0);
	virtual ~XmlElementsCategory();
	
	private:
	XmlElementsCategory(const XmlElementsCategory &);
	
	// methods
	public:
	virtual QString pathName() const;
	virtual QString virtualPath();
	
	virtual QString filePath();
	virtual bool hasFilePath();
	virtual void setFilePath(const QString &);
	
	virtual QList<ElementsCategory *> categories();
	virtual ElementsCategory *category(const QString &);
	virtual ElementsCategory *createCategory(const QString &);
	
	virtual QList<ElementDefinition *> elements();
	virtual ElementDefinition *element(const QString &);
	virtual ElementDefinition *createElement(const QString &);
	
	virtual bool exists();
	virtual bool isReadable();
	virtual bool isWritable();
	
	virtual void reload();
	virtual bool remove();
	virtual bool removeContent();
	virtual bool write();
	
	virtual QDomElement writeXml(QDomDocument &) const;
	
	public slots:
	void componentWritten();
	void componentRemoved(const QString &path);
	
	signals:
	void written();
	void removed(const QString &);
	
	private:
	void deleteContent();
	void loadContent(const QDomElement &);
	
	// attributes
	protected:
	/// Parent collection
	XmlElementsCollection *xml_parent_collection_;
	/// Parent category
	XmlElementsCategory   *xml_parent_category_;
	/// Child categories
	QHash<QString, XmlElementsCategory  *> categories_;
	/// Child elements
	QHash<QString, XmlElementDefinition *> elements_;
	/// Nae of this category within the tree
	QString name_;
	/// XML description of this category
	QDomDocument xml_element_;
};
#endif
