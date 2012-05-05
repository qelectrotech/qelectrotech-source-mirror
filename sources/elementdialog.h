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
#ifndef ELEMENT_DIALOG_H
#define ELEMENT_DIALOG_H
#include <QtCore>
#include "elementslocation.h"
class QDialog;
class QDialogButtonBox;
class ElementsCategoriesList;
class QFileNameEdit;
/**
	Cette classe permet d'acceder a plusieurs types de dialogues lies a la
	selection d'element ou de categorie.
*/
class ElementDialog : public QObject {
	Q_OBJECT
	// enumerations
	/**
		Cette enum represente les configurations possible pour ce dialogue
	*/
	enum {
		OpenElement  = 0, ///< Le dialogue sera en mode ouverture
		SaveElement  = 1, ///< Le dialogue sera en mode enregistrement
		OpenCategory = 2,  ///< Le dialogue concernera une categorie
		SaveCategory = 3 ///< Le dialogue concernera un element
	};
	
	// constructeurs, destructeur
	public:
	ElementDialog(uint = ElementDialog::OpenElement, QWidget * = 0, QObject * = 0);
	virtual ~ElementDialog();
	private:
	ElementDialog(const ElementDialog &);
	
	// methodes
	public:
	int exec();
	ElementsLocation location() const;
	static ElementsLocation getExistingCategoryLocation(QWidget * = 0);
	static ElementsLocation getNewCategoryLocation(QWidget * = 0);
	static ElementsLocation getOpenElementLocation(QWidget * = 0);
	static ElementsLocation getSaveElementLocation(QWidget * = 0);
	
	private:
	static ElementsLocation execConfiguredDialog(int, QWidget * = 0);
	
	private slots:
	void locationChanged(const ElementsLocation &);
	void textFieldChanged(const QString &);
	void dialogAccepted();
	void dialogRejected();
	void checkDialog();
	
	private:
	void makeInterface();
	
	// attributs
	private:
	uint mode_;
	ElementsLocation location_;
	QString title_;
	QString label_;
	QDialog *dialog_;
	QDialogButtonBox *buttons_;
	ElementsCategoriesList *list_;
	QFileNameEdit *textfield_;
};
#endif
