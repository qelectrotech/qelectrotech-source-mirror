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
#ifndef SHOWNKINDS_H
#define SHOWNKINDS_H

class QGraphicsItem;
class QGraphicsScene;

/**
	Show or hide whole kinds of folio items, View > Show (bugtracker #301):
	symbol texts, wire numbers, free texts and so on.

	Each item says which kind it is with a tag set by its constructor, so an
	item created while its kind is hidden starts hidden, whichever way it was
	created (load, paste, a text added to a symbol, a new cross-reference).
	A tag and not type(): the cross-reference under a contact's label is a
	plain QGraphicsTextItem.

	Some items also hide themselves (a wire number switched off, one text
	per potential, a cross-reference snapped elsewhere). They call
	setVisible() here instead of QGraphicsItem::setVisible(), and the kind
	state can only veto what they ask for: showing a kind again brings back
	only what was hidden for it, never what an item hid itself.

	The state is for the whole application and is not saved: it is a way of
	looking at the folios, like the grid. Hidden items are left out of
	printing and exports, and Qt does not select them, so nothing hidden is
	copied, moved or deleted by accident.

	This part knows nothing of Diagram, so it can be tested on a plain scene.
*/
namespace ShownKinds
{
	enum Kind {
		SymbolTexts,
		WireNumbers,
		FreeTexts,
		Shapes,
		Pictures,
		Tables,
		CrossReferences,
		KindCount
	};

		/// QGraphicsItem::data() key holding an item's kind.
	inline constexpr int data_key = 0x4b4e44; // "KND"
		/// QGraphicsItem::data() key set while an item that wants to be
		/// visible is hidden only because its kind is hidden.
	inline constexpr int hidden_key = 0x4b4e48; // "KNH"

	bool isShown(Kind kind);
	void setShown(Kind kind, bool shown);
	int hiddenCount();

	void tag(QGraphicsItem *item, Kind kind);
	void setVisible(QGraphicsItem *item, bool visible);
	bool isHidden(const QGraphicsItem *item);
	bool wantsVisible(const QGraphicsItem *item);
	void apply(QGraphicsScene *scene, Kind kind);
}

#endif // SHOWNKINDS_H
