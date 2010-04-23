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
#include "diagramtextitem.h"
#include "diagramcommands.h"
#include "qet.h"
#include "qetapp.h"

/**
	Constructeur
	@param parent Le QGraphicsItem parent du champ de texte
	@param parent_diagram Le schema auquel appartient le champ de texte
*/
DiagramTextItem::DiagramTextItem(QGraphicsItem *parent, Diagram *parent_diagram) :
	QGraphicsTextItem(parent, parent_diagram),
	parent_diagram_(parent_diagram),
	previous_text_(),
	rotation_angle_(0.0)
{
	setDefaultTextColor(Qt::black);
	setFont(QETApp::diagramTextsFont());
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable);
	connect(this, SIGNAL(lostFocus()), this, SLOT(setNonFocusable()));
}

/**
	Constructeur
	@param text Le texte affiche par le champ de texte
	@param parent Le QGraphicsItem parent du champ de texte
	@param parent_diagram Le schema auquel appartient le champ de texte
*/
DiagramTextItem::DiagramTextItem(const QString &text, QGraphicsItem *parent, Diagram *parent_diagram) :
	QGraphicsTextItem(text, parent, parent_diagram),
	parent_diagram_(parent_diagram),
	previous_text_(text),
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

/**
	@return le Diagram auquel ce texte appartient, ou 0 si ce texte n'est
	rattache a aucun schema
*/
Diagram *DiagramTextItem::diagram() const {
	return(parent_diagram_);
}

/**
	@return l'angle de rotation actuel de ce texte
*/
qreal DiagramTextItem::rotationAngle() const {
	return(rotation_angle_);
}

/**
	Permet de tourner le texte a un angle donne de maniere absolue.
	Un angle de 0 degres correspond a un texte horizontal non retourne.
	@param rotation Nouvel angle de rotation de ce texte
	@see applyRotation
*/
void DiagramTextItem::setRotationAngle(const qreal &rotation) {
	qreal applied_rotation = QET::correctAngle(rotation);
	applyRotation(applied_rotation - rotation_angle_);
	rotation_angle_ = applied_rotation;
}

/**
	Permet de tourner le texte de maniere relative.
	L'angle added_rotation est ajoute a l'orientation actuelle du texte.
	@param added_rotation Angle a ajouter a la rotation actuelle
	@see applyRotation
*/
void DiagramTextItem::rotateBy(const qreal &added_rotation) {
	qreal applied_added_rotation = QET::correctAngle(added_rotation);
	rotation_angle_ = QET::correctAngle(rotation_angle_ + applied_added_rotation);
	applyRotation(applied_added_rotation);
}

/**
	Gere les changements dont ce champ de texte est informe
*/
QVariant DiagramTextItem::itemChange(GraphicsItemChange change, const QVariant &value) {
	if (change == QGraphicsItem::ItemSceneHasChanged) {
		QGraphicsScene *qgscene = value.value<QGraphicsScene *>();
		parent_diagram_ = static_cast<Diagram *>(qgscene);
	}
	return(QGraphicsTextItem::itemChange(change, value));
}

/**
	Gere la prise de focus du champ de texte
*/
void DiagramTextItem::focusInEvent(QFocusEvent *e) {
	QGraphicsTextItem::focusInEvent(e);
	
	// memorise le texte avant que l'utilisateur n'y touche
	previous_text_ = toPlainText();
	// cela permettra de determiner si l'utilisateur a modifie le texte a la fin de l'edition
}

/**
	Gere la perte de focus du champ de texte
*/
void DiagramTextItem::focusOutEvent(QFocusEvent *e) {
	QGraphicsTextItem::focusOutEvent(e);
	
	// signale la modification du texte si besoin
	if (toPlainText() != previous_text_) {
		emit(diagramTextChanged(this, previous_text_, toPlainText()));
		previous_text_ = toPlainText();
	}
	
	// deselectionne le texte
	QTextCursor cursor = textCursor();
	cursor.clearSelection();
	setTextCursor(cursor);
	
	// hack a la con pour etre re-entrant
	setTextInteractionFlags(Qt::NoTextInteraction);
	QTimer::singleShot(0, this, SIGNAL(lostFocus()));
}

/**
	Gere les double-clics sur ce champ de texte.
	@param event un QGraphicsSceneMouseEvent decrivant le double-clic
*/
void DiagramTextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	if (!(textInteractionFlags() & Qt::TextEditable)) {
		// rend le champ de texte editable
		setTextInteractionFlags(Qt::TextEditorInteraction);
		
		// edite le champ de texte
		setFocus(Qt::MouseFocusReason);
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
	Effetue la rotation du texte en elle-meme
	Pour les DiagramTextItem, la rotation s'effectue autour du point (0, 0).
	Cette methode peut toutefois etre redefinie dans des classes 
	@param angle Angle de la rotation a effectuer
*/
void DiagramTextItem::applyRotation(const qreal &angle) {
	// un simple appel a QGraphicsTextItem::rotate suffit
	QGraphicsTextItem::rotate(angle);
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

/**
	@return la position du champ de texte
*/
QPointF DiagramTextItem::pos() const {
	return(QGraphicsTextItem::pos());
}

/// Rend le champ de texte non focusable
void DiagramTextItem::setNonFocusable() {
	setFlag(QGraphicsTextItem::ItemIsFocusable, false);
}
