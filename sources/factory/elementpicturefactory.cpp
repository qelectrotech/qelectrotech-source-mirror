/*
	Copyright 2006-2026 The QElectroTech Team
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

#include "../ElementsCollection/elementslocation.h"
#include "../editor/graphicspart/partline.h"
#include "../qetapp.h"
#include "../qetversion.h"

#include <QAbstractTextDocumentLayout>
#include <QDomElement>
#include <QGraphicsSimpleTextItem>
#include <QPainter>
#include <QPicture>
#include <QRegularExpression>
#include <QTextDocument>
#include <iostream>

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

		//Check if the current version can read the xml description
	const auto elmt_version = QetVersion::fromXmlAttribute(dom);
	if (!elmt_version.isNull()
		&& QetVersion::currentVersion() < elmt_version)
	{
		std::cerr << qPrintable(
						 QObject::tr("Avertissement : l'élément "
									 " a été enregistré avec une version"
									 " ultérieure de QElectroTech.")
						 ) << std::endl;
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

	const auto uuid_ = location.uuid();
	if (!picture) {
		m_pictures_H.insert(uuid_, pic);
		m_primitives_H.insert(uuid_, primitives_);
	}
	if (!low_picture) {
		m_low_pictures_H.insert(uuid_, low_pic);
		m_primitives_H.insert(uuid_, primitives_);
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

		//Check if we must draw extremity
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

			//Adjust the beginning according to the width of the pen
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

		//Instantiate a QTextDocument (like the QGraphicsTextItem class)
		//for generate the graphics rendering of the text
	QTextDocument text_document;
	text_document.setDefaultFont(font_);
	text_document.setPlainText(dom.attribute("text"));

	painter.setTransform(QTransform(), false);
	painter.translate(dom.attribute("x").toDouble(), dom.attribute("y").toDouble());
	painter.rotate(dom.attribute("rotation", "0").toDouble());

	/*
		Moves the QPainter's coordinate system to render in the right place;
		note: the font's ascent() is subtracted to determine the top left
		corner of the text, whereas the position indicated corresponds
		to the baseline.
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
				else if (style_value == "thin") pen.setWidthF(0.5);
				else if (style_value == "normal") pen.setWidthF(1.0);
				else if (style_value == "hight") pen.setWidthF(2.0);
				else if (style_value == "eleve") pen.setWidthF(5.0);

			} else if (style_name == "filling") {
				static const QMap<QString, QPair<Qt::BrushStyle, QColor>>
					filling_style_map = {
						{"white", {Qt::SolidPattern, Qt::white}},
						{"black", {Qt::SolidPattern, Qt::black}},
						{"blue", {Qt::SolidPattern, Qt::blue}},
						{"red", {Qt::SolidPattern, Qt::red}},
						{"green", {Qt::SolidPattern, Qt::green}},
						{"gray", {Qt::SolidPattern, Qt::gray}},
						{"brun", {Qt::SolidPattern, QColor(97, 44, 0)}},
						{"yellow", {Qt::SolidPattern, Qt::yellow}},
						{"cyan", {Qt::SolidPattern, Qt::cyan}},
						{"magenta", {Qt::SolidPattern, Qt::magenta}},
						{"lightgray", {Qt::SolidPattern, Qt::lightGray}},
						{"orange", {Qt::SolidPattern, QColor(255, 128, 0)}},
						{"purple", {Qt::SolidPattern, QColor(136, 28, 168)}},
						{"HTMLPinkPink",
						 {Qt::SolidPattern, QColor(255, 192, 203)}},
						{"HTMLPinkLightPink",
						 {Qt::SolidPattern, QColor(255, 182, 193)}},
						{"HTMLPinkHotPink",
						 {Qt::SolidPattern, QColor(255, 105, 180)}},
						{"HTMLPinkDeepPink",
						 {Qt::SolidPattern, QColor(255, 20, 147)}},
						{"HTMLPinkPaleVioletRed",
						 {Qt::SolidPattern, QColor(219, 112, 147)}},
						{"HTMLPinkMediumVioletRed",
						 {Qt::SolidPattern, QColor(199, 21, 133)}},
						{"HTMLRedLightSalmon",
						 {Qt::SolidPattern, QColor(255, 160, 122)}},
						{"HTMLRedSalmon",
						 {Qt::SolidPattern, QColor(250, 128, 114)}},
						{"HTMLRedDarkSalmon",
						 {Qt::SolidPattern, QColor(233, 150, 122)}},
						{"HTMLRedLightCoral",
						 {Qt::SolidPattern, QColor(240, 128, 128)}},
						{"HTMLRedIndianRed",
						 {Qt::SolidPattern, QColor(205, 92, 92)}},
						{"HTMLRedCrimson",
						 {Qt::SolidPattern, QColor(220, 20, 60)}},
						{"HTMLRedFirebrick",
						 {Qt::SolidPattern, QColor(178, 34, 34)}},
						{"HTMLRedDarkRed",
						 {Qt::SolidPattern, QColor(139, 0, 0)}},
						{"HTMLRedRed", {Qt::SolidPattern, QColor(255, 0, 0)}},
						{"HTMLOrangeOrangeRed",
						 {Qt::SolidPattern, QColor(255, 69, 0)}},
						{"HTMLOrangeTomato",
						 {Qt::SolidPattern, QColor(255, 99, 71)}},
						{"HTMLOrangeCoral",
						 {Qt::SolidPattern, QColor(255, 127, 80)}},
						{"HTMLOrangeDarkOrange",
						 {Qt::SolidPattern, QColor(255, 140, 0)}},
						{"HTMLOrangeOrange",
						 {Qt::SolidPattern, QColor(255, 165, 0)}},
						{"HTMLYellowYellow",
						 {Qt::SolidPattern, QColor(255, 255, 0)}},
						{"HTMLYellowLightYellow",
						 {Qt::SolidPattern, QColor(255, 255, 224)}},
						{"HTMLYellowLemonChiffon",
						 {Qt::SolidPattern, QColor(255, 250, 205)}},
						{"HTMLYellowLightGoldenrodYellow",
						 {Qt::SolidPattern, QColor(250, 250, 210)}},
						{"HTMLYellowPapayaWhip",
						 {Qt::SolidPattern, QColor(255, 239, 213)}},
						{"HTMLYellowMoccasin",
						 {Qt::SolidPattern, QColor(255, 228, 181)}},
						{"HTMLYellowPeachPuff",
						 {Qt::SolidPattern, QColor(255, 218, 185)}},
						{"HTMLYellowPaleGoldenrod",
						 {Qt::SolidPattern, QColor(238, 232, 170)}},
						{"HTMLYellowKhaki",
						 {Qt::SolidPattern, QColor(240, 230, 140)}},
						{"HTMLYellowDarkKhaki",
						 {Qt::SolidPattern, QColor(189, 183, 107)}},
						{"HTMLYellowGold",
						 {Qt::SolidPattern, QColor(255, 215, 0)}},
						{"HTMLBrownCornsilk",
						 {Qt::SolidPattern, QColor(255, 248, 220)}},
						{"HTMLBrownBlanchedAlmond",
						 {Qt::SolidPattern, QColor(255, 235, 205)}},
						{"HTMLBrownBisque",
						 {Qt::SolidPattern, QColor(255, 228, 196)}},
						{"HTMLBrownNavajoWhite",
						 {Qt::SolidPattern, QColor(255, 222, 173)}},
						{"HTMLBrownWheat",
						 {Qt::SolidPattern, QColor(245, 222, 179)}},
						{"HTMLBrownBurlywood",
						 {Qt::SolidPattern, QColor(222, 184, 135)}},
						{"HTMLBrownTan",
						 {Qt::SolidPattern, QColor(210, 180, 140)}},
						{"HTMLBrownRosyBrown",
						 {Qt::SolidPattern, QColor(188, 143, 143)}},
						{"HTMLBrownSandyBrown",
						 {Qt::SolidPattern, QColor(244, 164, 96)}},
						{"HTMLBrownGoldenrod",
						 {Qt::SolidPattern, QColor(218, 165, 32)}},
						{"HTMLBrownDarkGoldenrod",
						 {Qt::SolidPattern, QColor(184, 134, 11)}},
						{"HTMLBrownPeru",
						 {Qt::SolidPattern, QColor(205, 133, 63)}},
						{"HTMLBrownChocolate",
						 {Qt::SolidPattern, QColor(210, 105, 30)}},
						{"HTMLBrownSaddleBrown",
						 {Qt::SolidPattern, QColor(139, 69, 19)}},
						{"HTMLBrownSienna",
						 {Qt::SolidPattern, QColor(160, 82, 45)}},
						{"HTMLBrownBrown",
						 {Qt::SolidPattern, QColor(165, 42, 42)}},
						{"HTMLBrownMaroon",
						 {Qt::SolidPattern, QColor(128, 0, 0)}},
						{"HTMLGreenDarkOliveGreen",
						 {Qt::SolidPattern, QColor(85, 107, 47)}},
						{"HTMLGreenOlive",
						 {Qt::SolidPattern, QColor(128, 128, 0)}},
						{"HTMLGreenOliveDrab",
						 {Qt::SolidPattern, QColor(107, 142, 35)}},
						{"HTMLGreenYellowGreen",
						 {Qt::SolidPattern, QColor(154, 205, 50)}},
						{"HTMLGreenLimeGreen",
						 {Qt::SolidPattern, QColor(50, 205, 50)}},
						{"HTMLGreenLime",
						 {Qt::SolidPattern, QColor(0, 255, 0)}},
						{"HTMLGreenLawnGreen",
						 {Qt::SolidPattern, QColor(124, 252, 0)}},
						{"HTMLGreenChartreuse",
						 {Qt::SolidPattern, QColor(127, 255, 0)}},
						{"HTMLGreenGreenYellow",
						 {Qt::SolidPattern, QColor(173, 255, 47)}},
						{"HTMLGreenSpringGreen",
						 {Qt::SolidPattern, QColor(0, 255, 127)}},
						{"HTMLGreenMediumSpringGreen",
						 {Qt::SolidPattern, QColor(0, 250, 154)}},
						{"HTMLGreenLightGreen",
						 {Qt::SolidPattern, QColor(144, 238, 144)}},
						{"HTMLGreenPaleGreen",
						 {Qt::SolidPattern, QColor(152, 251, 152)}},
						{"HTMLGreenDarkSeaGreen",
						 {Qt::SolidPattern, QColor(143, 188, 143)}},
						{"HTMLGreenMediumAquamarine",
						 {Qt::SolidPattern, QColor(102, 205, 170)}},
						{"HTMLGreenMediumSeaGreen",
						 {Qt::SolidPattern, QColor(60, 179, 113)}},
						{"HTMLGreenSeaGreen",
						 {Qt::SolidPattern, QColor(46, 139, 87)}},
						{"HTMLGreenForestGreen",
						 {Qt::SolidPattern, QColor(34, 139, 34)}},
						{"HTMLGreenGreen",
						 {Qt::SolidPattern, QColor(0, 128, 0)}},
						{"HTMLGreenDarkGreen",
						 {Qt::SolidPattern, QColor(0, 100, 0)}},
						{"HTMLCyanAqua",
						 {Qt::SolidPattern, QColor(0, 255, 255)}},
						{"HTMLCyanCyan",
						 {Qt::SolidPattern, QColor(0, 255, 255)}},
						{"HTMLCyanLightCyan",
						 {Qt::SolidPattern, QColor(224, 255, 255)}},
						{"HTMLCyanPaleTurquoise",
						 {Qt::SolidPattern, QColor(175, 238, 238)}},
						{"HTMLCyanAquamarine",
						 {Qt::SolidPattern, QColor(127, 255, 212)}},
						{"HTMLCyanTurquoise",
						 {Qt::SolidPattern, QColor(64, 224, 208)}},
						{"HTMLCyanMediumTurquoise",
						 {Qt::SolidPattern, QColor(72, 209, 204)}},
						{"HTMLCyanDarkTurquoise",
						 {Qt::SolidPattern, QColor(0, 206, 209)}},
						{"HTMLCyanLightSeaGreen",
						 {Qt::SolidPattern, QColor(32, 178, 170)}},
						{"HTMLCyanCadetBlue",
						 {Qt::SolidPattern, QColor(95, 158, 160)}},
						{"HTMLCyanDarkCyan",
						 {Qt::SolidPattern, QColor(0, 139, 139)}},
						{"HTMLCyanTeal",
						 {Qt::SolidPattern, QColor(0, 128, 128)}},
						{"HTMLBlueLightSteelBlue",
						 {Qt::SolidPattern, QColor(176, 196, 222)}},
						{"HTMLBluePowderBlue",
						 {Qt::SolidPattern, QColor(176, 224, 230)}},
						{"HTMLBlueLightBlue",
						 {Qt::SolidPattern, QColor(173, 216, 230)}},
						{"HTMLBlueSkyBlue",
						 {Qt::SolidPattern, QColor(135, 206, 235)}},
						{"HTMLBlueLightSkyBlue",
						 {Qt::SolidPattern, QColor(135, 206, 250)}},
						{"HTMLBlueDeepSkyBlue",
						 {Qt::SolidPattern, QColor(0, 191, 255)}},
						{"HTMLBlueDodgerBlue",
						 {Qt::SolidPattern, QColor(30, 144, 255)}},
						{"HTMLBlueCornflowerBlue",
						 {Qt::SolidPattern, QColor(100, 149, 237)}},
						{"HTMLBlueSteelBlue",
						 {Qt::SolidPattern, QColor(70, 130, 180)}},
						{"HTMLBlueRoyalBlue",
						 {Qt::SolidPattern, QColor(65, 105, 225)}},
						{"HTMLBlueBlue", {Qt::SolidPattern, QColor(0, 0, 255)}},
						{"HTMLBlueMediumBlue",
						 {Qt::SolidPattern, QColor(0, 0, 205)}},
						{"HTMLBlueDarkBlue",
						 {Qt::SolidPattern, QColor(0, 0, 139)}},
						{"HTMLBlueNavy", {Qt::SolidPattern, QColor(0, 0, 128)}},
						{"HTMLBlueMidnightBlue",
						 {Qt::SolidPattern, QColor(25, 25, 112)}},
						{"HTMLPurpleLavender",
						 {Qt::SolidPattern, QColor(230, 230, 250)}},
						{"HTMLPurpleThistle",
						 {Qt::SolidPattern, QColor(216, 191, 216)}},
						{"HTMLPurplePlum",
						 {Qt::SolidPattern, QColor(221, 160, 221)}},
						{"HTMLPurpleViolet",
						 {Qt::SolidPattern, QColor(238, 130, 238)}},
						{"HTMLPurpleOrchid",
						 {Qt::SolidPattern, QColor(218, 112, 214)}},
						{"HTMLPurpleFuchsia",
						 {Qt::SolidPattern, QColor(255, 0, 255)}},
						{"HTMLPurpleMagenta",
						 {Qt::SolidPattern, QColor(255, 0, 255)}},
						{"HTMLPurpleMediumOrchid",
						 {Qt::SolidPattern, QColor(186, 85, 211)}},
						{"HTMLPurpleMediumPurple",
						 {Qt::SolidPattern, QColor(147, 112, 219)}},
						{"HTMLPurpleBlueViolet",
						 {Qt::SolidPattern, QColor(138, 43, 226)}},
						{"HTMLPurpleDarkViolet",
						 {Qt::SolidPattern, QColor(148, 0, 211)}},
						{"HTMLPurpleDarkOrchid",
						 {Qt::SolidPattern, QColor(153, 50, 204)}},
						{"HTMLPurpleDarkMagenta",
						 {Qt::SolidPattern, QColor(139, 0, 139)}},
						{"HTMLPurplePurple",
						 {Qt::SolidPattern, QColor(128, 0, 128)}},
						{"HTMLPurpleIndigo",
						 {Qt::SolidPattern, QColor(75, 0, 130)}},
						{"HTMLPurpleDarkSlateBlue",
						 {Qt::SolidPattern, QColor(72, 61, 139)}},
						{"HTMLPurpleSlateBlue",
						 {Qt::SolidPattern, QColor(106, 90, 205)}},
						{"HTMLPurpleMediumSlateBlue",
						 {Qt::SolidPattern, QColor(123, 104, 238)}},
						{"HTMLWhiteWhite",
						 {Qt::SolidPattern, QColor(255, 255, 255)}},
						{"HTMLWhiteSnow",
						 {Qt::SolidPattern, QColor(255, 250, 250)}},
						{"HTMLWhiteHoneydew",
						 {Qt::SolidPattern, QColor(240, 255, 240)}},
						{"HTMLWhiteMintCream",
						 {Qt::SolidPattern, QColor(245, 255, 250)}},
						{"HTMLWhiteAzure",
						 {Qt::SolidPattern, QColor(240, 255, 255)}},
						{"HTMLWhiteAliceBlue",
						 {Qt::SolidPattern, QColor(240, 248, 255)}},
						{"HTMLWhiteGhostWhite",
						 {Qt::SolidPattern, QColor(248, 248, 255)}},
						{"HTMLWhiteWhiteSmoke",
						 {Qt::SolidPattern, QColor(245, 245, 245)}},
						{"HTMLWhiteSeashell",
						 {Qt::SolidPattern, QColor(255, 245, 238)}},
						{"HTMLWhiteBeige",
						 {Qt::SolidPattern, QColor(245, 245, 220)}},
						{"HTMLWhiteOldLace",
						 {Qt::SolidPattern, QColor(253, 245, 230)}},
						{"HTMLWhiteFloralWhite",
						 {Qt::SolidPattern, QColor(255, 250, 240)}},
						{"HTMLWhiteIvory",
						 {Qt::SolidPattern, QColor(255, 255, 240)}},
						{"HTMLWhiteAntiqueWhite",
						 {Qt::SolidPattern, QColor(250, 235, 215)}},
						{"HTMLWhiteLinen",
						 {Qt::SolidPattern, QColor(250, 240, 230)}},
						{"HTMLWhiteLavenderBlush",
						 {Qt::SolidPattern, QColor(255, 240, 245)}},
						{"HTMLWhiteMistyRose",
						 {Qt::SolidPattern, QColor(255, 228, 225)}},
						{"HTMLGrayGainsboro",
						 {Qt::SolidPattern, QColor(220, 220, 220)}},
						{"HTMLGrayLightGray",
						 {Qt::SolidPattern, QColor(211, 211, 211)}},
						{"HTMLGraySilver",
						 {Qt::SolidPattern, QColor(192, 192, 192)}},
						{"HTMLGrayDarkGray",
						 {Qt::SolidPattern, QColor(169, 169, 169)}},
						{"HTMLGrayGray",
						 {Qt::SolidPattern, QColor(128, 128, 128)}},
						{"HTMLGrayDimGray",
						 {Qt::SolidPattern, QColor(105, 105, 105)}},
						{"HTMLGrayLightSlateGray",
						 {Qt::SolidPattern, QColor(119, 136, 153)}},
						{"HTMLGraySlateGray",
						 {Qt::SolidPattern, QColor(112, 128, 144)}},
						{"HTMLGrayDarkSlateGray",
						 {Qt::SolidPattern, QColor(47, 79, 79)}},
						{"HTMLGrayBlack", {Qt::SolidPattern, QColor(0, 0, 0)}},
						{"hor", {Qt::HorPattern, Qt::black}},
						{"ver", {Qt::VerPattern, Qt::black}},
						{"bdiag", {Qt::BDiagPattern, Qt::black}},
						{"fdiag", {Qt::FDiagPattern, Qt::black}}};

				if (style_value == "none") { brush.setStyle(Qt::NoBrush); }
				else
				{
					auto style_ = filling_style_map.find(style_value);
					if (style_ == filling_style_map.end()) { continue; }

					brush.setStyle(style_->first);
	 				brush.setColor(style_->second);
				}
			} else if (style_name == "color") {
				static const QMap<QString, QColor> color_style_map = {
					{"red", Qt::red},
					{"blue", Qt::blue},
					{"green", Qt::green},
					{"gray", Qt::gray},
					{"brun", QColor(97, 44, 0)},
					{"yellow", Qt::yellow},
					{"cyan", Qt::cyan},
					{"magenta", Qt::magenta},
					{"lightgray", Qt::lightGray},
					{"orange", QColor(255, 128, 0)},
					{"purple", QColor(136, 28, 168)},
					{"HTMLPinkPink", QColor(255, 192, 203)},
					{"HTMLPinkLightPink", QColor(255, 182, 193)},
					{"HTMLPinkHotPink", QColor(255, 105, 180)},
					{"HTMLPinkDeepPink", QColor(255, 20, 147)},
					{"HTMLPinkPaleVioletRed", QColor(219, 112, 147)},
					{"HTMLPinkMediumVioletRed", QColor(199, 21, 133)},
					{"HTMLRedLightSalmon", QColor(255, 160, 122)},
					{"HTMLRedSalmon", QColor(250, 128, 114)},
					{"HTMLRedDarkSalmon", QColor(233, 150, 122)},
					{"HTMLRedLightCoral", QColor(240, 128, 128)},
					{"HTMLRedIndianRed", QColor(205, 92, 92)},
					{"HTMLRedCrimson", QColor(220, 20, 60)},
					{"HTMLRedFirebrick", QColor(178, 34, 34)},
					{"HTMLRedDarkRed", QColor(139, 0, 0)},
					{"HTMLRedRed", QColor(255, 0, 0)},
					{"HTMLOrangeOrangeRed", QColor(255, 69, 0)},
					{"HTMLOrangeTomato", QColor(255, 99, 71)},
					{"HTMLOrangeCoral", QColor(255, 127, 80)},
					{"HTMLOrangeDarkOrange", QColor(255, 140, 0)},
					{"HTMLOrangeOrange", QColor(255, 165, 0)},
					{"HTMLYellowYellow", QColor(255, 255, 0)},
					{"HTMLYellowLightYellow", QColor(255, 255, 224)},
					{"HTMLYellowLemonChiffon", QColor(255, 250, 205)},
					{"HTMLYellowLightGoldenrodYellow", QColor(250, 250, 210)},
					{"HTMLYellowPapayaWhip", QColor(255, 239, 213)},
					{"HTMLYellowMoccasin", QColor(255, 228, 181)},
					{"HTMLYellowPeachPuff", QColor(255, 218, 185)},
					{"HTMLYellowPaleGoldenrod", QColor(238, 232, 170)},
					{"HTMLYellowKhaki", QColor(240, 230, 140)},
					{"HTMLYellowDarkKhaki", QColor(189, 183, 107)},
					{"HTMLYellowGold", QColor(255, 215, 0)},
					{"HTMLBrownCornsilk", QColor(255, 248, 220)},
					{"HTMLBrownBlanchedAlmond", QColor(255, 235, 205)},
					{"HTMLBrownBisque", QColor(255, 228, 196)},
					{"HTMLBrownNavajoWhite", QColor(255, 222, 173)},
					{"HTMLBrownWheat", QColor(245, 222, 179)},
					{"HTMLBrownBurlywood", QColor(222, 184, 135)},
					{"HTMLBrownTan", QColor(210, 180, 140)},
					{"HTMLBrownRosyBrown", QColor(188, 143, 143)},
					{"HTMLBrownSandyBrown", QColor(244, 164, 96)},
					{"HTMLBrownGoldenrod", QColor(218, 165, 32)},
					{"HTMLBrownDarkGoldenrod", QColor(184, 134, 11)},
					{"HTMLBrownPeru", QColor(205, 133, 63)},
					{"HTMLBrownChocolate", QColor(210, 105, 30)},
					{"HTMLBrownSaddleBrown", QColor(139, 69, 19)},
					{"HTMLBrownSienna", QColor(160, 82, 45)},
					{"HTMLBrownBrown", QColor(165, 42, 42)},
					{"HTMLBrownMaroon", QColor(128, 0, 0)},
					{"HTMLGreenDarkOliveGreen", QColor(85, 107, 47)},
					{"HTMLGreenOlive", QColor(128, 128, 0)},
					{"HTMLGreenOliveDrab", QColor(107, 142, 35)},
					{"HTMLGreenYellowGreen", QColor(154, 205, 50)},
					{"HTMLGreenLimeGreen", QColor(50, 205, 50)},
					{"HTMLGreenLime", QColor(0, 255, 0)},
					{"HTMLGreenLawnGreen", QColor(124, 252, 0)},
					{"HTMLGreenChartreuse", QColor(127, 255, 0)},
					{"HTMLGreenGreenYellow", QColor(173, 255, 47)},
					{"HTMLGreenSpringGreen", QColor(0, 255, 127)},
					{"HTMLGreenMediumSpringGreen", QColor(0, 250, 154)},
					{"HTMLGreenLightGreen", QColor(144, 238, 144)},
					{"HTMLGreenPaleGreen", QColor(152, 251, 152)},
					{"HTMLGreenDarkSeaGreen", QColor(143, 188, 143)},
					{"HTMLGreenMediumAquamarine", QColor(102, 205, 170)},
					{"HTMLGreenMediumSeaGreen", QColor(60, 179, 113)},
					{"HTMLGreenSeaGreen", QColor(46, 139, 87)},
					{"HTMLGreenForestGreen", QColor(34, 139, 34)},
					{"HTMLGreenGreen", QColor(0, 128, 0)},
					{"HTMLGreenDarkGreen", QColor(0, 100, 0)},
					{"HTMLCyanAqua", QColor(0, 255, 255)},
					{"HTMLCyanCyan", QColor(0, 255, 255)},
					{"HTMLCyanLightCyan", QColor(224, 255, 255)},
					{"HTMLCyanPaleTurquoise", QColor(175, 238, 238)},
					{"HTMLCyanAquamarine", QColor(127, 255, 212)},
					{"HTMLCyanTurquoise", QColor(64, 224, 208)},
					{"HTMLCyanMediumTurquoise", QColor(72, 209, 204)},
					{"HTMLCyanDarkTurquoise", QColor(0, 206, 209)},
					{"HTMLCyanLightSeaGreen", QColor(32, 178, 170)},
					{"HTMLCyanCadetBlue", QColor(95, 158, 160)},
					{"HTMLCyanDarkCyan", QColor(0, 139, 139)},
					{"HTMLCyanTeal", QColor(0, 128, 128)},
					{"HTMLBlueLightSteelBlue", QColor(176, 196, 222)},
					{"HTMLBluePowderBlue", QColor(176, 224, 230)},
					{"HTMLBlueLightBlue", QColor(173, 216, 230)},
					{"HTMLBlueSkyBlue", QColor(135, 206, 235)},
					{"HTMLBlueLightSkyBlue", QColor(135, 206, 250)},
					{"HTMLBlueDeepSkyBlue", QColor(0, 191, 255)},
					{"HTMLBlueDodgerBlue", QColor(30, 144, 255)},
					{"HTMLBlueCornflowerBlue", QColor(100, 149, 237)},
					{"HTMLBlueSteelBlue", QColor(70, 130, 180)},
					{"HTMLBlueRoyalBlue", QColor(65, 105, 225)},
					{"HTMLBlueBlue", QColor(0, 0, 255)},
					{"HTMLBlueMediumBlue", QColor(0, 0, 205)},
					{"HTMLBlueDarkBlue", QColor(0, 0, 139)},
					{"HTMLBlueNavy", QColor(0, 0, 128)},
					{"HTMLBlueMidnightBlue", QColor(25, 25, 112)},
					{"HTMLPurpleLavender", QColor(230, 230, 250)},
					{"HTMLPurpleThistle", QColor(216, 191, 216)},
					{"HTMLPurplePlum", QColor(221, 160, 221)},
					{"HTMLPurpleViolet", QColor(238, 130, 238)},
					{"HTMLPurpleOrchid", QColor(218, 112, 214)},
					{"HTMLPurpleFuchsia", QColor(255, 0, 255)},
					{"HTMLPurpleMagenta", QColor(255, 0, 255)},
					{"HTMLPurpleMediumOrchid", QColor(186, 85, 211)},
					{"HTMLPurpleMediumPurple", QColor(147, 112, 219)},
					{"HTMLPurpleBlueViolet", QColor(138, 43, 226)},
					{"HTMLPurpleDarkViolet", QColor(148, 0, 211)},
					{"HTMLPurpleDarkOrchid", QColor(153, 50, 204)},
					{"HTMLPurpleDarkMagenta", QColor(139, 0, 139)},
					{"HTMLPurplePurple", QColor(128, 0, 128)},
					{"HTMLPurpleIndigo", QColor(75, 0, 130)},
					{"HTMLPurpleDarkSlateBlue", QColor(72, 61, 139)},
					{"HTMLPurpleSlateBlue", QColor(106, 90, 205)},
					{"HTMLPurpleMediumSlateBlue", QColor(123, 104, 238)},
					{"HTMLWhiteWhite", QColor(255, 255, 255)},
					{"HTMLWhiteSnow", QColor(255, 250, 250)},
					{"HTMLWhiteHoneydew", QColor(240, 255, 240)},
					{"HTMLWhiteMintCream", QColor(245, 255, 250)},
					{"HTMLWhiteAzure", QColor(240, 255, 255)},
					{"HTMLWhiteAliceBlue", QColor(240, 248, 255)},
					{"HTMLWhiteGhostWhite", QColor(248, 248, 255)},
					{"HTMLWhiteWhiteSmoke", QColor(245, 245, 245)},
					{"HTMLWhiteSeashell", QColor(255, 245, 238)},
					{"HTMLWhiteBeige", QColor(245, 245, 220)},
					{"HTMLWhiteOldLace", QColor(253, 245, 230)},
					{"HTMLWhiteFloralWhite", QColor(255, 250, 240)},
					{"HTMLWhiteIvory", QColor(255, 255, 240)},
					{"HTMLWhiteAntiqueWhite", QColor(250, 235, 215)},
					{"HTMLWhiteLinen", QColor(250, 240, 230)},
					{"HTMLWhiteLavenderBlush", QColor(255, 240, 245)},
					{"HTMLWhiteMistyRose", QColor(255, 228, 225)},
					{"HTMLGrayGainsboro", QColor(220, 220, 220)},
					{"HTMLGrayLightGray", QColor(211, 211, 211)},
					{"HTMLGraySilver", QColor(192, 192, 192)},
					{"HTMLGrayDarkGray", QColor(169, 169, 169)},
					{"HTMLGrayGray", QColor(128, 128, 128)},
					{"HTMLGrayDimGray", QColor(105, 105, 105)},
					{"HTMLGrayLightSlateGray", QColor(119, 136, 153)},
					{"HTMLGraySlateGray", QColor(112, 128, 144)},
					{"HTMLGrayDarkSlateGray", QColor(47, 79, 79)},
					{"HTMLGrayBlack", QColor(0, 0, 0)}
				};

				if (style_value == "none") { pen.setBrush(Qt::transparent); }
				else if (style_value == "black")
				{
					pen.setBrush(QColor(0, 0, 0, pen.color().alpha()));
				}
				else if (style_value == "white")
				{
					pen.setBrush(QColor(255, 255, 255, pen.color().alpha()));
				}
				else
				{
					auto style_ = color_style_map.find(style_value);
					if (style_ == color_style_map.end()) { continue; }

					pen.setColor(*style_);
				}
			}
		}
	}

	painter.setPen(pen);
	painter.setBrush(brush);
}
