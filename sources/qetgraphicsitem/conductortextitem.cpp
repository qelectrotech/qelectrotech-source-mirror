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
#include "conductortextitem.h"
#include "conductor.h"
#include "diagramcommands.h"

/**
	Constructeur
	@param parent_conductor  Conducteur auquel ce texte est rattache
	@param parent_diagram    Schema auquel ce texte et son conducteur parent sont rattaches
*/
ConductorTextItem::ConductorTextItem(Conductor *parent_conductor, Diagram *parent_diagram) :
	DiagramTextItem(parent_conductor, parent_diagram),
	parent_conductor_(parent_conductor),
	moved_by_user_(false),
	rotate_by_user_(false),
	first_move_(true)
{
	// par defaut, les DiagramTextItem sont Selectable et Movable
	// cela nous convient, on ne touche pas a ces flags
}

/**
	Constructeur
	@param text Le texte affiche par le champ de texte
	@param parent_conductor  Conducteur auquel ce texte est rattache
	@param parent_diagram    Schema auquel ce texte et son conducteur parent sont rattaches
*/
ConductorTextItem::ConductorTextItem(const QString &text, Conductor *parent_conductor, Diagram *parent_diagram) :
	DiagramTextItem(text, parent_conductor, parent_diagram),
	parent_conductor_(parent_conductor),
	moved_by_user_(false),
	rotate_by_user_(false),
	first_move_(true)
{
	// par defaut, les DiagramTextItem sont Selectable et Movable
	// cela nous convient, on ne touche pas a ces flags
}

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
	Permet de lire le texte a mettre dans le champ a partir d'un element XML.
	Cette methode se base sur la position du champ pour assigner ou non la
	valeur a ce champ.
	@param e L'element XML representant le champ de texte
*/
void ConductorTextItem::fromXml(const QDomElement &e) {
	setPlainText(e.attribute("text"));
	
	qreal user_pos_x, user_pos_y;
	if (
		QET::attributeIsAReal(e, "userx", &user_pos_x) &&
		QET::attributeIsAReal(e, "usery", &user_pos_y)
	) {
		setPos(user_pos_x, user_pos_y);
	}
	
	setRotationAngle(e.attribute("rotation").toDouble());
}

/**
	@param document Le document XML a utiliser
	@return L'element XML representant ce champ de texte
*/
QDomElement ConductorTextItem::toXml(QDomDocument &document) const {
	QDomElement result = document.createElement("input");
	result.setAttribute("userx", QString("%1").arg(pos().x()));
	result.setAttribute("usery", QString("%1").arg(pos().y()));
	result.setAttribute("text", toPlainText());
	if (rotationAngle()) {
		result.setAttribute("rotation", QString("%1").arg(rotationAngle()));
	}
	return(result);
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
		parent_conductor_ -> adjustTextItemPosition();
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
		parent_conductor_ -> adjustTextItemPosition();
	}
}

/**
	Gere les clics de souris lies au champ de texte
	@param e Objet decrivant l'evenement souris
*/
void ConductorTextItem::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	if ((flags() & QGraphicsItem::ItemIsMovable) && (e -> buttons() & Qt::LeftButton)) {
		before_mov_pos_ = pos();
	}
	first_move_ = true;
	DiagramTextItem::mousePressEvent(e);
}

/**
	Gere les mouvements de souris lies au champ de texte
	@param e Objet decrivant l'evenement souris
*/
void ConductorTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	if (textInteractionFlags() & Qt::TextEditable) {
		QGraphicsTextItem::mouseMoveEvent(e);
	} else if ((flags() & QGraphicsItem::ItemIsMovable) && (e -> buttons() & Qt::LeftButton)) {
		if (first_move_) {
			mouse_to_origin_movement_ = before_mov_pos_ - mapToParent(e -> buttonDownPos(Qt::LeftButton));
		}
		
		QPointF intended_pos = mapToParent(e -> pos()) + mouse_to_origin_movement_;
		// si ce texte est attache a un conducteur, alors ses mouvements seront
		// limites a une certaine distance du trace de ce conducteur
		if (parent_conductor_) {
			if (parent_conductor_ -> isNearConductor(intended_pos)) {
				setPos(intended_pos);
				parent_conductor_ -> setHighlighted(Conductor::Normal);
			} else {
				parent_conductor_ -> setHighlighted(Conductor::Alert);
			}
		}
		
	} else e -> ignore();
	
	if (first_move_) {
		first_move_ = false;
	}
}

/**
	Gere le relachement de souris
	Cette methode cree un objet d'annulation pour le deplacement
	@param e Objet decrivant l'evenement souris
*/
void ConductorTextItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	if (flags() & QGraphicsItem::ItemIsMovable) {
		if (Diagram *diagram_ptr = diagram()) {
			// on cree un objet d'annulation correspondant au deplacement qui s'acheve
			QPointF applied_movement = pos() - before_mov_pos_;
			
			if (!applied_movement.isNull()) {
				// on cree un objet d'annulation seulement pour ce champ de texte
				MoveConductorsTextsCommand *undo_object = new MoveConductorsTextsCommand(diagram_ptr);
				undo_object -> addTextMovement(this, before_mov_pos_, pos(), moved_by_user_);
				
				// on active le flag indiquant que ce champ de texte a ete explicitement repositionne par l'utilisateur
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
