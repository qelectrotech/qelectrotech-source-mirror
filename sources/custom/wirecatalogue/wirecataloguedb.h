/*
	Copyright 2026 Trovo Tech Solutions
	This file is part of a custom feature set built on QElectroTech.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef WIRECATALOGUEDB_H
#define WIRECATALOGUEDB_H

#include "wirespec.h"

#include <QObject>
#include <QSqlDatabase>
#include <QVector>

/**
	@brief The WireCatalogueDb class
	SQLite-backed store for the wire / cable catalogue. Owns one named
	QSqlDatabase connection and exposes CRUD plus a simple text filter.

	Uses Qt's QSQLITE driver (Qt::Sql is already a QET dependency). The
	catalogue lives in its own database file so it stays isolated from QET's
	per-project database (sources/dataBase/projectdatabase.*).
*/
class WireCatalogueDb : public QObject
{
	Q_OBJECT

	public:
		explicit WireCatalogueDb(QObject *parent = nullptr);
		~WireCatalogueDb() override;

		/// Default shared catalogue file under the user's app-data location.
		static QString defaultPath();

		/// Open (creating if needed) the catalogue at the given file path.
		bool open(const QString &filePath);
		bool isOpen() const;
		QString lastError() const { return m_last_error; }

		// --- CRUD ---
		bool addWire   (const WireSpec &spec);   ///< INSERT (fails if wireId exists)
		bool updateWire(const WireSpec &spec);   ///< UPDATE by wireId
		bool upsertWire(const WireSpec &spec);   ///< INSERT OR REPLACE
		bool removeWire(const QString &wireId);
		bool contains  (const QString &wireId) const;

		WireSpec          wire(const QString &wireId) const;
		QVector<WireSpec> allWires() const;
		/// Case-insensitive match on id / manufacturer / part numbers / colour.
		QVector<WireSpec> search(const QString &text) const;
		int count() const;

		// --- CSV import / export ---
		/// Write the whole catalogue to a CSV file. Returns rows written, -1 on error.
		int exportCsv(const QString &filePath) const;
		/// Import wires from CSV (INSERT OR REPLACE). Returns rows imported, -1 on error.
		int importCsv(const QString &filePath);

	signals:
		void catalogueChanged();

	private:
		bool createSchema();
		void seedIfEmpty();   ///< Populate common IEC wires/cables when empty.
		static WireSpec specFromQuery(const class QSqlQuery &q);
		void bindSpec(class QSqlQuery &q, const WireSpec &spec) const;
		void setError(const QString &context, const class QSqlQuery &q);

	private:
		QSqlDatabase m_db;
		QString      m_connection_name;
		QString      m_last_error;
};

#endif // WIRECATALOGUEDB_H
