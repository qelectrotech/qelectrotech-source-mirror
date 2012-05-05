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
#ifndef ORIENTATION_SET_H
#define ORIENTATION_SET_H
#include "qet.h"
/**
	Cette classe represente un containeur d'orientations, c'est-a-dire une
	structure de donnees stockant une orientation par defaut, une orientation
	courante et l'autorisation ou non d'utiliser les quatre autres orientations
	: nord, est, sud, ouest.
	Elle possede des methodes pour definir les autorisations. Le changement
	d'autorisation peut echouer car il faut au moins une orientation autorisee.
	Le changement d'autorisation peut eventuellement modifier les orientations
	par defaut et courante.
*/
class OrientationSet {
	
	// constructeurs, destructeur
	public:
	OrientationSet();
	virtual ~OrientationSet() {};
	
	// attributs
	private:
	bool north_ori;
	bool east_ori;
	bool south_ori;
	bool west_ori;
	QET::Orientation default_ori;
	QET::Orientation current_ori;
	
	// methodes
	public:
	bool north() const;
	bool east() const;
	bool south() const;
	bool west() const;
	bool setNorth(bool);
	bool setEast(bool);
	bool setSouth(bool);
	bool setWest(bool);
	QET::Orientation defaultOrientation() const;
	void setDefaultOrientation(const QET::Orientation &);
	QET::Orientation current() const;
	bool setCurrent(QET::Orientation);
	QET::Orientation next() const;
	QET::Orientation previous() const;
	QET::Orientation setNext();
	QET::Orientation setPrevious();
	bool accept(QET::Orientation) const;
	const OrientationSet operator++(int);
	const OrientationSet operator--(int);
	const OrientationSet operator++();
	const OrientationSet operator--();
	bool operator==(const OrientationSet &) const;
	bool operator!=(const OrientationSet &) const;
	bool fromString(const QString &);
	QString toString() const;
};

/**
	@return true si l'orientation "Nord" est autorisee, false sinon
*/
inline bool OrientationSet::north() const {
	return(north_ori);
}

/**
	@return true si l'orientation "Est" est autorisee, false sinon
*/
inline bool OrientationSet::east() const {
	return(east_ori);
}

/**
	@return true si l'orientation "Sud" est autorisee, false sinon
*/
inline bool OrientationSet::south() const {
	return(south_ori);
}

/**
	@return true si l'orientation "Ouest" est autorisee, false sinon
*/
inline bool OrientationSet::west() const {
	return(west_ori);
}

/**
	@param theValue La nouvelle orientation par defaut
*/
inline void OrientationSet::setDefaultOrientation(const QET::Orientation& theValue) {
	default_ori = theValue;
}

/**
	@return L'orientation par defaut
*/
inline QET::Orientation OrientationSet::defaultOrientation() const {
	return(default_ori);
}

/**
	@return L'orientation actuelle
*/
inline QET::Orientation OrientationSet::current() const {
	return(current_ori);
}

#endif
