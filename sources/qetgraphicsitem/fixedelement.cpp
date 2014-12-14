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
#include "fixedelement.h"
/**
	Constructeur
*/
FixedElement::FixedElement(QGraphicsItem *parent) : Element(parent) {
}

/**
	Destructeur
*/
FixedElement::~FixedElement() {
}

/**
	@return Le nombre minimal de bornes que l'element peut avoir
*/
int FixedElement::minTerminalsCount() const {
	return(terminalsCount());
}

/**
	@return Le nombre maximal de bornes que l'element peut avoir
*/
int FixedElement::maxTerminalsCount() const {
	return(terminalsCount());
}
