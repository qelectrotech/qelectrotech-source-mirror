/*
	Copyright 2006-2012 Xavier Guerrin
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
	Cette classe represente les differentes options utilisees pour exporter un
	schema sous forme d'image, ou encore pour l'imprimer.
*/
class ExportProperties {
	// constructeur, destructeur
	public:
	ExportProperties();
	virtual ~ExportProperties();
	
	// methodes
	public:
	void toSettings  (QSettings &, const QString & = QString()) const;
	void fromSettings(QSettings &, const QString & = QString());
	
	// attributs
	public:
	QDir destination_directory;     ///< Quel est le repertoire de destination du ou des fichiers generes ?
	QString format;                 ///< Quel format d'image faut-il utiliser ?
	bool draw_grid;                 ///< Faut-il dessiner la grille ?
	bool draw_border;               ///< Faut-il dessiner le cadre ?
	bool draw_titleblock;                ///< Faut-il dessiner le cartouche ?
	bool draw_terminals;            ///< Faut-il dessiner les bornes ?
	bool draw_colored_conductors;   ///< Faut-il respecter les couleurs des conducteurs ?
	QET::DiagramArea exported_area; ///< Zone du schema a exporter
};
#endif
