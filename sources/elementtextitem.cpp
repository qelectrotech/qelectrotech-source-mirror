/*
	Copyright 2006-2009 Xavier Guerrin
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
#include "elementtextitem.h"
#include "diagram.h"
#include "diagramcommands.h"

/**
	Constructeur
	@param parent Le QGraphicsItem parent du champ de texte
	@param scene La scene a laquelle appartient le champ de texte
*/
ElementTextItem::ElementTextItem(QGraphicsItem *parent, QGraphicsScene *scene) :
	DiagramTextItem(parent, scene),
	follow_parent_rotations(false)
{
	// par defaut, les DiagramTextItem sont Selectable et Movable
	// on desactive Movable pour les textes des elements
	setFlag(QGraphicsItem::ItemIsMovable, false);
}

/**
	Constructeur
	@param parent Le QGraphicsItem parent du champ de texte
	@param scene La scene a laquelle appartient le champ de texte
	@param text Le texte affiche par le champ de texte
*/
ElementTextItem::ElementTextItem(const QString &text, QGraphicsItem *parent, QGraphicsScene *scene) :
	DiagramTextItem(text, parent, scene),
	follow_parent_rotations(false)
{
	// par defaut, les DiagramTextItem sont Selectable et Movable
	// on desactive Movable pour les textes des elements
	setFlag(QGraphicsItem::ItemIsMovable, false);
}

/// Destructeur
ElementTextItem::~ElementTextItem() {
}

/**
	Modifie la position du champ de texte
	@param pos La nouvelle position du champ de texte
*/
void ElementTextItem::setPos(const QPointF &pos) {
	QPointF actual_pos = pos;
	actual_pos -= QPointF(0.0, boundingRect().height() / 2.0);
	DiagramTextItem::setPos(actual_pos);
}

/**
	Modifie la position du champ de texte
	@param x La nouvelle abscisse du champ de texte
	@param y La nouvelle ordonnee du champ de texte
*/
void ElementTextItem::setPos(qreal x, qreal y) {
	setPos(QPointF(x, y));
}

/**
	@return La position (bidouillee) du champ de texte
*/
QPointF ElementTextItem::pos() const {
	QPointF actual_pos = DiagramTextItem::pos();
	actual_pos += QPointF(0.0, boundingRect().height() / 2.0);
	return(actual_pos);
}

/**
	Permet de lire le texte a mettre dans le champ a partir d'un element XML.
	Cette methode se base sur la position du champ pour assigner ou non la
	valeur a ce champ.
	@param e L'element XML representant le champ de texte
*/
void ElementTextItem::fromXml(const QDomElement &e) {
	QPointF _pos = pos();
	if (qFuzzyCompare(qreal(e.attribute("x").toDouble()), _pos.x()) && qFuzzyCompare(qreal(e.attribute("y").toDouble()), _pos.y())) {
		setPlainText(e.attribute("text"));
		previous_text = e.attribute("text");
	}
}

/**
	@param document Le document XML a utiliser
	@return L'element XML representant ce champ de texte
*/
QDomElement ElementTextItem::toXml(QDomDocument &document) const {
	QDomElement result = document.createElement("input");
	result.setAttribute("x", QString("%1").arg(originalPos().x()));
	result.setAttribute("y", QString("%1").arg(originalPos().y()));
	result.setAttribute("text", toPlainText());
	return(result);
}

/**
	@param p Position originale / de reference pour ce champ
	Cette position est utilisee lors de l'export en XML
*/
void ElementTextItem::setOriginalPos(const QPointF &p) {
	original_position = p;
}

/**
	@return la position originale / de reference pour ce champ
*/
QPointF ElementTextItem::originalPos() const {
	return(original_position);
}
