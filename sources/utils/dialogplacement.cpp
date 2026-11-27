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
#include "dialogplacement.h"

#include <QDialog>
#include <QGuiApplication>
#include <QScreen>

namespace {
	/**
		@return the title bar and borders around @p window, or no margins
		while the platform does not know them
	*/
	QMargins frameOf(const QWidget *window)
	{
		const QRect frame = window->frameGeometry();
		const QRect inside = window->geometry();
		return QMargins(inside.left() - frame.left(),
						inside.top() - frame.top(),
						frame.right() - inside.right(),
						frame.bottom() - inside.bottom());
	}
}

/**
	@brief DialogPlacement::placeBelow
	Put @p dialog just below @p area, or just above it when the screen
	has no room below, and inside the screen under @p area. In a
	right-to-left layout their right edges line up. A dialog already
	shown stays where it is: the user may have moved it.
	@param dialog : the dialog, not shown yet
	@param anchor : the widget that opens the dialog
	@param area : the part of @p anchor the dialog belongs to, in the
	coordinates of @p anchor; all of @p anchor when empty
*/
void DialogPlacement::placeBelow(QWidget *dialog, const QWidget *anchor,
								 const QRect &area)
{
	if (!dialog || !anchor || dialog->isVisible())
		return;

		//A window has its frame as soon as it exists on some platforms,
		//as on macOS. On others, as on X11, the window manager adds the
		//frame once the window is shown: it is then guessed from the
		//window of the anchor.
	QDialog probe;
	probe.winId();
	const QMargins probe_frame = frameOf(&probe);
	const bool frame_known = !probe_frame.isNull();
	const QMargins frame = frame_known ? probe_frame
									   : frameOf(anchor->window());
	const QSize size = dialog->size().grownBy(frame);

	const QRect local = area.isEmpty() ? anchor->rect() : area;
	const QRect global(anchor->mapToGlobal(local.topLeft()), local.size());
	QPoint pos(anchor->isRightToLeft()
				   ? global.left() + global.width() - size.width()
				   : global.left(),
			   global.top() + global.height());

	const QScreen *screen = QGuiApplication::screenAt(global.center());
	if (!screen)
		screen = anchor->screen();
	if (screen) {
		const QRect room = screen->availableGeometry();
			//The highest x and y that keep the whole dialog on the screen
		const int max_x = room.left() + room.width() - size.width();
		const int max_y = room.top() + room.height() - size.height();
		if (pos.y() > max_y)
			pos.setY(global.top() - size.height());
		pos.setX(qMax(room.left(), qMin(pos.x(), max_x)));
		pos.setY(qMax(room.top(), qMin(pos.y(), max_y)));
	}

		//Where the frame is known, the inside of the window is placed.
		//Elsewhere move(), before the window exists, asks the window
		//manager to put the frame at pos. On macOS move() opens the
		//window lower, by its title bar or twice that; on X11, moving
		//a window that exists puts its title bar over the anchor.
	if (frame_known)
		dialog->setGeometry(QRect(pos + QPoint(frame.left(), frame.top()),
								  dialog->size()));
	else
		dialog->move(pos);
}
