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

/**
	@brief The QetContainerDb namespace
	The engineering data of a project, moved between its document and the
	project.sqlite of a .qetz: what a folio's title block says, a symbol's
	information, a coil's contacts, a wire's ends and number. In a .qetz
	these live only in the database; the folio files keep where things are
	drawn. extract() takes them out of the document into a database,
	restore() puts them back, so that restore(extract(d)) is d.

	Each moved item leaves its key in the document (qetz-key, qetz-folio):
	its uuid where that is unique, else a made-up one. A block of a shape
	this does not model -- a value holding elements, say -- stays in the
	document, so nothing can be lost by moving.
*/
namespace QetContainerDb
{
	QByteArray extract(QDomElement project, QString *error = nullptr);
	bool restore(QDomElement project, const QByteArray &database, QString *error = nullptr);
}

#endif // QETCONTAINERDB_H
