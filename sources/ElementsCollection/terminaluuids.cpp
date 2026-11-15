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
#include "terminaluuids.h"

#include <QDomElement>
#include <QHash>
#include <QUuid>
#include <QSet>
#include <QStringList>

namespace {
/**
	@return the <terminal> of the definition held by the embedded
	collection element @p collection_element
*/
	//The <terminal> of the <definition> @p definition
QList<QDomElement> definitionTerminals(const QDomElement &definition)
{
	QList<QDomElement> terminals;
	const QDomElement description =
			definition.firstChildElement(QStringLiteral("description"));
	for (QDomElement t = description.firstChildElement(QStringLiteral("terminal"));
		 !t.isNull();
		 t = t.nextSiblingElement(QStringLiteral("terminal"))) {
		terminals << t;
	}
	return terminals;
}

QList<QDomElement> terminalsOf(const QDomElement &collection_element)
{
	return definitionTerminals(
				collection_element.firstChildElement(QStringLiteral("definition")));
}

	//Place and orientation, "10" and "10.0" being the same place
QString terminalPlace(const QDomElement &terminal)
{
	return QStringLiteral("%1|%2|%3")
			.arg(QString::number(terminal.attribute(QStringLiteral("x")).toDouble()),
				 QString::number(terminal.attribute(QStringLiteral("y")).toDouble()),
				 terminal.attribute(QStringLiteral("orientation")));
}

	//Qet::orientationFromString(), without pulling in qet.cpp
int orientationOf(const QDomElement &terminal)
{
	const QString o = terminal.attribute(QStringLiteral("orientation"));
	if (o.startsWith(QLatin1Char('e'))) return 1;
	if (o.startsWith(QLatin1Char('s'))) return 2;
	if (o.startsWith(QLatin1Char('w'))) return 3;
	return 0;
}

	//The <terminal> of one <definition> get a uuid where missing
int fillDefinition(const QDomElement &definition)
{
	const QList<QDomElement> terminals = definitionTerminals(definition);
	QSet<QUuid> taken;
	for (const QDomElement &t : terminals) {
		const QUuid uuid(t.attribute(QStringLiteral("uuid")));
		if (!uuid.isNull()) {
			taken << uuid;
		}
	}

	int filled = 0;
	QHash<QString, int> seen_at;
	for (QDomElement t : terminals) {
			//Same count as Terminal::setPlaceRank(): every terminal before
			//this one at the same point, with or without a uuid
		const int rank = seen_at[terminalPlace(t)]++;
		if (!QUuid(t.attribute(QStringLiteral("uuid"))).isNull()) {
			continue;
		}
		const qreal x = t.attribute(QStringLiteral("x")).toDouble();
		const qreal y = t.attribute(QStringLiteral("y")).toDouble();
		const int orientation = orientationOf(t);
		QUuid uuid;
		for (int occurrence = rank ; uuid.isNull() || taken.contains(uuid) ; ++occurrence) {
			uuid = TerminalUuids::derived(x, y, orientation, occurrence);
		}
		taken << uuid;
		t.setAttribute(QStringLiteral("uuid"), uuid.toString());
		++filled;
	}
	return filled;
}

int fillDirectory(const QDomElement &directory)
{
	int filled = 0;
	for (QDomElement child = directory.firstChildElement();
		 !child.isNull();
		 child = child.nextSiblingElement()) {
		if (child.tagName() == QLatin1String("category")) {
			filled += fillDirectory(child);
		} else if (child.tagName() == QLatin1String("element")) {
			filled += fillDefinition(
						  child.firstChildElement(QStringLiteral("definition")));
		}
	}
	return filled;
}

}

/**
	@brief TerminalUuids::keep
	Give each terminal of @p new_element the uuid of the terminal of
	@p old_element at the same place and orientation. Both are <element>
	of an embedded collection.

	A wire is saved against the uuids of the terminals it joins, and on
	loading it is reattached to a terminal with that uuid or not at all.
	The symbols already on the folios keep the terminals of the definition
	they were built from, so when the project's definition is replaced by
	one whose terminal uuids differ -- another copy of the same symbol, or
	one saved by a different version of the collection -- every wire on
	those symbols was lost the next time the project was opened.

	A terminal of the new definition that already carries one of the old
	uuids is that same terminal, perhaps moved: it keeps it, and that uuid
	is not given to anything else. Other terminals that moved, and new
	ones, keep their own. No uuid is ever given to two terminals. Where the
	old definition has two terminals at one place, they are matched in
	order.
*/
void TerminalUuids::keep(const QDomElement &old_element, QDomElement &new_element)
{
	QHash<QString, QStringList> old_uuids;
	QSet<QString> all_old_uuids;
	for (const QDomElement &t : terminalsOf(old_element)) {
		const QString uuid = t.attribute(QStringLiteral("uuid"));
		if (!uuid.isEmpty()) {
			old_uuids[terminalPlace(t)] << uuid;
			all_old_uuids << uuid;
		}
	}
	if (old_uuids.isEmpty()) {
		return;
	}

	const QList<QDomElement> new_terminals = terminalsOf(new_element);
	QSet<QString> taken;
	for (const QDomElement &t : new_terminals) {
		taken << t.attribute(QStringLiteral("uuid"));
	}

	for (QDomElement t : new_terminals) {
		const QString own = t.attribute(QStringLiteral("uuid"));
		if (all_old_uuids.contains(own)) {
			continue;
		}
		QStringList &uuids = old_uuids[terminalPlace(t)];
		while (!uuids.isEmpty() && taken.contains(uuids.first())) {
			uuids.removeFirst();
		}
		if (!uuids.isEmpty()) {
			const QString uuid = uuids.takeFirst();
			taken.remove(own);
			taken << uuid;
			t.setAttribute(QStringLiteral("uuid"), uuid);
		}
	}
}

/**
	@brief TerminalUuids::keepInDirectory
	keep() for every symbol of @p new_directory that has a counterpart of
	the same name at the same place under @p old_directory. Both are
	<category> of an embedded collection; used when a whole category of
	the project is replaced.
*/
void TerminalUuids::keepInDirectory(const QDomElement &old_directory,
									QDomElement &new_directory)
{
	for (QDomElement child = new_directory.firstChildElement();
		 !child.isNull();
		 child = child.nextSiblingElement()) {
		if (child.tagName() != QLatin1String("category")
			&& child.tagName() != QLatin1String("element")) {
			continue;
		}
		const QString name = child.attribute(QStringLiteral("name"));
		QDomElement old_child = old_directory.firstChildElement(child.tagName());
		while (!old_child.isNull()
			   && old_child.attribute(QStringLiteral("name")) != name) {
			old_child = old_child.nextSiblingElement(child.tagName());
		}
		if (old_child.isNull()) {
			continue;
		}
		if (child.tagName() == QLatin1String("category")) {
			keepInDirectory(old_child, child);
		} else {
			keep(old_child, child);
		}
	}
}

/**
	@brief TerminalUuids::derived
	The identity of a terminal that carries no uuid, worked out from where
	it is inside its symbol: its local position and orientation, which are
	what the project file itself uses to tell terminals apart. UUID v5 in a
	fixed namespace, so the same terminal gets the same value in any
	project, on any machine, and it cannot collide with the random (v4)
	uuids the element editor gives.

	@param orientation Qet::Orientation, as an int
	@param occurrence 0 for the value Terminal::stableUuid() uses; 1, 2...
	for a second, third... terminal at the same point of the same symbol
*/
QUuid TerminalUuids::derived(qreal x, qreal y, int orientation, int occurrence)
{
		//Fixed namespace for terminal identities derived from geometry.
	static const QUuid derived_ns(QStringLiteral("{6b1f6d1e-6a1a-5f7e-9a3d-9c0a5b2d7e11}"));

	QString key = QStringLiteral("%1|%2|%3")
			.arg(x, 0, 'f', 4)
			.arg(y, 0, 'f', 4)
			.arg(orientation);
	if (occurrence > 0) {
		key += QStringLiteral("|%1").arg(occurrence);
	}
	return QUuid::createUuidV5(derived_ns, key);
}

/**
	@brief TerminalUuids::fillMissing
	Give every terminal without a uuid, in every symbol of the embedded
	collection @p collection_root, the value derived() works out for it --
	the one QElectroTech already used for it as Terminal::stableUuid(), so
	nothing keyed on terminals changes. Saved with the project, it becomes
	a lasting identity: moving the terminal in the symbol editor no longer
	changes it.

	Where one symbol has two terminals at one point, the second gets the
	next occurrence; a value is never given twice within a symbol.
	@return the number of terminals given a uuid
*/
int TerminalUuids::fillMissing(const QDomElement &collection_root)
{
	return fillDirectory(collection_root);
}

/**
	@brief TerminalUuids::fillMissingInDefinition
	fillMissing() for a single element definition (@p definition is its
	<definition>), as the element editor reads it from a file: an old
	symbol opened and saved there gets the uuids a project gives the same
	terminals, not random ones that would make every copy of it differ.
	@return the number of terminals given a uuid
*/
int TerminalUuids::fillMissingInDefinition(const QDomElement &definition)
{
	return fillDefinition(definition);
}

namespace {
struct PlacedTerminal
{
	QPointF pos;
	QString orientation;
	QDomElement element;
};

QList<PlacedTerminal> placedTerminals(const QDomElement &definition)
{
	QList<PlacedTerminal> terminals;
	for (const QDomElement &t : definitionTerminals(definition))
		terminals << PlacedTerminal{QPointF(t.attribute(QStringLiteral("x")).toDouble(),
											t.attribute(QStringLiteral("y")).toDouble()),
									t.attribute(QStringLiteral("orientation")), t};
	return terminals;
}

bool samePoint(const QPointF &a, const QPointF &b)
{
	return qAbs(a.x() - b.x()) < 0.01 && qAbs(a.y() - b.y()) < 0.01;
}

	//The index in @p terminals of an unused one at @p pos with @p orientation, else -1
int findTerminal(const QList<PlacedTerminal> &terminals, const QList<bool> &used,
				 const QPointF &pos, const QString &orientation)
{
	for (int i = 0 ; i < terminals.size() ; ++i)
		if (!used.at(i) && terminals.at(i).orientation == orientation
			&& samePoint(terminals.at(i).pos, pos))
			return i;
	return -1;
}
}

/**
	@brief TerminalUuids::commonOffset
	Whether every terminal of @p new_definition is a terminal of
	@p old_definition moved by one and the same offset, with the same
	orientation: the drawing was moved as a whole, which is what moving its
	insertion point does. Two different offsets cannot both fit, so the
	answer is unique.
	@param old_definition, new_definition : two <definition>
	@param offset : set to the offset, new place less old place
	@return false if the terminals differ otherwise, or there are none
*/
bool TerminalUuids::commonOffset(const QDomElement &old_definition,
								 const QDomElement &new_definition, QPointF *offset)
{
	const QList<PlacedTerminal> old_terminals = placedTerminals(old_definition);
	const QList<PlacedTerminal> new_terminals = placedTerminals(new_definition);
	if (old_terminals.isEmpty() || old_terminals.size() != new_terminals.size())
		return false;

	const PlacedTerminal &first = old_terminals.first();
	for (const PlacedTerminal &candidate : new_terminals)
	{
		if (candidate.orientation != first.orientation)
			continue;
		const QPointF shift = candidate.pos - first.pos;
		QList<bool> used(new_terminals.size(), false);
		bool all = true;
		for (const PlacedTerminal &t : old_terminals)
		{
			const int found = findTerminal(new_terminals, used, t.pos + shift, t.orientation);
			if (found < 0) { all = false; break; }
			used[found] = true;
		}
		if (all) {
			if (offset) *offset = shift;
			return true;
		}
	}
	return false;
}

/**
	@brief TerminalUuids::keepShifted
	keep() for a drawing moved by @p offset (see commonOffset()): each
	terminal of @p new_definition takes the uuid of the terminal of
	@p old_definition it was moved from, so the wires saved against it find
	it again.
*/
void TerminalUuids::keepShifted(const QDomElement &old_definition,
								QDomElement &new_definition, const QPointF &offset)
{
	const QList<PlacedTerminal> old_terminals = placedTerminals(old_definition);
	QList<bool> used(old_terminals.size(), false);
	for (PlacedTerminal t : placedTerminals(new_definition))
	{
		const int found = findTerminal(old_terminals, used, t.pos - offset, t.orientation);
		if (found < 0)
			continue;
		used[found] = true;
		const QString uuid = old_terminals.at(found).element.attribute(QStringLiteral("uuid"));
		if (!uuid.isEmpty())
			t.element.setAttribute(QStringLiteral("uuid"), uuid);
	}
}
