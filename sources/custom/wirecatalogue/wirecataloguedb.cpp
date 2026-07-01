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
#include <QFile>
#include <QTextStream>

namespace {
	/// Ordered column list shared by the schema and the SELECT/INSERT helpers.
	const QString kColumns = QStringLiteral(
		"wire_id, manufacturer_name, manufacturer_part_no, supplier_name, "
		"supplier_part_no, cross_section_mm2, cable_outer_dia_mm, insulation_dia_mm, "
		"num_cores, core_colors, shield, shield_type, voltage_rating_v, "
		"temp_rating_c, flexible, color_primary, notes, family_name, core_sections");

	// Stored as a JSON array of arrays: [["White","Blue"],["Brown"]] — one
	// inner array per core (base + optional tracer colours).
	QString coresToJson(const QVector<QStringList> &cores)
	{
		QJsonArray arr;
		for (const QStringList &core : cores) {
			QJsonArray c;
			for (const QString &col : core)
				c.append(col);
			arr.append(c);
		}
		return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
	}

	QVector<QStringList> coresFromJson(const QString &json)
	{
		QVector<QStringList> out;
		const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
		if (!doc.isArray())
			return out;
		const QJsonArray arr = doc.array();
		for (const QJsonValue &v : arr) {
			if (v.isArray()) {                 // new nested format
				QStringList core;
				for (const QJsonValue &c : v.toArray())
					core << c.toString();
				out << core;
			} else if (v.isString()) {         // legacy flat format: one colour/core
				out << QStringList{v.toString()};
			}
		}
		return out;
	}

	QString sectionsToJson(const QVector<double> &sections)
	{
		QJsonArray arr;
		for (double s : sections)
			arr.append(s);
		return QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact));
	}

	QVector<double> sectionsFromJson(const QString &json)
	{
		QVector<double> out;
		const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
		if (doc.isArray())
			for (const QJsonValue &v : doc.array())
				out << v.toDouble();
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
		"  notes                TEXT,"
		"  family_name          TEXT,"
		"  core_sections        TEXT"          // JSON array of per-core sections
		")");
	if (!q.exec(ddl)) {
		setError(QStringLiteral("createSchema"), q);
		return false;
	}

	// Migrate older databases that predate family_name / core_sections.
	// ALTER TABLE ADD COLUMN is a no-op error if the column already exists.
	QSqlQuery alter(m_db);
	alter.exec(QStringLiteral("ALTER TABLE wire ADD COLUMN family_name TEXT"));
	alter.exec(QStringLiteral("ALTER TABLE wire ADD COLUMN core_sections TEXT"));

	seedIfEmpty();
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
	q.addBindValue(coresToJson(spec.coreColors));
	q.addBindValue(spec.hasShield ? 1 : 0);
	q.addBindValue(spec.shieldType);
	q.addBindValue(spec.voltageRatingV);
	q.addBindValue(spec.tempRatingC);
	q.addBindValue(spec.isFlexible ? 1 : 0);
	q.addBindValue(spec.colorPrimary);
	q.addBindValue(spec.notes);
	q.addBindValue(spec.familyName);
	q.addBindValue(sectionsToJson(spec.coreSections));
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
	s.coreColors         = coresFromJson(q.value(QStringLiteral("core_colors")).toString());
	s.hasShield          = q.value(QStringLiteral("shield")).toInt() != 0;
	s.shieldType         = q.value(QStringLiteral("shield_type")).toString();
	s.voltageRatingV     = q.value(QStringLiteral("voltage_rating_v")).toInt();
	s.tempRatingC        = q.value(QStringLiteral("temp_rating_c")).toInt();
	s.isFlexible         = q.value(QStringLiteral("flexible")).toInt() != 0;
	s.colorPrimary       = q.value(QStringLiteral("color_primary")).toString();
	s.notes              = q.value(QStringLiteral("notes")).toString();
	s.familyName         = q.value(QStringLiteral("family_name")).toString();
	s.coreSections       = sectionsFromJson(q.value(QStringLiteral("core_sections")).toString());
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
		"(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)").arg(kColumns));
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
		"(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)").arg(kColumns));
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

// --- Seed data ---------------------------------------------------------------

void WireCatalogueDb::seedIfEmpty()
{
	if (count() > 0)
		return;

	// A small starter set of common IEC single wires (H07V-K) + example cables.
	auto wire = [](const QString &id, const QString &colour, double mm2) {
		WireSpec s;
		s.wireId = id;
		s.manufacturerName = QStringLiteral("Generic");
		s.familyName = QStringLiteral("H07V-K");
		s.numCores = 1;
		s.crossSectionMm2 = mm2;
		s.colorPrimary = colour;
		s.coreColors = { QStringList{colour} };
		s.coreSections = { mm2 };
		s.voltageRatingV = 450;
		s.tempRatingC = 70;
		s.isFlexible = true;
		return s;
	};

	addWire(wire(QStringLiteral("WS-BK-1.5"),   QStringLiteral("Black"),        1.5));
	addWire(wire(QStringLiteral("WS-BU-1.5"),   QStringLiteral("Blue"),         1.5));
	addWire(wire(QStringLiteral("WS-BN-2.5"),   QStringLiteral("Brown"),        2.5));
	addWire(wire(QStringLiteral("WS-GNYE-2.5"), QStringLiteral("Green-Yellow"), 2.5));
	addWire(wire(QStringLiteral("WS-BK-6"),     QStringLiteral("Black"),        6.0));

	// Example 4-core control cable (ÖLFLEX-style), uniform 0.5 mm².
	WireSpec cable;
	cable.wireId = QStringLiteral("CS-4G0.5");
	cable.manufacturerName = QStringLiteral("Generic");
	cable.familyName = QStringLiteral("Control_Cable");
	cable.numCores = 4;
	cable.crossSectionMm2 = 0.5;
	cable.coreColors = { {QStringLiteral("Brown")}, {QStringLiteral("Black")},
						 {QStringLiteral("Grey")},  {QStringLiteral("Green-Yellow")} };
	cable.coreSections = { 0.5, 0.5, 0.5, 0.5 };
	cable.hasShield = true;
	cable.shieldType = QStringLiteral("Braid");
	cable.voltageRatingV = 300;
	cable.tempRatingC = 80;
	cable.isFlexible = true;
	addWire(cable);
}

// --- CSV import / export -----------------------------------------------------

namespace {
	const QStringList kCsvHeader = {
		QStringLiteral("wire_id"), QStringLiteral("manufacturer_name"),
		QStringLiteral("manufacturer_part_no"), QStringLiteral("supplier_name"),
		QStringLiteral("supplier_part_no"), QStringLiteral("family_name"),
		QStringLiteral("cross_section_mm2"), QStringLiteral("cable_outer_dia_mm"),
		QStringLiteral("insulation_dia_mm"), QStringLiteral("num_cores"),
		QStringLiteral("core_colors"), QStringLiteral("core_sections"),
		QStringLiteral("shield"), QStringLiteral("shield_type"),
		QStringLiteral("voltage_rating_v"), QStringLiteral("temp_rating_c"),
		QStringLiteral("flexible"), QStringLiteral("color_primary"),
		QStringLiteral("notes") };

	QString csvEscape(const QString &field)
	{
		QString f = field;
		if (f.contains(QLatin1Char(',')) || f.contains(QLatin1Char('"'))
			|| f.contains(QLatin1Char('\n'))) {
			f.replace(QLatin1Char('"'), QStringLiteral("\"\""));
			return QLatin1Char('"') + f + QLatin1Char('"');
		}
		return f;
	}

	// Split one CSV line into fields, honouring "quoted, fields" with "" escapes.
	QStringList csvSplit(const QString &line)
	{
		QStringList out;
		QString cur;
		bool in_quotes = false;
		for (int i = 0; i < line.size(); ++i) {
			const QChar c = line.at(i);
			if (in_quotes) {
				if (c == QLatin1Char('"')) {
					if (i + 1 < line.size() && line.at(i + 1) == QLatin1Char('"')) {
						cur += QLatin1Char('"'); ++i;
					} else {
						in_quotes = false;
					}
				} else {
					cur += c;
				}
			} else if (c == QLatin1Char('"')) {
				in_quotes = true;
			} else if (c == QLatin1Char(',')) {
				out << cur; cur.clear();
			} else {
				cur += c;
			}
		}
		out << cur;
		return out;
	}
}

int WireCatalogueDb::exportCsv(const QString &filePath) const
{
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		const_cast<WireCatalogueDb*>(this)->m_last_error =
			QStringLiteral("exportCsv: cannot write %1").arg(filePath);
		return -1;
	}
	QTextStream ts(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	ts.setCodec("UTF-8");
#endif

	ts << kCsvHeader.join(QLatin1Char(',')) << '\n';
	int n = 0;
	for (const WireSpec &w : allWires()) {
		QStringList row;
		row << w.wireId << w.manufacturerName << w.manufacturerPartNo
			<< w.supplierName << w.supplierPartNo << w.familyName
			<< QString::number(w.crossSectionMm2) << QString::number(w.outerDiaMm)
			<< QString::number(w.insulationDiaMm) << QString::number(w.numCores)
			<< coresToJson(w.coreColors) << sectionsToJson(w.coreSections)
			<< (w.hasShield ? QStringLiteral("1") : QStringLiteral("0"))
			<< w.shieldType << QString::number(w.voltageRatingV)
			<< QString::number(w.tempRatingC)
			<< (w.isFlexible ? QStringLiteral("1") : QStringLiteral("0"))
			<< w.colorPrimary << w.notes;
		QStringList esc;
		for (const QString &f : row)
			esc << csvEscape(f);
		ts << esc.join(QLatin1Char(',')) << '\n';
		++n;
	}
	return n;
}

int WireCatalogueDb::importCsv(const QString &filePath)
{
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		m_last_error = QStringLiteral("importCsv: cannot read %1").arg(filePath);
		return -1;
	}
	QTextStream ts(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	ts.setCodec("UTF-8");
#endif

	QStringList header;
	int n = 0;
	bool first = true;
	while (!ts.atEnd()) {
		const QString line = ts.readLine();
		if (line.isEmpty())
			continue;
		const QStringList fields = csvSplit(line);
		if (first) {
			header = fields;
			first = false;
			continue;
		}
		QHash<QString, QString> m;
		for (int i = 0; i < header.size() && i < fields.size(); ++i)
			m.insert(header.at(i).trimmed(), fields.at(i));

		WireSpec s;
		s.wireId             = m.value(QStringLiteral("wire_id")).trimmed();
		if (s.wireId.isEmpty())
			continue;
		s.manufacturerName   = m.value(QStringLiteral("manufacturer_name"));
		s.manufacturerPartNo = m.value(QStringLiteral("manufacturer_part_no"));
		s.supplierName       = m.value(QStringLiteral("supplier_name"));
		s.supplierPartNo     = m.value(QStringLiteral("supplier_part_no"));
		s.familyName         = m.value(QStringLiteral("family_name"));
		s.crossSectionMm2    = m.value(QStringLiteral("cross_section_mm2")).toDouble();
		s.outerDiaMm         = m.value(QStringLiteral("cable_outer_dia_mm")).toDouble();
		s.insulationDiaMm    = m.value(QStringLiteral("insulation_dia_mm")).toDouble();
		s.numCores           = m.value(QStringLiteral("num_cores")).toInt();
		s.coreColors         = coresFromJson(m.value(QStringLiteral("core_colors")));
		s.coreSections       = sectionsFromJson(m.value(QStringLiteral("core_sections")));
		s.hasShield          = m.value(QStringLiteral("shield")).trimmed() == QLatin1String("1");
		s.shieldType         = m.value(QStringLiteral("shield_type"));
		s.voltageRatingV     = m.value(QStringLiteral("voltage_rating_v")).toInt();
		s.tempRatingC        = m.value(QStringLiteral("temp_rating_c")).toInt();
		s.isFlexible         = m.value(QStringLiteral("flexible")).trimmed() == QLatin1String("1");
		s.colorPrimary       = m.value(QStringLiteral("color_primary"));
		s.notes              = m.value(QStringLiteral("notes"));
		if (s.numCores < 1) s.numCores = qMax(1, s.coreColors.size());

		if (upsertWire(s))
			++n;
	}
	return n;
}
