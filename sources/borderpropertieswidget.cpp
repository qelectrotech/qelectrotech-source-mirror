/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "bordertitleblock.h"
// added to incorporate QColor functionality and so that
// variable Diagram::background_color is recognized in this file
#include <QColor>
#include "diagram.h"
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
	@return Les proprietes editees par ce widget
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
	@return true si ce widget est en lecture seule, false sinon
*/
bool BorderPropertiesWidget::isReadOnly() const {
	return(columns_count -> isReadOnly());
}

/**
	@param ro true pour passer ce widget en lecture seule, false sinon
*/
void BorderPropertiesWidget::setReadOnly(bool ro) {
	columns_count   -> setReadOnly(ro);
	columns_width   -> setReadOnly(ro);
	display_columns -> setDisabled(ro);
	rows_count      -> setReadOnly(ro);
	rows_height     -> setReadOnly(ro);
	display_rows    -> setDisabled(ro);
}

/**
	Definit les proprietes a editer
	@param bp Nouvelles proprietes
*/
void BorderPropertiesWidget::setEditedBorder(const BorderProperties &bp) {
	border_ = bp;
	columns_count   -> setValue(border_.columns_count);
	columns_width   -> setValue(qRound(border_.columns_width));
	display_columns -> setChecked(border_.display_columns);
	rows_count      -> setValue(border_.rows_count);
	rows_height     -> setValue(qRound(border_.rows_height));
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
	columns_count -> setMinimum(BorderTitleBlock::minNbColumns());
	columns_count -> setMaximum(10000); // valeur arbitraire
	
	columns_width = new QSpinBox(diagram_size_box);
	columns_width -> setMinimum(qRound(BorderTitleBlock::minColumnsWidth()));
	columns_width -> setMaximum(10000);
	columns_width -> setSingleStep(10);
	columns_width -> setPrefix(tr("\327", "multiplication symbol"));
	columns_width -> setSuffix(tr("px",   "unit for cols width"));
	
	display_columns = new QCheckBox(tr("Afficher les en-t\352tes"), diagram_size_box);
	
	// lignes : nombre et largeur
	QLabel *ds2 = new QLabel(tr("Lignes :"));
	
	rows_count = new QSpinBox(diagram_size_box);
	rows_count -> setMinimum(BorderTitleBlock::minNbRows());
	rows_count -> setMaximum(10000); // valeur arbitraire
	
	rows_height  = new QSpinBox(diagram_size_box);
	rows_height -> setMinimum(qRound(BorderTitleBlock::minRowsHeight()));
	rows_height -> setMaximum(10000);
	rows_height -> setSingleStep(10);
	rows_height -> setPrefix(tr("\327", "multiplication symbol"));
	rows_height -> setSuffix(tr("px",   "unit for rows height"));
	
	display_rows = new QCheckBox(tr("Afficher les en-t\352tes"), diagram_size_box);

	widget_layout -> addWidget(diagram_size_box);
	// add background color field
	QLabel *ds3 = new QLabel(tr("Couleur de fond :"));
	background_color = new QCheckBox(tr("Gris"), diagram_size_box);
	bool isnotChecked = Diagram::background_color == Qt::white;
	background_color -> setChecked(!isnotChecked);
	
	// layout
	diagram_size_box_layout -> addWidget(ds1,            0, 0);
	diagram_size_box_layout -> addWidget(columns_count,  0, 1);
	diagram_size_box_layout -> addWidget(columns_width,  0, 2);
	diagram_size_box_layout -> addWidget(display_columns,0, 3);
	diagram_size_box_layout -> addWidget(ds2,            1, 0);
	diagram_size_box_layout -> addWidget(rows_count,     1, 1);
	diagram_size_box_layout -> addWidget(rows_height,    1, 2);
	diagram_size_box_layout -> addWidget(display_rows,   1, 3);
	
	diagram_size_box_layout -> addWidget(ds3,            2, 0, 1, 2);
	diagram_size_box_layout -> addWidget(background_color, 2, 2, 1, 2);

	//build button connection
	connect(background_color, SIGNAL(stateChanged(int)), this, SLOT(changeColor()));
	setLayout(widget_layout);
}
	/**
	Background color choose QColorDialog. Makes Diagram::background_color equal to new chosen color.
	*/
void BorderPropertiesWidget::changeColor() {
	Diagram::background_color = (background_color -> isChecked()) ? Qt::gray : Qt::white;

}
