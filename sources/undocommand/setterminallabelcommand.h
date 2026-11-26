// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef SETTERMINALLABELCOMMAND_H
#define SETTERMINALLABELCOMMAND_H

#include <properties/terminallabel.h>

#include <QUndoCommand>
#include <QUuid>
#include <QPointer>

#include <QList>
#include <optional>

class Element;

/**
	@brief The SetTerminalLabelCommand class
	Give the terminals of a placed symbol a name and a drawing of their
	own, or take back what the definition of the symbol says -- one step of
	undo for the whole row of terminals the properties changed
	(@see Element::setTerminalLabel(), Element::removeTerminalLabel()).
*/
class SetTerminalLabelCommand : public QUndoCommand
{
	public:
		/// One terminal's label; an empty value takes the definition's back
		struct Change
		{
			QUuid terminal;
			std::optional<TerminalLabel> old_label;
			std::optional<TerminalLabel> new_label;
		};

		SetTerminalLabelCommand(Element *element,
								const QList<Change> &changes,
								QUndoCommand *parent = nullptr);

		int id() const override;
		bool mergeWith(const QUndoCommand *other) override;
		void undo() override;
		void redo() override;

	private:
		void setLabel(const QUuid &terminal,
					  const std::optional<TerminalLabel> &label) const;

	private:
		QPointer<Element> m_element;
		QList<Change> m_changes;
};

#endif // SETTERMINALLABELCOMMAND_H
