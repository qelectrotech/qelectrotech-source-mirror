/*
	Copyright 2006-2009 Xavier Guerrin
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
	/// version de QElectroTech (utilisee pour estampiller les projets et elements)
	const QString version = "0.2";
	/// version affichee de QElectroTech
	const QString displayedVersion = "0.2";
	QString license();
	/// Orientation (utilise pour les bornes mais aussi pour les elements)
	enum Orientation {North, East, South, West};
	
	/// Mouvements orientes
	enum OrientedMovement {
		ToNorth,
		ToNorthEast,
		ToEast,
		ToSouthEast,
		ToSouth,
		ToSouthWest,
		ToWest,
		ToNorthWest
	};
	
	/// Types de segment de conducteurs
	enum ConductorSegmentType {
		Horizontal = 1, ///< Segment horizontal
		Vertical = 2,   ///< Segment vertical
		Both = 3        ///< Segment en biais / invalide
	};
	
	/**
		Cet enum represente les differents embouts possibles pour les
		extremites d'une ligne.
	*/
	enum EndType {
		None,      ///< Ligne normale
		Simple,    ///< Triangle sans base
		Triangle,  ///< Triangle
		Circle,    ///< Cercle
		Diamond    ///< Losange
	};
	
	/**
		Cet enum represente les differents items manipulables dans une
		collection d'elements.
	*/
	enum ItemType {
		Element    = 1, ///< Element
		Category   = 2, ///< Categorie
		Collection = 4, ///< Collection
		All        = 7  ///< Tous
	};
	
	
	/**
		Cet enum represente les differentes facons de gerer un probleme lors de
		la recopie ou du deplacement d'un element ou d'une categorie.
		@see MoveElementsHandler
	*/
	enum Action {
		Retry,   ///< il faut reessayer l'operation
		Ignore,  ///< il faut passer a la suite
		Erase,   ///< il faut ecraser le contenu cible
		Abort,   ///< il faut arreter : ignorer l'item en cours et ne pas continuer
		Managed, ///< le cas a ete gere par l'objet delegue : ne pas le traiter et passer a la suite
		Rename   ///< il faut renommer la cible
	};
	
	/**
		Cet enum represente la zone d'un schema a exporter / imprimer
	*/
	enum DiagramArea {
		BorderArea,      ///< Exporte le schema avec son cadre et son cartouche
		ElementsArea     ///< Exporte le contenu du schema sans le cadre et le cartouche
	};
	
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
	QString forbiddenCharactersString(bool = false);
	bool containsForbiddenCharacters(const QString &);
	QString stringToFileName(const QString &);
	QString escapeSpaces(const QString &);
	QString unescapeSpaces(const QString &);
	QString joinWithSpaces(const QStringList &);
	QStringList splitWithSpaces(const QString &);
	QString endTypeToString(const QET::EndType &);
	QET::EndType endTypeFromString(const QString &);
	QString diagramAreaToString(const QET::DiagramArea &);
	QET::DiagramArea diagramAreaFromString(const QString &);
	QString pointerString(void *);
}
#endif
