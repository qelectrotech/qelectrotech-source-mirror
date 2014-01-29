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
#include "elementprimitivedecorator.h"
#include "elementscene.h"
#include "customelementpart.h"
#include "editorcommands.h"
#include "qet.h"
#include <QPainter>
#include <QDebug>
#include <QGraphicsSceneHoverEvent>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsScene>
#include <QTransform>

/**
	Constructor
	@param parent Parent QGraphicsItem
*/
ElementPrimitiveDecorator::ElementPrimitiveDecorator(QGraphicsItem *parent):
	QGraphicsObject(parent)
{
	init();
}

/**
	Destructor
*/
ElementPrimitiveDecorator::~ElementPrimitiveDecorator() {
}

/**
	@return the internal bouding rect, i.e. the smallest rectangle containing
	the bounding rectangle of every selected item.
*/
QRectF ElementPrimitiveDecorator::internalBoundingRect() const {
	if (!decorated_items_.count() || !scene()) return(QRectF());

	//if @decorated_items_ contain one item and if this item is a vertical or horizontal partline, apply a specific methode
	if ((decorated_items_.count() == 1) && (decorated_items_.first() -> xmlName() == "line")) {
		QRectF horto = decorated_items_.first() -> sceneGeometricRect();
		if (!horto.width() || !horto.height()) {
			return (getSceneBoundingRect(decorated_items_.first() -> toItem()));
		}
	}
	QRectF rect = decorated_items_.first() -> sceneGeometricRect();
	foreach (CustomElementPart *item, decorated_items_) {
		rect = rect.united(item -> sceneGeometricRect());
	}
	return(rect);
}
/**
	@return the outer bounds of the decorator as a rectangle.
*/
QRectF ElementPrimitiveDecorator::boundingRect() const {
	const qreal additional_margin = 2.5;
	
	QRectF rect = effective_bounding_rect_;
	rect.adjust(-additional_margin, -additional_margin, additional_margin, additional_margin);
	return(rect);
}

/**
	Paint the contents of an item in local coordinates, using \a painter, with
	respect to \a option and
	@param option The option parameter provides style options for the item, such
	as its state, exposed area and its level-of-detail hints.
	@param The widget argument is optional. If provided, it points to the
	widget that is being painted on; otherwise, it is 0. For cached painting,
	widget is always 0.
*/
void ElementPrimitiveDecorator::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	Q_UNUSED(option)
	Q_UNUSED(widget)
	painter -> save();
	
	// paint the original bounding rect
	painter -> setPen(Qt::DashLine);
	//QGraphicsItemGroup::paint(painter, option, widget);
	painter -> drawRect(modified_bounding_rect_);
	drawSquares(painter, option, widget);
	
	// uncomment to draw the real bouding rect (=adjusted internal bounding rect)
	// painter -> setBrush(QBrush(QColor(240, 0, 0, 127)));
	// painter -> drawRect(boundingRect());
	painter -> restore();
}

/**
	@param items the new list of items this decorator is suposed to manipulate.
*/
void ElementPrimitiveDecorator::setItems(const QList<CustomElementPart *> &items) {
	if (CustomElementPart *single_item = singleItem()) {
		if (items.count() == 1 && items.first() == single_item) {
			// no actual change
			goto end_setItems;
		}
		
		// break any connection between the former single selected item (if any) and
		// the decorator
		single_item -> setDecorator(0);
		if (QGraphicsObject *single_object = dynamic_cast<QGraphicsObject *>(single_item)) {
			disconnect(single_object, 0, this, 0);
		}
	}
	
	decorated_items_ = items;
	
	// when only a single primitive is selected, the decorator behaves specially
	// to enable extra features, such as text edition, internal points movements,
	// etc.
	if (CustomElementPart *single_item = singleItem()) {
		single_item -> setDecorator(this);
	}
	
	end_setItems:
	adjust();
	show();
	if (focusItem() != this) {
		setFocus();
	}
}

/**
	@param items the new list of items this decorator is suposed to manipulate.
*/
void ElementPrimitiveDecorator::setItems(const QList<QGraphicsItem *> &items) {
	QList<CustomElementPart *> primitives;
	foreach (QGraphicsItem *item, items) {
		if (CustomElementPart *part_item = dynamic_cast<CustomElementPart *>(item)) {
			primitives << part_item;
		}
	}
	setItems(primitives);
}

/**
	@return the list of items this decorator is supposed to manipulate
*/
QList<CustomElementPart *> ElementPrimitiveDecorator::items() const {
	return(decorated_items_);
}

/**
	@return the list of items this decorator is supposed to manipulate
*/
QList<QGraphicsItem *> ElementPrimitiveDecorator::graphicsItems() const {
	QList<QGraphicsItem *> list;
	foreach (CustomElementPart *part_item, decorated_items_) {
		if (QGraphicsItem *item = dynamic_cast<QGraphicsItem *>(part_item)) {
			list << item;
		}
	}
	return(list);
}

/**
	Adjust the visual decorator according to the currently assigned items.
	It is notably called by setItems().
*/
void ElementPrimitiveDecorator::adjust() {
	saveOriginalBoundingRect();
	modified_bounding_rect_ = original_bounding_rect_;
	adjustEffectiveBoundingRect();
}

/**
	Handle events generated when the mouse hovers over the decorator.
	@param event Object describing the hover event.
*/
void ElementPrimitiveDecorator::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
	QList<QRectF> rects = getResizingSquares();
	QPointF pos = event -> pos();
	
	if (rects.at(QET::ResizeFromTopLeftCorner).contains(pos) || rects.at(QET::ResizeFromBottomRightCorner).contains(pos)) {
		setCursor(Qt::SizeFDiagCursor);
	} else if (rects.at(QET::ResizeFromTopRightCorner).contains(pos) || rects.at(QET::ResizeFromBottomLeftCorner).contains(pos)) {
		setCursor(Qt::SizeBDiagCursor);
	} else if (rects.at(QET::ResizeFromTopCenterCorner).contains(pos) || rects.at(QET::ResizeFromBottomCenterCorner).contains(pos)) {
		setCursor(Qt::SizeVerCursor);
	} else if (rects.at(QET::ResizeFromMiddleLeftCorner).contains(pos) || rects.at(QET::ResizeFromMiddleRightCorner).contains(pos)) {
		setCursor(Qt::SizeHorCursor);
	} else if (internalBoundingRect().contains(pos)) {
		setCursor(Qt::SizeAllCursor);
	} else {
		setCursor(Qt::ArrowCursor);
	}
}

/**
	Handle event generated when mouse buttons are pressed.
	@param event Object describing the mouse event
*/
void ElementPrimitiveDecorator::mousePressEvent(QGraphicsSceneMouseEvent *event) {
	QList<QRectF> rects = getResizingSquares();
	QPointF pos = event -> pos();
	
	current_operation_square_ = resizingSquareAtPos(pos);
	bool accept = false;
	if (current_operation_square_ != QET::NoOperation) {
		accept = true;
	} else {
		if (internalBoundingRect().contains(pos)) {
			if (CustomElementPart *single_item = singleItem()) {
				bool event_accepted = single_item -> singleItemPressEvent(this, event);
				if (event_accepted) {
					event -> ignore();
					return;
				}
			}
			current_operation_square_ = QET::MoveArea;
			accept = true;
		}
	}
	
	if (accept) {
		if (current_operation_square_ > QET::NoOperation) {
			first_pos_ = latest_pos_ = mapToScene(rects.at(current_operation_square_).center());
		} else {
			first_pos_ = decorated_items_.at(0) -> toItem() -> scenePos();
			latest_pos_ = event -> scenePos();
			mouse_offset_ = event -> scenePos() - first_pos_;
		}
		startMovement();
		event -> accept();
	} else {
		event -> ignore();
	}
}

/**
	Handle events generated when mouse buttons are double clicked.
	@param event Object describing the mouse event
*/
void ElementPrimitiveDecorator::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	//QGraphicsObject::mouseDoubleClickEvent(event);
	if (CustomElementPart *single_item = singleItem()) {
		bool event_accepted = single_item -> singleItemDoubleClickEvent(this, event);
		if (event_accepted) {
			event -> ignore();
			return;
		}
	}
}

/**
	Handle event generated when the mouse is moved and the decorator is the mouse grabber item.
	@param event Object describing the mouse event
	@see QGraphicsScene::mouseGrabberItem()
*/
void ElementPrimitiveDecorator::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
	QList<QRectF> rects = getResizingSquares();
	
	QPointF scene_pos = event -> scenePos();
	QPointF movement = scene_pos - latest_pos_;
	
	if (current_operation_square_ > QET::NoOperation) {
		// This is a scaling operation.
		
		// For convenience purposes, we may need to adjust mouse movements.
		QET::ScalingMethod scaling_method = scalingMethod(event);
		if (scaling_method > QET::FreeScaling) {
			// real, non-rounded movement from the mouse press event
			QPointF global_movement = scene_pos - first_pos_;
			
			QPointF rounded_global_movement;
			if (scaling_method == QET::SnapScalingPointToGrid) {
				// real, rounded movement from the mouse press event
				rounded_global_movement = snapConstPointToGrid(global_movement);
			}
			else {
				QRectF new_bounding_rect = original_bounding_rect_;
				applyMovementToRect(current_operation_square_, global_movement, new_bounding_rect);
				
				const qreal scale_epsilon = 20.0; // rounds to 0.05
				QPointF delta = deltaForRoundScaling(original_bounding_rect_, new_bounding_rect, scale_epsilon);
				
				// real, rounded movement from the mouse press event
				rounded_global_movement = global_movement + delta;
			}
			
			// rounded position of the current mouse move event
			QPointF rounded_scene_pos = first_pos_ + rounded_global_movement;
			
			// when scaling the selection, consider the center of the currently dragged resizing rectangle
			QPointF current_position = mapToScene(rects.at(current_operation_square_).center());
			// determine the final, effective movement
			movement = rounded_scene_pos - current_position;
		}
	}
	else if (current_operation_square_ == QET::MoveArea) {
		// When moving the selection, consider the position of the first selected item
		QPointF current_position = scene_pos - mouse_offset_;
		QPointF rounded_current_position = snapConstPointToGrid(current_position);
		movement = rounded_current_position - decorated_items_.at(0) -> toItem() -> scenePos();
	}
	else {
		// Neither a movement nor a scaling operation -- perhaps the underlying item
		// is interested in the mouse event for custom operations?
		if (CustomElementPart *single_item = singleItem()) {
			bool event_accepted = single_item -> singleItemMoveEvent(this, event);
			if (event_accepted) {
				event -> ignore();
				return;
			}
		}
	}
	
	QRectF bounding_rect = modified_bounding_rect_;
	applyMovementToRect(current_operation_square_, movement, modified_bounding_rect_);
	if (modified_bounding_rect_ != bounding_rect) {
		adjustEffectiveBoundingRect();
	}
	latest_pos_ = event -> scenePos();
	
	if (current_operation_square_ == QET::MoveArea) {
		translateItems(movement);
	} else {
		scaleItems(original_bounding_rect_, modified_bounding_rect_);
	}
}

/**
	Handle event generated when a mouse buttons are releaseis moved and the
	decorator is the mouse grabber item.
	@param event Object describing the mouse event
	@see QGraphicsScene::mouseGrabberItem()
*/
void ElementPrimitiveDecorator::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
	Q_UNUSED(event)
	
	ElementEditionCommand *command = 0;
	if (current_operation_square_ > QET::NoOperation) {
		ScalePartsCommand *scale_command = new ScalePartsCommand();
		scale_command -> setScaledPrimitives(items());
		scale_command -> setTransformation(
			mapToScene(original_bounding_rect_).boundingRect(),
			mapToScene(modified_bounding_rect_).boundingRect()
		);
		command = scale_command;
	} else if (current_operation_square_ == QET::MoveArea) {
		QPointF movement = mapToScene(modified_bounding_rect_.topLeft()) - mapToScene(original_bounding_rect_.topLeft());
		if (!movement.isNull()) {
			MovePartsCommand *move_command = new MovePartsCommand(movement, 0, graphicsItems());
			command = move_command;
		}
	} else {
		if (CustomElementPart *single_item = singleItem()) {
			bool event_accepted = single_item -> singleItemReleaseEvent(this, event);
			if (event_accepted) {
				event -> ignore();
				return;
			}
		}
	}
	
	if (command) {
		emit(actionFinished(command));
	}
	
	if (current_operation_square_ != QET::NoOperation) {
		adjust();
	}
	
	current_operation_square_ = QET::NoOperation;
}

/**
	@reimp QGraphicsItem::keyPressEvent
*/
void ElementPrimitiveDecorator::keyPressEvent(QKeyEvent *e) {
	const qreal movement_length = 1.0;
	QPointF movement;
	switch(e -> key()) {
		case Qt::Key_Left:  movement = QPointF(-movement_length, 0.0); break;
		case Qt::Key_Right: movement = QPointF(+movement_length, 0.0); break;
		case Qt::Key_Up:    movement = QPointF(0.0, -movement_length); break;
		case Qt::Key_Down:  movement = QPointF(0.0, +movement_length); break;
	}
	if (!movement.isNull() && focusItem() == this) {
		if (!moving_by_keys_) {
			moving_by_keys_ = true;
			keys_movement_ = movement;
		} else {
			keys_movement_ += movement;
		}
		foreach(QGraphicsItem *qgi, graphicsItems()) {
			qgi -> setPos(qgi -> pos() + movement);
			adjust();
		}
	}
	
	QGraphicsObject::keyPressEvent(e);
}

/**
	@reimp QGraphicsItem::keyReleaseEvent
*/
void ElementPrimitiveDecorator::keyReleaseEvent(QKeyEvent *e) {
	// detecte le relachement d'une touche de direction ( = deplacement de parties)
	if (
		(e -> key() == Qt::Key_Left  || e -> key() == Qt::Key_Right  ||\
		 e -> key() == Qt::Key_Up    || e -> key() == Qt::Key_Down) &&\
		moving_by_keys_  && !e -> isAutoRepeat()
	) {
		// cree un objet d'annulation pour le mouvement qui vient de se finir
		emit(actionFinished(new MovePartsCommand(keys_movement_, 0, graphicsItems())));
		keys_movement_ = QPointF();
		moving_by_keys_ = false;
	}
	QGraphicsObject::keyPressEvent(e);
}

/**
	Initialize an ElementPrimitiveDecorator
*/
void ElementPrimitiveDecorator::init() {
	setFlag(QGraphicsItem::ItemIsFocusable, true);
	grid_step_x_ = grid_step_y_ = 1;
	setAcceptHoverEvents(true);
}

/**
	Save the original bounding rectangle.
*/
void ElementPrimitiveDecorator::saveOriginalBoundingRect() {
	original_bounding_rect_ = internalBoundingRect();
}

/**
	Adjust the effective bounding rect. This method should be called after the
	modified_bouding_rect_ attribute was modified.
*/
void ElementPrimitiveDecorator::adjustEffectiveBoundingRect() {
	prepareGeometryChange();
	effective_bounding_rect_ = modified_bounding_rect_ | effective_bounding_rect_;
	update();
}

/**
	Start a movement (i.e. either a move or scaling operation)
*/
void ElementPrimitiveDecorator::startMovement() {
	adjust();
	
	foreach(CustomElementPart *item, decorated_items_) {
		item -> startUserTransformation(mapToScene(original_bounding_rect_).boundingRect());
	}
}

/**
	Apply the movement described by \a movement_type and \a movement to \a rect.
*/
void ElementPrimitiveDecorator::applyMovementToRect(int movement_type, const QPointF &movement, QRectF &rect) {
	qreal new_value;
	QPointF new_point;
	
	switch (movement_type) {
		case QET::MoveArea:
			rect.translate(movement.x(), movement.y());
			break;
		case QET::ResizeFromTopLeftCorner:
			new_point = rect.topLeft() + movement;
			rect.setTopLeft(new_point);
			break;
		case QET::ResizeFromTopCenterCorner:
			new_value = rect.top() + movement.y();
			rect.setTop(new_value);
			break;
		case QET::ResizeFromTopRightCorner:
			new_point = rect.topRight() + movement;
			rect.setTopRight(new_point);
			break;
		case QET::ResizeFromMiddleLeftCorner:
			new_value = rect.left() + movement.x();
			rect.setLeft(new_value);
			break;
		case QET::ResizeFromMiddleRightCorner:
			new_value = rect.right() + movement.x();
			rect.setRight(new_value);
			break;
		case QET::ResizeFromBottomLeftCorner:
			new_point = rect.bottomLeft() + movement;
			rect.setBottomLeft(new_point);
			break;
		case QET::ResizeFromBottomCenterCorner:
			new_value = rect.bottom() + movement.y();
			rect.setBottom(new_value);
			break;
		case QET::ResizeFromBottomRightCorner:
			new_point = rect.bottomRight() + movement;
			rect.setBottomRight(new_point);
			break;
	}
}

CustomElementPart *ElementPrimitiveDecorator::singleItem() const {
	if (decorated_items_.count() == 1) {
		return(decorated_items_.first());
	}
	return(0);
}

/**
	Translated the managed items by the \a movement
*/
void ElementPrimitiveDecorator::translateItems(const QPointF &movement) {
	if (!decorated_items_.count()) return;
	
	foreach(QGraphicsItem *qgi, graphicsItems()) {
		// this is a naive, proof-of-concept implementation; we actually need to take
		// the grid into account and create a command object in mouseReleaseEvent()
		qgi -> moveBy(movement.x(), movement.y());
	}
}


/**
	Scale the managed items, provided they originally fit within \a
	original_rect and they should now fit \a new_rect
*/
void ElementPrimitiveDecorator::scaleItems(const QRectF &original_rect, const QRectF &new_rect) {
	if (!decorated_items_.count()) return;
	if (original_rect == new_rect) return;
	if (!original_rect.width() || !original_rect.height()) return; // cowardly flee division by zero FIXME?
	
	QRectF scene_original_rect = mapToScene(original_rect).boundingRect();
	QRectF scene_new_rect = mapToScene(new_rect).boundingRect();
	
	foreach(CustomElementPart *item, decorated_items_) {
		item -> handleUserTransformation(scene_original_rect, scene_new_rect);
	}
}

/**
	@return the bounding rectangle of \a item, in scene coordinates
*/
QRectF ElementPrimitiveDecorator::getSceneBoundingRect(QGraphicsItem *item) const {
	if (!item) return(QRectF());
	return(item -> mapRectToScene(item -> boundingRect()));
}

/**
	Draw all known resizing squares using \a painter.
	@param option The option parameter provides style options for the item, such
	as its state, exposed area and its level-of-detail hints.
	@param The widget argument is optional. If provided, it points to the
	widget that is being painted on; otherwise, it is 0. For cached painting,
	widget is always 0.
*/
void ElementPrimitiveDecorator::drawSquares(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	foreach (QRectF rect, getResizingSquares()) {
		drawResizeSquare(painter, option, widget, rect);
	}
}

/**
	Draw the provided resizing square \a rect using \a painter.
	@param option The option parameter provides style options for the item, such
	as its state, exposed area and its level-of-detail hints.
	@param The widget argument is optional. If provided, it points to the
	widget that is being painted on; otherwise, it is 0. For cached painting,
	widget is always 0.
*/
void ElementPrimitiveDecorator::drawResizeSquare(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget, const QRectF &rect) {
	QColor inner(0xFF, 0xFF, 0xFF);
	QColor outer(0x00, 0x61, 0xFF);
	if (decorated_items_.count() > 1) {
		outer = QColor(0x1A, 0x5C, 0x14);
	}
	drawGenericSquare(painter, option, widget, rect, inner, outer);
}

/**
	Draw a generic square \a rect using \a painter.
	@param inner Color used to fill the square
	@param outer Color usd to draw the outline
	@param option The option parameter provides style options for the item, such
	as its state, exposed area and its level-of-detail hints.
	@param The widget argument is optional. If provided, it points to the
	widget that is being painted on; otherwise, it is 0. For cached painting,
	widget is always 0.
*/
void ElementPrimitiveDecorator::drawGenericSquare(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget, const QRectF &rect, const QColor &inner, const QColor &outer) {
	Q_UNUSED(option)
	Q_UNUSED(widget)
	// 1.0px will end up to level_of_details px once rendered
	// qreal level_of_details = option->levelOfDetailFromTransform(painter -> transform());
	
	painter -> save();
	painter -> setBrush(QBrush(inner));
	QPen square_pen(QBrush(outer), 2.0, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin);
	square_pen.setCosmetic(true);
	painter -> setPen(square_pen);
	painter -> drawRect(rect);
	painter -> restore();
}

/**
	@return A list containing all known resizing squares, based on the
	modified_bounding_rect_ attribute. They are ordered following
	QET::OperationAreas, so it is possible to use positive values from this enum
	to fetch the corresponding square in the list.
	@see QET::OperationAreas
*/
QList<QRectF> ElementPrimitiveDecorator::getResizingSquares() {
	QRectF primitive_rect = modified_bounding_rect_;
	QRectF half_primitive_rect1 = primitive_rect;
	half_primitive_rect1.setHeight(half_primitive_rect1.height() / 2.0);
	QRectF half_primitive_rect2 = primitive_rect;
	half_primitive_rect2.setWidth(half_primitive_rect2.width() / 2.0);
	
	QList<QRectF> rects;
	
	rects << getGenericSquare(primitive_rect.topLeft());
	rects << getGenericSquare(half_primitive_rect2.topRight());
	rects << getGenericSquare(primitive_rect.topRight());
	rects << getGenericSquare(half_primitive_rect1.bottomLeft());
	rects << getGenericSquare(half_primitive_rect1.bottomRight());
	rects << getGenericSquare(primitive_rect.bottomLeft());
	rects << getGenericSquare(half_primitive_rect2.bottomRight());
	rects << getGenericSquare(primitive_rect.bottomRight());
	
	/// TODO cache the rects instead of calculating them again and again?
	return(rects);
}

/**
	@return the square to be drawn to represent \a position
*/
QRectF ElementPrimitiveDecorator::getGenericSquare(const QPointF &position) {
	const qreal square_half_size = 0.5;
	return(
		QRectF(
			position.x() - square_half_size,
			position.y() - square_half_size,
			square_half_size * 2.0,
			square_half_size * 2.0
		)
	);
}

/**
	@return the index of the square containing the \a position point or -1 if
	none matches.
*/
int ElementPrimitiveDecorator::resizingSquareAtPos(const QPointF &position) {
	QList<QRectF> rects = getResizingSquares();
	int current_square = QET::NoOperation;
	for (int i = 0 ; i < rects.count() ; ++ i) {
		if (rects.at(i).contains(position)) {
			current_square = i;
		}
	}
	return(current_square);
}

/**
	Receive two rects, assuming they share a common corner and current is a \a
	scaled version of \a original.
	Calculate the scale ratios implied by this assumption, round them to the
	nearest multiple of \a epsilon, then return the horizontal and vertical
	offsets to be applied in order to pass from \a current to \a original scaled
	by the rounded factors.
	This method can be used to adjust a mouse movement so that it inputs a
	round scaling operation.
*/
QPointF ElementPrimitiveDecorator::deltaForRoundScaling(const QRectF &original, const QRectF &current, qreal epsilon) {
	qreal sx = current.width()  / original.width();
	qreal sy = current.height() / original.height();
	
	qreal sx_rounded = QET::round(sx, epsilon);
	qreal sy_rounded = QET::round(sy, epsilon);
	
	QPointF delta(
		original.width()  * (sx_rounded - sx),
		original.height() * (sy_rounded - sy)
	);
	return(delta);
}

/**
	Round the coordinates of \a point so it is snapped to the grid defined by the
	grid_step_x_ and grid_step_y_ attributes.
*/
QPointF ElementPrimitiveDecorator::snapConstPointToGrid(const QPointF &point) const {
	return(
		QPointF(
			qRound(point.x() / grid_step_x_) * grid_step_x_,
			qRound(point.y() / grid_step_y_) * grid_step_y_
		)
	);
}

/**
	Round the coordinates of \a point so it is snapped to the grid defined by the
	grid_step_x_ and grid_step_y_ attributes.
*/
void ElementPrimitiveDecorator::snapPointToGrid(QPointF &point) const {
	point.rx() = qRound(point.x() / grid_step_x_) * grid_step_x_;
	point.ry() = qRound(point.y() / grid_step_y_) * grid_step_y_;
}

/**
	@return whether the current operation should take the grid into account
	according to the state of the provided \a event
*/
bool ElementPrimitiveDecorator::mustSnapToGrid(QGraphicsSceneMouseEvent *event) {
	return(!(event -> modifiers() & Qt::ControlModifier));
}

/**
	@param event Mouse event during the scale operations -- simply passed to mustSnapToGrid()
	@return the scaling method to be used for the currently decorated items.
	@see QET::ScalingMethod
	@see mustSnapToGrid()
*/
QET::ScalingMethod ElementPrimitiveDecorator::scalingMethod(QGraphicsSceneMouseEvent *event) {
	if (event && !mustSnapToGrid(event)) {
		return(QET::FreeScaling);
	}
	if (CustomElementPart *single_item = singleItem()) {
		return single_item -> preferredScalingMethod();
	}
	return QET::RoundScaleRatios;
}
