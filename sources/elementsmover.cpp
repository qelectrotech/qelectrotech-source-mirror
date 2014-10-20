/*
	Copyright 2006-2014 The QElectroTech Team
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
#include "conductor.h"
#include "conductortextitem.h"
#include "diagram.h"
#include "diagramcommands.h"
#include "element.h"
#include "independenttextitem.h"
#include "diagramimageitem.h"
#include "elementtextitem.h"

/**
 * @brief ElementsMover::ElementsMover Constructor
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
 * @brief ElementsMover::~ElementsMover Destructor
 */
ElementsMover::~ElementsMover() {
}

/**
 * @brief ElementsMover::isReady
 * @return True if this element mover is ready to be used.
 * A element mover is ready when the previous managed movement is finish.
 */
bool ElementsMover::isReady() const {
	return(!movement_running_);
}

/**
 * @brief ElementsMover::beginMovement
 * Start a new movement
 * @param diagram diagram where the movement is applied
 * @param driver_item item moved by mouse and don't be moved by Element mover
 * @return the numbers of items to be moved or -1 if movement can't be init.
 */
int ElementsMover::beginMovement(Diagram *diagram, QGraphicsItem *driver_item) {
	// They must be no movement in progress
	if (movement_running_) return(-1);
	
	// Be sure we have diagram to work
	if (!diagram) return(-1);
	diagram_ = diagram;
	
	// Take count of driver item
	movement_driver_ = driver_item;
	
	// At the beginning of movement, move is NULL
	current_movement_ = QPointF(0.0, 0.0);
	
	moved_content_ = diagram -> selectedContent();

	if (driver_item) {
		if (driver_item -> parentItem()) {
			if (moved_content_.items().contains(driver_item -> parentItem()))
				moved_content_.clear();
		}
	}
	
	/* We need to save the position of conductor text (ConductorTextItem)
	 * if the position is defined by user
	 * It's needed only for conductors whose the path will be recalculated */
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

	if (!moved_content_.count()) return(-1);
	
	/* At this point, we've got all info to manage movement.
	 * There is now a move in progress */
	movement_running_ = true;
	
	return(moved_content_.count());
}

/**
 * @brief ElementsMover::continueMovement
 * Add a move to the current movement.
 * @param movement movement to applied
 */
void ElementsMover::continueMovement(const QPointF &movement) {
	if (!movement_running_ || movement.isNull()) return;

	current_movement_ += movement;

	//Move every movable item, except conductor
	typedef DiagramContent dc;
	foreach (QGraphicsItem *qgi, moved_content_.items(dc::Elements | dc::TextFields | dc::ElementTextFields | dc::Images | dc::Shapes)) {
		if (qgi == movement_driver_) continue;
		if (qgi->parentItem()) {
			if (moved_content_.items().contains(qgi->parentItem()))
				continue;
		}
		qgi -> setPos(qgi->pos() + movement);
	}
	
	// Move some conductors
	foreach(Conductor *conductor, moved_content_.conductorsToMove) {
		conductor -> setPos(conductor -> pos() + movement);
	}
	
	// Recalcul the path of other conductors
	foreach(Conductor *conductor, moved_content_.conductorsToUpdate) {
		conductor -> updatePath();
	}
}

/**
 * @brief ElementsMover::endMovement
 * Ended the current movement by creating an undo added to the undostack of the diagram.
 */
void ElementsMover::endMovement() {
	// A movement must be inited
	if (!movement_running_) return;
	
	// No need of an undo command if the movement is NULL
	if (!current_movement_.isNull()) {
		// Create an undo object for this new movement
		MoveElementsCommand *undo_object = new MoveElementsCommand(
			diagram_,
			moved_content_,
			current_movement_
		);
		
		// Add info needed to the position of conductors texte
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
	
	// There is no movement in progress now
	movement_running_ = false;
}
