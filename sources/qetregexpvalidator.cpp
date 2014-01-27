/*
	Copyright 2006-2013 The QElectroTech Team
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
#include "qetregexpvalidator.h"

/**
	Constructeur
	@param parent QObject parent
*/
QETRegExpValidator::QETRegExpValidator(QObject *parent) : QRegExpValidator(parent) {
}

/**
	Constructeur
	@param regexp Expression reguliere a valider
	@param parent QObject parent
*/
QETRegExpValidator::QETRegExpValidator(const QRegExp &regexp, QObject *parent) : QRegExpValidator(regexp, parent) {
}

/**
	Destructeur
*/
QETRegExpValidator::~QETRegExpValidator() {
}

/**
	@see QRegExpValidator::validate
	@see validationFailed()
	Emet le signal validationFailed si la validation echoue
*/
QValidator::State QETRegExpValidator::validate(QString &input, int &pos) const {
	QValidator::State result = QRegExpValidator::validate(input, pos);
	if (result == QValidator::Invalid) emit(validationFailed());
	return(result);
}
