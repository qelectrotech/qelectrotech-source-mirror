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
#ifndef DVEVENTINTERFACE_H
#define DVEVENTINTERFACE_H

#include <QObject>

class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class DiagramView;
class Diagram;

/**
	@brief The DVEventInterface class
	This class is the main interface for manage event of a Diagram View.
	This does nothing, to create new event behavior,
	we must create new class from this.
	Each method returns a bool:
	True if the method does something, else return false.
	Each method of DVEventInterface returns false;
	isRunning() returns true if action is started but not finished.
	By default: return false.
	isFinish() returns true when the action is finished, or not started.
	By default: return true.
*/
class DVEventInterface : public QObject
{
	Q_OBJECT

	public:
		DVEventInterface(DiagramView *dv);
		~DVEventInterface () override = 0;
		virtual bool mouseDoubleClickEvent (QMouseEvent *event);
		virtual bool mousePressEvent       (QMouseEvent *event);
		virtual bool mouseMoveEvent        (QMouseEvent *event);
		virtual bool mouseReleaseEvent     (QMouseEvent *event);
		virtual bool wheelEvent            (QWheelEvent *event);
		virtual bool keyPressEvent         (QKeyEvent *event);
		virtual bool KeyReleaseEvent       (QKeyEvent *event);
		virtual bool isRunning () const;
		virtual bool isFinish  () const;

	signals:
		/**
			@brief finish
			emitted when the interface finishes its job
		 */
		void finish();

	protected:
		DiagramView *m_dv;
		Diagram *m_diagram;
		bool m_running;
		bool m_abort;
};

#endif // DVEVENTINTERFACE_H
