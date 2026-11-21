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
#ifndef OPENWIREENDS_H
#define OPENWIREENDS_H

#include <QList>
#include <QSqlDatabase>
#include <QString>
#include <QUuid>

/**
	@brief The OpenWireEnds namespace
	Wires that lead nowhere. QElectroTech draws a wire between two
	terminals only, so a wire never has a loose end; what a wire can do is
	stop: run to a folio report that is linked to no other report, run
	into a junction on one side only, or carry a potential that, followed
	through its folio reports and junctions as potentialGroupsOf() follows
	it, reaches one symbol terminal or none.

	check() reads the conductor, element and link tables of @p db alone,
	so it builds no folio and gives the same answer for a .qetdb read by
	itself. It changes nothing. On an open project, call
	projectDataBase::openWireEnds(), which first writes the links, texts
	and positions the database still holds back.
*/
namespace OpenWireEnds
{
	enum class Kind {
		UnlinkedReport,   ///< a folio report with a wire, linked to no other report
		LoneJunction,     ///< a junction with one wire
		OneEnd            ///< a potential that reaches one symbol terminal, or none
	};

	struct Finding
	{
		Kind kind = Kind::OneEnd;
		QUuid element;            ///< the report or junction; for OneEnd, the one symbol, if any
		QString terminal;         ///< OneEnd: the name of that symbol's terminal
		QString label;            ///< the label of element
		QUuid folio;              ///< the folio of element, else of the first wire
		int folioNumber = 0;      ///< its number in the project, from 1 (diagram.pos)
		QString folioTitle;
		QString position;         ///< the cell of element on its folio, as A3
		QList<QUuid> wires;       ///< the wires of the potential
		QList<QUuid> folios;      ///< the folios those wires are on
	};

		/// Every finding of the project in @p db, in folio order. A report
		/// or junction found is not found again as the potential it is on.
	QList<Finding> check(const QSqlDatabase &db);
}

#endif // OPENWIREENDS_H
