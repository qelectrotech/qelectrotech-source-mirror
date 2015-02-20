/*
	Copyright 2006-2015 The QElectroTech Team
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
/**
	This class is a container for dimensions and display properties of a
	diagram.
*/
class BorderProperties {
	public:
	// constructor, destructor, operators
	BorderProperties();
	virtual ~BorderProperties();
	
	bool operator==(const BorderProperties &);
	bool operator!=(const BorderProperties &);
	
	void toXml(QDomElement &) const;
	void fromXml(QDomElement &);
	void toSettings(QSettings &, const QString & = QString()) const;
	void fromSettings(QSettings &, const QString & = QString());

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
