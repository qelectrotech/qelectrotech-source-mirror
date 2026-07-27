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
#include "partplctable.h"

#include "../../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../../QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "../../QetGraphicsItemModeler/qetgraphicshandlerutility.h"
#include "../../properties/elementdata.h"
#include "../elementscene.h"
#include "../editorcommands.h"
#include "../ui/qetelementeditor.h"

#include <QPen>
#include <algorithm>

/**
	@brief PartPlcTable::PartPlcTable
	Constructor
	@param editor the QETElementEditor of this item
	@param parent parent item
*/
PartPlcTable::PartPlcTable(QETElementEditor *editor, QGraphicsItem *parent) :
	CustomElementGraphicPart(editor, parent)
{
}

/**
	@brief PartPlcTable::~PartPlcTable
*/
PartPlcTable::~PartPlcTable()
{
	removeHandler();
}

/**
	@brief PartPlcTable::calculateTableSize
	Calculate the table size from PLC data in the element scene.
	@return the calculated size in item coordinates
*/
QSizeF PartPlcTable::calculateTableSize() const
{
	if (!elementEditor() || !elementScene())
		return QSizeF(100, 50);

	ElementData ed = elementScene()->elementData();
	if (ed.m_master_type != ElementData::PLC)
		return QSizeF(100, 50);

	const auto &plc_data = ed.plcMasterData();
	if (plc_data.ios.isEmpty())
		return QSizeF(100, 50);

	const int COL_COUNT = 5;

	// Build list of visible columns
	QList<int> visible_cols;
	if (!plc_data.columnOrder.isEmpty()) {
		for (int logical : plc_data.columnOrder) {
			if (logical >= 0 && logical < COL_COUNT
				&& plc_data.colVisible.value(logical, true)
				&& !visible_cols.contains(logical))
				visible_cols.append(logical);
		}
		for (int i = 0; i < COL_COUNT; ++i) {
			if (plc_data.colVisible.value(i, true) && !visible_cols.contains(i))
				visible_cols.append(i);
		}
	} else {
		for (int i = 0; i < COL_COUNT; ++i) {
			if (plc_data.colVisible.value(i, true))
				visible_cols.append(i);
		}
	}
	if (visible_cols.isEmpty())
		visible_cols << 0 << 1 << 2; // fallback: Type, Address, Function

	// Default column widths
	QMap<int, qreal> col_widths;
	for (int col : visible_cols) {
		if (plc_data.colWidths.contains(col) && plc_data.colWidths[col] > 0)
			col_widths[col] = plc_data.colWidths[col];
		else {
			switch (col) {
				case 0: col_widths[col] = 35; break; // Type
				case 1: col_widths[col] = 25; break; // Address
				case 2: col_widths[col] = 50; break; // Function
				case 3: col_widths[col] = 40; break; // Comment
				case 5: col_widths[col] = 30; break; // CrossRef
				default: col_widths[col] = 30; break;
			}
		}
	}

	qreal row_h = plc_data.rowHeight > 0 ? plc_data.rowHeight : 8.0;
	qreal header_h = plc_data.showHeaders ? (row_h + 2.0) : 0;

	qreal total_width = 0;
	for (int col : visible_cols)
		total_width += col_widths[col];

	int total_ios = plc_data.ios.size();

	// Collect active break positions (sorted)
	QList<int> breaks;
	for (int bp : plc_data.breakPositions) {
		if (bp > 0 && bp < total_ios && !breaks.contains(bp))
			breaks.append(bp);
	}
	std::sort(breaks.begin(), breaks.end());

	// Build block boundaries
	QList<int> block_starts;
	block_starts.append(0);
	for (int bp : breaks)
		block_starts.append(bp);

	qreal total_height;
	int block_count = block_starts.size();

	if (block_count > 1) {
		// Find the tallest block (most rows)
		int max_rows = 0;
		for (int b = 0; b < block_count; ++b) {
			int start = block_starts.at(b);
			int end = (b + 1 < block_starts.size()) ? block_starts.at(b + 1) : total_ios;
			max_rows = qMax(max_rows, end - start);
		}
		total_height = header_h + max_rows * row_h;
		total_width = total_width * block_count + (block_count - 1) * 3;
	} else {
		total_height = header_h + total_ios * row_h;
	}

	return QSizeF(total_width, total_height);
}

/**
	@brief PartPlcTable::paint
	Draw this PLC table
	@param painter
	@param options
	@param widget
*/
void PartPlcTable::paint(QPainter *painter, const QStyleOptionGraphicsItem *options, QWidget *widget)
{
	Q_UNUSED(widget);
	Q_UNUSED(options);

	// Auto-size from PLC data
	QSizeF table_size = calculateTableSize();
	if (m_rect.size() != table_size) {
		prepareGeometryChange();
		QPointF top_left = m_rect.topLeft();
		m_rect = QRectF(top_left, table_size);
	}

	applyStylesToQPainter(*painter);
	QPen t = painter->pen();
	t.setCosmetic(options && options->levelOfDetailFromTransform(painter->worldTransform()) < 1.0);
	if (isSelected())
		t.setColor(Qt::red);
	t.setJoinStyle(Qt::MiterJoin);
	if (!m_rect.width() || !m_rect.height())
		t.setWidth(0);
	painter->setPen(t);

	// Get PLC data
	ElementData ed = (elementEditor() && elementScene()) ? elementScene()->elementData() : ElementData();
	if (ed.m_master_type != ElementData::PLC) {
		// Draw placeholder
		painter->setBrush(QColor(255, 255, 200));
		painter->drawRect(m_rect);
		painter->drawText(m_rect, Qt::AlignCenter, QObject::tr("Table PLC"));
		return;
	}

	const auto &plc_data = ed.plcMasterData();
	if (plc_data.ios.isEmpty()) {
		painter->setBrush(QColor(255, 255, 200));
		painter->drawRect(m_rect);
		painter->drawText(m_rect, Qt::AlignCenter, QObject::tr("Table PLC (vide)"));
		return;
	}

	const int COL_TYPE      = 0;
	const int COL_ADDRESS   = 1;
	const int COL_FUNCTION  = 2;
	const int COL_COMMENT   = 3;
	const int COL_CROSSREF  = 4;
	const int COL_COUNT     = 5;

	QMap<int, QString> headers;
	headers[COL_TYPE]     = QObject::tr("Type");
	headers[COL_ADDRESS]  = QObject::tr("Adresse");
	headers[COL_FUNCTION] = QObject::tr("Fonction");
	headers[COL_COMMENT]  = QObject::tr("Commentaire");
	headers[COL_CROSSREF] = QObject::tr("Réf. croisée");

	// Override with custom column names if set
	if (!plc_data.columnNames.isEmpty()) {
		QList<int> all_cols;
		all_cols << COL_TYPE << COL_ADDRESS << COL_FUNCTION << COL_COMMENT << COL_CROSSREF;
		for (int i = 0; i < qMin(plc_data.columnNames.size(), all_cols.size()); ++i) {
			if (!plc_data.columnNames.at(i).isEmpty())
				headers[all_cols.at(i)] = plc_data.columnNames.at(i);
		}
	}

	QList<int> visible_cols;
	if (!plc_data.columnOrder.isEmpty()) {
		for (int logical : plc_data.columnOrder) {
			if (logical >= 0 && logical < COL_COUNT
				&& plc_data.colVisible.value(logical, true)
				&& !visible_cols.contains(logical))
				visible_cols.append(logical);
		}
		for (int i = 0; i < COL_COUNT; ++i) {
			if (plc_data.colVisible.value(i, true) && !visible_cols.contains(i))
				visible_cols.append(i);
		}
	} else {
		for (int i = 0; i < COL_COUNT; ++i) {
			if (plc_data.colVisible.value(i, true))
				visible_cols.append(i);
		}
	}
	if (visible_cols.isEmpty())
		return;

	QMap<int, qreal> col_widths;
	for (int col : visible_cols) {
		if (plc_data.colWidths.contains(col) && plc_data.colWidths[col] > 0)
			col_widths[col] = plc_data.colWidths[col];
		else {
			switch (col) {
				case COL_TYPE:     col_widths[col] = 35; break;
				case COL_ADDRESS:  col_widths[col] = 25; break;
				case COL_FUNCTION: col_widths[col] = 50; break;
				case COL_COMMENT:  col_widths[col] = 40; break;
				case COL_CROSSREF: col_widths[col] = 30; break;
				default:           col_widths[col] = 30; break;
			}
		}
	}

	qreal row_h = plc_data.rowHeight > 0 ? plc_data.rowHeight : 8.0;
	qreal header_h = plc_data.showHeaders ? (row_h + 2.0) : 0;

	int total_ios = plc_data.ios.size();

	// Collect active break positions (sorted)
	QList<int> breaks;
	for (int bp : plc_data.breakPositions) {
		if (bp > 0 && bp < total_ios && !breaks.contains(bp))
			breaks.append(bp);
	}
	std::sort(breaks.begin(), breaks.end());

	// Build block boundaries: start, break1, break2, ..., end
	QList<int> block_starts;
	block_starts.append(0);
	for (int bp : breaks)
		block_starts.append(bp);
	int block_count = block_starts.size();

	// Draw background
	painter->save();
	painter->setPen(Qt::NoPen);
	painter->setBrush(Qt::white);
	painter->drawRect(m_rect);
	painter->restore();

	// Draw outer border
	QPen border_pen(Qt::black, 0.5);
	painter->setPen(border_pen);
	painter->setBrush(Qt::NoBrush);
	painter->drawRect(m_rect);

	// Draw each block
	for (int block = 0; block < block_count; ++block) {
		qreal block_w = 0;
		for (int col : visible_cols)
			block_w += col_widths[col];

		qreal block_x = m_rect.x() + block * (block_w + 3);
		qreal cx = block_x;

		// Draw column headers
		QFont header_font = plc_data.headerFont.family().isEmpty()
			? painter->font() : plc_data.headerFont;
		header_font.setBold(true);
		painter->setFont(header_font);

		for (int col : visible_cols) {
			QRectF header_rect(cx, m_rect.y(), col_widths[col], header_h);
			painter->fillRect(header_rect, QColor(220, 220, 220));
			painter->setPen(border_pen);
			painter->drawRect(header_rect);
			QString header_text = headers.value(col, QString());
			painter->drawText(header_rect, Qt::AlignCenter, header_text);
			cx += col_widths[col];
		}

		// Draw IO rows
		QFont cell_font = plc_data.cellFont.family().isEmpty()
			? painter->font() : plc_data.cellFont;
		painter->setFont(cell_font);
		int start_idx = block_starts.at(block);
		int end_idx = (block + 1 < block_starts.size())
			? block_starts.at(block + 1) : total_ios;

		for (int row = 0; row < (end_idx - start_idx); ++row) {
			int io_idx = start_idx + row;
			const ElementData::PlcIO &io = plc_data.ios.at(io_idx);

			qreal ry = m_rect.y() + header_h + row * row_h;
			cx = block_x;

			for (int col : visible_cols) {
				QRectF cell_rect(cx, ry, col_widths[col], row_h);
				painter->setPen(border_pen);
				painter->drawRect(cell_rect);

				QString cell_text;
				switch (col) {
					case COL_TYPE:     cell_text = ElementData::translatedPlcIOType(io.type); break;
					case COL_ADDRESS:  cell_text = io.address; break;
					case COL_FUNCTION: cell_text = io.functionText; break;
					case COL_COMMENT:  cell_text = io.comment; break;
					case COL_CROSSREF: cell_text = io.crossRef; break;
				}

				QRectF text_rect = cell_rect.adjusted(1, 0, -1, 0);
				painter->drawText(text_rect, Qt::AlignLeft | Qt::AlignVCenter, cell_text);

				cx += col_widths[col];
			}
		}
	}

	if (m_hovered)
		drawShadowShape(painter);

	if (isSelected())
		drawCross(m_rect.center(), painter);
}

/**
	@brief PartPlcTable::toXml
	Export this PLC table part in xml
	@param xml_document : Xml document to use for create the xml element.
	@return an xml element that describe this part
*/
const QDomElement PartPlcTable::toXml(QDomDocument &xml_document) const
{
	QDomElement xml_element = xml_document.createElement("plc_table");
	qreal x = qRound(m_rect.x() * 100.0) / 100.0;
	qreal y = qRound(m_rect.y() * 100.0) / 100.0;

	xml_element.setAttribute("x", QString::number(x));
	xml_element.setAttribute("y", QString::number(y));

	stylesToXml(xml_element);
	return xml_element;
}

/**
	@brief PartPlcTable::fromXml
	Import the properties of this PLC table part from a xml element.
	@param qde : Xml document to use.
*/
void PartPlcTable::fromXml(const QDomElement &qde)
{
	stylesFromXml(qde);
	qreal x = qde.attribute("x", "0").toDouble();
	qreal y = qde.attribute("y", "0").toDouble();
	setPos(mapFromScene(x, y));

	// Auto-size from PLC data
	QSizeF table_size = calculateTableSize();
	prepareGeometryChange();
	m_rect = QRectF(QPointF(0, 0), table_size);
	update();
}

/**
	@brief PartPlcTable::rect
	@return : Returns the item's rectangle.
*/
QRectF PartPlcTable::rect() const
{
	return m_rect;
}

/**
	@brief PartPlcTable::setRect
	Sets the item's rectangle to be the given rectangle.
	@param rect
*/
void PartPlcTable::setRect(const QRectF &rect)
{
	if (rect == m_rect) return;
	prepareGeometryChange();
	m_rect = rect;
	adjustHandlerPos();
	update();
}

/**
	@brief PartPlcTable::sceneGeometricRect
	@return the minimum, margin-less rectangle this part can fit into, in scene
	coordinates.
*/
QRectF PartPlcTable::sceneGeometricRect() const
{
	return(mapToScene(m_rect).boundingRect());
}

/**
	@brief PartPlcTable::shape
	@return the shape of this item
*/
QPainterPath PartPlcTable::shape() const
{
	QPainterPath fill;
	fill.addRect(m_rect);

	QPainterPath stroke;
	stroke.addRect(m_rect);

	QPainterPathStroker pps;
	pps.setWidth(m_hovered? penWeight()+SHADOWS_HEIGHT : penWeight());
	stroke = pps.createStroke(stroke);

	return fill.united(stroke);
}

QPainterPath PartPlcTable::shadowShape() const
{
	QPainterPath shape;
	shape.addRect(m_rect);

	QPainterPathStroker pps;
	pps.setWidth(penWeight());

	return (pps.createStroke(shape));
}

/**
	@brief PartPlcTable::boundingRect
	@return Bounding rectangle this part can fit into
*/
QRectF PartPlcTable::boundingRect() const
{
	qreal adjust = (SHADOWS_HEIGHT + penWeight()) / 2;
	if (penWeight() == 0) adjust += 0.5;

	QRectF r = m_rect.normalized();
	r.adjust(-adjust, -adjust, adjust, adjust);

	return(r);
}

/**
	@brief PartPlcTable::isUseless
	@return true if this part is irrelevant and does not deserve to be saved.
	A PLC table part is always relevant.
*/
bool PartPlcTable::isUseless() const
{
	return false;
}

/**
	@brief PartPlcTable::startUserTransformation
	@param initial_selection_rect
*/
void PartPlcTable::startUserTransformation(const QRectF &initial_selection_rect)
{
	Q_UNUSED(initial_selection_rect)
	saved_points_.clear();
	saved_points_ << mapToScene(m_rect.topLeft()) << mapToScene(m_rect.bottomRight());
}

/**
	@brief PartPlcTable::handleUserTransformation
	@param initial_selection_rect
	@param new_selection_rect
*/
void PartPlcTable::handleUserTransformation(const QRectF &initial_selection_rect, const QRectF &new_selection_rect)
{
	QList<QPointF> mapped_points = mapPoints(initial_selection_rect, new_selection_rect, saved_points_);
	setRect(QRectF(mapFromScene(mapped_points.at(0)), mapFromScene(mapped_points.at(1))));
}

/**
	@brief PartPlcTable::mouseReleaseEvent
*/
void PartPlcTable::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	CustomElementGraphicPart::mouseReleaseEvent(event);
}

/**
	@brief PartPlcTable::itemChange
	@param change
	@param value
	@return
*/
QVariant PartPlcTable::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemPositionHasChanged)
	{
		adjustHandlerPos();
	}
	else if (change == ItemSceneChange)
	{
		setSelected(false);
	}
	else if (change == ItemSceneHasChanged)
	{
		if (ElementScene *es = elementScene()) {
			connect(es, &ElementScene::elementInfoChanged,
					this, [this]() {
				QSizeF table_size = calculateTableSize();
				if (m_rect.size() != table_size) {
					QPointF top_left = m_rect.topLeft();
					setRect(QRectF(top_left, table_size));
				}
				update();
			});
		}
	}

	return QGraphicsItem::itemChange(change, value);
}

/**
	@brief PartPlcTable::sceneEventFilter
	@param watched
	@param event
	@return
*/
bool PartPlcTable::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
	if(watched->type() == QetGraphicsHandlerItem::Type)
	{
		QetGraphicsHandlerItem *qghi = qgraphicsitem_cast<QetGraphicsHandlerItem *>(watched);

		if(m_handler_vector.contains(qghi))
		{
			m_vector_index = m_handler_vector.indexOf(qghi);
			if (m_vector_index != -1)
			{
				if(event->type() == QEvent::GraphicsSceneMousePress)
				{
					handlerMousePressEvent(qghi, static_cast<QGraphicsSceneMouseEvent *>(event));
					return true;
				}
				else if(event->type() == QEvent::GraphicsSceneMouseMove)
				{
					handlerMouseMoveEvent(qghi, static_cast<QGraphicsSceneMouseEvent *>(event));
					return true;
				}
				else if (event->type() == QEvent::GraphicsSceneMouseRelease)
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
	@brief PartPlcTable::switchResizeMode
*/
void PartPlcTable::switchResizeMode()
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
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
		addHandler();
		for (QetGraphicsHandlerItem *qghi : m_handler_vector) {
			qghi->setColor(Qt::blue);
		}
	}
}

/**
	@brief PartPlcTable::adjustHandlerPos
*/
void PartPlcTable::adjustHandlerPos()
{
	if (m_handler_vector.isEmpty())
		return;

	QVector<QPointF> points_vector = QetGraphicsHandlerUtility::pointsForRect(m_rect);

	if (m_handler_vector.size() == points_vector.size())
	{
		points_vector = mapToScene(points_vector);
		for (int i = 0 ; i < points_vector.size() ; ++i)
			m_handler_vector.at(i)->setPos(points_vector.at(i));
	}
	else
	{
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
		addHandler();
	}
}

void PartPlcTable::handlerMousePressEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi)
	Q_UNUSED(event)

	m_old_rect = m_rect;
}

void PartPlcTable::handlerMouseMoveEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi)

	QPointF new_pos = event->scenePos();
	if (event->modifiers() != Qt::ControlModifier)
		new_pos = elementScene()->snapToGrid(event->scenePos());
	new_pos = mapFromScene(new_pos);

	setRect(QetGraphicsHandlerUtility::rectForPosAtIndex(m_rect, new_pos, m_vector_index));
	adjustHandlerPos();
}

void PartPlcTable::handlerMouseReleaseEvent(QetGraphicsHandlerItem *qghi, QGraphicsSceneMouseEvent *event)
{
	Q_UNUSED(qghi)
	Q_UNUSED(event)

	QUndoCommand *undo = new QUndoCommand("Modifier une table PLC");
	if (m_old_rect != m_rect) {
		QPropertyUndoCommand *u = new QPropertyUndoCommand(this, "rect", QVariant(m_old_rect.normalized()), QVariant(m_rect.normalized()), undo);
		u->setAnimated(true, false);
	}

	elementScene()->undoStack().push(undo);
	m_vector_index = -1;
}

/**
	@brief PartPlcTable::addHandler
	Do not add resize handlers - table size is data-driven.
	Move is handled by the base class QGraphicsItem drag behavior.
*/
void PartPlcTable::addHandler()
{
	// No resize handlers - size comes from PLC data
}

/**
	@brief PartPlcTable::removeHandler
	Remove the handlers of this item
*/
void PartPlcTable::removeHandler()
{
	if (!m_handler_vector.isEmpty())
	{
		qDeleteAll(m_handler_vector);
		m_handler_vector.clear();
	}
}
