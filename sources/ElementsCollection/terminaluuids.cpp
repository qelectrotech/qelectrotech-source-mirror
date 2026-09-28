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

	Terminals that moved, and new ones, keep their own uuid. Where the old
	definition has two terminals at one place, they are matched in order.
*/
void TerminalUuids::keep(const QDomElement &old_element, QDomElement &new_element)
{
	QHash<QString, QStringList> old_uuids;
	for (const QDomElement &t : terminalsOf(old_element)) {
		const QString uuid = t.attribute(QStringLiteral("uuid"));
		if (!uuid.isEmpty()) {
			old_uuids[terminalPlace(t)] << uuid;
		}
	}
	if (old_uuids.isEmpty()) {
		return;
	}

	for (QDomElement t : terminalsOf(new_element)) {
		QStringList &uuids = old_uuids[terminalPlace(t)];
		if (!uuids.isEmpty()) {
			t.setAttribute(QStringLiteral("uuid"), uuids.takeFirst());
		}
	}
}
