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
#ifndef DIAGRAMEVENTADDCABLE_H
#define DIAGRAMEVENTADDCABLE_H

#include "../cable/cablemanager.h"
#include "diagrameventinterface.h"

class QLineF;
class QGraphicsLineItem;

/**
	@brief The DiagramEventAddCable class draws the trunk line of a cable.

	It behaves like every other drawing tool of QElectroTech: the first
	click puts the start point down, the mouse then shows the line as it
	will be, and either a second click or the release of a drag lays it
	-- whichever the user does, he does not have to aim at anything.

	The line stays horizontal or vertical, whichever way the user pulls
	harder, and it keeps following him: the direction is worked out again
	on every mouse move (with a dead band so it does not flicker), never
	frozen by the first stray pixel of the gesture -- a cable trunk runs
	straight along a row of drawings, but along the row the user meant.
*/
class DiagramEventAddCable : public DiagramEventInterface
{
		Q_OBJECT

	public:
		explicit DiagramEventAddCable(Diagram *diagram);
		~DiagramEventAddCable() override;

		void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
		void keyPressEvent(QKeyEvent *event) override;
		void init() override;

	private:
		void reset();
		void updatePreview(const QPointF &scene_pos);
		QPointF finishPoint(const QPointF &scene_pos) const;
		void finishLine(const QPointF &end);
		void showGuides(const QPointF &scene_pos);
		void clearHint() const;
		void updateHint() const;

	private:
		QGraphicsLineItem *m_preview = nullptr;
			///The cross QElectroTech shows while any shape is drawn
		QGraphicsLineItem *m_help_horiz = nullptr;
		QGraphicsLineItem *m_help_verti = nullptr;
			///True once the first click has put the line's start down
		bool m_has_anchor = false;
		QPointF m_anchor;
			///Which way the line is locked:
			///-1 undecided, 0 horizontal, 1 vertical
		int m_axis = -1;
		QList<CableCrossing> m_crossings;
};

#endif // DIAGRAMEVENTADDCABLE_H
