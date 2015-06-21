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
#ifndef CHANGESHAPESTYLECOMMAND_H
#define CHANGESHAPESTYLECOMMAND_H

#include <QUndoCommand>

class QetShapeItem;

/**
 * @brief The ChangeShapeStyleCommand class
 * This class manage undo/redo to change the shape style.
 */
class ChangeShapeStyleCommand : public QUndoCommand
{
	public:
		ChangeShapeStyleCommand(QetShapeItem *item, const Qt::PenStyle &old_ps, const Qt::PenStyle new_ps, QUndoCommand *parent = nullptr);

		int id() const {return 4;}
		bool mergeWith(const QUndoCommand *other);
		void undo();
		void redo();

	private:
		QetShapeItem *m_shape;
		Qt::PenStyle m_old_ps, m_new_ps;
};

#endif // CHANGESHAPESTYLECOMMAND_H
