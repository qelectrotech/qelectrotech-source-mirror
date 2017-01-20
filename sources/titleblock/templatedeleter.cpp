/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "templatedeleter.h"
#include "qetmessagebox.h"
#include "qetproject.h"
/**
	Constructor
	@param tbt_location Location of the title block template to be deleted
	@param parent Parent QWidget
*/
TitleBlockTemplateDeleter::TitleBlockTemplateDeleter(const TitleBlockTemplateLocation &tbt_location, QWidget *parent) :
	QWidget(parent),
	template_location_(tbt_location)
{
}

/**
	Destructor
*/
TitleBlockTemplateDeleter::~TitleBlockTemplateDeleter() {
}

/**
	Delete the title block template: check the provided location matches an
	existing template, interactively require confirmationfrom the user before
	actually proceeding to the deletion.
	@return true if the deletion succeeded, false otherwise.
*/
bool TitleBlockTemplateDeleter::exec() {
	if (!template_location_.isValid()) return(false);
	
	QString name = template_location_.name();
	TitleBlockTemplatesCollection *collection = template_location_.parentCollection();
	if (!collection) return(false);
	
	if (!collection -> templates().contains(name)) {
		return(false);
	}
	
	// require confirmation from the user
	QMessageBox::StandardButton answer = QET::QetMessageBox::question(
		this,
		tr("Supprimer le modèle de cartouche ?", "message box title"),
		QString(
			tr(
				"Êtes-vous sûr  de vouloir supprimer ce modèle de cartouche (%1) ?\n",
				"message box content"
			)
		).arg(name),
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
	);
	if (answer != QMessageBox::Yes) return(false);
	
	// delete the title block template
	collection -> removeTemplate(name);
	return(true);
}
