/*
	Copyright 2006-2008 Xavier Guerrin
	This file is part of QElectroTech.
	
	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.
	
	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.
	
	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "borderpropertieswidget.h"
#include <QtGui>
#include "qetapp.h"
#include "borderinset.h"

/**
	Constructeur
	Construit un widget editant les proprietes d'une bordure
	@param bp proprietes a editer
	@param parent QWidget parent
*/
BorderPropertiesWidget::BorderPropertiesWidget(const BorderProperties &bp, QWidget *parent) :
	QWidget(parent)
{
	build();
	setEditedBorder(bp);
}

/**
	Destructeur
*/
BorderPropertiesWidget::~BorderPropertiesWidget() {
}

/**
	@return the border properties edited and modified by
*/
const BorderProperties &BorderPropertiesWidget::borderProperties() {
	border_.columns_count   = columns_count   -> value();
	border_.columns_width   = columns_width   -> value();
	border_.display_columns = display_columns -> isChecked();
	border_.rows_count      = rows_count      -> value();
	border_.rows_height     = rows_height     -> value();
	border_.display_rows    = display_rows    -> isChecked();
	return(border_);
}

/**
	Definit les proprietes a editer
	@param bp Nouvelles proprietes
*/
void BorderPropertiesWidget::setEditedBorder(const BorderProperties &bp) {
	border_ = bp;
	columns_count   -> setValue(border_.columns_count);
	columns_width   -> setValue(border_.columns_width);
	display_columns -> setChecked(border_.display_columns);
	rows_count      -> setValue(border_.rows_count);
	rows_height     -> setValue(border_.rows_height);
	display_rows    -> setChecked(border_.display_rows);
}

/**
	Construit le widget
*/
void BorderPropertiesWidget::build() {
	QVBoxLayout *widget_layout = new QVBoxLayout();
	widget_layout -> setContentsMargins(0, 0, 0, 0);
	
	QGroupBox *diagram_size_box = new QGroupBox(tr("Dimensions du sch\351ma"));
	QGridLayout *diagram_size_box_layout = new QGridLayout(diagram_size_box);
	
	// colonnes : nombre et largeur
	QLabel *ds1 = new QLabel(tr("Colonnes :"));
	
	columns_count = new QSpinBox(diagram_size_box);
	columns_count -> setMinimum(BorderInset::minNbColumns());
	
	columns_width = new QSpinBox(diagram_size_box);
	columns_width -> setMinimum(qRound(BorderInset::minColumnsWidth()));
	columns_width -> setSingleStep(10);
	columns_width -> setPrefix(tr("\327"));
	columns_width -> setSuffix(tr("px"));
	
	display_columns = new QCheckBox(tr("Afficher les en-têtes"), diagram_size_box);
	
	// lignes : nombre et largeur
	QLabel *ds2 = new QLabel(tr("Lignes :"));
	
	rows_count = new QSpinBox(diagram_size_box);
	rows_count -> setMinimum(BorderInset::minNbRows());
	
	rows_height  = new QSpinBox(diagram_size_box);
	rows_height -> setMinimum(qRound(BorderInset::minRowsHeight()));
	rows_height -> setSingleStep(10);
	rows_height -> setPrefix(tr("\327"));
	rows_height -> setSuffix(tr("px"));
	
	display_rows = new QCheckBox(tr("Afficher les en-têtes"), diagram_size_box);
	
	// layout
	diagram_size_box_layout -> addWidget(ds1,            0, 0);
	diagram_size_box_layout -> addWidget(columns_count,  0, 1);
	diagram_size_box_layout -> addWidget(columns_width,  0, 2);
	diagram_size_box_layout -> addWidget(display_columns,0, 3);
	diagram_size_box_layout -> addWidget(ds2,            1, 0);
	diagram_size_box_layout -> addWidget(rows_count,     1, 1);
	diagram_size_box_layout -> addWidget(rows_height,    1, 2);
	diagram_size_box_layout -> addWidget(display_rows,   1, 3);
	
	widget_layout -> addWidget(diagram_size_box);
	setLayout(widget_layout);
}
