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
PartArc::~PartArc() {
	if(m_undo_command) delete m_undo_command;
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
	{
		drawCross(m_rect.center(), painter);
		if (scene()->selectedItems().size() == 1) {
			if (m_resize_mode == 3)
				m_handler.drawHandler(painter, m_handler.pointsForArc(m_rect, m_start_angle /16, m_span_angle /16));
			else
				m_handler.drawHandler(painter, m_handler.pointsForRect(m_rect));
		}
	}
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

QRectF PartArc::boundingRect() const
{
	QRectF r = AbstractPartEllipse::boundingRect();

	for (QRectF rect: m_handler.handlerRect(m_handler.pointsForRect(m_rect)))
		r |= rect;

	return r;
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

	if (isSelected())
		for (QRectF rect: m_handler.handlerRect(m_handler.pointsForRect(m_rect)))
			shape.addRect(rect);

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

void PartArc::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
	if (!isSelected())
	{
		CustomElementGraphicPart::hoverMoveEvent(event);
		return;
	}

	if (m_resize_mode == 1 || m_resize_mode == 2) {
		int handler = m_handler.pointIsHoverHandler(event->pos(), m_handler.pointsForRect(m_rect));

		if (handler >= 0)
		{
			if (handler == 0 || handler == 2 || handler == 5 || handler == 7)
				setCursor(Qt::SizeAllCursor);
			else if (handler == 1 || handler == 6)
				setCursor(Qt::SizeVerCursor);
			else if (handler == 3 || handler == 4)
				setCursor(Qt::SizeHorCursor);

			return;
		}
	}
	else if (m_resize_mode == 3) {
		if (m_handler.pointIsHoverHandler(event->pos(), m_handler.pointsForArc(m_rect, m_start_angle /16, m_span_angle /16)) >= 0) {
			setCursor(Qt::SizeAllCursor);
			return;
		}
	}

	CustomElementGraphicPart::hoverMoveEvent(event);
}

/**
 * @brief PartArc::mousePressEvent
 * Handle mouse press event
 * @param event
 */
void PartArc::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
	{
		setCursor(Qt::ClosedHandCursor);
		if (isSelected())
		{
				//resize rect
			if (m_resize_mode == 1 || m_resize_mode == 2) {
				m_handler_index = m_handler.pointIsHoverHandler(event->pos(), m_handler.pointsForRect(m_rect));

				if(m_handler_index >= 0 && m_handler_index <= 7) //User click on an handler
				{
					m_undo_command = new QPropertyUndoCommand(this, "rect", QVariant(m_rect));
					m_undo_command->setText(tr("Modifier un arc"));
					m_undo_command->enableAnimation();
					return;
				}
			}
				//resize angle
			if (m_resize_mode == 3) {
				m_handler_index = m_handler.pointIsHoverHandler(event->pos(), m_handler.pointsForArc(m_rect, m_start_angle /16, m_span_angle /16));
				if (m_handler_index == 0) {
					m_span_point = m_handler.pointsForArc(m_rect, m_start_angle /16, m_span_angle /16).at(1);

					m_undo_command = new QPropertyUndoCommand(this, "startAngle", QVariant(m_start_angle));
					m_undo_command->setText(tr("Modifier un arc"));
					m_undo_command->enableAnimation();

					m_undo_command2 = new QPropertyUndoCommand(this, "spanAngle", QVariant(m_span_angle), m_undo_command);
					m_undo_command2->setText(tr("Modifier un arc"));
					m_undo_command2->enableAnimation();

					return;
				}
				else if (m_handler_index == 1) {
					m_undo_command = new QPropertyUndoCommand(this, "spanAngle", QVariant(m_span_angle));
					m_undo_command->setText(tr("Modifier un arc"));
					m_undo_command->enableAnimation();

					return;
				}
			}

		}
	}

	CustomElementGraphicPart::mousePressEvent(event);
}

/**
 * @brief PartArc::mouseMoveEvent
 * Handle mouse move event
 * @param event
 */
void PartArc::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_resize_mode == 1 || m_resize_mode == 2) {
		if (m_handler_index >= 0 && m_handler_index <= 7) {
			QPointF pos_ = event->modifiers() == Qt::ControlModifier ? event->pos() : mapFromScene(elementScene()->snapToGrid(event->scenePos()));
			prepareGeometryChange();

			if (m_resize_mode == 1)
				setRect(m_handler.rectForPosAtIndex(m_rect, pos_, m_handler_index));
			else
				setRect(m_handler.mirrorRectForPosAtIndex(m_rect, pos_, m_handler_index));

			return;
		}
	}
	else if (m_resize_mode == 3) {
		if (m_handler_index == 0 || m_handler_index == 1) {
			QLineF line(m_rect.center(), event->pos());
			prepareGeometryChange();

			if (m_handler_index == 0) {
				setStartAngle(line.angle()*16);
				setSpanAngle(line.angleTo(QLineF(m_rect.center(), m_span_point))*16);
			}
			else if (m_handler_index == 1) {
				QLineF line2(m_rect.center(), m_handler.pointsForArc(m_rect, m_start_angle/16, m_span_angle/16).at(0));
				setSpanAngle (line2.angleTo(line)*16);
			}

			return;
		}
	}

	CustomElementGraphicPart::mouseMoveEvent(event);
}

/**
 * @brief PartArc::mouseReleaseEvent
 * Handle mouse release event
 * @param event
 */
void PartArc::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		setCursor(Qt::OpenHandCursor);
		if (event->buttonDownPos(Qt::LeftButton) == event->pos())
			switchResizeMode();
	}

	if (m_resize_mode == 1 || m_resize_mode == 2) {
		if (m_handler_index >= 0 && m_handler_index <= 7) {
			if (!m_rect.isValid())
				m_rect = m_rect.normalized();

			m_undo_command->setNewValue(QVariant(m_rect));
			elementScene()->undoStack().push(m_undo_command);
			m_undo_command = nullptr;
			m_handler_index = -1;
			return;
		}
	}
	else if (m_resize_mode == 3) {
		if (m_handler_index == 0) {
			m_undo_command->setNewValue(QVariant(m_start_angle));
			m_undo_command2->setNewValue(QVariant(m_span_angle));
			elementScene()->undoStack().push(m_undo_command);
			m_undo_command = nullptr;
			m_undo_command2 = nullptr;
			m_handler_index = -1;
			return;
		}
		else if (m_handler_index == 1) {
			m_undo_command->setNewValue(QVariant(m_span_angle));
			elementScene()->undoStack().push(m_undo_command);
			m_undo_command = nullptr;
			m_handler_index = -1;
			return;
		}
	}

	CustomElementGraphicPart::mouseReleaseEvent(event);
}

void PartArc::switchResizeMode()
{
	if (m_resize_mode == 1) {
		m_resize_mode = 2;
		m_handler.setOuterColor(Qt::darkGreen);
	}
	else if (m_resize_mode == 2 ) {
		m_resize_mode = 3;
		m_handler.setOuterColor(Qt::magenta);
	}
	else {
		m_resize_mode = 1;
		m_handler.setOuterColor(Qt::blue);
	}
	update();
}
