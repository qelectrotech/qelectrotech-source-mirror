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
#include "elementtextsmover.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/elementtextitem.h"
#include "diagram.h"
#include "diagramcommands.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"

/**
	Constructeur
*/
ElementTextsMover::ElementTextsMover() :
	movement_running_(false),
	current_movement_(),
	diagram_(0),
	movement_driver_(0),
	moved_texts_()
{
	
}

/**
	Destructeur
*/
ElementTextsMover::~ElementTextsMover() {
}

/**
	@return true si ce gestionnaire de deplacement est pret a etre utilise,
	false sinon. Un gestionnaire de deplacement est pret a etre utilise a partir
	du moment ou le mouvement precedemment gere n'est plus en cours.
*/
bool ElementTextsMover::isReady() const {
	return(!movement_running_);
}

/**
	Demarre un nouveau mouvement d'ElementTextItems
	@param diagram Schema sur lequel se deroule le deplacement
	@param driver_item Item deplace par la souris et ne necessitant donc pas
	d'etre deplace lors des appels a continueMovement.
	@return le nombre d'items concernes par le deplacement, ou -1 si le
	mouvement n'a pas ete initie
*/
int ElementTextsMover::beginMovement(Diagram *diagram, QGraphicsItem *driver_item) {
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
	moved_texts_.clear();
	foreach(QGraphicsItem *item, diagram -> selectedItems()) {
		if (ElementTextItem *text_item = qgraphicsitem_cast<ElementTextItem *>(item)) {
			moved_texts_ << text_item;
		}
	}
	
	// on s'assure qu'il y a quelque chose a deplacer
	if (!moved_texts_.count()) return(-1);
	
	// a ce stade, on dispose de toutes les informations necessaires pour
	// prendre en compte les mouvements
	
	// il y a desormais un mouvement en cours
	movement_running_ = true;
	
	return(moved_texts_.count());
}

/**
	Ajoute un mouvement au deplacement en cours. Cette methode
	@param movement mouvement a ajouter au deplacement en cours
*/
void ElementTextsMover::continueMovement(const QPointF &movement) {
	// un mouvement doit avoir ete initie
	if (!movement_running_) return;
	
	// inutile de faire quoi que ce soit s'il n'y a pas eu de mouvement concret
	if (movement.isNull()) return;
	
	// prise en compte du mouvement
	current_movement_ += movement;
	
	// deplace les elements selectionnes
	foreach(ElementTextItem *text_item, moved_texts_) {
		if (movement_driver_ && text_item == movement_driver_) continue;
		QPointF applied_movement = text_item -> mapMovementToParent(text_item-> mapMovementFromScene(movement));
		text_item -> setPos(text_item -> pos() + applied_movement);
	}
}

/**
	Termine le deplacement en creant un objet d'annulation et en l'ajoutant a
	la QUndoStack du schema concerne.
	@see Diagram::undoStack()
*/
void ElementTextsMover::endMovement() {
	// un mouvement doit avoir ete initie
	if (!movement_running_) return;
	
	// inutile de faire quoi que ce soit s'il n'y a pas eu de mouvement concret
	if (!current_movement_.isNull()) {
		// cree un objet d'annulation pour le mouvement ainsi realise
		MoveElementsTextsCommand*undo_object = new MoveElementsTextsCommand(
			diagram_,
			moved_texts_,
			current_movement_
		);
		
		diagram_ -> undoStack().push(undo_object);
	}
	
	// il n'y a plus de mouvement en cours
	movement_running_ = false;
}
