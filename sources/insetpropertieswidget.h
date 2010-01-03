/*
	Copyright 2006-2010 Xavier Guerrin
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
#ifndef INSET_PROPERTIES_WIDGET_H
#define INSET_PROPERTIES_WIDGET_H
#include <QtGui>
#include "insetproperties.h"
/**
	Ce widget permet d'editer un objet InsetProperties, c'est-a-dire les
	valeurs affichees par le cartouche d'un schema.
*/
class InsetPropertiesWidget : public QWidget {
	Q_OBJECT
	// constructeurs, destructeur
	public:
	InsetPropertiesWidget(const InsetProperties &inset = InsetProperties(), bool = false, QWidget * = 0);
	virtual ~InsetPropertiesWidget();
	private:
	InsetPropertiesWidget(const InsetPropertiesWidget &);
	
	// methodes
	public:
	InsetProperties insetProperties() const;
	void setInsetProperties(const InsetProperties &);
	bool displayCurrentDate() const;
	
	// attributs
	private:
	QLineEdit    *inset_title;
	QLineEdit    *inset_author;
	QDateEdit    *inset_date;
	QLineEdit    *inset_filename;
	QLineEdit    *inset_folio;
	QRadioButton *inset_no_date;
	QRadioButton *inset_current_date;
	QRadioButton *inset_fixed_date;
	bool display_current_date;
};
#endif
