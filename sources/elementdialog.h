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
#ifndef ELEMENT_DIALOG_H
#define ELEMENT_DIALOG_H
#include <QtCore>
#include "elementslocation.h"
class QDialog;
class QDialogButtonBox;
class ElementsCategoriesList;
class QFileNameEdit;
/**
	This class provides several dialogs to select an element or a category
	(e.g. new or existing, for opening or for saving...).
*/
class ElementDialog : public QObject {
	Q_OBJECT
	// enumerations
	/**
		This enum represents the available configurations for the required dialog
	*/
	enum {
		OpenElement  = 0, ///< The dialog should open an element
		SaveElement  = 1, ///< The dialog should select an element for saving
		OpenCategory = 2, ///< The dialog should open a category
		SaveCategory = 3  ///< The dialog should select a category for saving
	};
	
	// constructors, destructor
	public:
	ElementDialog(uint = ElementDialog::OpenElement, QWidget * = 0, QObject * = 0);
	virtual ~ElementDialog();
	private:
	ElementDialog(const ElementDialog &);
	
	// methods
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
	
	// attributes
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
