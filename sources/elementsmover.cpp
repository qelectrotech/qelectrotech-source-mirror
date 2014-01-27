/*
	Copyright 2006-2013 The QElectroTech Team
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
#include "elementsmover.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "diagram.h"
#include "diagramcommands.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/diagramimageitem.h"

/**
	Constructeur
*/
ElementsMover::ElementsMover() :
	movement_running_(false),
	current_movement_(),
	diagram_(0),
	movement_driver_(0),
	moved_content_()
{
	
}

/**
	Destructeur
*/
ElementsMover::~ElementsMover() {
}

/**
	@return true si ce gestionnaire de deplacement est pret a etre utilise,
	false sinon. Un gestionnaire de deplacement est pret a etre utilise a partir
	du moment ou le mouvement precedemment gere n'est plus en cours.
*/
bool ElementsMover::isReady() const {
	return(!movement_running_);
}

/**
	Demarre un nouveau mouvement d'element
	@param diagram Schema sur lequel se deroule le deplacement
	@param driver_item Item deplace par la souris et ne necessitant donc pas
	d'etre deplace lors des appels a continueMovement.
	@return le nombre d'items concernes par le deplacement, ou -1 si le
	mouvement n'a pas ete initie
*/
int ElementsMover::beginMovement(Diagram *diagram, QGraphicsItem *driver_item) {
	// il ne doit pas y avoir de mouvement en cours
	if (movement_running_) return(-1);
	
	// on s'assure que l'on dispose d'un schema pour travailler
	if (!diagram) return(-1);
	diagram_ = diagram;
	
	// on prend en compte le driver_item
	movement_driver_ = driver_item;
	
	// au debut du mouvement, le deplacement est nul
	current_movement_ = QPointF(0.0, 0.0);
	
	// on stocke dans cet objet les items concernes par le deplacement
	moved_content_ = diagram -> selectedContent();
	
	// on a egalement besoin de retenir la position des champs de textes
	// rattaches a un conducteur (ConductorTextItem) si cette position a ete
	// personnalisee.
	// ceci n'est necessaire que pour les conducteurs dont le trajet sera
	// recalcule a cause du mouvement
	foreach(Conductor *conductor, moved_content_.conductorsToUpdate) {
		if (ConductorTextItem *text_item = conductor -> textItem()) {
			if (text_item -> wasMovedByUser()) {
				updated_conductors_text_pos_.insert(
					text_item,
					text_item -> pos()
				);
			}
		}
	}
	// on s'assure qu'il y a quelque chose a deplacer
	if (!moved_content_.count()) return(-1);
	
	// a ce stade, on dispose de toutes les informations necessaires pour
	// prendre en compte les mouvements
	
	// il y a desormais un mouvement en cours
	movement_running_ = true;
	
	return(moved_content_.count());
}

/**
	Ajoute un mouvement au deplacement en cours. Cette methode
	@param movement mouvement a ajouter au deplacement en cours
*/
void ElementsMover::continueMovement(const QPointF &movement) {
	// un mouvement doit avoir ete initie
	if (!movement_running_) return;
	
	// inutile de faire quoi que ce soit s'il n'y a pas eu de mouvement concret
	if (movement.isNull()) return;
	
	// prise en compte du mouvement
	current_movement_ += movement;
	
	// deplace les elements selectionnes
	foreach(Element *element, moved_content_.elements) {
		if (movement_driver_ && element == movement_driver_) continue;
		element -> setPos(element -> pos() + movement);
	}
	
	// deplace certains conducteurs
	foreach(Conductor *conductor, moved_content_.conductorsToMove) {
		conductor -> setPos(conductor -> pos() + movement);
	}
	
	// recalcule les autres conducteurs
	foreach(Conductor *conductor, moved_content_.conductorsToUpdate) {
		conductor -> updatePath();
	}
	
	// deplace les champs de texte
	foreach(IndependentTextItem *text_field, moved_content_.textFields) {
		if (movement_driver_ && text_field == movement_driver_) continue;
		text_field -> setPos(text_field -> pos() + movement);
	}

	//deplace les images
	foreach(DiagramImageItem *dii, moved_content_.images) {
		if (movement_driver_ && dii == movement_driver_) continue;
		dii -> setPos(dii -> pos() + movement);
	}

}

/**
	Termine le deplacement en creant un objet d'annulation et en l'ajoutant a
	la QUndoStack du schema concerne.
	@see Diagram::undoStack()
*/
void ElementsMover::endMovement() {
	// un mouvement doit avoir ete initie
	if (!movement_running_) return;
	
	// inutile de faire quoi que ce soit s'il n'y a pas eu de mouvement concret
	if (!current_movement_.isNull()) {
		// cree un objet d'annulation pour le mouvement ainsi realise
		MoveElementsCommand *undo_object = new MoveElementsCommand(
			diagram_,
			moved_content_,
			current_movement_
		);
		
		// ajoute les informations necessaires au repositionnement des champs
		// de textes des conducteurs
		foreach(ConductorTextItem *text_item, updated_conductors_text_pos_.keys()) {
			if (text_item -> pos() != updated_conductors_text_pos_[text_item]) {
				undo_object -> addConductorTextItemMovement(
					text_item,
					updated_conductors_text_pos_[text_item],
					text_item -> pos()
				);
			}
		}
		
		diagram_ -> undoStack().push(undo_object);
	}
	
	// il n'y a plus de mouvement en cours
	movement_running_ = false;
}
