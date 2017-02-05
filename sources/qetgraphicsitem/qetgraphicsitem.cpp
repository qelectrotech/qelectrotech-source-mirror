/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "qetgraphicsitem.h"
#include "diagram.h"

/**
 * @brief QetGraphicsItem::QetGraphicsItem Default constructor
 * @param uuid, uuid of the item
 * @param diagram, diagram aka QGraphicsScene of the item
 * @param parent, Parent Item
 */
QetGraphicsItem::QetGraphicsItem(QGraphicsItem *parent):
	QGraphicsObject(parent),
	is_movable_(true),
	first_move_(true),
	snap_to_grid_(true)
{}

QetGraphicsItem::~QetGraphicsItem()
{}

/**
 * @brief QetGraphicsItem::diagram
 *return the diagram of this item
 */
Diagram* QetGraphicsItem::diagram() const{
	return(qobject_cast<Diagram *>(scene()));
}

/**
 * @brief QetGraphicsItem::setPos
 *set the position of the item to p
 * @param p the new position of item
 */
void QetGraphicsItem::setPos(const QPointF &p) {
	QPointF pp = Diagram::snapToGrid(p);
	if (pp == pos() || !is_movable_) return;
	if (scene() && snap_to_grid_) {
		QGraphicsItem::setPos(pp);
	} else QGraphicsItem::setPos(pp);
}

/**
 * @brief QetGraphicsItem::setPos
 *set the position of the item
 * @param x new abscisse of item
 * @param y new ordonne of item
 */
void QetGraphicsItem::setPos(qreal x, qreal y) {
	setPos(QPointF(x, y));
}

/**
	Permet de tourner l'item de maniere relative.
	L'angle added_rotation est ajoute a l'orientation actuelle du image.
	@param added_rotation Angle a ajouter a la rotation actuelle
	@see applyRotation
*/
void QetGraphicsItem::rotateBy(const qreal &added_rotation) {
	qreal applied_added_rotation = QET::correctAngle(added_rotation + rotation());
	applyRotation(applied_added_rotation);
}

/**
	Effectue la rotation de l'item en lui même
	Cette methode peut toutefois etre redefinie dans des classes filles
	@param angle Angle de la rotation a effectuer
*/
void QetGraphicsItem::applyRotation(const qreal &angle) {
	setRotation(QET::correctAngle(angle));
}

/**
 * @brief QetGraphicsItem::mousePressEvent
 *handle the mouse click
 * @param e
 */
void QetGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *e)
{
	if (e -> button() == Qt::LeftButton)
	{
			//Disable views context menu
		if (scene())
			foreach (QGraphicsView *view, scene()->views())
				view->setContextMenuPolicy(Qt::NoContextMenu);

		first_move_ = true;
		if (e -> modifiers() & Qt::ControlModifier)
			setSelected(!isSelected());
	}

	QGraphicsItem::mousePressEvent(e);
}

/**
 * @brief QetGraphicsItem::mouseDoubleClickEvent
 *handle the mouse double click
 * @param e
 */
void QetGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *e) {
	Q_UNUSED (e);
	editProperty();
}

/**
 * @brief QetGraphicsItem::mouseMoveEvent
 *handle mouse movement
 * @param e
 */
void QetGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *e) {
	if (isSelected() && e -> buttons() & Qt::LeftButton) {
		//Item is moving
		if(diagram() && first_move_) {
			//It's the first movement, we signal it to parent diagram
			diagram() -> beginMoveElements(this);
		}

		//we apply the mouse movement
		QPointF old_pos = pos();
		if (first_move_) {
			mouse_to_origin_movement_ = old_pos - e -> buttonDownScenePos(Qt::LeftButton);
		}
		QPointF expected_pos = e -> scenePos() + mouse_to_origin_movement_;
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
 * @brief QetGraphicsItem::mouseReleaseEvent
 *handle mouse release click
 * @param e
 */
void QetGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *e) {
	if (diagram()) diagram() -> endMoveElements();

	if (!(e -> modifiers() & Qt::ControlModifier))
		QGraphicsItem::mouseReleaseEvent(e);

		//Enable views context menu
	if (e -> button() == Qt::LeftButton)
		if (scene())
			foreach (QGraphicsView *view, scene()->views())
				view -> setContextMenuPolicy(Qt::DefaultContextMenu);
}
