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
#include "qetproject.h"
#include "autobreakconductor.h"
#include "conductorautonumerotation.h"
#include "cable/cablepart.h"
#include "cable/editcablecommand.h"
#include "diagram.h"
#include "itemgroups.h"
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
	m_driver_held = false;

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

		//A grouped driver left out of the move belongs to a group that one
		//locked member holds in place: it stays with the group
	m_driver_held = driver_item
			&& !ItemGroups::groupOf(driver_item).isNull()
			&& !m_moved_content.items().contains(driver_item);
	if (m_driver_held && m_status_bar) {
		m_status_bar->showMessage(QObject::tr(
			"This group cannot be moved: the position of one "
			"of its elements is locked."));
	}

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

		//Steps left over by a gesture which never reached its end are
		//never worth keeping: whatever starts here starts from nothing.
	qDeleteAll(m_extra_commands);
	m_extra_commands.clear();

		//The cable lines he marked together with the elements come along
		//too: a selection of elements and lines is one thing to move.
		//When the finger is on a line itself, that line carries its own
		//kind -- every line of the selection is begun by the one being
		//dragged (see CablePart) -- so there is nothing to collect here.
	m_moved_cables.clear();
	if (!qgraphicsitem_cast<CablePart *>(m_movement_driver))
	{
		for (QGraphicsItem *item : diagram->selectedItems())
		{
			auto *cable = qgraphicsitem_cast<CablePart *>(item);
			if (!cable || !cable->cable()) continue;

			cable->beginLineGesture();
			m_moved_cables.append(cable);
		}
	}

		//Where every one of them stands at this moment: calling the
		//movement off puts them all back here, recorded rather than
		//worked out from the movement, since every step of that movement
		//was snapped to the grid on its way and need not land on the
		//same place coming back.
	m_start_positions.clear();
	typedef DiagramContent dc;
	for (auto &qgi : m_moved_content.items(dc::Elements
										   | dc::TextFields
										   | dc::Images
										   | dc::Shapes
										   | dc::ElementTextFields
										   | dc::TextGroup))
	{
		m_start_positions.insert(qgi, qgi->pos());
	}
	for (auto *conductor : m_moved_content.m_conductors_to_move)
	{
		if (conductor && conductor->textItem()
			&& conductor->textItem()->wasMovedByUser()) {
			m_start_positions.insert(conductor->textItem(),
									 conductor->textItem()->pos());
		}
	}

	/* At this point, we've got all info to manage movement.
	 * There is now a move in progress */
	m_movement_running = true;

	return(m_moved_content.count());
}

/**
	@brief ElementsMover::holds
	@return true if @a item is the item the user drags and it must not move:
	it is in a group that a locked member keeps in place (#1146). Each item
	that drives a movement asks before moving itself.
*/
bool ElementsMover::holds(const QGraphicsItem *item) const
{
	return m_driver_held && item && item == m_movement_driver;
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

		//The cable lines he marked together with the elements come along
		//by the very same step: one gesture moves the whole selection
		//instead of the lines standing still while the elements go, or
		//the other way round. They are drawn where the gesture brings
		//them, exactly as if the finger had been on the line itself, and
		//settled once at the end (see endMovement).
	for (CablePart *cable : std::as_const(m_moved_cables))
	{
		if (cable) cable->continueLineGesture(movement);
	}

	if (m_status_bar && m_movement_driver)
	{
			//A line does not stand at a point of its own -- it is drawn
			//straight on the sheet -- so its own end is what is worth
			//showing rather than its origin, which never moves.
		auto *cable = qgraphicsitem_cast<CablePart *>(m_movement_driver);
		const auto point_{cable ? cable->firstPoint()
								: m_movement_driver->scenePos()};
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

		//The lines he marked together with the elements settle first:
		//they may have to be asked something before they can rest, and
		//their answer decides whether the whole gesture is written at
		//all -- one of them calling it off calls off everything, the
		//elements included, and there is nothing to undo because nothing
		//at all has been written.
	QList<QUndoCommand *> cable_steps;
	for (CablePart *cable : std::as_const(m_moved_cables))
	{
		if (!cable) continue;
		if (!cable->settleLineGesture(cable_steps))
		{
			qDeleteAll(cable_steps);
			cable_steps.clear();
			cancelMovement();
			return;
		}
	}
	for (CablePart *cable : std::as_const(m_moved_cables))
	{
		if (cable) cable->doneLineGesture();
	}
	m_moved_cables.clear();

		//empty command to be used has parent of commands below
	QUndoCommand *undo_object{new QUndoCommand()};

		//Create undo move if there is a movement
	if (!m_current_movement.isNull()) {
		QUndoCommand *quc{new MoveGraphicsItemCommand(m_diagram, m_moved_content, m_current_movement, undo_object)};
		undo_object->setText(quc->text());
	}

		//Auto-break conductors: for each moved element, break any conductor
		//whose path passes through a terminal dock point, and reconnect through
		//the element.  The element is already at its final position on screen.
	if (m_diagram->project()->autoBreakConductor())
	{
		QList<Conductor *> conductors_handled;
		QSet<Terminal *> used_terminals;
		for (Element *e : m_moved_content.m_elements) {
			autoBreakConductors(m_diagram, e, undo_object,
					    conductors_handled, used_terminals);
		}
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
					undo_object->setText(QObject::tr("Add %n conductors", "add a numbers of conductor one or more", acc));

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

		//Add undo_object if have child -- or, when the lines of this
		//selection brought their own undo over (see addExtraCommands),
		//one single step for the whole gesture: taking it back has to
		//take all of it back, or half the selection would stay where
		//the gesture left it.
	QList<QUndoCommand *> steps = m_extra_commands;
	m_extra_commands.clear();
	steps.append(cable_steps);

	if (steps.isEmpty())
	{
		if (undo_object->childCount())
			m_diagram->undoStack().push(undo_object);
		else
			delete undo_object;
	}
	else
	{
		QString text = undo_object->text();
		if (text.isEmpty())
			text = steps.first()->text();

		if (undo_object->childCount())
			steps.prepend(undo_object);
		else
			delete undo_object;

		if (steps.size() == 1)
			m_diagram->undoStack().push(steps.takeFirst());
		else
			m_diagram->undoStack().push(new BatchCommand(steps, text));
	}

		// There is no movement in progress now
	m_movement_running = false;
	m_moved_content.clear();
	m_moved_cables.clear();
	m_start_positions.clear();

		//Keep saying why a held group did not move
	if (m_status_bar && !m_driver_held) {
		m_status_bar->clearMessage();
	}
}

/**
	@brief ElementsMover::addExtraCommands
	Take over undo steps of items this mover does not move itself --
	the cable lines the user dragged himself -- so that the end of the
	movement pushes them together with its own: one gesture, one step
	in the history, whatever the selection is made of.

	They are thrown away by cancelMovement() if the gesture is called
	off, and pushed by endMovement() otherwise.
	@param steps the commands, given away with them
*/
void ElementsMover::addExtraCommands(const QList<QUndoCommand *> &steps)
{
	for (QUndoCommand *step : steps)
	{
		if (step) m_extra_commands.append(step);
	}
}

/**
	@brief ElementsMover::cancelMovement
	Call the whole movement off without writing anything: every element
	goes back to the place the gesture found it, every line of the
	selection puts itself back where it stood and the undo steps which
	were waiting for this movement are thrown away.

	One gesture is written as a whole or not at all, which is what a
	"Annuler" answer to a question asked while the lines settle means.
*/
void ElementsMover::cancelMovement()
{
	if (!m_movement_running) return;

		//Back to the places the gesture found them in: the recorded
		//ones rather than the movement taken back, since every step of
		//that movement was snapped to the grid on its way and coming
		//back by the same amount need not land on the same place.
	for (auto it = m_start_positions.constBegin();
		 it != m_start_positions.constEnd(); ++it)
	{
		if (it.key()) it.key()->setPos(it.value());
	}
	m_start_positions.clear();

	for (auto *conductor : m_moved_content.m_conductors_to_move)
	{
		if (conductor) conductor->updatePath();
	}

		//Every line of the selection puts itself back where it stood
	for (CablePart *cable : std::as_const(m_moved_cables))
	{
		if (cable) cable->abortLineGesture();
	}
	m_moved_cables.clear();

		//Steps handed over by a line he dragged himself go with
		//everything else: nothing has been written, so there is nothing
		//to take back.
	qDeleteAll(m_extra_commands);
	m_extra_commands.clear();

	m_current_movement -= m_current_movement;
	m_movement_running = false;
	m_moved_content.clear();

	if (m_status_bar && !m_driver_held) {
		m_status_bar->clearMessage();
	}
}
