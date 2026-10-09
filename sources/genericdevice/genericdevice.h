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
// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef GENERICDEVICE_H
#define GENERICDEVICE_H

#include <QDomElement>
#include <QFont>
#include <QLineF>
#include <QList>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QUuid>

/**
	@brief The GenericDevice namespace
	A generic device is a plain box symbol with terminals on any side, made
	by the generic device wizard for a device used too rarely to draw a
	symbol by hand (a VSD, a power supply). Everything here is pure: a
	description of the device (Spec) goes in, an ordinary element
	definition comes out. No widget, no scene, no project.

	Coordinates are the element's own, with (0, 0) at the top-left corner
	of the box body. Every terminal, stub and the body size are multiples
	of the grid step, so a device placed on the grid puts every terminal on
	a grid point and wires to it run straight.
*/
namespace GenericDevice
{
	enum Side { Left = 0, Right, Top, Bottom };
	constexpr int SideCount = 4;

	constexpr int Grid = 10;           ///< grid step of the folio
	constexpr int Stub = 10;           ///< terminal point is this far outside the body
	constexpr int MinWidth = 40;
	constexpr int MinHeight = 30;
	constexpr int Inset = 3;           ///< names sit this far inside the body edge
	constexpr int MaxPatternCount = 200; ///< largest {n} or {a-b} a pattern may expand

	/**
		@brief The Slot struct
		One position along a side: a terminal, a gap the user asked for, or
		padding added to line groups up across opposite sides. Gaps and
		padding take up a pitch of space and hold no terminal; a gap may
		carry a divider mark, padding never does.
	*/
	struct Slot
	{
		enum Kind { Terminal, Gap, Padding };
		Kind kind = Terminal;
		QString name;
		QString function;
		QString type = QStringLiteral("Generic"); ///< Generic, Inner or Outer
		QUuid uuid;

		bool isTerminal() const { return kind == Terminal; }
		static Slot terminal(const QString &name) { Slot s; s.name = name; return s; }
		static Slot gap() { Slot s; s.kind = Gap; return s; }
		static Slot padding() { Slot s; s.kind = Padding; return s; }
		/// The text drawn beside the terminal: "name function", or the name.
		QString shownText() const;
	};
	using Slots = QList<Slot>;

	/**
		@brief The Spec struct
		Everything the wizard collects. sides[] are ordered top to bottom
		(left, right) and left to right (top, bottom).
	*/
	struct Spec
	{
		QString name;
		QString label;
		QString manufacturer;
		QString reference;
		QString description;
		bool show_reference = true;

		Slots sides[SideCount];
		int pitch = 20;          ///< 10 to 50, a multiple of the grid
		int extra_width = 0;     ///< grid squares added to the automatic size
		int extra_height = 0;
		bool mark_gaps = true;   ///< draw a divider mark on each gap
		bool line_up = true;     ///< pad groups so dividers meet across the box

		int terminalCount() const;
	};

	/**
		@brief The Fonts struct
		What the names and the label are drawn with. Measuring with the font
		the folio will draw with is what keeps names inside the box.
	*/
	struct Fonts
	{
		QFont names;
		QFont label;
	};

	/**
		@brief The Layout struct
		Where everything goes, computed from a Spec. Pure geometry.
	*/
	struct Layout
	{
		struct PlacedTerminal {
			Slot slot;
			Side side;
			QPointF point;   ///< the terminal point, outside the body
			QPointF edge;    ///< where its stub meets the body
		};
		struct Name {
			QPointF pos;     ///< baseline-left, as a static text part is anchored
			QString text;
			int rotation = 0;
		};

		int body_width = 0;
		int body_height = 0;
		int auto_width = 0;  ///< the size before the extra width/height
		int auto_height = 0;
		Slots sides[SideCount]; ///< the spec's list after line-up padding
		QList<PlacedTerminal> terminals;
		QList<Name> names;
		QList<QLineF> dividers;
		QRectF label_box;    ///< x, y, width and the height of one line
		QStringList warnings;
	};

	struct PatternResult
	{
		Slots list;
		QString error;  ///< empty when the pattern was read
		bool ok() const { return error.isEmpty(); }
	};

	struct PasteResult
	{
		Slots sides[SideCount];
		bool has_side[SideCount] = {false, false, false, false};
		int rows = 0;            ///< rows read
		QStringList bad_rows;    ///< rows that could not be read, as typed
	};

	QString sideName(Side side);

	PatternResult expandPattern(const QString &pattern);
	QString toPattern(const Slots &list, bool *expressible = nullptr);
	Slots applyPattern(const Slots &current, const Slots &expanded);
	Slots groupEvery(const Slots &list, int n);
	Slots resize(const Slots &list, int count, int group_every);
	void lineUpGroups(Slots &a, Slots &b);
	void numberPins(Spec &spec);
	bool hasNumberedPins(const Spec &spec);
	void nameUnnamed(Spec &spec);
	PasteResult parsePastedTable(const QString &text);

	Layout layout(const Spec &spec, const Fonts &fonts);
	QDomElement toDefinition(const Spec &spec,
				 const Fonts &fonts,
				 QDomDocument &document,
				 const QString &version = QString());
	bool fromDefinition(const QDomElement &definition,
			    const Fonts &fonts,
			    Spec *spec);
}

#endif // GENERICDEVICE_H
