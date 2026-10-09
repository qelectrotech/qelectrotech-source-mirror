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
#ifndef IMAGEDROP_H
#define IMAGEDROP_H

#include <QImage>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QList>
#include <QStringList>

class QMimeData;

/**
	Picture files dropped on a folio: which files qualify, how they are
	loaded, and where they are placed. Kept free of any QElectroTech class
	so that it can be tested on its own.
*/
namespace ImageDrop
{
	/// Pictures are embedded in the project, so larger files are refused.
	constexpr qint64 maxFileBytes = 10LL * 1024 * 1024;
	/// Refuses decompression bombs before any pixel is allocated.
	constexpr qint64 maxPixels = 64LL * 1024 * 1024;
	/// Space left between the pictures of one drop laid out in a grid, as
	/// a share of a grid cell.
	constexpr qreal gridGap = 0.05;
	/// Free margin left between a scaled-down picture and the folio frame,
	/// as a share of the frame on each side.
	constexpr qreal frameMargin = 0.2;

	/// The suffixes the "add image" file dialog offers, lower case.
	QStringList supportedSuffixes();

	/// The local picture files among the URLs of @a mime, in drop order.
	QStringList imageFiles(const QMimeData *mime);

	/// True when @a mime carries URLs and none of them is a picture file:
	/// the drop is meant for someone else (a .qet project, an element).
	bool hasOnlyOtherUrls(const QMimeData *mime);

	/**
		Load the picture at @a path, checking that it is a regular file of
		at most maxFileBytes and maxPixels. On failure the image is null
		and @a error, if given, says why.
	*/
	QImage load(const QString &path, QString *error = nullptr);

	/// @a frame shrunk by frameMargin on every side.
	QRectF innerFrame(const QRectF &frame);

	/**
		Scale that fits a picture of @a size into innerFrame(@a frame),
		keeping its proportions; 1.0 when it already fits. Pictures are
		never enlarged.
	*/
	qreal fitScale(const QSizeF &size, const QRectF &frame);

	/**
		@a picture moved by the least amount that puts it inside @a area;
		centred on @a area along an axis where it is larger.
	*/
	QRectF keepInside(const QRectF &picture, const QRectF &area);

	/**
		Lay out pictures of @a sizes, dropped together, side by side in a
		grid filling @a area: as many columns as needed for a roughly
		square grid, every picture shrunk -- never enlarged -- into its
		cell, proportions kept, and centred in it.
		@return where each picture goes, in the order of @a sizes
	*/
	QList<QRectF> gridLayout(const QList<QSizeF> &sizes, const QRectF &area);
}

#endif // IMAGEDROP_H
