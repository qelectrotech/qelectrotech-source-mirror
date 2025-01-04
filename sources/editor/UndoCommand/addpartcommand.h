/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef ADDPARTCOMMAND_H
#define ADDPARTCOMMAND_H

#include <QUndoCommand>
#include <QPointer>
#include <QVector>

class ElementScene;
class QGraphicsItem;

/**
 * @brief The AddPartCommand class
 * Undo command use to add a graphics part into an element scene.
 */
class AddPartCommand : public QUndoCommand
{
	public:
		AddPartCommand(const QString &text, QPointer<ElementScene> scene, QGraphicsItem *part, QUndoCommand *parent = nullptr);
		~AddPartCommand() override;

	private:
		AddPartCommand(const AddPartCommand &);

	public:
		void undo() override;
		void redo() override;

	private:
		QPointer<ElementScene> m_scene;
		QVector<QGraphicsItem *> m_part;
		bool m_first_redo{true};
};

#endif // ADDPARTCOMMAND_H
