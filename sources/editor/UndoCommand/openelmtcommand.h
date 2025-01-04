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
#ifndef OPENELMTCOMMAND_H
#define OPENELMTCOMMAND_H

#include <QUndoCommand>
#include <QDomDocument>
#include <QPointer>

class ElementScene;
class QGraphicsItem;

class OpenElmtCommand : public QUndoCommand
{
	public:
		OpenElmtCommand(const QDomDocument &document, QPointer<ElementScene> scene, QUndoCommand *parent = nullptr);
		~OpenElmtCommand() override;

		virtual void undo() override;
		virtual void redo() override;

	private:
		QDomDocument m_document;
		bool m_first_redo{true};
		QPointer<ElementScene> m_scene;
		QList<QGraphicsItem *> m_graphics_item;
};

#endif // OPENELMTCOMMAND_H
