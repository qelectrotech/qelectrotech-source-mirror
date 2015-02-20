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
#include "qfilenameedit.h"
#include  "qetregexpvalidator.h"
#include <QKeyEvent>
#include <QRegExp>
#include <QToolTip>

/**
	Constructeur
	@param parent QWidget parent de ce champ de texte
*/
QFileNameEdit::QFileNameEdit(QWidget *parent) : QLineEdit(parent) {
	init();
}

/**
	Constructeur
	@param contents Contenu initial du champ
	@param parent QWidget parent de ce champ de texte
*/
QFileNameEdit::QFileNameEdit(const QString &contents, QWidget *parent) : QLineEdit(parent) {
	init();
	if (!contents.isEmpty() && regexp_.exactMatch(contents)) {
		setText(contents);
	}
}

/**
	Destructeur
*/
QFileNameEdit::~QFileNameEdit() {
}

/**
	@return true si le champ de texte est vide, false sinon
*/
bool QFileNameEdit::isEmpty() {
	return(text().isEmpty());
}

/**
	@return true si le champ de texte n'est pas vide et est valide
*/
bool QFileNameEdit::isValid() {
	return(regexp_.exactMatch(text()));
}

/**
	Construit l'objet
*/
void QFileNameEdit::init() {
	regexp_ = QRegExp("^[0-9a-z_\\-\\.]+$", Qt::CaseSensitive);
	validator_ = new QETRegExpValidator(regexp_, this);
	setValidator(validator_);
	tooltip_text_ = QString(
		tr(
			"Les caract\350res autoris\351s sont : \n"
			" - les chiffres [0-9]\n"
			" - les minuscules [a-z]\n"
			" - le tiret [-], l'underscore [_] et le point [.]\n",
			"tooltip content when editing a filename"
		)
	);
	connect(validator_, SIGNAL(validationFailed()), this, SLOT(validationFailed()));
}

/**
	Affiche l'info-bulle informant l'utilisateur des caracteres autorises.
*/
void QFileNameEdit::displayToolTip() {
	QToolTip::showText(
		mapToGlobal(QPoint(x() + width(), 0)),
		tooltip_text_,
		this,
		QRect()
	);
}

/**
	Gere le fait que la validation du champ de texte ait echoue.
*/
void QFileNameEdit::validationFailed() {
	displayToolTip();
}
