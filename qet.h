/*
	Copyright 2006-2007 Xavier Guerrin
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
#ifndef _QET_H
#define _QET_H
#include <QtXml>
/**
	Ce fichier contient des fonctions utiles pouvant etre appelees depuis
	n'importe ou. Il contient egalement des enums utilises dans plusieurs
	classes de l'application
*/
namespace QET {
	/// version de QElectroTech
	const QString version = "0.1";
	QString license();
	/// Orientation (utilise pour les bornes mais aussi pour les elements)
	enum Orientation {North, East, South, West};
	/// Types de segment de conducteurs
	enum ConductorSegmentType { Horizontal = 1, Vertical = 2, Both = 3 };
	QET::Orientation nextOrientation(QET::Orientation);
	QET::Orientation previousOrientation(QET::Orientation);
	QET::Orientation orientationFromString(const QString &);
	QString orientationToString(QET::Orientation);
	bool surLeMemeAxe(QET::Orientation, QET::Orientation);
	bool estHorizontale(QET::Orientation);
	bool estVerticale(QET::Orientation);
	bool attributeIsAnInteger(const QDomElement &, QString , int * = NULL);
	bool attributeIsAReal(const QDomElement &, QString , double * = NULL);
	QString ElementsAndConductorsSentence(int, int, int = 0);
	QList<QDomElement> findInDomElement(const QDomElement &, const QString &, const QString &);
	QList<QChar> forbiddenCharacters();
	bool containsForbiddenCharacters(const QString &);
}
#endif
