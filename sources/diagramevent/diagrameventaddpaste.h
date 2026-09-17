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
#ifndef DIAGRAMEVENTADDPASTE_H
#define DIAGRAMEVENTADDPASTE_H

#include "diagrameventinterface.h"
#include "../diagramcontent.h"

#include <QHash>
#include <QPointer>

class QStatusBar;

/**
	@brief The DiagramEventAddPaste class
	Paste the clipboard as a placement you can still move.

	The pasted items are added straight away and follow the cursor until a
	left click drops them; Escape or a right click takes them away again.
	This is the same interaction as placing a new element, so a paste behaves
	like every other "put something on the folio" action.

	The items are the real ones from the start, not a preview: Diagram::fromXml
	creates them, this class moves them, and PasteDiagramCommand is pushed only
	once they are dropped. That keeps one copy of the paste logic rather than
	two, and means a cancelled paste leaves nothing on the undo stack.
*/
class DiagramEventAddPaste : public DiagramEventInterface
{
		Q_OBJECT

	public:
			///Items with a position of their own. Conductors are left out
			///deliberately: they are drawn from their terminals, so they
			///follow when the elements they attach to move.
		static const int MovableItems =
				DiagramContent::Elements
				| DiagramContent::TextFields
				| DiagramContent::Images
				| DiagramContent::Shapes
				| DiagramContent::Tables
				| DiagramContent::TerminalStrip;

		DiagramEventAddPaste(Diagram *diagram, const QPointF &start_pos);
		~DiagramEventAddPaste() override;

		void mouseMoveEvent    (QGraphicsSceneMouseEvent *event) override;
		void mousePressEvent   (QGraphicsSceneMouseEvent *event) override;
		void mouseReleaseEvent (QGraphicsSceneMouseEvent *event) override;
		void keyPressEvent     (QKeyEvent *event) override;
		void init() override;

			///@return true if the clipboard holds something this can paste.
		static bool clipboardHasDiagram();

	private:
		void moveTo(const QPointF &scene_pos);
		void commit();
		void cancel();
		void showHint();
		void removeItems();

		DiagramContent m_content;
			///Each movable item's position relative to the group's top left,
			///taken once so repeated moves cannot accumulate rounding drift.
		QHash<QGraphicsItem *, QPointF> m_relative_pos;
		QPointer<QStatusBar> m_status_bar;
		bool m_finished{false};
};

#endif // DIAGRAMEVENTADDPASTE_H
