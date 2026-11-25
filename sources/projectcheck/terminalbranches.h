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
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef TERMINALBRANCHES_H
#define TERMINALBRANCHES_H

#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <QUuid>

/**
	@brief The TerminalBranches namespace
	Branches between two terminals of a terminal strip (#1878 B4). A wire
	from one strip terminal to another cannot be split on its way: a
	branch is made on a terminal, where a second wire can be clamped. So a
	junction on such a wire, with a third wire going off it, is drawn as
	nothing can be built.

	A run is the wires joined by junctions and by linked folio reports,
	which only pass a wire on; it stops at every other terminal, those of
	symbols and of strip terminals. A run is found when it ends on two or
	more different strip terminals and is split at a junction: one with
	three wires or more. Wires that meet on a terminal are not a branch
	between terminals: they are clamped there. Nor is a junction whose
	definition gives its chain (#1500), with each of its wires on a
	connection of the chain: the wiring list runs it from terminal to
	terminal, the way it is wired, so a run stops there as at a terminal.

	check() reads the conductor, element, element_info, terminal, link and
	symbol_def tables of @p db alone, so it builds no folio and gives the same answer
	for a .qetdb read by itself. It changes nothing. On an open project,
	call projectDataBase::terminalBranches(), which first writes the links,
	texts and positions the database still holds back.
*/
namespace TerminalBranches
{
	struct End
	{
		QUuid element;            ///< the strip terminal
		QString label;            ///< its label, else its strip and place on it, "X1:7"
		QString terminal;         ///< the name of its terminal the run reaches
		QUuid folio;
		int folioNumber = 0;      ///< from 1 (diagram.pos)
		QString position;         ///< its cell, as A3
	};

	struct Finding
	{
		QUuid junction;           ///< the junction the run is split at; of several, the one with the most wires, first in folio order
		QString label;            ///< its label
		QUuid folio;              ///< its folio
		int folioNumber = 0;      ///< from 1 (diagram.pos)
		QString folioTitle;
		QString position;         ///< its cell, as A3
		QList<End> terminals;     ///< the strip terminals the run reaches, one each, in folio order
		int others = 0;           ///< the other terminals it reaches
		QList<QUuid> wires;       ///< the wires of the run
		QList<QUuid> folios;      ///< the folios those wires are on
	};

		/// Every finding of the project in @p db, in folio order
	QList<Finding> check(const QSqlDatabase &db);
}

#endif // TERMINALBRANCHES_H
