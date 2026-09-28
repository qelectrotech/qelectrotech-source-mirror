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
#include <QSet>
#include <QStringList>

namespace {
/**
	@return the <terminal> of the definition held by the embedded
	collection element @p collection_element
*/
QList<QDomElement> terminalsOf(const QDomElement &collection_element)
{
	QList<QDomElement> terminals;
	const QDomElement description = collection_element
			.firstChildElement(QStringLiteral("definition"))
			.firstChildElement(QStringLiteral("description"));
	for (QDomElement t = description.firstChildElement(QStringLiteral("terminal"));
		 !t.isNull();
		 t = t.nextSiblingElement(QStringLiteral("terminal"))) {
		terminals << t;
	}
	return terminals;
}

	//Place and orientation, "10" and "10.0" being the same place
QString terminalPlace(const QDomElement &terminal)
{
	return QStringLiteral("%1|%2|%3")
			.arg(QString::number(terminal.attribute(QStringLiteral("x")).toDouble()),
				 QString::number(terminal.attribute(QStringLiteral("y")).toDouble()),
				 terminal.attribute(QStringLiteral("orientation")));
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
