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
#ifndef ESEVENTINTERFACE_H
#define ESEVENTINTERFACE_H

class QGraphicsSceneMouseEvent;
class QGraphicsSceneWheelEvent;
class QKeyEvent;
class ElementScene;
class QETElementEditor;
class QGraphicsLineItem;
class QPointF;

class ESEventInterface
{
	public:
		ESEventInterface(ElementScene *scene);
		virtual ~ESEventInterface();

		void init();

		virtual bool mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event);
		virtual bool mousePressEvent       (QGraphicsSceneMouseEvent *event);
		virtual bool mouseMoveEvent        (QGraphicsSceneMouseEvent *event);
		virtual bool mouseReleaseEvent     (QGraphicsSceneMouseEvent *event);
		virtual bool wheelEvent            (QGraphicsSceneWheelEvent *event);
		virtual bool keyPressEvent         (QKeyEvent *event);
		virtual bool KeyReleaseEvent       (QKeyEvent *event);
		virtual bool isRunning () const;
		virtual bool isFinish  () const;

	protected:
		void updateHelpCross (const QPointF &p);

	protected:
		ElementScene     *m_scene;
		QETElementEditor *m_editor;
		QGraphicsLineItem *m_help_horiz, *m_help_verti;
		bool m_running, m_abort;
};

#endif // ESEVENTINTERFACE_H
