/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef DIAGRAMEVENTINTERFACE_H
#define DIAGRAMEVENTINTERFACE_H

#include <QObject>

class QGraphicsSceneMouseEvent;
class QGraphicsSceneWheelEvent;
class QKeyEvent;
class Diagram;

/**
 * @brief The DiagramEventInterface class
 * Each method return a bool: True if the methode do something else return false.
 * Each method of DVEventInterface return false;
 * isRunning() return true if action is running (do something). By default return false.
 *
 * ##USE DiagramEventInterface##
 * This class is the basic interface for manage event on a diagram.
 * To create a behavior for event diagram, we need to herite this class.
 * This interface work like this :
 * You need to create an interface and call diagram::setEventInterface(*your_interface).
 * When a diagram get an event (mouse or key) if they have an event interface,
 * they send the event to the interface (for exemple mousePressEvent).
 * If the interface do something with this event, you need to return true to signal the diagram you work with this event.
 * (if you do nothing by defaut the interface return false, so diagram do nothing)
 * When the interface job is done, we need to emit the signal finish(), the diagram use this signal to delete the interface.
 * Be carreful with the destructor, diagram can at any time (even if interface is still running) delete the interface,
 * the bool m_abort is here for that at destruction time.
 *
 */
class DiagramEventInterface : public QObject
{
		Q_OBJECT

	public:
		DiagramEventInterface(Diagram *diagram);
		virtual ~DiagramEventInterface() = 0;
		virtual bool mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event);
		virtual bool mousePressEvent       (QGraphicsSceneMouseEvent *event);
		virtual bool mouseMoveEvent        (QGraphicsSceneMouseEvent *event);
		virtual bool mouseReleaseEvent     (QGraphicsSceneMouseEvent *event);
		virtual bool wheelEvent            (QGraphicsSceneWheelEvent *event);
		virtual bool keyPressEvent         (QKeyEvent *event);
		virtual bool KeyReleaseEvent       (QKeyEvent *event);
		virtual bool isRunning () const;
		virtual void init();

	signals:
		void finish(); //Emited when the interface finish is job.

	protected:
		Diagram *m_diagram;
		bool m_running;
		bool m_abort;
};

#endif // DIAGRAMEVENTINTERFACE_H
