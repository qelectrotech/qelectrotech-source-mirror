/*
	Copyright 2006-2019 The QElectroTech Team
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
 * isRunning() return true if action is running (do something). By default return false.
 *
 * ##USE DiagramEventInterface##
 * This class is the basic interface for manage event on a diagram.
 * To create a behavior for event diagram, we need to herite this class.
 * This interface work like this :
 * You need to create an interface and call diagram::setEventInterface(pointer_of_your_interface).
 * When a diagram get an event (mouse or key) if they have an event interface,
 * they send the event (with the status accepted to false) to the interface (for exemple mousePressEvent).
 * If the interface do something with this event, you need to set to true the accepted status of the event, then diagram do nothing.
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
		~DiagramEventInterface() override = 0;
		virtual void mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event);
		virtual void mousePressEvent       (QGraphicsSceneMouseEvent *event);
		virtual void mouseMoveEvent        (QGraphicsSceneMouseEvent *event);
		virtual void mouseReleaseEvent     (QGraphicsSceneMouseEvent *event);
		virtual void wheelEvent            (QGraphicsSceneWheelEvent *event);
		virtual void keyPressEvent         (QKeyEvent *event);
		virtual void keyReleaseEvent       (QKeyEvent *event);
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
