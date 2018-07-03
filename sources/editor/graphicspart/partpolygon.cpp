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
#include "partpolygon.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "elementscene.h"
#include "QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "qetelementeditor.h"
#include "qeticons.h"
#include "QetGraphicsItemModeler/qetgraphicshandlerutility.h"


/**
 * @brief PartPolygon::PartPolygon
 * Constructor
 * @param editor : editor of this item
 * @param parent : parent item
 */
PartPolygon::PartPolygon(QETElementEditor *editor, QGraphicsItem *parent) :
	CustomElementGraphicPart(editor, parent),
	m_closed(false),
	m_undo_command(nullptr)
{
	m_insert_point = new QAction(tr("Ajouter un point"), this);
	m_insert_point->setIcon(QET::Icons::Add);
	connect(m_insert_point, &QAction::triggered, this, &PartPolygon::insertPoint);
	m_remove_point = new QAction(tr("Supprimer ce point"), this);
	m_remove_point->setIcon(QET::Icons::Remove);
	connect(m_remove_point, &QAction::triggered, this, &PartPolygon::removePoint);
}

/**
 * @brief PartPolygon::~PartPolygon
 */
PartPolygon::~PartPolygon()
{
	if(m_undo_command) delete m_undo_command;
	removeHandler();
}

/**
 * @brief PartPolygon::paint
 * Draw this polygon
 * @param painter
 * @param options
 * @param widget
 */
void PartPolygon::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget)
{
	Q_UNUSED(widget);

	applyStylesToQPainter(*painter);

	QPen t = painter -> pen();
	t.setCosmetic(options && options -> levelOfDetail < 1.0);
	if (isSelected()) t.setColor(Qt::red);
	painter -> setPen(t);

	m_closed ? painter -> drawPolygon (m_polygon) :
			   painter -> drawPolyline(m_polygon);

	if (m_hovered)
		drawShadowShape(painter);
}

/**
 * @brief PartPolygon::fromXml
 * Import the properties of this polygon from a xml element
 * @param qde : Xml document to use
 */
void PartPolygon::fromXml(const QDomElement &qde)
{
	stylesFromXml(qde);

	int i = 1;
	while(true)
	{
		if (QET::attributeIsAReal(qde, QString("x%1").arg(i)) &&\
			QET::attributeIsAReal(qde, QString("y%1").arg(i)))
			++ i;

		else break;
	}
	
	QPolygonF temp_polygon;
	for (int j = 1 ; j < i ; ++ j)
	{
		temp_polygon << QPointF(qde.attribute(QString("x%1").arg(j)).toDouble(),
								qde.attribute(QString("y%1").arg(j)).toDouble());
	}
	m_polygon = temp_polygon;
	
	m_closed = qde.attribute("closed") != "false";
}

/**
 * @brief PartPolygon::toXml
 * Export this polygin in xml
 * @param xml_document : Xml document to use for create the xml element
 * @return an xml element that describe this polygon
 */
const QDomElement PartPolygon::toXml(QDomDocument &xml_document) const
{
	QDomElement xml_element = xml_document.createElement("polygon");
	int i = 1;
	foreach(QPointF point, m_polygon) {
		point = mapToScene(point);
		xml_element.setAttribute(QString("x%1").arg(i), QString("%1").arg(point.x()));
		xml_element.setAttribute(QString("y%1").arg(i), QString("%1").arg(point.y()));
		++ i;
	}
	if (!m_closed) xml_element.setAttribute("closed", "false");
	stylesToXml(xml_element);
	return(xml_element);
}

/**
 * @brief PartPolygon::isUseless
 * @return true if this part is irrelevant and does not deserve to be Retained / registered.
 * A polygon is relevant when he have 2 differents points
 */
bool PartPolygon::isUseless() const
{
	if (m_polygon.count() < 2) return(true);

	for (int i = 1 ; i < m_polygon.count() ; ++ i)
		if (m_polygon[i] != m_polygon[i-1]) return(false);

	return(true);
}

/**
 * @brief PartPolygon::sceneGeometricRect
 * @return the minimum, margin-less rectangle this part can fit into, in scene
 * coordinates. It is different from boundingRect() because it is not supposed
 * to imply any margin, and it is different from shape because it is a regular
 * rectangle, not a complex shape.
 */
QRectF PartPolygon::sceneGeometricRect() const {
	return(mapToScene(m_polygon.boundingRect()).boundingRect());
}

/**
 * @brief PartPolygon::startUserTransformation
 * Start the user-induced transformation, provided this primitive is contained
 * within the initial_selection_rect bounding rectangle.
 * @param initial_selection_rect
 */
void PartPolygon::startUserTransformation(const QRectF &initial_selection_rect)
{
	Q_UNUSED(initial_selection_rect)
	saved_points_ = mapToScene(m_polygon).toList();
}

/**
 * @brief PartPolygon::handleUserTransformation
 * Handle the user-induced transformation from initial_selection_rect to new_selection_rect
 * @param initial_selection_rect
 * @param new_selection_rect
 */
void PartPolygon::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect)
{
	QList<QPointF> mapped_points = mapPoints(initial_selection_rect, new_selection_rect, saved_points_);
	m_polygon = (mapFromScene(QPolygonF(mapped_points.toVector())));
}

/**
 * @brief PartPolygon::preferredScalingMethod
 * This method is called by the decorator when it needs to determine the best
 * way to interactively scale a primitive. It is typically called when only a
 * single primitive is being scaled.
 * @return : This reimplementation systematically returns QET::RoundScaleRatios.
 */
QET::ScalingMethod PartPolygon::preferredScalingMethod() const {
	return(QET::RoundScaleRatios);
}

/**
 * @brief PartPolygon::polygon
 * @return the item's polygon, or an empty polygon if no polygon has been set.
 */
QPolygonF PartPolygon::polygon() const {
	return m_polygon;
}

/**
 * @brief PartPolygon::setPolygon
 * Sets the item's polygon to be the given polygon.
 * @param polygon
 */
void PartPolygon::setPolygon(const QPolygonF &polygon)
{
	if (m_polygon == polygon) return;
	prepareGeometryChange();
	m_polygon = polygon;
	adjusteHandlerPos();
	emit polygonChanged();
}

/**
 * @brief PartPolygon::addPoint
 * Add new point to polygon
 * @param point
 */
void PartPolygon::addPoint(const QPointF &point)
{
	prepareGeometryChange();
	m_polygon << point;
}

/**
 * @brief PartPolygon::setLastPoint
 * Set the last point of polygon to @point
 * @param point
 */
void PartPolygon::setLastPoint(const QPointF &point)
{
	if (m_polygon.size())
		m_polygon.pop_back();

	prepareGeometryChange();
	m_polygon << point;
}

/**
 * @brief PartPolygon::removeLastPoint
 * Remove the last point of polygon
 */
void PartPolygon::removeLastPoint()
{
	if (m_polygon.size())
	{
		prepareGeometryChange();
		m_polygon.pop_back();
	}
}

void PartPolygon::setClosed(bool close)
{
	if (m_closed == close) return;
	prepareGeometryChange();
	m_closed = close;
	emit closedChange();
}

/**
 * @brief PartPolygon::itemChange
 * @param change
 * @param value
 * @return 
 */
QVariant PartPolygon::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemSelectedHasChanged && scene())
	{
		if (value.toBool() == true)
		{
				//When item is selected, he must to be up to date whene the selection in the scene change, for display or not the handler,
				//according to the number of selected items.
			connect(scene(), &QGraphicsScene::selectionChanged, this, &PartPolygon::sceneSelectionChanged); 
			
			if (scene()->selectedItems().size() == 1)
				addHandler();
		}
		else
		{
			disconnect(scene(), &QGraphicsScene::selectionChanged, this, &PartPolygon::sceneSelectionChanged);
			removeHandler();
		}
	}
	else if (change == ItemPositionHasChanged)
	{
		adjusteHandlerPos();
	}
	else if (change == ItemSceneChange)
	{
		if(scene())
			disconnect(scene(), &QGraphicsScene::selectionChanged, this, &PartPolygon::sceneSelectionChanged);
		
		setSelected(false); //This is item removed from scene, then we deselect this, and so, the handlers is also removed.
	}
	
	return QGraphicsItem::itemChange(change, value);
}

/**
 * @brief PartPolygon::sceneEventFilter
 * @param watched
 * @param event
 * @return 
 */
bool PartPolygon::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
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

void PartPolygon::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
	m_context_menu_pos = event->pos();
	event->ignore();
	if (isSelected() && elementScene() && (elementScene()->behavior() == ElementScene::Normal))
	{
		QList<QAction *> list;
		list << m_insert_point;
		if (m_handler_vector.count() > 2)
		{
			for (QetGraphicsHandlerItem *qghi : m_handler_vector)
			{
				if (qghi->contains(qghi->mapFromScene(event->scenePos())))
				{
					list << m_remove_point;
					break;
				}
			}
		}
		elementScene()->editor()->contextMenu(event->screenPos(), list);
		event->accept();
	}
}

/**
 * @brief PartPolygon::adjusteHandlerPos
 */
void PartPolygon::adjusteHandlerPos()
{
	if(m_handler_vector.isEmpty())
		return;
	
	if (m_handler_vector.size() == m_polygon.size())
	{
		QVector <QPointF> points_vector = mapToScene(m_polygon);
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

/**
 * @brief PartPolygon::handlerMousePressEvent
 * @param qghi
 * @param event
 */
void PartPolygon::handlerMousePressEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi);
	Q_UNUSED(event);
	
	m_undo_command = new QPropertyUndoCommand(this, "polygon", QVariant(m_polygon));
	m_undo_command->setText(tr("Modifier un polygone"));
}

/**
 * @brief PartPolygon::handlerMouseMoveEvent
 * @param qghi
 * @param event
 */
void PartPolygon::handlerMouseMoveEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi);
	
	QPointF new_pos = event->scenePos();
	if (event->modifiers() != Qt::ControlModifier)
		new_pos = elementScene()->snapToGrid(event->scenePos());
	new_pos = mapFromScene(new_pos);
	
	prepareGeometryChange();
	m_polygon.replace(m_vector_index, new_pos);
	adjusteHandlerPos();
	emit polygonChanged();
}

/**
 * @brief PartPolygon::handlerMouseReleaseEvent
 * @param qghi
 * @param event
 */
void PartPolygon::handlerMouseReleaseEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi);
	Q_UNUSED(event);
	
	m_undo_command->setNewValue(QVariant(m_polygon));
	elementScene()->undoStack().push(m_undo_command);
	m_undo_command = nullptr;
	m_vector_index = -1;
}

/**
 * @brief PartPolygon::sceneSelectionChanged
 * When the scene selection change, if there are several primitive selected, we remove the handler of this item
 */
void PartPolygon::sceneSelectionChanged()
{
	if (this->isSelected() && scene()->selectedItems().size() == 1)
		addHandler();
	else
		removeHandler();
}

/**
 * @brief PartPolygon::addHandler
 * Add handlers for this item
 */
void PartPolygon::addHandler()
{
	if (m_handler_vector.isEmpty() && scene())
	{		
		m_handler_vector = QetGraphicsHandlerItem::handlerForPoint(mapToScene(m_polygon));
		
		for(QetGraphicsHandlerItem *handler : m_handler_vector)
		{
			handler->setColor(Qt::blue);
			scene()->addItem(handler);
			handler->installSceneEventFilter(this);
			handler->setZValue(this->zValue()+1);
		}
	}
}

/**
 * @brief PartPolygon::removeHandler
 * Remove the handlers of this item
 */
void PartPolygon::removeHandler()
{
	if (!m_handler_vector.isEmpty())
	{
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
	}
}

/**
 * @brief PartPolygon::insertPoint
 * Insert a point in this polygone
 */
void PartPolygon::insertPoint()
{
	QPolygonF new_polygon = QetGraphicsHandlerUtility::polygonForInsertPoint(m_polygon, m_closed, elementScene()->snapToGrid(m_context_menu_pos));
	
	if(new_polygon != m_polygon)
	{
			//Wrap the undo for avoid to merge the undo commands when user add several points.
		QUndoCommand *undo = new QUndoCommand(tr("Ajouter un point à un polygone"));
		new QPropertyUndoCommand(this, "polygon", m_polygon, new_polygon, undo);
		elementScene()->undoStack().push(undo);
	}
}

/**
 * @brief PartPolygon::removePoint
 * remove a point on this polygon
 */
void PartPolygon::removePoint()
{
	if (m_handler_vector.size() == 2)
		return;
	
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
		elementScene()->undoStack().push(undo);
	}
	
}

/**
 * @brief PartPolygon::shape
 * @return the shape of this item
 */
QPainterPath PartPolygon::shape() const
{
	QPainterPath shape;
	shape.addPolygon(m_polygon);

	if (m_closed)
		shape.lineTo(m_polygon.first());

	QPainterPathStroker pps;
	pps.setWidth(m_hovered? penWeight()+SHADOWS_HEIGHT : penWeight());
	shape = pps.createStroke(shape);

	return shape;
}

QPainterPath PartPolygon::shadowShape() const
{
	QPainterPath shape;
	shape.addPolygon(m_polygon);

	if (m_closed)
		shape.lineTo(m_polygon.first());

	QPainterPathStroker pps;
	pps.setWidth(penWeight());

	return (pps.createStroke(shape));
}

/**
 * @brief PartPolygon::boundingRect
 * @return the bounding rect of this item
 */
QRectF PartPolygon::boundingRect() const
{
	QRectF r = m_polygon.boundingRect();

	qreal adjust = (SHADOWS_HEIGHT + penWeight()) / 2;
		//We add 0.5 because CustomElementGraphicPart::drawShadowShape
		//draw a shape bigger of 0.5 when pen weight is to 0.
	if (penWeight() == 0) adjust += 0.5;

	r.adjust(-adjust, -adjust, adjust, adjust);

	return(r);
}
