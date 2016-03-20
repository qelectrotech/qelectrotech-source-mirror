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
#ifndef ELEMENTS_LOCATION_H
#define ELEMENTS_LOCATION_H

#include "nameslist.h"
#include <QString>
#include <QIcon>

class QETProject;
class XmlElementCollection;

/**
	Cette classe represente la localisation, l'emplacement d'un element ou
	d'une categorie, voire d'une collection... dans une collection. Elle
	encapsule un chemin virtuel.
*/
class ElementsLocation
{
	public:
		ElementsLocation();
		ElementsLocation(const QString &path, QETProject *project = nullptr);
		ElementsLocation(const ElementsLocation &);
		ElementsLocation(const QMimeData *data);
		virtual ~ElementsLocation();
		ElementsLocation &operator=(const ElementsLocation &);
		bool operator==(const ElementsLocation &) const;
		bool operator!=(const ElementsLocation &) const;
	
	public:
		QString baseName() const;

		QString collectionPath(bool protocol = true) const;
		QString projectCollectionPath() const;
		QString fileSystemPath() const;
		QString path() const;
		void setPath(const QString &path);
		bool addToPath(const QString &);

		ElementsLocation parent() const;
		QETProject *project() const;
		void setProject(QETProject *);
		bool isNull() const;
		QString toString() const;

		bool isElement() const;
		bool isDirectory() const;
		bool isFileSystem() const;
		bool isProject() const;
		bool exist() const;

		XmlElementCollection *projectCollection() const;
		NamesList nameList();

		QDomElement xml();
		QUuid uuid();
		QIcon icon();
		QString name();
		QString fileName() const;
	
	private:
		QString m_collection_path;
		QString m_file_system_path;
		QETProject *m_project = nullptr;
		QDomElement m_xml;
		QUuid m_uuid;
		QIcon m_icon;
	
	public:
		static int MetaTypeId; ///< Id of the corresponding Qt meta type
};
Q_DECLARE_METATYPE(ElementsLocation)
uint qHash(const ElementsLocation &);
#endif
