/*
        Copyright 2006-2015 The QElectroTech Team
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
#include "qetshapegeometrycommand.h"
#include "qetshapeitem.h"
#include <QObject>

/**
 * @brief QetShapeGeometryCommand::QetShapeGeometryCommand
 * Constructor for a line shape
 * @param item : item to change the geometry
 * @param old_line : old line
 * @param parent : parent undo command
 */
QetShapeGeometryCommand::QetShapeGeometryCommand(QetShapeItem *item, const QLineF &old_line, QUndoCommand *parent) :
    QUndoCommand(parent),
    m_shape_item(item),
	m_old_line(old_line)
{
    setText(QObject::tr("Modifier la géometrie de : %1").arg(m_shape_item->name()));
}

/**
 * @brief QetShapeGeometryCommand::QetShapeGeometryCommand
 * Constructor for a rectangle or ellipse shape
 * @param item : item to change the geometry
 * @param old_rect : old rectangle
 * @param parent : parent undo command
 */
QetShapeGeometryCommand::QetShapeGeometryCommand(QetShapeItem *item, const QRectF &old_rect, QUndoCommand *parent):
    QUndoCommand(parent),
	m_shape_item(item),
	m_old_rect(old_rect)
{
    setText(QObject::tr("Modifier la géometrie de : %1").arg(m_shape_item->name()));
}

/**
 * @brief QetShapeGeometryCommand::QetShapeGeometryCommand
 * Constructor for polygon shape
 * @param item : item to change the geometry
 * @param old_polygon : old polygon
 * @param parent : parent undo command
 */
QetShapeGeometryCommand::QetShapeGeometryCommand(QetShapeItem *item, const QPolygonF &old_polygon, QUndoCommand *parent):
    QUndoCommand(parent),
	m_shape_item(item),
	m_old_polygon(old_polygon)
{
    setText(QObject::tr("Modifier la géometrie de : %1").arg(m_shape_item->name()));
}

/**
 * @brief QetShapeGeometryCommand::mergeWith
 * Try to merge this undo command with @other
 * @param other
 * @return true if the two command was merged
 */
bool QetShapeGeometryCommand::mergeWith(const QUndoCommand *other)
{
    if (other->id() != id() || other->childCount()) return false;
    const QetShapeGeometryCommand *other_undo = static_cast<const QetShapeGeometryCommand*>(other);
    if (other_undo->m_shape_item != m_shape_item) return false;

    switch (m_shape_item->shapeType())
    {
		case QetShapeItem::Line:      m_new_line = other_undo->m_new_line;       break;
		case QetShapeItem::Rectangle: m_new_rect = other_undo->m_new_rect;       break;
		case QetShapeItem::Ellipse:   m_new_rect = other_undo->m_new_rect;       break;
		case QetShapeItem::Polyline:  m_new_polygon = other_undo->m_new_polygon; break;
    }

    return true;
}

/**
 * @brief QetShapeGeometryCommand::redo
 * Redo this command
 */
void QetShapeGeometryCommand::redo()
{
    switch (m_shape_item->shapeType())
    {
		case QetShapeItem::Line:      m_shape_item->setLine(m_new_line);       break;
		case QetShapeItem::Rectangle: m_shape_item->setRect(m_new_rect);       break;
		case QetShapeItem::Ellipse:   m_shape_item->setRect(m_new_rect);       break;
		case QetShapeItem::Polyline:  m_shape_item->setPolygon(m_new_polygon); break;
    }

    QUndoCommand::redo();
}

/**
 * @brief QetShapeGeometryCommand::undo
 * Undo this command
 */
void QetShapeGeometryCommand::undo()
{
    switch (m_shape_item->shapeType())
    {
		case QetShapeItem::Line:      m_shape_item->setLine(m_old_line);       break;
		case QetShapeItem::Rectangle: m_shape_item->setRect(m_old_rect);       break;
		case QetShapeItem::Ellipse:   m_shape_item->setRect(m_old_rect);       break;
		case QetShapeItem::Polyline:  m_shape_item->setPolygon(m_old_polygon); break;
    }

    QUndoCommand::undo();
}
