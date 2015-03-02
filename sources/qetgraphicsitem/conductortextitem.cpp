/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "conductortextitem.h"
#include "conductor.h"
#include "diagramcommands.h"
#include "diagram.h"

/**
	Constructeur
	@param parent_conductor  Conducteur auquel ce texte est rattache
	@param parent_diagram    Schema auquel ce texte et son conducteur parent sont rattaches
*/
ConductorTextItem::ConductorTextItem(Conductor *parent_conductor) :
	DiagramTextItem(parent_conductor),
	parent_conductor_(parent_conductor),
	moved_by_user_(false),
	rotate_by_user_(false)
{
	setAcceptHoverEvents(true);
}

/**
	Constructeur
	@param text Le texte affiche par le champ de texte
	@param parent_conductor  Conducteur auquel ce texte est rattache
	@param parent_diagram    Schema auquel ce texte et son conducteur parent sont rattaches
*/
ConductorTextItem::ConductorTextItem(const QString &text, Conductor *parent_conductor) :
	DiagramTextItem(text, parent_conductor),
	parent_conductor_(parent_conductor),
	moved_by_user_(false),
	rotate_by_user_(false)
{}

/**
	Destructeur
*/
ConductorTextItem::~ConductorTextItem() {
}

/**
	@return le conducteur parent de ce champ de texte, ou 0 si celui-ci n'en a
	pas
*/
Conductor *ConductorTextItem::parentConductor() const {
	return(parent_conductor_);
}

/**
 * @brief ConductorTextItem::fromXml
 * Read the properties stored in the xml element given in parameter
 * @param e
 */
void ConductorTextItem::fromXml(const QDomElement &e) {
	if (e.hasAttribute("userx")) {
		setPos(e.attribute("userx").toDouble(),
			   e.attribute("usery").toDouble());
		moved_by_user_ = true;
	}
	if (e.hasAttribute("rotation")) {
		setRotation(e.attribute("rotation").toDouble());
		rotate_by_user_ = true;
	}
}

/**
 * @brief ConductorTextItem::toXml
 * Export the properties of this text in the attribute of the xml element given in parameter
 * The properties exported are position and rotation (only if moved or rotate by user)
 * @param xml
 */
void ConductorTextItem::toXml(QDomElement &xml) const {
	if (moved_by_user_) {
		xml.setAttribute("userx", QString("%1").arg(pos().x()));
		xml.setAttribute("usery", QString("%1").arg(pos().y()));
	}
	if (rotate_by_user_) {
		xml.setAttribute("rotation", QString("%1").arg(rotation()));
	}
}

/**
	@return true si ce champ de texte a ete explictement deplace par
	l'utilisateur, false sinon
*/
bool ConductorTextItem::wasMovedByUser() const {
	return(moved_by_user_);
}

/**
 * @brief ConductorTextItem::wasRotateByUser
 * @return true if text was explicit moved by user else false
 */
bool ConductorTextItem::wasRotateByUser() const {
	return(rotate_by_user_);
}

/**
	@param moved_by_user true pour que la position du texte soit consideree
	comme ayant ete definie par l'utilisateur (et donc soit sauvegardee), false
	pour remettre le texte a sa position originelle
*/
void ConductorTextItem::forceMovedByUser(bool moved_by_user) {
	if (moved_by_user == moved_by_user_) return;
	
	moved_by_user_ = moved_by_user;
	if (!moved_by_user && parent_conductor_) {
		parent_conductor_ -> calculateTextItemPosition();
	}
	
}

/**
 * @brief ConductorTextItem::forceRotateByUser
 * @param rotate_by_user true pour que la rotation du texte soit consideree
	comme ayant ete definie par l'utilisateur (et donc soit sauvegardee), false
	pour remettre le texte a sont angle originelle
 */
void ConductorTextItem::forceRotateByUser(bool rotate_by_user) {
	if (rotate_by_user == rotate_by_user_) return;

	rotate_by_user_ = rotate_by_user;
	if (!rotate_by_user && parent_conductor_) {
		parent_conductor_ -> calculateTextItemPosition();
	}
}

/**
 * @brief ConductorTextItem::mousePressEvent
 * @param event
 */
void ConductorTextItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	before_mov_pos_ = pos();
	DiagramTextItem::mousePressEvent(event);
}

/**
 * @brief ConductorTextItem::mouseMoveEvent
 * @param event
 */
void ConductorTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	if (textInteractionFlags() & Qt::TextEditable) QGraphicsTextItem::mouseMoveEvent(event);

	else if ((flags() & QGraphicsItem::ItemIsMovable) && (event -> buttons() & Qt::LeftButton)) {

		QPointF intended_pos = event ->scenePos() + m_mouse_to_origin_movement;

		if (parent_conductor_) {
			if (parent_conductor_ -> isNearConductor(intended_pos)) {
				event->modifiers() == Qt::ControlModifier ? setPos(intended_pos) : setPos(Diagram::snapToGrid(intended_pos));
				parent_conductor_ -> setHighlighted(Conductor::Normal);
			} else {
				parent_conductor_ -> setHighlighted(Conductor::Alert);
			}
		}
		
	}

	else event -> ignore();
}

/**
 * @brief ConductorTextItem::mouseReleaseEvent
 * @param e
 */
void ConductorTextItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	if (flags() & QGraphicsItem::ItemIsMovable) {

		if (Diagram *diagram_ptr = diagram()) {
			QPointF applied_movement = pos() - before_mov_pos_;
			
			if (!applied_movement.isNull()) {
				//Create an undo object
				MoveConductorsTextsCommand *undo_object = new MoveConductorsTextsCommand(diagram_ptr);
				undo_object -> addTextMovement(this, before_mov_pos_, pos(), moved_by_user_);

				moved_by_user_ = true;
				
				diagram_ptr -> undoStack().push(undo_object);
			}
			
			if (parent_conductor_) {
				parent_conductor_ -> setHighlighted(Conductor::None);
			}
		}
	}
	if (!(e -> modifiers() & Qt::ControlModifier)) {
		QGraphicsTextItem::mouseReleaseEvent(e);
	}
}



/**
	When mouse over element
	change m_mouse_hover to true   (used in paint() function )
	@param e  QGraphicsSceneHoverEvent
*/
void ConductorTextItem::hoverEnterEvent(QGraphicsSceneHoverEvent *e) {
	Q_UNUSED(e);

	m_mouse_hover = true;
	QString str_ToolTip = toPlainText();
	setToolTip( str_ToolTip );
	update();
}

/**
	When mouse over element leave the position
	change m_mouse_hover to false(used in paint() function )
	@param e  QGraphicsSceneHoverEvent
*/
void ConductorTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *e) {
	Q_UNUSED(e);
	//qDebug() << "Leave mouse over";
	m_mouse_hover = false;
	update();
}

/**
	Do nothing default function .
	@param e  QGraphicsSceneHoverEvent
*/
void ConductorTextItem::hoverMoveEvent(QGraphicsSceneHoverEvent *e) {
	Q_UNUSED(e);
	QGraphicsTextItem::hoverMoveEvent(e);
}
