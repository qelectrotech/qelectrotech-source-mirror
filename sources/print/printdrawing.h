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
#ifndef PRINTDRAWING_H
#define PRINTDRAWING_H

#include <QGraphicsScene>
#include <QPageLayout>
#include <QPainter>
#include <QPicture>


/**
	@brief QetPrint
	Draws a folio on a printer at the printer's own resolution while its
	texts keep their size relative to the drawing (#1596).

	A folio's coordinates are logical pixels, 96 to the inch, and its texts
	are sized in points. Drawn straight onto a 600 dpi printer, a point is
	worth 6 times more device pixels than at 96 dpi while the drawing is
	only scaled to fit, so the texts come out too big. Forcing the printer
	down to 96 dpi, as QET did on Windows, kept the texts right but made
	the Windows printer round every line to a 96 dpi pixel, so lines of the
	same width printed one or two pixels thick.

	The folio is recorded instead as vector drawing commands on a picture
	that reports 96 dpi, then played back on the printer, scaled to fit.
	Nothing is turned into an image.

	From the "Custom 2.6" build by victorienea (PR #1650).
*/
namespace QetPrint {

	/// The resolution of a folio's coordinates, in logical pixels per inch.
	constexpr qreal drawingDpi = 96.0;

	/**
		@brief The DrawingPicture class
		A QPicture that reports drawingDpi, so texts are recorded at the
		size they have on screen.
	*/
	class DrawingPicture : public QPicture
	{
		protected:
			int metric(PaintDeviceMetric m) const override
			{
				if (m == PdmDpiX || m == PdmDpiY
				    || m == PdmPhysicalDpiX || m == PdmPhysicalDpiY)
					return int(drawingDpi);
				return QPicture::metric(m);
			}
	};

	/**
		@brief unitsPerMm
		@return how many folio units (scene pixels) make a millimetre of
		paper: drawingDpi / 25.4 as before, or the project's pixels per
		millimetre when the folio is printed to scale (docs/mm-scale.md).
		@param px_per_mm : QETProject::pxPerMm() to print to scale, 0 as
		before
	*/
	constexpr qreal unitsPerMm(qreal px_per_mm = 0)
	{
		return px_per_mm > 0 ? px_per_mm : drawingDpi / 25.4;
	}

	/**
		@brief pixelsPerUnit
		@return the device pixels of a printer of @p resolution dpi that
		one folio unit takes up, to scale or at drawingDpi.
		@param px_per_mm : QETProject::pxPerMm() to print to scale, 0 as
		before
	*/
	constexpr qreal pixelsPerUnit(qreal resolution, qreal px_per_mm = 0)
	{
		return px_per_mm > 0 ? resolution / (px_per_mm * 25.4)
				: resolution / drawingDpi;
	}

	/**
		@brief folioPageLayout
		@return the page of a folio of @p folio_px scene pixels: as large
		as the folio on paper, without margins. A folio wider than high is
		a landscape page, since QPageSize knows the sheets upright only.
		@param px_per_mm : QETProject::paperPxPerMm(). To scale (> 0) the
		page is a standard sheet within 3 pt that is not smaller than the
		folio in either direction, else exactly the size of the folio. In
		pixels (0) it is at 96 dpi and QPageSize rounds a size within 3 pt
		of a standard sheet to that sheet, as the export always did.
	*/
	inline QPageLayout folioPageLayout(const QSizeF &folio_px, qreal px_per_mm = 0)
	{
		const qreal pt_per_px = px_per_mm > 0 ? 72.0 / (px_per_mm * 25.4)
						      : 72.0 / drawingDpi;
		QSizeF points(folio_px.width() * pt_per_px, folio_px.height() * pt_per_px);
		const bool wide = points.width() > points.height();
		if (wide) points.transpose();
			//To scale a standard sheet within 3 pt that is not smaller
			//than the folio in either direction (one smaller would cut
			//off its edge), else the size of the folio
		QPageSize size(points, QPageSize::Point);
		if (px_per_mm > 0) {
			const QSizeF sheet = size.size(QPageSize::Point);
			const bool standard = size.id() != QPageSize::Custom
				&& sheet.width() >= points.width() - 0.01
				&& sheet.height() >= points.height() - 0.01;
			if (!standard)
				size = QPageSize(points, QPageSize::Point, QString(),
						 QPageSize::ExactMatch);
		}
		return QPageLayout(size,
				   wide ? QPageLayout::Landscape : QPageLayout::Portrait,
				   QMarginsF(0, 0, 0, 0));
	}

	/**
		@brief pageSizeInDrawingUnits
		@return the printable size of a page in folio coordinates
	*/
	inline QSizeF pageSizeInDrawingUnits(const QPageLayout &layout, bool full_page,
					     qreal px_per_mm = 0)
	{
		const QRectF mm = full_page ? layout.fullRect(QPageLayout::Millimeter)
					    : layout.paintRect(QPageLayout::Millimeter);
		return mm.size() * unitsPerMm(px_per_mm);
	}

	/**
		@brief render
		Draw @p source of @p scene into @p target of @p painter, keeping the
		aspect ratio and anchored at the top left of @p target, like
		QGraphicsScene::render() with Qt::KeepAspectRatio.
	*/
	inline void render(QGraphicsScene *scene, QPainter *painter,
			   const QRectF &target, const QRectF &source)
	{
		if (source.isEmpty() || target.isEmpty())
			return;

		DrawingPicture picture;
		{
			QPainter recorder(&picture);
			scene->render(&recorder, QRectF(QPointF(), source.size()),
				      source, Qt::IgnoreAspectRatio);
		}

		const qreal scale = qMin(target.width() / source.width(),
					 target.height() / source.height());
		painter->save();
		painter->setClipRect(target, Qt::IntersectClip);
		painter->translate(target.topLeft());
			//drawPicture() scales by the device's resolution divided by
			//the screen's (qt_defaultDpi, which a plain QPicture reports:
			//96 on most screens, 72 on macOS): cancel it, then fit the
			//folio (#1977).
		const QPicture screen_dpi;
		const QPaintDevice *device = painter->device();
		painter->scale(scale * screen_dpi.logicalDpiX() / device->logicalDpiX(),
			       scale * screen_dpi.logicalDpiY() / device->logicalDpiY());
		painter->drawPicture(QPointF(), picture);
		painter->restore();
	}

	/**
		@brief svgViewBox
		@return the view box of an SVG of @p size pixels made by playing a
		QPicture on a QSvgGenerator of @p svg_dpi. play() scales by
		@p svg_dpi divided by the screen's resolution (72/96 on most
		screens, 1 on macOS), so the view box follows it or macOS cuts
		the folio to 75 % (#1977).
	*/
	inline QRect svgViewBox(const QSize &size, int svg_dpi)
	{
		const qreal ratio = qreal(svg_dpi) / QPicture().logicalDpiX();
		return QRect(0, 0, int(size.width() * ratio),
			     int(size.height() * ratio));
	}
}

#endif // PRINTDRAWING_H
