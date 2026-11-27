// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "revisionlist.h"

namespace {
	const QString revision_attributes[] = {
		QStringLiteral("revision"), QStringLiteral("date"),
		QStringLiteral("description"), QStringLiteral("author"),
		QStringLiteral("approver")};
}

bool Revision::operator==(const Revision &other) const
{
	return revision == other.revision && date == other.date
			&& description == other.description && author == other.author
			&& approver == other.approver;
}

/**
	@brief RevisionList::toXml
	@return <revisions>, one <revision> each, every field an attribute
	written only when not empty
*/
QDomElement RevisionList::toXml(QDomDocument &document) const
{
	QDomElement list = document.createElement(QStringLiteral("revisions"));
	for (const Revision &r : m_revisions) {
		QDomElement e = document.createElement(QStringLiteral("revision"));
		const QString values[] = {r.revision, r.date, r.description, r.author, r.approver};
		for (int i = 0 ; i < 5 ; ++i)
			if (!values[i].isEmpty())
				e.setAttribute(revision_attributes[i], values[i]);
		list.appendChild(e);
	}
	return list;
}

/**
	@brief RevisionList::fromXml
	@param element : a <revisions> element, possibly null
	@return its revisions, in order; none for a null element
*/
RevisionList RevisionList::fromXml(const QDomElement &element)
{
	RevisionList list;
	for (QDomElement e = element.firstChildElement(QStringLiteral("revision")) ; !e.isNull()
		 ; e = e.nextSiblingElement(QStringLiteral("revision"))) {
		Revision r;
		r.revision = e.attribute(QStringLiteral("revision"));
		r.date = e.attribute(QStringLiteral("date"));
		r.description = e.attribute(QStringLiteral("description"));
		r.author = e.attribute(QStringLiteral("author"));
		r.approver = e.attribute(QStringLiteral("approver"));
		list.m_revisions << r;
	}
	return list;
}
