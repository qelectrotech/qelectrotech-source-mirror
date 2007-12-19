/*
	Copyright 2006-2007 Xavier Guerrin
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
#include "insetpropertieswidget.h"
#include "qetapp.h"

/**
	Constructeur
	@param inset InsetProperties a afficher
	@param current true pour afficher l'option "Date courante"
	@param parent QWidget parent
*/
InsetPropertiesWidget::InsetPropertiesWidget(const InsetProperties &inset, bool current, QWidget *parent) : QWidget(parent), display_current_date(false) {
	
	QVBoxLayout *this_layout = new QVBoxLayout(this);
	this_layout -> setContentsMargins(0, 0, 0, 0);
	QGroupBox *inset_infos = new QGroupBox(tr("Informations du cartouche"), this);
	inset_infos -> setMinimumSize(300, 260);
	this_layout -> addWidget(inset_infos);
	
	inset_title = new QLineEdit(this);
	inset_author = new QLineEdit(this);
	
	QButtonGroup *date_policy_group = new QButtonGroup(this);
	inset_no_date = new QRadioButton(tr("Pas de date"), this);
	inset_current_date = new QRadioButton(tr("Date courante"), this);
	inset_fixed_date = new QRadioButton(tr("Date fixe : "), this);
	date_policy_group -> addButton(inset_no_date);
	date_policy_group -> addButton(inset_current_date);
	date_policy_group -> addButton(inset_fixed_date);
	inset_date = new QDateEdit(QDate::currentDate(), this);
	inset_date -> setEnabled(inset_fixed_date -> isChecked());
	inset_current_date -> setVisible(display_current_date);
	connect(inset_fixed_date, SIGNAL(toggled(bool)), inset_date, SLOT(setEnabled(bool)));
	inset_date -> setCalendarPopup(true);
	
	QGridLayout *layout_date = new QGridLayout();
	layout_date -> addWidget(inset_no_date,      0, 0);
	layout_date -> addWidget(inset_current_date, 1, 0);
	layout_date -> addWidget(inset_fixed_date,   2, 0);
	layout_date -> addWidget(inset_date,         2, 1);
	layout_date -> setColumnStretch(0, 1);
	layout_date -> setColumnStretch(1, 500);
	
	inset_filename = new QLineEdit(this);
	inset_folio = new QLineEdit(this);
	QGridLayout *layout_champs = new QGridLayout(inset_infos);
	
	layout_champs -> addWidget(new QLabel(tr("Titre : ")),   0, 0);
	layout_champs -> addWidget(inset_title,                  0, 1);
	layout_champs -> addWidget(new QLabel(tr("Auteur : ")),  1, 0);
	layout_champs -> addWidget(inset_author,                 1, 1);
	layout_champs -> addWidget(new QLabel(tr("Date : ")),    2, 0);
	layout_champs -> addLayout(layout_date,                  3, 1);
	layout_champs -> addWidget(new QLabel(tr("Fichier : ")), 4, 0);
	layout_champs -> addWidget(inset_filename,               4, 1);
	layout_champs -> addWidget(new QLabel(tr("Folio : ")),   5, 0);
	layout_champs -> addWidget(inset_folio,                  5, 1);
	
	inset_current_date -> setVisible(display_current_date = current);
	setInsetProperties(inset);
	setLayout(this_layout);
}

/// Destructeur
InsetPropertiesWidget::~InsetPropertiesWidget() {
}

/**
	@return Les proprietes affichees par le widget
*/
InsetProperties InsetPropertiesWidget::insetProperties() const {
	InsetProperties prop;
	prop.title    = inset_title -> text();
	prop.author   = inset_author -> text();
	prop.filename = inset_filename -> text();
	prop.folio    = inset_folio -> text();
	if (inset_no_date -> isChecked()) {
		prop.useDate = InsetProperties::UseDateValue;
		prop.date = QDate();
	} else if (inset_fixed_date -> isChecked()) {
		prop.useDate = InsetProperties::UseDateValue;
		prop.date = inset_date -> date();
	} else if (display_current_date && inset_current_date -> isChecked()) {
		prop.useDate = InsetProperties::CurrentDate;
		prop.date = QDate::currentDate();
	}
	return(prop);
}

/**
	Specifie les proprietes que le widget doit afficher
	@param inset nouvelles proprietes affichees par le widget
*/
void InsetPropertiesWidget::setInsetProperties(const InsetProperties &inset) {
	inset_title    -> setText(inset.title);
	inset_author   -> setText(inset.author);
	inset_filename -> setText(inset.filename);
	inset_folio    -> setText(inset.folio);
	if (display_current_date) {
		if (inset.useDate == InsetProperties::CurrentDate) {
			inset_current_date -> setChecked(true);
		} else {
			if (inset.date.isNull()) {
				inset_no_date -> setChecked(true);
			} else {
				inset_fixed_date -> setChecked(true);
				inset_date -> setDate(inset.date);
			}
		}
	} else {
		if (inset.useDate == InsetProperties::CurrentDate) {
			inset_fixed_date -> setChecked(true);
			inset_date -> setDate(QDate::currentDate());
		} else {
			if (inset.date.isNull()) {
				inset_no_date -> setChecked(true);
			} else {
				inset_fixed_date -> setChecked(true);
				inset_date -> setDate(inset.date);
			}
		}
	}
}

/**
	@return true si le widget affiche la proposition "Date courante", false sinon
*/
bool InsetPropertiesWidget::displayCurrentDate() const {
	return(display_current_date);
}
