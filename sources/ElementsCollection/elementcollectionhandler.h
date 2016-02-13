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
#ifndef ELEMENTCOLLECTIONHANDLER_H
#define ELEMENTCOLLECTIONHANDLER_H

#include "elementlocation.h"

class QWidget;

class ECHStrategy
{
    public:
        ECHStrategy(ElementLocation &source, ElementLocation &destination);
        virtual ~ECHStrategy();
        virtual ElementLocation copy() =0;

        ElementLocation m_source, m_destination;
};

class ECHSFileToFile : public ECHStrategy
{
    public:
        ECHSFileToFile (ElementLocation &source, ElementLocation &destination);

        ElementLocation copy();

    private:
        ElementLocation copyDirectory(ElementLocation &source, ElementLocation &destination, QString rename = QString());
        ElementLocation copyElement(ElementLocation &source, ElementLocation &destination, QString rename = QString());
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

        ElementLocation copy(ElementLocation &source, ElementLocation &destination);

    private:
        ECHStrategy *m_strategy = nullptr;
};

#endif // ELEMENTCOLLECTIONHANDLER_H
