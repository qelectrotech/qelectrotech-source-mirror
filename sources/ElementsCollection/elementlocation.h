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
#ifndef ELEMENTLOCATION_H
#define ELEMENTLOCATION_H

#include <QString>
#include <QDomElement>
#include <QUuid>
#include <QIcon>

class QETProject;

/**
 * @brief The ElementLocation class
 * This class represent the location of an element or a directory in the file system
 * or an embedded collection of a project.
 * They also provide common things about an element, like the icon, uuid etc...
 */
class ElementLocation
{
    public:
        ElementLocation(QString path = QString());
        ElementLocation(QString path, QETProject *project);
        ~ElementLocation();

        bool setPath(QString path);
		bool isNull() const;
        void setProject(QETProject *project);
		bool isElement() const;
		bool isDirectory() const;

        QString collectionPath() const;
        QString fileSystemPath() const;

        QETProject *project() const;

		QDomElement xml();
        QUuid uuid();
		QIcon icon();
		QString name();
		QString fileName() const;

    private:
        QString m_collection_path;
        QString m_file_system_path;
        QETProject *m_project;
        QDomElement m_xml;
        QUuid m_uuid;
		QIcon m_icon;
};

#endif // ELEMENTLOCATION_H
