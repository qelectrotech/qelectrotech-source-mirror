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
#include "genericdevice.h"

#include "../editor/terminalnamecheck.h"

#include <QCoreApplication>
#include <QFontMetricsF>
#include <QRegularExpression>
#include <QtMath>

#include <algorithm>

namespace GenericDevice {

namespace {

int ceilGrid(qreal v)
{
	return int(qCeil(v / Grid - 1e-9)) * Grid;
}

int textWidth(const QFont &font, const QString &text)
{
		//+2: QTextDocument, which draws static texts, lays out a hair
		//wider than the font metrics on some fonts
	return int(qCeil(QFontMetricsF(font).horizontalAdvance(text))) + 2;
}

QString number(qreal v)
{
	return QString::number(qRound(v * 100.0) / 100.0);
}

const char *orientationOf(Side side)
{
	switch (side) {
		case Left:   return "w";
		case Right:  return "e";
		case Top:    return "n";
		case Bottom: return "s";
	}
	return "w";
}

int terminalsIn(const Slots &list)
{
	return int(std::count_if(list.cbegin(), list.cend(),
				 [](const Slot &s) { return s.isTerminal(); }));
}

QString normalisedType(const QString &type, bool *ok)
{
	for (const QString &known : {QStringLiteral("Generic"),
				     QStringLiteral("Inner"),
				     QStringLiteral("Outer")}) {
		if (type.compare(known, Qt::CaseInsensitive) == 0) {
			*ok = true;
			return known;
		}
	}
	*ok = type.isEmpty();
	return QStringLiteral("Generic");
}

	/// The list of all four sides, in pin number order: counter-clockwise
	/// from the top of the left side, as on an IC package.
QList<Slot *> pinOrder(Spec &spec)
{
	QList<Slot *> order;
	for (Slot &s : spec.sides[Left]) order << &s;
	for (Slot &s : spec.sides[Bottom]) order << &s;
	for (int i = spec.sides[Right].size() - 1; i >= 0; --i)
		order << &spec.sides[Right][i];
	for (int i = spec.sides[Top].size() - 1; i >= 0; --i)
		order << &spec.sides[Top][i];
	return order;
}

} // namespace

QString Slot::shownText() const
{
	return function.isEmpty() ? name : name + QLatin1Char(' ') + function;
}

int Spec::terminalCount() const
{
	int count = 0;
	for (const Slots &side : sides)
		count += terminalsIn(side);
	return count;
}

QString sideName(Side side)
{
	switch (side) {
		case Left:   return QCoreApplication::translate("GenericDevice", "Left");
		case Right:  return QCoreApplication::translate("GenericDevice", "Right");
		case Top:    return QCoreApplication::translate("GenericDevice", "Top");
		case Bottom: return QCoreApplication::translate("GenericDevice", "Bottom");
	}
	return QString();
}

/**
	@brief expandPattern
	Read one side written as a line: entries separated by commas, an empty
	entry is a gap, and one {…} per entry expands to numbered names:
	{n} is 1…n, {zn} is 0…n-1, {a-b} is a…b. The number may sit anywhere in
	the name: "X1:{4}" gives X1:1 … X1:4.
	@return the list, or an error saying which entry could not be read
*/
PatternResult expandPattern(const QString &pattern)
{
	PatternResult result;
	if (pattern.trimmed().isEmpty())
		return result;

	static const QRegularExpression braces(
		QStringLiteral("^([^{}]*)\\{(z?)(\\d+)(?:-(\\d+))?\\}([^{}]*)$"));

	const QStringList entries = pattern.split(QLatin1Char(','));
	for (const QString &raw : entries)
	{
		const QString entry = raw.trimmed();
		if (entry.isEmpty()) {
			result.list << Slot::gap();
			continue;
		}
		if (!entry.contains(QLatin1Char('{')) && !entry.contains(QLatin1Char('}'))) {
			result.list << Slot::terminal(entry);
			continue;
		}

		const QRegularExpressionMatch m = braces.match(entry);
		if (!m.hasMatch()) {
			result.error = QCoreApplication::translate("GenericDevice", "Cannot read \"%1\": use one {n}, {zn} or {a-b} per name.").arg(entry);
			result.list.clear();
			return result;
		}
		const bool from_zero = !m.captured(2).isEmpty();
		const bool range = !m.captured(4).isEmpty();
		int first = 1, last = 0;
		if (range) {
			if (from_zero) {
				result.error = QCoreApplication::translate("GenericDevice", "Cannot read \"%1\": z does not go with a range.").arg(entry);
				result.list.clear();
				return result;
			}
			first = m.captured(3).toInt();
			last = m.captured(4).toInt();
		} else {
			const int count = m.captured(3).toInt();
			first = from_zero ? 0 : 1;
			last = first + count - 1;
		}
		const int count = last - first + 1;
		if (count < 1) {
			result.error = QCoreApplication::translate("GenericDevice", "\"%1\" gives no terminal.").arg(entry);
			result.list.clear();
			return result;
		}
		if (count > MaxPatternCount) {
			result.error = QCoreApplication::translate("GenericDevice", "\"%1\" gives %2 terminals; at most %3 at once.")
					   .arg(entry).arg(count).arg(MaxPatternCount);
			result.list.clear();
			return result;
		}
		for (int n = first ; n <= last ; ++n)
			result.list << Slot::terminal(m.captured(1) + QString::number(n) + m.captured(5));
	}
	return result;
}

/**
	@brief toPattern
	Write @a list as a pattern line, the reverse of expandPattern(). Runs
	of three or more numbered names are folded: In1 In2 In3 is "In{3}".
	@param expressible : set to false when a name holds a comma, a brace
	or surrounding spaces, which a pattern cannot say
*/
QString toPattern(const Slots &list, bool *expressible)
{
	bool ok = true;
	static const QRegularExpression numbered(QStringLiteral("^(.*?)(\\d+)$"));

	QStringList entries;
	int i = 0;
	while (i < list.size())
	{
		const Slot &slot = list.at(i);
		if (!slot.isTerminal()) {
			entries << QString();
			++i;
			continue;
		}
		const QString &name = slot.name;
		if (name.isEmpty() || name != name.trimmed()
		    || name.contains(QLatin1Char(',')) || name.contains(QLatin1Char('{'))
		    || name.contains(QLatin1Char('}'))) {
			ok = false;
		}

			//Fold a run prefix+n, prefix+n+1, … written without leading zeros
		const QRegularExpressionMatch m = numbered.match(name);
		int run = 1;
		if (m.hasMatch() && QString::number(m.captured(2).toInt()) == m.captured(2))
		{
			const QString prefix = m.captured(1);
			const int start = m.captured(2).toInt();
			while (i + run < list.size() && list.at(i + run).isTerminal()
			       && list.at(i + run).name == prefix + QString::number(start + run)) {
				++run;
			}
			if (run >= 3) {
				const int end = start + run - 1;
				if (start == 1)
					entries << prefix + QStringLiteral("{%1}").arg(run);
				else if (start == 0)
					entries << prefix + QStringLiteral("{z%1}").arg(run);
				else
					entries << prefix + QStringLiteral("{%1-%2}").arg(start).arg(end);
				i += run;
				continue;
			}
		}
		entries << name;
		++i;
	}

	if (expressible)
		*expressible = ok;
	return entries.join(QStringLiteral(","));
}

/**
	@brief applyPattern
	The list of a side after its pattern line was edited: the names and
	gaps come from @a expanded, and the n-th terminal keeps the function,
	type and uuid of the n-th terminal of @a current, so renaming through
	the pattern loses nothing typed in the table.
*/
Slots applyPattern(const Slots &current, const Slots &expanded)
{
	Slots old_terminals;
	for (const Slot &s : current)
		if (s.isTerminal()) old_terminals << s;

	Slots result;
	int n = 0;
	for (Slot s : expanded)
	{
		if (s.isTerminal() && n < old_terminals.size()) {
			const Slot &old = old_terminals.at(n);
			s.function = old.function;
			s.type = old.type;
			s.uuid = old.uuid;
		}
		if (s.isTerminal()) ++n;
		result << s;
	}
	return result;
}

/**
	@brief groupEvery
	The terminals of @a list with a gap after every @a n of them, never
	after the last. Gaps already there are replaced. @a n of 0 means no
	groups: the terminals alone.
*/
Slots groupEvery(const Slots &list, int n)
{
	Slots terminals;
	for (const Slot &s : list)
		if (s.isTerminal()) terminals << s;
	if (n <= 0)
		return terminals;

	Slots result;
	for (int i = 0 ; i < terminals.size() ; ++i)
	{
		if (i > 0 && i % n == 0)
			result << Slot::gap();
		result << terminals.at(i);
	}
	return result;
}

/**
	@brief resize
	@a list with @a count terminals: new ones, unnamed, are added at the
	end and extra ones removed from the end, so every name already typed is
	kept. With @a group_every above 0 the gaps are laid again.
*/
Slots resize(const Slots &list, int count, int group_every)
{
	Slots result = list;
	count = std::max(0, count);
	int have = terminalsIn(result);
	while (have > count)
	{
		const Slot last = result.takeLast();
		if (last.isTerminal()) --have;
	}
		//Do not leave the side ending on a gap
	while (!result.isEmpty() && !result.last().isTerminal())
		result.removeLast();
	while (have < count) {
		result << Slot::terminal(QString());
		++have;
	}
	if (group_every > 0)
		result = groupEvery(result, group_every);
	return result;
}

/**
	@brief lineUpGroups
	Pad the shorter of each pair of groups facing each other, so that the
	k-th gap of @a a and the k-th gap of @a b fall on the same row. Only
	groups followed by a gap on both sides are padded; padding goes just
	before that gap.
*/
void lineUpGroups(Slots &a, Slots &b)
{
	auto split = [](const Slots &list) {
		QList<Slots> groups{Slots()};
		for (const Slot &s : list) {
			if (s.kind == Slot::Gap)
				groups << Slots();
			else
				groups.last() << s;
		}
		return groups;
	};

	QList<Slots> ga = split(a), gb = split(b);
	const int gaps = std::min(ga.size(), gb.size()) - 1;
	for (int k = 0 ; k < gaps ; ++k)
	{
		while (ga[k].size() < gb[k].size()) ga[k] << Slot::padding();
		while (gb[k].size() < ga[k].size()) gb[k] << Slot::padding();
	}

	auto join = [](const QList<Slots> &groups) {
		Slots list;
		for (int k = 0 ; k < groups.size() ; ++k) {
			if (k) list << Slot::gap();
			list << groups.at(k);
		}
		return list;
	};
	a = join(ga);
	b = join(gb);
}

/**
	@brief numberPins
	Name every terminal by its pin number, counter-clockwise from the top
	of the left side as on an IC package. Gaps take no number.
*/
void numberPins(Spec &spec)
{
	int n = 1;
	for (Slot *s : pinOrder(spec))
		if (s->isTerminal()) s->name = QString::number(n++);
}

/**
	@return true if every terminal of @a spec is named by numberPins(), so
	the names were never typed and may be renumbered.
*/
bool hasNumberedPins(const Spec &spec)
{
	Spec numbered = spec;
	numberPins(numbered);
	for (int side = 0 ; side < SideCount ; ++side)
		for (int i = 0 ; i < spec.sides[side].size() ; ++i)
			if (spec.sides[side][i].name != numbered.sides[side][i].name)
				return false;
	return true;
}

/**
	@brief nameUnnamed
	Give every terminal without a name the pin number of its position, and
	leave the named ones alone.
*/
void nameUnnamed(Spec &spec)
{
	Spec numbered = spec;
	numberPins(numbered);
	for (int side = 0 ; side < SideCount ; ++side)
		for (int i = 0 ; i < spec.sides[side].size() ; ++i)
			if (spec.sides[side][i].isTerminal() && spec.sides[side][i].name.isEmpty())
				spec.sides[side][i].name = numbered.sides[side][i].name;
}

/**
	@brief parsePastedTable
	Read rows of "side, name, function, type" pasted from a spreadsheet
	(tabs) or a CSV (commas). Side is left/right/top/bottom or L/R/T/B; an
	empty name is a gap; function and type are optional. A first row whose
	side reads "side" is a header and skipped. Rows that cannot be read
	are listed, as typed.
*/
PasteResult parsePastedTable(const QString &text)
{
	PasteResult result;
	const QStringList lines = text.split(QRegularExpression(QStringLiteral("\\r\\n|\\n|\\r")));
	bool first = true;
	for (const QString &line : lines)
	{
		if (line.trimmed().isEmpty())
			continue;
		const QChar separator = line.contains(QLatin1Char('\t')) ? QLatin1Char('\t')
									   : QLatin1Char(',');
		QStringList fields = line.split(separator);
		for (QString &f : fields) f = f.trimmed();

		const QString side_text = fields.value(0).toLower();
		if (first && side_text == QLatin1String("side")) {
			first = false;
			continue;
		}
		first = false;

		int side = -1;
		if (side_text == QLatin1String("left") || side_text == QLatin1String("l")) side = Left;
		else if (side_text == QLatin1String("right") || side_text == QLatin1String("r")) side = Right;
		else if (side_text == QLatin1String("top") || side_text == QLatin1String("t")) side = Top;
		else if (side_text == QLatin1String("bottom") || side_text == QLatin1String("b")) side = Bottom;

		bool type_ok = false;
		const QString type = normalisedType(fields.value(3), &type_ok);
		if (side < 0 || fields.size() > 4 || !type_ok) {
			result.bad_rows << line.trimmed();
			continue;
		}

		const QString name = fields.value(1);
		Slot slot = name.isEmpty() ? Slot::gap() : Slot::terminal(name);
		if (slot.isTerminal()) {
			slot.function = fields.value(2);
			slot.type = type;
		}
		result.sides[side] << slot;
		result.has_side[side] = true;
		++result.rows;
	}
	return result;
}

/**
	@brief layout
	Where everything of @a spec goes. The body is as small as the terminals
	and their names allow, then grown by the extra width and height.
	Opposite sides are packed from the top (left), never spread, so their
	rows line up and every terminal stays on the grid.
*/
Layout layout(const Spec &spec, const Fonts &fonts)
{
	Layout l;
	for (int s = 0 ; s < SideCount ; ++s)
		l.sides[s] = spec.sides[s];
	if (spec.line_up) {
		lineUpGroups(l.sides[Left], l.sides[Right]);
		lineUpGroups(l.sides[Top], l.sides[Bottom]);
	}

	const int pitch = std::clamp(spec.pitch / Grid * Grid, 10, 50);
	auto longest = [&](Side side) {
		int w = 0;
		for (const Slot &s : l.sides[side])
			if (s.isTerminal()) w = std::max(w, textWidth(fonts.names, s.shownText()));
		return w;
	};
	auto has = [&](Side side) { return terminalsIn(l.sides[side]) > 0; };

	const int rows = int(std::max(l.sides[Left].size(), l.sides[Right].size()));
	const int cols = int(std::max(l.sides[Top].size(), l.sides[Bottom].size()));
	const int label_w = textWidth(fonts.label, QStringLiteral("-XX000"));
	const int label_h = int(qCeil(QFontMetricsF(fonts.label).height()));

	int width_for_names = 0;
	if (has(Left) || has(Right))
		width_for_names = longest(Left) + longest(Right) + label_w + 4 * Inset + Grid;
	int height_for_names = 0;
	if (has(Top) || has(Bottom))
		height_for_names = longest(Top) + longest(Bottom) + 2 * label_h + 4 * Inset;

		//Where two sides meet, the first and last terminal of one side keep
		//clear of the other side's names, or "+24V" at the end of the
		//bottom row runs into "PE" at the foot of the right column.
	auto clearance = [&](Side side, int along) {
		if (has(side) && along)
			return std::max(pitch, ceilGrid(longest(side) + 2 * Inset + 2));
		return pitch;
	};
	const int x_first = clearance(Left, cols), x_last = clearance(Right, cols);
	const int y_first = clearance(Top, rows), y_last = clearance(Bottom, rows);

	l.auto_width = ceilGrid(std::max({MinWidth,
					  cols ? x_first + (cols - 1) * pitch + x_last : 0,
					  width_for_names,
					  label_w + 2 * Inset}));
	l.auto_height = ceilGrid(std::max({MinHeight,
					   rows ? y_first + (rows - 1) * pitch + y_last : 0,
					   height_for_names,
					   2 * label_h}));
	l.body_width = l.auto_width + std::max(0, spec.extra_width) * Grid;
	l.body_height = l.auto_height + std::max(0, spec.extra_height) * Grid;
	const int w = l.body_width, h = l.body_height;

		//A name is centred on its terminal's line: half the visible height
		//of the font below the line is where its baseline goes
	const QFontMetricsF names_metrics(fonts.names);
	const qreal centre = (names_metrics.ascent() - names_metrics.descent()) / 2.0;

	for (int s = 0 ; s < SideCount ; ++s)
	{
		const Side side = Side(s);
		const bool vertical = side == Left || side == Right;
		for (int i = 0 ; i < l.sides[s].size() ; ++i)
		{
			const Slot &slot = l.sides[s].at(i);
			const qreal along = (vertical ? y_first : x_first) + pitch * i;
			if (slot.kind == Slot::Gap && spec.mark_gaps)
			{
				switch (side) {
					case Left:   l.dividers << QLineF(0, along, Grid, along); break;
					case Right:  l.dividers << QLineF(w, along, w - Grid, along); break;
					case Top:    l.dividers << QLineF(along, 0, along, Grid); break;
					case Bottom: l.dividers << QLineF(along, h, along, h - Grid); break;
				}
			}
			if (!slot.isTerminal())
				continue;

			Layout::PlacedTerminal t{slot, side, {}, {}};
			Layout::Name name;
			name.text = slot.shownText();
			const int tw = textWidth(fonts.names, name.text);
			switch (side) {
				case Left:
					t.point = {qreal(-Stub), along}; t.edge = {0, along};
					name.pos = {qreal(Inset), along + centre};
					break;
				case Right:
					t.point = {qreal(w + Stub), along}; t.edge = {qreal(w), along};
					name.pos = {qreal(w - Inset - tw), along + centre};
					break;
				case Top:
					t.point = {along, qreal(-Stub)}; t.edge = {along, 0};
					name.pos = {along + centre, qreal(Inset + tw)};
					name.rotation = 270;
					break;
				case Bottom:
					t.point = {along, qreal(h + Stub)}; t.edge = {along, qreal(h)};
					name.pos = {along + centre, qreal(h - Inset)};
					name.rotation = 270;
					break;
			}
			l.terminals << t;
			l.names << name;
		}
	}

		//The label and the reference sit between the left and right names;
		//with no room there they are centred and may cross the names rather
		//than vanish.
	const int left_room = has(Left) ? longest(Left) + 2 * Inset : Inset;
	const int right_room = has(Right) ? longest(Right) + 2 * Inset : Inset;
	qreal text_w = w - left_room - right_room;
	qreal text_x = left_room;
	if (text_w < label_w) {
		text_w = label_w;
		text_x = (w - text_w) / 2.0;
	}
	l.label_box = QRectF(text_x, h / 2.0 - label_h, text_w, label_h);

	if (pitch < 20 && spec.terminalCount())
		l.warnings << QCoreApplication::translate("GenericDevice", "At a pitch of 1 grid square the names of neighbouring terminals overlap.");
	QStringList names;
	for (const auto &t : l.terminals) names << t.slot.name;
	const auto repeated = TerminalNameCheck::repeatedNames(names);
	if (!repeated.isEmpty())
		l.warnings << QCoreApplication::translate("GenericDevice", "Repeated terminal names: %1. Fine if the device really repeats them, as N or PE often are.")
				      .arg(TerminalNameCheck::describe(repeated));
	if (TerminalNameCheck::unnamedCount(names))
		l.warnings << QCoreApplication::translate("GenericDevice", "%n terminal(s) without a name.", "",
							  TerminalNameCheck::unnamedCount(names));
	return l;
}

/**
	@brief toDefinition
	@return the element definition of @a spec, appended to @a document: an
	ordinary symbol made of a rectangle, lines and static texts, with a
	dynamic text for the label and one for the manufacturer reference. Each
	terminal keeps the uuid of its slot, or gets a new one.
	@param version : the version written in the definition
*/
QDomElement toDefinition(const Spec &spec,
			 const Fonts &fonts,
			 QDomDocument &document,
			 const QString &version)
{
	const Layout l = layout(spec, fonts);
	const int w = l.body_width, h = l.body_height;

		//The declared box contains the drawing: the body, the stubs and the
		//label, with a margin, rounded out to the grid.
	qreal min_x = std::min(0.0, l.label_box.left()), max_x = std::max(qreal(w), l.label_box.right());
	qreal min_y = 0, max_y = h;
	for (const auto &t : l.terminals) {
		min_x = std::min(min_x, t.point.x()); max_x = std::max(max_x, t.point.x());
		min_y = std::min(min_y, t.point.y()); max_y = std::max(max_y, t.point.y());
	}
	const int hotspot_x = ceilGrid(-min_x + 5), hotspot_y = ceilGrid(-min_y + 5);
	const int width = ceilGrid(max_x + hotspot_x + 5), height = ceilGrid(max_y + hotspot_y + 5);

	QDomElement definition = document.createElement(QStringLiteral("definition"));
	if (!version.isEmpty())
		definition.setAttribute(QStringLiteral("version"), version);
	definition.setAttribute(QStringLiteral("type"), QStringLiteral("element"));
	definition.setAttribute(QStringLiteral("link_type"), QStringLiteral("simple"));
	definition.setAttribute(QStringLiteral("width"), width);
	definition.setAttribute(QStringLiteral("height"), height);
	definition.setAttribute(QStringLiteral("hotspot_x"), hotspot_x);
	definition.setAttribute(QStringLiteral("hotspot_y"), hotspot_y);
	document.appendChild(definition);

	QDomElement uuid = document.createElement(QStringLiteral("uuid"));
	uuid.setAttribute(QStringLiteral("uuid"), QUuid::createUuid().toString());
	definition.appendChild(uuid);

		//Typed once, so stored once, under English, which every language
		//falls back to (NamesList::name)
	QDomElement names = document.createElement(QStringLiteral("names"));
	QDomElement name = document.createElement(QStringLiteral("name"));
	name.setAttribute(QStringLiteral("lang"), QStringLiteral("en"));
	name.appendChild(document.createTextNode(spec.name.trimmed()));
	names.appendChild(name);
	definition.appendChild(names);

	QDomElement informations = document.createElement(QStringLiteral("elementInformations"));
	const QList<QPair<QString, QString>> infos{
		{QStringLiteral("description"), spec.description},
		{QStringLiteral("label"), spec.label},
		{QStringLiteral("manufacturer"), spec.manufacturer},
		{QStringLiteral("manufacturer_reference"), spec.reference}};
	for (const auto &info : infos)
	{
		if (info.second.trimmed().isEmpty())
			continue;
		QDomElement e = document.createElement(QStringLiteral("elementInformation"));
		e.setAttribute(QStringLiteral("show"), 1);
		e.setAttribute(QStringLiteral("name"), info.first);
		e.appendChild(document.createTextNode(info.second.trimmed()));
		informations.appendChild(e);
	}
	definition.appendChild(informations);

	QDomElement description = document.createElement(QStringLiteral("description"));
	const QString style = QStringLiteral("line-style:normal;line-weight:normal;filling:none;color:black");

	QDomElement rect = document.createElement(QStringLiteral("rect"));
	rect.setAttribute(QStringLiteral("x"), 0);
	rect.setAttribute(QStringLiteral("y"), 0);
	rect.setAttribute(QStringLiteral("width"), w);
	rect.setAttribute(QStringLiteral("height"), h);
	rect.setAttribute(QStringLiteral("rx"), 0);
	rect.setAttribute(QStringLiteral("ry"), 0);
	rect.setAttribute(QStringLiteral("style"), style);
	rect.setAttribute(QStringLiteral("antialias"), QStringLiteral("false"));
	description.appendChild(rect);

	auto add_line = [&](const QLineF &line) {
		QDomElement e = document.createElement(QStringLiteral("line"));
		e.setAttribute(QStringLiteral("x1"), number(line.x1()));
		e.setAttribute(QStringLiteral("y1"), number(line.y1()));
		e.setAttribute(QStringLiteral("x2"), number(line.x2()));
		e.setAttribute(QStringLiteral("y2"), number(line.y2()));
		e.setAttribute(QStringLiteral("end1"), QStringLiteral("none"));
		e.setAttribute(QStringLiteral("end2"), QStringLiteral("none"));
		e.setAttribute(QStringLiteral("length1"), QStringLiteral("1.5"));
		e.setAttribute(QStringLiteral("length2"), QStringLiteral("1.5"));
		e.setAttribute(QStringLiteral("style"), style);
		e.setAttribute(QStringLiteral("antialias"), QStringLiteral("false"));
		description.appendChild(e);
	};
	for (const auto &t : l.terminals)
		add_line(QLineF(t.edge, t.point));
	for (const QLineF &divider : l.dividers)
		add_line(divider);

	for (const auto &n : l.names)
	{
		QDomElement e = document.createElement(QStringLiteral("text"));
		e.setAttribute(QStringLiteral("x"), number(n.pos.x()));
		e.setAttribute(QStringLiteral("y"), number(n.pos.y()));
		e.setAttribute(QStringLiteral("text"), n.text);
		e.setAttribute(QStringLiteral("font"), fonts.names.toString());
		e.setAttribute(QStringLiteral("rotation"), n.rotation);
		e.setAttribute(QStringLiteral("color"), QStringLiteral("#000000"));
		e.setAttribute(QStringLiteral("uuid"), QUuid::createUuid().toString());
		description.appendChild(e);
	}

	auto add_dynamic = [&](const QString &info_name, const QString &cached, qreal y, const QFont &font) {
		QDomElement e = document.createElement(QStringLiteral("dynamic_text"));
		e.setAttribute(QStringLiteral("x"), number(l.label_box.x()));
		e.setAttribute(QStringLiteral("y"), number(y));
		e.setAttribute(QStringLiteral("z"), 2);
		e.setAttribute(QStringLiteral("text_width"), number(l.label_box.width()));
		e.setAttribute(QStringLiteral("Halignment"), QStringLiteral("AlignHCenter"));
		e.setAttribute(QStringLiteral("Valignment"), QStringLiteral("AlignTop"));
		e.setAttribute(QStringLiteral("frame"), QStringLiteral("false"));
		e.setAttribute(QStringLiteral("rotation"), 0);
		e.setAttribute(QStringLiteral("keep_visual_rotation"), QStringLiteral("true"));
		e.setAttribute(QStringLiteral("text_from"), QStringLiteral("ElementInfo"));
		e.setAttribute(QStringLiteral("uuid"), QUuid::createUuid().toString());
		e.setAttribute(QStringLiteral("font"), font.toString());
		QDomElement text = document.createElement(QStringLiteral("text"));
		text.appendChild(document.createTextNode(cached));
		e.appendChild(text);
		QDomElement info = document.createElement(QStringLiteral("info_name"));
		info.appendChild(document.createTextNode(info_name));
		e.appendChild(info);
		description.appendChild(e);
	};
	add_dynamic(QStringLiteral("label"), spec.label.trimmed(), l.label_box.y(), fonts.label);
	if (spec.show_reference && !spec.reference.trimmed().isEmpty())
		add_dynamic(QStringLiteral("manufacturer_reference"), spec.reference.trimmed(),
			    l.label_box.bottom(), fonts.names);

	for (const auto &t : l.terminals)
	{
		QDomElement e = document.createElement(QStringLiteral("terminal"));
		e.setAttribute(QStringLiteral("x"), number(t.point.x()));
		e.setAttribute(QStringLiteral("y"), number(t.point.y()));
		e.setAttribute(QStringLiteral("uuid"),
			       (t.slot.uuid.isNull() ? QUuid::createUuid() : t.slot.uuid).toString());
		e.setAttribute(QStringLiteral("name"), t.slot.name);
		e.setAttribute(QStringLiteral("orientation"), QLatin1String(orientationOf(t.side)));
		e.setAttribute(QStringLiteral("type"), t.slot.type);
		description.appendChild(e);
	}

	definition.appendChild(description);
	return definition;
}

/**
	@brief fromDefinition
	Read back a definition written by toDefinition(): side from each
	terminal's orientation, order from its position along the edge, gaps
	from the pitch list with no terminal, names, types and uuids from the
	terminals, functions from the drawn names.

	Not recovered: gaps before the first or after the last terminal of a
	side, which leave no trace in the drawing. With divider marks on, an
	unmarked empty slot is line-up padding and is dropped.
	@return false if @a definition is not a generic device: anything other
	than one body rectangle, stubs, divider marks, names and terminals on
	the edges at a common pitch.
*/
bool fromDefinition(const QDomElement &definition,
		    const Fonts &fonts,
		    Spec *spec)
{
	if (definition.tagName() != QLatin1String("definition") || !spec)
		return false;
	const QDomElement description = definition.firstChildElement(QStringLiteral("description"));
	if (description.isNull())
		return false;

	Spec s;
	s.mark_gaps = false;
	s.show_reference = false;

	const QDomElement names = definition.firstChildElement(QStringLiteral("names"));
	for (QDomElement n = names.firstChildElement(QStringLiteral("name")) ; !n.isNull() ;
	     n = n.nextSiblingElement(QStringLiteral("name"))) {
		if (s.name.isEmpty() || n.attribute(QStringLiteral("lang")) == QLatin1String("en"))
			s.name = n.text();
	}
	const QDomElement infos = definition.firstChildElement(QStringLiteral("elementInformations"));
	for (QDomElement i = infos.firstChildElement(QStringLiteral("elementInformation")) ; !i.isNull() ;
	     i = i.nextSiblingElement(QStringLiteral("elementInformation"))) {
		const QString key = i.attribute(QStringLiteral("name"));
		if (key == QLatin1String("label")) s.label = i.text();
		else if (key == QLatin1String("manufacturer")) s.manufacturer = i.text();
		else if (key == QLatin1String("manufacturer_reference")) s.reference = i.text();
		else if (key == QLatin1String("description")) s.description = i.text();
	}

	int rects = 0, w = 0, h = 0;
	QList<QLineF> lines;
	struct TextPart { QPointF pos; QString text; int rotation; };
	QList<TextPart> texts;
	struct Read { Slot slot; qreal along; QPointF point; };
	QList<Read> by_side[SideCount];
	for (QDomElement e = description.firstChildElement() ; !e.isNull() ; e = e.nextSiblingElement())
	{
		const QString tag = e.tagName();
		if (tag == QLatin1String("rect")) {
			if (++rects > 1 || e.attribute(QStringLiteral("x")).toDouble() != 0
			    || e.attribute(QStringLiteral("y")).toDouble() != 0)
				return false;
			w = e.attribute(QStringLiteral("width")).toInt();
			h = e.attribute(QStringLiteral("height")).toInt();
		} else if (tag == QLatin1String("line")) {
			lines << QLineF(e.attribute(QStringLiteral("x1")).toDouble(), e.attribute(QStringLiteral("y1")).toDouble(),
					e.attribute(QStringLiteral("x2")).toDouble(), e.attribute(QStringLiteral("y2")).toDouble());
		} else if (tag == QLatin1String("text")) {
			texts << TextPart{QPointF(e.attribute(QStringLiteral("x")).toDouble(), e.attribute(QStringLiteral("y")).toDouble()),
					  e.attribute(QStringLiteral("text")), e.attribute(QStringLiteral("rotation")).toInt()};
		} else if (tag == QLatin1String("dynamic_text")) {
			const QString info = e.firstChildElement(QStringLiteral("info_name")).text();
			if (info == QLatin1String("manufacturer_reference"))
				s.show_reference = true;
			else if (info != QLatin1String("label"))
				return false;
		} else if (tag == QLatin1String("terminal")) {
			Slot slot = Slot::terminal(e.attribute(QStringLiteral("name")));
			slot.type = e.attribute(QStringLiteral("type"), QStringLiteral("Generic"));
			slot.uuid = QUuid(e.attribute(QStringLiteral("uuid")));
			const qreal x = e.attribute(QStringLiteral("x")).toDouble();
			const qreal y = e.attribute(QStringLiteral("y")).toDouble();
			const QString o = e.attribute(QStringLiteral("orientation"));
			const QPointF point(x, y);
			if (o == QLatin1String("w")) by_side[Left] << Read{slot, y, point};
			else if (o == QLatin1String("e")) by_side[Right] << Read{slot, y, point};
			else if (o == QLatin1String("n")) by_side[Top] << Read{slot, x, point};
			else if (o == QLatin1String("s")) by_side[Bottom] << Read{slot, x, point};
			else return false;
		} else {
			return false;
		}
	}
	if (rects != 1 || w <= 0 || h <= 0)
		return false;

		//Every terminal sits one stub outside its edge, joined to it by a
		//line; every other line is a divider mark on the inside of an edge
	QList<QLineF> stubs;
	for (int side = 0 ; side < SideCount ; ++side)
		for (const Read &r : by_side[side])
		{
			QPointF edge;
			switch (side) {
				case Left:   edge = QPointF(0, r.along); break;
				case Right:  edge = QPointF(w, r.along); break;
				case Top:    edge = QPointF(r.along, 0); break;
				case Bottom: edge = QPointF(r.along, h); break;
			}
			const QPointF out = edge + (side == Left ? QPointF(-Stub, 0) : side == Right ? QPointF(Stub, 0)
						    : side == Top ? QPointF(0, -Stub) : QPointF(0, Stub));
			if (r.point != out || !lines.contains(QLineF(edge, out)))
				return false;
			stubs << QLineF(edge, out);
		}

	QList<qreal> dividers[SideCount];
	for (const QLineF &line : lines)
	{
		if (stubs.contains(line))
			continue;
		if (line == QLineF(0, line.y1(), Grid, line.y1())) dividers[Left] << line.y1();
		else if (line == QLineF(w, line.y1(), w - Grid, line.y1())) dividers[Right] << line.y1();
		else if (line == QLineF(line.x1(), 0, line.x1(), Grid)) dividers[Top] << line.x1();
		else if (line == QLineF(line.x1(), h, line.x1(), h - Grid)) dividers[Bottom] << line.x1();
		else return false;
	}

		//The pitch is the smallest step between two list of a side; every
		//other step must be a multiple of it
	QList<qreal> alongs[SideCount];
	int pitch = 0;
	for (int side = 0 ; side < SideCount ; ++side)
	{
		std::sort(by_side[side].begin(), by_side[side].end(),
			  [](const Read &a, const Read &b) { return a.along < b.along; });
		for (const Read &r : by_side[side]) alongs[side] << r.along;
		alongs[side] << dividers[side];
		std::sort(alongs[side].begin(), alongs[side].end());
		for (int i = 1 ; i < alongs[side].size() ; ++i) {
			const int step = qRound(alongs[side][i] - alongs[side][i - 1]);
			if (step > 0 && (!pitch || step < pitch)) pitch = step;
		}
	}
	if (!pitch)
		pitch = 20;
	if (pitch % Grid || pitch < 10 || pitch > 50)
		return false;
	s.pitch = pitch;

	int padding = 0;
	for (int side = 0 ; side < SideCount ; ++side)
	{
		if (alongs[side].isEmpty())
			continue;
		const qreal first = alongs[side].first();
		const qreal last = alongs[side].last();
		if (qRound(last - first) % pitch)
			return false;
		int next = 0;
		for (qreal along = first ; along <= last + 0.5 ; along += pitch)
		{
			if (next < by_side[side].size() && qFuzzyCompare(by_side[side][next].along + 1, along + 1)) {
				s.sides[side] << by_side[side][next++].slot;
			} else if (std::any_of(dividers[side].cbegin(), dividers[side].cend(),
					       [&](qreal d) { return qFuzzyCompare(d + 1, along + 1); })) {
				s.sides[side] << Slot::gap();
				s.mark_gaps = true;
			} else {
				Slot gap = Slot::gap();
				gap.kind = Slot::Padding; //sorted out below
				s.sides[side] << gap;
			}
		}
		if (next != by_side[side].size())
			return false;
	}

		//With divider marks on, every gap the user made is marked, so an
		//unmarked one is padding; with them off the two cannot be told
		//apart, and every gap is kept as typed.
	for (Slots &side : s.sides)
		for (int i = side.size() - 1 ; i >= 0 ; --i)
			if (side[i].kind == Slot::Padding) {
				if (s.mark_gaps) { side.removeAt(i); ++padding; }
				else side[i].kind = Slot::Gap;
			}
	if (s.mark_gaps) {
			//Line-up was on if it padded, or if it would change nothing
		Spec check = s;
		lineUpGroups(check.sides[Left], check.sides[Right]);
		lineUpGroups(check.sides[Top], check.sides[Bottom]);
		bool adds = false;
		for (int side = 0 ; side < SideCount ; ++side)
			adds |= check.sides[side].size() != s.sides[side].size();
		s.line_up = padding > 0 || !adds;
	} else {
		s.line_up = false;
	}

		//Functions: the drawn name is "name function", on the terminal's
		//line (within half a pitch), on the terminal's side of the body
	for (int side = 0 ; side < SideCount ; ++side)
	{
		const bool vertical = side == Left || side == Right;
		int next = 0;
		for (Slot &slot : s.sides[side])
		{
			if (!slot.isTerminal())
				continue;
			const qreal along = by_side[side].at(next++).along;
			const QString prefix = slot.name + QLatin1Char(' ');
			for (const TextPart &t : texts)
			{
				if ((t.rotation != 0) == vertical || !t.text.startsWith(prefix))
					continue;
				const qreal t_along = vertical ? t.pos.y() : t.pos.x();
				if (qAbs(t_along - along) >= pitch / 2.0)
					continue;
				const bool near_start = vertical ? t.pos.x() == Inset : t.pos.y() != h - Inset;
				if (near_start != (side == Left || side == Top))
					continue;
				slot.function = t.text.mid(prefix.size());
				break;
			}
		}
	}

		//The extra size is whatever the body has beyond the automatic size
	Spec sized = s;
	sized.extra_width = sized.extra_height = 0;
	const Layout auto_layout = layout(sized, fonts);
	s.extra_width = std::max(0, (w - auto_layout.auto_width) / Grid);
	s.extra_height = std::max(0, (h - auto_layout.auto_height) / Grid);

	*spec = s;
	return true;
}

} // namespace GenericDevice
