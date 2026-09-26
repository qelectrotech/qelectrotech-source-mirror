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
#include "setautonumcontextcommand.h"

#include <utility>

/**
	@brief SetAutoNumContextCommand::SetAutoNumContextCommand
	@param setter the QETProject setter to call on undo/redo
	(addConductorAutoNum/addElementAutoNum/addFolioAutoNum, bound to a project)
	@param key the numerotation context's name/key
	@param old_context the context's value before this placement
	@param new_context the context's value after this placement
	@param parent parent undo command
*/
SetAutoNumContextCommand::SetAutoNumContextCommand(
		Setter setter,
		const QString &key,
		const NumerotationContext &old_context,
		const NumerotationContext &new_context,
		QUndoCommand *parent) :
	QUndoCommand(parent),
	m_setter(std::move(setter)),
	m_key(key),
	m_old_context(old_context),
	m_new_context(new_context)
{}

void SetAutoNumContextCommand::redo()
{
	m_setter(m_key, m_new_context);
}

void SetAutoNumContextCommand::undo()
{
	m_setter(m_key, m_old_context);
}
