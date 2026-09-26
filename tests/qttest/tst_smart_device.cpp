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
#include "bomexport.h"

#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

class SmartDeviceTest : public QObject
{
	Q_OBJECT

	private slots:
		void metadataColumns();
		void defaultQueryAndCsv();
		void atomicWrite();
};

void SmartDeviceTest::metadataColumns()
{
	const auto columns = BomExport::defaultColumns();
	for (const QString &column : {
			 QStringLiteral("label"), QStringLiteral("manufacturer"),
			 QStringLiteral("manufacturer_reference"), QStringLiteral("model"),
			 QStringLiteral("description"), QStringLiteral("category"),
			 QStringLiteral("quantity"), QStringLiteral("voltage_rating"),
			 QStringLiteral("current_rating"), QStringLiteral("folio"),
			 QStringLiteral("notes")})
	{
		QVERIFY2(columns.contains(column), qPrintable(column));
	}
}

void SmartDeviceTest::defaultQueryAndCsv()
{
	const QString connection = QStringLiteral("smart-device-%1")
			.arg(QUuid::createUuid().toString());
	{
		auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
		db.setDatabaseName(QStringLiteral(":memory:"));
		QVERIFY2(db.open(), qPrintable(db.lastError().text()));
		QSqlQuery setup(db);
		QVERIFY(setup.exec(QStringLiteral(
			"CREATE TABLE element_nomenclature_view ("
			"label TEXT, designation TEXT, manufacturer TEXT, "
			"manufacturer_reference TEXT, model TEXT, description TEXT, "
			"category TEXT, quantity TEXT, voltage_rating TEXT, "
			"current_rating TEXT, folio TEXT, notes TEXT, "
			"element_type TEXT, diagram_position INTEGER, position INTEGER)")));

		QSqlQuery insert(db);
		insert.prepare(QStringLiteral(
			"INSERT INTO element_nomenclature_view VALUES "
			"(?, '', ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
		auto add = [&insert](const QString &tag, const QString &type, int page,
						 int position, const QString &manufacturer) {
			insert.bindValue(0, tag);
			insert.bindValue(1, manufacturer);
			insert.bindValue(2, QStringLiteral("0012"));
			insert.bindValue(3, QStringLiteral("M-1"));
			insert.bindValue(4, QStringLiteral("Power contactor"));
			insert.bindValue(5, QStringLiteral("Control"));
			insert.bindValue(6, QStringLiteral("1"));
			insert.bindValue(7, QStringLiteral("24 V DC"));
			insert.bindValue(8, QStringLiteral("16 A"));
			insert.bindValue(9, QString::number(page));
			insert.bindValue(10, QStringLiteral("Quoted \"note\"\nnext line"));
			insert.bindValue(11, type);
			insert.bindValue(12, page);
			insert.bindValue(13, position);
			return insert.exec();
		};
		QVERIFY(add(QStringLiteral("-K2"), QStringLiteral("simple"), 2, 1,
					QStringLiteral("日本電機")));
		QVERIFY(add(QStringLiteral("-K1"), QStringLiteral("master"), 1, 2,
					QStringLiteral("Müller")));
		//A slave and a terminal are both separately orderable hardware -- an
		//auxiliary contact block has its own order code, and so does a
		//terminal block -- so both belong in the bill of materials. See
		//discussion #847. Anything that should not be ordered is kept out by
		//exclude_from_bom rather than by its base type.
		QVERIFY(add(QStringLiteral("-K1.1"), QStringLiteral("slave"), 1, 3,
					QStringLiteral("Aux contact block")));
		QVERIFY(add(QStringLiteral("X1"), QStringLiteral("terminal"), 1, 4,
					QStringLiteral("Terminal block")));
		//Still filtered out: a folio report arrow is not hardware.
		QVERIFY(add(QStringLiteral(">1"), QStringLiteral("next_report"), 1, 5,
					QStringLiteral("Must not be exported")));

		QSqlQuery query(db);
		QVERIFY2(query.exec(BomExport::defaultQuery()),
				 qPrintable(query.lastError().text()));
		int rows = 0;
		const auto csv = BomExport::toCsv(
				query, BomExport::defaultColumns(), true, &rows);
		QCOMPARE(rows, 4);
		QVERIFY(csv.startsWith("\xEF\xBB\xBF\"label\";\"designation\";"));
		QVERIFY(csv.contains(QStringLiteral("Müller").toUtf8()));
		QVERIFY(csv.contains(QStringLiteral("日本電機").toUtf8()));
		QVERIFY(csv.contains("\"Quoted \"\"note\"\"\nnext line\""));
		QVERIFY(csv.contains("Aux contact block"));
		QVERIFY(csv.contains("Terminal block"));
		QVERIFY(!csv.contains("Must not be exported"));
		QVERIFY(csv.indexOf("-K1") < csv.indexOf("-K2"));
	}
	QSqlDatabase::removeDatabase(connection);
}

void SmartDeviceTest::atomicWrite()
{
	QTemporaryDir temporary;
	QVERIFY(temporary.isValid());
	const auto path = temporary.filePath(QStringLiteral("bom.csv"));
	const QByteArray csv("\xEF\xBB\xBF\"label\"\n");
	QString error = QStringLiteral("stale");
	QVERIFY(BomExport::writeCsv(path, csv, &error));
	QVERIFY(error.isEmpty());
	QFile file(path);
	QVERIFY(file.open(QIODevice::ReadOnly));
	QCOMPARE(file.readAll(), csv);
	QVERIFY(!BomExport::writeCsv(
			temporary.filePath(QStringLiteral("missing/bom.csv")), csv, &error));
	QVERIFY(!error.isEmpty());
}

QTEST_GUILESS_MAIN(SmartDeviceTest)
#include "tst_smart_device.moc"
