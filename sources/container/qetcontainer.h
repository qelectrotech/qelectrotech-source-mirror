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
#include <QHash>

/**
	@brief The QetContainer namespace
	Splits a project document into the parts of a .qetz file and joins
	them back:

	  mimetype                 first, stored
	  manifest.xml             format and oldest format allowed to read it
	  project.xml              the project, each part replaced by
	                           <qetz-part file="..."/>
	  project.sqlite           the engineering data (format 2 on,
	                           QetContainerDb): title-block fields, symbol
	                           information, links, wires
	  folios/<uuid>.xml        one per folio
	  elements/.../<name>      the project's symbol definitions
	  titleblocks/<name>.titleblock
	  images/<sha256>.png      folio pictures, once each, not base64

	join(split(d)) is d. Nothing here knows what the parts mean: the
	project is read and written by the same code as a .qet file.

	Given a Pictures map, join() leaves the pictures out of the document:
	each <image> keeps the name of its file, pictureFile(), and the bytes
	go in the map. Turning them back into base64 text costs seconds on a
	project with large pictures, and the folio would only decode it again.

	The format is written down in QETZ-FORMAT.md, beside this file.
*/
namespace QetContainer
{
	extern const char MimeType[];
		/// The MIME type .qetz files had before it was renamed; still read
	extern const char OldMimeType[];
		/// Picture bytes by file name, see join()
	using Pictures = QHash<QString, QByteArray>;
		/// The format written; files of a later format are refused by join()
		/// unless their min-reader allows this one.
	constexpr int Format = 4;
		/// 3: the database's rows keyed by the items' uuids, true/false and
		/// show flags as 1/0, its PRAGMA user_version the format, the symbols' type left
		/// in the folio files, no column for where a block sat.
		/// 4: what QElectroTech computes left out of the folios
		/// (dropComputed()), the tables written without keys or indexes.
		/// project.sqlite holds the engineering data once: QElectroTech's
		/// own project database is built from it when the project opens.
		/// Formats 2 and 3 are still read; a later format whose min-reader
		/// allows this one is read too, and said so through join()'s
		/// @p format.

	QList<QetZip::Entry> split(const QDomDocument &project,
							   const QString &writer = QString(),
							   QString *error = nullptr,
							   const QHash<QString, QByteArray> &kept_from = QHash<QString, QByteArray>());
	bool join(const QList<QetZip::Entry> &entries, QDomDocument *project,
			  QString *error = nullptr, QStringList *folio_files = nullptr,
			  Pictures *pictures = nullptr, int *format = nullptr,
			  QStringList *lost = nullptr);
		/// What can be read of a damaged .qetz, and in @p lost what could not
	bool recover(const QByteArray &content, QDomDocument *project, QStringList *lost,
				 QString *error = nullptr);
		/// The file join() left the picture of @p image in, or an empty string
	QString pictureFile(const QDomElement &image);
		/// @p image, which names its picture's file (pictureFile()), given
		/// the picture itself, @p bytes, as a document read whole has it
	void inlinePicture(QDomElement image, const QByteArray &bytes);

		/// Take what QElectroTech computes out of a folio (format 4)
	void dropComputed(QDomElement diagram, bool terminal_lists);
		/// true if every folio, symbol and wire has a uuid of its own
	bool ownUuids(const QDomElement &project);
		/// Parse @p xml as a .qet is read (QETProject::openFile)
	bool parse(const QByteArray &xml, QDomDocument *document, QString *error = nullptr);
		/// true if @p content is a zip, so a .qetz rather than a .qet
	bool isZip(const QByteArray &content);
		/// The project document in @p content, a .qet or a .qetz
	bool readDocument(const QByteArray &content, QDomDocument *document,
					  QString *error = nullptr, Pictures *pictures = nullptr, int *format = nullptr);
}

#endif // QETCONTAINER_H
