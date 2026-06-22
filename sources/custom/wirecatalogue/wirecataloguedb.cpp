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
#include "wirecataloguedb.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>
#include <QVariant>
#include <QStandardPaths>
#include <QDir>

namespace {
	/// Ordered column list shared by the schema and the SELECT/INSERT helpers.
	const QString kColumns = QStringLiteral(
		"wire_id, manufacturer_name, manufacturer_part_no, supplier_name, "
		"supplier_part_no, cross_section_mm2, cable_outer_dia_mm, insulation_dia_mm, "
		"num_cores, core_colors, shield, shield_type, voltage_rating_v, "
		"temp_rating_c, flexible, color_primary, notes");

	QString colorsToJson(const QStringList &colors)
	{
		QJsonArray arr;
		for (const QString &c : colors)
			arr.append(c);
		return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
	}

	QStringList colorsFromJson(const QString &json)
	{
		QStringList out;
		const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
		if (doc.isArray()) {
			const QJsonArray arr = doc.array();
			for (const QJsonValue &v : arr)
				out << v.toString();
		}
		return out;
	}
}

WireCatalogueDb::WireCatalogueDb(QObject *parent) :
	QObject(parent),
	m_connection_name(QStringLiteral("wirecatalogue_") + QUuid::createUuid().toString())
{}

WireCatalogueDb::~WireCatalogueDb()
{
	if (m_db.isOpen())
		m_db.close();
	m_db = QSqlDatabase();
	QSqlDatabase::removeDatabase(m_connection_name);
}

QString WireCatalogueDb::defaultPath()
{
	const QString dir = QStandardPaths::writableLocation(
		QStandardPaths::AppDataLocation);
	QDir().mkpath(dir);
	return dir + QStringLiteral("/wirecatalogue.sqlite");
}

bool WireCatalogueDb::open(const QString &filePath)
{
	if (m_db.isOpen())
		m_db.close();

	m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connection_name);
	m_db.setDatabaseName(filePath);
	if (!m_db.open()) {
		m_last_error = m_db.lastError().text();
		return false;
	}
	// Enforce TEXT PK uniqueness and reasonable durability for a desktop file.
	QSqlQuery pragma(m_db);
	pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
	return createSchema();
}

bool WireCatalogueDb::isOpen() const
{
	return m_db.isOpen();
}

bool WireCatalogueDb::createSchema()
{
	QSqlQuery q(m_db);
	const QString ddl = QStringLiteral(
		"CREATE TABLE IF NOT EXISTS wire ("
		"  wire_id              TEXT PRIMARY KEY,"
		"  manufacturer_name    TEXT,"
		"  manufacturer_part_no TEXT,"
		"  supplier_name        TEXT,"
		"  supplier_part_no     TEXT,"
		"  cross_section_mm2    REAL,"
		"  cable_outer_dia_mm   REAL,"
		"  insulation_dia_mm    REAL,"
		"  num_cores            INTEGER DEFAULT 1,"
		"  core_colors          TEXT,"          // JSON array of IEC 60757 names
		"  shield               INTEGER DEFAULT 0,"
		"  shield_type          TEXT,"
		"  voltage_rating_v     INTEGER DEFAULT 0,"
		"  temp_rating_c        INTEGER DEFAULT 0,"
		"  flexible             INTEGER DEFAULT 0,"
		"  color_primary        TEXT,"
		"  notes                TEXT"
		")");
	if (!q.exec(ddl)) {
		setError(QStringLiteral("createSchema"), q);
		return false;
	}
	return true;
}

void WireCatalogueDb::bindSpec(QSqlQuery &q, const WireSpec &spec) const
{
	q.addBindValue(spec.wireId);
	q.addBindValue(spec.manufacturerName);
	q.addBindValue(spec.manufacturerPartNo);
	q.addBindValue(spec.supplierName);
	q.addBindValue(spec.supplierPartNo);
	q.addBindValue(spec.crossSectionMm2);
	q.addBindValue(spec.outerDiaMm);
	q.addBindValue(spec.insulationDiaMm);
	q.addBindValue(spec.numCores);
	q.addBindValue(colorsToJson(spec.coreColors));
	q.addBindValue(spec.hasShield ? 1 : 0);
	q.addBindValue(spec.shieldType);
	q.addBindValue(spec.voltageRatingV);
	q.addBindValue(spec.tempRatingC);
	q.addBindValue(spec.isFlexible ? 1 : 0);
	q.addBindValue(spec.colorPrimary);
	q.addBindValue(spec.notes);
}

WireSpec WireCatalogueDb::specFromQuery(const QSqlQuery &q)
{
	WireSpec s;
	s.wireId             = q.value(QStringLiteral("wire_id")).toString();
	s.manufacturerName   = q.value(QStringLiteral("manufacturer_name")).toString();
	s.manufacturerPartNo = q.value(QStringLiteral("manufacturer_part_no")).toString();
	s.supplierName       = q.value(QStringLiteral("supplier_name")).toString();
	s.supplierPartNo     = q.value(QStringLiteral("supplier_part_no")).toString();
	s.crossSectionMm2    = q.value(QStringLiteral("cross_section_mm2")).toDouble();
	s.outerDiaMm         = q.value(QStringLiteral("cable_outer_dia_mm")).toDouble();
	s.insulationDiaMm    = q.value(QStringLiteral("insulation_dia_mm")).toDouble();
	s.numCores           = q.value(QStringLiteral("num_cores")).toInt();
	s.coreColors         = colorsFromJson(q.value(QStringLiteral("core_colors")).toString());
	s.hasShield          = q.value(QStringLiteral("shield")).toInt() != 0;
	s.shieldType         = q.value(QStringLiteral("shield_type")).toString();
	s.voltageRatingV     = q.value(QStringLiteral("voltage_rating_v")).toInt();
	s.tempRatingC        = q.value(QStringLiteral("temp_rating_c")).toInt();
	s.isFlexible         = q.value(QStringLiteral("flexible")).toInt() != 0;
	s.colorPrimary       = q.value(QStringLiteral("color_primary")).toString();
	s.notes              = q.value(QStringLiteral("notes")).toString();
	return s;
}

bool WireCatalogueDb::addWire(const WireSpec &spec)
{
	if (!spec.isValid()) {
		m_last_error = QStringLiteral("addWire: empty wire_id");
		return false;
	}
	QSqlQuery q(m_db);
	q.prepare(QStringLiteral("INSERT INTO wire (%1) VALUES "
		"(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)").arg(kColumns));
	bindSpec(q, spec);
	if (!q.exec()) {
		setError(QStringLiteral("addWire"), q);
		return false;
	}
	emit catalogueChanged();
	return true;
}

bool WireCatalogueDb::upsertWire(const WireSpec &spec)
{
	if (!spec.isValid()) {
		m_last_error = QStringLiteral("upsertWire: empty wire_id");
		return false;
	}
	QSqlQuery q(m_db);
	q.prepare(QStringLiteral("INSERT OR REPLACE INTO wire (%1) VALUES "
		"(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)").arg(kColumns));
	bindSpec(q, spec);
	if (!q.exec()) {
		setError(QStringLiteral("upsertWire"), q);
		return false;
	}
	emit catalogueChanged();
	return true;
}

bool WireCatalogueDb::updateWire(const WireSpec &spec)
{
	if (!contains(spec.wireId)) {
		m_last_error = QStringLiteral("updateWire: unknown wire_id %1").arg(spec.wireId);
		return false;
	}
	return upsertWire(spec);
}

bool WireCatalogueDb::removeWire(const QString &wireId)
{
	QSqlQuery q(m_db);
	q.prepare(QStringLiteral("DELETE FROM wire WHERE wire_id = ?"));
	q.addBindValue(wireId);
	if (!q.exec()) {
		setError(QStringLiteral("removeWire"), q);
		return false;
	}
	emit catalogueChanged();
	return true;
}

bool WireCatalogueDb::contains(const QString &wireId) const
{
	QSqlQuery q(m_db);
	q.prepare(QStringLiteral("SELECT 1 FROM wire WHERE wire_id = ? LIMIT 1"));
	q.addBindValue(wireId);
	return q.exec() && q.next();
}

WireSpec WireCatalogueDb::wire(const QString &wireId) const
{
	QSqlQuery q(m_db);
	q.prepare(QStringLiteral("SELECT %1 FROM wire WHERE wire_id = ?").arg(kColumns));
	q.addBindValue(wireId);
	if (q.exec() && q.next())
		return specFromQuery(q);
	return WireSpec();
}

QVector<WireSpec> WireCatalogueDb::allWires() const
{
	QVector<WireSpec> out;
	QSqlQuery q(m_db);
	if (q.exec(QStringLiteral("SELECT %1 FROM wire ORDER BY wire_id").arg(kColumns))) {
		while (q.next())
			out << specFromQuery(q);
	}
	return out;
}

QVector<WireSpec> WireCatalogueDb::search(const QString &text) const
{
	if (text.trimmed().isEmpty())
		return allWires();

	QVector<WireSpec> out;
	QSqlQuery q(m_db);
	q.prepare(QStringLiteral(
		"SELECT %1 FROM wire WHERE "
		"  wire_id              LIKE :p OR "
		"  manufacturer_name    LIKE :p OR "
		"  manufacturer_part_no LIKE :p OR "
		"  supplier_name        LIKE :p OR "
		"  supplier_part_no     LIKE :p OR "
		"  color_primary        LIKE :p "
		"ORDER BY wire_id").arg(kColumns));
	q.bindValue(QStringLiteral(":p"), QStringLiteral("%%%1%%").arg(text.trimmed()));
	if (q.exec()) {
		while (q.next())
			out << specFromQuery(q);
	}
	return out;
}

int WireCatalogueDb::count() const
{
	QSqlQuery q(m_db);
	if (q.exec(QStringLiteral("SELECT COUNT(*) FROM wire")) && q.next())
		return q.value(0).toInt();
	return 0;
}

void WireCatalogueDb::setError(const QString &context, const QSqlQuery &q)
{
	m_last_error = QStringLiteral("%1: %2").arg(context, q.lastError().text());
}
