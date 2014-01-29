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
#ifndef Q_FILENAME_EDIT_H
#define Q_FILENAME_EDIT_H
#include <QLineEdit>
#include <QRegExp>
#include <QString>
class QETRegExpValidator;
/**
	This class represents a textfield dedicated to input a portable filename (not
	a path).
	It enables users to input a name matching the regular expression
	^[0-9a-z_-\.]+$, thus avoiding problems with diacritics, non-printable,
	non-ASCII or uppercase characters, which should improve the portability of
	elements created by users.
*/
class QFileNameEdit : public QLineEdit {
	Q_OBJECT
	
	// constructors, destructor
	public:
	QFileNameEdit(QWidget * = 0);
	QFileNameEdit(const QString &, QWidget * = 0);
	virtual ~QFileNameEdit();
	private:
	QFileNameEdit(const QFileNameEdit &);
	
	// methods
	public:
	bool isEmpty();
	bool isValid();
	
	private:
	void init();
	void displayToolTip();
	
	private slots:
	void validationFailed();
	
	// attributes
	private:
	QRegExp regexp_;
	QETRegExpValidator *validator_;
	QString tooltip_text_;
};
#endif
