/*
	Copyright 2006-2020 The QElectroTech Team
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
#include <QGraphicsSimpleTextItem>
#include <QRegularExpression>

ElementPictureFactory* ElementPictureFactory::m_factory = nullptr;

/**
	@brief ElementPictureFactory::getPictures
	Set the picture of the element at location.
	Note, picture can be null
	@param location
	@param picture
	@param low_picture
*/
void ElementPictureFactory::getPictures(const ElementsLocation &location, QPicture &picture, QPicture &low_picture)
{
	if(!location.exist()) {
		return;
	}

	QUuid uuid = location.uuid();
	if(Q_UNLIKELY(uuid.isNull()))
	{
		build(location, &picture, &low_picture);
		return;
	}

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
	@brief ElementPictureFactory::pixmap
	@param location
	@return the pixmap of the element at location
	Note pixmap can be null
*/
QPixmap ElementPictureFactory::pixmap(const ElementsLocation &location)
{
	QUuid uuid = location.uuid();

	if (m_pixmap_H.contains(uuid)) {
		return m_pixmap_H.value(uuid);
	}

	if(build(location))
	{
		auto doc = location.pugiXml();
			//size
		int w = doc.document_element().attribute("width").as_int();
		int h = doc.document_element().attribute("height").as_int();
		while (w % 10) ++ w;
		while (h % 10) ++ h;
			//hotspot
		int hsx = qMin(doc.document_element().attribute("hotspot_x").as_int(), w);
		int hsy = qMin(doc.document_element().attribute("hotspot_y").as_int(), h);

		QPixmap pix(w, h);
		pix.fill(QColor(255, 255, 255, 0));

		QPainter painter(&pix);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
		painter.translate(hsx, hsy);
		painter.drawPicture(0, 0, m_pictures_H.value(uuid));

		if (!uuid.isNull()) {
			m_pixmap_H.insert(uuid, pix);
		}
		return pix;
	}

	return QPixmap();
}


/**
	@brief ElementPictureFactory::getPrimitives
	@param location
	@return The primtive used to draw the element at location
*/
ElementPictureFactory::primitives ElementPictureFactory::getPrimitives(
		const ElementsLocation &location)
{
	if(!m_primitives_H.contains(location.uuid()))
		build(location);

	return m_primitives_H.value(location.uuid());
}

ElementPictureFactory::~ElementPictureFactory()
{
	for (primitives p : m_primitives_H.values()) {
		qDeleteAll(p.m_texts);
	}
}

/**
	@brief ElementPictureFactory::build
	Build the picture from location.
	@param location
	@param picture
	@param low_picture
	if picture and/or low_picture are not null
	this function draw on it and don't store it.
	if null, this function create a QPicture for normal and low zoom,
	draw on it and store it in m_pictures_H and m_low_pictures_H
	@return
*/
bool ElementPictureFactory::build(const ElementsLocation &location,
				  QPicture *picture,
				  QPicture *low_picture)
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

	QPainter painter;
	QPicture pic;
	primitives primitives_;
	if (picture) {
		painter.begin(picture);
	}
	else {
		painter.begin(&pic);
	}
	painter.setRenderHint(QPainter::Antialiasing,         true);
	painter.setRenderHint(QPainter::TextAntialiasing,     true);
	painter.setRenderHint(QPainter::SmoothPixmapTransform,true);


	QPainter low_painter;
	QPicture low_pic;
	if (low_picture) {
		low_painter.begin(low_picture);
	}
	else {
		low_painter.begin(&low_pic);
	}
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

	if (!picture) {
		m_pictures_H.insert(location.uuid(), pic);
		m_primitives_H.insert(location.uuid(), primitives_);
	}
	if (!low_picture) {
		m_low_pictures_H.insert(location.uuid(), low_pic);
		m_primitives_H.insert(location.uuid(), primitives_);
	}
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
	Q_UNUSED(prim)

	if (dom.tagName() != "text") {
		return;
	}

	painter.save();
	setPainterStyle(dom, painter);

		//Get the font and metric
	QFont font_;
	if (dom.hasAttribute("size")) {
		font_ = QETApp::diagramTextsFont(dom.attribute("size").toDouble());
	}
	else if (dom.hasAttribute("font")) {
		font_.fromString(dom.attribute("font"));
	}

	QColor text_color(dom.attribute("color", "#000000"));

		//Instanciate a QTextDocument (like the QGraphicsTextItem class)
		//for generate the graphics rendering of the text
	QTextDocument text_document;
	text_document.setDefaultFont(font_);
	text_document.setPlainText(dom.attribute("text"));

	painter.setTransform(QTransform(), false);
	painter.translate(dom.attribute("x").toDouble(), dom.attribute("y").toDouble());
	painter.rotate(dom.attribute("rotation", "0").toDouble());

	/*
		Deplace le systeme de coordonnees du QPainter pour effectuer le rendu au
		bon endroit ; note : on soustrait l'ascent() de la police pour
		determiner le coin superieur gauche du texte alors que la position
		indiquee correspond a la baseline.
	*/
	QFontMetrics qfm(font_);
	QPointF qpainter_offset(0.0, -qfm.ascent());

		//adjusts the offset by the margin of the text document
	text_document.setDocumentMargin(0.0);

	painter.translate(qpainter_offset);

		// force the palette used to render the QTextDocument
	QAbstractTextDocumentLayout::PaintContext ctx;
	ctx.palette.setColor(QPalette::Text, text_color);
	text_document.documentLayout() -> draw(&painter, ctx);

		//A very dirty workaround for export this text to dxf
	QGraphicsSimpleTextItem *qgsti = new QGraphicsSimpleTextItem();
	qgsti->setText(dom.attribute("text"));
	qgsti->setFont(font_);
	qgsti->setPos(dom.attribute("x").toDouble(), dom.attribute("y").toDouble());
	qgsti->setRotation(dom.attribute("rotation", "0").toDouble());
	prim.m_texts << qgsti;

	painter.restore();
}

/**
	@brief ElementPictureFactory::setPainterStyle
	apply the style store in dom to painter.
	@param dom
	@param painter
*/
void ElementPictureFactory::setPainterStyle(const QDomElement &dom, QPainter &painter) const
{
	QPen pen = painter.pen();
	QBrush brush = painter.brush();

	pen.setJoinStyle(Qt::BevelJoin);
	pen.setCapStyle(Qt::SquareCap);

		//Get the couples style/value
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)	// ### Qt 6: remove
	const QStringList styles = dom.attribute("style").split(";", QString::SkipEmptyParts);
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 5.14 or later")
#endif
	const QStringList styles = dom.attribute("style").split(";", Qt::SkipEmptyParts);
#endif

	QRegularExpression rx("^(?<name>[a-z-]+):(?<value>[a-zA-Z-]+)$");
	if (!rx.isValid())
	{
		qWarning() <<QObject::tr("this is an error in the code")
			  << rx.errorString()
			  << rx.patternErrorOffset();
		return;
	}
	for (auto style : styles)
	{
		QRegularExpressionMatch match = rx.match(style);
		if (!match.hasMatch()) {
			qDebug() << "no Match" << style;
		}else {
			QString style_name = match.captured("name");
			QString style_value = match.captured("value");
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
				} else if (style_value == "HTMLPinkPink") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 192, 203));
				} else if (style_value == "HTMLPinkLightPink") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 182, 193));
				} else if (style_value == "HTMLPinkHotPink") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 105, 180));
				} else if (style_value == "HTMLPinkDeepPink") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 20, 147));
				} else if (style_value == "HTMLPinkPaleVioletRed") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(219, 112, 147));
				} else if (style_value == "HTMLPinkMediumVioletRed") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(199, 21, 133));
				} else if (style_value == "HTMLRedLightSalmon") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 160, 122));
				} else if (style_value == "HTMLRedSalmon") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(250, 128, 114));
				} else if (style_value == "HTMLRedDarkSalmon") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(233, 150, 122));
				} else if (style_value == "HTMLRedLightCoral") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(240, 128, 128));
				} else if (style_value == "HTMLRedIndianRed") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(205, 92, 92));
				} else if (style_value == "HTMLRedCrimson") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(220, 20, 60));
				} else if (style_value == "HTMLRedFirebrick") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(178, 34, 34));
				} else if (style_value == "HTMLRedDarkRed") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(139, 0, 0));
				} else if (style_value == "HTMLRedRed") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 0, 0));
				} else if (style_value == "HTMLOrangeOrangeRed") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 69, 0));
				} else if (style_value == "HTMLOrangeTomato") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 99, 71));
				} else if (style_value == "HTMLOrangeCoral") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 127, 80));
				} else if (style_value == "HTMLOrangeDarkOrange") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 140, 0));
				} else if (style_value == "HTMLOrangeOrange") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 165, 0));
				} else if (style_value == "HTMLYellowYellow") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 255, 0));
				} else if (style_value == "HTMLYellowLightYellow") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 255, 224));
				} else if (style_value == "HTMLYellowLemonChiffon") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 250, 205));
				} else if (style_value == "HTMLYellowLightGoldenrodYellow") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(250, 250, 210));
				} else if (style_value == "HTMLYellowPapayaWhip") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 239, 213));
				} else if (style_value == "HTMLYellowMoccasin") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 228, 181));
				} else if (style_value == "HTMLYellowPeachPuff") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 218, 185));
				} else if (style_value == "HTMLYellowPaleGoldenrod") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(238, 232, 170));
				} else if (style_value == "HTMLYellowKhaki") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(240, 230, 140));
				} else if (style_value == "HTMLYellowDarkKhaki") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(189, 183, 107));
				} else if (style_value == "HTMLYellowGold") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 215, 0));
				} else if (style_value == "HTMLBrownCornsilk") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 248, 220));
				} else if (style_value == "HTMLBrownBlanchedAlmond") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 235, 205));
				} else if (style_value == "HTMLBrownBisque") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 228, 196));
				} else if (style_value == "HTMLBrownNavajoWhite") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 222, 173));
				} else if (style_value == "HTMLBrownWheat") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(245, 222, 179));
				} else if (style_value == "HTMLBrownBurlywood") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(222, 184, 135));
				} else if (style_value == "HTMLBrownTan") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(210, 180, 140));
				} else if (style_value == "HTMLBrownRosyBrown") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(188, 143, 143));
				} else if (style_value == "HTMLBrownSandyBrown") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(244, 164, 96));
				} else if (style_value == "HTMLBrownGoldenrod") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(218, 165, 32));
				} else if (style_value == "HTMLBrownDarkGoldenrod") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(184, 134, 11));
				} else if (style_value == "HTMLBrownPeru") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(205, 133, 63));
				} else if (style_value == "HTMLBrownChocolate") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(210, 105, 30));
				} else if (style_value == "HTMLBrownSaddleBrown") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(139, 69, 19));
				} else if (style_value == "HTMLBrownSienna") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(160, 82, 45));
				} else if (style_value == "HTMLBrownBrown") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(165, 42, 42));
				} else if (style_value == "HTMLBrownMaroon") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(128, 0, 0));
				} else if (style_value == "HTMLGreenDarkOliveGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(85, 107, 47));
				} else if (style_value == "HTMLGreenOlive") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(128, 128, 0));
				} else if (style_value == "HTMLGreenOliveDrab") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(107, 142, 35));
				} else if (style_value == "HTMLGreenYellowGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(154, 205, 50));
				} else if (style_value == "HTMLGreenLimeGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(50, 205, 50));
				} else if (style_value == "HTMLGreenLime") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 255, 0));
				} else if (style_value == "HTMLGreenLawnGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(124, 252, 0));
				} else if (style_value == "HTMLGreenChartreuse") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(127, 255, 0));
				} else if (style_value == "HTMLGreenGreenYellow") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(173, 255, 47));
				} else if (style_value == "HTMLGreenSpringGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 255, 127));
				} else if (style_value == "HTMLGreenMediumSpringGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 250, 154));
				} else if (style_value == "HTMLGreenLightGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(144, 238, 144));
				} else if (style_value == "HTMLGreenPaleGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(152, 251, 152));
				} else if (style_value == "HTMLGreenDarkSeaGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(143, 188, 143));
				} else if (style_value == "HTMLGreenMediumAquamarine") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(102, 205, 170));
				} else if (style_value == "HTMLGreenMediumSeaGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(60, 179, 113));
				} else if (style_value == "HTMLGreenSeaGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(46, 139, 87));
				} else if (style_value == "HTMLGreenForestGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(34, 139, 34));
				} else if (style_value == "HTMLGreenGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 128, 0));
				} else if (style_value == "HTMLGreenDarkGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 100, 0));
				} else if (style_value == "HTMLCyanAqua") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 255, 255));
				} else if (style_value == "HTMLCyanCyan") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 255, 255));
				} else if (style_value == "HTMLCyanLightCyan") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(224, 255, 255));
				} else if (style_value == "HTMLCyanPaleTurquoise") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(175, 238, 238));
				} else if (style_value == "HTMLCyanAquamarine") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(127, 255, 212));
				} else if (style_value == "HTMLCyanTurquoise") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(64, 224, 208));
				} else if (style_value == "HTMLCyanMediumTurquoise") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(72, 209, 204));
				} else if (style_value == "HTMLCyanDarkTurquoise") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 206, 209));
				} else if (style_value == "HTMLCyanLightSeaGreen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(32, 178, 170));
				} else if (style_value == "HTMLCyanCadetBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(95, 158, 160));
				} else if (style_value == "HTMLCyanDarkCyan") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 139, 139));
				} else if (style_value == "HTMLCyanTeal") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 128, 128));
				} else if (style_value == "HTMLBlueLightSteelBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(176, 196, 222));
				} else if (style_value == "HTMLBluePowderBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(176, 224, 230));
				} else if (style_value == "HTMLBlueLightBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(173, 216, 230));
				} else if (style_value == "HTMLBlueSkyBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(135, 206, 235));
				} else if (style_value == "HTMLBlueLightSkyBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(135, 206, 250));
				} else if (style_value == "HTMLBlueDeepSkyBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 191, 255));
				} else if (style_value == "HTMLBlueDodgerBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(30, 144, 255));
				} else if (style_value == "HTMLBlueCornflowerBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(100, 149, 237));
				} else if (style_value == "HTMLBlueSteelBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(70, 130, 180));
				} else if (style_value == "HTMLBlueRoyalBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(65, 105, 225));
				} else if (style_value == "HTMLBlueBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 0, 255));
				} else if (style_value == "HTMLBlueMediumBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 0, 205));
				} else if (style_value == "HTMLBlueDarkBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 0, 139));
				} else if (style_value == "HTMLBlueNavy") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 0, 128));
				} else if (style_value == "HTMLBlueMidnightBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(25, 25, 112));
				} else if (style_value == "HTMLPurpleLavender") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(230, 230, 250));
				} else if (style_value == "HTMLPurpleThistle") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(216, 191, 216));
				} else if (style_value == "HTMLPurplePlum") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(221, 160, 221));
				} else if (style_value == "HTMLPurpleViolet") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(238, 130, 238));
				} else if (style_value == "HTMLPurpleOrchid") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(218, 112, 214));
				} else if (style_value == "HTMLPurpleFuchsia") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 0, 255));
				} else if (style_value == "HTMLPurpleMagenta") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 0, 255));
				} else if (style_value == "HTMLPurpleMediumOrchid") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(186, 85, 211));
				} else if (style_value == "HTMLPurpleMediumPurple") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(147, 112, 219));
				} else if (style_value == "HTMLPurpleBlueViolet") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(138, 43, 226));
				} else if (style_value == "HTMLPurpleDarkViolet") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(148, 0, 211));
				} else if (style_value == "HTMLPurpleDarkOrchid") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(153, 50, 204));
				} else if (style_value == "HTMLPurpleDarkMagenta") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(139, 0, 139));
				} else if (style_value == "HTMLPurplePurple") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(128, 0, 128));
				} else if (style_value == "HTMLPurpleIndigo") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(75, 0, 130));
				} else if (style_value == "HTMLPurpleDarkSlateBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(72, 61, 139));
				} else if (style_value == "HTMLPurpleSlateBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(106, 90, 205));
				} else if (style_value == "HTMLPurpleMediumSlateBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(123, 104, 238));
				} else if (style_value == "HTMLWhiteWhite") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 255, 255));
				} else if (style_value == "HTMLWhiteSnow") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 250, 250));
				} else if (style_value == "HTMLWhiteHoneydew") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(240, 255, 240));
				} else if (style_value == "HTMLWhiteMintCream") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(245, 255, 250));
				} else if (style_value == "HTMLWhiteAzure") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(240, 255, 255));
				} else if (style_value == "HTMLWhiteAliceBlue") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(240, 248, 255));
				} else if (style_value == "HTMLWhiteGhostWhite") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(248, 248, 255));
				} else if (style_value == "HTMLWhiteWhiteSmoke") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(245, 245, 245));
				} else if (style_value == "HTMLWhiteSeashell") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 245, 238));
				} else if (style_value == "HTMLWhiteBeige") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(245, 245, 220));
				} else if (style_value == "HTMLWhiteOldLace") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(253, 245, 230));
				} else if (style_value == "HTMLWhiteFloralWhite") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 250, 240));
				} else if (style_value == "HTMLWhiteIvory") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 255, 240));
				} else if (style_value == "HTMLWhiteAntiqueWhite") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(250, 235, 215));
				} else if (style_value == "HTMLWhiteLinen") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(250, 240, 230));
				} else if (style_value == "HTMLWhiteLavenderBlush") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 240, 245));
				} else if (style_value == "HTMLWhiteMistyRose") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(255, 228, 225));
				} else if (style_value == "HTMLGrayGainsboro") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(220, 220, 220));
				} else if (style_value == "HTMLGrayLightGray") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(211, 211, 211));
				} else if (style_value == "HTMLGraySilver") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(192, 192, 192));
				} else if (style_value == "HTMLGrayDarkGray") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(169, 169, 169));
				} else if (style_value == "HTMLGrayGray") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(128, 128, 128));
				} else if (style_value == "HTMLGrayDimGray") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(105, 105, 105));
				} else if (style_value == "HTMLGrayLightSlateGray") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(119, 136, 153));
				} else if (style_value == "HTMLGraySlateGray") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(112, 128, 144));
				} else if (style_value == "HTMLGrayDarkSlateGray") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(47, 79, 79));
				} else if (style_value == "HTMLGrayBlack") {
					brush.setStyle(Qt::SolidPattern);
					brush.setColor(QColor(0, 0, 0));
				} else if (style_value == "hor") {
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
				} else if (style_value == "blue") {
					pen.setColor(Qt::blue);
				} else if (style_value == "green") {
					pen.setColor(Qt::green);
				} else if (style_value == "gray") {
					pen.setColor(Qt::gray);
				} else if (style_value == "brun") {
					pen.setColor(QColor(97, 44, 0));
				} else if (style_value == "yellow") {
					pen.setColor(Qt::yellow);
				} else if (style_value == "cyan") {
					pen.setColor(Qt::cyan);
				} else if (style_value == "magenta") {
					pen.setColor(Qt::magenta);
				} else if (style_value == "lightgray") {
					pen.setColor(Qt::lightGray);
				} else if (style_value == "orange") {
					pen.setColor(QColor(255, 128, 0));
				} else if (style_value == "purple") {
					pen.setColor(QColor(136, 28, 168));
				} else if (style_value == "HTMLPinkPink") {
					pen.setColor(QColor(255, 192, 203));
				} else if (style_value == "HTMLPinkLightPink") {
					pen.setColor(QColor(255, 182, 193));
				} else if (style_value == "HTMLPinkHotPink") {
					pen.setColor(QColor(255, 105, 180));
				} else if (style_value == "HTMLPinkDeepPink") {
					pen.setColor(QColor(255, 20, 147));
				} else if (style_value == "HTMLPinkPaleVioletRed") {
					pen.setColor(QColor(219, 112, 147));
				} else if (style_value == "HTMLPinkMediumVioletRed") {
					pen.setColor(QColor(199, 21, 133));
				} else if (style_value == "HTMLRedLightSalmon") {
					pen.setColor(QColor(255, 160, 122));
				} else if (style_value == "HTMLRedSalmon") {
					pen.setColor(QColor(250, 128, 114));
				} else if (style_value == "HTMLRedDarkSalmon") {
					pen.setColor(QColor(233, 150, 122));
				} else if (style_value == "HTMLRedLightCoral") {
					pen.setColor(QColor(240, 128, 128));
				} else if (style_value == "HTMLRedIndianRed") {
					pen.setColor(QColor(205, 92, 92));
				} else if (style_value == "HTMLRedCrimson") {
					pen.setColor(QColor(220, 20, 60));
				} else if (style_value == "HTMLRedFirebrick") {
					pen.setColor(QColor(178, 34, 34));
				} else if (style_value == "HTMLRedDarkRed") {
					pen.setColor(QColor(139, 0, 0));
				} else if (style_value == "HTMLRedRed") {
					pen.setColor(QColor(255, 0, 0));
				} else if (style_value == "HTMLOrangeOrangeRed") {
					pen.setColor(QColor(255, 69, 0));
				} else if (style_value == "HTMLOrangeTomato") {
					pen.setColor(QColor(255, 99, 71));
				} else if (style_value == "HTMLOrangeCoral") {
					pen.setColor(QColor(255, 127, 80));
				} else if (style_value == "HTMLOrangeDarkOrange") {
					pen.setColor(QColor(255, 140, 0));
				} else if (style_value == "HTMLOrangeOrange") {
					pen.setColor(QColor(255, 165, 0));
				} else if (style_value == "HTMLYellowYellow") {
					pen.setColor(QColor(255, 255, 0));
				} else if (style_value == "HTMLYellowLightYellow") {
					pen.setColor(QColor(255, 255, 224));
				} else if (style_value == "HTMLYellowLemonChiffon") {
					pen.setColor(QColor(255, 250, 205));
				} else if (style_value == "HTMLYellowLightGoldenrodYellow") {
					pen.setColor(QColor(250, 250, 210));
				} else if (style_value == "HTMLYellowPapayaWhip") {
					pen.setColor(QColor(255, 239, 213));
				} else if (style_value == "HTMLYellowMoccasin") {
					pen.setColor(QColor(255, 228, 181));
				} else if (style_value == "HTMLYellowPeachPuff") {
					pen.setColor(QColor(255, 218, 185));
				} else if (style_value == "HTMLYellowPaleGoldenrod") {
					pen.setColor(QColor(238, 232, 170));
				} else if (style_value == "HTMLYellowKhaki") {
					pen.setColor(QColor(240, 230, 140));
				} else if (style_value == "HTMLYellowDarkKhaki") {
					pen.setColor(QColor(189, 183, 107));
				} else if (style_value == "HTMLYellowGold") {
					pen.setColor(QColor(255, 215, 0));
				} else if (style_value == "HTMLBrownCornsilk") {
					pen.setColor(QColor(255, 248, 220));
				} else if (style_value == "HTMLBrownBlanchedAlmond") {
					pen.setColor(QColor(255, 235, 205));
				} else if (style_value == "HTMLBrownBisque") {
					pen.setColor(QColor(255, 228, 196));
				} else if (style_value == "HTMLBrownNavajoWhite") {
					pen.setColor(QColor(255, 222, 173));
				} else if (style_value == "HTMLBrownWheat") {
					pen.setColor(QColor(245, 222, 179));
				} else if (style_value == "HTMLBrownBurlywood") {
					pen.setColor(QColor(222, 184, 135));
				} else if (style_value == "HTMLBrownTan") {
					pen.setColor(QColor(210, 180, 140));
				} else if (style_value == "HTMLBrownRosyBrown") {
					pen.setColor(QColor(188, 143, 143));
				} else if (style_value == "HTMLBrownSandyBrown") {
					pen.setColor(QColor(244, 164, 96));
				} else if (style_value == "HTMLBrownGoldenrod") {
					pen.setColor(QColor(218, 165, 32));
				} else if (style_value == "HTMLBrownDarkGoldenrod") {
					pen.setColor(QColor(184, 134, 11));
				} else if (style_value == "HTMLBrownPeru") {
					pen.setColor(QColor(205, 133, 63));
				} else if (style_value == "HTMLBrownChocolate") {
					pen.setColor(QColor(210, 105, 30));
				} else if (style_value == "HTMLBrownSaddleBrown") {
					pen.setColor(QColor(139, 69, 19));
				} else if (style_value == "HTMLBrownSienna") {
					pen.setColor(QColor(160, 82, 45));
				} else if (style_value == "HTMLBrownBrown") {
					pen.setColor(QColor(165, 42, 42));
				} else if (style_value == "HTMLBrownMaroon") {
					pen.setColor(QColor(128, 0, 0));
				} else if (style_value == "HTMLGreenDarkOliveGreen") {
					pen.setColor(QColor(85, 107, 47));
				} else if (style_value == "HTMLGreenOlive") {
					pen.setColor(QColor(128, 128, 0));
				} else if (style_value == "HTMLGreenOliveDrab") {
					pen.setColor(QColor(107, 142, 35));
				} else if (style_value == "HTMLGreenYellowGreen") {
					pen.setColor(QColor(154, 205, 50));
				} else if (style_value == "HTMLGreenLimeGreen") {
					pen.setColor(QColor(50, 205, 50));
				} else if (style_value == "HTMLGreenLime") {
					pen.setColor(QColor(0, 255, 0));
				} else if (style_value == "HTMLGreenLawnGreen") {
					pen.setColor(QColor(124, 252, 0));
				} else if (style_value == "HTMLGreenChartreuse") {
					pen.setColor(QColor(127, 255, 0));
				} else if (style_value == "HTMLGreenGreenYellow") {
					pen.setColor(QColor(173, 255, 47));
				} else if (style_value == "HTMLGreenSpringGreen") {
					pen.setColor(QColor(0, 255, 127));
				} else if (style_value == "HTMLGreenMediumSpringGreen") {
					pen.setColor(QColor(0, 250, 154));
				} else if (style_value == "HTMLGreenLightGreen") {
					pen.setColor(QColor(144, 238, 144));
				} else if (style_value == "HTMLGreenPaleGreen") {
					pen.setColor(QColor(152, 251, 152));
				} else if (style_value == "HTMLGreenDarkSeaGreen") {
					pen.setColor(QColor(143, 188, 143));
				} else if (style_value == "HTMLGreenMediumAquamarine") {
					pen.setColor(QColor(102, 205, 170));
				} else if (style_value == "HTMLGreenMediumSeaGreen") {
					pen.setColor(QColor(60, 179, 113));
				} else if (style_value == "HTMLGreenSeaGreen") {
					pen.setColor(QColor(46, 139, 87));
				} else if (style_value == "HTMLGreenForestGreen") {
					pen.setColor(QColor(34, 139, 34));
				} else if (style_value == "HTMLGreenGreen") {
					pen.setColor(QColor(0, 128, 0));
				} else if (style_value == "HTMLGreenDarkGreen") {
					pen.setColor(QColor(0, 100, 0));
				} else if (style_value == "HTMLCyanAqua") {
					pen.setColor(QColor(0, 255, 255));
				} else if (style_value == "HTMLCyanCyan") {
					pen.setColor(QColor(0, 255, 255));
				} else if (style_value == "HTMLCyanLightCyan") {
					pen.setColor(QColor(224, 255, 255));
				} else if (style_value == "HTMLCyanPaleTurquoise") {
					pen.setColor(QColor(175, 238, 238));
				} else if (style_value == "HTMLCyanAquamarine") {
					pen.setColor(QColor(127, 255, 212));
				} else if (style_value == "HTMLCyanTurquoise") {
					pen.setColor(QColor(64, 224, 208));
				} else if (style_value == "HTMLCyanMediumTurquoise") {
					pen.setColor(QColor(72, 209, 204));
				} else if (style_value == "HTMLCyanDarkTurquoise") {
					pen.setColor(QColor(0, 206, 209));
				} else if (style_value == "HTMLCyanLightSeaGreen") {
					pen.setColor(QColor(32, 178, 170));
				} else if (style_value == "HTMLCyanCadetBlue") {
					pen.setColor(QColor(95, 158, 160));
				} else if (style_value == "HTMLCyanDarkCyan") {
					pen.setColor(QColor(0, 139, 139));
				} else if (style_value == "HTMLCyanTeal") {
					pen.setColor(QColor(0, 128, 128));
				} else if (style_value == "HTMLBlueLightSteelBlue") {
					pen.setColor(QColor(176, 196, 222));
				} else if (style_value == "HTMLBluePowderBlue") {
					pen.setColor(QColor(176, 224, 230));
				} else if (style_value == "HTMLBlueLightBlue") {
					pen.setColor(QColor(173, 216, 230));
				} else if (style_value == "HTMLBlueSkyBlue") {
					pen.setColor(QColor(135, 206, 235));
				} else if (style_value == "HTMLBlueLightSkyBlue") {
					pen.setColor(QColor(135, 206, 250));
				} else if (style_value == "HTMLBlueDeepSkyBlue") {
					pen.setColor(QColor(0, 191, 255));
				} else if (style_value == "HTMLBlueDodgerBlue") {
					pen.setColor(QColor(30, 144, 255));
				} else if (style_value == "HTMLBlueCornflowerBlue") {
					pen.setColor(QColor(100, 149, 237));
				} else if (style_value == "HTMLBlueSteelBlue") {
					pen.setColor(QColor(70, 130, 180));
				} else if (style_value == "HTMLBlueRoyalBlue") {
					pen.setColor(QColor(65, 105, 225));
				} else if (style_value == "HTMLBlueBlue") {
					pen.setColor(QColor(0, 0, 255));
				} else if (style_value == "HTMLBlueMediumBlue") {
					pen.setColor(QColor(0, 0, 205));
				} else if (style_value == "HTMLBlueDarkBlue") {
					pen.setColor(QColor(0, 0, 139));
				} else if (style_value == "HTMLBlueNavy") {
					pen.setColor(QColor(0, 0, 128));
				} else if (style_value == "HTMLBlueMidnightBlue") {
					pen.setColor(QColor(25, 25, 112));
				} else if (style_value == "HTMLPurpleLavender") {
					pen.setColor(QColor(230, 230, 250));
				} else if (style_value == "HTMLPurpleThistle") {
					pen.setColor(QColor(216, 191, 216));
				} else if (style_value == "HTMLPurplePlum") {
					pen.setColor(QColor(221, 160, 221));
				} else if (style_value == "HTMLPurpleViolet") {
					pen.setColor(QColor(238, 130, 238));
				} else if (style_value == "HTMLPurpleOrchid") {
					pen.setColor(QColor(218, 112, 214));
				} else if (style_value == "HTMLPurpleFuchsia") {
					pen.setColor(QColor(255, 0, 255));
				} else if (style_value == "HTMLPurpleMagenta") {
					pen.setColor(QColor(255, 0, 255));
				} else if (style_value == "HTMLPurpleMediumOrchid") {
					pen.setColor(QColor(186, 85, 211));
				} else if (style_value == "HTMLPurpleMediumPurple") {
					pen.setColor(QColor(147, 112, 219));
				} else if (style_value == "HTMLPurpleBlueViolet") {
					pen.setColor(QColor(138, 43, 226));
				} else if (style_value == "HTMLPurpleDarkViolet") {
					pen.setColor(QColor(148, 0, 211));
				} else if (style_value == "HTMLPurpleDarkOrchid") {
					pen.setColor(QColor(153, 50, 204));
				} else if (style_value == "HTMLPurpleDarkMagenta") {
					pen.setColor(QColor(139, 0, 139));
				} else if (style_value == "HTMLPurplePurple") {
					pen.setColor(QColor(128, 0, 128));
				} else if (style_value == "HTMLPurpleIndigo") {
					pen.setColor(QColor(75, 0, 130));
				} else if (style_value == "HTMLPurpleDarkSlateBlue") {
					pen.setColor(QColor(72, 61, 139));
				} else if (style_value == "HTMLPurpleSlateBlue") {
					pen.setColor(QColor(106, 90, 205));
				} else if (style_value == "HTMLPurpleMediumSlateBlue") {
					pen.setColor(QColor(123, 104, 238));
				} else if (style_value == "HTMLWhiteWhite") {
					pen.setColor(QColor(255, 255, 255));
				} else if (style_value == "HTMLWhiteSnow") {
					pen.setColor(QColor(255, 250, 250));
				} else if (style_value == "HTMLWhiteHoneydew") {
					pen.setColor(QColor(240, 255, 240));
				} else if (style_value == "HTMLWhiteMintCream") {
					pen.setColor(QColor(245, 255, 250));
				} else if (style_value == "HTMLWhiteAzure") {
					pen.setColor(QColor(240, 255, 255));
				} else if (style_value == "HTMLWhiteAliceBlue") {
					pen.setColor(QColor(240, 248, 255));
				} else if (style_value == "HTMLWhiteGhostWhite") {
					pen.setColor(QColor(248, 248, 255));
				} else if (style_value == "HTMLWhiteWhiteSmoke") {
					pen.setColor(QColor(245, 245, 245));
				} else if (style_value == "HTMLWhiteSeashell") {
					pen.setColor(QColor(255, 245, 238));
				} else if (style_value == "HTMLWhiteBeige") {
					pen.setColor(QColor(245, 245, 220));
				} else if (style_value == "HTMLWhiteOldLace") {
					pen.setColor(QColor(253, 245, 230));
				} else if (style_value == "HTMLWhiteFloralWhite") {
					pen.setColor(QColor(255, 250, 240));
				} else if (style_value == "HTMLWhiteIvory") {
					pen.setColor(QColor(255, 255, 240));
				} else if (style_value == "HTMLWhiteAntiqueWhite") {
					pen.setColor(QColor(250, 235, 215));
				} else if (style_value == "HTMLWhiteLinen") {
					pen.setColor(QColor(250, 240, 230));
				} else if (style_value == "HTMLWhiteLavenderBlush") {
					pen.setColor(QColor(255, 240, 245));
				} else if (style_value == "HTMLWhiteMistyRose") {
					pen.setColor(QColor(255, 228, 225));
				} else if (style_value == "HTMLGrayGainsboro") {
					pen.setColor(QColor(220, 220, 220));
				} else if (style_value == "HTMLGrayLightGray") {
					pen.setColor(QColor(211, 211, 211));
				} else if (style_value == "HTMLGraySilver") {
					pen.setColor(QColor(192, 192, 192));
				} else if (style_value == "HTMLGrayDarkGray") {
					pen.setColor(QColor(169, 169, 169));
				} else if (style_value == "HTMLGrayGray") {
					pen.setColor(QColor(128, 128, 128));
				} else if (style_value == "HTMLGrayDimGray") {
					pen.setColor(QColor(105, 105, 105));
				} else if (style_value == "HTMLGrayLightSlateGray") {
					pen.setColor(QColor(119, 136, 153));
				} else if (style_value == "HTMLGraySlateGray") {
					pen.setColor(QColor(112, 128, 144));
				} else if (style_value == "HTMLGrayDarkSlateGray") {
					pen.setColor(QColor(47, 79, 79));
				} else if (style_value == "HTMLGrayBlack") {
					pen.setColor(QColor(0, 0, 0));
				} else if (style_value == "none") {
					pen.setBrush(Qt::transparent);
				}
			}
		}
	}

	painter.setPen(pen);
	painter.setBrush(brush);
}
