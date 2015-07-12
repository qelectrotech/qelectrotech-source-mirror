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
#ifndef QETSHAPEGEOMETRYCOMMAND_H
#define QETSHAPEGEOMETRYCOMMAND_H

#include <QUndoCommand>
#include <QLineF>
#include <QPolygonF>
#include <QRectF>
class QetShapeItem;

/**
 * @brief The QetShapeGeometryCommand class
 * This undo command class manage the geometry change of a QetShapeItem.
 */
class QetShapeGeometryCommand : public QUndoCommand
{
    public:
		QetShapeGeometryCommand(QetShapeItem *item, const QLineF &old_line, QUndoCommand *parent = nullptr);
		QetShapeGeometryCommand(QetShapeItem *item, const QRectF &old_rect, QUndoCommand *parent = nullptr);
		QetShapeGeometryCommand(QetShapeItem *item, const QPolygonF &old_polygon, QUndoCommand *parent = nullptr);

		void setNewLine    (const QLineF &new_line)       {m_new_line = new_line;}
		void setNewRect    (const QRectF &new_rect)       {m_new_rect = new_rect;}
		void setNewPolygon (const QPolygonF &new_polygon) {m_new_polygon = new_polygon;}

        int id() const {return 5;}
        bool mergeWith(const QUndoCommand *other);
        void redo();
        void undo();

    private:
        QetShapeItem *m_shape_item;
		QLineF m_old_line, m_new_line;
        QPolygonF m_old_polygon, m_new_polygon;
        QRectF m_old_rect, m_new_rect;
};

#endif // QETSHAPEGEOMETRYCOMMAND_H
