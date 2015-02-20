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
#ifndef ABOUTQET_H
#define ABOUTQET_H
#include <QDialog>
class QLabel;
/**
	This class represents the "About QElectroTech" dialog.
*/
class AboutQET : public QDialog {
	Q_OBJECT
	
	// constructors, destructor
	public:
	AboutQET(QWidget * = 0);
	virtual ~AboutQET();
	
	private:
	AboutQET(AboutQET &);
	
	// methods
	private:
	QWidget *title() const;
	QWidget *aboutTab() const;
	QWidget *authorsTab() const;
	QWidget *translatorsTab() const;
	QWidget *contributorsTab() const;
	QWidget *licenseTab() const;
	void addAuthor(QLabel *, const QString &, const QString &, const QString &) const;
};
#endif
