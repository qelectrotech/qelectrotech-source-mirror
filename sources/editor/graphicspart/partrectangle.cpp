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
#include "partrectangle.h"
#include "elementscene.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "QetGraphicsItemModeler/qetgraphicshandlerutility.h"

/**
 * @brief PartRectangle::PartRectangle
 * Constructor
 * @param editor the QETElementEditor of this item
 * @param parent parent item
 */
PartRectangle::PartRectangle(QETElementEditor *editor, QGraphicsItem *parent) :
	CustomElementGraphicPart(editor, parent),
	m_undo_command(nullptr)
{}

/**
 * @brief PartRectangle::~PartRectangle
 */
PartRectangle::~PartRectangle()
{
	if(m_undo_command) delete m_undo_command;
	removeHandler();
}

/**
 * @brief PartRectangle::paint
 * Draw this Rectangle
 * @param painter
 * @param options
 * @param widget
 */
void PartRectangle::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget)
{
	Q_UNUSED(widget);
	applyStylesToQPainter(*painter);
	QPen t = painter -> pen();
	t.setCosmetic(options && options -> levelOfDetail < 1.0);

	if (isSelected())
		t.setColor(Qt::red);

	t.setJoinStyle(Qt::MiterJoin);
	
		//Force the pen to width 0 if one of dimension is null
	if (!rect().width() || !rect().height())
		t.setWidth(0);
	
	painter -> setPen(t);
	painter -> drawRect(rect());

	if (m_hovered)
		drawShadowShape(painter);

	if (isSelected())
		drawCross(m_rect.center(), painter);
}

/**
 * @brief PartRectangle::toXml
 * Export this rectangle in xml
 * @param xml_document : Xml document to use for create the xml element.
 * @return an xml element that describe this ellipse
 */
const QDomElement PartRectangle::toXml(QDomDocument &xml_document) const
{
	QDomElement xml_element = xml_document.createElement("rect");
	QPointF top_left(sceneTopLeft());
	xml_element.setAttribute("x", QString("%1").arg(top_left.x()));
	xml_element.setAttribute("y", QString("%1").arg(top_left.y()));
	xml_element.setAttribute("width",  QString("%1").arg(m_rect.width()));
	xml_element.setAttribute("height", QString("%1").arg(m_rect.height()));
	stylesToXml(xml_element);
	return(xml_element);
}

/**
 * @brief PartRectangle::fromXml
 * Import the properties of this rectangle from a xml element.
 * @param qde : Xml document to use.
 */
void PartRectangle::fromXml(const QDomElement &qde)
{
	stylesFromXml(qde);
	setPos(mapFromScene(qde.attribute("x", "0").toDouble(),
						qde.attribute("y", "0").toDouble()));

	QRectF rect(QPointF(0,0), QSizeF(qde.attribute("width",  "0").toDouble(),
									 qde.attribute("height", "0").toDouble()));

	setRect(rect.normalized());
}

/**
 * @brief PartRectangle::rect
 * @return : Returns the item's rectangle.
 */
QRectF PartRectangle::rect() const {
	return m_rect;
}

/**
 * @brief PartRectangle::setRect
 * Sets the item's rectangle to be the given rectangle.
 * @param rect
 */
void PartRectangle::setRect(const QRectF &rect)
{
	if (rect == m_rect) return;
	prepareGeometryChange();
	m_rect = rect;
	adjusteHandlerPos();
	emit rectChanged();
}

/**
 * @brief PartRectangle::sceneGeometricRect
 * @return the minimum, margin-less rectangle this part can fit into, in scene
 * coordinates. It is different from boundingRect() because it is not supposed
 * to imply any margin, and it is different from shape because it is a regular
 * rectangle, not a complex shape.
 */
QRectF PartRectangle::sceneGeometricRect() const {
	return(mapToScene(rect()).boundingRect());
}

/**
 * @brief PartRectangle::sceneTopLeft
 * @return the top left of rectangle, in scene coordinate
 */
QPointF PartRectangle::sceneTopLeft() const {
	return(mapToScene(rect().topLeft()));
}

/**
 * @brief PartRectangle::shape
 * @return the shape of this item
 */
QPainterPath PartRectangle::shape() const
{
	QPainterPath shape;
	shape.addRect(m_rect);

	QPainterPathStroker pps;
	pps.setWidth(m_hovered? penWeight()+SHADOWS_HEIGHT : penWeight());
	shape = pps.createStroke(shape);

	return shape;
}

QPainterPath PartRectangle::shadowShape() const
{
	QPainterPath shape;
	shape.addRect(m_rect);

	QPainterPathStroker pps;
	pps.setWidth(penWeight());

	return (pps.createStroke(shape));
}

/**
 * @brief PartRectangle::boundingRect
 * @return Bounding rectangle this part can fit into
 */
QRectF PartRectangle::boundingRect() const
{
	qreal adjust = (SHADOWS_HEIGHT + penWeight()) / 2;
		//We add 0.5 because CustomElementGraphicPart::drawShadowShape
		//draw a shape bigger of 0.5 when pen weight is to 0.
	if (penWeight() == 0) adjust += 0.5;

	QRectF r = m_rect.normalized();
	r.adjust(-adjust, -adjust, adjust, adjust);

	return(r);
}

/**
 * @brief PartRectangle::isUseless
 * @return true if this part is irrelevant and does not deserve to be Retained / registered.
 * An rectangle is relevant when he's not null.
 */
bool PartRectangle::isUseless() const {
	return(rect().isNull());
}

/**
 * @brief PartRectangle::startUserTransformation
 * Start the user-induced transformation, provided this primitive is contained
 * within the initial_selection_rect bounding rectangle.
 * @param initial_selection_rect
 */
void PartRectangle::startUserTransformation(const QRectF &initial_selection_rect)
{
	Q_UNUSED(initial_selection_rect)
		// we keep track of our own rectangle at the moment in scene coordinates too
	saved_points_.clear();
	saved_points_ << mapToScene(rect().topLeft()) << mapToScene(rect().bottomRight());
}

/**
 * @brief PartRectangle::handleUserTransformation
 * Handle the user-induced transformation from \a initial_selection_rect to \a new_selection_rect
 * @param initial_selection_rect
 * @param new_selection_rect
 */
void PartRectangle::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect)
{
	QList<QPointF> mapped_points = mapPoints(initial_selection_rect, new_selection_rect, saved_points_);
	setRect(QRectF(mapFromScene(mapped_points.at(0)), mapFromScene(mapped_points.at(1))));
}

/**
 * @brief PartRectangle::mouseReleaseEvent
 * Handle mouse release event
 * @param event
 */
void PartRectangle::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && event->buttonDownPos(Qt::LeftButton) == event->pos())
		switchResizeMode();
	
	CustomElementGraphicPart::mouseReleaseEvent(event);
}

/**
 * @brief PartRectangle::itemChange
 * @param change
 * @param value
 * @return 
 */
QVariant PartRectangle::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemSelectedHasChanged && scene())
	{
		if (value.toBool() == true)
		{
				//When item is selected, he must to be up to date whene the selection in the scene change, for display or not the handler,
				//according to the number of selected items.
			connect(scene(), &QGraphicsScene::selectionChanged, this, &PartRectangle::sceneSelectionChanged); 
			
			if (scene()->selectedItems().size() == 1)
				addHandler();
		}
		else
		{
			disconnect(scene(), &QGraphicsScene::selectionChanged, this, &PartRectangle::sceneSelectionChanged);
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
			disconnect(scene(), &QGraphicsScene::selectionChanged, this, &PartRectangle::sceneSelectionChanged);
		
		setSelected(false); //This item is removed from scene, then we deselect this, and so, the handlers is also removed.
	}
	
	return QGraphicsItem::itemChange(change, value);
}

/**
 * @brief PartRectangle::sceneEventFilter
 * @param watched
 * @param event
 * @return 
 */
bool PartRectangle::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
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

/**
 * @brief PartRectangle::switchResizeMode
 */
void PartRectangle::switchResizeMode()
{
	if (m_resize_mode == 1)
	{
		m_resize_mode = 2;
		for (QetGraphicsHandlerItem *qghi : m_handler_vector)
			qghi->setColor(Qt::darkGreen);
	}
	else
	{
		m_resize_mode = 1;
		for (QetGraphicsHandlerItem *qghi : m_handler_vector)
			qghi->setColor(Qt::blue);
	}
}

/**
 * @brief PartRectangle::adjusteHandlerPos
 */
void PartRectangle::adjusteHandlerPos()
{
	if (m_handler_vector.isEmpty())
		return;
	
	QVector <QPointF> points_vector = QetGraphicsHandlerUtility::pointsForRect(m_rect);
	
	if (m_handler_vector.size() == points_vector.size())
	{
		points_vector = mapToScene(points_vector);
		for (int i = 0 ; i < points_vector.size() ; ++i)
			m_handler_vector.at(i)->setPos(points_vector.at(i));
	}
}

/**
 * @brief PartRectangle::handlerMousePressEvent
 * @param qghi
 * @param event
 */
void PartRectangle::handlerMousePressEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi);
	Q_UNUSED(event);

	m_undo_command = new QPropertyUndoCommand(this, "rect", QVariant(m_rect));
	m_undo_command->setText(tr("Modifier un rectangle"));
	m_undo_command->enableAnimation();
	return;	
}

/**
 * @brief PartRectangle::handlerMouseMoveEvent
 * @param qghi
 * @param event
 */
void PartRectangle::handlerMouseMoveEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi);
	
	QPointF new_pos = event->scenePos();
	if (event->modifiers() != Qt::ControlModifier)
		new_pos = elementScene()->snapToGrid(event->scenePos());
	new_pos = mapFromScene(new_pos);
	
	if (m_resize_mode == 1)
		setRect(QetGraphicsHandlerUtility::rectForPosAtIndex(m_rect, new_pos, m_vector_index));
	else
		setRect(QetGraphicsHandlerUtility::mirrorRectForPosAtIndex(m_rect, new_pos, m_vector_index));
	
	adjusteHandlerPos();
}

void PartRectangle::handlerMouseReleaseEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi);
	Q_UNUSED(event);
	
	m_undo_command->setNewValue(QVariant(m_rect));
	elementScene()->undoStack().push(m_undo_command);
	m_undo_command = nullptr;
	m_vector_index = -1;
}

/**
 * @brief PartRectangle::sceneSelectionChanged
 * When the scene selection change, if there are several primitive selected, we remove the handler of this item
 */
void PartRectangle::sceneSelectionChanged()
{
	if (this->isSelected() && scene()->selectedItems().size() == 1)
		addHandler();
	else
		removeHandler();
}

/**
 * @brief PartRectangle::addHandler
 * Add handlers for this item
 */
void PartRectangle::addHandler()
{
	if (m_handler_vector.isEmpty() && scene())
	{		
		m_handler_vector = QetGraphicsHandlerItem::handlerForPoint(mapToScene(QetGraphicsHandlerUtility::pointsForRect(m_rect)));
		
		for(QetGraphicsHandlerItem *handler : m_handler_vector)
		{
			QColor color = Qt::blue;
			if (m_resize_mode == 2)
				color = Qt::darkGreen;
			
			handler->setColor(color);
			scene()->addItem(handler);
			handler->installSceneEventFilter(this);
			handler->setZValue(this->zValue()+1);
		}
	}
}

/**
 * @brief PartRectangle::removeHandler
 * Remove the handlers of this item
 */
void PartRectangle::removeHandler()
{
	if (!m_handler_vector.isEmpty())
	{
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
	}
}
