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
#ifndef CHANGEELEMENTINFORMATIONCOMMAND_H
#define CHANGEELEMENTINFORMATIONCOMMAND_H

#include <QUndoCommand>
#include "diagramcontext.h"

class Element;

/**
 * @brief The ChangeElementInformationCommand class
 * This class manage undo/redo to change the element information.
 */
class ChangeElementInformationCommand : public QUndoCommand
{
	public:
		ChangeElementInformationCommand(Element *elmt, DiagramContext &old_info, DiagramContext &new_info, QUndoCommand *parent = nullptr);

		virtual int id() const {return 1;}
		virtual bool mergeWith(const QUndoCommand *other);
		virtual void undo();
		virtual void redo();

	private:
		Element       *m_element;
		DiagramContext m_old_info,
					   m_new_info;
};

#endif // CHANGEELEMENTINFORMATIONCOMMAND_H
