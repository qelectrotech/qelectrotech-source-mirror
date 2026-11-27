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
#ifndef REVISIONLIST_H
#define REVISIONLIST_H

#include <QDomElement>
#include <QList>
#include <QString>

/**
	@brief The Revision struct
	One revision of the project: what changed, when, who made it and who
	approved it (#1295, rule 2). Every field is free text.
*/
struct Revision
{
	QString revision;    ///< its index, e.g. A, B or 01
	QString date;        ///< as typed, ISO 8601 (2026-11-25) suggested
	QString description;
	QString author;
	QString approver;

	bool operator==(const Revision &other) const;
	bool operator!=(const Revision &other) const {return !(*this == other);}
};

/**
	@brief The RevisionList class
	The revision history of a project: one list, in the project properties
	(#1878 item C8). Saved as <revisions> only when it is not empty, so a
	project without revisions is saved as before; an older QElectroTech
	skips it.
*/
class RevisionList
{
	public:
		const QList<Revision> &revisions() const {return m_revisions;}
		void setRevisions(const QList<Revision> &revisions) {m_revisions = revisions;}
		bool isEmpty() const {return m_revisions.isEmpty();}
			/// The last revision of the list, empty when there is none
		Revision latest() const {return m_revisions.isEmpty() ? Revision() : m_revisions.last();}

		QDomElement toXml(QDomDocument &document) const;
		static RevisionList fromXml(const QDomElement &element);

		bool operator==(const RevisionList &other) const {return m_revisions == other.m_revisions;}
		bool operator!=(const RevisionList &other) const {return !(*this == other);}

	private:
		QList<Revision> m_revisions;
};

#endif // REVISIONLIST_H
