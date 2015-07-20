/*
	Copyright 2006-2015 The QElectroTech Team
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

/**
 * @brief PartEllipse::PartEllipse
 * Constructor
 * @param editor : QETElementEditor of this part
 * @param parent : parent item
 */
PartEllipse::PartEllipse(QETElementEditor *editor, QGraphicsItem *parent) :
	AbstractPartEllipse(editor, parent),
	m_handler(10),
	m_handler_index(-1),
	m_undo_command(nullptr)
{}

/**
 * @brief PartEllipse::~PartEllipse
 * Destructor
 */
PartEllipse::~PartEllipse() {
	if(m_undo_command) delete m_undo_command;
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
	{
		drawCross(m_rect.center(), painter);
		if (scene()->selectedItems().size() == 1)
			m_handler.drawHandler(painter, m_handler.pointsForRect(m_rect));
	}
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
	pps.setWidth(penWeight());
	shape = pps.createStroke(shape);

	if (isSelected())
		foreach(QRectF rect, m_handler.handlerRect(m_handler.pointsForRect(m_rect)))
			shape.addRect(rect);

	return shape;
}

/**
 * @brief PartEllipse::mousePressEvent
 * Handle mouse press event
 * @param event
 */
void PartEllipse::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (isSelected() && event->button() == Qt::LeftButton)
	{
		m_handler_index = m_handler.pointIsHoverHandler(event->pos(), m_handler.pointsForRect(m_rect));

		if(m_handler_index >= 0 && m_handler_index <= 7) //User click on an handler
		{
			m_undo_command = new QPropertyUndoCommand(this, "rect", QVariant(m_rect));
			m_undo_command->setText(tr("Modifier une ellipse"));
			m_undo_command->enableAnimation();
		}
		else
			CustomElementGraphicPart::mousePressEvent(event);
	}
	else
		CustomElementGraphicPart::mousePressEvent(event);
}

/**
 * @brief PartEllipse::mouseMoveEvent
 * Handle mouse move event
 * @param event
 */
void PartEllipse::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if(m_handler_index >= 0 && m_handler_index <= 7)
	{
		QPointF pos_ = event->modifiers() == Qt::ControlModifier ? event->pos() : mapFromScene(elementScene()->snapToGrid(event->scenePos()));
		prepareGeometryChange();
		setRect(m_handler.rectForPosAtIndex(m_rect, pos_, m_handler_index));
	}
	else
		CustomElementGraphicPart::mouseMoveEvent(event);
}

/**
 * @brief PartEllipse::mouseReleaseEvent
 * Handle mouse release event
 * @param event
 */
void PartEllipse::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_handler_index >= 0 && m_handler_index <= 7)
	{
		if (!m_rect.isValid())
			m_rect = m_rect.normalized();

		m_undo_command->setNewValue(QVariant(m_rect));
		elementScene()->undoStack().push(m_undo_command);
		m_undo_command = nullptr;
		m_handler_index = -1;
	}
	else
		CustomElementGraphicPart::mouseReleaseEvent(event);
}
