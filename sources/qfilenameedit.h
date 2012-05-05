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
#ifndef Q_FILENAME_EDIT_H
#define Q_FILENAME_EDIT_H
#include <QLineEdit>
#include <QRegExp>
#include <QString>
class QETRegExpValidator;
/**
	Cette classe represente un champ de texte dedie a la saisie d'un nom de
	fichier. Il permet de saisir un nom correspondant a l'expression reguliere
	^[0-9a-z_-\.]+$. Cela permet d'eviter les problemes avec des caracteres
	accentues, en majuscules, sortant de la table ASCII ou non-imprimables, ce
	qui devrait ameliorer la portabilite des elements crees par l'utilisateur.
*/
class QFileNameEdit : public QLineEdit {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	QFileNameEdit(QWidget * = 0);
	QFileNameEdit(const QString &, QWidget * = 0);
	virtual ~QFileNameEdit();
	private:
	QFileNameEdit(const QFileNameEdit &);
	
	// methodes
	public:
	bool isEmpty();
	bool isValid();
	
	private:
	void init();
	void displayToolTip();
	
	private slots:
	void validationFailed();
	
	// attributs
	private:
	QRegExp regexp_;
	QETRegExpValidator *validator_;
	QString tooltip_text_;
};
#endif
