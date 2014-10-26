/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef EXPORT_PROPERTIES_H
#define EXPORT_PROPERTIES_H
#include <QtCore>
#include "qet.h"

/**
	This class is a container for various options used when printing or
	exporting a diagram as an image file.
*/
class ExportProperties {
	// constructeur, destructeur
	public:
	ExportProperties();
	virtual ~ExportProperties();
	
	// methods
	public:
	void toSettings  (QSettings &, const QString & = QString()) const;
	void fromSettings(QSettings &, const QString & = QString());

	static ExportProperties defaultExportProperties ();
	static ExportProperties defaultPrintProperties  ();
	
	// attributes
	public:
	QDir destination_directory;     ///< Target directory for generated files
	QString format;                 ///< Image format of generated files
	bool draw_grid;                 ///< Whether to render the diagram grid
	bool draw_border;               ///< Whether to render the border (along with rows/columns headers)
	bool draw_titleblock;           ///< Whether to render the title block
	bool draw_terminals;            ///< Whether to render terminals
	bool draw_colored_conductors;   ///< Whether to render conductors colors
	QET::DiagramArea exported_area; ///< Area of diagrams to be rendered
};
#endif
