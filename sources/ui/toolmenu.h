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
#ifndef TOOLMENU_H
#define TOOLMENU_H

#include <QMenu>
#include <QStyleOptionMenuItem>

/**
	@brief The ToolMenu class
	A menu of drawing tools. A tool's action is checkable so that its
	toolbar button stays down while the tool is in use, and the actions
	of one group are exclusive, so a plain QMenu draws a radio button in
	front of each of them. A menu closes on the click that picks a tool,
	so that radio button never shows anything: this menu leaves it out.
	The toolbar buttons are unchanged.
*/
class ToolMenu : public QMenu
{
	public:
		using QMenu::QMenu;

	protected:
		void initStyleOption(QStyleOptionMenuItem *option, const QAction *action) const override
		{
			QMenu::initStyleOption(option, action);
			option->checkType = QStyleOptionMenuItem::NotCheckable;
			option->checked = false;
		}
};

#endif // TOOLMENU_H
