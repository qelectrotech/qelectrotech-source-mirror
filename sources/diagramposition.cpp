/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "diagramposition.h"

/**
	Constructeur
	@param letter Lettre(s) composant la position
	@param number Numero composant la position
	Si une chaine entierement invalide ou vide, ou bien un 0 est passe en
	parametre, il en resulte un objet DiagramPosition invalide, dont la methode
	isOutOfBounds renverra true.
*/
DiagramPosition::DiagramPosition(const QString &letter, unsigned int number) {
	// purifie les lettres
	letter_ = letter.toUpper();
	letter_.remove(QRegExp("[^A-Z]"));
	number_ = number;
}

/**
	Destructeur
*/
DiagramPosition::~DiagramPosition() {
}

/**
	@return les coordonnees stockees dans cet objet, ou un QPointF nul sinon.
*/
QPointF DiagramPosition::position() const {
	return(position_);
}

/**
	@param position Position a stocker dans cet objet
*/
void DiagramPosition::setPosition(const QPointF &position) {
	position_ = position;
}

/**
	@return une representation textuelle de la position
*/
QString DiagramPosition::toString() {
	if (isOutOfBounds()) {
		return("-");
	}
	return(QString("%1%2").arg(letter_).arg(number_));
}

/**
	@return true si l'element est en dehors des bords du schema
*/
bool DiagramPosition::isOutOfBounds() const {
	return(letter_.isEmpty() || !number_);
}
