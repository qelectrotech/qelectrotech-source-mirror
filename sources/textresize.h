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
#ifndef TEXTRESIZE_H
#define TEXTRESIZE_H

#include <QGraphicsItem>
#include <QScopedPointer>
#include <QTextDocument>
#include <QTextOption>
#include <QTransform>

/**
	The geometry of changing the width of a text box by dragging one of
	its corners: the text wraps to the new width, the opposite corner
	stays where it is drawn.
*/
namespace TextResize
{
	/**
		A corner of the box of a text, in the order of the resize handles.
	*/
	enum Corner {
		TopLeft,
		TopRight,
		BottomRight,
		BottomLeft
	};

	inline Corner opposite(Corner corner) {
		return Corner((int(corner) + 2) % 4);
	}

	inline bool isLeft(Corner corner) {
		return corner == TopLeft || corner == BottomLeft;
	}

	/**
		@return the corner of rect, in the coordinates of rect.
	*/
	inline QPointF cornerOf(const QRectF &rect, Corner corner)
	{
		switch (corner) {
			case TopLeft:     return rect.topLeft();
			case TopRight:    return rect.topRight();
			case BottomRight: return rect.bottomRight();
			case BottomLeft:  return rect.bottomLeft();
		}
		return rect.topLeft();
	}

	/**
		@return the smallest width that document can be given: the width
		of its longest word, which is never broken, plus the margins.
	*/
	inline qreal minimumWidth(const QTextDocument *document)
	{
		QScopedPointer<QTextDocument> narrow(document->clone());
		QTextOption option = narrow->defaultTextOption();
		option.setWrapMode(QTextOption::WordWrap);
		narrow->setDefaultTextOption(option);
		narrow->setTextWidth(0);
		return narrow->size().width();
	}

	/**
		@return the width given by dragging the corner dragged of a text
		box to mouse_scene, while the opposite corner stays at fixed_local.
		@param press_inverse : the inverse of the scene transform of the
		text when the drag started. Moving the text to keep the opposite
		corner in place changes its transform, using the transform of the
		press keeps that move from feeding back into the width.
		@param fixed_local : the opposite corner, in the coordinates of the
		text when the drag started.
		@param min_width : see minimumWidth().
		Vertical movements of the mouse are ignored: the height of the box
		follows the wrapped text.
	*/
	inline qreal widthForDrag(const QTransform &press_inverse,
							  const QPointF &fixed_local,
							  const QPointF &mouse_scene,
							  Corner dragged,
							  qreal min_width)
	{
		const QPointF mouse = press_inverse.map(mouse_scene);
		const qreal width = isLeft(dragged) ? fixed_local.x() - mouse.x()
											: mouse.x() - fixed_local.x();
		return qMax(width, min_width);
	}

	/**
		Move item so that its corner corner is at scene_point, in scene
		coordinates. Right for any rotation, rotation point and alignment.
	*/
	inline void pinCorner(QGraphicsItem *item, Corner corner, const QPointF &scene_point)
	{
		const QPointF now = item->mapToScene(cornerOf(item->boundingRect(), corner));
		QPointF delta = scene_point - now;
		if (const QGraphicsItem *parent = item->parentItem())
			delta = parent->mapFromScene(scene_point) - parent->mapFromScene(now);
		item->setPos(item->pos() + delta);
	}

}

#endif // TEXTRESIZE_H
