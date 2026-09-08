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
#ifndef CONTACTUSAGE_H
#define CONTACTUSAGE_H

#include <algorithm>

/**
	@brief The ContactUsage struct
	How many slave contacts a master element currently uses, broken down
	by contact type.

	Header-only and free of any graphics dependency so that the counting
	rules can be unit tested on their own. MasterElement::contactUsage()
	is the thin wrapper that feeds it the linked elements.

	Two rules are easy to get wrong, and both live here so that every
	caller gets them right:
	 - a slave stands for as many contacts as its "number" kind
	   information says, so a 4 pole contact counts as 4, not as 1
	 - a changeover contact is counted once, as sw. CrossRefItem's
	   NOElements() and NCElements() both return it, so adding those two
	   lists together would count it twice.
*/
struct ContactUsage
{
	/**
		Contact types a slave can declare. Mirrors
		ElementData::SlaveState, which is not used directly so that this
		header stays free of the element data dependencies and can be
		unit tested on its own. MasterElement::contactUsage() maps
		between the two.
	*/
	enum Type
	{
		NO,		///< Normally open
		NC,		///< Normally closed
		SW,		///< Changeover
		Other	///< Neither of the above
	};

	int no    = 0;	///< Normally open
	int nc    = 0;	///< Normally closed
	int sw    = 0;	///< Changeover
	int other = 0;	///< Neither of the above

	int total() const { return no + nc + sw + other; }

	/**
		Add one slave element to the tally.
		@param type     the contact type the slave declares
		@param contacts how many contacts it stands for. Values below 1
		                are treated as 1: an element which declares no
		                contact count is still one contact.
	*/
	void addSlave(Type type, int contacts)
	{
		const int n = std::max(1, contacts);

		switch (type)
		{
			case NO:    no    += n; break;
			case NC:    nc    += n; break;
			case SW:    sw    += n; break;
			case Other: other += n; break;
		}
	}
};

#endif // CONTACTUSAGE_H
