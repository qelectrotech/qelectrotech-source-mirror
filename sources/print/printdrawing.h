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
		@brief pageSizeInDrawingUnits
		@return the printable size of a page in folio coordinates
	*/
	inline QSizeF pageSizeInDrawingUnits(const QPageLayout &layout, bool full_page)
	{
		const QRectF mm = full_page ? layout.fullRect(QPageLayout::Millimeter)
					    : layout.paintRect(QPageLayout::Millimeter);
		return mm.size() * (drawingDpi / 25.4);
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
			//drawPicture() scales by the ratio of the two devices'
			//resolutions itself: cancel it, then fit the folio.
		painter->scale(scale * drawingDpi / painter->device()->logicalDpiX(),
			       scale * drawingDpi / painter->device()->logicalDpiY());
		painter->drawPicture(QPointF(), picture);
		painter->restore();
	}
}

#endif // PRINTDRAWING_H
