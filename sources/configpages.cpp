/*
	Copyright 2006-2008 Xavier Guerrin
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
#include "configpages.h"
#include "borderpropertieswidget.h"
#include "conductorpropertieswidget.h"
#include "insetpropertieswidget.h"
#include "qetapp.h"
#include "qetdiagrameditor.h"
#include "borderinset.h"

/**
	Constructeur
	@param parent QWidget parent
*/
NewDiagramPage::NewDiagramPage(QWidget *parent) : ConfigPage(parent) {
	
	// acces a la configuration de QElectroTech
	QSettings &settings = QETApp::settings();
	
	// dimensions par defaut d'un schema
	bpw = new BorderPropertiesWidget(QETDiagramEditor::defaultBorderProperties());
	
	// proprietes par defaut d'un cartouche
	ipw = new InsetPropertiesWidget(QETDiagramEditor::defaultInsetProperties(), true);
	
	// proprietes par defaut des conducteurs
	ConductorProperties cp;
	cp.fromSettings(settings, "diagrameditor/defaultconductor");
	cpw = new ConductorPropertiesWidget(cp);
	cpw -> setContentsMargins(0, 0, 0, 0);
	
	QVBoxLayout *vlayout1 = new QVBoxLayout();
	
	QLabel *title = new QLabel(tr("Nouveau sch\351ma"));
	vlayout1 -> addWidget(title);
	
	QFrame *horiz_line = new QFrame();
	horiz_line -> setFrameShape(QFrame::HLine);
	vlayout1 -> addWidget(horiz_line);
	
	QHBoxLayout *hlayout1 = new QHBoxLayout();
	QVBoxLayout *vlayout2 = new QVBoxLayout();
	
	vlayout2 -> addWidget(bpw);
	vlayout2 -> addWidget(ipw);
	vlayout2 -> setSpacing(5);
	hlayout1 -> addLayout(vlayout2);
	hlayout1 -> addWidget(cpw);
	vlayout1 -> addLayout(hlayout1);
	vlayout1 -> addStretch(1);
	hlayout1 -> setAlignment(cpw, Qt::AlignTop);
	setLayout(vlayout1);
}

/// Destructeur
NewDiagramPage::~NewDiagramPage() {
}

/**
	Applique la configuration de cette page
*/
void NewDiagramPage::applyConf() {
	QSettings &settings = QETApp::settings();
	
	// dimensions des nouveaux schemas
	BorderProperties border = bpw -> borderProperties();
	settings.setValue("diagrameditor/defaultcols",        border.columns_count);
	settings.setValue("diagrameditor/defaultcolsize",     border.columns_width);
	settings.setValue("diagrameditor/defaultdisplaycols", border.display_columns);
	settings.setValue("diagrameditor/defaultrows",        border.rows_count);
	settings.setValue("diagrameditor/defaultrowsize",     border.rows_height);
	settings.setValue("diagrameditor/defaultdisplayrows", border.display_rows);
	
	// proprietes du cartouche
	InsetProperties inset = ipw-> insetProperties();
	settings.setValue("diagrameditor/defaulttitle",    inset.title);
	settings.setValue("diagrameditor/defaultauthor",   inset.author);
	settings.setValue("diagrameditor/defaultfilename", inset.filename);
	settings.setValue("diagrameditor/defaultfolio",    inset.folio);
	QString date_setting_value;
	if (inset.useDate == InsetProperties::UseDateValue) {
		if (inset.date.isNull()) date_setting_value = "null";
		else date_setting_value = inset.date.toString("yyyyMMdd");
	} else {
		date_setting_value = "now";
	}
	settings.setValue("diagrameditor/defaultdate", date_setting_value);
	
	// proprietes par defaut des conducteurs
	cpw -> conductorProperties().toSettings(settings, "diagrameditor/defaultconductor");
}

/// @return l'icone de cette page
QIcon NewDiagramPage::icon() const {
	return(QIcon(":/ico/conf_new_diagram.png"));
}

/// @return le titre de cette page
QString NewDiagramPage::title() const {
	return(tr("Nouveau sch\351ma"));
}
