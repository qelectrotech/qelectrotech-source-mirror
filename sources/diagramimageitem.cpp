/*
	Copyright 2006-2013 QElectroTech Team
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
#include "diagramimageitem.h"
#include "diagramcommands.h"
#include "qet.h"
#include "qetapp.h"

DiagramImageItem::DiagramImageItem(Diagram *parent_diagram) :
	QGraphicsPixmapItem(0, parent_diagram),
	first_move_(false)
{
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable);
#if QT_VERSION >= 0x040600
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif
	//connect(this, SIGNAL(lostFocus()), this, SLOT(setNonFocusable()));
}

/**
 * @brief DiagramImageItem::DiagramImageItem
 * @param parent
 * @param parent_diagram
 */
DiagramImageItem::DiagramImageItem(const QPixmap &pixmap, Diagram *parent_diagram) :
	QGraphicsPixmapItem(pixmap, 0, parent_diagram),
	first_move_(false)
{
	setCursor(Qt::PointingHandCursor);
	setFlags(QGraphicsItem::ItemIsSelectable|QGraphicsItem::ItemIsMovable);
#if QT_VERSION >= 0x040600
	setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
#endif
	//connect(this, SIGNAL(lostFocus()), this, SLOT(setNonFocusable()));
}

/// Destructeur
DiagramImageItem::~DiagramImageItem() {
}

/**
	@return le Diagram auquel ce image appartient, ou 0 si ce image n'est
	rattache a aucun schema
*/
Diagram *DiagramImageItem::diagram() const {
	return(qobject_cast<Diagram *>(scene()));
}

/**
	Permet de tourner le image a un angle donne de maniere absolue.
	Un angle de 0 degres correspond a un image horizontal non retourne.
	@param rotation Nouvel angle de rotation de ce image
	@see applyRotation
*/
void DiagramImageItem::setRotationAngle(const qreal &rotation_angle) {
	qreal applied_rotation = QET::correctAngle(rotation_angle);
	applyRotation(applied_rotation - rotation());
}

/**
	Permet de tourner le image de maniere relative.
	L'angle added_rotation est ajoute a l'orientation actuelle du image.
	@param added_rotation Angle a ajouter a la rotation actuelle
	@see applyRotation
*/
void DiagramImageItem::rotateBy(const qreal &added_rotation) {
	qreal applied_added_rotation = QET::correctAngle(added_rotation);
	applyRotation(applied_added_rotation);
}

/**
	Traduit en coordonnees de la scene un mouvement / vecteur initialement
	exprime en coordonnees locales.
	@param movement Vecteur exprime en coordonnees locales
	@return le meme vecteur, exprime en coordonnees de la scene
*/
QPointF DiagramImageItem::mapMovementToScene(const QPointF &movement) const {
	// on definit deux points en coordonnees locales
	QPointF local_origin(0.0, 0.0);
	QPointF local_movement_point(movement);
	
	// on les mappe sur la scene
	QPointF scene_origin(mapToScene(local_origin));
	QPointF scene_movement_point(mapToScene(local_movement_point));
	
	// on calcule le vecteur represente par ces deux points
	return(scene_movement_point - scene_origin);
}

/**
	Traduit en coordonnees locales un mouvement / vecteur initialement
	exprime en coordonnees de la scene.
	@param movement Vecteur exprime en coordonnees de la scene
	@return le meme vecteur, exprime en coordonnees locales
*/
QPointF DiagramImageItem::mapMovementFromScene(const QPointF &movement) const {
	// on definit deux points sur la scene
	QPointF scene_origin(0.0, 0.0);
	QPointF scene_movement_point(movement);
	
	// on les mappe sur ce QGraphicsItem
	QPointF local_origin(mapFromScene(scene_origin));
	QPointF local_movement_point(mapFromScene(scene_movement_point));
	
	// on calcule le vecteur represente par ces deux points
	return(local_movement_point - local_origin);
}

/**
	Traduit en coordonnees de l'item parent un mouvement / vecteur initialement
	exprime en coordonnees locales.
	@param movement Vecteur exprime en coordonnees locales
	@return le meme vecteur, exprime en coordonnees du parent
*/
QPointF DiagramImageItem::mapMovementToParent(const QPointF &movement) const {
	// on definit deux points en coordonnees locales
	QPointF local_origin(0.0, 0.0);
	QPointF local_movement_point(movement);
	
	// on les mappe sur la scene
	QPointF parent_origin(mapToParent(local_origin));
	QPointF parent_movement_point(mapToParent(local_movement_point));
	
	// on calcule le vecteur represente par ces deux points
	return(parent_movement_point - parent_origin);
}

/**
	Traduit en coordonnees locales un mouvement / vecteur initialement
	exprime en coordonnees du parent.
	@param movement Vecteur exprime en coordonnees du parent
	@return le meme vecteur, exprime en coordonnees locales
*/
QPointF DiagramImageItem::mapMovementFromParent(const QPointF &movement) const {
	// on definit deux points sur le parent
	QPointF parent_origin(0.0, 0.0);
	QPointF parent_movement_point(movement);
	
	// on les mappe sur ce QGraphicsItem
	QPointF local_origin(mapFromParent(parent_origin));
	QPointF local_movement_point(mapFromParent(parent_movement_point));
	
	// on calcule le vecteur represente par ces deux points
	return(local_movement_point - local_origin);
}

/**
	Dessine le champ de image.
	Cette methode delegue simplement le travail a QGraphicsPixmapItem::paint apres
	avoir desactive l'antialiasing.
	@param painter Le QPainter a utiliser pour dessiner le champ de image
	@param option Les options de style pour le champ de image
	@param widget Le QWidget sur lequel on dessine 
*/
void DiagramImageItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	painter -> setRenderHint(QPainter::Antialiasing, false);
	QGraphicsPixmapItem::paint(painter, option, widget);
}

/**
	Gere la prise de focus du champ de image
	@param e Objet decrivant la prise de focus
*/
void DiagramImageItem::focusInEvent(QFocusEvent *e) {
	QGraphicsPixmapItem::focusInEvent(e);
	
	// empeche le deplacement du image pendant son edition
	setFlag(QGraphicsItem::ItemIsMovable, false);
}

/**
	Gere la perte de focus du champ de image
	@param e Objet decrivant la perte de focus
*/
void DiagramImageItem::focusOutEvent(QFocusEvent *e) {
	QGraphicsPixmapItem::focusOutEvent(e);
	
	/*// deselectionne le image
	QTextCursor cursor = textCursor();
	cursor.clearSelection();
	setTextCursor(cursor);
	
	// hack a la con pour etre re-entrant
	setTextInteractionFlags(Qt::NoTextInteraction);
	
	// autorise de nouveau le deplacement du image
	setFlag(QGraphicsItem::ItemIsMovable, true);
	QTimer::singleShot(0, this, SIGNAL(lostFocus()));*/
}

/**
	Gere le clic sur le champ de texte
	@param e Objet decrivant l'evenement souris
*/
void DiagramImageItem::mousePressEvent(QGraphicsSceneMouseEvent *e) {
	first_move_ = true;
	if (e -> modifiers() & Qt::ControlModifier) {
		setSelected(!isSelected());
	}
	QGraphicsItem::mousePressEvent(e);
}

/**
	Gere les double-clics sur ce champ de image.
	@param event un QGraphicsSceneMouseEvent decrivant le double-clic
*/
void DiagramImageItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	/*if (!(textInteractionFlags() & Qt::imageditable)) {
		// rend le champ de image editable
		setTextInteractionFlags(Qt::imageditorInteraction);
		
		// edite le champ de image
		setFocus(Qt::MouseFocusReason);
	} else {
		QGraphicsPixmapItem::mouseDoubleClickEvent(event);
	}*/
}

/**
 * @brief DiagramImageItem::mouseMoveEvent
 * Gere les mouvements de souris lies a l'image
 * @param e Objet decrivant l'evenement souris
 */
void DiagramImageItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	if (isSelected() && e -> buttons() & Qt::LeftButton) {
		//Image is moving
		if(diagram()) {
			if (first_move_) {
				//It's the first movement, we signal it to parent diagram
				diagram() -> beginMoveElements(this);
			}
		}

		//we apply the mouse movement
		QPointF old_pos = pos();
		if (first_move_) {
			mouse_to_origin_movement_ = old_pos - e -> buttonDownScenePos(Qt::LeftButton);
		}
		QPointF expected_pos = e-> scenePos() + mouse_to_origin_movement_;
		setPos(expected_pos); // setPos() will snap the expected position to the grid

		//we calcul the real movement apply by setPos()
		QPointF effective_movement = pos() - old_pos;
		if (diagram()) {
			//we signal the real movement apply to diagram,
			//who he apply to other selected item
			diagram() -> continueMoveElements(effective_movement);
		}
	} else e -> ignore();

	if (first_move_) first_move_ = false;
}

/**
 * @brief DiagramImageItem::mouseReleaseEvent
 * 	Gere le relachement de souris
 *  Cette methode a ete reimplementee pour tenir a jour la liste
 *  des images à deplacer au niveau du schema.
 * @param e Objet decrivant l'evenement souris
 */
void DiagramImageItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	if (diagram()) {
		diagram() -> endMoveElements();
	}
	if (!(e -> modifiers() & Qt::ControlModifier)) {
		QGraphicsItem::mouseReleaseEvent(e);
	}
}

/**
	Effectue la rotation du image en elle-meme
	Pour les DiagramImageItem, la rotation s'effectue autour du point (0, 0).
	Cette methode peut toutefois etre redefinie dans des classes filles
	@param angle Angle de la rotation a effectuer
*/
void DiagramImageItem::applyRotation(const qreal &angle) {
	// un simple appel a QGraphicsPixmapItem::setRotation suffit
	setTransformOriginPoint(boundingRect().center());
	QGraphicsPixmapItem::setRotation(QGraphicsPixmapItem::rotation() + angle);
}

/**
	Change la position du champ de image en veillant a ce qu'il
	reste sur la grille du schema auquel il appartient.
	@param p Nouvelles coordonnees de l'element
*/
void DiagramImageItem::setPos(const QPointF &p) {
	if (p == pos()) return;
	// pas la peine de positionner sur la grille si l'element n'est pas sur un Diagram
	if (scene()) {
		// arrondit l'abscisse a 10 px pres
		int p_x = qRound(p.x() / (Diagram::xGrid * 1.0)) * Diagram::xGrid;
		// arrondit l'ordonnee a 10 px pres
		int p_y = qRound(p.y() / (Diagram::yGrid * 1.0)) * Diagram::yGrid;
		QGraphicsPixmapItem::setPos(p_x, p_y);
	} else QGraphicsPixmapItem::setPos(p);
}

/**
	Change la position du champ de image en veillant a ce que l'il
	reste sur la grille du schema auquel il appartient.
	@param x Nouvelle abscisse de l'element
	@param y Nouvelle ordonnee de l'element
*/
void DiagramImageItem::setPos(qreal x, qreal y) {
	setPos(QPointF(x, y));
}

/**
	@return la position du champ de image
*/
QPointF DiagramImageItem::pos() const {
	return(QGraphicsPixmapItem::pos());
}

/// Rend le champ de image non focusable
void DiagramImageItem::setNonFocusable() {
	setFlag(QGraphicsPixmapItem::ItemIsFocusable, false);
}

/**
 * @brief Edit the image with ....
 */
void DiagramImageItem::edit() {
	// waiting
}

/**
	Load the image from this xml element
	@param e xml element that define an image
*/
bool DiagramImageItem::fromXml(const QDomElement &e) {
	if (e.tagName() != "image") return (false);
	QDomNode image_node = e.firstChild();
	if (!image_node.isText()) return (false);

	//load xml text image to QByteArray
	QByteArray array;
	array = QByteArray::fromBase64(e.text().toAscii());

	//Set QPixmap from the @array
	QPixmap pixmap;
	pixmap.loadFromData(array);
	setPixmap(pixmap);

	setPos(e.attribute("x").toDouble(), e.attribute("y").toDouble());
	if (e.hasAttribute("rotation")) setRotationAngle(e.attribute("rotation").toDouble());

	return (true);
}

/**
	@param document Le document XML a utiliser
	@return L'element XML representant ce champ de texte
*/
QDomElement DiagramImageItem::toXml(QDomDocument &document) const {
	QDomElement result = document.createElement("image");
	//write some attribute
	result.setAttribute("x", QString("%1").arg(pos().x()));
	result.setAttribute("y", QString("%1").arg(pos().y()));
	if (rotation()) result.setAttribute("rotation", QString("%1").arg(rotation()));

	//write the pixmap in the xml element after he was been transformed to base64
	QByteArray array;
	QBuffer buffer(&array);
	buffer.open(QIODevice::ReadWrite);
	pixmap().save(&buffer, "PNG");
	QDomText base64 = document.createTextNode(array.toBase64());
	result.appendChild(base64);

	return(result);
}
