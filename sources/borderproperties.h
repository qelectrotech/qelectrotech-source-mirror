/*
	Copyright 2006-2012 Xavier Guerrin
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
	Cette classe est un conteneur pour les dimensions et proprietes d'affichage
	d'un schema : affichage, nombre et dimensions des colonnes et lignes, ...
*/
class BorderProperties {
	public:
	// constructeur, destructeur, operateurs
	BorderProperties();
	virtual ~BorderProperties();
	
	bool operator==(const BorderProperties &);
	bool operator!=(const BorderProperties &);
	
	void toXml(QDomElement &) const;
	void fromXml(QDomElement &);
	void toSettings(QSettings &, const QString & = QString()) const;
	void fromSettings(QSettings &, const QString & = QString());
	
	// attributs
	int columns_count;            ///< Nombre de colonnes
	qreal columns_width;          ///< Largeur des colonnes
	qreal columns_header_height;  ///< Hauteur des entetes des colonnes
	bool display_columns;         ///< Afficher ou non les entetes des colonnes
	
	int rows_count;               ///< Nombre de lignes
	qreal rows_height;            ///< Hauteur des lignes
	qreal rows_header_width;      ///< Largeur des entetes des lignes
	bool display_rows;            ///< Afficher ou non les entetes des lignes
};
#endif
