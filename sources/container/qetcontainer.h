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
#ifndef QETCONTAINER_H
#define QETCONTAINER_H

#include "qetzip.h"

#include <QDomDocument>

/**
	@brief The QetContainer namespace
	Splits a project document into the parts of a .qetz file and joins
	them back:

	  mimetype                 first, stored
	  manifest.xml             format and oldest format allowed to read it
	  project.xml              the project, each part replaced by
	                           <qetz-part file="..."/>
	  project.sqlite           the engineering data (format 2,
	                           QetContainerDb): title-block fields, symbol
	                           information, links, wires
	  folios/<uuid>.xml        one per folio
	  elements/.../<name>      the project's symbol definitions
	  titleblocks/<name>.titleblock
	  images/<sha256>.png      folio pictures, once each, not base64

	join(split(d)) is d. Nothing here knows what the parts mean: the
	project is read and written by the same code as a .qet file.
*/
namespace QetContainer
{
	extern const char MimeType[];
		/// The format written; files of a later format are refused by join()
		/// unless their min-reader allows this one.
	constexpr int Format = 2;

	QList<QetZip::Entry> split(const QDomDocument &project,
							   const QString &writer = QString(),
							   QString *error = nullptr);
	bool join(const QList<QetZip::Entry> &entries, QDomDocument *project,
			  QString *error = nullptr);

		/// Parse @p xml as a .qet is read (QETProject::openFile)
	bool parse(const QByteArray &xml, QDomDocument *document, QString *error = nullptr);
		/// true if @p content is a zip, so a .qetz rather than a .qet
	bool isZip(const QByteArray &content);
		/// The project document in @p content, a .qet or a .qetz
	bool readDocument(const QByteArray &content, QDomDocument *document, QString *error = nullptr);
}

#endif // QETCONTAINER_H
