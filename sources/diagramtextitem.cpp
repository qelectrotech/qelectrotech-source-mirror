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
#include "diagramtextitem.h"
#include "diagramcommands.h"
#include "qetapp.h"

/**
	Constructeur
	@param parent Le QGraphicsItem parent du champ de texte
	@param scene La scene a laquelle appartient le champ de texte
*/
DiagramTextItem::DiagramTextItem(QGraphicsItem *parent, QGraphicsScene *scene) :
	QGraphicsTextItem(parent, scene),
	rotation_angle_(0.0)
{
	setDefaultTextColor(Qt::black);
	setFont(QETApp::diagramTextsFont());
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable);
	connect(this, SIGNAL(lostFocus()), this, SLOT(setNonFocusable()));
}

/**
	Constructeur
	@param parent Le QGraphicsItem parent du champ de texte
	@param scene La scene a laquelle appartient le champ de texte
	@param text Le texte affiche par le champ de texte
*/
DiagramTextItem::DiagramTextItem(const QString &text, QGraphicsItem *parent, QGraphicsScene *scene) :
	QGraphicsTextItem(text, parent, scene),
	previous_text(text),
	rotation_angle_(0.0)
{
	setDefaultTextColor(Qt::black);
	setFont(QETApp::diagramTextsFont());
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable);
	connect(this, SIGNAL(lostFocus()), this, SLOT(setNonFocusable()));
}

/// Destructeur
DiagramTextItem::~DiagramTextItem() {
}

/// @return le Diagram auquel ce texte appartient, ou 0 si ce texte est independant
Diagram *DiagramTextItem::diagram() const {
	return(qobject_cast<Diagram *>(scene()));
}

/**
	@return l'angle de rotation actuel de ce texte
*/
qreal DiagramTextItem::rotationAngle() const {
	return(rotation_angle_);
}

/**
	Permet de tourner le texte a un angle donne. La rotation s'effectue par
	rapport au point (0, 0) du texte.
	@param rotation Nouvel angle de rotation de ce texte
*/
void DiagramTextItem::setRotationAngle(const qreal &rotation) {
	// ramene l'angle demande entre -360.0 et +360.0 degres
	qreal applied_rotation = rotation;
	while (applied_rotation < -360.0) applied_rotation += 360.0;
	while (applied_rotation >  360.0) applied_rotation -= 360.0;
	
	rotate(applied_rotation - rotation_angle_);
	rotation_angle_ = applied_rotation;
}

/**
	Ajoute 
	@param added_rotation Angle a ajouter a la rotation actuelle
*/
void DiagramTextItem::rotateBy(const qreal &added_rotation) {
	// ramene l'angle demande entre -360.0 et +360.0 degres
	qreal applied_added_rotation = added_rotation;
	while (applied_added_rotation < 360.0) applied_added_rotation += 360.0;
	while (applied_added_rotation > 360.0) applied_added_rotation -= 360.0;
	
	rotation_angle_ += applied_added_rotation;
	rotate(applied_added_rotation);
}

/**
	gere la perte de focus du champ de texte
*/
void DiagramTextItem::focusOutEvent(QFocusEvent *e) {
	QGraphicsTextItem::focusOutEvent(e);
	// signale la modification du texte si besoin
	if (toPlainText() != previous_text) {
		emit(diagramTextChanged(this, previous_text, toPlainText()));
		previous_text = toPlainText();
	}
	
	// deselectionne le texte
	QTextCursor cursor = textCursor();
	cursor.clearSelection();
	setTextCursor(cursor);
	
	if (flags() & QGraphicsItem::ItemIsMovable) {
		// hack a la con pour etre re-entrant
		setTextInteractionFlags(Qt::NoTextInteraction);
		QTimer::singleShot(0, this, SIGNAL(lostFocus()));
	}
}

/**
	Permet de lire le texte a mettre dans le champ a partir d'un element XML.
	Cette methode se base sur la position du champ pour assigner ou non la
	valeur a ce champ.
	@param e L'element XML representant le champ de texte
*/
void DiagramTextItem::fromXml(const QDomElement &e) {
	setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	setPlainText(e.attribute("text"));
	previous_text = e.attribute("text");
	setRotationAngle(e.attribute("rotation").toDouble());
}

/**
	@param document Le document XML a utiliser
	@return L'element XML representant ce champ de texte
*/
QDomElement DiagramTextItem::toXml(QDomDocument &document) const {
	QDomElement result = document.createElement("input");
	result.setAttribute("x", QString("%1").arg(pos().x()));
	result.setAttribute("y", QString("%1").arg(pos().y()));
	result.setAttribute("text", toPlainText());
	if (rotation_angle_) {
		result.setAttribute("rotation", QString("%1").arg(rotation_angle_));
	}
	return(result);
}

/**
	Gere les double-clics sur ce champ de texte.
	@param event un QGraphicsSceneMouseEvent decrivant le double-clic
*/
void DiagramTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	if (flags() & QGraphicsItem::ItemIsMovable && !(textInteractionFlags() & Qt::TextEditable)) {
		// rend le champ de texte editable
		setTextInteractionFlags(Qt::TextEditorInteraction);
		
		// simule un clic simple, ce qui edite le champ de texte
		QGraphicsSceneMouseEvent *mouseEvent = new QGraphicsSceneMouseEvent(QEvent::GraphicsSceneMousePress);
		mouseEvent -> setAccepted(true);
		mouseEvent -> setPos(event -> pos());
		mouseEvent -> setScenePos(event -> scenePos());
		mouseEvent -> setScreenPos(event -> screenPos());
		mouseEvent -> setButtonDownPos(Qt::LeftButton, event -> buttonDownPos(Qt::LeftButton));
		mouseEvent -> setButtonDownScreenPos(Qt::LeftButton, event -> buttonDownScreenPos(Qt::LeftButton));
		mouseEvent -> setButtonDownScenePos(Qt::LeftButton, event -> buttonDownScenePos(Qt::LeftButton));
		mouseEvent -> setWidget(event -> widget());
		QGraphicsTextItem::mousePressEvent(mouseEvent);
		delete mouseEvent;
	} else {
		QGraphicsTextItem::mouseDoubleClickEvent(event);
	}
}

/**
	Gere le clic sur le champ de texte
*/
void DiagramTextItem::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	if (e -> modifiers() & Qt::ControlModifier) {
		setSelected(!isSelected());
	}
	QGraphicsTextItem::mousePressEvent(e);
}

/**
	Gere les mouvements de souris lies au champ de texte
*/
void DiagramTextItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	if (textInteractionFlags() & Qt::TextEditable) {
		QGraphicsTextItem::mouseMoveEvent(e);
	} else if ((flags() & QGraphicsItem::ItemIsMovable) && (e -> buttons() & Qt::LeftButton)) {
		QPointF oldPos = pos();
		setPos(mapToParent(e -> pos()) - matrix().map(e -> buttonDownPos(Qt::LeftButton)));
		if (Diagram *diagram_ptr = diagram()) {
			diagram_ptr -> moveElements(pos() - oldPos, this);
		}
	} else e -> ignore();
}

/**
	Gere le relachement de souris
	Cette methode a ete reimplementee pour tenir a jour la liste des elements
	et conducteurs a deplacer au niveau du schema.
*/
void DiagramTextItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	if (Diagram *diagram_ptr = diagram()) {
		if ((flags() & QGraphicsItem::ItemIsMovable) && (!diagram_ptr -> current_movement.isNull())) {
			diagram_ptr -> undoStack().push(
				new MoveElementsCommand(
					diagram_ptr,
					diagram_ptr -> selectedContent(),
					diagram_ptr -> current_movement
				)
			);
			diagram_ptr -> current_movement = QPointF();
		}
		diagram_ptr -> invalidateMovedElements();
	}
	if (!(e -> modifiers() & Qt::ControlModifier)) {
		QGraphicsTextItem::mouseReleaseEvent(e);
	}
}

/**
	Change la position du champ de texte en veillant a ce qu'il
	reste sur la grille du schema auquel il appartient.
	@param p Nouvelles coordonnees de l'element
*/
void DiagramTextItem::setPos(const QPointF &p) {
	if (p == pos()) return;
	// pas la peine de positionner sur la grille si l'element n'est pas sur un Diagram
	if (scene()) {
		// arrondit l'abscisse a 10 px pres
		int p_x = qRound(p.x() / (Diagram::xGrid * 1.0)) * Diagram::xGrid;
		// arrondit l'ordonnee a 10 px pres
		int p_y = qRound(p.y() / (Diagram::yGrid * 1.0)) * Diagram::yGrid;
		QGraphicsTextItem::setPos(p_x, p_y);
	} else QGraphicsTextItem::setPos(p);
}

/**
	Change la position du champ de texte en veillant a ce que l'il
	reste sur la grille du schema auquel il appartient.
	@param x Nouvelle abscisse de l'element
	@param y Nouvelle ordonnee de l'element
*/
void DiagramTextItem::setPos(qreal x, qreal y) {
	setPos(QPointF(x, y));
}

/// Rend le champ de texte non focusable
void DiagramTextItem::setNonFocusable() {
	setFlag(QGraphicsTextItem::ItemIsFocusable, false);
}
