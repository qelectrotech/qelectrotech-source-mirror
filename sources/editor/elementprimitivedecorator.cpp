/*
	Copyright 2006-2021 The QElectroTech Team
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

#include "../QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "../editor/graphicspart/customelementpart.h"
#include "../qet.h"
#include "editorcommands.h"
#include "elementscene.h"

#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

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
ElementPrimitiveDecorator::~ElementPrimitiveDecorator()
{
	removeHandler();
}

/**
	@return the internal bounding rect, i.e. the smallest rectangle containing
	the bounding rectangle of every selected item.
*/
QRectF ElementPrimitiveDecorator::internalBoundingRect() const
{
	if (!decorated_items_.count() || !scene()) return(QRectF());

	//if @decorated_items_ contains one item and if this item is a vertical or horizontal partline, apply a specific method
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
QRectF ElementPrimitiveDecorator::boundingRect() const
{
	return effective_bounding_rect_;
}

/**
	@brief ElementPrimitiveDecorator::paint
	Paint the contents of an item in local coordinates, using \a painter,
	with respect to \a option and
	@param painter :
	@param option :
	The option parameter provides style options for the item, such
	as its state, exposed area and its level-of-detail hints.
	@param widget :
	The widget argument is optional. If provided,
	it points to the widget that is being painted on; otherwise, it is 0.
	For cached painting, widget is always 0.
*/
void ElementPrimitiveDecorator::paint(QPainter *painter,
				      const QStyleOptionGraphicsItem *option,
				      QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)
	painter -> save();
	
		// paint the original bounding rect
	QPen pen(Qt::DashLine);
	pen.setCosmetic(true);
	painter -> setPen(pen);
	painter -> drawRect(modified_bounding_rect_);
	
	// uncomment to draw the real bounding rect (=adjusted internal bounding rect)
	// painter -> setBrush(QBrush(QColor(240, 0, 0, 127)));
	// painter -> drawRect(boundingRect());
	painter -> restore();
}

/**
	@param items the new list of items this decorator is suposed to manipulate.
*/
void ElementPrimitiveDecorator::setItems(const QList<CustomElementPart *> &items)
{
	decorated_items_ = items;

	adjust();
	show();
	if (focusItem() != this) {
		setFocus();
	}
	adjusteHandlerPos();
}

/**
	@param items the new list of items this decorator is suposed to manipulate.
*/
void ElementPrimitiveDecorator::setItems(const QList<QGraphicsItem *> &items)
{
	QList<CustomElementPart *> primitives;
	for(QGraphicsItem *item : items)
	{
		if (CustomElementPart *part_item = dynamic_cast<CustomElementPart *>(item))
		{
			primitives << part_item;
		}
	}
	setItems(primitives);
}

/**
	@return the list of items this decorator is supposed to manipulate
*/
QList<CustomElementPart *> ElementPrimitiveDecorator::items() const
{
	return(decorated_items_);
}

/**
	@return the list of items this decorator is supposed to manipulate
*/
QList<QGraphicsItem *> ElementPrimitiveDecorator::graphicsItems() const
{
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
void ElementPrimitiveDecorator::adjust()
{
	saveOriginalBoundingRect();
	modified_bounding_rect_ = original_bounding_rect_;
	adjustEffectiveBoundingRect();
}

/**
	Handle event generated when mouse buttons are pressed.
	@param event Object describing the mouse event
*/
void ElementPrimitiveDecorator::mousePressEvent(QGraphicsSceneMouseEvent *event)
{	
	if (internalBoundingRect().contains(event->pos()))
	{
		current_operation_square_ = QET::MoveArea;
		
		first_pos_ = decorated_items_.at(0) -> toItem() -> scenePos();
		latest_pos_ = event -> scenePos();
		mouse_offset_ = event -> scenePos() - first_pos_;
		startMovement();
		event->accept();
	}
	else
		event->ignore();
}

/**
	Handle event generated when the mouse is moved and the decorator is the mouse grabber item.
	@param event Object describing the mouse event
	@see QGraphicsScene::mouseGrabberItem()
*/
void ElementPrimitiveDecorator::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QPointF scene_pos = event -> scenePos();
	QPointF movement = scene_pos - latest_pos_;
	
	if (current_operation_square_ == QET::MoveArea)
	{
		// When moving the selection, consider the position of the first selected item
		QPointF current_position = scene_pos - mouse_offset_;
		QPointF rounded_current_position = snapConstPointToGrid(current_position);
		movement = rounded_current_position - decorated_items_.at(0) -> toItem() -> scenePos();
		
		QRectF bounding_rect = modified_bounding_rect_;
		applyMovementToRect(current_operation_square_, movement, modified_bounding_rect_);
		if (modified_bounding_rect_ != bounding_rect) {
			adjustEffectiveBoundingRect();
		}
		latest_pos_ = event -> scenePos();
		translateItems(movement);
	}
	

	

}

/**
	Handle event generated when a mouse buttons are releaseis moved and the
	decorator is the mouse grabber item.
	@param event Object describing the mouse event
	@see QGraphicsScene::mouseGrabberItem()
*/
void ElementPrimitiveDecorator::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event)
	
	ElementEditionCommand *command = nullptr;

	if (current_operation_square_ == QET::MoveArea)
	{
		QPointF movement = mapToScene(modified_bounding_rect_.topLeft()) - mapToScene(original_bounding_rect_.topLeft());
		if (!movement.isNull())
		{
			MovePartsCommand *move_command = new MovePartsCommand(movement, nullptr, graphicsItems());
			command = move_command;
		}
		
		if (command) {
			emit(actionFinished(command));
		}
		
		adjust();
	}
	
	current_operation_square_ = QET::NoOperation;
}

/**
	@brief ElementPrimitiveDecorator::keyPressEvent
	@param e
	@see QGraphicsItem::keyPressEvent
*/
void ElementPrimitiveDecorator::keyPressEvent(QKeyEvent *e)
{
	const qreal movement_length = 1.0;
	QPointF movement;
	
	if (e->modifiers() & Qt::ControlModifier) {
	switch(e -> key())
	{
		case Qt::Key_Left:  movement = QPointF(-movement_length / 10, 0.0); break;
		case Qt::Key_Right: movement = QPointF(+movement_length / 10, 0.0); break;
		case Qt::Key_Up:    movement = QPointF(0.0, -movement_length / 10); break;
		case Qt::Key_Down:  movement = QPointF(0.0, +movement_length / 10); break;
	}
	}
	else {
		switch(e -> key())
	{
		case Qt::Key_Left:  movement = QPointF(-movement_length, 0.0); break;
		case Qt::Key_Right: movement = QPointF(+movement_length, 0.0); break;
		case Qt::Key_Up:    movement = QPointF(0.0, -movement_length); break;
		case Qt::Key_Down:  movement = QPointF(0.0, +movement_length); break;
	}
	}
	if (!movement.isNull() && focusItem() == this)
	{
		if (!moving_by_keys_)
		{
			moving_by_keys_ = true;
			keys_movement_ = movement;
		}
		else
		{
			keys_movement_ += movement;
		}
		
		for(QGraphicsItem *qgi : graphicsItems())
		{
			qgi -> setPos(qgi -> pos() + movement);
			adjust();
		}
		
		e->accept();
		return;
	}
	
	QGraphicsObject::keyPressEvent(e);
}

/**
	@brief ElementPrimitiveDecorator::keyReleaseEvent
	@param e
	@see QGraphicsItem::keyReleaseEvent
*/
void ElementPrimitiveDecorator::keyReleaseEvent(QKeyEvent *e) {
	// detecte le relachement d'une touche de direction ( = deplacement de parties)
	if(
		(e -> key() == Qt::Key_Left  || e -> key() == Qt::Key_Right  ||\
		 e -> key() == Qt::Key_Up    || e -> key() == Qt::Key_Down) &&\
		moving_by_keys_  && !e -> isAutoRepeat()
	) {
		// cree un objet d'annulation pour le mouvement qui vient de se finir
		emit(actionFinished(new MovePartsCommand(keys_movement_, nullptr, graphicsItems())));
		keys_movement_ = QPointF();
		moving_by_keys_ = false;
		e->accept();
		return;
	}
	QGraphicsObject::keyPressEvent(e);
}

/**
	Initialize an ElementPrimitiveDecorator
*/
void ElementPrimitiveDecorator::init()
{
	setFlag(QGraphicsItem::ItemIsFocusable, true);
	grid_step_x_ = grid_step_y_ = 1;
	setAcceptHoverEvents(true);
}

/**
	Save the original bounding rectangle.
*/
void ElementPrimitiveDecorator::saveOriginalBoundingRect()
{
	original_bounding_rect_ = internalBoundingRect();
}

/**
	Adjust the effective bounding rect. This method should be called after the
	modified_bounding_rect_ attribute was modified.
*/
void ElementPrimitiveDecorator::adjustEffectiveBoundingRect()
{
	prepareGeometryChange();
	effective_bounding_rect_ = modified_bounding_rect_ | effective_bounding_rect_;
	update();
	adjusteHandlerPos();
}

/**
	Start a movement (i.e. either a move or scaling operation)
*/
void ElementPrimitiveDecorator::startMovement()
{
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

CustomElementPart *ElementPrimitiveDecorator::singleItem() const
{
	if (decorated_items_.count() == 1) {
		return(decorated_items_.first());
	}
	return(nullptr);
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
QRectF ElementPrimitiveDecorator::getSceneBoundingRect(QGraphicsItem *item) const
{
	if (!item) return(QRectF());
	return(item -> mapRectToScene(item -> boundingRect()));
}

QVector<QPointF> ElementPrimitiveDecorator::getResizingsPoints() const
{
	QRectF primitive_rect = modified_bounding_rect_;
	QVector <QPointF> vector;
	QPointF half;

	vector << primitive_rect.topLeft(); //top left
	half = primitive_rect.center();
	half.setY(primitive_rect.top());
	vector << half; //middle top
	vector << primitive_rect.topRight(); //top right
	half = primitive_rect.center();
	half.setX(primitive_rect.left());
	vector << half; //middle left
	half = primitive_rect.center();
	half.setX(primitive_rect.right());
	vector << half; //middle right
	vector << primitive_rect.bottomLeft(); //bottom left
	half = primitive_rect.center();
	half.setY(primitive_rect.bottom());
	vector << half; //middle bottom
	vector << primitive_rect.bottomRight(); //bottom right

	return vector;
}

/**
	@brief ElementPrimitiveDecorator::adjusteHandlerPos
*/
void ElementPrimitiveDecorator::adjusteHandlerPos()
{
	QVector <QPointF> points_vector = mapToScene(getResizingsPoints());
	for (int i = 0 ; i < points_vector.size() ; ++i)
		m_handler_vector.at(i)->setPos(points_vector.at(i));
}

/**
	@brief ElementPrimitiveDecorator::handlerMousePressEvent
	@param qghi
	@param event
*/
void ElementPrimitiveDecorator::handlerMousePressEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(event);
	
	QVector <QPointF> points = getResizingsPoints();
	
	current_operation_square_ = m_handler_vector.indexOf(qghi);
	
	first_pos_ = latest_pos_ = mapToScene(points.at(current_operation_square_));
	startMovement();
}

/**
	@brief ElementPrimitiveDecorator::handlerMouseMoveEvent
	@param qghi
	@param event
*/
void ElementPrimitiveDecorator::handlerMouseMoveEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi);
	
	QPointF scene_pos = event -> scenePos();
	QPointF movement = scene_pos - latest_pos_;
	
		// For convenience purposes, we may need to adjust mouse movements.
	QET::ScalingMethod scaling_method = scalingMethod(event);
	if (scaling_method > QET::FreeScaling)
	{
			// real, non-rounded movement from the mouse press event
		QPointF global_movement = scene_pos - first_pos_;
		
		QPointF rounded_global_movement;
		if (scaling_method == QET::SnapScalingPointToGrid)
		{
				// real, rounded movement from the mouse press event
			rounded_global_movement = snapConstPointToGrid(global_movement);
		}
		else
		{
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
		QPointF current_position = mapToScene(getResizingsPoints().at(current_operation_square_));
			// determine the final, effective movement
		movement = rounded_scene_pos - current_position;
	}
	
	QRectF bounding_rect = modified_bounding_rect_;
	applyMovementToRect(current_operation_square_, movement, modified_bounding_rect_);
	if (modified_bounding_rect_ != bounding_rect) {
		adjustEffectiveBoundingRect();
	}
	latest_pos_ = event -> scenePos();
	scaleItems(original_bounding_rect_, modified_bounding_rect_);
}

/**
	@brief ElementPrimitiveDecorator::handlerMouseReleaseEvent
	@param qghi
	@param event
*/
void ElementPrimitiveDecorator::handlerMouseReleaseEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi);
	Q_UNUSED(event);
	
	ElementEditionCommand *command = nullptr;
	if (current_operation_square_ > QET::NoOperation)
	{
		ScalePartsCommand *scale_command = new ScalePartsCommand();
		scale_command -> setScaledPrimitives(items());
		scale_command -> setTransformation(
					mapToScene(original_bounding_rect_).boundingRect(),
					mapToScene(modified_bounding_rect_).boundingRect()
					);
		command = scale_command;
	}
	
	if (command) {
		emit(actionFinished(command));
	}
	
	adjust();
	
	current_operation_square_ = QET::NoOperation;
}

/**
	@brief ElementPrimitiveDecorator::addHandler
	Add handlers for this item
*/
void ElementPrimitiveDecorator::addHandler()
{
	if (m_handler_vector.isEmpty() && scene())
	 {
		m_handler_vector = QetGraphicsHandlerItem::handlerForPoint(mapFromScene(getResizingsPoints()));
		 
		 for(QetGraphicsHandlerItem *handler : m_handler_vector)
		 { 
			 scene()->addItem(handler);
			 handler->setColor(Qt::darkGreen);
			 handler->installSceneEventFilter(this);
			 handler->setZValue(this->zValue()+1);
		 }
	 }
}

/**
	@brief ElementPrimitiveDecorator::removeHandler
	Remove the handlers of this item
*/
void ElementPrimitiveDecorator::removeHandler()
{
	if (!m_handler_vector.isEmpty())
	{
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
	}
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
QPointF ElementPrimitiveDecorator::snapConstPointToGrid(const QPointF &point) const
{
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
void ElementPrimitiveDecorator::snapPointToGrid(QPointF &point) const
{
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

/**
	@brief ElementPrimitiveDecorator::itemChange
	@param change
	@param value
	@return
*/
QVariant ElementPrimitiveDecorator::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemSceneHasChanged)
	{
		if(scene()) //Item is added to scene, we also add handlers
			addHandler();
		else //Item is removed from scene, we also remove the handlers
			removeHandler();
	}
	else if (change == ItemVisibleHasChanged)
	{
		bool visible = value.toBool();
		for(QetGraphicsHandlerItem *qghi : m_handler_vector)
			qghi->setVisible(visible);
	}
	else if (change == ItemZValueHasChanged && !m_handler_vector.isEmpty())
	{
		for (QetGraphicsHandlerItem *qghi : m_handler_vector)
			qghi->setZValue(this->zValue()+1);
	}
	
	return QGraphicsObject::itemChange(change, value);
}

/**
	@brief ElementPrimitiveDecorator::sceneEventFilter
	@param watched
	@param event
	@return
*/
bool ElementPrimitiveDecorator::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
	//Watched must be an handler
	if(watched->type() == QetGraphicsHandlerItem::Type)
	{
		QetGraphicsHandlerItem *qghi = qgraphicsitem_cast<QetGraphicsHandlerItem *>(watched);
		
		if(m_handler_vector.contains(qghi)) //Handler must be in m_vector_index, then we can start resize
		{
			m_vector_index = m_handler_vector.indexOf(qghi);
			if (m_vector_index != -1)
			{
				if(event->type() == QEvent::GraphicsSceneMousePress) //Click
				{
					handlerMousePressEvent(qghi, static_cast<QGraphicsSceneMouseEvent *>(event));
					return true;
				}
				else if(event->type() == QEvent::GraphicsSceneMouseMove) //Move
				{
					handlerMouseMoveEvent(qghi, static_cast<QGraphicsSceneMouseEvent *>(event));
					return true;
				}
				else if (event->type() == QEvent::GraphicsSceneMouseRelease) //Release
				{
					handlerMouseReleaseEvent(qghi, static_cast<QGraphicsSceneMouseEvent *>(event));
					return true;
				}
			}
		}
	}
	
	return false;
}
