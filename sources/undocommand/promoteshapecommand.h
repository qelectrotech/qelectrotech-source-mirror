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
#ifndef PROMOTESHAPECOMMAND_H
#define PROMOTESHAPECOMMAND_H

#include <QUndoCommand>
#include <QPointer>
#include <QDomElement>

class QetShapeItem;

/**
	@brief The PromoteShapeCommand class
	Used whenever a shape's *identity* changes -- a Rectangle's corner is
	dragged independently and it becomes a Polygon, an Arc is explicitly
	converted to a free-form Path, etc.

	The saved .qet file never records what a shape used to be; that fact
	only needs to survive as long as the rest of the edit history does, so
	it lives entirely here, as a full XML snapshot on each side. Undo
	restores the exact prior shape -- type, geometry and transform alike --
	by round-tripping through the same fromXml() every other load uses.
	Once the file is saved and reopened, the promotion is permanent, same
	as any other undo history.
*/
class PromoteShapeCommand : public QUndoCommand
{
	public:
		PromoteShapeCommand(
				QetShapeItem *shape,
				const QDomElement &priorStateXml,
				const QDomElement &newStateXml,
				QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		QPointer<QetShapeItem> m_shape;
		// Deep-cloned into private documents at construction time: a
		// QDomElement is only a reference into whatever QDomDocument it
		// came from, and that document (typically a short-lived one built
		// just to call toXml()) is not guaranteed to outlive this command.
		QDomDocument m_priorDoc, m_newDoc;
};

#endif // PROMOTESHAPECOMMAND_H
