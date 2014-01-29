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
#include "independenttextitem.h"
#include "diagram.h"

/**
	Constructeur
	@param parent_diagram Le schema auquel est rattache le champ de texte
*/
IndependentTextItem::IndependentTextItem(Diagram *parent_diagram) :
	DiagramTextItem(0, parent_diagram),
	first_move_(true)
{
}

/**
	Constructeur
	@param text Le texte affiche par le champ de texte
	@param parent_diagram Le schema auquel est rattache le champ de texte
*/
IndependentTextItem::IndependentTextItem(const QString &text, Diagram *parent_diagram) :
	DiagramTextItem(text, 0, parent_diagram),
	first_move_(true)
{
}

/// Destructeur
IndependentTextItem::~IndependentTextItem() {
}

/**
	Permet de lire le texte a mettre dans le champ a partir d'un element XML.
	Cette methode se base sur la position du champ pour assigner ou non la
	valeur a ce champ.
	@param e L'element XML representant le champ de texte
*/
void IndependentTextItem::fromXml(const QDomElement &e) {
	setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	setHtml(e.attribute("text"));
	setRotationAngle(e.attribute("rotation").toDouble());
}

/**
	@param document Le document XML a utiliser
	@return L'element XML representant ce champ de texte
*/
QDomElement IndependentTextItem::toXml(QDomDocument &document) const {
	QDomElement result = document.createElement("input");
	result.setAttribute("x", QString("%1").arg(pos().x()));
	result.setAttribute("y", QString("%1").arg(pos().y()));
	result.setAttribute("text", toHtml());
	if (rotationAngle()) {
		result.setAttribute("rotation", QString("%1").arg(rotationAngle()));
	}
	return(result);
}

/**
	Gere le clic sur le champ de texte
	@param e Objet decrivant l'evenement souris
*/
void IndependentTextItem::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	first_move_ = true;
	if (e -> modifiers() & Qt::ControlModifier) {
		setSelected(!isSelected());
	}
	DiagramTextItem::mousePressEvent(e);
}

/**
	Gere les mouvements de souris lies au champ de texte
	@param e Objet decrivant l'evenement souris
*/
void IndependentTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	if (textInteractionFlags() & Qt::TextEditable) {
		DiagramTextItem::mouseMoveEvent(e);
	} else if ((flags() & QGraphicsItem::ItemIsMovable) && isSelected() && (e -> buttons() & Qt::LeftButton)) {
		// le champ de texte est en train d'etre deplace
		Diagram *diagram_ptr = diagram();
		if (diagram_ptr) {
			if (first_move_) {
				// il s'agit du premier mouvement du deplacement, on le signale
				// au schema parent
				diagram_ptr -> beginMoveElements(this);
			}
		}
		
		// on applique le mouvement impose par la souris
		QPointF old_pos = pos();
		if (first_move_) {
			mouse_to_origin_movement_ = old_pos - e -> buttonDownScenePos(Qt::LeftButton);
		}
		QPointF expected_pos = e-> scenePos() + mouse_to_origin_movement_;
		setPos(expected_pos); // setPos() will snap the expected position to the grid
		
		// on calcule le mouvement reellement applique par setPos()
		QPointF effective_movement = pos() - old_pos;
		if (diagram_ptr) {
			// on signale le mouvement ainsi applique au schema parent, qui
			// l'appliquera aux autres items selectionnes selon son bon vouloir
			diagram_ptr -> continueMoveElements(effective_movement);
		}
	} else e -> ignore();
	
	if (first_move_) {
		first_move_ = false;
	}
}

/**
	Gere le relachement de souris
	Cette methode a ete reimplementee pour tenir a jour la liste des elements
	et conducteurs a deplacer au niveau du schema.
	@param e Objet decrivant l'evenement souris
*/
void IndependentTextItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	if (flags() & QGraphicsItem::ItemIsMovable) {
		if (Diagram *diagram_ptr = diagram()) {
			diagram_ptr -> endMoveElements();
		}
	}
	if (!(e -> modifiers() & Qt::ControlModifier)) {
		DiagramTextItem::mouseReleaseEvent(e);
	}
}
