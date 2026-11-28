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
#ifndef EPLANPARTIMPORTER_H
#define EPLANPARTIMPORTER_H

#include <QDomDocument>
#include <QMap>
#include <QString>
#include <QStringList>

/**
	@brief Imports manufacturer EPLAN DXF data as QET elements.

	Three sources:
	- an EPLAN Data Portal DXF package (MAC_<part>.zip or its unpacked
	  folder): commercialdata.csv with the part data and one DXF per
	  variant and representation; every variant becomes an element;
	- a single EPLAN DXF, e.g. a manufacturer download; a drawing with
	  black-box frames (one per part of a multi-part device) gives one
	  element per frame. Part data may come from the matching .edz;
	- an EPLAN part (.edz): a generic element from its part data and,
	  where the part carries the manufacturer's EPLAN macro, one element
	  per schematic variant and its panel layout.

	Elements are written to <collection>/<manufacturer>/<category>/<type>/,
	each folder with a qet_directory file. UI-decoupled like EdzImporter.
*/
class EplanPartImporter
{
	public:
		bool importPackage(const QString &path, const QString &category,
				   const QString &collection_dir);
		bool importDxf(const QString &dxf_path, const QString &edz_path,
			       const QString &manufacturer, const QString &category,
			       const QString &type, const QString &collection_dir);
		/// an EPLAN part (.edz) as a generic element, in the same folder layout
		bool importEdz(const QString &edz_path, const QString &manufacturer,
			       const QString &category, const QString &type,
			       const QString &collection_dir);
		/// display name of a manufacturer ("SIE" -> "Siemens")
		static QString manufacturerName(const QString &name);

		/// part data (commercialdata.csv columns) of a package, for the dialog
		static QMap<QString, QString> packageInfo(const QString &path,
							  QString *error = nullptr,
							  const QString &macro = QString());

		/**
			An element file that already exists is overwritten only while it
			is as the import wrote it (ElementFingerprint); one edited since,
			or one without fingerprint, is kept and named in messages() and
			keptPaths(). @p force overwrites it all the same. Either way the
			uuids, names and element information of the old file stay.
		*/
		void setForce(bool force) { m_force = force; }
		QStringList writtenPaths() const { return m_written; }
		/// existing elements left as they are (edited since their import)
		QStringList keptPaths() const { return m_kept; }
		QStringList messages() const { return m_messages; }
		QString errorString() const { return m_error; }

	private:
		enum class Stored { Written, Kept, Failed };
		Stored store(QDomDocument &doc, const QString &path, int indent = 4);
		void clear();
		bool finish();

		bool m_force = false;
		QStringList m_written;
		QStringList m_kept;
		QStringList m_from_macro;	///< elements with the channel table of their EPLAN macro
		QStringList m_messages;
		QString m_error;
};

#endif // EPLANPARTIMPORTER_H
