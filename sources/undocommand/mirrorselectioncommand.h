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
#ifndef MIRRORSELECTIONCOMMAND_H
#define MIRRORSELECTIONCOMMAND_H

#include <QList>
#include <QUndoCommand>

class Diagram;
class Element;

/**
	@brief The MirrorSelectionCommand class
	Mirror elements in place, horizontally (left and right swap) or
	vertically (top and bottom swap), on the folio. See Element::setMirror()
*/
class MirrorSelectionCommand : public QUndoCommand
{
	public:
		MirrorSelectionCommand(Diagram *diagram,
							   Qt::Orientation orientation,
							   QUndoCommand *parent = nullptr);
		MirrorSelectionCommand(const QList<Element *> &elements,
							   Qt::Orientation orientation,
							   QUndoCommand *parent = nullptr);

		bool isValid() const;

	private:
		static QList<Element *> selectedElements(const Diagram *diagram);
};

#endif // MIRRORSELECTIONCOMMAND_H
