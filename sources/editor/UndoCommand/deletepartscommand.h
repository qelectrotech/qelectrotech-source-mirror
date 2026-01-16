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
#ifndef DELETEPARTSCOMMAND_H
#define DELETEPARTSCOMMAND_H

#include <QUndoCommand>
#include <QPointer>
#include <QVector>

class ElementScene;
class QGraphicsItem;

/**
 * @brief The DeletePartsCommand class
 * Class used to remove part from an element scene.
 */
class DeletePartsCommand : public QUndoCommand
{
	public:
		DeletePartsCommand(QPointer<ElementScene> scene, const QVector<QGraphicsItem *> &parts, QUndoCommand *parent = nullptr);
		~DeletePartsCommand() override;
	private:
		DeletePartsCommand(const DeletePartsCommand &);

	public:
		void undo() override;
		void redo() override;

	private:
		QPointer<ElementScene> m_scene;
		QVector<QGraphicsItem *> m_parts;
};

#endif // DELETEPARTSCOMMAND_H
