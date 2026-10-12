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
#ifndef EDITCABLECOMMAND_H
#define EDITCABLECOMMAND_H

#include "cable.h"

#include "../autoNum/numerotationcontext.h"

#include <QHash>
#include <QList>
#include <QPointer>
#include <QUndoCommand>

class CablePart;
class Diagram;
class QETProject;

/**
	@brief Puts a dragged cable line back where it was, or where the user
	just dragged it to.

	The colour labels of its cores follow the line -- they are drawn on
	it -- and come back with it when the move is undone. Which core lands
	on which conductor never depends on where the trunk line stands, so
	the assignments are only carried along by this step, never changed.
*/
class MoveCablePartCommand : public QUndoCommand
{
	public:
		MoveCablePartCommand(Cable *cable,
							 CablePart *part,
							 const QPointF &before_p1,
							 const QPointF &before_p2,
							 const QPointF &after_p1,
							 const QPointF &after_p2,
							 const QList<CableCore> &before_cores,
							 const QList<CableCore> &after_cores);

		void undo() override;
		void redo() override;

	private:
		void apply(const QPointF &p1,
				   const QPointF &p2,
				   const QList<CableCore> &cores);

	private:
		QPointer<Cable> m_cable;
		QPointer<CablePart> m_part;
		QPointF m_before_p1;
		QPointF m_before_p2;
		QPointF m_after_p1;
		QPointF m_after_p2;
		QList<CableCore> m_before_cores;
		QList<CableCore> m_after_cores;
};

/**
	@brief Rewrites which cores a cable wires, the way dragging the slash
	of a core does: the whole set of assignments is taken before and
	after the gesture, so undoing puts every one of them back -- the core
	which was moved, but also the one it swapped colours with.
*/
class ChangeCableCoresCommand : public QUndoCommand
{
	public:
		/**
			@param cable the cable whose assignments are rewritten
			@param before every core it held before
			@param after every core it holds after
			@param text what undo says about this step: an empty string
			for the usual step of moving a core around, since putting a
			core down for the first time is not a move at all
		*/
		ChangeCableCoresCommand(Cable *cable,
								const QList<CableCore> &before,
								const QList<CableCore> &after,
								const QString &text = QString());

		void undo() override;
		void redo() override;

	private:
		void apply(const QList<CableCore> &cores);

	private:
		QPointer<Cable> m_cable;
		QList<CableCore> m_before;
		QList<CableCore> m_after;
};

/**
	@brief Writes what the user typed into the fields of a cable -- and
	what he ticks off again -- as one single undo step.

	All the fields belong to the cable rather than to one line of it, so
	one change puts every line of that cable, on every folio, right.
*/
class ChangeCablePropertiesCommand : public QUndoCommand
{
	public:
		ChangeCablePropertiesCommand(Cable *cable,
									 const CableProperties &before,
									 const CableProperties &after);

		void undo() override;
		void redo() override;

	private:
		QPointer<Cable> m_cable;
		CableProperties m_before;
		CableProperties m_after;
};

/**
	@brief Gives every cable of a project the number the numbering rule
	says it should have, laid out in the order the cables are met along
	the sheets.

	That is how a project is numbered again after a cable has been taken
	away: the hole its number leaves closes because the cables behind it
	move up one place, and a rule which has just been changed reaches
	every cable of the project at once.

	One command covers them all -- numbering a whole project is one
	single act, so undo takes all of it back together. What the
	numbering rule itself holds is part of that act: once the cables
	are numbered, the counters of the rule stand where the last number
	was taken from, and undo puts them back where they stood.
*/
class RenumberCablesCommand : public QUndoCommand
{
	public:
		/**
			@param cables the cables to number, in the order they are
			numbered in
			@param designations the number each of them gets, one for
			each cable
			@param project the project whose numbering rule moves along
			with the numbering
			@param num_key the name that rule is kept under in the
			project
			@param num_before where its counters stood before the
			numbering ran
			@param num_after where the numbering leaves them
		*/
		RenumberCablesCommand(const QList<Cable *> &cables,
							  const QStringList &designations,
							  QETProject *project,
							  const QString &num_key,
							  const NumerotationContext &num_before,
							  const NumerotationContext &num_after);

		void undo() override;
		void redo() override;

	private:
		void apply(bool use_after);

	private:
		QList<QPointer<Cable>> m_cables;
		QStringList m_before;
		QStringList m_after;
		QList<bool> m_by_hand_before;
		QList<bool> m_by_hand_after;
		QETProject *m_project = nullptr;
		QString m_num_key;
		NumerotationContext m_num_before;
		NumerotationContext m_num_after;
};

/**
	@brief Changes which type a cable is -- its name, how many cores it
	has and the colour of each of them -- together with the cores it
	may lose on the way, as one single step.

	The cores belong to this step rather than to a step of their own:
	a type with fewer cores takes marks off the drawing, and undoing has
	to bring them back along with the name and the colours, never half
	of it.
*/
class ChangeCableTypeCommand : public QUndoCommand
{
	public:
		/**
			@param cable the cable whose type is changed
			@param before the state the cable was in
			@param after the state it is put into
		*/
		ChangeCableTypeCommand(Cable *cable,
							   const CableTypeState &before,
							   const CableTypeState &after);

		void undo() override;
		void redo() override;

	private:
		void apply(const CableTypeState &state);

	private:
		QPointer<Cable> m_cable;
		CableTypeState m_before;
		CableTypeState m_after;
};

/**
	@brief Holds the steps of one single gesture together, so a single
	undo takes all of them back -- an ordinary deletion with the cable
	deletions which go with it, or a drag which moved several cable
	lines at once.

	It keeps its steps rather than handing them to QUndoCommand as
	children: Qt6.11 has no addChild() any more.
*/
class BatchCommand : public QUndoCommand
{
	public:
		BatchCommand(QUndoCommand *first, QUndoCommand *second);
		BatchCommand(const QList<QUndoCommand *> &steps, const QString &text);
		~BatchCommand() override;

		void undo() override;
		void redo() override;

	private:
		QList<QUndoCommand *> m_steps;
};

/**
	@brief Takes one or more drawn cable lines off their folios -- and the
	cable itself with them when no line of it is left anywhere.

	Undo puts everything back: the lines on their folios, the sections in
	the cable, the cores on those sections, and the cable in the project
	when it had to leave it.
*/
class RemoveCableCommand : public QUndoCommand
{
	public:
		RemoveCableCommand(Diagram *diagram, const QList<CablePart *> &parts);
		~RemoveCableCommand() override;

		void undo() override;
		void redo() override;

	private:
		QETProject *project() const;

	private:
		struct Entry
		{
				///The line as it is drawn on the folio
			QPointer<CablePart> part;
				///Where the cable holds that section
			CablePartData data;
				///The cable the section belongs to
			QPointer<Cable> cable;
		};

		QPointer<Diagram> m_diagram;
		QList<Entry> m_entries;
			///Every cable which loses a section here, with all the cores
			///it wires at this moment: removing a section drops the cores
			///drawn on it, and undo has to bring them back
		QList<QPointer<Cable>> m_cables;
		QHash<Cable *, QList<CableCore>> m_cores;
};

#endif // EDITCABLECOMMAND_H
