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
#include "qetapp.h"

/**
	Constructeur
	@param parent QWidget parent
*/
NewDiagramPage::NewDiagramPage(QWidget *parent) : ConfigPage(parent) {
	
	// acces a la configuration de QElectroTech
	QSettings &settings = QETApp::settings();
	
	// recupere les dimensions du schema
	int columns_count_value  = settings.value("diagrameditor/defaultcols",       15).toInt();
	int columns_width_value  = qRound(settings.value("diagrameditor/defaultcolsize",  50.0).toDouble());
	int columns_height_value = qRound(settings.value("diagrameditor/defaultheight",  500.0).toDouble());
	
	QVBoxLayout *vlayout1 = new QVBoxLayout();
	
	QLabel *title = new QLabel(tr("Nouveau sch\351ma"));
	vlayout1 -> addWidget(title);
	
	QFrame *horiz_line = new QFrame();
	horiz_line -> setFrameShape(QFrame::HLine);
	vlayout1 -> addWidget(horiz_line);
	
	QHBoxLayout *hlayout1 = new QHBoxLayout();
	QVBoxLayout *vlayout2 = new QVBoxLayout();
	
	QGroupBox *diagram_size_box = new QGroupBox(tr("Dimensions du sch\351ma"));
	diagram_size_box -> setMinimumWidth(300);
	QGridLayout *diagram_size_box_layout = new QGridLayout(diagram_size_box);
	
	QLabel *ds1 = new QLabel(tr("Colonnes :"));
	
	columns_count = new QSpinBox(diagram_size_box);
	columns_count -> setMinimum(3); /// @todo methode statique pour recuperer ca
	columns_count -> setValue(columns_count_value);
	
	columns_width = new QSpinBox(diagram_size_box);
	columns_width -> setMinimum(1);
	columns_width -> setSingleStep(10);
	columns_width -> setValue(columns_width_value);
	columns_width -> setPrefix(tr("\327"));
	columns_width -> setSuffix(tr("px"));
	
	QLabel *ds2 = new QLabel(tr("Hauteur :"));
	
	columns_height = new QSpinBox(diagram_size_box);
	columns_height -> setRange(80, 10000); /// @todo methode statique pour recuperer ca
	columns_height -> setSingleStep(80);
	columns_height -> setValue(columns_height_value);
	
	diagram_size_box_layout -> addWidget(ds1,            0, 0);
	diagram_size_box_layout -> addWidget(columns_count,  0, 1);
	diagram_size_box_layout -> addWidget(columns_width,  0, 2);
	diagram_size_box_layout -> addWidget(ds2,            1, 0);
	diagram_size_box_layout -> addWidget(columns_height, 1, 1);
	
	ipw = new InsetPropertiesWidget(QETDiagramEditor::defaultInsetProperties(), true, this);
	
	// proprietes par defaut des conducteurs
	ConductorProperties cp;
	cp.fromSettings(settings, "diagrameditor/defaultconductor");
	cpw = new ConductorPropertiesWidget(cp);
	
	vlayout2 -> addWidget(diagram_size_box);
	vlayout2 -> addWidget(ipw);
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
	settings.setValue("diagrameditor/defaultcols",    columns_count -> value());
	settings.setValue("diagrameditor/defaultcolsize", columns_width -> value());
	settings.setValue("diagrameditor/defaultheight",  columns_height -> value());
	
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
