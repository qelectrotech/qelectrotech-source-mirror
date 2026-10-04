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
#include <QSizeF>
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
	/// Offset between the pictures of one drop, in scene units.
	constexpr qreal cascadeStep = 20.0;
	/// Share of the visible area a large dropped picture is fitted into.
	constexpr qreal fitMargin = 0.9;

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

	/**
		Scale that fits a picture of @a size into fitMargin of @a available,
		keeping its proportions; 1.0 when it already fits. Pictures are
		never enlarged.
	*/
	qreal fitScale(const QSizeF &size, const QSizeF &available);

	/// Offset of the @a index-th picture of one drop from the first one.
	QPointF cascadeOffset(int index);
}

#endif // IMAGEDROP_H
