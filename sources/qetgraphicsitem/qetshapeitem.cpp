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
#include "qetshapeitem.h"

#include "../PropertiesEditor/propertieseditordialog.h"
#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../QetGraphicsItemModeler/qetgraphicshandlerutility.h"
#include "../createdxf.h"
#include "../diagram.h"
#include "../diagramview.h"
#include "../qet.h"
#include "../qeticons.h"
#include "../qetxml.h"
#include "../ui/shapegraphicsitempropertieswidget.h"
#include "../utils/qetutils.h"

/**
	@brief QetShapeItem::QetShapeItem
	Constructor of shape item. point 1 and 2 must be in scene coordinate
	@param p1 first point
	@param p2 second point
	@param type type of item (line, rectangle, ellipse)
	@param parent parent item
*/
QetShapeItem::QetShapeItem(QPointF p1, QPointF p2, ShapeType type, QGraphicsItem *parent) :
	QetGraphicsItem(parent),
	m_shapeType(type),
	m_P1 (p1),
	m_P2 (p2),
	m_hovered(false)
{
	if (type == Polygon) m_polygon << m_P1 << m_P2;
	setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);
	setAcceptHoverEvents(true);
	m_pen.setStyle(Qt::SolidLine);
		//ensure handlers are always above this item
	connect(this, &QetShapeItem::zChanged, [this]()
	{
		for(QetGraphicsHandlerItem *qghi : m_handler_vector)
			qghi->setZValue(this->zValue()+1);
	});

	m_insert_point = new QAction(tr("Ajouter un point"), this);
	m_insert_point->setIcon(QET::Icons::Add);
	connect(m_insert_point, &QAction::triggered, this, &QetShapeItem::insertPoint);
	m_remove_point = new QAction(tr("Supprimer ce point"), this);
	m_remove_point->setIcon(QET::Icons::Remove);
	connect(m_remove_point, &QAction::triggered, this, &QetShapeItem::removePoint);

}

QetShapeItem::~QetShapeItem()
{
	if(!m_handler_vector.isEmpty())
		qDeleteAll(m_handler_vector);
}

/**
	@brief QetShapeItem::setPen
	Set the pen to use for draw the shape
	@param pen
*/
void QetShapeItem::setPen(const QPen &pen)
{
	if (m_pen == pen) return;
	m_pen = pen;
	update();
	emit penChanged();
}

/**
	@brief QetShapeItem::setBrush
	Set the brush to use for the fill the shape
	@param brush
*/
void QetShapeItem::setBrush(const QBrush &brush)
{
	if (m_brush == brush) return;
	m_brush = brush;
	update();
	emit brushChanged();
}

/**
	@brief QetShapeItem::setP2
	Set the second point of this item.
	If this item is a polyline,
	the last point of the polyline is replaced by P2.
	@param P2
*/
void QetShapeItem::setP2(const QPointF &P2)
{
	if (m_shapeType == Polygon && m_polygon.last() != P2)
	{
		prepareGeometryChange();
		m_polygon.replace(m_polygon.size()-1, P2);
	}
	else if (P2 != m_P2)
	{
		prepareGeometryChange();
		m_P2 = P2;
	}
}

/**
	@brief QetShapeItem::setLine
	Set item geometry to line (only available for line shape)
	@param line
	@return  : true when shape is a Line, else false
*/
bool QetShapeItem::setLine(const QLineF &line)
{
	if (Q_UNLIKELY(m_shapeType != Line)) return false;
	prepareGeometryChange();
	m_P1 = line.p1();
	m_P2 = line.p2();
	adjusteHandlerPos();
	return true;
}

/**
	@brief QetShapeItem::setRect
	Set this item geometry to rect (only available if shape is a rectangle or an ellipse)
	@param rect : new rect
	@return  : true when shape is rectangle or ellipse, else false
*/
bool QetShapeItem::setRect(const QRectF &rect)
{
	if (Q_LIKELY(m_shapeType == Rectangle || m_shapeType == Ellipse))
	{
		prepareGeometryChange();
		m_P1 = rect.topLeft();
		m_P2 = rect.bottomRight();
		adjusteHandlerPos();
		return true;
	}

	return false;
}

/**
	@brief QetShapeItem::setPolygon
	Set this item geometry to polygon (only available if shape is a polyline)
	@param polygon : new polygon
	@return true if item is polygon, else false
*/
bool QetShapeItem::setPolygon(const QPolygonF &polygon)
{
	if (Q_UNLIKELY(m_shapeType != Polygon)) {
		return false;
	}
	prepareGeometryChange();
	m_polygon = polygon;
	adjusteHandlerPos();
	return true;
}

/**
	@brief QetShapeItem::setClosed
	Close this item, have effect only if this item is a polygon.
	@param close
*/
void QetShapeItem::setClosed(bool close)
{
	if (m_shapeType == Polygon && close != m_closed)
	{
		prepareGeometryChange();
		m_closed = close;
		emit closeChanged();
	}
}

void QetShapeItem::setXRadius(qreal X)
{
	m_xRadius = X;
	update();
	adjusteHandlerPos();
	emit XRadiusChanged();
}

void QetShapeItem::setYRadius(qreal Y)
{
	m_yRadius = Y;
	update();
	adjusteHandlerPos();
	emit YRadiusChanged();
}

/**
	@brief QetShapeItem::pointCount
	@return the number of point in the polygon
*/
int QetShapeItem::pointsCount() const
{
	return m_polygon.size();
}

/**
	@brief QetShapeItem::setNextPoint
	Add a new point to the current polygon
	@param P the new point.
*/
void QetShapeItem::setNextPoint(QPointF P)
{
	prepareGeometryChange();
	m_polygon.append(Diagram::snapToGrid(P));
}

/**
	@brief QetShapeItem::removePoints
	Number of point to remove on the polygon
	If number is superior to number of polygon points-2,
	all points of polygon will be removed except
	the first two (minimum point for the polygon);
*/
void QetShapeItem::removePoints(int number)
{
	if (pointsCount() == 2 || number < 1) return;
	if ((pointsCount()-2) < number)
		number = pointsCount() - 2;

	int i = 0;
	do
	{
		i++;
		prepareGeometryChange();
		m_polygon.pop_back();
		setTransformOriginPoint(boundingRect().center());

	} while (i < number);
}

/**
	@brief QetShapeItem::boundingRect
	@return the bounding rect of this item
*/
QRectF QetShapeItem::boundingRect() const
{
	return shape().boundingRect().adjusted(-6, -6, 6, 6);
}

/**
	@brief QetShapeItem::shape
	@return the shape of this item
*/
QPainterPath QetShapeItem::shape() const
{
	QPainterPath path;

	switch (m_shapeType)
	{
		case Line:
			path.moveTo(m_P1);
			path.lineTo(m_P2);
			break;
		case Rectangle:
			path.addRoundedRect(
						QRectF(m_P1, m_P2),
						m_xRadius,
						m_yRadius);
			break;
		case Ellipse:
			path.addEllipse(QRectF(m_P1, m_P2));
			break;
		case Polygon:
			path.addPolygon(m_polygon);
			if (m_closed) {
				path.closeSubpath();
			}
			break;
	}

	QPainterPathStroker pps;
	pps.setWidth(m_hovered? m_pen.widthF()+10 : m_pen.widthF());
	pps.setJoinStyle(Qt::RoundJoin);
	path = pps.createStroke(path);

	return (path);
}

/**
	@brief QetShapeItem::paint
	Paint this item
	@param painter
	@param option
	@param widget
*/
void QetShapeItem::paint(
		QPainter *painter,
		const QStyleOptionGraphicsItem *option,
		QWidget *widget)
{
	Q_UNUSED(option)
	Q_UNUSED(widget)

	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);
	painter->setPen(m_pen);
	painter->setBrush(m_brush);

		//Draw hovered shadow
	if (m_hovered)
	{
		painter->save();
		QColor color(Qt::darkBlue);
		color.setAlpha(50);
		painter -> setBrush (QBrush (color));
		painter -> setPen   (Qt::NoPen);
		painter -> drawPath (shape());
		painter -> restore  ();
	}

	switch (m_shapeType)
	{
	case Line:      painter->drawLine(QLineF(m_P1, m_P2)); break;
	case Rectangle: painter->drawRoundedRect(QRectF(m_P1, m_P2),
						 m_xRadius,
						 m_yRadius); break;
	case Ellipse:   painter->drawEllipse(QRectF(m_P1, m_P2)); break;
	case Polygon:   m_closed ? painter->drawPolygon(m_polygon)
				 : painter->drawPolyline(m_polygon); break;
	}

	painter->restore();
}

/**
	@brief QetShapeItem::hoverEnterEvent
	Handle hover enter event
	@param event
*/
void QetShapeItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	m_hovered = true;
	QetGraphicsItem::hoverEnterEvent(event);
}

/**
	@brief QetShapeItem::hoverLeaveEvent
	Handle hover leave event
	@param event
*/
void QetShapeItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	m_hovered = false;
	QetGraphicsItem::hoverLeaveEvent(event);
}

void QetShapeItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	event->ignore();
	QetGraphicsItem::mousePressEvent(event);

	if (event->button() == Qt::LeftButton) {
		switchResizeMode();
		event->accept();
	}
}

/**
	@brief QetShapeItem::itemChange
	@param change
	@param value
	@return
*/
QVariant QetShapeItem::itemChange(QGraphicsItem::GraphicsItemChange change,
				  const QVariant &value)
{
	if (change == ItemSelectedHasChanged)
	{
		if (value.toBool() == true) {
			//If this is selected, wa add handlers.
			addHandler();
		}
		else //Else this is deselected, we remove handlers
		{
			if(!m_handler_vector.isEmpty())
			{
				qDeleteAll(m_handler_vector);
				m_handler_vector.clear();
			}
			m_resize_mode = 1;
		}
	}
	else if (change == ItemPositionHasChanged) {
		adjusteHandlerPos();
	}
	else if (change == ItemSceneHasChanged)
	{
		if (!scene()) //This is removed from scene, then we deselect this, and so, the handlers is also removed.
		{
			setSelected(false);
		}
	}

	return QGraphicsItem::itemChange(change, value);
}

/**
	@brief QetShapeItem::sceneEventFilter
	@param watched
	@param event
	@return
*/
bool QetShapeItem::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
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
					handlerMousePressEvent();
					return true;
				}
				else if(event->type() == QEvent::GraphicsSceneMouseMove) //Move
				{
					handlerMouseMoveEvent(static_cast<QGraphicsSceneMouseEvent *>(event));
					return true;
				}
				else if (event->type() == QEvent::GraphicsSceneMouseRelease) //Release
				{
					handlerMouseReleaseEvent();
					return true;
				}
			}
		}
	}

	return false;
}

/**
	@brief QetShapeItem::contextMenuEvent
	@param event
*/
void QetShapeItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	m_context_menu_pos = event->pos();

	if (m_shapeType == QetShapeItem::Polygon)
	{
		if (diagram()->selectedItems().isEmpty()) {
			this->setSelected(true);
		}

		if (isSelected() && scene()->selectedItems().size() == 1)
		{
			if (diagram())
			{
				DiagramView *d_view = nullptr;
				for (QGraphicsView *view : diagram()->views())
				{
					if (view->isActiveWindow())
					{
						d_view = dynamic_cast<DiagramView *>(view);
						if (d_view)
							continue;
					}
				}

				if (d_view)
				{
					QScopedPointer<QMenu> menu(new QMenu());
					menu.data()->addAction(m_insert_point);

					if (m_handler_vector.count() > 2)
					{
						for (QetGraphicsHandlerItem *qghi : m_handler_vector)
						{
							if (qghi->contains(qghi->mapFromScene(event->scenePos())))
							{
								menu.data()->addAction(m_remove_point);
								break;
							}
						}
					}

					menu.data()->addSeparator();
					menu.data()->addActions(d_view->contextMenuActions());
					menu.data()->exec(event->screenPos());
					event->accept();
					return;
				}
			}
		}
	}

	QetGraphicsItem::contextMenuEvent(event);
}

/**
	@brief QetShapeItem::switchResizeMode
*/
void QetShapeItem::switchResizeMode()
{
	if (m_shapeType == Ellipse)
	{
		if (m_resize_mode == 1)
		{
			m_resize_mode = 2;
			for (QetGraphicsHandlerItem *qghi : m_handler_vector) {
				qghi->setColor(Qt::darkGreen);
			}
		}
		else
		{
			m_resize_mode = 1;
			for (QetGraphicsHandlerItem *qghi : m_handler_vector) {
				qghi->setColor(Qt::blue);
			}
		}
	}
	else if (m_shapeType == Rectangle)
	{
		if (m_resize_mode == 1)
		{
			m_resize_mode = 2;
			for (QetGraphicsHandlerItem *qghi : m_handler_vector)
				qghi->setColor(Qt::darkGreen);
		}
		else if (m_resize_mode == 2)
		{
			m_resize_mode = 3;
			qDeleteAll(m_handler_vector);
			m_handler_vector.clear();
			addHandler();
			for (QetGraphicsHandlerItem *qghi : m_handler_vector) {
				qghi->setColor(Qt::magenta);
			}
		}
		else if (m_resize_mode == 3)
		{
			m_resize_mode = 1;
			qDeleteAll(m_handler_vector);
			m_handler_vector.clear();
			addHandler();
			for (QetGraphicsHandlerItem *qghi : m_handler_vector) {
				qghi->setColor(Qt::blue);
			}

		}
	}
}

void QetShapeItem::addHandler()
{
	if (m_handler_vector.isEmpty())
	{
		QVector <QPointF> points_vector;
		switch (m_shapeType)
		{
			case Line:
				points_vector << m_P1 << m_P2;
				break;
			case Rectangle:
				if (m_resize_mode == 3) {
					points_vector = QetGraphicsHandlerUtility::pointForRadiusRect(QRectF(m_P1, m_P2), m_xRadius, m_yRadius);
				}
				else {
					points_vector = QetGraphicsHandlerUtility::pointsForRect(QRectF(m_P1, m_P2));
				}
				break;
			case Ellipse:
				points_vector = QetGraphicsHandlerUtility::pointsForRect(QRectF(m_P1, m_P2));
				break;
			case Polygon:
				points_vector = m_polygon;
				break;
		}

		if(!points_vector.isEmpty() && scene())
		{
			m_handler_vector = QetGraphicsHandlerItem::handlerForPoint(mapToScene(points_vector), QETUtils::graphicsHandlerSize(this));

			for(const auto handler : qAsConst(m_handler_vector))
			{
				handler->setZValue(this->zValue()+1);
				handler->setColor(Qt::blue);
				scene()->addItem(handler);
				handler->installSceneEventFilter(this);
			}
		}
	}
}

/**
	@brief QetShapeItem::adjusteHandlerPos
	Adjust the position of the handler item
*/
void QetShapeItem::adjusteHandlerPos()
{
	if (m_handler_vector.isEmpty()) {
		return;
	}

	QVector <QPointF> points_vector;
	switch (m_shapeType)
	{
		case Line: {
			points_vector << m_P1 << m_P2;
			break;
		}
		case Rectangle: {
			if (m_resize_mode != 3) {
				points_vector = QetGraphicsHandlerUtility::pointsForRect(QRectF(m_P1, m_P2));
			}
			else {
				points_vector = QetGraphicsHandlerUtility::pointForRadiusRect(QRectF(m_P1, m_P2), m_xRadius, m_yRadius);
			}
			break;
		}
		case Ellipse: {
			points_vector = QetGraphicsHandlerUtility::pointsForRect(QRectF(m_P1, m_P2));
			break;
		}
		case Polygon: {
			points_vector = m_polygon;
			break;
		}
	}

	if (m_handler_vector.size() == points_vector.size())
	{
		points_vector = mapToScene(points_vector);
		for (int i = 0 ; i < points_vector.size() ; ++i)
			m_handler_vector.at(i)->setPos(points_vector.at(i));
	}
	else
	{
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
		addHandler();
	}
}

void QetShapeItem::insertPoint()
{
	if (m_shapeType == QetShapeItem::Polygon)
	{
		QPolygonF new_polygon = QetGraphicsHandlerUtility::polygonForInsertPoint(this->polygon(), m_closed, Diagram::snapToGrid(m_context_menu_pos));

		if(new_polygon != m_polygon)
		{
				//Wrap the undo for avoid to merge the undo commands when user add several points.
			QUndoCommand *undo = new QUndoCommand(tr("Ajouter un point à un polygone"));
			new QPropertyUndoCommand(this, "polygon", m_polygon, new_polygon, undo);
			diagram()->undoStack().push(undo);
		}
	}
}

void QetShapeItem::removePoint()
{
	if (m_shapeType != QetShapeItem::Polygon) {
		return;
	}

	if (m_handler_vector.size() == 2) {
		return;
	}

	QPointF point = mapToScene(m_context_menu_pos);
	int index = -1;
	for (int i=0 ; i<m_handler_vector.size() ; i++)
	{
		QetGraphicsHandlerItem *qghi = m_handler_vector.at(i);
		if (qghi->contains(qghi->mapFromScene(point)))
		{
			index = i;
			break;
		}
	}
	if (index > -1 && index<m_handler_vector.count())
	{
		QPolygonF polygon = this->polygon();
		polygon.removeAt(index);

			//Wrap the undo for avoid to merge the undo commands when user add several points.
		QUndoCommand *undo = new QUndoCommand(tr("Supprimer un point d'un polygone"));
		new QPropertyUndoCommand(this, "polygon", this->polygon(), polygon, undo);
		diagram()->undoStack().push(undo);
	}
}

/**
	@brief QetShapeItem::handlerMousePressEvent
	@param qghi
	@param event
*/
void QetShapeItem::handlerMousePressEvent()
{
	m_old_P1 = m_P1;
	m_old_P2 = m_P2;
	m_old_polygon = m_polygon;
	m_old_xRadius = m_xRadius;
	m_old_yRadius = m_yRadius;
	if(m_xRadius == 0 && m_yRadius == 0) {
		m_modifie_radius_equaly = true;
	}
}

/**
	@brief QetShapeItem::handlerMouseMoveEvent
	@param event
*/
void QetShapeItem::handlerMouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	QPointF new_pos = event->scenePos();
	if (event->modifiers() != Qt::ControlModifier)
		new_pos = Diagram::snapToGrid(event->scenePos());
	new_pos = mapFromScene(new_pos);

	switch (m_shapeType)
	{
		case Line:
			prepareGeometryChange();
			m_vector_index == 0 ? m_P1 = new_pos : m_P2 = new_pos;
			adjusteHandlerPos();
			break;

		case Rectangle:
			if (m_resize_mode == 1) {
				setRect(QetGraphicsHandlerUtility::rectForPosAtIndex(QRectF(m_P1, m_P2), new_pos, m_vector_index));
				break;
			}
			else if (m_resize_mode == 2) {
				setRect(QetGraphicsHandlerUtility::mirrorRectForPosAtIndex(QRectF(m_P1, m_P2), new_pos, m_vector_index));
				break;
			}
			else {
				qreal radius = QetGraphicsHandlerUtility::radiusForPosAtIndex(QRectF(m_P1, m_P2), new_pos, m_vector_index);
				if(m_modifie_radius_equaly) {
					setXRadius(radius);
					setYRadius(radius);
				}
				else if(m_vector_index == 0) {
					setXRadius(radius);
				}
				else {
					setYRadius(radius);
				}
				adjusteHandlerPos();
				break;
			}
		case Ellipse:
			if (m_resize_mode == 1) {
				setRect(QetGraphicsHandlerUtility::rectForPosAtIndex(QRectF(m_P1, m_P2), new_pos, m_vector_index));
				break;
			}
			else {
				setRect(QetGraphicsHandlerUtility::mirrorRectForPosAtIndex(QRectF(m_P1, m_P2), new_pos, m_vector_index));
				break;
			}

		case Polygon:
			prepareGeometryChange();
			m_polygon.replace(m_vector_index, new_pos);
			adjusteHandlerPos();
			break;
	}	//End switch
}

/**
	@brief QetShapeItem::handlerMouseReleaseEvent
	@param qghi
	@param event
*/
void QetShapeItem::handlerMouseReleaseEvent()
{
	m_modifie_radius_equaly = false;

	if (diagram())
	{
		QPropertyUndoCommand *undo = nullptr;
		if ((m_shapeType & (Line | Rectangle | Ellipse)) && ((m_P1 != m_old_P1 || m_P2 != m_old_P2) ||
															 (m_old_xRadius != XRadius() || m_old_yRadius != m_yRadius))
			)
		{
			switch(m_shapeType)
			{
				case Line: {
					undo = new QPropertyUndoCommand(this, "line",QLineF(m_old_P1, m_old_P2), QLineF(m_P1, m_P2));
					break;
				}
				case Rectangle: {
					if (m_resize_mode == 1 || m_resize_mode == 2) {
						undo = new QPropertyUndoCommand(this, "rect",QRectF(m_old_P1, m_old_P2), QRectF(m_P1, m_P2).normalized());
					}
					else if (m_resize_mode == 3)
					{
						undo = new QPropertyUndoCommand(this, "xRadius", m_old_xRadius, m_xRadius);
						QPropertyUndoCommand *undo_ = new QPropertyUndoCommand(this, "yRadius", m_old_yRadius, m_yRadius, undo);
						undo_->setAnimated();
					}
					break;
				}
				case Ellipse: {
					undo = new QPropertyUndoCommand(this, "rect",QRectF(m_old_P1, m_old_P2), QRectF(m_P1, m_P2).normalized());
					break;
				}
				case Polygon: break;
			}
			if (undo) {
				undo->setAnimated(true, false);
			}
		}
		else if (m_shapeType == Polygon && (m_polygon != m_old_polygon))
			undo = new QPropertyUndoCommand(this, "polygon", m_old_polygon, m_polygon);

		if(undo)
		{
			undo->setText(tr("Modifier %1").arg(name()));
			diagram()->undoStack().push(undo);
		}
	}
}

/**
	@brief QetShapeItem::fromXml
	Build this item from the xml description
	@param e element where is stored this item
	@return true if load success
*/
bool QetShapeItem::fromXml(const QDomElement &e)
{
	if (e.tagName() != "shape") return (false);

	is_movable_ = (e.attribute("is_movable").toInt());
	m_closed = e.attribute("closed", "0").toInt();
	m_pen = QETXML::penFromXml(e.firstChildElement("pen"));
	m_brush = QETXML::brushFromXml(e.firstChildElement("brush"));

	QString type = e.attribute("type");
	QMetaEnum me = metaObject()->enumerator(metaObject()->indexOfEnumerator("ShapeType"));
	m_shapeType = QetShapeItem::ShapeType(me.keysToValue(type.toStdString().data()));

	if (m_shapeType != Polygon)
	{
		m_P1.setX(e.attribute("x1", nullptr).toDouble());
		m_P1.setY(e.attribute("y1", nullptr).toDouble());
		m_P2.setX(e.attribute("x2", nullptr).toDouble());
		m_P2.setY(e.attribute("y2", nullptr).toDouble());

		if (m_shapeType == Rectangle)
		{
			setXRadius(e.attribute("rx", "0").toDouble());
			setYRadius(e.attribute("ry", "0").toDouble());
		}
	}
	else {
		for(const QDomElement& de : QET::findInDomElement(e, "points", "point")) {
			m_polygon << QPointF(de.attribute("x", nullptr).toDouble(), de.attribute("y", nullptr).toDouble());
		}
	}
	setZValue(e.attribute("z", QString::number(this->zValue())).toDouble());

	return (true);
}

/**
	@brief QetShapeItem::toXml
	Save this item to xml element
	@param document parent document xml
	@return element xml where is write this item
*/
QDomElement QetShapeItem::toXml(QDomDocument &document) const
{
	QDomElement result = document.createElement("shape");

		//write some attribute
	QMetaEnum me = metaObject()->enumerator(metaObject()->indexOfEnumerator("ShapeType"));
	result.setAttribute("type", me.valueToKey(m_shapeType));
	result.appendChild(QETXML::penToXml(document, m_pen));
	result.appendChild(QETXML::brushToXml(document, m_brush));
	result.setAttribute("is_movable", bool(is_movable_));
	result.setAttribute("closed", bool(m_closed));

	if (m_shapeType != Polygon)
	{
		result.setAttribute("x1", QString::number(mapToScene(m_P1).x()));
		result.setAttribute("y1", QString::number(mapToScene(m_P1).y()));
		result.setAttribute("x2", QString::number(mapToScene(m_P2).x()));
		result.setAttribute("y2", QString::number(mapToScene(m_P2).y()));

		if (m_shapeType == Rectangle)
		{
			QRectF rect(m_P1, m_P2);
			rect = rect.normalized();
			qreal x = m_xRadius;
			if (x > rect.width()/2) {
				x = rect.width()/2;
			}
			qreal y = m_yRadius;
			if (y > rect.height()/2) {
				y = rect.height()/2;
			}

			result.setAttribute("rx", QString::number(m_xRadius));
			result.setAttribute("ry", QString::number(m_yRadius));
		}
	}
	else
	{
		QDomElement points = document.createElement("points");
		for (QPointF p : m_polygon)
		{
			QDomElement point = document.createElement("point");
			QPointF pf = mapToScene(p);
			point.setAttribute("x", QString::number(pf.x()));
			point.setAttribute("y", QString::number(pf.y()));
			points.appendChild(point);
		}
		result.appendChild(points);
	}
	result.setAttribute("z", QString::number(this->zValue()));

	return(result);
}

/**
	@brief QetShapeItem::toDXF
	Draw this element to the dxf document
	@param filepath file path of the the dxf document
	@param pen
	@return true if draw success
*/
bool QetShapeItem::toDXF(const QString &filepath,const QPen &pen)
{

	switch (m_shapeType)
	{
		case Line:
	    Createdxf::drawLine(filepath,
			QLineF( mapToScene(m_P1),
				mapToScene(m_P2)),
				Createdxf::dxfColor(pen));
	    return true;
		case Rectangle:
	    Createdxf::drawRectangle(filepath,
			 QRectF(mapToScene(m_P1),
				mapToScene(m_P2)).normalized(),
				Createdxf::dxfColor(pen));
	    return true;
		case Ellipse:
	    Createdxf::drawEllipse(filepath,
			 QRectF(mapToScene(m_P1),
				mapToScene(m_P2)).normalized(),
				Createdxf::dxfColor(pen));
	    return true;
	case Polygon:
        if(m_polygon.isClosed())
            Createdxf::drawPolygon(filepath,m_polygon,Createdxf::dxfColor(pen));
        else
            Createdxf::drawPolyline(filepath,m_polygon,Createdxf::dxfColor(pen));
	    return true;
	default:
	    return false;
	}
}

/**
	@brief QetShapeItem::editProperty
	Edit the property of this item
*/
void QetShapeItem::editProperty()
{
	if (diagram() -> isReadOnly()) return;

	PropertiesEditorDialog ped(new ShapeGraphicsItemPropertiesWidget(this), diagram()->views().at(0));
	ped.exec();
}

/**
	@brief QetShapeItem::name
	@return the name of the current shape.
*/
QString QetShapeItem::name() const
{
	switch (m_shapeType) {
		case Line:	    return tr("une ligne");
		case Rectangle:	return tr("un rectangle");
		case Ellipse:	return tr("une éllipse");
		case Polygon:	return tr("une polyligne");
		default:	    return tr("une shape");
	}
}
