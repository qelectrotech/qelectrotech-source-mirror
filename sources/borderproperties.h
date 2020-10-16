/*
    Copyright 2006-2020 The QElectroTech Team
    This file is part of QElectroTech.
    
    QElectroTech is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.
    
    QElectroTech is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef BORDER_PROPERTIES_H
#define BORDER_PROPERTIES_H
#include <QtCore>
#include <QtXml>

#include "propertiesinterface.h"

/**
    @brief The BorderProperties class
    This class is a container for dimensions and display properties of a
    diagram.
    @remark Attributes are public
*/
class BorderProperties : public PropertiesInterface {
    public:
        // constructor, destructor, operators
        BorderProperties();
        virtual ~BorderProperties();
    
        bool operator==(const BorderProperties &);
        bool operator!=(const BorderProperties &);
    
        QDomElement toXml(QDomDocument &dom_doc) const override;
        bool fromXml(const QDomElement &) override;
        static bool valideXml(QDomElement& e);
        void toSettings(QSettings &, const QString & = QString()) const;
        void fromSettings(QSettings &, const QString & = QString());

        static BorderProperties defaultProperties();
    
        // attributes
        int columns_count{17};            ///< Columns count
        qreal columns_width{60.0};          ///< Columns width
        qreal columns_header_height{20.0};  ///< Column headers height
        bool display_columns{true};         ///< Whether to display column headers
    
        int rows_count{8};               ///< Rows count
        qreal rows_height{80.0};            ///< Rows height
        qreal rows_header_width{20.0};      ///< Row headers width
        bool display_rows{true};            ///< Whether to display row headers
};
#endif
