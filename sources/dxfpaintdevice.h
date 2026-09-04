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
#ifndef DXFPAINTDEVICE_H
#define DXFPAINTDEVICE_H

#include <QPaintDevice>
#include <QPaintEngine>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QString>

/**
	@brief The DxfPaintEngine class
	A QPaintEngine that translates the small set of QPainter calls made by
	QGraphicsItem::paint() implementations (drawLines, drawRects,
	drawEllipse, drawPolygon/drawPolyline, drawTextItem) into DXF entities
	written through Createdxf, instead of pixels.

	This exists so that an item's existing, already-correct paint() code
	can be reused unmodified to produce a DXF export: construct a QPainter
	on a DxfPaintDevice targeting the item, call painter.begin()/end()
	around item->paint(&painter, ...), and every primitive the item draws
	becomes a DXF entity at the item's scene position instead of a pixel
	on screen.

	Scope (deliberately not the full QPainter surface - only what
	QElectroTech's own paint() implementations are observed to call):
	  - drawLines      -> LINE
	  - drawRects      -> LWPOLYLINE (closed 4-point outline; DXF has no
	                       filled-rect primitive in the AC1006 dialect this
	                       exporter targets, so brush fill is not emitted -
	                       see fillPath below)
	  - drawEllipse    -> Createdxf::drawArcEllipse (full sweep) or CIRCLE
	  - drawPath       -> arcs (from QPainterPath::Arc elements, used by
	                       drawArc/drawPie) become a sequence of LINE chords;
	                       anything else in the path is flattened to
	                       polyline segments via QPainterPath::toSubpathPolygons
	  - drawPolygon    -> LWPOLYLINE
	  - drawTextItem   -> TEXT (drawText() calls route through this)
	  - fillPath       -> same outline-only handling as drawRects; no HATCH
	                       support in v1 (see design note in the PR)

	Anything outside this list (images, gradients, etc.) is intentionally
	unimplemented and asserts in debug builds rather than silently
	producing an incomplete drawing - callers should know immediately if
	an item they're exporting uses something this engine doesn't cover
	yet, rather than getting a DXF file quietly missing content.
*/
class DxfPaintEngine : public QPaintEngine
{
	public:
		explicit DxfPaintEngine(const QString &filepath);

		bool begin(QPaintDevice *pdev) override;
		bool end() override;
		void updateState(const QPaintEngineState &state) override;

		void drawLines(const QLineF *lines, int lineCount) override;
		void drawRects(const QRectF *rects, int rectCount) override;
		void drawEllipse(const QRectF &rect) override;
		void drawPath(const QPainterPath &path) override;
		void drawPolygon(const QPointF *points, int pointCount, PolygonDrawMode mode) override;
		void drawTextItem(const QPointF &p, const QTextItem &textItem) override;
		void drawPixmap(const QRectF &r, const QPixmap &pm, const QRectF &sr) override;

		Type type() const override { return QPaintEngine::User; }

	private:
		QPointF toDxf(const QPointF &scene_point) const;
		void strokePolygon(const QPolygonF &poly, bool close);

		QString m_filepath;
		QTransform m_world_transform;
		QPen m_pen;
		QBrush m_brush;
		QFont m_font;
};

/**
	@brief The DxfPaintDevice class
	Pairs with DxfPaintEngine. One instance targets one already-open DXF
	file (Createdxf::dxfBegin()/dxfEnd() bracket the whole export, same as
	today - this device only ever appends entities in between).
*/
class DxfPaintDevice : public QPaintDevice
{
	public:
		explicit DxfPaintDevice(const QString &filepath);
		~DxfPaintDevice() override;

		QPaintEngine *paintEngine() const override;

	protected:
		int metric(PaintDeviceMetric metric) const override;

	private:
		QString m_filepath;
		DxfPaintEngine *m_engine;
};

#endif // DXFPAINTDEVICE_H
