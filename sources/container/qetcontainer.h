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

#include "qetcontainerdb.h"
#include "qetzip.h"

#include <QDomDocument>
#include <QHash>

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
	constexpr int Format = 3;

	QList<QetZip::Entry> split(const QDomDocument &project,
							   const QString &writer = QString(),
							   QString *error = nullptr,
							   const QByteArray &index = QByteArray(),
							   const QHash<QString, QByteArray> &kept_from = QHash<QString, QByteArray>());
	bool join(const QList<QetZip::Entry> &entries, QDomDocument *project,
			  QString *error = nullptr, QStringList *folio_files = nullptr);
		/// A folio part of @p files, which join() left out, with its
		/// pictures and its engineering data back (QETZ-DB-TRUTH-PLAN.md D2)
		/// @p reader, if any, reads the engineering data of @p files
	bool readFolio(const QHash<QString, QByteArray> &files, const QString &file,
				   QDomDocument *folio, QString *error = nullptr,
				   QetContainerDb::FolioReader *reader = nullptr);

		/// Take what QElectroTech computes out of a folio (format 3)
	void dropComputed(QDomElement diagram, bool terminal_lists);
		/// Parse @p xml as a .qet is read (QETProject::openFile)
	bool parse(const QByteArray &xml, QDomDocument *document, QString *error = nullptr);
		/// true if @p content is a zip, so a .qetz rather than a .qet
	bool isZip(const QByteArray &content);
		/// The project document in @p content, a .qet or a .qetz
	bool readDocument(const QByteArray &content, QDomDocument *document, QString *error = nullptr);
}

#endif // QETCONTAINER_H
