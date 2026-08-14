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
#include "dxfpaintdevice.h"
#include "createdxf.h"

#include <QPainterPath>
#include <QTextItem>
#include <QtMath>
#include <QDebug>

/**
	@brief DxfPaintEngine::DxfPaintEngine
	@param filepath path of the already-open DXF file entities are
	appended to (see Createdxf::dxfBegin()/dxfEnd()).
*/
DxfPaintEngine::DxfPaintEngine(const QString &filepath) :
	QPaintEngine(QPaintEngine::AllFeatures),
	m_filepath(filepath)
{
}

bool DxfPaintEngine::begin(QPaintDevice *pdev)
{
	Q_UNUSED(pdev)
	setActive(true);
	return true;
}

bool DxfPaintEngine::end()
{
	setActive(false);
	return true;
}

/**
	@brief DxfPaintEngine::updateState
	Track the properties this engine cares about. Everything drawn is
	mapped through the transform in effect at draw time - this is what
	lets an item's paint() be replayed unmodified at whatever scene
	position/rotation it actually has, exactly as QPainter does for
	on-screen painting.
*/
void DxfPaintEngine::updateState(const QPaintEngineState &state)
{
	QPaintEngine::DirtyFlags flags = state.state();

	if (flags & QPaintEngine::DirtyTransform)
		m_world_transform = state.transform();
	if (flags & QPaintEngine::DirtyPen)
		m_pen = state.pen();
	if (flags & QPaintEngine::DirtyBrush)
		m_brush = state.brush();
	if (flags & QPaintEngine::DirtyFont)
		m_font = state.font();
}

QPointF DxfPaintEngine::toDxf(const QPointF &local_point) const
{
	// Createdxf's QRectF/QLineF/QPolygonF/QPointF convenience overloads
	// each apply xScale/yScale and the sheetHeight Y-flip internally, so
	// this engine's whole job is composing the world transform - not
	// touching DXF units at all.
	return m_world_transform.map(local_point);
}

void DxfPaintEngine::strokePolygon(const QPolygonF &poly, bool close)
{
	if (poly.size() < 2)
		return;
	if (m_pen.style() == Qt::NoPen)
		return;

	QPolygonF mapped;
	mapped.reserve(poly.size());
	for (const QPointF &p : poly)
		mapped << toDxf(p);

	if (close && mapped.first() != mapped.last())
		mapped << mapped.first();

	Createdxf::drawPolyline(m_filepath, mapped, Createdxf::dxfColor(m_pen));
}

void DxfPaintEngine::drawLines(const QLineF *lines, int lineCount)
{
	if (m_pen.style() == Qt::NoPen)
		return;
	for (int i = 0; i < lineCount; ++i)
	{
		QLineF mapped(toDxf(lines[i].p1()), toDxf(lines[i].p2()));
		Createdxf::drawLine(m_filepath, mapped, Createdxf::dxfColor(m_pen));
	}
}

void DxfPaintEngine::drawRects(const QRectF *rects, int rectCount)
{
	for (int i = 0; i < rectCount; ++i)
	{
		QPolygonF corners;
		corners << rects[i].topLeft() << rects[i].topRight()
				<< rects[i].bottomRight() << rects[i].bottomLeft();

		// No HATCH support in v1 (see the design note in the header) - a
		// filled rect is emitted as its outline only, same as an unfilled
		// one. Still emitted (not skipped) when there's no pen but there
		// is a brush, so a filled-only shape isn't dropped entirely.
		if (m_pen.style() != Qt::NoPen || m_brush.style() != Qt::NoBrush)
			strokePolygon(corners, true);
	}
}

void DxfPaintEngine::drawEllipse(const QRectF &rect)
{
	if (m_pen.style() == Qt::NoPen && m_brush.style() == Qt::NoBrush)
		return;

	// A non axis-aligned ellipse (rotation baked into m_world_transform)
	// can't be expressed as Createdxf::drawEllipse(), which only accepts
	// an axis-aligned QRectF. Approximate with a flattened polygon in
	// that case; use the exact CIRCLE/ELLIPSE entity when the transform
	// is axis-aligned, matching what the existing hand-written exporters
	// already produce for unrotated shapes.
	const bool axis_aligned = qFuzzyIsNull(m_world_transform.m12())
							&& qFuzzyIsNull(m_world_transform.m21());
	if (axis_aligned)
	{
		QRectF mapped = m_world_transform.mapRect(rect);
		Createdxf::drawEllipse(m_filepath, mapped, Createdxf::dxfColor(m_pen));
		return;
	}

	QPainterPath path;
	path.addEllipse(rect);
	const auto polygons = path.toSubpathPolygons();
	for (const QPolygonF &poly : polygons)
		strokePolygon(poly, true);
}

/**
	@brief DxfPaintEngine::drawPath
	Handles drawArc()/drawPie() (QPainterPath::Arc elements - emitted as
	chord segments, since Createdxf's ARC-equivalent
	(drawArcEllipse()) only takes an axis-aligned ellipse + angle pair,
	which doesn't compose with an arbitrary path transform) and
	fillPath()/generic paths (flattened to polylines).
*/
void DxfPaintEngine::drawPath(const QPainterPath &path)
{
	if (m_pen.style() == Qt::NoPen && m_brush.style() == Qt::NoBrush)
		return;

	const auto polygons = path.toSubpathPolygons();
	for (const QPolygonF &poly : polygons)
		strokePolygon(poly, path.isEmpty() ? false : true);
}

void DxfPaintEngine::drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode)
{
	if (pointCount < 2)
		return;
	QPolygonF poly;
	poly.reserve(pointCount);
	for (int i = 0; i < pointCount; ++i)
		poly << points[i];

	strokePolygon(poly, mode != PolylineMode);
}

void DxfPaintEngine::drawTextItem(const QPointF &p, const QTextItem &textItem)
{
	qreal font_size = textItem.font().pointSizeF();
	if (font_size < 0)
		font_size = textItem.font().pixelSize();

	// Rotation: extract the angle baked into the world transform by
	// mapping a unit vector, the same approach used to compute rotation
	// for the existing element-text DXF export in exportdialog.cpp.
	QPointF origin = m_world_transform.map(p);
	QPointF right  = m_world_transform.map(p + QPointF(1, 0));
	qreal angle = qRadiansToDegrees(qAtan2(-(right.y() - origin.y()), right.x() - origin.x()));

	Createdxf::drawText(m_filepath, textItem.text(), toDxf(p), font_size,
						 angle, Createdxf::dxfColor(m_pen), 0.72);
}

/**
	@brief DxfPaintEngine::drawPixmap
	Not implemented in v1 - out of scope per the design note in the
	header (CrossRefItem, the only item exported through this engine so
	far, never draws a pixmap). qWarning() rather than a hard failure, so
	an item that does call this in the future degrades to "one entity
	missing" instead of crashing the whole export.
*/
void DxfPaintEngine::drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr)
{
	Q_UNUSED(r)
	Q_UNUSED(pm)
	Q_UNUSED(sr)
	qWarning() << "DxfPaintEngine::drawPixmap: not supported, entity skipped";
}

/**
	@brief DxfPaintDevice::DxfPaintDevice
	@param filepath path of the already-open DXF file (see
	Createdxf::dxfBegin()/dxfEnd() - this device only appends entities in
	between, it does not open/close the file itself).
*/
DxfPaintDevice::DxfPaintDevice(const QString &filepath) :
	QPaintDevice(),
	m_filepath(filepath),
	m_engine(new DxfPaintEngine(filepath))
{
}

DxfPaintDevice::~DxfPaintDevice()
{
	delete m_engine;
}

QPaintEngine *DxfPaintDevice::paintEngine() const
{
	return m_engine;
}

int DxfPaintDevice::metric(PaintDeviceMetric metric) const
{
	// A DXF document has no pixel grid or physical size in the sense
	// QPaintDevice::metric() expects - large-but-finite values here are
	// enough to keep QPainter's own bookkeeping (e.g. clip region setup)
	// from misbehaving; nothing in this engine actually consults them.
	switch (metric)
	{
		case PdmWidth:
		case PdmHeight:
			return 1000000;
		case PdmDpiX:
		case PdmDpiY:
		case PdmPhysicalDpiX:
		case PdmPhysicalDpiY:
			return 96;
		case PdmWidthMM:
		case PdmHeightMM:
			return 1000000;
		case PdmNumColors:
			return 16777216;
		case PdmDepth:
			return 24;
		case PdmDevicePixelRatio:
		case PdmDevicePixelRatioScaled:
			return 1;
		default:
			return 0;
	}
}
