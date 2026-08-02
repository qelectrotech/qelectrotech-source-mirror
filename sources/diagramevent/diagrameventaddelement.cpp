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
#include "diagrameventaddelement.h"
#include "../qetproject.h"
#include "../conductorautonumerotation.h"
#include "../diagram.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../undocommand/deleteqgraphicsitemcommand.h"
#include "../factory/elementfactory.h"
#include "../qetapp.h"
#include "../qetdiagrameditor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qet.h"
#include <QPainterPath>
#include <limits>

/**
	@brief DiagramEventAddElement::DiagramEventAddElement
	Defaut constructor
	@param location :location of diagram
	@param diagram : diagram owner of this event
	@param pos : first pos of item ( optional, by defaut QPointF(0,0) )
*/
DiagramEventAddElement::DiagramEventAddElement(ElementsLocation &location, Diagram *diagram, QPointF pos) :
	DiagramEventInterface(diagram),
	m_location(location),
	m_element(nullptr)
{
		//Check if there is an element at this location
	if (location.isElement() && location.exist())
	{
			//location is an element, we build it, if build fail,
			//m_running stay to false (by default), so this interface will be deleted at next event
		if (buildElement())
		{
			init();
			m_element -> setPos(pos);
			m_element -> displayHelpLine(true);
			m_element -> setFlag(QGraphicsItem::ItemIsSelectable, false);
			m_diagram -> addItem(m_element);
			m_running = true;

			if (!diagram->views().isEmpty()) {
				const auto qde = QETApp::diagramEditorAncestorOf(diagram->views().at(0));
				if (qde) {
					m_status_bar = qde->statusBar();
				}
			} else {
				m_status_bar.clear();
			}
		}
	}
}

/**
	@brief DiagramEventAddElement::~DiagramEventAddElement
	Destructor
	Enable context menu for each view of diagram
*/
DiagramEventAddElement::~DiagramEventAddElement()
{
	if (m_element)
	{
		m_diagram->removeItem(m_element);
		m_element->deleteLater();
	}

	if (m_status_bar) {
		m_status_bar->clearMessage();
	}

	for (auto view : m_diagram->views())
		view -> setContextMenuPolicy(Qt::DefaultContextMenu);
}

/**
	@brief DiagramEventAddElement::mouseMoveEvent
	Move the element to new pos of mouse
	the event is always accepted
	@param event
*/
void DiagramEventAddElement::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_element)
	{
		const auto pos_{Diagram::snapToGrid(event->scenePos())};
		m_element->setPos(pos_);

		if (m_status_bar) {
			m_status_bar->showMessage(QString("x %1 : y %2").arg(QString::number(pos_.x()), QString::number(pos_.y())));
		}
	}
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddElement::mousePressEvent
	Do nothing, but return true for not transit the event to other thing in diagram.
	the event is always accepted
	@param event
*/
void DiagramEventAddElement::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddElement::mouseReleaseEvent
	Right button finish this event (isRunning = false) and emit finish.
	Left button add an element to diagram
	the event is always accepted
	@param event
*/
void DiagramEventAddElement::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_element)
	{
		if (event->button() == Qt::RightButton)
		{
			m_diagram->removeItem(m_element);
			m_element->deleteLater();
			m_element = nullptr;
			m_running = false;
			emit finish();
		}
		else if (event->button() == Qt::LeftButton)
		{
			addElement();
		}
	}

	event->setAccepted(true);
}

/**
	@brief DiagramEventAddElement::mouseDoubleClickEvent
	If mouse left double clic, finish this event (isRunning = false) and emit finish
	the event is always accepted
	@param event
*/
void DiagramEventAddElement::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_element && (event -> button() == Qt::LeftButton))
	{
		m_diagram->removeItem(m_element);
		m_element->deleteLater();
		m_element = nullptr;
		m_running = false;
		emit finish();
	}
	
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddElement::keyPressEvent
	Press space key rotate the element to 90° (return true)
	else  call DiagramEventInterface::keyPressEvent(event), and return the value.
	@param event
*/
void DiagramEventAddElement::keyPressEvent(QKeyEvent *event)
{
	if (m_element && event->key() == Qt::Key_Space)
	{
		m_element->setRotation(m_element->rotation() + 90);
		event->setAccepted(true);
	}
	else {
		DiagramEventInterface::keyPressEvent(event);
	}
}

/**
	@brief DiagramEventAddElement::init
	Init this event.
*/
void DiagramEventAddElement::init()
{
	foreach(QGraphicsView *view, m_diagram->views())
		view->setContextMenuPolicy(Qt::NoContextMenu);
}

/**
	@brief DiagramEventAddElement::buildElement
	Build the element, if the element is build successfully, we return true, otherwise false
*/
bool DiagramEventAddElement::buildElement()
{
	ElementsLocation import_loc = m_diagram->project()->importElement(m_location);
	if (import_loc.exist()) {
		m_integrate_path = import_loc.projectCollectionPath();
	}
	else {
		qDebug() << "DiagramView::addDroppedElement : Impossible d'ajouter l'element.";
		return false;
	}

	int state;
	ElementsLocation loc(m_integrate_path);
	m_element = ElementFactory::Instance() -> createElement(loc, nullptr, &state);
		//The creation of element failed, we delete it
	if (state) {
		delete m_element;
		return(false);
	}
		//Everything is good
	return true;
}

/**
	@brief DiagramEventAddElement::addElement
	Add an element at the current pos and current rotation,
	if project autoconductor option is enable, and the element can be wired, we do it.
*/
void DiagramEventAddElement::addElement()
{
	int state;
	Element *element;
	if (m_integrate_path.isEmpty())
		element = ElementFactory::Instance() -> createElement(m_location, nullptr, &state);
	else
		element = ElementFactory::Instance() -> createElement(ElementsLocation(m_integrate_path), nullptr, &state);

		//Build failed
	if (state)
	{
		delete element;
		return;
	}

		//We must add item to scene (even if addItemCommand do this)
		//for create the autoconnection below
	element -> setPos(m_element->pos());
	element -> setRotation(m_element -> rotation());
	m_diagram -> addItem(element);

	QUndoCommand *undo_object = new QUndoCommand(tr("Ajouter %1").arg(element->name()));
	new AddGraphicsObjectCommand(element, m_diagram, m_element -> pos(), undo_object);

	//When we search for free aligned terminal we temporally remove m_element to
	//avoid any interaction with the function Element::AlignedFreeTerminals
	//This is useful when an element has two (or more) terminals on opposite sides,
	//because m_element is exactly at the same pos of the new element
	//added to the scene so new conductor are created between terminal of the new element
	//and the opposite terminal of m_element.
	m_diagram->removeItem(m_element);

		//Auto break conductor: if a terminal of the new element lies on an existing
		//conductor, break the conductor and reconnect through the new element's terminal.
		//Track the endpoints of broken conductors so auto-connect doesn't create duplicates.
	QSet<Terminal *> broken_endpoints;

	if (m_diagram->project()->autoBreakConductor())
	{
			//Track which conductors we already handled for this element
		QList<Conductor *> conductors_handled;
			//Track which terminals of the new element are already used (by break or other-connect)
		QSet<Terminal *> used_terminals;

		foreach (Terminal *t, element->terminals())
		{
			QPointF t_dock = t->dockConductor();

			foreach (Conductor *c, m_diagram->conductors())
			{
					//Skip conductors we already handled or connected to the new element
				if (conductors_handled.contains(c) ||
				    c->terminal1->parentElement() == element ||
				    c->terminal2->parentElement() == element)
					continue;

					//Check if dock point lies on the conductor path.
					//Convert dock_point to the conductor's local coordinate system,
					//because c->path() is in local coordinates (generated via mapFromScene).
				QPointF local_dock = c->mapFromScene(t_dock);
				bool point_on_conductor = false;
				QPainterPath path = c->path();
				for (int i = 0; i < path.elementCount() - 1; ++i)
				{
					const QPainterPath::Element &e1 = path.elementAt(i);
					const QPainterPath::Element &e2 = path.elementAt(i + 1);
					QLineF segment(QPointF(e1.x, e1.y), QPointF(e2.x, e2.y));
					QPointF projection;
					if (QET::orthogonalProjection(local_dock, segment, &projection))
					{
						qreal dist = QLineF(local_dock, projection).length();
						if (dist < 5.0)
						{
							point_on_conductor = true;
							break;
						}
					}
				}

				if (!point_on_conductor)
					continue;

					//The terminal lies on this conductor.
					//Break it: delete old conductor, create one new conductor from the
					//aligned endpoint to the new terminal. The other endpoint is left
					//for auto-connect to handle (e.g. connecting to the opposite terminal
					//of the new element).
				Terminal *c1 = c->terminal1;
				Terminal *c2 = c->terminal2;

				conductors_handled.append(c);

					//Get scene positions for endpoint selection
				QPointF c1_dock = c1->dockConductor();
				QPointF c2_dock = c2->dockConductor();

					//Determine which endpoint to connect based on terminal orientation.
					//Terminal::orientation() already accounts for element rotation,
					//so a North terminal rotated 90° returns East, etc.
				Terminal *connect_to = nullptr;
				Terminal *other = nullptr;

				switch (t->orientation()) {
					case Qet::North:
						if (c1_dock.y() < t_dock.y()) { connect_to = c1; other = c2; }
						else if (c2_dock.y() < t_dock.y()) { connect_to = c2; other = c1; }
						break;
					case Qet::South:
						if (c1_dock.y() > t_dock.y()) { connect_to = c1; other = c2; }
						else if (c2_dock.y() > t_dock.y()) { connect_to = c2; other = c1; }
						break;
					case Qet::East:
						if (c1_dock.x() > t_dock.x()) { connect_to = c1; other = c2; }
						else if (c2_dock.x() > t_dock.x()) { connect_to = c2; other = c1; }
						break;
					case Qet::West:
						if (c1_dock.x() < t_dock.x()) { connect_to = c1; other = c2; }
						else if (c2_dock.x() < t_dock.x()) { connect_to = c2; other = c1; }
						break;
				}

					//Fallback: use nearest endpoint
				if (!connect_to) {
					qreal d1 = QLineF(t_dock, c1_dock).length();
					qreal d2 = QLineF(t_dock, c2_dock).length();
					if (d1 <= d2) { connect_to = c1; other = c2; }
					else { connect_to = c2; other = c1; }
				}

				//Delete the old conductor
			DiagramContent content;
			content.m_other_conductors.append(c);
			new DeleteQGraphicsItemCommand(m_diagram, content, undo_object);

				//Create new conductor from the aligned endpoint to the new terminal
			Conductor *new_c = new Conductor(connect_to, t);
			new AddGraphicsObjectCommand(new_c, m_diagram, QPointF(), undo_object);
				ConductorAutoNumerotation can(new_c, m_diagram, undo_object);
				can.numerate();
				if (m_diagram->freezeNewConductors() || m_diagram->project()->isFreezeNewConductors())
					new_c->setFreezeLabel(true);

			broken_endpoints.insert(connect_to);
			used_terminals.insert(t);

				//Also connect the 'other' endpoint to preserve bent conductor segments.
				//For L-shaped conductors (e.g. horizontal + vertical), the 'other' endpoint
				//may be far from the new element. Find the nearest free terminal of the new
				//element and create a conductor to it.
			QPointF other_dock = other->dockConductor();
			Terminal *other_terminal = nullptr;
			qreal best_dist = std::numeric_limits<qreal>::max();
			foreach (Terminal *ot, element->terminals())
			{
				if (used_terminals.contains(ot))
					continue;

				qreal dist = QLineF(ot->dockConductor(), other_dock).length();
				if (dist < best_dist) {
					best_dist = dist;
					other_terminal = ot;
				}
			}

			if (other_terminal) {
				Conductor *new_c2 = new Conductor(other, other_terminal);
				new AddGraphicsObjectCommand(new_c2, m_diagram, QPointF(), undo_object);
				ConductorAutoNumerotation can2(new_c2, m_diagram, undo_object);
				can2.numerate();
				if (m_diagram->freezeNewConductors() || m_diagram->project()->isFreezeNewConductors())
					new_c2->setFreezeLabel(true);

				broken_endpoints.insert(other);
				used_terminals.insert(other_terminal);
			}
			}
		}
	}

		//Auto-connect: collect all aligned pairs first, then filter and process.
	QList<QPair<Terminal *, Terminal *>> aligned_pairs;
	if (m_diagram->project()->autoConductor())
		aligned_pairs = element->AlignedFreeTerminals();

	for (const QPair<Terminal *, Terminal *> &pair : aligned_pairs)
	{
			//Skip if the other terminal was an endpoint of a broken conductor
		if (broken_endpoints.contains(pair.second))
			continue;

		Conductor *conductor = new Conductor(pair.first, pair.second);
		new AddGraphicsObjectCommand(conductor, m_diagram, QPointF(), undo_object);

			//Autonum the new conductor, the undo command associated for this, have for parent undo_object
		ConductorAutoNumerotation can  (conductor, m_diagram, undo_object);
		can.numerate();
		if (m_diagram->freezeNewConductors() || m_diagram->project()->isFreezeNewConductors()) {
			conductor->setFreezeLabel(true);
		}
	}
	m_diagram->addItem(m_element);

	m_diagram -> undoStack().push(undo_object);
	element->setUpFormula();
	element->freezeNewAddedElement();
}
