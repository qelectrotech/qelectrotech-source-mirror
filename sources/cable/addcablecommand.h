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
#ifndef ADDCABLECOMMAND_H
#define ADDCABLECOMMAND_H

#include "cable.h"

#include "../autoNum/numerotationcontext.h"

#include <QList>
#include <QPointer>
#include <QUndoCommand>

class CablePart;
class Diagram;
class QETProject;

/**
	@brief Puts a drawn cable line on the folio, or takes it back off.

	Both halves are one single step, because a cable line without its
	cable is of no use to anybody: undoing has to take the line away and
	leave nothing behind, and redoing has to put both back exactly as
	they were.
*/
class AddCableCommand : public QUndoCommand
{
	public:
		AddCableCommand(Cable *cable,
						CablePart *part,
						Diagram *diagram,
						bool own_cable,
						const QList<CableCoreTaken> &taken = QList<CableCoreTaken>());
		~AddCableCommand() override;

		void undo() override;
		void redo() override;

	private:
		QETProject *project() const;

	private:
		/**
			The cable the line belongs to.

			A pointer, not a raw one: RemoveCableCommand deletes the
			cable when it goes, and both of them are torn down together
			when the stack takes a new command. Whoever of the two runs
			first frees the cable, so the other one must not follow it
			into the freed memory -- QPointer goes null instead, and
			this command then simply has nothing left to clean up.
		*/
		QPointer<Cable> m_cable;
		///The line itself, owned by the folio once it is drawn
		QPointer<CablePart> m_part;
		QPointer<Diagram> m_diagram;
		///Which section of the cable this line is
		CablePartData m_part_data;
		///The cores the line was giving to the cable, put back by redo
		QList<CableCore> m_cores;
		///The cores this line took away from other cables: redo takes
		///them again, undo gives them back
		QList<CableCoreTaken> m_taken;
		///True when this command brought the cable into existence, so it
		///is the one which has to take it away again
		bool m_own_cable = false;
		/**
			When this cable took a number out of the project's numbering
			rule, the rule as it was before it did and as it was after:
			an empty key means the project numbers its cables by no rule
			and nothing here applies. Handing the number out is part of
			bringing the cable into being, so undo has to hand it back --
			otherwise undoing a cable and drawing it once more would
			leave a gap in the numbering for good.
		*/
		QString m_num_key;
		NumerotationContext m_num_before;
};

#endif // ADDCABLECOMMAND_H
