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
#include "titleblockpropertieswidget.h"
#include "qetapp.h"

/**
	Constructeur
	@param titleblock TitleBlockProperties a afficher
	@param current true pour afficher l'option "Date courante"
	@param parent QWidget parent
*/
TitleBlockPropertiesWidget::TitleBlockPropertiesWidget(const TitleBlockProperties &titleblock, bool current, QWidget *parent) : QWidget(parent), display_current_date(false) {
	
	QVBoxLayout *this_layout = new QVBoxLayout(this);
	this_layout -> setContentsMargins(0, 0, 0, 0);
	QGroupBox *titleblock_infos = new QGroupBox(tr("Informations du cartouche"), this);
	titleblock_infos -> setMinimumSize(300, 330);
	this_layout -> addWidget(titleblock_infos);
	
	titleblock_title = new QLineEdit(this);
	titleblock_author = new QLineEdit(this);
	
	QButtonGroup *date_policy_group = new QButtonGroup(this);
	titleblock_no_date = new QRadioButton(tr("Pas de date"), this);
	titleblock_current_date = new QRadioButton(tr("Date courante"), this);
	titleblock_fixed_date = new QRadioButton(tr("Date fixe : "), this);
	date_policy_group -> addButton(titleblock_no_date);
	date_policy_group -> addButton(titleblock_current_date);
	date_policy_group -> addButton(titleblock_fixed_date);
	titleblock_date = new QDateEdit(QDate::currentDate(), this);
	titleblock_date -> setEnabled(titleblock_fixed_date -> isChecked());
	titleblock_current_date -> setVisible(display_current_date);
	connect(titleblock_fixed_date, SIGNAL(toggled(bool)), titleblock_date, SLOT(setEnabled(bool)));
	titleblock_date -> setCalendarPopup(true);
	
	QGridLayout *layout_date = new QGridLayout();
	layout_date -> addWidget(titleblock_no_date,      0, 0);
	layout_date -> addWidget(titleblock_current_date, 1, 0);
	layout_date -> addWidget(titleblock_fixed_date,   2, 0);
	layout_date -> addWidget(titleblock_date,         2, 1);
	layout_date -> setColumnStretch(0, 1);
	layout_date -> setColumnStretch(1, 500);
	
	titleblock_filename = new QLineEdit(this);
	titleblock_folio = new QLineEdit(this);
	QLabel *folio_tip = new QLabel(
		tr(
			"Les variables suivantes sont utilisables dans le champ Folio :\n"
			"  - %id : num\351ro du sch\351ma courant dans le projet\n"
			"  - %total : nombre total de sch\351mas dans le projet"
		)
	);
	folio_tip -> setWordWrap(true);
	
	QGridLayout *layout_champs = new QGridLayout(titleblock_infos);
	
	layout_champs -> addWidget(new QLabel(tr("Titre : ")),   0, 0);
	layout_champs -> addWidget(titleblock_title,                  0, 1);
	layout_champs -> addWidget(new QLabel(tr("Auteur : ")),  1, 0);
	layout_champs -> addWidget(titleblock_author,                 1, 1);
	layout_champs -> addWidget(new QLabel(tr("Date : ")),    2, 0, Qt::AlignTop);
	layout_champs -> addLayout(layout_date,                  2, 1);
	layout_champs -> addWidget(new QLabel(tr("Fichier : ")), 3, 0);
	layout_champs -> addWidget(titleblock_filename,               3, 1);
	layout_champs -> addWidget(new QLabel(tr("Folio : ")),   4, 0);
	layout_champs -> addWidget(titleblock_folio,                  4, 1);
	layout_champs -> addWidget(folio_tip,                    5, 1, Qt::AlignTop);
	layout_champs -> setRowStretch(5, 500);
	
	titleblock_current_date -> setVisible(display_current_date = current);
	setTitleBlockProperties(titleblock);
	setLayout(this_layout);
}

/// Destructeur
TitleBlockPropertiesWidget::~TitleBlockPropertiesWidget() {
}

/**
	@return Les proprietes affichees par le widget
*/
TitleBlockProperties TitleBlockPropertiesWidget::titleBlockProperties() const {
	TitleBlockProperties prop;
	prop.title    = titleblock_title -> text();
	prop.author   = titleblock_author -> text();
	prop.filename = titleblock_filename -> text();
	prop.folio    = titleblock_folio -> text();
	if (titleblock_no_date -> isChecked()) {
		prop.useDate = TitleBlockProperties::UseDateValue;
		prop.date = QDate();
	} else if (titleblock_fixed_date -> isChecked()) {
		prop.useDate = TitleBlockProperties::UseDateValue;
		prop.date = titleblock_date -> date();
	} else if (display_current_date && titleblock_current_date -> isChecked()) {
		prop.useDate = TitleBlockProperties::CurrentDate;
		prop.date = QDate::currentDate();
	}
	return(prop);
}

/**
	Specifie les proprietes que le widget doit afficher
	@param titleblock nouvelles proprietes affichees par le widget
*/
void TitleBlockPropertiesWidget::setTitleBlockProperties(const TitleBlockProperties &titleblock) {
	titleblock_title    -> setText(titleblock.title);
	titleblock_author   -> setText(titleblock.author);
	titleblock_filename -> setText(titleblock.filename);
	titleblock_folio    -> setText(titleblock.folio);
	if (display_current_date) {
		if (titleblock.useDate == TitleBlockProperties::CurrentDate) {
			titleblock_current_date -> setChecked(true);
		} else {
			if (titleblock.date.isNull()) {
				titleblock_no_date -> setChecked(true);
			} else {
				titleblock_fixed_date -> setChecked(true);
				titleblock_date -> setDate(titleblock.date);
			}
		}
	} else {
		if (titleblock.useDate == TitleBlockProperties::CurrentDate) {
			titleblock_fixed_date -> setChecked(true);
			titleblock_date -> setDate(QDate::currentDate());
		} else {
			if (titleblock.date.isNull()) {
				titleblock_no_date -> setChecked(true);
			} else {
				titleblock_fixed_date -> setChecked(true);
				titleblock_date -> setDate(titleblock.date);
			}
		}
	}
}

/**
	@return true si le widget affiche la proposition "Date courante", false sinon
*/
bool TitleBlockPropertiesWidget::displayCurrentDate() const {
	return(display_current_date);
}

/**
	@return true si ce widget est en lecture seule, false sinon
*/
bool TitleBlockPropertiesWidget::isReadOnly() const {
	return(titleblock_title -> isReadOnly());
}

/**
	@param ro true pour passer ce widget en lecture seule, false sinon
*/
void TitleBlockPropertiesWidget::setReadOnly(bool ro) {
	titleblock_title        -> setReadOnly(ro);
	titleblock_author       -> setReadOnly(ro);
	titleblock_date         -> setReadOnly(ro);
	titleblock_filename     -> setReadOnly(ro);
	titleblock_folio        -> setReadOnly(ro);
	titleblock_no_date      -> setDisabled(ro);
	titleblock_current_date -> setDisabled(ro);
	titleblock_fixed_date   -> setDisabled(ro);
}
