/*
	Copyright 2006-2021 The QElectroTech Team
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
#ifndef XMLELEMENTCOLLECTION_H
#define XMLELEMENTCOLLECTION_H

#include <QObject>
#include <QDomElement>
#include "elementslocation.h"

class QDomElement;
class QFile;
class QETProject;

/**
	@brief The XmlElementCollection class
	This class represent a collection of elements stored to xml
*/
class XmlElementCollection : public QObject
{
		Q_OBJECT

	public:
		XmlElementCollection (QETProject *project);
		XmlElementCollection (const QDomElement &dom_element,
				      QETProject *project);
		QDomElement root() const;
		QDomElement importCategory() const;
		QDomNodeList childs(const QDomElement &parent_element) const;
		QDomElement child(const QDomElement &parent_element,
				  const QString &child_name) const;
		QDomElement child(const QString &path) const;
		QList<QDomElement> directories(
				const QDomElement &parent_element) const;
		QStringList directoriesNames(
				const QDomElement &parent_element) const;
		QList<QDomElement> elements(
				const QDomElement &parent_element) const;
		QStringList elementsNames(
				const QDomElement &parent_element) const;
		QDomElement element(const QString &path) const;
		QDomElement directory(const QString &path) const;
		QString addElement (ElementsLocation &location);
		bool addElementDefinition (const QString &dir_path,
					   const QString &elmt_name,
					   const QDomElement &xml_definition);
		bool removeElement(const QString& path);
		ElementsLocation copy (ElementsLocation &source,
				       ElementsLocation &destination,
				       const QString& rename = QString(),
				       bool deep_copy = true);
		bool exist (const QString &path) const;
		bool createDir (const QString& path,
				const QString& name,
				const NamesList &name_list);
		bool removeDir (const QString& path);

		QList <ElementsLocation> elementsLocation (
				QDomElement dom_element = QDomElement(),
				bool childs = true) const;
		ElementsLocation domToLocation(QDomElement dom_element) const;

		void cleanUnusedElement();
		void cleanUnusedDirectory();

	private:
		ElementsLocation copyDirectory(
				ElementsLocation &source,
				ElementsLocation &destination,
				const QString& rename = QString(),
				bool deep_copy = true);
		ElementsLocation copyElement(ElementsLocation &source,
					     ElementsLocation &destination,
					     const QString& rename = QString());

	signals:
		/**
			@brief elementAdded
			This signal is emitted when a element is added to this collection
			@param collection_path : the path of element in this collection
		*/
		void elementAdded(QString collection_path);
		/**
			@brief elementChanged
			This signal is emitted when the definition of the element at path was changed
			@param collection_path : the path of this element in this collection
		*/
		void elementChanged (QString collection_path);
		/**
			@brief elementRemoved
			This signal is emitted when an element is removed from this collection
			@param collection_path : the path of the removed element in this collection
		*/
		void elementRemoved(QString collection_path);
		/**
			@brief directorieAdded
			This signal is emitted when a directory is added to this collection
			@param collection_path : the path of the new directory
		*/
		void directorieAdded(QString collection_path);
		/**
			@brief directoryRemoved
			This signal is emitted when a directory is removed from this collection
			@param collection_path : the path of the removed directory
		*/
		void directoryRemoved(QString collection_path);

	private:
		QDomDocument m_dom_document;
		QETProject *m_project = nullptr;
};

#endif // XMLELEMENTCOLLECTION_H
