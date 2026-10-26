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
#ifndef TERMINALSTRIPAUTOIMPORT_H
#define TERMINALSTRIPAUTOIMPORT_H

#include <QString>

class QETProject;

/**
 * The TerminalStripAutoImport namespace works out the terminal strips of a
 * project from the labels of its free terminals, so that the user does not
 * have to create the strips and move the terminals one by one.
 *
 * The rule : the name of a strip is the label of its terminals up to the
 * first colon, "X1:2" giving the strip "X1", as numbered by the terminal
 * numbering dialog. Terminals whose label holds no colon use the whole
 * label, terminals with no label are left alone.
 *
 * The plant and the location work as follows : they are read from the
 * terminal and only decide which strip to pick when they are set. An
 * existing strip with the same name is reused as soon as they are empty
 * (in most projects the plant and the location live on the strip, not on
 * the terminals), while a set plant or location must match exactly, a
 * strip being created when no one matches. That way the import never
 * builds a second strip for a name that already exists.
 *
 * Two actions run this import :
 * - the default one places free terminals only, it never moves or
 *   re-sorts a terminal the user already assigned to a strip;
 * - "re-group all terminals" also moves a placed terminal whose label
 *   prefix or plant and location no longer match its strip, for the case
 *   where the user corrects a terminal afterwards. Existing strips are
 *   never re-sorted.
 *
 * The import is idempotent : a second run adds nothing, the terminals
 * of a strip the user ordered or completed by hand are never re-ordered,
 * only new terminals are added (at the end of the strip). Only a strip
 * created by the import itself is sorted. When nothing has to change,
 * nothing is pushed to the undo stack : the project stays untouched.
 */
namespace TerminalStripAutoImport
{
	struct ImportResult
	{
		int terminals = 0;	///< Terminals added to a strip, moved ones included
		int strips = 0;		///< Terminal strips created
	};

	/**
	 * @brief stripNameForLabel
	 * @param label : the label of a terminal, for example "X1:2"
	 * @return the name of the strip holding this terminal, "X1", or an
	 * empty string when the label holds nothing usable.
	 */
	inline QString stripNameForLabel(const QString &label)
	{
		const auto label_ = label.trimmed();
		if (label_.isEmpty()) {
			return QString();
		}
		const auto colon_ = label_.indexOf(QLatin1Char(':'));
		if (colon_ > 0) {
			return label_.left(colon_).trimmed();
		}
		if (colon_ == 0) {
			return QString();	//Label starts with ":" : no name before it
		}
		return label_;	//No colon : the whole label names the strip
	}

	/**
	 * @brief importFreeTerminals
	 * Add every free terminal of @a project to the terminal strip named
	 * after its label prefix, creating the strip when it does not exist
	 * yet. Everything happens on the undo stack, as one undoable step.
	 * @param move_placed : false (default, the "assign free terminals"
	 * action) only places free terminals, placed ones are never touched.
	 * True (the "re-group all terminals" action) also moves a placed
	 * terminal whose label prefix or plant and location no longer match
	 * its strip. Existing strips are never re-sorted.
	 * @return What has been done. Empty : nothing was pushed, the
	 * project is untouched.
	 */
	ImportResult importFreeTerminals(QETProject *project, bool move_placed = false);
}

#endif // TERMINALSTRIPAUTOIMPORT_H
