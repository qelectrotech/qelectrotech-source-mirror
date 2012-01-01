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
#ifndef BORDER_PROPERTIES_WIDGET_H
#define BORDER_PROPERTIES_WIDGET_H
#include <QWidget>
#include "borderproperties.h"
class QCheckBox;
class QSpinBox;
/**
	Cette classe represente un widget permettant d'editer les dimensions et les
	options d'affichage d'un schema, cartouche non inclus.
	@see TitleBlockPropertiesWidget
*/
class BorderPropertiesWidget : public QWidget {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	BorderPropertiesWidget(const BorderProperties &, QWidget * = 0);
	virtual ~BorderPropertiesWidget();
	private:
	BorderPropertiesWidget(const BorderPropertiesWidget &);
	
	// methodes
	public:
	const BorderProperties &borderProperties();
	bool isReadOnly() const;
	void setReadOnly(bool);
	
	private:
	void setEditedBorder(const BorderProperties &);
	void build();
	
	// attributs
	private:
	BorderProperties border_;       ///< Proprietes editees
	QSpinBox *columns_count;        ///< Widget d'edition du nombre de colonnes
	QSpinBox *columns_width;        ///< Widget d'edition de la largeur des colonnes
	QCheckBox *display_columns;     ///< Case a cocher pour afficher ou non les entetes des colonnes
	QSpinBox *rows_count;           ///< Widget d'edition du nombre de lignes
	QSpinBox *rows_height;          ///< Widget d'edition de la hauteur des lignes
	QCheckBox *display_rows;        ///< Case a cocher pour afficher ou non les entetes des lignes
};
#endif
