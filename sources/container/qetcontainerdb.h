/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef QETCONTAINERDB_H
#define QETCONTAINERDB_H

#include <QByteArray>
#include <QDomElement>
#include <QStringList>

/**
	@brief The QetContainerDb namespace
	The engineering data of a project, moved between its document and the
	project.sqlite of a .qetz: what a folio's title block says, a symbol's
	information, a coil's contacts, a wire's ends and number. In a .qetz
	these live only in the database; the folio files keep where things are
	drawn. extract() takes them out of the document into a database,
	restore() puts them back, so that restore(extract(d)) is d.

	A row's key is its item's uuid, which stays in the folio file. An item
	without a uuid of its own -- none, or one another item shares -- gets a
	uuid made from what it holds, left in the document as qetz-key or
	qetz-folio. A block of a shape
	this does not model -- a value holding elements, say -- stays in the
	document, so nothing can be lost by moving.
*/
namespace QetContainerDb
{
	QByteArray extract(QDomElement project, QString *error = nullptr,
					   const QByteArray &kept_from = QByteArray());
		/// With @p lost, a damaged project being recovered: what does not
		/// match is noted there and left out, instead of refusing it all
	bool restore(QDomElement project, const QByteArray &database, QString *error = nullptr,
				 QStringList *lost = nullptr);
		/// What only the database could fill, left in @p project after a
		/// restore() with lost, taken out; said in @p lost
	void dropLeftovers(QDomElement project, QStringList *lost);
		/// true if @p database is of the format this writes
	bool currentFormat(const QByteArray &database);
}

#endif // QETCONTAINERDB_H
