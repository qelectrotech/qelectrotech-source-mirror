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
#include "../autobreakconductor.h"
#include "../autoconnect.h"
#include <QGraphicsLineItem>
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
	endDrag();
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
		//Pressed and dragged: copies along the drag
	if (m_element && m_pressed && (event->buttons() & Qt::LeftButton))
	{
		const QPointF to = Diagram::snapToGrid(event->scenePos());
		if (!m_dragging && QLineF(m_press_pos, to).length() >= Diagram::xGrid)
			m_dragging = true;
		if (m_dragging) {
			updateDrag(to);
			event->setAccepted(true);
			return;
		}
	}

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
void DiagramEventAddElement::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	m_drag_cancelled = false;
	if (m_element && event->button() == Qt::LeftButton && AutoConnect::isOn(m_diagram)) {
		m_pressed = true;
		m_press_pos = Diagram::snapToGrid(event->scenePos());
	}
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
			if (m_drag_cancelled) {
					//The release of a drag given up with Escape places nothing
				m_drag_cancelled = false;
			} else if (m_dragging) {
				const QList<QPointF> positions = m_drag_positions;
				endDrag();
				addElements(positions);
			} else {
				endDrag();
				addElement();
			}
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
		if (m_dragging && m_drag_line)
			updateDrag(m_drag_line->line().p2());
		event->setAccepted(true);
	}
	else if (m_dragging && event->key() == Qt::Key_Escape)
	{
			//Escape gives up the drag, not the placing
		endDrag();
		m_drag_cancelled = true;
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
/**
	@brief DiagramEventAddElement::createElement
	@return a new copy of the element being placed, or nullptr when it
	cannot be built
*/
Element *DiagramEventAddElement::createElement() const
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
		return nullptr;
	}
	return element;
}

void DiagramEventAddElement::addElement()
{
	Element *element = createElement();
	if (!element)
		return;

		//We must add item to scene (even if addItemCommand do this)
		//for create the autoconnection below
	element -> setPos(m_element->pos());
	element -> setRotation(m_element -> rotation());
	m_diagram -> addItem(element);

	QUndoCommand *undo_object = new QUndoCommand(tr("insert %1").arg(element->name()));
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
	QList<Conductor *> conductors_handled;
	QSet<Terminal *> used_terminals;
	QSet<Terminal *> broken_endpoints = autoBreakConductors(m_diagram, element, undo_object,
							    conductors_handled, used_terminals);

		//Auto-connect: collect all aligned pairs first, then filter and process.
	QList<QPair<Terminal *, Terminal *>> aligned_pairs;
	if (AutoConnect::isOn(m_diagram))
		AutoConnect::update(m_diagram, {element}, {}, undo_object);
	else if (m_diagram->project()->autoConductor())
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

		//Autonum the new element before pushing undo_object, so the counter
		//change it triggers is part of the same undo macro as the element's
		//own placement (one Ctrl+Z reverts both, instead of silently
		//leaving the counter advanced).
	element->setUpFormula(true, undo_object);

	m_diagram -> undoStack().push(undo_object);
	element->freezeNewAddedElement();
}

/**
	@brief DiagramEventAddElement::updateDrag
	Show the drag from where the mouse was pressed to @a to, and a preview
	of each copy it places.
*/
void DiagramEventAddElement::updateDrag(const QPointF &to)
{
	if (!m_drag_line) {
		m_drag_line = new QGraphicsLineItem();
		QPen pen(Qt::blue, 0, Qt::DashLine);
		pen.setCosmetic(true);
		m_drag_line->setPen(pen);
		m_drag_line->setZValue(1e6);
		m_diagram->addItem(m_drag_line);
	}
	m_drag_line->setLine(QLineF(m_press_pos, to));
	m_element->setVisible(false);

	QSet<const Element *> ignore;
	for (Element *ghost : std::as_const(m_ghosts))
		ignore << ghost;
	m_drag_positions = AutoConnect::dragPlacements(
				m_diagram, m_element, m_drag_line->line(), ignore);

	while (m_ghosts.size() < m_drag_positions.size())
	{
		Element *ghost = createElement();
		if (!ghost)
			break;
		ghost->setFlag(QGraphicsItem::ItemIsSelectable, false);
		ghost->setOpacity(0.5);
		m_diagram->addItem(ghost);
		m_ghosts << ghost;
	}
	for (int i = 0; i < m_ghosts.size(); ++i)
	{
		Element *ghost = m_ghosts.at(i);
		ghost->setVisible(i < m_drag_positions.size());
		ghost->setRotation(m_element->rotation());
		if (i < m_drag_positions.size())
			ghost->setPos(m_drag_positions.at(i));
	}

	if (m_status_bar)
		m_status_bar->showMessage(tr("%n symbol(s) along the drag", "", int(m_drag_positions.size())));
}

/**
	@brief DiagramEventAddElement::endDrag
	Remove the drag line and the previews.
*/
void DiagramEventAddElement::endDrag()
{
	m_pressed = false;
	m_dragging = false;
	m_drag_positions.clear();
	if (m_drag_line) {
		m_diagram->removeItem(m_drag_line);
		delete m_drag_line;
		m_drag_line = nullptr;
	}
	for (Element *ghost : std::as_const(m_ghosts)) {
		m_diagram->removeItem(ghost);
		ghost->deleteLater();
	}
	m_ghosts.clear();
	if (m_element)
		m_element->setVisible(true);
}

/**
	@brief DiagramEventAddElement::addElements
	Add a copy at each of @a positions, at the current rotation, in one
	undo step; the automatic connections go with it.
*/
void DiagramEventAddElement::addElements(const QList<QPointF> &positions)
{
	if (positions.isEmpty())
		return;

	QList<Element *> added;
	QUndoCommand *undo_object = new QUndoCommand();
	for (const QPointF &pos : positions)
	{
		Element *element = createElement();
		if (!element)
			continue;
		element->setPos(pos);
		element->setRotation(m_element->rotation());
		m_diagram->addItem(element);
		new AddGraphicsObjectCommand(element, m_diagram, pos, undo_object);
		added << element;
	}
	if (added.isEmpty()) {
		delete undo_object;
		return;
	}
	undo_object->setText(tr("insert %1 × %2").arg(added.size()).arg(added.first()->name()));

		//As addElement(): the element that follows the mouse is not part
		//of the folio
	m_diagram->removeItem(m_element);
	AutoConnect::update(m_diagram, added, {}, undo_object);
	m_diagram->addItem(m_element);

	for (Element *element : std::as_const(added))
		element->setUpFormula(true, undo_object);
	m_diagram->undoStack().push(undo_object);
	for (Element *element : std::as_const(added))
		element->freezeNewAddedElement();
}
