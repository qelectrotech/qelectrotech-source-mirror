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
#include "xmlelementscollection.h"
#include "xmlelementscategory.h"
#include "qetproject.h"
#include "qetapp.h"

/**
	Construit une collection vide
	@param parent Item parent
*/
XmlElementsCollection::XmlElementsCollection(ElementsCollectionItem *parent) :
	ElementsCollection(parent)
{
	protocol_ = "unknown";
	project_ = 0;
	// cree une categorie racine vide
	root = new XmlElementsCategory(0, this);
	connect(root, SIGNAL(written()), this, SLOT(componentWritten()));
}

/**
	Construit une collection a partir d'un element XML suppose la decrire
	@param xml_element Element XML decrivant la collection
	@param parent Item parent
*/
XmlElementsCollection::XmlElementsCollection(const QDomElement &xml_element, ElementsCollectionItem *parent) :
	ElementsCollection(parent)
{
	protocol_ = "unknown";
	project_ = 0;
	// cree sa categorie racine a partir de l'element XML
	root = new XmlElementsCategory(xml_element, 0, this);
	connect(root, SIGNAL(written()), this, SLOT(componentWritten()));
}

/**
	Destructeur
*/
XmlElementsCollection::~XmlElementsCollection() {
	deleteContent();
}

QString XmlElementsCollection::title() const {
	if (!title_.isEmpty()) return(title_);
	
	// if the title attribute is empty, we generate a suitable one using the
	// parent project
	QString final_title;
	if (project_) {
		QString project_title = project_ -> title();
		if (project_title.isEmpty()) {
			final_title = QString(
				tr(
					"Collection du projet sans titre (id %1)",
					"Elements collection title when the parent project has an empty title -- %1 is the project internal id"
				)
			);
			final_title = final_title.arg(QETApp::projectId(project_));
		} else {
			final_title = QString(
				tr(
					"Collection du projet \"%1\"",
					"Elements collection title when the project has a suitable title -- %1 is the project title"
				)
			);
			final_title = final_title.arg(project_title);
		}
	}
	return(final_title);
}

ElementsCategory *XmlElementsCollection::rootCategory() {
	return(root);
}

/**
	@return toujours false ; une collection XML n'est representee nul part sur
	le systeme de fichiers.
*/
bool XmlElementsCollection::hasFilePath() {
	return(false);
}

/**
	@return le chemin du repertoire representant cette collection
*/
QString XmlElementsCollection::filePath() {
	return(QString());
}

/**
	Ne fait rien - une collection XML n'est representee nul part sur le systeme
	de fichiers.
*/
void XmlElementsCollection::setFilePath(const QString &) {
}

void XmlElementsCollection::reload() {
	if (root) root -> reload();
}

/**
	@return toujours true
*/
bool XmlElementsCollection::exists() {
	return(true);
}

/**
	@return true si la collection est accessible en lecture
*/
bool XmlElementsCollection::isReadable() {
	// une collection XML n'a aucune raison de ne pas etre accessible en lecture
	return(true);
}

bool XmlElementsCollection::isWritable() {
	// une collection XML peut etre en lecture seule si le projet auquel elle
	// appartient l'est
	if (QETProject *parent_project = project()) {
		return(!parent_project -> isReadOnly());
	} else {
		return(true);
	}
}

bool XmlElementsCollection::write() {
	emit(written());
	return(true);
}

/**
	@return always false, since an XMl-based elements collection should never
	be cached.
*/
bool XmlElementsCollection::isCacheable() const {
	return(false);
}

QDomElement XmlElementsCollection::writeXml(QDomDocument &xml_doc) const {
	QDomElement collection_elmt = root -> writeXml(xml_doc);
	collection_elmt.setTagName("collection");
	xml_doc.appendChild(collection_elmt);
	return(collection_elmt);
}

void XmlElementsCollection::componentWritten() {
	write();
}

/**
	Supprime le contenu en memoire de cette collection
*/
void XmlElementsCollection::deleteContent() {
	delete root;
	root = 0;
}
