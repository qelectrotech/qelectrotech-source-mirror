/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "integrationmoveelementshandler.h"
#include "elementfactory.h"
#include "diagram.h"
#include "element.h"
#include "diagramcommands.h"
#include "conductorautonumerotation.h"


/**
 * @brief DiagramEventAddElement::DiagramEventAddElement
 * Defaut constructor
 * @param location :location of diagram
 * @param diagram : diagram owner of this event
 * @param pos : first pos of item ( optional, by defaut QPointF(0,0) )
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
			m_diagram -> addItem(m_element);
			m_running = true;
		}
	}
}

/**
 * @brief DiagramEventAddElement::~DiagramEventAddElement
 * Destructor
 * Enable context menu for each view of diagram
 */
DiagramEventAddElement::~DiagramEventAddElement()
{
	if (m_element) delete m_element;
	foreach(QGraphicsView *view, m_diagram->views())
		view -> setContextMenuPolicy(Qt::DefaultContextMenu);
}

/**
 * @brief DiagramEventAddElement::mouseMoveEvent
 * Move the element to new pos of mouse
 * @param event
 * @return always true
 */
bool DiagramEventAddElement::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_element)
		m_element -> setPos(Diagram::snapToGrid(event->scenePos()));

	return true;
}

/**
 * @brief DiagramEventAddElement::mousePressEvent
 * Do nothing, but return true for not transit the event to other thing in diagram.
 * @param event
 * @return always true
 */
bool DiagramEventAddElement::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event);
	return true;
}

/**
 * @brief DiagramEventAddElement::mouseReleaseEvent
 * Right button finish this event (isRunning = false) and emit finish.
 * Left button add an element to diagram
 * @param event
 * @return  always true
 */
bool DiagramEventAddElement::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_element)
	{
		if (event->button() == Qt::RightButton)
		{
			delete m_element;
			m_element = nullptr;
			m_running = false;
			emit finish();
		}
		else if (event->button() == Qt::LeftButton)
		{
			addElement();
		}
	}

	return true;
}

/**
 * @brief DiagramEventAddElement::mouseDoubleClickEvent
 * If mouse left double clic, finish this event (isRunning = false) and emit finish
 * @param event
 * @return always true
 */
bool DiagramEventAddElement::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_element && (event -> button() == Qt::LeftButton))
	{
		delete m_element;
		m_element = nullptr;
		m_running = false;
		emit finish();
	}

	return true;
}

/**
 * @brief DiagramEventAddElement::keyPressEvent
 * Press space key rotate the element to 90° (return true)
 * else  call DiagramEventInterface::keyPressEvent(event), and return the value.
 * @param event
 * @return
 */
bool DiagramEventAddElement::keyPressEvent(QKeyEvent *event)
{
	if (m_element && event->key() == Qt::Key_Space)
	{
		m_element->rotateBy(90);
		return true;
	}

	return DiagramEventInterface::keyPressEvent(event);
}

/**
 * @brief DiagramEventAddElement::init
 * Init this event.
 */
void DiagramEventAddElement::init()
{
	foreach(QGraphicsView *view, m_diagram->views())
		view->setContextMenuPolicy(Qt::NoContextMenu);
}

/**
 * @brief DiagramEventAddElement::buildElement
 * Build the element, if the element is build successfully, we return true, otherwise false
 */
bool DiagramEventAddElement::buildElement()
{
	if (QETProject::integrateElementToProject(m_location, m_diagram -> project()))
	{
		ElementsLocation loc = m_diagram->project()->importElement(m_location);
		if (loc.exist()) {
			m_integrate_path = loc.projectCollectionPath();
		}
		else {
			qDebug() << "DiagramView::addDroppedElement : Impossible d'ajouter l'element.";
			return false;
		}

	}

	int state;
	ElementsLocation loc(m_integrate_path);
	m_element = ElementFactory::Instance() -> createElement(loc, 0, &state);
		//The creation of element failed, we delete it
	if (state) {
		delete m_element;
		return(false);
	}
		//Everything is good
	return true;
}

/**
 * @brief DiagramEventAddElement::addElement
 * Add an element at the current pos en current rotation,
 * if project autoconductor option is enable, and the element can be wired, we do it.
 */
void DiagramEventAddElement::addElement()
{
	int state;
	Element *element;
	if (m_integrate_path.isEmpty())
		element = ElementFactory::Instance() -> createElement(m_location, 0, &state);
	else
		element = ElementFactory::Instance() -> createElement(ElementsLocation(m_integrate_path), 0, &state);

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

	QUndoCommand *undo_object = new AddItemCommand<Element *>(element, m_diagram, m_element -> pos());

	while (!element -> AlignedFreeTerminals().isEmpty() && m_diagram -> project() -> autoConductor())
	{
		QPair <Terminal *, Terminal *> pair = element -> AlignedFreeTerminals().takeFirst();

		Conductor *conductor = new Conductor(pair.first, pair.second);
		conductor -> setProperties(m_diagram -> defaultConductorProperties);

		new AddItemCommand<Conductor *>(conductor, m_diagram, QPointF(), undo_object);

			//Autonum the new conductor, the undo command associated for this, have for parent undo_object
		ConductorAutoNumerotation can  (conductor, m_diagram, undo_object);
		can.numerate();
	};
	m_diagram -> undoStack().push(undo_object);
}
