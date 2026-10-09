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
#include "textresizehandles.h"

#include "qetgraphicshandleritem.h"

#include <QAbstractTextDocumentLayout>
#include <QCursor>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QTextDocument>

#include <cmath>
#include <utility>

/**
	@brief TextResizeHandles::TextResizeHandles
	@param text : the text to resize, becomes the parent of the handles.
	The text may be in a scene or not yet.
	@param handle_size : see QETUtils::graphicsHandlerSize()
*/
TextResizeHandles::TextResizeHandles(QGraphicsTextItem *text, qreal handle_size) :
	m_text(text)
{
	setFlag(QGraphicsItem::ItemHasNoContents);
	setParentItem(text);

	for (int i = TextResize::TopLeft ; i <= TextResize::BottomLeft ; ++i)
	{
		auto *handle = new QetGraphicsHandlerItem(handle_size);
		handle->setParentItem(this);
		handle->setColor(Qt::darkGreen);
		handle->setToolTip(tr("Glisser pour changer la largeur du texte, "
							  "double-cliquer pour une largeur automatique"));
		handle->setCursor(i == TextResize::TopLeft || i == TextResize::BottomRight
						  ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor);
		m_handles << handle;
	}
	installHandleFilters();

	m_size_connection = connect(text->document()->documentLayout(),
								&QAbstractTextDocumentLayout::documentSizeChanged,
								this, &TextResizeHandles::updateHandlesPos);
	updateHandlesPos();
}

TextResizeHandles::~TextResizeHandles()
{
	disconnect(m_size_connection);
}

QRectF TextResizeHandles::boundingRect() const {
	return QRectF();
}

/**
	@return the handle at corner
*/
QetGraphicsHandlerItem *TextResizeHandles::handle(TextResize::Corner corner) const {
	return m_handles.at(corner);
}

/**
	@brief TextResizeHandles::updateHandlesPos
	Put the handles at the corners of the box of the text. Called when the
	size of the text changes; the owner calls it when the box changes in an
	other way.
	The box is boundingRect(), the box a user sees selected and resizes,
	not the tighter frame of a framed text (qelectrotech#591).
*/
void TextResizeHandles::updateHandlesPos()
{
	const QRectF rect = m_text->boundingRect();
	for (int i = TextResize::TopLeft ; i <= TextResize::BottomLeft ; ++i)
		m_handles.at(i)->setPos(TextResize::cornerOf(rect, TextResize::Corner(i)));
}

/**
	@brief TextResizeHandles::itemChange
	A scene drops the event filters of an item that leaves it, so the
	handles stop reacting to the mouse when the text is removed from its
	scene and added back (undoing a delete does that), or when they were
	created before the text was in a scene. Install them again each time
	these handles enter a scene.
*/
QVariant TextResizeHandles::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == QGraphicsItem::ItemSceneHasChanged)
		installHandleFilters();
	return QGraphicsObject::itemChange(change, value);
}

/**
	@brief TextResizeHandles::installHandleFilters
	Filter the mouse events of the handles, when they are in a scene.
*/
void TextResizeHandles::installHandleFilters()
{
	if (!scene())
		return;
	for (QetGraphicsHandlerItem *handle : std::as_const(m_handles))
		if (handle->scene() == scene())
			handle->installSceneEventFilter(this);
}

bool TextResizeHandles::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
	const int index = m_handles.indexOf(static_cast<QetGraphicsHandlerItem *>(watched));
	if (index < 0)
		return false;

	switch (event->type())
	{
		case QEvent::GraphicsSceneMousePress:
		{
			auto *mouse_event = static_cast<QGraphicsSceneMouseEvent *>(event);
			if (mouse_event->button() != Qt::LeftButton)
				return false;
			pressed(TextResize::Corner(index), mouse_event->scenePos());
			return true;
		}
		case QEvent::GraphicsSceneMouseMove:
			if (m_dragging)
				moved(static_cast<QGraphicsSceneMouseEvent *>(event)->scenePos());
			return true;
		case QEvent::GraphicsSceneMouseRelease:
			released();
			return true;
		case QEvent::GraphicsSceneMouseDoubleClick:
			resetToAutomaticWidth();
			return true;
		default:
			return false;
	}
}

void TextResizeHandles::pressed(TextResize::Corner corner, const QPointF &scene_pos)
{
	m_dragging = true;
	m_press_scene = scene_pos;
	m_dragged = corner;
	m_old_width = currentWidth();
	m_old_pos = m_text->pos();
	m_press_inverse = m_text->sceneTransform().inverted();
	m_fixed_local = TextResize::cornerOf(m_text->boundingRect(), TextResize::opposite(corner));
	m_fixed_scene = m_text->mapToScene(m_fixed_local);
	m_min_width = TextResize::minimumWidth(m_text->document());
}

void TextResizeHandles::moved(const QPointF &scene_pos)
{
		//A click without a move keeps an automatic width automatic
	if (scene_pos == m_press_scene)
		return;

		//Whole pixels, as in the width spin boxes
	const qreal width = qMax(qreal(qRound(TextResize::widthForDrag(m_press_inverse, m_fixed_local,
																	scene_pos, m_dragged, 0))),
							 std::ceil(m_min_width));
	m_text->setProperty("textWidth", width);
	TextResize::pinCorner(m_text, TextResize::opposite(m_dragged), m_fixed_scene);
	updateHandlesPos();
}

void TextResizeHandles::released()
{
	if (!m_dragging)
		return;
	m_dragging = false;

	const qreal new_width = currentWidth();
	if (!qFuzzyCompare(m_old_width, new_width) || m_old_pos != m_text->pos())
		emit resizeFinished(m_old_width, new_width, m_old_pos, m_text->pos());
}

/**
	@brief TextResizeHandles::resetToAutomaticWidth
	Back to the automatic width (-1): the text keeps the point chosen by its
	alignment in place, as it does for any other change of its width.
*/
void TextResizeHandles::resetToAutomaticWidth()
{
	m_dragging = false;

	const qreal old_width = currentWidth();
	if (old_width < 0)
		return;

	const QPointF old_pos = m_text->pos();
	m_text->setProperty("textWidth", qreal(-1));
	updateHandlesPos();
	emit resizeFinished(old_width, currentWidth(), old_pos, m_text->pos());
}

qreal TextResizeHandles::currentWidth() const {
	return m_text->property("textWidth").toReal();
}


/**
	@brief TextResizeCommand::TextResizeCommand
	@param text : a text with a "textWidth" property
	@param old_width
	@param new_width
	@param old_pos
	@param new_pos
	@param parent
*/
TextResizeCommand::TextResizeCommand(QGraphicsObject *text,
									 qreal old_width, qreal new_width,
									 const QPointF &old_pos, const QPointF &new_pos,
									 QUndoCommand *parent) :
	QUndoCommand(parent),
	m_text(text),
	m_old_width(old_width),
	m_new_width(new_width),
	m_old_pos(old_pos),
	m_new_pos(new_pos)
{
	setText(tr("Redimensionner un texte"));
}

void TextResizeCommand::undo() {
	apply(m_old_width, m_old_pos);
}

void TextResizeCommand::redo() {
	apply(m_new_width, m_new_pos);
}

void TextResizeCommand::apply(qreal width, const QPointF &pos)
{
	if (!m_text)
		return;
	m_text->setProperty("textWidth", width);
	m_text->setPos(pos);
}
