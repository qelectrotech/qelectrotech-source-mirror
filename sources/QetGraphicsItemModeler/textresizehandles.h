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
#ifndef TEXTRESIZEHANDLES_H
#define TEXTRESIZEHANDLES_H

#include "../textresize.h"

#include <QCoreApplication>
#include <QGraphicsObject>
#include <QPointer>
#include <QTransform>
#include <QUndoCommand>
#include <QVector>

class QetGraphicsHandlerItem;
class QGraphicsTextItem;

/**
	@brief The TextResizeHandles class
	Four corner handles to change the width of a text box with the mouse.
	The opposite corner of the dragged one stays in place, the text wraps
	to the new width and the height of the box follows the text.
	Double-clicking a handle goes back to the automatic width.

	The text must have a qreal "textWidth" property (-1 = automatic width).
	The handles are a child of the text, so they follow it when it, or its
	symbol, is moved, rotated or zoomed (qelectrotech#1002).

	The width is changed live while dragging; the owner of the text makes
	the change undoable when resizeFinished() is emitted, by pushing a
	TextResizeCommand on its own undo stack.
*/
class TextResizeHandles : public QGraphicsObject
{
		Q_OBJECT

	public:
		TextResizeHandles(QGraphicsTextItem *text, qreal handle_size = 10);
		~TextResizeHandles() override;

		QRectF boundingRect() const override;
		void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override {}

		QetGraphicsHandlerItem *handle(TextResize::Corner corner) const;
		void updateHandlesPos();

	signals:
		void resizeFinished(qreal old_width, qreal new_width,
							QPointF old_pos, QPointF new_pos);

	protected:
		bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;
		QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

	private:
		void installHandleFilters();
		void pressed(TextResize::Corner corner, const QPointF &scene_pos);
		void moved(const QPointF &scene_pos);
		void released();
		void resetToAutomaticWidth();
		qreal currentWidth() const;

		QGraphicsTextItem *m_text;
		QVector<QetGraphicsHandlerItem *> m_handles;
		QMetaObject::Connection m_size_connection;

			//The drag in progress
		bool m_dragging = false;
		TextResize::Corner m_dragged = TextResize::BottomRight;
		QTransform m_press_inverse;
		QPointF m_press_scene,
				m_fixed_local,
				m_fixed_scene,
				m_old_pos;
		qreal m_old_width = -1,
			  m_min_width = 0;
};

/**
	@brief The TextResizeCommand class
	Undo a width change made with TextResizeHandles: the width, then the
	position, which keeps the opposite corner of the dragged one in place.
	Both are set in the same order on undo and redo, because a text may
	move itself when its width changes (to keep its alignment point).
*/
class TextResizeCommand : public QUndoCommand
{
		Q_DECLARE_TR_FUNCTIONS(TextResizeCommand)

	public:
		TextResizeCommand(QGraphicsObject *text,
						  qreal old_width, qreal new_width,
						  const QPointF &old_pos, const QPointF &new_pos,
						  QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		void apply(qreal width, const QPointF &pos);

		QPointer<QGraphicsObject> m_text;
		qreal m_old_width,
			  m_new_width;
		QPointF m_old_pos,
				m_new_pos;
};

#endif // TEXTRESIZEHANDLES_H
