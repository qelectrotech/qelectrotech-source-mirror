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
#ifndef TEXTANCHOR_H
#define TEXTANCHOR_H

#include <QGraphicsItem>
#include <QPointF>
#include <QRectF>

/**
	The anchor point of a text: the point of the text chosen by its
	alignment (top-left, right edge, centre...). It is the point that
	stays in place when the text changes, and the point shown as
	"Position X/Y" in the text properties.

	The saved position of a text is still its top-left corner (pos()),
	so existing projects and symbols are read and written unchanged.
*/
namespace TextAnchor
{
	/**
		@return the point of rect chosen by alignment, in the
		coordinates of rect. Left and top are x = 0 and y = 0, as
		finishAlignment() of the text items assumes.
	*/
	inline QPointF localPoint(const QRectF &rect, Qt::Alignment alignment)
	{
		qreal x = 0, y = 0;

		if (alignment & Qt::AlignRight)
			x = rect.right();
		else if (alignment & Qt::AlignHCenter)
			x = rect.center().x();

		if (alignment & Qt::AlignBottom)
			y = rect.bottom();
		else if (alignment & Qt::AlignVCenter)
			y = rect.center().y();

		return QPointF(x, y);
	}

	/**
		@return the anchor point of item for alignment, in parent
		coordinates. For a top-left alignment this is item->pos().
		The rotation of the item is taken into account.
	*/
	inline QPointF pos(const QGraphicsItem *item, Qt::Alignment alignment)
	{
		return item->pos()
				+ item->mapToParent(localPoint(item->boundingRect(), alignment))
				- item->mapToParent(QPointF(0, 0));
	}

	/**
		@return the position (top-left corner) to give to item so that
		its anchor point for alignment is at anchor.
	*/
	inline QPointF itemPosFor(const QGraphicsItem *item,
							  Qt::Alignment alignment,
							  const QPointF &anchor)
	{
		return item->pos() + anchor - pos(item, alignment);
	}
}

#endif // TEXTANCHOR_H
