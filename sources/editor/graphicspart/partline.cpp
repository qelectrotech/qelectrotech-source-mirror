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
#include "partline.h"
#include <cmath>
#include "elementscene.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"


/**
 * @brief PartLine::PartLine
 * Constructor
 * @param editor : QETElementEditor of this part
 * @param parent : parent item
 */
PartLine::PartLine(QETElementEditor *editor, QGraphicsItem *parent) :
	CustomElementGraphicPart(editor, parent),
	first_end(Qet::None),
	first_length(1.5),
	second_end(Qet::None),
	second_length(1.5),
	m_handler(10),
	m_handler_index(-1),
	m_undo_command(nullptr)
{}

/// Destructeur
PartLine::~PartLine() {
	if(m_undo_command) delete m_undo_command;
}

/**
 * @brief PartLine::requiredLengthForEndType
 * @param end_type
 * @return the number of "length" needed to draw a extremity of type Qet::EndType.
 */
uint PartLine::requiredLengthForEndType(const Qet::EndType &end_type)
{
	uint length_count_required = 0;

	if (end_type == Qet::Circle || end_type == Qet::Diamond)
		length_count_required = 2;
	else if (end_type == Qet::Simple || end_type == Qet::Triangle)
		length_count_required = 1;

	return(length_count_required);
}

/**
 * @brief PartLine::paint
 * Draw this line
 * @param painter
 * @param options
 * @param widget
 */
void PartLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget)
{
	Q_UNUSED(widget);
	if (isUseless()) return;

	painter->save();
	applyStylesToQPainter(*painter);
	QPen t = painter -> pen();
	t.setJoinStyle(Qt::MiterJoin);
	t.setCosmetic(options && options -> levelOfDetail < 1.0);

	if (isSelected()) t.setColor(Qt::red);

	painter -> setPen(t);
	
	if (first_end || second_end)
		painter -> drawPath(path());
	else
		painter -> drawLine(m_line);

	if (m_hovered)
		drawShadowShape(painter);

	if (isSelected() && scene()->selectedItems().size() == 1)
		m_handler.drawHandler(painter, m_handler.pointsForLine(m_line));

	painter->restore();
}

/**
 * @brief PartLine::toXml
 * Export this line in xml
 * @param xml_document : Xml document to use for create the xml element.
 * @return an xml element that describe this line
 */
const QDomElement PartLine::toXml(QDomDocument &xml_document) const
{
	QPointF p1(sceneP1());
	QPointF p2(sceneP2());
	
	QDomElement xml_element = xml_document.createElement("line");
	xml_element.setAttribute("x1", QString("%1").arg(p1.x()));
	xml_element.setAttribute("y1", QString("%1").arg(p1.y()));
	xml_element.setAttribute("x2", QString("%1").arg(p2.x()));
	xml_element.setAttribute("y2", QString("%1").arg(p2.y()));
	xml_element.setAttribute("end1", Qet::endTypeToString(first_end));
	xml_element.setAttribute("length1", QString("%1").arg(first_length));
	xml_element.setAttribute("end2", Qet::endTypeToString(second_end));
	xml_element.setAttribute("length2", QString("%1").arg(second_length));
	
	stylesToXml(xml_element);
	return(xml_element);
}

/**
 * @brief PartLine::fromXml
 * Import the properties of this line from a xml element.
 * @param qde : Xml document to use
 */
void PartLine::fromXml(const QDomElement &qde) {
	stylesFromXml(qde);
	m_line = QLineF(mapFromScene(qde.attribute("x1", "0").toDouble(),
								 qde.attribute("y1", "0").toDouble()),
					mapFromScene(qde.attribute("x2", "0").toDouble(),
								 qde.attribute("y2", "0").toDouble()));

	first_end     = Qet::endTypeFromString(qde.attribute("end1"));
	first_length  = qde.attribute("length1", "1.5").toDouble();
	second_end    = Qet::endTypeFromString(qde.attribute("end2"));
	second_length = qde.attribute("length2", "1.5").toDouble();
}

/**
 * @brief PartLine::p1
 * @return the point p1 of line.
 */
QPointF PartLine::p1() const {
	return m_line.p1();
}

/**
 * @brief PartLine::setP1
 * set first point to P1
 * @param p1
 */
void PartLine::setP1(const QPointF &p1)
{
	if (p1 == m_line.p1()) return;
	prepareGeometryChange();
	m_line.setP1(p1);
}

/**
 * @brief PartLine::p2
 * @return  the point p2 of line
 */
QPointF PartLine::p2() const {
	return m_line.p2();
}

/**
 * @brief PartLine::setP2
 * set second point to P2
 * @param p2
 */
void PartLine::setP2(const QPointF &p2)
{
	if (p2 == m_line.p2()) return;
	prepareGeometryChange();
	m_line.setP2(p2);
}

/**
 * @brief PartLine::mousePressEvent
 * Handle mouse press event
 * @param event
 */
void PartLine::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if(isSelected() && event->button() == Qt::LeftButton)
	{
		m_handler_index = m_handler.pointIsHoverHandler(event->pos(), m_handler.pointsForLine(m_line));

		if(m_handler_index >= 0 && m_handler_index <= 1) //User click on an handler
		{
			m_undo_command = new QPropertyUndoCommand(this, "line", QVariant(m_line));
			m_undo_command->setText(tr("Modifier une ligne"));
			m_undo_command->enableAnimation();
		}
		else
			CustomElementGraphicPart::mousePressEvent(event);
	}
	else
		CustomElementGraphicPart::mousePressEvent(event);
}

/**
 * @brief PartLine::mouseMoveEvent
 * Handle pouse move event
 * @param event
 */
void PartLine::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if(m_handler_index >= 0 && m_handler_index <= 1)
	{
		QPointF pos_ = event->modifiers() == Qt::ControlModifier ? event->pos() : mapFromScene(elementScene()->snapToGrid(event->scenePos()));
		prepareGeometryChange();
		setLine(m_handler.lineForPosAtIndex(m_line, pos_, m_handler_index));
	}
	else
		CustomElementGraphicPart::mouseMoveEvent(event);
}

/**
 * @brief PartLine::mouseReleaseEvent
 * Handle mouse release event
 * @param event
 */
void PartLine::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_handler_index >= 0 && m_handler_index <= 1)
	{
		m_undo_command->setNewValue(QVariant(m_line));
		elementScene()->undoStack().push(m_undo_command);
		m_undo_command = nullptr;
		m_handler_index = -1;
	}
	else
		CustomElementGraphicPart::mouseReleaseEvent(event);
}

/**
 * @brief PartLine::sceneP1
 * @return the point p1 in scene coordinate
 */
QPointF PartLine::sceneP1() const {
	return(mapToScene(p1()));
}

/**
 * @brief PartLine::sceneP2
 * @return the point p2 in scen coordinate
 */
QPointF PartLine::sceneP2() const {
	return(mapToScene(p2()));
}

/**
 * @brief PartLine::shape
 * @return the shape of this item
 */
QPainterPath PartLine::shape() const
{
	QPainterPath shape;

		//We calcul path only if there is an end type
		//Else we just draw a line
	if (first_end || second_end)
		shape.addPath(path());
	else
	{
		shape.moveTo(m_line.p1());
		shape.lineTo(m_line.p2());
	}

	QPainterPathStroker pps;
	pps.setWidth(penWeight());

	return (pps.createStroke(shape));
}

/**
 * @brief PartLine::fourShapePoints
 * @return a list with the two points that delimite the line
 * + the four points surrounding these two points
 */
QList<QPointF> PartLine::fourShapePoints() const
{
	const qreal marge = 2.0;

	QPointF a = m_line.p1();
	QPointF b = m_line.p2();
	
	QList<QPointF> result;
	
		//Special case, the line is defined by one point
	if (a == b)
	{
		result << QPointF(a.x() - marge, a.y() - marge);
		result << QPointF(a.x() - marge, a.y() + marge);
		result << QPointF(a.x() + marge, a.y() + marge);
		result << QPointF(a.x() + marge, a.y() - marge);
	}
	else
	{
			//We calcule the vector AB : (xb-xa, yb-ya)
		QPointF v_ab = b - a;
		
			//And the distance AB: root of the coordinates of the vector squared
		qreal ab = sqrt(pow(v_ab.x(), 2) + pow(v_ab.y(), 2));
		
			//Next, we define the vector u(a, b) wich is equal to the vector AB divided
			//by is length and multiplied by the length of marge.
		QPointF u = v_ab / ab * marge;
		
			//We define the vector v(-b, a) wich is perpendicular to AB
		QPointF v(-u.y(), u.x());
		QPointF m = -u + v; // we have vector M = -u + v
		QPointF n = -u - v; // and vector N=-u-v
		QPointF h =  a + m; // H = A + M
		QPointF k =  a + n; // K = A + N
		QPointF i =  b - n; // I = B - N
		QPointF j =  b - m; // J = B - M
		
		result << h << i << j << k;
	}
	return(result);
}

/**
 * @brief PartLine::firstEndCircleRect
 * @return the rectangle bordering the entirety of the first extremity
 */
QRectF PartLine::firstEndCircleRect() const
{
	QList<QPointF> interesting_points = fourEndPoints(m_line.p1(),
													  m_line.p2(),
													  first_length);
	
	QRectF end_rect(
		interesting_points[0] - QPointF(first_length, first_length),
		QSizeF(2.0 * first_length, 2.0 * first_length)
	);
	
	return(end_rect);
}

/**
 * @brief PartLine::secondEndCircleRect
 * @return the rectangle bordering the entirety of the second extremity
 */
QRectF PartLine::secondEndCircleRect() const {
	QList<QPointF> interesting_points = fourEndPoints(m_line.p2(),
													  m_line.p1(),
													  second_length);
	
	QRectF end_rect(
		interesting_points[0] - QPointF(second_length, second_length),
		QSizeF(2.0 * second_length, 2.0 * second_length)
	);
	
	return(end_rect);
}

/**
 * @brief PartLine::debugPaint
 * Display several composante of the drawing
 * -the bounding rect
 * -special points at each extremity
 * -the quadrature of the circle at each extremity, even if itself is an other type
 * @param painter
 */
void PartLine::debugPaint(QPainter *painter)
{
	painter -> save();
	painter -> setPen(Qt::gray);
	painter -> drawRect(boundingRect());
	
	painter -> setPen(Qt::green);
	painter -> drawRect(firstEndCircleRect());
	painter -> drawRect(secondEndCircleRect());
	
	painter -> setPen(Qt::red);

	foreach(QPointF pointy, fourEndPoints(p1(), p2(), first_length))
		painter -> drawEllipse(pointy, 0.1, 0.1);

	foreach(QPointF pointy, fourEndPoints(p2(), p1(), second_length))
		painter -> drawEllipse(pointy, 0.1, 0.1);
	
	painter -> restore();
}

/**
 * @brief PartLine::boundingRect
 * @return the bounding rect of this part
 */
QRectF PartLine::boundingRect() const
{	
	QRectF bound;
	if (first_end || second_end)
		bound = path().boundingRect();
	else
		bound = QRectF (m_line.p1(), m_line.p2());

	qreal adjust = (SHADOWS_HEIGHT + penWeight()) / 2;
		//We add 0.5 because CustomElementGraphicPart::drawShadowShape
		//draw a shape bigger of 0.5 when pen weight is to 0.
	if (penWeight() == 0) adjust += 0.5;

	bound = bound.normalized();
	bound.adjust(-adjust, -adjust, adjust, adjust);
	return bound;
}

/**
 * @brief PartLine::isUseless
 * @return true if this part is irrelevant and does not deserve to be Retained / registered.
 * A line is relevant when is two point is different
 */
bool PartLine::isUseless() const {
	return(m_line.p1() == m_line.p2());
}

/**
 * @brief PartLine::sceneGeometricRect
 * @return the minimum, margin-less rectangle this part can fit into, in scene
 * coordinates. It is different from boundingRect() because it is not supposed
 * to imply any margin, and it is different from shape because it is a regular
 * rectangle, not a complex shape.
 */
QRectF PartLine::sceneGeometricRect() const {
	return(QRectF(sceneP1(), sceneP2()));
}

/**
 * @brief PartLine::startUserTransformation
 * Start the user-induced transformation, provided this primitive is contained
 * within the \a initial_selection_rect bounding rectangle.
 * @param initial_selection_rect
 */
void PartLine::startUserTransformation(const QRectF &initial_selection_rect)
{
	Q_UNUSED(initial_selection_rect)
	saved_points_.clear();
	saved_points_ << sceneP1() << sceneP2();
}

/**
 * @brief PartLine::handleUserTransformation
 * Handle the user-induced transformation from \a initial_selection_rect to \a new_selection_rect
 * @param initial_selection_rect
 * @param new_selection_rect
 */
void PartLine::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect)
{
	QList<QPointF> mapped_points = mapPoints(initial_selection_rect, new_selection_rect, saved_points_);
	prepareGeometryChange();
	m_line = QLineF(mapFromScene(mapped_points.at(0)), mapFromScene(mapped_points.at(1)));
}

/**
 * @brief PartLine::fourEndPoints
 * Return the four interesting point needed to draw the shape
 * at extremity of line (circle, diamond, arrow, triangle)
 * This points are in order :
 *		O : point on the line, at a distance 'length' of the extremity
 *		A : point on the line at a 'length' of 2x the extremity length
 *		B : point at a distance of length O - O is the projection of B on the line
 *		C : point at a distance of length O - O is the projection of C on the line
 * @param end_point : The concerned extremity
 * @param other_point : other needed point to define the line
 * @param length : length to use between the extremity and the point O
 * @return
 */
QList<QPointF> PartLine::fourEndPoints(const QPointF &end_point, const QPointF &other_point, const qreal &length)
{
		//Vector and length of the line
	QPointF line_vector = end_point - other_point;
	qreal line_length = sqrt(pow(line_vector.x(), 2) + pow(line_vector.y(), 2));

		//Unitary vector and perpendicular vector
	QPointF u(line_vector / line_length * length);
	QPointF v(-u.y(), u.x());
	
		// points O, A, B, C
	QPointF o(end_point - u);
	QPointF a(o - u);
	QPointF b(o + v);
	QPointF c(o - v);
	
	return(QList<QPointF>() << o << a << b << c);
}

QLineF PartLine::line() const {
	return m_line;
}

void PartLine::setLine(const QLineF &line)
{
	if (m_line != line)
	{
		prepareGeometryChange();
		m_line = line;
	}
}

/**
 * @brief PartLine::path
 * @return this line has a QPainterPath.
 * It's notably use when this line have an end type (circle, triangle etc....),
 * because return a QPainterPath with end already draw.
 * Else if there isn't an end type get P1 and P2 of line is better (faster).
 */
QPainterPath PartLine::path() const
{
	QPainterPath path;

	QPointF point1(m_line.p1());
	QPointF point2(m_line.p2());

	qreal line_length(m_line.length());
	qreal pen_width = penWeight();

	qreal length1 = first_length;
	qreal length2 = second_length;

	//debugPaint(painter);

		//Determine if we must to draw extremity
	qreal reduced_line_length = line_length - (length1 * requiredLengthForEndType(first_end));
	bool draw_1st_end = first_end && reduced_line_length >= 0;

	if (draw_1st_end)
		reduced_line_length -= (length2 * requiredLengthForEndType(second_end));
	else
		reduced_line_length = line_length - (length2 * requiredLengthForEndType(second_end));


		//Draw the first extremity
	QPointF start_point;
	if (draw_1st_end)
	{
		QList<QPointF> four_points1(fourEndPoints(point1, point2, length1));

		if (first_end == Qet::Circle)
		{
			path.addEllipse(QRectF(four_points1[0] - QPointF(length1, length1), QSizeF(length1 * 2.0, length1 * 2.0)));
			start_point = four_points1[1];
		}
		else if (first_end == Qet::Diamond)
		{
			path.addPolygon(QPolygonF() << four_points1[1] << four_points1[2] << point1 << four_points1[3] << four_points1[1]);
			start_point = four_points1[1];
		}
		else if (first_end == Qet::Simple)
		{
			path.addPolygon(QPolygonF() << four_points1[3] << point1 << four_points1[2]);
			start_point = point1;

		}
		else if (first_end == Qet::Triangle)
		{
			path.addPolygon(QPolygonF() << four_points1[0] << four_points1[2] << point1 << four_points1[3] << four_points1[0]);
			start_point = four_points1[0];
		}

			//Adjust the start point according to the pen width
		if (pen_width && (first_end == Qet::Simple || first_end == Qet::Circle))
			start_point = QLineF(start_point, point2).pointAt(pen_width / 2.0 / line_length);
	}
	else
	{
		start_point = point1;
	}

		//Draw the second extremity
	QPointF stop_point;
	bool draw_2nd_end = second_end && reduced_line_length >= 0;
	if (draw_2nd_end)
	{
		QList<QPointF> four_points2(fourEndPoints(point2, point1, length2));

		if (second_end == Qet::Circle)
		{
			path.addEllipse(QRectF(four_points2[0] - QPointF(length2, length2), QSizeF(length2 * 2.0, length2 * 2.0)));
			stop_point = four_points2[1];
		}
		else if (second_end == Qet::Diamond)
		{
			path.addPolygon(QPolygonF() << four_points2[2] << point2 << four_points2[3] << four_points2[1] << four_points2[2]);
			stop_point = four_points2[1];
		}
		else if (second_end == Qet::Simple)
		{
			path.addPolygon(QPolygonF() << four_points2[3] << point2 << four_points2[2]);
			stop_point = point2;
		}
		else if (second_end == Qet::Triangle)
		{/**
	@return true si cette partie n'est pas pertinente et ne merite pas d'etre
	conservee / enregistree.
	Une ligne est pertinente des lors que ses deux points sont differents
*/
			path.addPolygon(QPolygonF() << four_points2[0] << four_points2[2] << point2 << four_points2[3] << four_points2[0]);
			stop_point = four_points2[0];
		}

			//Adjust the end point accordint to the pen width
		if (pen_width && (second_end == Qet::Simple || second_end == Qet::Circle))
			stop_point = QLineF(point1, stop_point).pointAt((line_length - (pen_width / 2.0)) / line_length);
	}
	else
	{
		stop_point = point2;
	}

	path.moveTo(start_point);
	path.lineTo(stop_point);

	return path;
}
