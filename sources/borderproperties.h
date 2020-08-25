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
        void toSettings(QSettings &, const QString & = QString()) const override;
        void fromSettings(const QSettings &, const QString & = QString()) override;

		static BorderProperties defaultProperties();
	
		// attributes
		int columns_count;            ///< Columns count
		qreal columns_width;          ///< Columns width
		qreal columns_header_height;  ///< Column headers height
		bool display_columns;         ///< Whether to display column headers
	
		int rows_count;               ///< Rows count
		qreal rows_height;            ///< Rows height
		qreal rows_header_width;      ///< Row headers width
		bool display_rows;            ///< Whether to display row headers
};
#endif
