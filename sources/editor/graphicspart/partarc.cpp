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
#include "partarc.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "elementscene.h"
#include "QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "QetGraphicsItemModeler/qetgraphicshandlerutility.h"


/**
 * @brief PartArc::PartArc
 * Constructor
 * @param editor : QETElementEditor of this part
 * @param parent : parent item
 */
PartArc::PartArc(QETElementEditor *editor, QGraphicsItem *parent) :
	AbstractPartEllipse(editor, parent)
{
	m_start_angle = 0;
	m_span_angle = -1440;
}

/**
 * @brief PartArc::~PartArc
 * Destructor
 */
PartArc::~PartArc()
{
	if(m_undo_command) delete m_undo_command;
	removeHandler();
}

/**
 * @brief PartArc::paint
 * Draw this arc
 * @param painter
 * @param options
 * @param widget
 */
void PartArc::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget)
{
	Q_UNUSED(widget);

	applyStylesToQPainter(*painter);

		//Always remove the brush
	painter -> setBrush(Qt::NoBrush);
	QPen t = painter -> pen();
	t.setCosmetic(options && options -> levelOfDetail < 1.0);
	painter -> setPen(t);
	
	if (isSelected())
	{
		painter->save();
		QPen pen(Qt::DotLine);
		pen.setWidth(1);
		pen.setCosmetic(true);
		painter->setPen(pen);
			//Draw the ellipse in black
		painter -> drawEllipse(rect());
		painter->restore();

			//Draw the arc in red
		t.setColor(Qt::red);
		painter -> setPen(t);
	}
	
	painter -> drawArc(m_rect, m_start_angle, m_span_angle);

	if (m_hovered)
		drawShadowShape(painter);

	if (isSelected())
		drawCross(m_rect.center(), painter);
}

/**
 * @brief PartArc::toXml
 * Export this arc in xml
 * @param xml_document : Xml document to use for create the xml element.
 * @return : an xml element that describe this arc
 */
const QDomElement PartArc::toXml(QDomDocument &xml_document) const {
	QDomElement xml_element = xml_document.createElement("arc");
	QPointF top_left(sceneTopLeft());
	xml_element.setAttribute("x", QString("%1").arg(top_left.x()));
	xml_element.setAttribute("y", QString("%1").arg(top_left.y()));
	xml_element.setAttribute("width",  QString("%1").arg(rect().width()));
	xml_element.setAttribute("height", QString("%1").arg(rect().height()));
		//to maintain compatibility with the previous version, we write the angle in degrees.
	xml_element.setAttribute("start", QString("%1").arg(m_start_angle / 16));
	xml_element.setAttribute("angle", QString("%1").arg(m_span_angle / 16));
	stylesToXml(xml_element);
	return(xml_element);
}

/**
 * @brief PartArc::fromXml
 * Import the properties of this arc from a xml element.
 * @param qde : Xml document to use.
 */
void PartArc::fromXml(const QDomElement &qde) {
	stylesFromXml(qde);
	m_rect = QRectF(mapFromScene(qde.attribute("x", "0").toDouble(),
								 qde.attribute("y", "0").toDouble()),
					QSizeF(qde.attribute("width",  "0").toDouble(),
						   qde.attribute("height", "0").toDouble()) );

	m_start_angle = qde.attribute("start", "0").toDouble() * 16;
	m_span_angle  = qde.attribute("angle", "-1440").toDouble() * 16;
}

/**
 * @brief PartArc::shape
 * @return the shape of this item
 */
QPainterPath PartArc::shape() const
{
	QPainterPath shape;
	shape.arcMoveTo(m_rect, m_start_angle/16);
	shape.arcTo(m_rect, m_start_angle /16, m_span_angle /16);

	QPainterPathStroker pps;
	pps.setWidth(m_hovered? penWeight()+SHADOWS_HEIGHT : penWeight());
	shape = pps.createStroke(shape);

	return shape;
}

QPainterPath PartArc::shadowShape() const
{
	QPainterPath shape;
	shape.arcMoveTo(m_rect, m_start_angle/16);
	shape.arcTo(m_rect, m_start_angle /16, m_span_angle /16);

	QPainterPathStroker pps;
	pps.setWidth(penWeight());

	return (pps.createStroke(shape));
}

/**
 * @brief PartArc::mouseReleaseEvent
 * Handle mouse release event
 * @param event
 */
void PartArc::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && event->buttonDownPos(Qt::LeftButton) == event->pos())
		switchResizeMode();

	CustomElementGraphicPart::mouseReleaseEvent(event);
}

/**
 * @brief PartArc::itemChange
 * @param change
 * @param value
 * @return 
 */
QVariant PartArc::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemSelectedHasChanged && scene())
	{
		if (value.toBool() == true)
		{
				//When item is selected, he must to be up to date whene the selection in the scene change, for display or not the handler,
				//according to the number of selected items.
			connect(scene(), &QGraphicsScene::selectionChanged, this, &PartArc::sceneSelectionChanged); 
			
			if (scene()->selectedItems().size() == 1)
				addHandler();
		}
		else
		{
			disconnect(scene(), &QGraphicsScene::selectionChanged, this, &PartArc::sceneSelectionChanged);
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
			disconnect(scene(), &QGraphicsScene::selectionChanged, this, &PartArc::sceneSelectionChanged);
		
		setSelected(false); //This is item removed from scene, then we deselect this, and so, the handlers is also removed.
	}
	
	return QGraphicsItem::itemChange(change, value);
}

/**
 * @brief PartArc::sceneEventFilter
 * @param watched
 * @param event
 * @return 
 */
bool PartArc::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
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
 * @brief PartArc::switchResizeMode
 */
void PartArc::switchResizeMode()
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
			
			//From rect mode to angle mode, then numbers of handlers change
		removeHandler();
		addHandler();
		
		for (QetGraphicsHandlerItem *qghi : m_handler_vector)
			qghi->setColor(Qt::magenta);
	}
	else
	{		
		m_resize_mode = 1;
		
			//From angle mode to rect mode, then numbers of handlers change
		removeHandler();
		addHandler();
		
		for (QetGraphicsHandlerItem *qghi : m_handler_vector)
			qghi->setColor(Qt::blue);
	}
}

/**
 * @brief PartArc::adjusteHandlerPos
 */
void PartArc::adjusteHandlerPos()
{
	if (m_handler_vector.isEmpty())
		return;
	
	QVector <QPointF> points_vector;
	
	if(m_resize_mode == 3)
		points_vector = QetGraphicsHandlerUtility::pointsForArc(m_rect, m_start_angle/16, m_span_angle/16);
	else
		points_vector = QetGraphicsHandlerUtility::pointsForRect(m_rect);
		
	
	if (m_handler_vector.size() == points_vector.size())
	{
		points_vector = mapToScene(points_vector);
		for (int i = 0 ; i < points_vector.size() ; ++i)
			m_handler_vector.at(i)->setPos(points_vector.at(i));
	}	
}

/**
 * @brief PartArc::handlerMousePressEvent
 * @param qghi
 * @param event
 */
void PartArc::handlerMousePressEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi);
	Q_UNUSED(event);
	
	if (m_resize_mode == 3) //Resize angle
	{
		if (m_vector_index == 0)
		{
			m_span_point = QetGraphicsHandlerUtility::pointsForArc(m_rect, m_start_angle /16, m_span_angle /16).at(1);

			m_undo_command = new QPropertyUndoCommand(this, "startAngle", QVariant(m_start_angle));
			m_undo_command->setText(tr("Modifier un arc"));
			m_undo_command->enableAnimation();

			m_undo_command2 = new QPropertyUndoCommand(this, "spanAngle", QVariant(m_span_angle), m_undo_command);
			m_undo_command2->setText(tr("Modifier un arc"));
			m_undo_command2->enableAnimation();
		}
		else if (m_vector_index == 1)
		{
			m_undo_command = new QPropertyUndoCommand(this, "spanAngle", QVariant(m_span_angle));
			m_undo_command->setText(tr("Modifier un arc"));
			m_undo_command->enableAnimation();
		}
	}
	else //resize rect
	{
		m_undo_command = new QPropertyUndoCommand(this, "rect", QVariant(m_rect));
		m_undo_command->setText(tr("Modifier un arc"));
		m_undo_command->enableAnimation();
	}
}

/**
 * @brief PartArc::handlerMouseMoveEvent
 * @param qghi
 * @param event
 */
void PartArc::handlerMouseMoveEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi);
	
	QPointF new_pos = event->scenePos();
	if (event->modifiers() != Qt::ControlModifier)
		new_pos = elementScene()->snapToGrid(event->scenePos());
	new_pos = mapFromScene(new_pos);
	
	if (m_resize_mode == 1)
		setRect(QetGraphicsHandlerUtility::rectForPosAtIndex(m_rect, new_pos, m_vector_index));
	else if (m_resize_mode == 2)
		setRect(QetGraphicsHandlerUtility::mirrorRectForPosAtIndex(m_rect, new_pos, m_vector_index));
	else
	{
		QLineF line(m_rect.center(), mapFromScene(event->scenePos()));
		prepareGeometryChange();

		if (m_vector_index == 0) {
			setStartAngle(line.angle()*16);
			setSpanAngle(line.angleTo(QLineF(m_rect.center(), m_span_point))*16);
		}
		else if (m_vector_index == 1) {
			QLineF line2(m_rect.center(), QetGraphicsHandlerUtility::pointsForArc(m_rect, m_start_angle/16, m_span_angle/16).at(0));
			setSpanAngle (line2.angleTo(line)*16);
		}
	}
}

/**
 * @brief PartArc::handlerMouseReleaseEvent
 * @param qghi
 * @param event
 */
void PartArc::handlerMouseReleaseEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi);
	Q_UNUSED(event);
	
	if (m_resize_mode == 3)
	{
		if (m_vector_index == 0)
		{
			m_undo_command->setNewValue(QVariant(m_start_angle));
			m_undo_command2->setNewValue(QVariant(m_span_angle));
			elementScene()->undoStack().push(m_undo_command);
			m_undo_command = nullptr;
			m_undo_command2 = nullptr;
			m_vector_index = -1;
		}
		else if (m_vector_index == 1)
		{
			m_undo_command->setNewValue(QVariant(m_span_angle));
			elementScene()->undoStack().push(m_undo_command);
			m_undo_command = nullptr;
			m_vector_index = -1;
		}
	}
	else
	{
		if (!m_rect.isValid())
			m_rect = m_rect.normalized();

		m_undo_command->setNewValue(QVariant(m_rect));
		elementScene()->undoStack().push(m_undo_command);
		m_undo_command = nullptr;
		m_vector_index = -1;
	}
}

/**
 * @brief PartArc::sceneSelectionChanged
 * When the scene selection change, if there are several primitive selected, we remove the handler of this item
 */
void PartArc::sceneSelectionChanged()
{
	if (this->isSelected() && scene()->selectedItems().size() == 1)
		addHandler();
	else
		removeHandler();
}

/**
 * @brief PartArc::addHandler
 * Add handlers for this item
 */
void PartArc::addHandler()
{
	if (m_handler_vector.isEmpty() && scene())
	{
		if(m_resize_mode == 3)
		{
			m_handler_vector = QetGraphicsHandlerItem::handlerForPoint(mapToScene(QetGraphicsHandlerUtility::pointsForArc(m_rect, m_start_angle/16, m_span_angle/16)));
		}
		else
			m_handler_vector = QetGraphicsHandlerItem::handlerForPoint(mapToScene(QetGraphicsHandlerUtility::pointsForRect(m_rect)));
		
		for(QetGraphicsHandlerItem *handler : m_handler_vector)
		{
			QColor color = Qt::blue;
			if (m_resize_mode == 2)
				color = Qt::darkGreen;
			else if (m_resize_mode == 3)
				color = Qt::magenta;
			
			handler->setColor(color);
			scene()->addItem(handler);
			handler->installSceneEventFilter(this);
			handler->setZValue(this->zValue()+1);
		}
	}
}

/**
 * @brief PartArc::removeHandler
 * Remove the handlers of this item
 */
void PartArc::removeHandler()
{
	if (!m_handler_vector.isEmpty())
	{
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
	}
}
