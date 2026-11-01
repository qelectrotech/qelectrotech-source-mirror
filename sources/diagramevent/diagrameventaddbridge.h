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
#ifndef DIAGRAMEVENTADDBRIDGE_H
#define DIAGRAMEVENTADDBRIDGE_H

#include "../bridge/bridgemanager.h"
#include "diagrameventinterface.h"

class QGraphicsLineItem;
class QGraphicsSimpleTextItem;

/**
	@brief The DiagramEventAddBridge class draws a bridge over terminals.

	The line is the user's own: he draws it where he wants it, straight
	or not, and every terminal symbol it crosses becomes one of the
	terminals the bridge lands on -- counted as he pulls, so he sees
	what the bridge is going to hold before the window opens.

	The second click ends the line and the window of the bridge opens:
	a new bridge, or one already there to land on. The line then becomes
	a drawn bridge like any other line of the drawing, in the colour of
	its bridge.
*/
class DiagramEventAddBridge : public DiagramEventInterface
{
		Q_OBJECT

	public:
		explicit DiagramEventAddBridge(Diagram *diagram);
		~DiagramEventAddBridge() override;

		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
		void keyPressEvent(QKeyEvent *event) override;
		void init() override;

	private:
		void reset();
		void updatePreview(const QPointF &scene_pos);
		void finishLine(const QPointF &end);
		void showGuides(const QPointF &scene_pos);
		void clearHint() const;
		void updateHint() const;

	private:
		QGraphicsLineItem *m_preview = nullptr;
		QGraphicsSimpleTextItem *m_counter = nullptr;
			///The cross QElectroTech shows while any shape is drawn
		QGraphicsLineItem *m_help_horiz = nullptr;
		QGraphicsLineItem *m_help_verti = nullptr;
			///True once the first click has put the line's start down
		bool m_has_anchor = false;
		QPointF m_anchor;
		QList<BridgeCrossing> m_crossings;
};

#endif // DIAGRAMEVENTADDBRIDGE_H
