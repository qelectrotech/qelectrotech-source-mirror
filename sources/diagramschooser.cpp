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
#include "diagramschooser.h"
#include "qetproject.h"
#include "diagram.h"

/**
	Constructeur
	@param project Projet dont il faut afficher les schemas
	@param parent QWidget parent de ce widget
*/
DiagramsChooser::DiagramsChooser(QETProject *project, QWidget *parent) :
	QScrollArea(parent),
	project_(project),
	widget0_(0),
	vlayout0_(0)
{
	setFrameShadow(QFrame::Sunken);
	setFrameShape(QFrame::StyledPanel);
	setLineWidth(3);
	setMidLineWidth(3);
	
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding));
	
	updateList();
	
	setWidget(widget0_);
}

/**
	Destructeur
*/
DiagramsChooser::~DiagramsChooser() {
}

/**
	@return le projet dont ce widget affiche les schemas
*/
QETProject *DiagramsChooser::project() const {
	return(project_);
}

/**
	@return la liste des schemas selectionnes
*/
QList<Diagram *> DiagramsChooser::selectedDiagrams() const {
	QList<Diagram *> selected_diagrams;
	for (Diagram *diagram: project_ -> diagrams()) {
		QCheckBox *check_box = diagrams_[diagram];
		if (check_box && check_box -> isChecked()) {
			selected_diagrams << diagram;
		}
	}
	return(selected_diagrams);
}

/**
	@return la liste des schemas qui ne sont pas selectionnes
*/
QList<Diagram *> DiagramsChooser::nonSelectedDiagrams() const {
	QList<Diagram *> selected_diagrams;
	for (Diagram *diagram: diagrams_.keys()) {
		if (!(diagrams_[diagram] -> isChecked())) {
			selected_diagrams << diagram;
		}
	}
	return(selected_diagrams);
}

/**
	@param diagram Un schema cense etre present dans ce widget
*/
bool DiagramsChooser::diagramIsSelected(Diagram *const diagram) const {
	QCheckBox *checkbox = diagrams_.value(diagram);
	if (!checkbox) return(false);
	return(checkbox -> isChecked());
}

/**
	Selectionne les schemas contenus dans la liste diagrams_list
	@param diagrams_list Liste de schemas a selectionner
	@param select true pour selectionne les schemas de la liste, false pour les
	deselectionner
	@param reset true pour deselectionner tous les schemas avant de
	selectionner ceux de la liste
*/
void DiagramsChooser::setSelectedDiagrams(const QList<Diagram *> &diagrams_list, bool select, bool reset) {
	// evite d'emettre une rafale de signaux pour cette operation
	blockSignals(true);
	
	// deselectionne tous les schemas si demande
	if (reset) {
		for (QCheckBox *check_box: diagrams_.values()) {
			check_box -> setChecked(false);
		}
	}
	
	int changes = 0;
	QCheckBox *check_box;
	for (Diagram *diagram: diagrams_list) {
		if ((check_box = diagrams_[diagram])) {
			if (check_box -> isChecked() != select) {
				check_box -> setChecked(select);
				++ changes;
			}
		}
	}
	
	blockSignals(false);
	if (reset || changes) {
		emit(selectionChanged());
	}
}

/**
	Selectionne ou deselectionne tous les schemas
	@param select true pour selectionne les schemas de la liste, false pour les
	deselectionner
*/
void DiagramsChooser::setSelectedAllDiagrams(bool select) {
	blockSignals(true);
	for (QCheckBox *check_box: diagrams_.values()) {
		check_box -> setChecked(select);
	}
	blockSignals(false);
	emit(selectionChanged());
}

/**
	Met a jour la liste des schemas du projet
*/
void DiagramsChooser::updateList() {
	if (!project_) return;
	
	// retient la liste des schemas deja selectionnes
	QList<Diagram *> selected_diagrams = selectedDiagrams();
	
	// detruit les checkbox existantes
	QList<QCheckBox *> checkboxes = diagrams_.values();
	qDeleteAll(checkboxes.begin(), checkboxes.end());
	
	buildLayout();
	
	// recree les checkbox necessaires
	for (Diagram *diagram: project_ -> diagrams()) {
		// titre du schema
		QString diagram_title = diagram -> title();
        if (diagram_title.isEmpty()) diagram_title = tr("Folio sans titre");
		
		QCheckBox *checkbox = new QCheckBox(diagram_title);
		checkbox -> setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum));
		checkbox -> setChecked(selected_diagrams.contains(diagram));
		connect(checkbox, SIGNAL(toggled(bool)), this, SIGNAL(selectionChanged()));
		diagrams_.insert(diagram, checkbox);
		vlayout0_ -> addWidget(checkbox, 0, Qt::AlignLeft | Qt::AlignTop);
	}
}

/**
	Met en place la disposition du widget
*/
void DiagramsChooser::buildLayout() {
	if (vlayout0_) return;
	vlayout0_ = new QVBoxLayout();
	widget0_  = new QWidget();
	widget0_ -> setLayout(vlayout0_);
	widget0_ -> setMinimumSize(170, 0);
	widget0_ -> setMaximumSize(470, 10000);
	widget0_ -> setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
}
