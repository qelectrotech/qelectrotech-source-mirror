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
#ifndef CABLERENUMBER_H
#define CABLERENUMBER_H

#include <QList>
#include <QString>
#include <QStringList>

#include "../autoNum/numerotationcontext.h"

class Cable;
class QETProject;

/**
	@brief What numbering a whole project would do, worked out before a
	single cable is written to.

	Working it out first means the answer can be refused as a whole: a
	rule which hands out the same number twice, or a number some cable
	which is left alone already carries, stops the whole numbering
	rather than numbering half the project and then getting stuck.
*/
struct CableRenumberPlan
{
		///The cables to number, in the order they are numbered in
	QList<Cable *> cables;
		///The number each of them gets, one for each cable
	QStringList designations;
		///The rule as it stands once the last cable has been numbered:
		///its counters are where the next number would be taken from
	NumerotationContext after;
		///What stopped the numbering, empty while it may be carried out
	QString error;

	bool ok() const {return error.isEmpty();}
};

/**
	@brief Working out the numbers a whole project should carry.

	Everything here is pure: nothing is written to the project and
	nothing is put on the undo stack, so a caller may look at the plan
	and decide against it without anything having happened.
*/
namespace CableRenumber
{
	/**
		How many cables of this project carry a name which was typed in
		rather than handed out by the numbering rule. Those are the ones
		renumbering asks about before it takes their name away.
		@param project
		@return the number of such cables
	*/
	int byHandCount(QETProject *project);

	/**
		Work the number of every cable of the project out of @p rule.

		The cables are laid out first -- folio by folio, and on each
		folio either from the left to the right or from the top to the
		bottom, whichever axis leads -- and are then numbered one after
		the other from the beginning of the rule, so that a hole left by
		a cable which has been deleted closes behind it.

		@param project the project whose cables are numbered
		@param rule the numbering rule to read; its counters are put
		back to 1 first, since numbering starts from the beginning
		@param x_axis_first true to number the cables of a folio from
		the left to the right, false to number them from the top to the
		bottom
		@param include_by_hand false to leave the cables whose name was
		typed in exactly as they are
		@return the numbers, or what stopped the numbering. The plan
		also carries the rule as it stands once every cable has been
		given its number, so that the caller may put that into the
		project: the counters have to stay where the last number was
		taken from, or the next cable drawn would skip over every
		number in between
	*/
	CableRenumberPlan plan(QETProject *project,
							const NumerotationContext &rule,
							bool x_axis_first,
							bool include_by_hand);
}

#endif // CABLERENUMBER_H
