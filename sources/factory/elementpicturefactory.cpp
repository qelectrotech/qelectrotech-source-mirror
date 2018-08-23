/*
	Copyright 2006-2018 The QElectroTech Team
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
#include "elementpicturefactory.h"
#include "elementslocation.h"
#include "qet.h"
#include "qetapp.h"
#include "partline.h"

#include <QDomElement>
#include <QPainter>
#include <QTextDocument>
#include <QPicture>
#include <iostream>
#include <QAbstractTextDocumentLayout>

ElementPictureFactory* ElementPictureFactory::m_factory = nullptr;

/**
 * @brief ElementPictureFactory::getPictures
 * Set the picture of the element at location.
 * Note, picture can be null
 * @param location
 * @param picture
 * @param low_picture
 */
void ElementPictureFactory::getPictures(const ElementsLocation &location, QPicture &picture, QPicture &low_picture)
{
	if(!location.exist()) {
		return;
	}
	
	QUuid uuid = location.uuid();
	
	if(m_pictures_H.keys().contains(uuid))
	{
		picture = m_pictures_H.value(uuid);
		low_picture = m_low_pictures_H.value(uuid);
	}
	else
	{
		if (build(location))
		{
			picture = m_pictures_H.value(uuid);
			low_picture = m_low_pictures_H.value(uuid);
		}
	}
}

/**
 * @brief ElementPictureFactory::pixmap
 * @param location
 * @return the pixmap of the element at @location
 * Note pixmap can be null
 */
QPixmap ElementPictureFactory::pixmap(const ElementsLocation &location)
{
	QUuid uuid = location.uuid();
	if (m_pixmap_H.contains(uuid)) {
		return m_pixmap_H.value(uuid);
	}
	
	if(build(location))
	{
		QDomElement dom = location.xml();
			//size
		int w = dom.attribute("width").toInt();
		int h = dom.attribute("height").toInt();
		while (w % 10) ++ w;
		while (h % 10) ++ h;
			//hotspot
		int hsx = qMin(dom.attribute("hotspot_x").toInt(), w);
		int hsy = qMin(dom.attribute("hotspot_y").toInt(), h);

		QPixmap pix(w, h);
		pix.fill(QColor(255, 255, 255, 0));
		
		QPainter painter(&pix);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
		painter.translate(hsx, hsy);
		painter.drawPicture(0, 0, m_pictures_H.value(uuid));
		
		m_pixmap_H.insert(uuid, pix);
		return pix;
	}
	
	return QPixmap();
}


/**
 * @brief ElementPictureFactory::getPrimitives
 * @param location
 * @return The primtive used to draw the element at @location
 */
ElementPictureFactory::primitives ElementPictureFactory::getPrimitives(const ElementsLocation &location)
{
	if(!m_primitives_H.contains(location.uuid()))
		build(location);
	
	return m_primitives_H.value(location.uuid());
}


bool ElementPictureFactory::build(const ElementsLocation &location)
{
	QDomElement dom = location.xml();
	
		//Check if the curent version can read the xml description
	if (dom.hasAttribute("version"))
	{
		bool conv_ok;
		qreal element_version = dom.attribute("version").toDouble(&conv_ok);
		if (conv_ok && QET::version.toDouble() < element_version)
		{
			std::cerr << qPrintable(
			QObject::tr("Avertissement : l'élément "
			" a été enregistré avec une version"
			" ultérieure de QElectroTech.")
			) << std::endl;
		}
	}
	
		//This attributes must be present and valid
	int w, h, hot_x, hot_y;
	if (!QET::attributeIsAnInteger(dom, QString("width"), &w) ||\
		!QET::attributeIsAnInteger(dom, QString("height"), &h) ||\
		!QET::attributeIsAnInteger(dom, QString("hotspot_x"), &hot_x) ||\
		!QET::attributeIsAnInteger(dom, QString("hotspot_y"), &hot_y))
	{
		return(false);
	}
	
	QPicture pic;
	QPicture low_pic;
	primitives primitives_;
	
	m_pictures_H.insert(location.uuid(), pic);
	m_low_pictures_H.insert(location.uuid(), low_pic);
	m_primitives_H.insert(location.uuid(), primitives_);
	
	QPainter painter;
	painter.begin(&pic);
	painter.setRenderHint(QPainter::Antialiasing,         true);
	painter.setRenderHint(QPainter::TextAntialiasing,     true);
	painter.setRenderHint(QPainter::SmoothPixmapTransform,true);

	
	QPainter low_painter;
	low_painter.begin(&low_pic);
	low_painter.setRenderHint(QPainter::Antialiasing,         true);
	low_painter.setRenderHint(QPainter::TextAntialiasing,     true);
	low_painter.setRenderHint(QPainter::SmoothPixmapTransform,true);
	QPen tmp;
	tmp.setWidthF(1.0); //Vaudoo line to take into account the setCosmetic - don't remove
	tmp.setCosmetic(true);
	low_painter.setPen(tmp);
	
		//scroll of the Children of the Definition: Parts of the Drawing
	for (QDomNode node = dom.firstChild() ; !node.isNull() ; node = node.nextSibling())
	{
		QDomElement elmts = node.toElement();
		if (elmts.isNull()) {
			continue;
		}
		
		if (elmts.tagName() == "description")
		{	
				//Manage the graphic description = part of drawing
			for (QDomNode n = node.firstChild() ; !n.isNull() ; n = n.nextSibling())
			{
				QDomElement qde = n.toElement();
				if (qde.isNull()) {
					continue;
				}
				parseElement(qde, painter, primitives_);
				primitives fake_prim;
				parseElement(qde, low_painter, fake_prim);
			}
		}
	}
	
		//End of the drawing
	painter.end();
	low_painter.end();
	return true;
}

void ElementPictureFactory::parseElement(const QDomElement &dom, QPainter &painter, primitives &prim) const
{
	     if (dom.tagName() == "line")    (parseLine   (dom, painter, prim));
	else if (dom.tagName() == "rect")    (parseRect   (dom, painter, prim));
	else if (dom.tagName() == "ellipse") (parseEllipse(dom, painter, prim));
	else if (dom.tagName() == "circle")  (parseCircle (dom, painter, prim));
	else if (dom.tagName() == "arc")     (parseArc    (dom, painter, prim));
	else if (dom.tagName() == "polygon") (parsePolygon(dom, painter, prim));
	else if (dom.tagName() == "text")    (parseText   (dom, painter, prim));
}

void ElementPictureFactory::parseLine(const QDomElement &dom, QPainter &painter, primitives &prim) const
{
		//This attributes must be present and valid
	qreal x1, y1, x2, y2;
	if (!QET::attributeIsAReal(dom, QString("x1"), &x1)) return;
	if (!QET::attributeIsAReal(dom, QString("y1"), &y1)) return;
	if (!QET::attributeIsAReal(dom, QString("x2"), &x2)) return;
	if (!QET::attributeIsAReal(dom, QString("y2"), &y2)) return;

	Qet::EndType first_end = Qet::endTypeFromString(dom.attribute("end1"));
	Qet::EndType second_end = Qet::endTypeFromString(dom.attribute("end2"));
	qreal length1, length2;
	if (!QET::attributeIsAReal(dom, QString("length1"), &length1)) length1 = 1.5;
	if (!QET::attributeIsAReal(dom, QString("length2"), &length2)) length2 = 1.5;

	painter.save();
	setPainterStyle(dom, painter);
	QPen t = painter.pen();
	t.setJoinStyle(Qt::MiterJoin);
	painter.setPen(t);

	QLineF line(x1, y1, x2, y2);

	prim.m_lines << line;

	QPointF point1(line.p1());
	QPointF point2(line.p2());

	qreal line_length(line.length());
	qreal pen_width = painter.pen().widthF();

		//Check if we must to draw extremity
	bool draw_1st_end, draw_2nd_end;
	qreal reduced_line_length = line_length - (length1 * PartLine::requiredLengthForEndType(first_end));
	draw_1st_end = first_end && reduced_line_length >= 0;
	if (draw_1st_end) {
		reduced_line_length -= (length2 * PartLine::requiredLengthForEndType(second_end));
	} else {
		reduced_line_length = line_length - (length2 * PartLine::requiredLengthForEndType(second_end));
	}
	draw_2nd_end = second_end && reduced_line_length >= 0;

		//Draw first extremity
	QPointF start_point, stop_point;
	if (draw_1st_end) {
		QList<QPointF> four_points1(PartLine::fourEndPoints(point1, point2, length1));
		if (first_end == Qet::Circle) {
			painter.drawEllipse(QRectF(four_points1[0] - QPointF(length1, length1), QSizeF(length1 * 2.0, length1 * 2.0)));
			start_point = four_points1[1];
		} else if (first_end == Qet::Diamond) {
			painter.drawPolygon(QPolygonF() << four_points1[1] << four_points1[2] << point1 << four_points1[3]);
			start_point = four_points1[1];
		} else if (first_end == Qet::Simple) {
			painter.drawPolyline(QPolygonF() << four_points1[3] << point1 << four_points1[2]);
			start_point = point1;

		} else if (first_end == Qet::Triangle) {
			painter.drawPolygon(QPolygonF() << four_points1[0] << four_points1[2] << point1 << four_points1[3]);
			start_point = four_points1[0];
		}

			//Adjust the begining according to the width of the pen
		if (pen_width && (first_end == Qet::Simple || first_end == Qet::Circle)) {
			start_point = QLineF(start_point, point2).pointAt(pen_width / 2.0 / line_length);
		}
	} else {
		start_point = point1;
	}

		//Draw second extremity
	if (draw_2nd_end) {
		QList<QPointF> four_points2(PartLine::fourEndPoints(point2, point1, length2));
		if (second_end == Qet::Circle) {
			painter.drawEllipse(QRectF(four_points2[0] - QPointF(length2, length2), QSizeF(length2 * 2.0, length2 * 2.0)));
			stop_point = four_points2[1];
		} else if (second_end == Qet::Diamond) {
			painter.drawPolygon(QPolygonF() << four_points2[2] << point2 << four_points2[3] << four_points2[1]);
			stop_point = four_points2[1];
		} else if (second_end == Qet::Simple) {
			painter.drawPolyline(QPolygonF() << four_points2[3] << point2 << four_points2[2]);
			stop_point = point2;
		} else if (second_end == Qet::Triangle) {
			painter.drawPolygon(QPolygonF() << four_points2[0] << four_points2[2] << point2 << four_points2[3] << four_points2[0]);
			stop_point = four_points2[0];
		}

			//Adjust the end according to the width of the pen
		if (pen_width && (second_end == Qet::Simple || second_end == Qet::Circle)) {
			stop_point = QLineF(point1, stop_point).pointAt((line_length - (pen_width / 2.0)) / line_length);
		}
	} else {
		stop_point = point2;
	}

	painter.drawLine(start_point, stop_point);

	painter.restore();
}

void ElementPictureFactory::parseRect(const QDomElement &dom, QPainter &painter, ElementPictureFactory::primitives &prim) const
{
		//This attributes must be present and valid
	qreal rect_x, rect_y, rect_w, rect_h, rect_rx, rect_ry;
	if (!QET::attributeIsAReal(dom, QString("x"),       &rect_x))  return;
	if (!QET::attributeIsAReal(dom, QString("y"),       &rect_y))  return;
	if (!QET::attributeIsAReal(dom, QString("width"),   &rect_w))  return;
	if (!QET::attributeIsAReal(dom, QString("height"),  &rect_h))  return;
	rect_rx = dom.attribute("rx", "0").toDouble();
	rect_ry = dom.attribute("ry", "0").toDouble();

	prim.m_rectangles << QRectF(rect_x, rect_y, rect_w, rect_h);

	painter.save();
	setPainterStyle(dom, painter);

	QPen p = painter.pen();
	p.setJoinStyle(Qt::MiterJoin);
	painter.setPen(p);

	painter.drawRoundedRect(QRectF(rect_x, rect_y, rect_w, rect_h), rect_rx, rect_ry);
	painter.restore();
}

void ElementPictureFactory::parseEllipse(const QDomElement &dom, QPainter &painter, ElementPictureFactory::primitives &prim) const
{
		//This attributes must be present and valid
	qreal ellipse_x, ellipse_y, ellipse_l, ellipse_h;
	if (!QET::attributeIsAReal(dom, QString("x"),      &ellipse_x))  return;
	if (!QET::attributeIsAReal(dom, QString("y"),      &ellipse_y))  return;
	if (!QET::attributeIsAReal(dom, QString("width"),  &ellipse_l))  return;
	if (!QET::attributeIsAReal(dom, QString("height"), &ellipse_h))  return;
	painter.save();
	setPainterStyle(dom, painter);

	QVector<qreal> arc;
	arc.push_back(ellipse_x);
	arc.push_back(ellipse_y);
	arc.push_back(ellipse_l);
	arc.push_back(ellipse_h);
	arc.push_back(0);
	arc.push_back(360);
	prim.m_arcs << arc;

	painter.drawEllipse(QRectF(ellipse_x, ellipse_y, ellipse_l, ellipse_h));
	painter.restore();
}

void ElementPictureFactory::parseCircle(const QDomElement &dom, QPainter &painter, ElementPictureFactory::primitives &prim) const
{
		//This attributes must be present and valid
	qreal cercle_x, cercle_y, cercle_r;
	if (!QET::attributeIsAReal(dom, QString("x"),        &cercle_x)) return;
	if (!QET::attributeIsAReal(dom, QString("y"),        &cercle_y)) return;
	if (!QET::attributeIsAReal(dom, QString("diameter"), &cercle_r)) return;
	painter.save();
	setPainterStyle(dom, painter);
	QRectF circle_bounding_rect(cercle_x, cercle_y, cercle_r, cercle_r);

	prim.m_circles << circle_bounding_rect;

	painter.drawEllipse(circle_bounding_rect);
	painter.restore();
}

void ElementPictureFactory::parseArc(const QDomElement &dom, QPainter &painter, ElementPictureFactory::primitives &prim) const
{
		//This attributes must be present and valid
	qreal arc_x, arc_y, arc_l, arc_h, arc_s, arc_a;
	if (!QET::attributeIsAReal(dom, QString("x"),       &arc_x))  return;
	if (!QET::attributeIsAReal(dom, QString("y"),       &arc_y))  return;
	if (!QET::attributeIsAReal(dom, QString("width"),   &arc_l))  return;
	if (!QET::attributeIsAReal(dom, QString("height"),  &arc_h))  return;
	if (!QET::attributeIsAReal(dom, QString("start"),   &arc_s))  return;
	if (!QET::attributeIsAReal(dom, QString("angle"),   &arc_a))  return;

	painter.save();
	setPainterStyle(dom, painter);

	QVector<qreal> arc;
	arc.push_back(arc_x);
	arc.push_back(arc_y);
	arc.push_back(arc_l);
	arc.push_back(arc_h);
	arc.push_back(arc_s);
	arc.push_back(arc_a);
	prim.m_arcs << arc;

	painter.drawArc(QRectF(arc_x, arc_y, arc_l, arc_h), (int)(arc_s * 16), (int)(arc_a * 16));
	painter.restore();
}

void ElementPictureFactory::parsePolygon(const QDomElement &dom, QPainter &painter, ElementPictureFactory::primitives &prim) const
{
	int i = 1;
	while(true) {
		if (QET::attributeIsAReal(dom, QString("x%1").arg(i)) && QET::attributeIsAReal(dom, QString("y%1").arg(i))) ++ i;
		else break;
	}
	if (i < 3) {
		return;
	}
	
	QVector<QPointF> points; // empty vector created instead of default initialized vector with i-1 elements.
	for (int j = 1 ; j < i ; ++ j) {
		points.insert(
			j - 1,
			QPointF(
				dom.attribute(QString("x%1").arg(j)).toDouble(),
				dom.attribute(QString("y%1").arg(j)).toDouble()
			)
		);
	}

	painter.save();
	setPainterStyle(dom, painter);
	if (dom.attribute("closed") == "false") painter.drawPolyline(points.data(), i-1);
	else {
		painter.drawPolygon(points.data(), i-1);

		// insert first point at the end again for DXF export.
		points.push_back(points[0]);
	}

	prim.m_polygons << points;

	painter.restore();
}

void ElementPictureFactory::parseText(const QDomElement &dom, QPainter &painter, ElementPictureFactory::primitives &prim) const
{
	Q_UNUSED(prim);
	
	qreal pos_x, pos_y;
	int size;
	if (!QET::attributeIsAReal(dom, "x", &pos_x) ||\
		!QET::attributeIsAReal(dom, "y", &pos_y) ||\
		!QET::attributeIsAnInteger(dom, "size", &size) ||\
		!dom.hasAttribute("text")) {
		return;
	}

	painter.save();
	setPainterStyle(dom, painter);

		//Get the font and metric
	QFont used_font = QETApp::diagramTextsFont(size);
	QFontMetrics qfm(used_font);
	QColor text_color = (dom.attribute("color") != "white"? Qt::black : Qt::white);

		//Instanciate a QTextDocument (like the QGraphicsTextItem class)
		//for generate the graphics rendering of the text
	QTextDocument text_document;
	text_document.setDefaultFont(used_font);
	text_document.setPlainText(dom.attribute("text"));

	painter.setTransform(QTransform(), false);
	painter.translate(pos_x, pos_y);

	qreal default_rotation_angle = 0.0;
	if (QET::attributeIsAReal(dom, "rotation", &default_rotation_angle)) {
		painter.rotate(default_rotation_angle);
	}

	/*
		Deplace le systeme de coordonnees du QPainter pour effectuer le rendu au
		bon endroit ; note : on soustrait l'ascent() de la police pour
		determiner le coin superieur gauche du texte alors que la position
		indiquee correspond a la baseline.
	*/
	QPointF qpainter_offset(0.0, -qfm.ascent());

		//adjusts the offset by the margin of the text document
	text_document.setDocumentMargin(0.0);

	painter.translate(qpainter_offset);

		// force the palette used to render the QTextDocument
	QAbstractTextDocumentLayout::PaintContext ctx;
	ctx.palette.setColor(QPalette::Text, text_color);
	text_document.documentLayout() -> draw(&painter, ctx);

	painter.restore();
}

/**
 * @brief ElementPictureFactory::setPainterStyle
 * apply the style store in dom to painter.
 * @param dom
 * @param painter
 */
void ElementPictureFactory::setPainterStyle(const QDomElement &dom, QPainter &painter) const
{
	QPen pen = painter.pen();
	QBrush brush = painter.brush();

	pen.setJoinStyle(Qt::BevelJoin);
	pen.setCapStyle(Qt::SquareCap);

		//Get the couples style/value
	const QStringList styles = dom.attribute("style").split(";", QString::SkipEmptyParts);

	QRegExp rx("^\\s*([a-z-]+)\\s*:\\s*([a-z-]+)\\s*$");
	for (QString style : styles) {
		if (rx.exactMatch(style)) {
			QString style_name = rx.cap(1);
			QString style_value = rx.cap(2);
			if (style_name == "line-style") {
				if (style_value == "dashed") pen.setStyle(Qt::DashLine);
				else if (style_value == "dotted") pen.setStyle(Qt::DotLine);
				else if (style_value == "dashdotted") pen.setStyle(Qt::DashDotLine);
				else if (style_value == "normal") pen.setStyle(Qt::SolidLine);
			} else if (style_name == "line-weight") {
				if (style_value == "none") pen.setColor(QColor(0, 0, 0, 0));
				else if (style_value == "thin") pen.setWidth(0);
				else if (style_value == "normal") pen.setWidthF(1.0);
				else if (style_value == "hight") pen.setWidthF(2.0);
				else if (style_value == "eleve") pen.setWidthF(5.0);

			} else if (style_name == "filling") {
				if (style_value == "white") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::white);
				} else if (style_value == "black") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::black);
				} else if (style_value == "blue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::blue);
				} else if (style_value == "red") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::red);
				} else if (style_value == "green") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::green);
				} else if (style_value == "gray") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::gray);
				} else if (style_value == "brun") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(97, 44, 0));
				} else if (style_value == "yellow") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::yellow);
				} else if (style_value == "cyan") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::cyan);
				} else if (style_value == "magenta") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::magenta);
				} else if (style_value == "lightgray") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(Qt::lightGray);
				} else if (style_value == "orange") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 128, 0));
				} else if (style_value == "purple") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(136, 28, 168));
				}else if (style_value == "hor") {
					brush.setStyle(Qt::HorPattern);
					brush.setColor(Qt::black);
				} else if (style_value == "ver") {
					brush.setStyle(Qt::VerPattern);
					brush.setColor(Qt::black);
				} else if (style_value == "bdiag") {
					brush.setStyle(Qt::BDiagPattern);
					brush.setColor(Qt::black);
				} else if (style_value == "fdiag") {
					brush.setStyle(Qt::FDiagPattern);
					brush.setColor(Qt::black);
				} else if (style_value == "none") {
					brush.setStyle(Qt::NoBrush);
				}
			} else if (style_name == "color") {
				if (style_value == "black") {
					pen.setColor(QColor(0, 0, 0, pen.color().alpha()));
				} else if (style_value == "white") {
					pen.setColor(QColor(255, 255, 255, pen.color().alpha()));
				} else if (style_value == "red") {
					pen.setColor(Qt::red);
				}else if (style_value == "blue") {
					pen.setColor(Qt::blue);
				}else if (style_value == "green") {
					pen.setColor(Qt::green);
				}else if (style_value == "gray") {
					pen.setColor(Qt::gray);
				}else if (style_value == "brun") {
					pen.setColor(QColor(97, 44, 0));
				}else if (style_value == "yellow") {
					pen.setColor(Qt::yellow);
				}else if (style_value == "cyan") {
					pen.setColor(Qt::cyan);
				}else if (style_value == "magenta") {
					pen.setColor(Qt::magenta);
				}else if (style_value == "lightgray") {
					pen.setColor(Qt::lightGray);
				}else if (style_value == "orange") {
					pen.setColor(QColor(255, 128, 0));
				}else if (style_value == "purple") {
					pen.setColor(QColor(136, 28, 168));
				} else if (style_value == "none") {
					pen.setBrush(Qt::transparent);
				}
			}
		}
	}

	painter.setPen(pen);
	painter.setBrush(brush);
}
