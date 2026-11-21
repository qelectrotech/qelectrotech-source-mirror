/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef DIALOGPLACEMENT_H
#define DIALOGPLACEMENT_H

#include <QRect>

class QWidget;

/**
	Where a dialog opens: next to the button or the table cell that opens
	it, rather than where the window system puts a new window.
*/
namespace DialogPlacement
{
	void placeBelow(QWidget *dialog, const QWidget *anchor,
					const QRect &area = QRect());
}

#endif // DIALOGPLACEMENT_H
