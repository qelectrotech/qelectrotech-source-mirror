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
#ifndef ROTATETEXTSCOMMAND_H
#define ROTATETEXTSCOMMAND_H

#include <QUndoCommand>
#include <QPointer>
#include <QHash>

class ConductorTextItem;
class Diagram;
class QParallelAnimationGroup;

/**
	@brief The RotateTextsCommand class
	Apply @p rotation to the currently selected texts and texts group of a
	diagram. Just instantiate this undo command and push it in a QUndoStack.

	This command does not ask the user for anything: obtaining the angle is the
	caller's job, via the askRotation() helper below. Keeping the dialog out of
	the constructor is what makes the command usable outside an interactive
	session — from a test harness, a regression sweep, or a script — and stops
	a headless caller from blocking forever on a modal nobody can answer.

	Typical interactive use:
	@code
	if (RotateTextsCommand::hasSelectedTexts(diagram)) {
		qreal rotation = 0;
		if (RotateTextsCommand::askRotation(rotation))
			diagram->undoStack().push(new RotateTextsCommand(diagram, rotation));
	}
	@endcode
*/
class RotateTextsCommand : public QUndoCommand
{
	public:
		RotateTextsCommand(Diagram *diagram, qreal rotation, QUndoCommand *parent=nullptr);

			/// @return true if @p diagram has at least one selected text or text group to rotate.
		static bool hasSelectedTexts(Diagram *diagram);
			/// Open the orientation dialog. @return true and set @p rotation if accepted, false if cancelled.
		static bool askRotation(qreal &rotation);

		void undo() override;
		void redo() override;

	private:
		void setupAnimation(QObject *target, const QByteArray &propertyName, const QVariant& start, const QVariant& end);

	private:
		QPointer<Diagram> m_diagram;
		QHash<ConductorTextItem *, bool> m_cond_texts;
		qreal m_rotation=0;
		QParallelAnimationGroup *m_anim_group = nullptr;
};

#endif // ROTATETEXTSCOMMAND_H
