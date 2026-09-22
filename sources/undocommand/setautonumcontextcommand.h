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
#ifndef SETAUTONUMCONTEXTCOMMAND_H
#define SETAUTONUMCONTEXTCOMMAND_H

#include "../autoNum/numerotationcontext.h"

#include <QUndoCommand>
#include <functional>

/**
	@brief The SetAutoNumContextCommand class
	Undo/redo wrapper around one of QETProject's add*AutoNum() setters
	(conductor/element/folio numerotation counters). Placing an
	auto-numbered item advances one of these counters as a side effect;
	without this command the counter change sits outside the undo stack
	entirely, so undoing the placement removes the visible number but
	leaves the counter advanced, silently burning it.
*/
class SetAutoNumContextCommand : public QUndoCommand
{
	public:
		using Setter = std::function<void(const QString &, const NumerotationContext &)>;

		SetAutoNumContextCommand(
				Setter setter,
				const QString &key,
				const NumerotationContext &old_context,
				const NumerotationContext &new_context,
				QUndoCommand *parent = nullptr);

		void undo() override;
		void redo() override;

	private:
		Setter m_setter;
		QString m_key;
		NumerotationContext m_old_context;
		NumerotationContext m_new_context;
};

#endif // SETAUTONUMCONTEXTCOMMAND_H
