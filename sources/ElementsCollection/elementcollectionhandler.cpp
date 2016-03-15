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
#include "elementcollectionhandler.h"
#include "renamedialog.h"
#include "xmlelementcollection.h"
#include "qetxml.h"
#include <QFile>
#include <QDir>

/******************************************************/

ECHStrategy::ECHStrategy(ElementsLocation &source, ElementsLocation &destination) :
    m_source(source),
    m_destination (destination)
{}

ECHStrategy::~ECHStrategy() {}

/******************************************************/

ECHSFileToFile::ECHSFileToFile(ElementsLocation &source, ElementsLocation &destination) :
    ECHStrategy(source, destination)
{}

ElementsLocation ECHSFileToFile::copy()
{
		//Check if the destination already have an item with the same name of the item to copy
	ElementsLocation location(m_destination.fileSystemPath() + "/" + m_source.fileName());
	QString rename;
	if (location.exist())
	{
		RenameDialog rd(location.fileSystemPath());
		if (rd.exec() == QDialog::Accepted)
		{
			if (rd.selectedAction() == QET::Erase)
			{
				if (location.isDirectory())
				{
					QDir dir(location.fileSystemPath());
					dir.removeRecursively();
				}
				else
				{
					QFile file(location.fileSystemPath());
					file.remove();
				}
			}
			else if (rd.selectedAction() == QET::Rename)
			{
				rename = rd.newName();
			}
		}
		else
			return ElementsLocation();
	}

	if (m_source.isElement())
		return copyElement(m_source, m_destination, rename);
    else
		return copyDirectory(m_source, m_destination, rename);
}

ElementsLocation ECHSFileToFile::copyDirectory(ElementsLocation &source, ElementsLocation &destination, QString rename)
{
    QDir source_dir(source.fileSystemPath());
    QDir destination_dir(destination.fileSystemPath());

	if (!source_dir.exists() || !destination_dir.exists()) return ElementsLocation();

    QString new_dir_name = rename.isEmpty() ? source_dir.dirName() : rename;

        //Create a new dir
    if (destination_dir.mkdir(new_dir_name))
    {
            //The new created directory
        QDir created_dir(destination_dir.canonicalPath() + "/" + new_dir_name);

            //Copy the qet_directory file
        QFile::copy(source_dir.canonicalPath() + "/qet_directory", created_dir.canonicalPath() + "/qet_directory");

            //Copy all dirs found in source_dir to destination_dir
		ElementsLocation created_location(created_dir.canonicalPath());
        foreach(QString str, source_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name))
        {
			ElementsLocation sub_source(source.fileSystemPath() + "/" + str);
            copyDirectory(sub_source, created_location);
        }

            //Copy all elements found in source_dir to destination_dir
        source_dir.setNameFilters(QStringList() << "*.elmt");
        foreach(QString str, source_dir.entryList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name))
        {
			ElementsLocation sub_source(source.fileSystemPath() + "/" + str);
            copyElement(sub_source, created_location);
        }

        return created_location;
    }

	return ElementsLocation();
}

ElementsLocation ECHSFileToFile::copyElement(ElementsLocation &source, ElementsLocation &destination, QString rename)
{
    QString new_elmt_name = rename.isEmpty() ? source.fileName() : rename;
    bool rb = QFile::copy(source.fileSystemPath(), destination.fileSystemPath() + "/" + new_elmt_name);
    if (rb)
		return ElementsLocation (destination.fileSystemPath() + "/" + new_elmt_name);
    else
		return ElementsLocation();
}

/******************************************************/

ECHSXmlToFile::ECHSXmlToFile(ElementsLocation &source, ElementsLocation &destination) :
	ECHStrategy(source, destination)
{}

ElementsLocation ECHSXmlToFile::copy()
{
		//Check if the destination already have an item with the same name of the item to copy
	ElementsLocation location(m_destination.fileSystemPath() + "/" + m_source.fileName());
	QString rename;
	if (location.exist())
	{
		RenameDialog rd(location.fileSystemPath());
		if (rd.exec() == QDialog::Accepted)
		{
			if (rd.selectedAction() == QET::Erase)
			{
				if (location.isDirectory())
				{
					QDir dir(location.fileSystemPath());
					dir.removeRecursively();
				}
				else
				{
					QFile file(location.fileSystemPath());
					file.remove();
				}
			}
			else if (rd.selectedAction() == QET::Rename)
			{
				rename = rd.newName();
			}
		}
		else
			return ElementsLocation();
	}

	if (m_source.isElement())
		return copyElement(m_source, m_destination, rename);
	else
		return copyDirectory(m_source, m_destination, rename);
}

ElementsLocation ECHSXmlToFile::copyDirectory(ElementsLocation &source, ElementsLocation &destination, QString rename)
{
	QDir destination_dir(destination.fileSystemPath());

	if (!(destination_dir.exists() && source.exist())) return ElementsLocation();

	QString new_dir_name = rename.isEmpty() ? source.fileName() : rename;

		//Create new dir
	if (destination_dir.mkdir(new_dir_name))
	{
		QDir created_dir(destination_dir.canonicalPath() + "/" + new_dir_name);
		ElementsLocation created_location(created_dir.canonicalPath());

			//Create the qet-directory file
		QDomDocument document;
		QDomElement root = document.createElement("qet-directory");
		document.appendChild(root);
		root.appendChild(source.nameList().toXml(document));

		QString filepath = created_dir.canonicalPath() + "/qet_directory";
		QET::writeXmlFile(document, filepath);

			//Create all directory found in source to created_dir
		XmlElementCollection *project_collection = source.projectCollection();

		QStringList directories_names = project_collection->directoriesNames( project_collection->directory(source.collectionPath(false)) );
		foreach(QString name, directories_names)
		{
			ElementsLocation sub_source_dir(source.projectCollectionPath() + "/" + name);
			copyDirectory(sub_source_dir,  created_location);
		}

			//Create all elements found in source to destination
		QStringList elements_names = project_collection->elementsNames( project_collection->directory(source.collectionPath(false))) ;
		foreach (QString name, elements_names)
		{
			ElementsLocation source_element(source.projectCollectionPath() + "/" + name);
			copyElement(source_element, created_location);
		}

		return created_location;
	}

	return ElementsLocation();
}

ElementsLocation ECHSXmlToFile::copyElement(ElementsLocation &source, ElementsLocation &destination, QString rename)
{	
	if (!(destination.exist() && source.exist())) return ElementsLocation();

	QString new_element_name = rename.isEmpty() ? source.fileName() : rename;

		//Get the xml descrption of the element
	QDomDocument document;
	document.appendChild(document.importNode(source.xml(), true));

		//Create the .elmt file
	QString filepath = destination.fileSystemPath() + "/" + new_element_name;
	if (QET::writeXmlFile(document, filepath))
		return ElementsLocation(filepath);
	else
		return ElementsLocation();
}

/******************************************************/

ECHSToXml::ECHSToXml(ElementsLocation &source, ElementsLocation &destination) :
	ECHStrategy(source, destination)
{}

ElementsLocation ECHSToXml::copy()
{
	if (!(m_source.exist() && m_destination.isDirectory() && m_destination.isProject())) return ElementsLocation();

		//Check if the destination already have an item with the same name of the item to copy
	ElementsLocation location(m_destination.projectCollectionPath() + "/" + m_source.fileName());

	QString rename;
	if (location.exist())
	{
		RenameDialog rd(location.collectionPath());
		if(rd.exec() == QDialog::Accepted)
		{
			if (rd.selectedAction() == QET::Rename)
				rename = rd.newName();
		}
		else
			return ElementsLocation();
	}

	return m_destination.projectCollection()->copy(m_source, m_destination, rename);
}

/******************************************************/

/**
 * @brief ElementCollectionHandler::ElementCollectionHandler
 * @param widget
 */
ElementCollectionHandler::ElementCollectionHandler() {}

ElementCollectionHandler::~ElementCollectionHandler()
{
    if (m_strategy) delete m_strategy;
}

/**
 * @brief ElementCollectionHandler::copy
 * Copy the content of collection represented by source to the collection represented by destination.
 * Destination must be a directory, else the copy do nothing and return a null ElementLocation
 * if destination have an item with the same name of source, a dialog ask to user what to do.
 * @param source
 * @param destination
 * @return
 */
ElementsLocation ElementCollectionHandler::copy(ElementsLocation &source, ElementsLocation &destination)
{
	if (!source.exist() || !destination.exist() || destination.isElement()) return ElementsLocation();

    if (source.isFileSystem() && destination.isFileSystem()) m_strategy = new ECHSFileToFile(source, destination);
	if (source.isProject() && destination.isFileSystem()) m_strategy = new ECHSXmlToFile(source, destination);
	else if (destination.isProject()) m_strategy = new ECHSToXml(source, destination);

	if (m_strategy)
		return m_strategy->copy();
	else
		return ElementsLocation();
}
