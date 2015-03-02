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
#include "exportproperties.h"
#include "qetapp.h"

#include <QStandardPaths>

/**
	Constructeur par defaut :
	  * le repertoire de destination est le Bureau de l'utilisateur
	  * le format d'export est PNG
	  * la grille et les bornes ne doivent pas etre dessinees
	  * la bordure et le cartouche doivent etre dessines
	  * la zone exportee est le schema avec son cadre et son cartouche
*/
ExportProperties::ExportProperties() :
	destination_directory(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)),
	format("PNG"),
	draw_grid(false),
	draw_border(true),
	draw_titleblock(true),
	draw_terminals(false),
	draw_colored_conductors(true),
	exported_area(QET::BorderArea)
{
}

/**
	Destructeur
*/
ExportProperties::~ExportProperties() {
}

/**
	Exporte les options dans une configuration.
	@param settings Parametres a ecrire
	@param prefix prefixe a ajouter devant les noms des parametres
*/
void ExportProperties::toSettings(QSettings &settings, const QString &prefix) const {
	settings.setValue(prefix + "path",                  QDir::toNativeSeparators(destination_directory.absolutePath()));
	settings.setValue(prefix + "format",                format);
	settings.setValue(prefix + "drawgrid",              draw_grid);
	settings.setValue(prefix + "drawborder",            draw_border);
	settings.setValue(prefix + "drawtitleblock",             draw_titleblock);
	settings.setValue(prefix + "drawterminals",         draw_terminals);
	settings.setValue(prefix + "drawcoloredconductors", draw_colored_conductors);
	settings.setValue(prefix + "area",                  QET::diagramAreaToString(exported_area));
}

/**
	Lit les options depuis une configuration.
	@param settings Parametres a lire
	@param prefix prefixe a ajouter devant les noms des parametres
*/
void ExportProperties::fromSettings(QSettings &settings, const QString &prefix) {
	QString desktop_path = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
	destination_directory.setPath(settings.value(prefix + "path", desktop_path).toString());
	if (!destination_directory.exists()) destination_directory.setPath(desktop_path);
	
	format = settings.value(prefix + "format").toString();
	
	draw_grid               = settings.value(prefix + "drawgrid",              false).toBool();
	draw_border             = settings.value(prefix + "drawborder",            true ).toBool();
	draw_titleblock         = settings.value(prefix + "drawtitleblock",             true ).toBool();
	draw_terminals          = settings.value(prefix + "drawterminals",         false).toBool();
	draw_colored_conductors = settings.value(prefix + "drawcoloredconductors", true ).toBool();
	
	exported_area  = QET::diagramAreaFromString(settings.value(prefix + "area", "border").toString());
}

/**
 * @brief ExportProperties::defaultProperties
 * @return the default properties stored in the setting file
 */
ExportProperties ExportProperties::defaultExportProperties() {
	QSettings &settings = QETApp::settings();

	ExportProperties def;
	def.fromSettings(settings, "export/default");

	return(def);
}

/**
 * @brief ExportProperties::defaultPrintProperties
 * @return the default properties stored in the setting file
 */
ExportProperties ExportProperties::defaultPrintProperties() {
	QSettings &settings = QETApp::settings();

	ExportProperties def;
	def.fromSettings(settings, "print/default");

	return(def);
}
