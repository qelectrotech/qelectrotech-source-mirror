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
#ifndef DOCKTEXTWRAP_H
#define DOCKTEXTWRAP_H

class QWidget;

/**
	The texts of the docked panels wrap, so that no text holds a panel,
	and with it every panel docked beside it, at the width of its longest
	line: a dock can be made as narrow as its fields allow.

	Every widget inside a QDockWidget is handled once, when it is polished,
	before it is first shown, so the editors a panel builds later (the
	Selection properties of what is selected) are handled too:
	- a label wraps its text;
	- a check box or a radio button, which never wraps its own text, gets
	  its text in a wrapping label beside it; a click on the text still
	  toggles it, as on its own text;
	- a combo box no longer widens to its longest entry.
*/
namespace DockTextWrap
{
		/// Handle the docks of the whole application from now on; once is
		/// enough, more calls do nothing
	void install();

		/// Handle @a widget as a widget inside a dock, now
	void wrap(QWidget *widget);
}

#endif // DOCKTEXTWRAP_H
