/*
	Copyright 2006-2017 The QElectroTech Team
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

class ConductorTextItem;
class Diagram;
class QParallelAnimationGroup;

/**
 * @brief The RotateTextsCommand class
 * Open a dialog for edit the rotation of the current selected texts and texts group in diagram.
 * Just instantiate this undo command and push it in a QUndoStack.
 */
class RotateTextsCommand : public QUndoCommand
{
	public:
		RotateTextsCommand(Diagram *diagram, QUndoCommand *parent=nullptr);
		
		void undo() override;
		void redo() override;
		
	private:
		void openDialog();
		void setupAnimation(QObject *target, const QByteArray &propertyName, const QVariant start, const QVariant end);
		
	private:
		QPointer<Diagram> m_diagram;
		QHash<ConductorTextItem *, bool> m_cond_texts;
		qreal m_rotation=0;
		QParallelAnimationGroup *m_anim_group = nullptr;
};

#endif // ROTATETEXTSCOMMAND_H
