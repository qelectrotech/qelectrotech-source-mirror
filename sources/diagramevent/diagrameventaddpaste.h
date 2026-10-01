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

			///Where the group starts, relative to what was copied.
		enum PastePlacement {
				///Move the group so its top left lands at \a start_pos
				///(normally the cursor): Ctrl+V, the copy appears under
				///the pointer and follows it.
			UnderCursor,
				///Leave the group at its original XML position and warp
				///the OS cursor to the group's origin so the baseline
				///matches what is on screen: Ctrl+Shift+V, "paste at the
				///origin point".
			AtOrigin
		};

		DiagramEventAddPaste(Diagram *diagram, const QPointF &start_pos,
				     PastePlacement placement = UnderCursor);
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
		///Where this paste starts: UnderCursor moves the group to
		///start_pos, AtOrigin leaves it at its original XML position.
	PastePlacement m_placement{UnderCursor};
		///Where the group's grid-snapped top left sits when the paste
		///started, in scene coordinates -- under the cursor for
		///UnderCursor, the copy's own origin for AtOrigin.
	QPointF m_group_origin;
		///Cursor position (scene coords) the delta-based movement in
		///moveTo() measures from. Equal to m_group_origin: either the
		///group was put there (UnderCursor) or the cursor was warped
		///there (AtOrigin).
	QPointF m_initial_cursor;
		///Whether m_initial_cursor holds a usable baseline. A flag rather
		///than testing m_initial_cursor.isNull(), which cannot tell "not
		///set yet" from a baseline that is legitimately scene (0,0).
	bool m_baseline_captured{false};
	QPointer<QStatusBar> m_status_bar;
	bool m_finished{false};
};

#endif // DIAGRAMEVENTADDPASTE_H
