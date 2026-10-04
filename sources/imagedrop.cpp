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
#include "imagedrop.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeData>
#include <QUrl>

#include <algorithm>
#include <cmath>

namespace
{
	bool isImageFile(const QUrl &url)
	{
		if (!url.isLocalFile())
			return false;
		const QString suffix = QFileInfo(url.toLocalFile()).suffix().toLower();
		return ImageDrop::supportedSuffixes().contains(suffix);
	}

	QString tr(const char *text)
	{
		return QCoreApplication::translate("ImageDrop", text);
	}
}

QStringList ImageDrop::supportedSuffixes()
{
	return {QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
			QStringLiteral("bmp"), QStringLiteral("svg")};
}

QStringList ImageDrop::imageFiles(const QMimeData *mime)
{
	QStringList files;
	if (!mime || !mime->hasUrls())
		return files;
	for (const QUrl &url : mime->urls())
		if (isImageFile(url))
			files << url.toLocalFile();
	return files;
}

bool ImageDrop::hasOnlyOtherUrls(const QMimeData *mime)
{
	return mime && mime->hasUrls() && imageFiles(mime).isEmpty();
}

QImage ImageDrop::load(const QString &path, QString *error)
{
	auto fail = [error](const QString &message) {
		if (error)
			*error = message;
		return QImage();
	};

	const QFileInfo info(path);
	if (!info.isFile())
		return fail(tr("not a file"));
	if (info.size() > maxFileBytes)
		return fail(tr("the file is larger than 10 MB"));

	QImageReader reader(path);
	const QSize size = reader.size();
	if (size.isValid() && qint64(size.width()) * size.height() > maxPixels)
		return fail(tr("the image has too many pixels"));

	const QImage image = reader.read();
	if (image.isNull())
		return fail(tr("unable to read the image"));
	return image;
}

QRectF ImageDrop::innerFrame(const QRectF &frame)
{
	return frame.adjusted(frame.width() * frameMargin, frame.height() * frameMargin,
						  -frame.width() * frameMargin, -frame.height() * frameMargin);
}

qreal ImageDrop::fitScale(const QSizeF &size, const QRectF &frame)
{
	const QRectF inner = innerFrame(frame);
	if (size.isEmpty() || inner.isEmpty())
		return 1.0;
	const qreal scale = std::min(inner.width() / size.width(),
								 inner.height() / size.height());
	return std::min(1.0, scale);
}

QRectF ImageDrop::keepInside(const QRectF &picture, const QRectF &area)
{
	if (area.isEmpty())
		return picture;
	auto axis = [](qreal start, qreal length, qreal area_start, qreal area_length) {
		if (length >= area_length)
			return area_start + (area_length - length) / 2;
		return std::clamp(start, area_start, area_start + area_length - length);
	};
	QRectF result = picture;
	result.moveLeft(axis(picture.left(), picture.width(), area.left(), area.width()));
	result.moveTop(axis(picture.top(), picture.height(), area.top(), area.height()));
	return result;
}

QList<QRectF> ImageDrop::gridLayout(const QList<QSizeF> &sizes, const QRectF &area)
{
	QList<QRectF> rects;
	const int count = int(sizes.size());
	if (count == 0)
		return rects;
	const int columns = int(std::ceil(std::sqrt(qreal(count))));
	const int rows = (count + columns - 1) / columns;
	const QSizeF cell(area.width() / columns, area.height() / rows);
	const QSizeF room = cell * (1.0 - gridGap);
	for (int i = 0; i < count; ++i)
	{
		const QSizeF size = sizes.at(i);
		qreal scale = 1.0;
		if (!size.isEmpty() && !room.isEmpty())
			scale = std::min({1.0, room.width() / size.width(), room.height() / size.height()});
		QRectF r(QPointF(), size * scale);
		r.moveCenter(area.topLeft() + QPointF(cell.width() * (i % columns + 0.5),
											  cell.height() * (i / columns + 0.5)));
		rects << r;
	}
	return rects;
}
