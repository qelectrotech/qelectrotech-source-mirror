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
#ifndef CHANGEELEMENTINFORMATIONCOMMAND_H
#define CHANGEELEMENTINFORMATIONCOMMAND_H

#include "../diagramcontext.h"

#include <QUndoCommand>

class Element;

/**
	@brief The ChangeElementInformationCommand class
	This class manage undo/redo to change the element information.
*/
class ChangeElementInformationCommand : public QUndoCommand
{
	public:
		ChangeElementInformationCommand(
				Element *elmt,
				const DiagramContext &old_info,
				const DiagramContext &new_info,
				QUndoCommand *parent = nullptr);

		ChangeElementInformationCommand(QMap<QPointer<Element>, QPair<DiagramContext, DiagramContext>> map,
										QUndoCommand *parent = nullptr);

		int id() const override {return 1;}
		bool mergeWith(const QUndoCommand *other) override;
		void undo() override;
		void redo() override;

	private:
		void updateProjectDB();

	private:
		QMap<QPointer<Element>, QPair<DiagramContext, DiagramContext>> m_map;
};

#endif // CHANGEELEMENTINFORMATIONCOMMAND_H
