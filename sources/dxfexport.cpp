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
#include "dxfexport.h"

#include "conductorsegment.h"
#include "createdxf.h"
#include "diagram.h"
#include "dxfpaintdevice.h"
#include "exportproperties.h"
#include "factory/elementpicturefactory.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/crossrefitem.h"
#include "qetgraphicsitem/diagramimageitem.h"
#include "qetgraphicsitem/diagramtextitem.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/qetshapeitem.h"
#include "qetgraphicsitem/terminal.h"

#include <QGraphicsSimpleTextItem>
#include <QSet>
#include <cmath>
#include <utility>

/**
	@brief DxfExport::folioSize
	@return the size of @a diagram as exported with @a properties: its border
	and title block, or just its items, following properties.exported_area.
	The export dialog offers this size by default, and --export-dxf uses it.
*/
QSize DxfExport::folioSize(Diagram *diagram, const ExportProperties &properties)
{
	const bool state_useBorder = diagram -> useBorder();
	diagram -> setUseBorder(properties.exported_area == QET::BorderArea);
	const QSize size = diagram -> imageSize();
	diagram -> setUseBorder(state_useBorder);
	return size;
}

/**
	@brief DxfExport::write
	Export @a diagram as a DXF file. Moved here unchanged from
	ExportDialog::generateDxf(), except that the options come from
	@a properties instead of the dialog, so the command line can use it.
	The diagram's own display options are applied for the export and put
	back afterwards, as the dialog always did.
	@param diagram : folio to export
	@param width : width of the export, as offered by folioSize()
	@param height : height of the export
	@param path : file to write
	@param properties : export options
*/
void DxfExport::write(Diagram *diagram, int width, int height,
					  const QString &path, const ExportProperties &properties)
{
		//Non-const: BorderTitleBlock::drawDxf() takes a QString &.
	QString file_path = path;
	const ExportProperties previous = diagram -> applyProperties(properties);

	width  -= 2*Diagram::margin;
	height -= 2*Diagram::margin;

	Createdxf::xScale = Createdxf::sheetWidth  / double(width);
	Createdxf::yScale = Createdxf::sheetHeight / double(height);

	Createdxf::dxfBegin(file_path, Layer::all());

		//Each kind of content on its own layer (discussion #1071). The
		//border's title block switches to Layer::TitleBlock itself, see
		//BorderTitleBlock::drawDxf().
	Createdxf::layer = Layer::Border;
	//Add project elements (lines, rectangles, circles, texts) to dxf file
	if (properties.draw_border) {
		QRectF rect(Diagram::margin,Diagram::margin,width,height);
		Createdxf::drawRectangle(file_path,rect,0);
	}
	diagram -> border_and_titleblock.drawDxf(file_path, 0);

	// Build the lists of elements.
	QList<Element *> list_elements;
	QList<Conductor *> list_conductors;
	QList<DiagramTextItem *> list_texts;
	QList<DiagramImageItem *> list_images;
		//Slave cross-reference labels. They hang off a DynamicElementTextItem
		//as plain QGraphicsTextItem children, so neither cast below picks them
		//up and they were missing from the DXF entirely.
	QList<QGraphicsTextItem *> list_xref_texts;
		//Master-side cross-reference item (the table/cross drawn next to a
		//report/master element). It paints itself with hand-written
		//QPainter code across three modes (drawAsCross/drawAsContacts/
		//drawAsPlcTable), so instead of hand-porting each one it's replayed
		//through DxfPaintEngine, which reuses paint() unmodified.
	QList<CrossRefItem *> list_master_xrefs;
	QList<QLineF *> list_lines;
	QList<QRectF *> list_rectangles;
	//QList<QRectF *> list_ellipses;
	QList <QetShapeItem *> list_shapes;
	QList <QetGraphicsTableItem *> list_tables;
//	QList <Terminal *> list_terminals;

	// Determine les elements a "XMLiser"
		//In stacking order (z, then insertion order, which a load makes the
		//file's order), not items() order: that follows memory addresses, so
		//the same folio came out in a different order on every run (FINDINGS
		//F050), as saving did before bugtracker #343. A rect query gives
		//stacking order even with NoIndex; anything it misses keeps its
		//items() position after the rest.
	QList<QGraphicsItem *> stacked_items = diagram -> items(
				QRectF(-1e9, -1e9, 2e9, 2e9), Qt::IntersectsItemBoundingRect,
				Qt::AscendingOrder);
	{
		const QSet<QGraphicsItem *> ranked(stacked_items.cbegin(), stacked_items.cend());
		for (QGraphicsItem *qgi : diagram -> items()) {
			if (!ranked.contains(qgi)) {
				stacked_items << qgi;
			}
		}
	}
	for (QGraphicsItem *qgi : std::as_const(stacked_items)) {
		if (Element *elmt = qgraphicsitem_cast<Element *>(qgi)) {
			list_elements << elmt;
		} else if (Conductor *f = qgraphicsitem_cast<Conductor *>(qgi)) {
			list_conductors << f;
		} else if (IndependentTextItem *iti = qgraphicsitem_cast<IndependentTextItem *>(qgi)) {
			list_texts << iti;
		} else if (DiagramImageItem *dii = qgraphicsitem_cast<DiagramImageItem *>(qgi)) {
			list_images << dii;
		} else if (QetShapeItem *dii = qgraphicsitem_cast<QetShapeItem *>(qgi)) {
			list_shapes << dii;
		} else if (DynamicElementTextItem *deti = qgraphicsitem_cast<DynamicElementTextItem *>(qgi)) {
			list_texts << deti;
			if (QGraphicsTextItem *xref = deti->slaveXrefItem()) {
				list_xref_texts << xref;
			}
		} else if (QetGraphicsTableItem *gti = qgraphicsitem_cast<QetGraphicsTableItem *>(qgi)) {
			list_tables << gti;
		} else if (CrossRefItem *xref = qgraphicsitem_cast<CrossRefItem *>(qgi)) {
			list_master_xrefs << xref;
		}
	}

	// Draw shapes
	Createdxf::layer = Layer::Shapes;
	foreach (QetShapeItem *qsi, list_shapes) qsi->toDXF(file_path, qsi->pen());

	// Draw tables
	Createdxf::layer = Layer::Tables;
	foreach (QetGraphicsTableItem *gti, list_tables) {
		gti->toDXF(file_path);
	}

	//Draw elements
	foreach(Element *elmt, list_elements)
	{
		double rotation_angle = elmt -> orientation() * 90;

		qreal elem_pos_x = elmt -> pos().x();
		qreal elem_pos_y = elmt -> pos().y();// - (diagram -> margin / 2);

		ElementPictureFactory::primitives primitives = ElementPictureFactory::instance()->getPrimitives(elmt->location());

		Createdxf::layer = Layer::SymbolTexts;
		for(QGraphicsSimpleTextItem *text : primitives.m_texts)
		{
			qreal fontSize = text->font().pointSizeF();
			if (fontSize < 0)
				fontSize = text->font().pixelSize();

			qreal x = elem_pos_x + text->pos().x();
			qreal y = elem_pos_y + text->pos().y();

			qreal angle = text -> rotation() + rotation_angle;
			qreal angler = angle * M_PI/180;
			int xdir = -sin(angler);
			int ydir = -cos(angler);

			QPointF transformed_point = rotation_transformed(x, y, elem_pos_x, elem_pos_y, -rotation_angle);
			x = transformed_point.x() - ydir * fontSize * 0.5;
			y = transformed_point.y() - xdir * fontSize * 0.5;
			QStringList lines = text->text().split('\n');
			qreal offset = fontSize * 1.6;
			for (QString line : lines)
			{
				if (line.size() > 0 && line != "_" ) {
					Createdxf::drawText(file_path, line, QPointF(x, y), fontSize, 360 - angle, 0, 0.72);
				}
				x += offset * xdir;
				y -= offset * ydir;
			}
		}

		Createdxf::layer = Layer::Symbols;
		for (QLineF line : primitives.m_lines)
		{
			QTransform t = QTransform().translate(elem_pos_x,elem_pos_y).rotate(rotation_angle);
			QLineF l = t.map(line);
			Createdxf::drawLine(file_path, l, 0);
		}

		for (QRectF rect : primitives.m_rectangles)
		{
			QTransform t = QTransform().translate(elem_pos_x,elem_pos_y).rotate(rotation_angle);
			QRectF r = t.mapRect(rect);
			Createdxf::drawRectangle(file_path,r,0);
		}

		for (QRectF circle_rect : primitives.m_circles)
		{
			QTransform t = QTransform().translate(elem_pos_x,elem_pos_y).rotate(rotation_angle);
			QPointF c = t.map(QPointF(circle_rect.center().x(),circle_rect.center().y()));
			Createdxf::drawCircle(file_path,c,circle_rect.width()/2,0);
		}

		for (QVector<QPointF> polygon : primitives.m_polygons)
		{
			if (polygon.size() == 0)
				continue;
			QTransform t = QTransform().translate(elem_pos_x,elem_pos_y).rotate(rotation_angle);
			QPolygonF poly = t.map(polygon);
			if(poly.isClosed())
				Createdxf::drawPolygon(file_path,poly,0);
			else
				Createdxf::drawPolyline(file_path,poly,0);
		}

		// Draw arcs and ellipses
		for (QVector<qreal> arc : primitives.m_arcs)
		{
			if (arc.size() == 0)
				continue;
			qreal x = (elem_pos_x + arc.at(0));
			qreal y = (elem_pos_y + arc.at(1));
			qreal w = arc.at(2);
			qreal h = arc.at(3);
			qreal startAngle = arc.at(4);
			qreal spanAngle = arc .at(5);
			QRectF r(x,y,w,h);
			QPointF hotspot(elem_pos_x,elem_pos_y);
			Createdxf::drawArcEllipse(file_path, r, startAngle, spanAngle, hotspot, rotation_angle, 0);
		}
		if (properties.draw_terminals) {
			Createdxf::layer = Layer::Terminals;
			// Draw terminals
			QList<Terminal *> list_terminals = elmt->terminals();
			QColor col("red");
			QTransform t = QTransform().translate(elem_pos_x,elem_pos_y).rotate(rotation_angle);
			foreach(Terminal *tp, list_terminals) {
				QPointF c = t.map(QPointF(tp->dock_elmt_.x(),tp->dock_elmt_.y()));
				Createdxf::drawCircle(file_path,c,3.0,Createdxf::dxfColor(col));
			}
		}
	}

	//Draw conductors
	foreach(Conductor *cond, list_conductors) {
		QPolygonF poly;
		bool firstseg = true;
		foreach(ConductorSegment *segment, cond -> segmentsList()) {
			//Createdxf::drawLine(file_path,QLineF(cond->pos()+segment->firstPoint(),cond->pos()+segment->secondPoint()),0);
			if(firstseg){
				poly << cond->pos()+segment->firstPoint();
				firstseg = false;
			}
			poly << cond->pos()+segment->secondPoint();
		}
		Createdxf::layer = Layer::Wires;
			//The wire's own colour, as on the folio (a two-colour wire gets
			//its main one). Black comes out as BYLAYER, see Createdxf.
		const ConductorProperties wire_properties = cond -> properties();
		Createdxf::drawPolyline(file_path, poly, Createdxf::dxfColor(wire_properties.color));
		//Draw conductor text item
		Createdxf::layer = Layer::WireNumbers;
		ConductorTextItem *textItem = cond -> textItem();

		if (textItem) {
			qreal fontSize = textItem -> font().pointSizeF();
			if (fontSize < 0)
				fontSize = textItem -> font().pixelSize();
			qreal angle = textItem -> rotation();
			qreal angler = angle * M_PI/180;
			int xdir = -sin(angler);
			int ydir = -cos(angler);

			qreal x = (cond->pos().x() + textItem -> pos().x())
					+ xdir * fontSize * 1.8
					- ydir * fontSize;
			qreal y = (cond->pos().y() + textItem -> pos().y())
					- ydir * fontSize * 1.8
					- xdir * fontSize * 0.9;
			QStringList lines = textItem->toPlainText().split('\n');
			qreal offset = fontSize * 1.6;
			foreach (QString line, lines) {
				if (line.size() > 0 && line != "_" )
					Createdxf::drawText(file_path, line, QPointF(x, y), fontSize, 360-angle,
										Createdxf::dxfColor(wire_properties.text_color), 0.72 );
				x += offset * xdir;
				y -= offset * ydir;
			}
		}

		// Draw the junctions
		Createdxf::layer = Layer::Junctions;
		QList<QPointF> junctions_list = cond->junctions();
		if (!junctions_list.isEmpty()) {
			foreach(QPointF point, junctions_list) {
				Createdxf::drawEllipse(file_path,QRectF(cond->pos().x() + point.x() - 1.5, cond->pos().y() + point.y() - 1.5, 3.0, 3.0),0);
			}
		}
	}

	//Draw text items
	foreach(DiagramTextItem *dti, list_texts) {
			//A free text, or a symbol's own text (its label and the like)
		Createdxf::layer = qgraphicsitem_cast<IndependentTextItem *>(dti)
				? Layer::Texts : Layer::SymbolTexts;
		qreal fontSize = dti -> font().pointSizeF();
		if (fontSize < 0)
			fontSize = dti -> font().pixelSize();

		qreal angle = dti -> rotation();

		QGraphicsItem *parent = dti->parentItem();
		while (parent) {
			angle += parent->rotation();
			parent = parent->parentItem();
		}

		qreal angler = angle * M_PI/180;
		int xdir = -sin(angler);
		int ydir = -cos(angler);
		qreal x = (dti->scenePos().x())
				+ xdir * fontSize * 1.8
				- ydir * fontSize;
		qreal y = dti->scenePos().y()
				- ydir * fontSize * 1.8
				- xdir * fontSize * 0.9;
		QStringList lines = dti -> toPlainText().split('\n');
		qreal offset = fontSize * 1.6;
		foreach (QString line, lines) {
			if (line.size() > 0 && line != "_" )
				Createdxf::drawText(file_path, line, QPointF(x, y), fontSize, 360-angle, Createdxf::dxfColor(dti->color()), 0.72 );
			x += offset * xdir;
			y -= offset * ydir;
		}
	}

	//Draw the slave cross-reference labels
	Createdxf::layer = Layer::Xrefs;
	for (QGraphicsTextItem *xref : std::as_const(list_xref_texts))
	{
		qreal fontSize = xref->font().pointSizeF();
		if (fontSize < 0)
			fontSize = xref->font().pixelSize();

		qreal angle = xref->rotation();
		QGraphicsItem *parent = xref->parentItem();
		while (parent) {
			angle += parent->rotation();
			parent = parent->parentItem();
		}

		qreal angler = angle * M_PI/180;
		int xdir = -sin(angler);
		int ydir = -cos(angler);
		qreal x = xref->scenePos().x()
				+ xdir * fontSize * 1.8
				- ydir * fontSize;
		qreal y = xref->scenePos().y()
				- ydir * fontSize * 1.8
				- xdir * fontSize * 0.9;

		const QStringList lines = xref->toPlainText().split('\n');
		const qreal offset = fontSize * 1.6;
		for (const QString &line : lines) {
			if (line.size() > 0 && line != QLatin1String("_")) {
				Createdxf::drawText(file_path, line, QPointF(x, y), fontSize,
									360-angle, Createdxf::dxfColor(xref->defaultTextColor()), 0.72);
			}
			x += offset * xdir;
			y -= offset * ydir;
		}
	}

	//Draw the master-side cross-reference items (table/cross), replaying
	//their existing paint() unmodified through DxfPaintEngine instead of
	//hand-porting drawAsCross()/drawAsContacts()/drawAsPlcTable().
	Createdxf::layer = Layer::Xrefs;
	for (CrossRefItem *xref : std::as_const(list_master_xrefs))
	{
		DxfPaintDevice dxf_device(file_path);
		QPainter painter(&dxf_device);
		painter.setWorldTransform(xref->sceneTransform());
		xref->paintForExport(&painter);
		painter.end();
	}

	//Draw images -- collected above (list_images) but never actually
	//drawn until now, an existing gap this reuses the same paint()
	//-replay approach to fix: DiagramImageItem::paint() has no
	//viewport-dependent logic (unlike CrossRefItem, which needs its own
	//paintForExport() for that reason), so it's called directly with a
	//default QStyleOptionGraphicsItem rather than needing an export-
	//specific variant of its own. DxfPaintEngine::drawPixmap() is what
	//actually turns the drawPixmap() call inside paint() into a
	//placeholder outline, since this DXF dialect has no raster image
	//entity to draw instead.
	Createdxf::layer = Layer::Images;
	for (DiagramImageItem *image : std::as_const(list_images))
	{
		DxfPaintDevice dxf_device(file_path);
		QPainter painter(&dxf_device);
		painter.setWorldTransform(image->sceneTransform());
		image->paintForExport(&painter);
		painter.end();
	}

	Createdxf::dxfEnd(file_path);

	diagram -> applyProperties(previous);
}

QPointF DxfExport::rotation_transformed(qreal px,
					   qreal py,
					   qreal origin_x,
					   qreal origin_y,
					   qreal angle) {

	angle *= -3.14159265 / 180;

	float s = sin(angle);
	float c = cos(angle);

	// Vector to rotate:
	qreal Vx = px - origin_x;
	qreal Vy = py - origin_y;

	// rotate vector
	float xnew = Vx * c - Vy * s;
	float ynew = Vx * s + Vy * c;

	return QPointF(xnew + origin_x, ynew + origin_y);
}
