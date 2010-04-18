/*
	Copyright 2006-2010 Xavier Guerrin
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
#include "element.h"

/**
	Constructeur
	@param parent_element Le QGraphicsItem parent du champ de texte
	@param parent_diagram Le schema auquel appartient le champ de texte
*/
ElementTextItem::ElementTextItem(Element *parent_element, Diagram *parent_diagram) :
	DiagramTextItem(parent_element, parent_diagram),
	parent_element_(parent_element),
	follow_parent_rotations(false),
	original_rotation_angle_(0.0)
{
	// par defaut, les DiagramTextItem sont Selectable et Movable
	// on desactive Movable pour les textes des elements
	setFlag(QGraphicsItem::ItemIsMovable, false);
	
	// ajuste la position du QGraphicsItem lorsque le QTextDocument change
	connect(document(), SIGNAL(blockCountChanged(int)), this, SLOT(adjustItemPosition(int)));
}

/**
	Constructeur
	@param parent_element L'element parent du champ de texte
	@param parent_diagram Le schema auquel appartient le champ de texte
	@param text Le texte affiche par le champ de texte
*/
ElementTextItem::ElementTextItem(const QString &text, Element *parent_element, Diagram *parent_diagram) :
	DiagramTextItem(text, parent_element, parent_diagram),
	parent_element_(parent_element),
	follow_parent_rotations(false),
	original_rotation_angle_(0.0)
{
	// par defaut, les DiagramTextItem sont Selectable et Movable
	// on desactive Movable pour les textes des elements
	setFlag(QGraphicsItem::ItemIsMovable, false);
	
	// ajuste la position du QGraphicsItem lorsque le QTextDocument change
	connect(document(), SIGNAL(blockCountChanged(int)), this, SLOT(adjustItemPosition(int)));
}

/// Destructeur
ElementTextItem::~ElementTextItem() {
}

/**
	@return L'element parent de ce champ de texte, ou 0 si celui-ci n'en a pas.
*/
Element *ElementTextItem::parentElement() const {
	return(parent_element_);
}

/**
	Modifie la position du champ de texte
	@param pos La nouvelle position du champ de texte
*/
void ElementTextItem::setPos(const QPointF &pos) {
	// annule toute transformation (rotation notamment)
	resetTransform();
	
	// effectue le positionnement en lui-meme
	QPointF actual_pos = pos;
	actual_pos -= QPointF(0.0, boundingRect().bottom() / 2.0);
	QGraphicsTextItem::setPos(actual_pos);
	
	// applique a nouveau la rotation du champ de texte
	applyRotation(rotationAngle());
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
	actual_pos += QPointF(0.0, boundingRect().bottom() / 2.0);
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
	if (
		qFuzzyCompare(qreal(e.attribute("x").toDouble()), _pos.x()) &&
		qFuzzyCompare(qreal(e.attribute("y").toDouble()), _pos.y())
	) {
		setPlainText(e.attribute("text"));
		previous_text = e.attribute("text");
		qreal xml_rotation_angle;
		if (QET::attributeIsAReal(e, "userrotation", &xml_rotation_angle)) {
			setRotationAngle(xml_rotation_angle);
		}
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
	if (rotationAngle() != originalRotationAngle()) {
		result.setAttribute("userrotation", QString("%1").arg(rotationAngle()));
	}
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

/**
	Definit l'angle de rotation original de ce champ de texte
	@param rotation_angle un angle de rotation
*/
void ElementTextItem::setOriginalRotationAngle(const qreal &rotation_angle) {
	original_rotation_angle_ = QET::correctAngle(rotation_angle);
}

/**
	@return l'angle de rotation original de ce champ de texte
*/
qreal ElementTextItem::originalRotationAngle() const {
	return(original_rotation_angle_);
}

/**
	Cette methode s'assure que la position de l'ElementTextItem est coherente
	en repositionnant son origine (c-a-d le milieu du bord gauche du champ de
	texte) a la position originale. Cela est notamment utile lorsque le champ
	de texte est agrandi ou retreci verticalement (ajout ou retrait de lignes).
	@param new_block_count Nombre de blocs dans l'ElementTextItem
	@see originalPos()
*/
void ElementTextItem::adjustItemPosition(int new_block_count) {
	Q_UNUSED(new_block_count);
	setPos(originalPos());
}

/**
	Effetue la rotation du texte en elle-meme
	Pour les ElementTextItem, la rotation s'effectue autour du milieu du bord
	gauche du champ de texte.
	@param angle Angle de la rotation a effectuer
*/
void ElementTextItem::applyRotation(const qreal &angle) {
	qreal origin_offset = boundingRect().bottom() / 2.0;
	
	QTransform rotation;
	rotation.translate(0.0,  origin_offset);
	rotation.rotate(angle);
	rotation.translate(0.0, -origin_offset);
	
	QGraphicsTextItem::setTransform(rotation, true);
}
