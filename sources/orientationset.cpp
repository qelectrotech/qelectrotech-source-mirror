/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "orientationset.h"

/**
	Constructeur
	Par defaut, toutes les orientations sont autorisees. L'orientation courante
	vaut l'orientation par defaut, c'est-a-dire Nord.
*/
OrientationSet::OrientationSet() :
	north_ori(true),
	east_ori(true),
	south_ori(true),
	west_ori(true),
	default_ori(Qet::North),
	current_ori(Qet::North)
{}

/**
	@param ori true pour autoriser l'orientation vers le Nord, false pour l'interdire
	@return true si le changement d'autorisation a reussi, false sinon
*/
bool OrientationSet::setNorth (bool ori) {
	// pour desactiver une orientation, il doit y avoir au moins une autre orientation possible
	bool can_set_ori = ori ? true : east_ori || south_ori || west_ori;
	if (can_set_ori) {
		north_ori = ori;
		// en cas de desactivation d'une orientation, il faut verifier voire corriger les orientations courante et par defaut 
		if (!ori) {
			if (default_ori == Qet::North) default_ori = next();
			if (current_ori == Qet::North) current_ori = next();
		}
	}
	return(can_set_ori);
}

/**
	@param ori true pour autoriser l'orientation vers l'Est, false pour l'interdire
	@return true si le changement d'autorisation a reussi, false sinon
*/
bool OrientationSet::setEast (bool ori) {
	// pour desactiver une orientation, il doit y avoir au moins une autre orientation possible
	bool can_set_ori = ori ? true : south_ori || west_ori || north_ori;
	if (can_set_ori) {
		east_ori = ori;
		// en cas de desactivation d'une orientation, il faut verifier voire corriger les orientations courante et par defaut 
		if (!ori) {
			if (default_ori == Qet::East) default_ori = next();
			if (current_ori == Qet::East) current_ori = next();
		}
	}
	return(can_set_ori);
}

/**
	@param ori true pour autoriser l'orientation vers le Sud, false pour l'interdire
	@return true si le changement d'autorisation a reussi, false sinon
*/
bool OrientationSet::setSouth (bool ori) {
	// pour desactiver une orientation, il doit y avoir au moins une autre orientation possible
	bool can_set_ori = ori ? true : west_ori || north_ori || east_ori;
		if (can_set_ori) {
		south_ori = ori;
		// en cas de desactivation d'une orientation, il faut verifier voire corriger les orientations courante et par defaut 
		if (!ori) {
			if (default_ori == Qet::South) default_ori = next();
			if (current_ori == Qet::South) current_ori = next();
		}
	}
	return(can_set_ori);
}

/**
	@param ori true pour autoriser l'orientation vers l'Ouest, false pour l'interdire
	@return true si le changement d'autorisation a reussi, false sinon
*/
bool OrientationSet::setWest (bool ori) {
	// pour desactiver une orientation, il doit y avoir au moins une autre orientation possible
	bool can_set_ori = ori ? true : north_ori || east_ori || south_ori;
	if (can_set_ori) {
		west_ori = ori;
		// en cas de desactivation d'une orientation, il faut verifier voire corriger les orientations courante et par defaut 
		if (!ori) {
			if (default_ori == Qet::West) default_ori = next();
			if (current_ori == Qet::West) current_ori = next();
		}
	}
	return(can_set_ori);
}

/**
	Definit l'orientation courante
	@param ori nouvelle orientation courante
	@return true si le changement d'orientation a reussi, false sinon
*/
bool OrientationSet::setCurrent(Qet::Orientation ori) {
	bool can_set_ori = accept(ori);
	if (can_set_ori) current_ori = ori;
	return(can_set_ori);
}

/**
	@return l'orientation suivant l'orientation courante
*/
Qet::Orientation OrientationSet::next() const {
	Qet::Orientation result = current_ori;
	do result = Qet::nextOrientation(result); while (!accept(result));
	return(result);
}

/**
	@return l'orientation precedant l'orientation courante
*/
Qet::Orientation OrientationSet::previous() const {
	Qet::Orientation result = current_ori;
	do result = Qet::previousOrientation(result); while (!accept(result));
	return(result);
}

/**
	Equivaut a setNext()
	@return l'OrientationSet precedent
*/
const OrientationSet OrientationSet::operator++(int) {
	OrientationSet before(*this);
	setNext();
	return(before);
}

/**
	Equivaut a setPrevious()
	@return l'OrientationSet precedent
*/
const OrientationSet OrientationSet::operator--(int) {
	OrientationSet before(*this);
	setPrevious();
	return(before);
}

/**
	Permet de savoir si une orientation donnee peut etre utilisee.
	@param ori L'orientation en question
	@return true si l'orientation est utilisable, false sinon
*/
bool OrientationSet::accept(Qet::Orientation ori) const {
	bool accepted_ori = false;
	switch(ori) {
		case Qet::North: accepted_ori = north_ori; break;
		case Qet::East : accepted_ori = east_ori;  break;
		case Qet::South: accepted_ori = south_ori; break;
		case Qet::West : accepted_ori = west_ori;  break;
	}
	return(accepted_ori);
}

/**
	Definit l'orientation suivante comme etant l'orientation courante
	@return la nouvelle orientation courante
*/
Qet::Orientation OrientationSet::setNext() {
	setCurrent(next());
	return(current_ori);
}

/**
	Definit l'orientation precedente comme etant l'orientation courante
	@return la nouvelle orientation courante
*/
Qet::Orientation OrientationSet::setPrevious() {
	setCurrent(previous());
	return(current_ori);
}

/**
	Equivaut a setNext()
	@return l'OrientationSet courant
*/
const OrientationSet OrientationSet::operator++() {
	setNext();
	return(*this);
}

/**
	Equivaut a setPrevious()
	@return l'OrientationSet courant
*/
const OrientationSet OrientationSet::operator--() {
	setPrevious();
	return(*this);
}

/**
	@param os autre OrientationSet
	@return true si os et cet OrientationSet sont identiques, false sinon
*/
bool OrientationSet::operator==(const OrientationSet &os) const {
	if (north_ori   != os.north_ori)   return(false);
	if (east_ori    != os.east_ori)    return(false);
	if (south_ori   != os.south_ori)   return(false);
	if (west_ori    != os.west_ori)    return(false);
	if (default_ori != os.default_ori) return(false);
	if (current_ori != os.current_ori) return(false);
	return(true);
}

/**
	@param os autre OrientationSet
	@return false si os et cet OrientationSet sont identiques, true sinon
*/
bool OrientationSet::operator!=(const OrientationSet &os) const {
	return(!(this -> operator==(os)));
}

/**
	Charge l'orientationSet depuis une chaine de caractere.
	Cette chaine doit faire 4 caracteres, representant respectivement
	le Nord, l'Est, le Sud et l'Ouest. Le caractere y indique que l'orientation
	est autorisee, le caractere n indique que l'orientation est interdite et le
	caractere d designe l'orientation par defaut. L'orientation courante est
	celle par defaut.
	@param str Chaine de caracteres a analyser et charger
	@return true si l'analyse a reussie, false sinon
*/
bool OrientationSet::fromString(const QString &str) {
	QRegExp osv("^([dyn])([dyn])([dyn])([dyn])$");	// osv : Orientation String Validator
	if (osv.indexIn(str) == -1) return(false);
	QStringList matches = osv.capturedTexts();
	
	// il doit y avoir exactement UN d dans les 4 lettres capturees
	if (matches.count("d") != 1) return(false);
	
	bool *ori_pointers[4] = { &north_ori, &east_ori, &south_ori, &west_ori };
	Qet::Orientation ori_ints[4] = { Qet::North, Qet::East, Qet::South, Qet::West };
	for(int i = 0 ; i < 4 ; ++ i) {
		QString current = matches.at(i + 1);
		if (current == "d") {
			current_ori = default_ori = ori_ints[i];
			current = "y";
		}
		*(ori_pointers[i]) = (current == "y");
	}
	return(true);
}

/**
	@return Une chaine de caracteres representant cet OrientationSet.
	@see fromString
*/
QString OrientationSet::toString() const {
	bool ori_pointers[4] = { north_ori, east_ori, south_ori, west_ori };
	Qet::Orientation ori_ints[4] = { Qet::North, Qet::East, Qet::South, Qet::West };
	QString result("");
	for(int i = 0 ; i < 4 ; ++ i) {
		if (default_ori == ori_ints[i]) result += "d";
		else result += (ori_pointers[i] ? "y" : "n");
	}
	return(result);
}
