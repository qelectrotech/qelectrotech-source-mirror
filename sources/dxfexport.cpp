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
#include "shownkinds.h"

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
#include "qetinformation.h"
#include "textlines.h"

#include <QFontMetricsF>
#include <QGraphicsSimpleTextItem>
#include <QSet>
#include <cmath>
#include <utility>

namespace {
	/**
		Draw the drawing of @a elmt's definition: its static texts, lines,
		rectangles, circles, polygons and arcs, and its terminals when
		@a draw_terminals, as if it were placed at @a elem_pos_x,
		@a elem_pos_y and turned @a rotation_angle degrees, with @a elmt's
		mirrors and its texts kept readable (Element::symbolTextsTransform())
		when @a as_placed. Used for every symbol drawn in full, and once per
		block for a block's content.
	*/
	void drawSymbol(const QString &file_path, Element *elmt,
					qreal elem_pos_x, qreal elem_pos_y,
					double rotation_angle, bool draw_terminals,
					bool as_placed = false)
	{
		using namespace DxfExport;
			//The mirrors of an element are about its own axes, before it is
			//rotated, see Element::setMirror()
		const bool mirror_h = as_placed && elmt -> hasHorizontalMirror();
		const bool mirror_v = as_placed && elmt -> hasVerticalMirror();
		const QTransform texts_transform = as_placed
				? elmt -> symbolTextsTransform()
				: QTransform();
		const qreal text_turn = as_placed && elmt -> hasUprightSymbolTexts()
				? 0
				: rotation_angle;
		const qreal mirror_x = mirror_h ? -1 : 1;
		const qreal mirror_y = mirror_v ? -1 : 1;
		ElementPictureFactory::primitives primitives = ElementPictureFactory::instance()->getPrimitives(elmt->location());

		Createdxf::layer = Layer::SymbolTexts;
		for(QGraphicsSimpleTextItem *text : primitives.m_texts)
		{
			qreal fontSize = text->font().pointSizeF();
			if (fontSize < 0)
				fontSize = text->font().pixelSize();

			QPointF text_pos = text->pos();
				//A text the element mirrors, or turns in a project that keeps
				//symbol texts horizontal, keeps reading as in the symbol:
				//only its box moves, as ElementPictureFactory draws it on the
				//folio. It is then drawn as in a symbol neither mirrored nor
				//turned, moved to where the element puts the box's centre.
			if (!texts_transform.isIdentity())
			{
				QTransform box_transform;
				box_transform.translate(text->pos().x(), text->pos().y());
				box_transform.rotate(text->rotation());
				const QFontMetricsF metrics(text->font());
				const QRectF box(0, -metrics.ascent(),
								 text->boundingRect().width(),
								 text->boundingRect().height());
				const QPointF centre = box_transform.mapRect(box).center();
				text_pos += texts_transform.map(centre) - centre;
			}

			qreal x = elem_pos_x + text_pos.x();
			qreal y = elem_pos_y + text_pos.y();

			qreal angle = text -> rotation() + text_turn;
			qreal angler = angle * M_PI/180;
			int xdir = -sin(angler);
			int ydir = -cos(angler);

			QPointF transformed_point = DxfExport::rotation_transformed(x, y, elem_pos_x, elem_pos_y, -text_turn);
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
			QTransform t = QTransform().translate(elem_pos_x,elem_pos_y).rotate(rotation_angle).scale(mirror_x, mirror_y);
			QLineF l = t.map(line);
			Createdxf::drawLine(file_path, l, 0);
		}

		for (QRectF rect : primitives.m_rectangles)
		{
			QTransform t = QTransform().translate(elem_pos_x,elem_pos_y).rotate(rotation_angle).scale(mirror_x, mirror_y);
			QRectF r = t.mapRect(rect);
			Createdxf::drawRectangle(file_path,r,0);
		}

		for (QRectF circle_rect : primitives.m_circles)
		{
			QTransform t = QTransform().translate(elem_pos_x,elem_pos_y).rotate(rotation_angle).scale(mirror_x, mirror_y);
			QPointF c = t.map(QPointF(circle_rect.center().x(),circle_rect.center().y()));
			Createdxf::drawCircle(file_path,c,circle_rect.width()/2,0);
		}

		for (QVector<QPointF> polygon : primitives.m_polygons)
		{
			if (polygon.size() == 0)
				continue;
			QTransform t = QTransform().translate(elem_pos_x,elem_pos_y).rotate(rotation_angle).scale(mirror_x, mirror_y);
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
				//In a mirror the arc starts where it ended: an angle a
				//becomes 180 - a in a horizontal one, -a in a vertical one
			if (mirror_h)
			{
				x = elem_pos_x - arc.at(0) - w;
				startAngle = 180 - startAngle - spanAngle;
			}
			if (mirror_v)
			{
				y = elem_pos_y - arc.at(1) - h;
				startAngle = -startAngle - spanAngle;
			}
			QRectF r(x,y,w,h);
			QPointF hotspot(elem_pos_x,elem_pos_y);
			Createdxf::drawArcEllipse(file_path, r, startAngle, spanAngle, hotspot, rotation_angle, 0);
		}
		if (draw_terminals) {
			Createdxf::layer = Layer::Terminals;
			// Draw terminals
			QList<Terminal *> list_terminals = elmt->terminals();
			QColor col("red");
			QTransform t = QTransform().translate(elem_pos_x,elem_pos_y).rotate(rotation_angle).scale(mirror_x, mirror_y);
			foreach(Terminal *tp, list_terminals) {
				QPointF c = t.map(QPointF(tp->dock_elmt_.x(),tp->dock_elmt_.y()));
				Createdxf::drawCircle(file_path,c,3.0,Createdxf::dxfColor(col));
			}
		}
	}

		/// One line of a text as the export writes it: where it starts, in
		/// scene coordinates, its size, and its angle (degrees, clockwise)
	struct TextLine
	{
		QString text;
		QPointF pos;
		qreal font_size;
		qreal angle;
	};

		/// The lines of @a dti that are written, one under the other as drawn
	QList<TextLine> textLines(DiagramTextItem *dti)
	{
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
			//As drawn: a text with a width is wrapped
		QStringList lines = TextLines::layoutLines(dti -> document());
		qreal offset = fontSize * 1.6;
		QList<TextLine> result;
		foreach (QString line, lines) {
			if (line.size() > 0 && line != "_" )
				result << TextLine{line, QPointF(x, y), fontSize, angle};
			x += offset * xdir;
			y -= offset * ydir;
		}
		return result;
	}

		/// @a tag, or @a tag with _2, _3... if @a used has it, at most 31
		/// characters
	QString uniqueTag(const QString &tag, const QSet<QString> &used)
	{
		QString unique = tag;
		for (int i = 2 ; used.contains(unique) ; ++i) {
			const QString suffix = QStringLiteral("_%1").arg(i);
			unique = tag.left(31 - suffix.size()) + suffix;
		}
		return unique;
	}

		/// Nothing would be drawn for @a elmt (its definition is missing)
	bool drawsNothing(Element *elmt, bool draw_terminals)
	{
		const ElementPictureFactory::primitives p =
				ElementPictureFactory::instance()->getPrimitives(elmt->location());
		return p.m_lines.isEmpty() && p.m_rectangles.isEmpty()
				&& p.m_circles.isEmpty() && p.m_polygons.isEmpty()
				&& p.m_arcs.isEmpty() && p.m_texts.isEmpty()
				&& (!draw_terminals || elmt->terminals().isEmpty());
	}
}

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

		//One scale for both axes, the largest that fits the sheet: two
		//scales stretched the drawing and turned every circle into an oval
		//(issue #1339).
	const double scale = qMin(Createdxf::sheetWidth  / double(width),
							  Createdxf::sheetHeight / double(height));
	Createdxf::xScale = scale;
	Createdxf::yScale = scale;

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
			//Left out like on screen and in print (View > Show, #301)
		if (ShownKinds::isHidden(qgi)) {
			continue;
		}
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
			QGraphicsTextItem *xref = deti->slaveXrefItem();
			if (xref && !ShownKinds::isHidden(xref)) {
				list_xref_texts << xref;
			}
		} else if (QetGraphicsTableItem *gti = qgraphicsitem_cast<QetGraphicsTableItem *>(qgi)) {
			list_tables << gti;
		} else if (CrossRefItem *xref = qgraphicsitem_cast<CrossRefItem *>(qgi)) {
			list_master_xrefs << xref;
		}
	}

		//Symbols as blocks: one block per symbol definition the folio
		//uses, named after its file and numbered in the order first met,
		//so a repeat export is identical. A block has to be written before
		//any entity, so they are all collected first.
	QHash<QString, QString> block_names; // location -> block name
	QList<Element *> block_models;       // one placed symbol per block
	if (properties.dxf_blocks) {
		QSet<QString> used;
		for (Element *elmt : std::as_const(list_elements)) {
			const QString key = elmt -> location().toString();
			if (!elmt -> symbolTextsTransform().isIdentity()
				|| block_names.contains(key)
				|| drawsNothing(elmt, properties.draw_terminals))
				continue;
			QString file_name = elmt -> location().fileName();
			if (file_name.endsWith(QLatin1String(".elmt")))
				file_name.chop(5);
			const QString base = Createdxf::blockName(
						QStringLiteral("QET_") + file_name);
			QString name = base;
			for (int i = 2 ; used.contains(name) ; ++i) {
				const QString suffix = QStringLiteral("_%1").arg(i);
				name = base.left(31 - suffix.size()) + suffix;
			}
			used << name;
			block_names.insert(key, name);
			block_models << elmt;
		}
	}
		//A mirrored symbol, or a turned one whose texts stay horizontal,
		//is drawn in full: an INSERT would mirror or turn its texts too,
		//which QElectroTech keeps readable
	const auto blockOf = [&block_names](Element *elmt) {
		return !elmt -> symbolTextsTransform().isIdentity()
				? QString()
				: block_names.value(elmt -> location().toString());
	};

		//With dxf_attributes, a symbol's own texts (its label, its
		//function...) become attributes of its INSERT, written where the
		//text would be, so a CAD program moves them with the symbol and
		//lists them. Off by default: LibreCAD does not show attributes,
		//so the labels would vanish there. Tagged
		//with the information they show (LABEL, FUNCTION...), or TEXT1,
		//TEXT2... for a typed text; a second line gets _2. The block
		//defines each tag once, where the first symbol has it.
	QHash<Element *, QList<Createdxf::Attribute>> attributes;
	QHash<QString, QList<Createdxf::Attribute>> attribute_definitions; // block -> ATTDEFs
	QSet<DiagramTextItem *> attribute_texts;
	{
		QHash<Element *, QSet<QString>> used_tags;
		QHash<Element *, int> typed_texts;
		QHash<QString, QSet<QString>> defined_tags;
		for (DiagramTextItem *dti : std::as_const(list_texts)) {
			if (!properties.dxf_attributes)
				break;
			auto *deti = qgraphicsitem_cast<DynamicElementTextItem *>(dti);
			Element *elmt = deti ? deti -> parentElement() : nullptr;
			const QString block = elmt ? blockOf(elmt) : QString();
			if (block.isEmpty())
				continue;

			const QString base =
					deti -> textFrom() == DynamicElementTextItem::ElementInfo
					&& !deti -> infoName().isEmpty()
					? Createdxf::blockName(deti -> infoName())
					: QStringLiteral("TEXT%1").arg(++typed_texts[elmt]);
				//From the folio into the block: undo the INSERT
			const QPointF insert(elmt -> pos().x() * Createdxf::xScale,
								 Createdxf::sheetHeight - elmt -> pos().y() * Createdxf::yScale);
			const double turn = std::fmod(360 - elmt -> orientation() * 90, 360);
			const QTransform to_block = QTransform()
					.translate(0, Createdxf::sheetHeight)
					.rotate(-turn)
					.translate(-insert.x(), -insert.y());

			const QList<TextLine> lines = textLines(dti);
			for (int i = 0 ; i < lines.size() ; ++i) {
				const TextLine &line = lines.at(i);
				const QString tag = uniqueTag(
							i == 0 ? base : base + QStringLiteral("_%1").arg(i + 1),
							used_tags.value(elmt));
				used_tags[elmt] << tag;

				Createdxf::Attribute attribute;
				attribute.tag = tag;
				attribute.text = line.text;
				attribute.x = line.pos.x() * Createdxf::xScale;
				attribute.y = Createdxf::sheetHeight - line.pos.y() * Createdxf::yScale;
				attribute.height = line.font_size * Createdxf::yScale;
				attribute.rotation = 360 - line.angle;
				attribute.xScaleW = 0.72;
				attribute.colour = Createdxf::dxfColor(dti -> color());
				attribute.layer = Layer::SymbolTexts;
				attributes[elmt] << attribute;

				if (!defined_tags[block].contains(tag)) {
					defined_tags[block] << tag;
					Createdxf::Attribute definition = attribute;
					const QPointF in_block = to_block.map(QPointF(attribute.x, attribute.y));
					definition.x = in_block.x();
					definition.y = in_block.y();
					definition.rotation = std::fmod(attribute.rotation - turn + 720, 360);
					definition.text.clear();
					attribute_definitions[block] << definition;
				}
			}
			attribute_texts << dti;
		}

			//The rest of what is known about a symbol (manufacturer,
			//reference, supplier, quantity...) as hidden attributes, so a
			//CAD program can list the parts from the drawing. A field
			//already written as a visible attribute (dxf_attributes) is not
			//repeated; the label formula is how the label is made, not
			//part data.
		for (Element *elmt : std::as_const(list_elements)) {
			const QString block = blockOf(elmt);
			if (block.isEmpty())
				continue;
			const DiagramContext information = elmt -> elementInformations();
			const QPointF insert(elmt -> pos().x() * Createdxf::xScale,
								 Createdxf::sheetHeight - elmt -> pos().y() * Createdxf::yScale);
			for (const QString &key : QETInformation::elementInfoKeys()) {
				if (key == QETInformation::ELMT_FORMULA)
					continue;
				const QString value = information.value(key).toString();
				const QString tag = Createdxf::blockName(key);
				if (value.isEmpty() || used_tags.value(elmt).contains(tag))
					continue;
				used_tags[elmt] << tag;

				Createdxf::Attribute attribute;
				attribute.tag = tag;
				attribute.text = value;
				attribute.x = insert.x();
				attribute.y = insert.y();
				attribute.height = 9 * Createdxf::yScale;
				attribute.invisible = true;
				attribute.layer = Layer::SymbolTexts;
				attributes[elmt] << attribute;

				if (!defined_tags[block].contains(tag)) {
					defined_tags[block] << tag;
					Createdxf::Attribute definition = attribute;
					definition.x = 0;
					definition.y = Createdxf::sheetHeight;
					definition.text.clear();
					attribute_definitions[block] << definition;
				}
			}
		}
	}

		//A block is drawn as if its symbol sat unturned at the folio's
		//origin, which is DXF (0, sheetHeight); that is its base point.
	Createdxf::dxfBegin(file_path, Layer::all(), [&]() {
		for (Element *model : std::as_const(block_models)) {
			const QString block = block_names.value(model -> location().toString());
			const QList<Createdxf::Attribute> definitions = attribute_definitions.value(block);
			Createdxf::dxfBlockBegin(file_path, block, 0, Createdxf::sheetHeight,
									 !definitions.isEmpty());
			drawSymbol(file_path, model, 0, 0, 0, properties.draw_terminals);
			for (const Createdxf::Attribute &definition : definitions)
				Createdxf::drawAttdef(file_path, definition);
			Createdxf::dxfBlockEnd(file_path);
		}
	});

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

	// Draw shapes
	Createdxf::layer = Layer::Shapes;
	foreach (QetShapeItem *qsi, list_shapes) qsi->toDXF(file_path, qsi->pen());

	// Draw tables
	Createdxf::layer = Layer::Tables;
	foreach (QetGraphicsTableItem *gti, list_tables) {
		gti->toDXF(file_path);
	}

	//Draw elements: each one in full, or as an INSERT of its block
	foreach(Element *elmt, list_elements)
	{
		const double rotation_angle = elmt -> orientation() * 90;
		const qreal elem_pos_x = elmt -> pos().x();
		const qreal elem_pos_y = elmt -> pos().y();

		const QString block = blockOf(elmt);
		if (block.isEmpty()) {
			drawSymbol(file_path, elmt, elem_pos_x, elem_pos_y,
					   rotation_angle, properties.draw_terminals, true);
		} else {
			Createdxf::layer = Layer::Symbols;
				//QElectroTech turns a symbol clockwise, DXF counter-clockwise
			Createdxf::drawInsert(file_path, block,
								  elem_pos_x * Createdxf::xScale,
								  Createdxf::sheetHeight - elem_pos_y * Createdxf::yScale,
								  std::fmod(360 - rotation_angle, 360),
								  attributes.value(elmt));
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

		if (textItem && !ShownKinds::isHidden(textItem)) {
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
			//Already written as an attribute of its symbol's INSERT
		if (attribute_texts.contains(dti))
			continue;
			//A free text, or a symbol's own text (its label and the like)
		Createdxf::layer = qgraphicsitem_cast<IndependentTextItem *>(dti)
				? Layer::Texts : Layer::SymbolTexts;
		for (const TextLine &line : textLines(dti))
			Createdxf::drawText(file_path, line.text, line.pos, line.font_size, 360-line.angle, Createdxf::dxfColor(dti->color()), 0.72 );
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
