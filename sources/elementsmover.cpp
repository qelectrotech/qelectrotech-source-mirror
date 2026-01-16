/*
	Copyright 2006-2026 The QElectroTech Team
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

#include "conductorautonumerotation.h"
#include "diagram.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/diagramimageitem.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/elementtextitemgroup.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "undocommand/addgraphicsobjectcommand.h"
#include "qetapp.h"
#include "qetdiagrameditor.h"
#include "undocommand/movegraphicsitemcommand.h"

/**
	@brief ElementsMover::ElementsMover Constructor
*/
ElementsMover::ElementsMover(){}

/**
	@brief ElementsMover::~ElementsMover Destructor
*/
ElementsMover::~ElementsMover(){}

/**
	@brief ElementsMover::isReady
	@return True if this element mover is ready to be used.
	A element mover is ready when the previous managed movement is finish.
*/
bool ElementsMover::isReady() const
{
	return(!m_movement_running);
}

/**
	@brief ElementsMover::beginMovement
	Start a new movement
	@param diagram diagram where the movement is applied
	@param driver_item item moved by mouse and don't be moved by Element mover
	@return the numbers of items to be moved or -1 if movement can't be init.
*/
int ElementsMover::beginMovement(Diagram *diagram, QGraphicsItem *driver_item)
{
		// They must be no movement in progress
	if (m_movement_running) return(-1);

		// Be sure we have diagram to work
	if (!diagram) return(-1);
	m_diagram = diagram;

	if (!diagram->views().isEmpty()) {
		const auto qde = QETApp::diagramEditorAncestorOf(diagram->views().at(0));
		if (qde) {
			m_status_bar = qde->statusBar();
		}
	} else {
		m_status_bar.clear();
	}

		// Take count of driver item
	m_movement_driver = driver_item;

		// At the beginning of movement, move is NULL
	m_current_movement -= m_current_movement;

	m_moved_content = DiagramContent(diagram);
	m_moved_content.removeNonMovableItems();

		//Remove element text and text group, if the parent element is selected.
	const auto element_text{m_moved_content.m_element_texts};
	for(const auto &deti : element_text) {
		if(m_moved_content.m_elements.contains(deti->parentElement())) {
			m_moved_content.m_element_texts.remove(deti);
		}
	}
	const auto element_text_group{m_moved_content.m_texts_groups};
	for(const auto &etig : element_text_group) {
		if (m_moved_content.m_elements.contains(etig->parentElement())) {
			m_moved_content.m_texts_groups.remove(etig);
		}
	}

	if (!m_moved_content.count()) return(-1);

	/* At this point, we've got all info to manage movement.
	 * There is now a move in progress */
	m_movement_running = true;

	return(m_moved_content.count());
}

/**
	@brief ElementsMover::continueMovement
	Add a move to the current movement.
	@param movement movement to applied
*/
void ElementsMover::continueMovement(const QPointF &movement)
{
	if (!m_movement_running || movement.isNull()) return;

	m_current_movement += movement;

		//Move every movable item, except conductor
	typedef DiagramContent dc;
	for (auto &qgi : m_moved_content.items(dc::Elements
										   | dc::TextFields
										   | dc::Images
										   | dc::Shapes
										   | dc::ElementTextFields
										   | dc::TextGroup))
	{
		if (qgi == m_movement_driver)
			continue;
		qgi->setPos(qgi->pos() + movement);
	}

	QVector<Conductor *>list_conductors;
	for(auto *con : m_moved_content.m_conductors_to_move){
		list_conductors << con;
	}

	// update conductors 'conductors_to_move'
	for(auto *cond : list_conductors){
		cond->updatePath();
		if(cond->textItem()->wasMovedByUser() == true)
			cond->textItem()->setPos(cond->textItem()->pos()+movement);
	}

	// update conductors 'conductors_to_update'
	for (auto &conductor : m_moved_content.m_conductors_to_update)
	{
		conductor->updatePath();
	}

	if (m_status_bar && m_movement_driver)
	{
		const auto point_{m_movement_driver->scenePos()};
		m_status_bar->showMessage(QString("x %1 : y %2").arg(QString::number(point_.x()), QString::number(point_.y())));
	}
}

/**
	@brief ElementsMover::endMovement
	Ended the current movement by creating an undo added to the undostack of the diagram.
	If there is only one element moved, we try to auto-connect new conductor from this element
	and other possible element.
*/
void ElementsMover::endMovement()
{
		// A movement must be inited
	if (!m_movement_running) return;

		//empty command to be used has parent of commands below
	QUndoCommand *undo_object{new QUndoCommand()};

		//Create undo move if there is a movement
	if (!m_current_movement.isNull()) {
		QUndoCommand *quc{new MoveGraphicsItemCommand(m_diagram, m_moved_content, m_current_movement, undo_object)};
		undo_object->setText(quc->text());
	}

		//There is only one element moved, and project authorize auto conductor,
		//we try auto connection of conductor;
	typedef DiagramContent dc;
	if (m_moved_content.items(dc::TextFields
								| dc::Images
								| dc::Shapes
								| dc::TerminalStrip).isEmpty()
		&& m_moved_content.items(dc::Elements).size() == 1
		&& m_diagram->project()->autoConductor())
	{
		const Element *elmt{m_moved_content.m_elements.first()};
		const auto aligned_free_terminals{elmt->AlignedFreeTerminals()};

		if (const int acc = aligned_free_terminals.size())
		{
			for (const auto &pair : aligned_free_terminals)
			{
				Conductor *conductor{new Conductor(pair.first, pair.second)};

					//Create an undo object for each new auto conductor, with undo_object for parent
				new AddGraphicsObjectCommand(conductor, m_diagram, QPointF(), undo_object);
				if (undo_object->text().isEmpty())
					undo_object->setText(QObject::tr("Ajouter %n conducteur(s)", "add a numbers of conductor one or more", acc));

					//Get all conductors at the same potential of conductor
				const auto conductors_list{conductor->relatedPotentialConductors()};

					//Compare the properties of every conductors stored in conductors_list,
					//if every conductors properties is equal, we use this properties for conductor.
				ConductorProperties others_properties;
				bool use_properties = false;
				if (!conductors_list.isEmpty())
				{
					use_properties = true;
					others_properties = (*conductors_list.cbegin())->properties();
					for (const auto &cond :  conductors_list)
						if (cond->properties() != others_properties)
							use_properties = false;
				}

				if (use_properties)
					conductor->setProperties(others_properties);
				else
				{
					conductor -> setProperties(m_diagram -> defaultConductorProperties);
					//Autonum the new conductor, the undo command associated for this, have for parent undo_object
					ConductorAutoNumerotation can  (conductor, m_diagram, undo_object);
					can.numerate();
				}
			}
		}
	}

		//Add undo_object if have child
	if (undo_object->childCount())
		m_diagram->undoStack().push(undo_object);
	else
		delete undo_object;

		// There is no movement in progress now
	m_movement_running = false;
	m_moved_content.clear();

	if (m_status_bar) {
		m_status_bar->clearMessage();
	}
}
