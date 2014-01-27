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
#ifndef NAMES_LIST_WIDGET_H
#define NAMES_LIST_WIDGET_H
#include <QtGui>
#include "nameslist.h"
/**
	This class provides a widget enabling users to enter localized names for
	categories and elements.
*/
class NamesListWidget : public QWidget {
	Q_OBJECT
	
	// constructors, destructor
	public:
	NamesListWidget(QWidget * = 0);
	virtual ~NamesListWidget();
	
	private:
	NamesListWidget(const NamesListWidget &);
	
	// attributes
	private:
	QTreeWidget *tree_names;
	QPushButton *button_add_line;
	NamesList hash_names;
	bool read_only;
	
	// methods
	public:
	bool checkOneName();
	NamesList names();
	void setNames(const NamesList &);
	void setReadOnly(bool);
	bool isReadOnly() const;
	
	private:
	void clean();
	void updateHash();
	
	public slots:
	void addLine();
	void check();
	
	signals:
	/**
		Signal emitted after the widget successfully checked there was at least one
		name entered
	*/
	void inputChecked();
};
#endif
