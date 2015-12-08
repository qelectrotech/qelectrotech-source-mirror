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
#include "elementlocation.h"
#include "qetapp.h"
#include "qetproject.h"
#include "elementscollectioncache.h"

/**
 * @brief ElementLocation::ElementLocation
 * @param path : path of item in file system
 */
ElementLocation::ElementLocation(QString path):
    m_project(nullptr)
{
    if (!path.isEmpty())
        setPath(path);
}

/**
 * @brief ElementLocation::ElementLocation
 * @param path : path of item embedded in @project
 * @param project : project
 */
ElementLocation::ElementLocation(QString path, QETProject *project) :
    m_project(project)
{
    if (!path.isEmpty())
        setPath(path);
}

ElementLocation::~ElementLocation()
{}

/**
 * @brief ElementLocation::setPath
 * Set the path of this item.
 * If the path is for a file collection, the path can be in file system or relative to the beginning
 * of the colection, in this case the path must start with (common:// or custom://).
 * @param path
 * @return true if the element pointed by path exist, else false
 */
bool ElementLocation::setPath(QString path)
{
    if (!path.endsWith(".elmt")) return false;

    QString tmp_path = path;

        //The path is in file system
    if (!m_project)
    {
            //Common collection
        if (path.startsWith("common://"))
        {
            tmp_path.remove("common://");
            QString p = QETApp::commonElementsDir() + tmp_path;
            QFile file(p);
            if (file.exists())
            {
                m_file_system_path = p;
                m_collection_path = path;
                return true;
            }

            return false;
        }

            //Custom collection
        if (path.startsWith("custom://"))
        {
            tmp_path.remove("custom://");
            QString p = QETApp::customElementsDir() + tmp_path;
            QFile file(p);
            if (file.exists())
            {
                m_file_system_path = p;
                m_collection_path = path;
                return true;
            }

            return false;
        }
            //In this case, the path is supposed to be relative to the file system.
        else
        {
            QFile file(path);
            if (file.exists())
            {
                m_file_system_path = path;
                if (path.startsWith(QETApp::commonElementsDir()))
                {
                    path.remove(QETApp::commonElementsDir());
                    path.prepend("common://");
                    m_collection_path = path;
                }
                else if (path.startsWith(QETApp::customElementsDir()))
                {
                    path.remove(QETApp::customElementsDir());
                    path.prepend("common://");
                    m_collection_path = path;
                }
                return true;
            }
            return false;
        }
    }

	return false;
}

bool ElementLocation::isNull() const
{
	if (m_file_system_path.isEmpty())
		return true;
	else
		return false;
}

/**
 * @brief ElementLocation::setProject
 * @param project : set the project of this location to @project.
 */
void ElementLocation::setProject(QETProject *project)
{
    m_project = project;
}

/**
 * @brief ElementLocation::collectionPath
 * @return the colletion relative to the collection
 */
QString ElementLocation::collectionPath() const
{
    return m_collection_path;
}

/**
 * @brief ElementLocation::fileSystemPath
 * @return The file system path of this element,
 * If this element is embedded in a project return an empty string;
 */
QString ElementLocation::fileSystemPath() const
{
    if (!m_project)
        return QDir::fromNativeSeparators(m_file_system_path);
    else
        return QString();
}

/**
 * @brief ElementLocation::project
 * @return the project of this location if he was set.
 */
QETProject *ElementLocation::project() const
{
    return m_project;
}

/**
 * @brief ElementLocation::xml
 * @return The definition of this element.
 * The definition can be null.
 */
QDomElement ElementLocation::xml()
{
    if (!m_xml.isNull())
        return m_xml;

    if (!m_project)
    {
        QFile file (m_file_system_path);
        QDomDocument docu;
        if (docu.setContent(&file))
            m_xml = docu.documentElement().cloneNode().toElement();
    }

    return m_xml;
}

/**
 * @brief ElementLocation::uuid
 * @return The uuid of the pointed element
 */
QUuid ElementLocation::uuid()
{
    if (!m_uuid.isNull()) return m_uuid;

        //Get the uuid of element
    QList<QDomElement>  list_ = QET::findInDomElement(xml(), "uuid");

    if (!list_.isEmpty())
        m_uuid = QUuid(list_.first().attribute("uuid"));
//    else
//        qDebug() << "The element : " << m_file_system_path << "haven't got an uuid, please edit and save this element with element editor to create an uuid";

	return m_uuid;
}

QIcon ElementLocation::icon()
{
	if (!m_icon.isNull()) return m_icon;

	ElementsCollectionCache *cache = QETApp::collectionCache();
	if (cache->fetchElement(*this))
		m_icon = QIcon(cache->pixmap());

	return m_icon;
}

QString ElementLocation::name()
{
	ElementsCollectionCache *cache = QETApp::collectionCache();
	if (cache->fetchElement(*this))
		return cache->name();
	else
		return QString();
}
