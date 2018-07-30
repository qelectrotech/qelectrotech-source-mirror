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
#include "partellipse.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "elementscene.h"
#include "QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "QetGraphicsItemModeler/qetgraphicshandlerutility.h"

/**
 * @brief PartEllipse::PartEllipse
 * Constructor
 * @param editor : QETElementEditor of this part
 * @param parent : parent item
 */
PartEllipse::PartEllipse(QETElementEditor *editor, QGraphicsItem *parent) :
	AbstractPartEllipse(editor, parent),
	m_undo_command(nullptr)
{}

/**
 * @brief PartEllipse::~PartEllipse
 * Destructor
 */
PartEllipse::~PartEllipse()
{
	if(m_undo_command) delete m_undo_command;
	removeHandler();
}

/**
 * @brief PartEllipse::paint
 * Draw this ellpise
 * @param painter
 * @param options
 * @param widget
 */
void PartEllipse::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget)
{
	Q_UNUSED(widget);
	applyStylesToQPainter(*painter);

	QPen t = painter -> pen();
	t.setCosmetic(options && options -> levelOfDetail < 1.0);

	if (isSelected())
		t.setColor(Qt::red);

	painter -> setPen(t);
	painter -> drawEllipse(rect());

	if (m_hovered)
		drawShadowShape(painter);

	if (isSelected())
		drawCross(m_rect.center(), painter);
}

/**
 * @brief PartEllipse::toXml
 * Export this ellipse in xml
 * @param xml_document : Xml document to use for create the xml element.
 * @return : an xml element that describe this ellipse
 */
const QDomElement PartEllipse::toXml(QDomDocument &xml_document) const
{
	QDomElement xml_element;
	if (qFuzzyCompare(rect().width(), rect().height()))
	{
		xml_element = xml_document.createElement("circle");
		xml_element.setAttribute("diameter", QString("%1").arg(rect().width()));
	}
	else
	{
		xml_element = xml_document.createElement("ellipse");
		xml_element.setAttribute("width",  QString("%1").arg(rect().width()));
		xml_element.setAttribute("height", QString("%1").arg(rect().height()));
	}

	QPointF top_left(sceneTopLeft());
	xml_element.setAttribute("x", QString("%1").arg(top_left.x()));
	xml_element.setAttribute("y", QString("%1").arg(top_left.y()));

	stylesToXml(xml_element);

	return(xml_element);
}

/**
 * @brief PartEllipse::fromXml
 * Import the properties of this ellipse from a xml element.
 * @param qde : Xml document to use.
 */
void PartEllipse::fromXml(const QDomElement &qde)
{
	stylesFromXml(qde);
	qreal width, height;

	if (qde.tagName() == "ellipse")
	{
		width = qde.attribute("width",  "0").toDouble();
		height = qde.attribute("height", "0").toDouble();
	}
	else
		width = height = qde.attribute("diameter", "0").toDouble();

	m_rect = QRectF(mapFromScene(qde.attribute("x", "0").toDouble(),
								 qde.attribute("y", "0").toDouble()),
					QSizeF(width, height));
}

/**
 * @brief PartEllipse::shape
 * @return the shape of this item
 */
QPainterPath PartEllipse::shape() const
{
	QPainterPath shape;
	shape.addEllipse(m_rect);

	QPainterPathStroker pps;
	pps.setWidth(m_hovered? penWeight()+SHADOWS_HEIGHT : penWeight());
	shape = pps.createStroke(shape);

	return shape;
}

QPainterPath PartEllipse::shadowShape() const
{
	QPainterPath shape;
	shape.addEllipse(m_rect);

	QPainterPathStroker pps;
	pps.setWidth(penWeight());

	return (pps.createStroke(shape));
}

/**
 * @brief PartEllipse::mouseReleaseEvent
 * Handle mouse release event
 * @param event
 */
void PartEllipse::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && event->buttonDownPos(Qt::LeftButton) == event->pos())
		switchResizeMode();
	
	CustomElementGraphicPart::mouseReleaseEvent(event);
}

/**
 * @brief PartEllipse::itemChange
 * @param change
 * @param value
 * @return 
 */
QVariant PartEllipse::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemSelectedHasChanged && scene())
	{
		if (value.toBool() == true)
		{
				//When item is selected, he must to be up to date whene the selection in the scene change, for display or not the handler,
				//according to the number of selected items.
			connect(scene(), &QGraphicsScene::selectionChanged, this, &PartEllipse::sceneSelectionChanged); 
			
			if (scene()->selectedItems().size() == 1)
				addHandler();
		}
		else
		{
			disconnect(scene(), &QGraphicsScene::selectionChanged, this, &PartEllipse::sceneSelectionChanged);
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
			disconnect(scene(), &QGraphicsScene::selectionChanged, this, &PartEllipse::sceneSelectionChanged);
		
		setSelected(false); //This item is removed from scene, then we deselect this, and so, the handlers is also removed.
	}
	
	return QGraphicsItem::itemChange(change, value);
}

/**
 * @brief PartEllipse::sceneEventFilter
 * @param watched
 * @param event
 * @return 
 */
bool PartEllipse::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
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

void PartEllipse::switchResizeMode()
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
 * @brief PartEllipse::adjusteHandlerPos
 */
void PartEllipse::adjusteHandlerPos()
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
 * @brief PartEllipse::handlerMousePressEvent
 * @param qghi
 * @param event
 */
void PartEllipse::handlerMousePressEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi);
	Q_UNUSED(event);

	m_undo_command = new QPropertyUndoCommand(this, "rect", QVariant(m_rect));
	m_undo_command->setText(tr("Modifier un rectangle"));
	m_undo_command->enableAnimation();
	return;	
}

/**
 * @brief PartEllipse::handlerMouseMoveEvent
 * @param qghi
 * @param event
 */
void PartEllipse::handlerMouseMoveEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
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

/**
 * @brief PartEllipse::handlerMouseReleaseEvent
 * @param qghi
 * @param event
 */
void PartEllipse::handlerMouseReleaseEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi);
	Q_UNUSED(event);
	
	m_undo_command->setNewValue(QVariant(m_rect));
	elementScene()->undoStack().push(m_undo_command);
	m_undo_command = nullptr;
	m_vector_index = -1;
}

/**
 * @brief PartEllipse::sceneSelectionChanged
 * When the scene selection change, if there are several primitive selected, we remove the handler of this item
 */
void PartEllipse::sceneSelectionChanged()
{
	if (this->isSelected() && scene()->selectedItems().size() == 1)
		addHandler();
	else
		removeHandler();
}

/**
 * @brief PartEllipse::addHandler
 * Add handlers for this item
 */
void PartEllipse::addHandler()
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
 * @brief PartEllipse::removeHandler
 * Remove the handlers of this item
 */
void PartEllipse::removeHandler()
{
	if (!m_handler_vector.isEmpty())
	{
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
	}
}
