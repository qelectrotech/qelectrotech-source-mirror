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
#ifndef GROUPITEMSCOMMAND_H
#define GROUPITEMSCOMMAND_H

#include <QList>
#include <QPointer>
#include <QUndoCommand>
#include <QUuid>

class Diagram;
class QGraphicsItem;
class QGraphicsObject;

/**
	@brief The GroupItemsCommand class
	Groups or ungroups the selected items of a diagram (discussion #1070).
	Undo gives every item back the exact group it had, so a redo after it
	still refers to the same groups.
*/
class GroupItemsCommand : public QUndoCommand
{
	public:
		static GroupItemsCommand *group(Diagram *diagram);
		static GroupItemsCommand *ungroup(Diagram *diagram);
		static bool isGroupable(const QGraphicsItem *item);
		static bool canGroup(Diagram *diagram);
		static bool canUngroup(Diagram *diagram);

		void undo() override;
		void redo() override;

	private:
		GroupItemsCommand(Diagram *diagram, const QList<QGraphicsObject *> &items, const QUuid &group);
		void apply(bool redo);

		struct Change {
			QPointer<QGraphicsObject> item;
			QUuid before;
		};

		QPointer<Diagram> m_diagram;
		QList<Change> m_changes;
		QUuid m_group;
};

#endif // GROUPITEMSCOMMAND_H
