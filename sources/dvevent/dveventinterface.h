/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef DVEVENTINTERFACE_H
#define DVEVENTINTERFACE_H

class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class DiagramView;
class Diagram;

/**
 * @brief The DVEventInterface class
 * This class is the main interface for manage event of a Diagram View.
 * This do nothing, for create new event behavior, we must to create new class from this.
 * Each method return a bool: True if the methode do something else return false.
 * Each method of DVEventInterface return false;
 * isRunning() return true if action is started but not finish. By default return false.
 * isFinish() return true when the action is finish, or not started. By default return true.
 */
class DVEventInterface
{
	public:
		DVEventInterface(DiagramView *dv);
		virtual ~DVEventInterface () = 0;
		virtual bool mouseDoubleClickEvent (QMouseEvent *event);
		virtual bool mousePressEvent       (QMouseEvent *event);
		virtual bool mouseMoveEvent        (QMouseEvent *event);
		virtual bool mouseReleaseEvent     (QMouseEvent *event);
		virtual bool wheelEvent            (QWheelEvent *event);
		virtual bool keyPressEvent         (QKeyEvent *event);
		virtual bool KeyReleaseEvent       (QKeyEvent *event);
		virtual bool isRunning () const;
		virtual bool isFinish  () const;

	protected:
		DiagramView *m_dv;
		Diagram *m_diagram;
		bool m_running;
		bool m_abort;
};

#endif // DVEVENTINTERFACE_H
