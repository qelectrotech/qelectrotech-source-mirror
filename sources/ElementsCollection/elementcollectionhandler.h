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
#ifndef ELEMENTCOLLECTIONHANDLER_H
#define ELEMENTCOLLECTIONHANDLER_H

#include "elementslocation.h"
#include "nameslist.h"

class QWidget;

/**
 * @brief The ECHStrategy class
 * Abstract class for manage copy of directory or element from a collection to another
 */
class ECHStrategy
{
    public:
		ECHStrategy(ElementsLocation &source, ElementsLocation &destination);
        virtual ~ECHStrategy();
		virtual ElementsLocation copy() =0;

		ElementsLocation m_source, m_destination;
};

/**
 * @brief The ECHSFileToFile class
 * Manage the copy of directory or element from a file system collection to another file system collection
 */
class ECHSFileToFile : public ECHStrategy
{
    public:
		ECHSFileToFile (ElementsLocation &source, ElementsLocation &destination);
		ElementsLocation copy();

    private:
		ElementsLocation copyDirectory(ElementsLocation &source, ElementsLocation &destination, QString rename = QString());
		ElementsLocation copyElement(ElementsLocation &source, ElementsLocation &destination, QString rename = QString());
};

/**
 * @brief The ECHSXmlToFile class
 * Manage the copy of a directory or element from an xml collection to a file.
 */
class ECHSXmlToFile : public ECHStrategy
{
	public:
		ECHSXmlToFile (ElementsLocation &source, ElementsLocation &destination);
		ElementsLocation copy();

	private:
		ElementsLocation copyDirectory(ElementsLocation &source, ElementsLocation &destination, QString rename = QString());
		ElementsLocation copyElement(ElementsLocation &source, ElementsLocation &destination, QString rename = QString());
};

/**
 * @brief The ECHSToXml class
 * Manage the copy of a directory or element from a collection (no matter if the source is a file system collection or an xml collection)
 * to an xml collection
 */
class ECHSToXml : public ECHStrategy
{
	public:
		ECHSToXml (ElementsLocation &source, ElementsLocation &destination);
		ElementsLocation copy();
};

/**
 * @brief The ElementCollectionHandler class
 * Provide several method to copy element or directory from a collection
 * to another collection.
 */
class ElementCollectionHandler
{
    public:
        ElementCollectionHandler();
        ~ElementCollectionHandler();

		ElementsLocation copy(ElementsLocation &source, ElementsLocation &destination);
		ElementsLocation createDir(ElementsLocation &parent, const QString &name, const NamesList &name_list);
		bool importFromProject (QETProject *project, ElementsLocation &location);
		bool setNames(ElementsLocation &location, const NamesList &name_list);

    private:
        ECHStrategy *m_strategy = nullptr;
};

#endif // ELEMENTCOLLECTIONHANDLER_H
