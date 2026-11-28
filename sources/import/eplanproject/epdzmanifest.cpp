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
#include "epdzmanifest.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QHash>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QUuid>

QString EpdzManifest::bare(QString value)
{
	value = value.trimmed();
	while (!value.isEmpty() && (value.at(0) == QLatin1Char('=') || value.at(0) == QLatin1Char('+')
								|| value.at(0) == QLatin1Char('#') || value.at(0) == QLatin1Char('&')))
		value.remove(0, 1);
	return value.trimmed();
}

QString EpdzManifest::csvCell(QString text)
{
	text.replace(QLatin1Char('\n'), QLatin1Char(' ')).replace(QLatin1Char('\r'), QLatin1Char(' '));
	if (text.contains(QLatin1Char(';')) || text.contains(QLatin1Char('"'))) {
		text.replace(QStringLiteral("\""), QStringLiteral("\"\""));
		return QLatin1Char('"') + text + QLatin1Char('"');
	}
	return text;
}

bool EpdzManifest::read(const QString &folder, Manifest &manifest, QString &error)
{
	manifest = Manifest();
	error.clear();
		// every file of the archive by its name (the manifest gives names only)
	QHash<QString, QString> files;
	for (QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories); it.hasNext();) {
		const QString f = it.next();
		files.insert(QFileInfo(f).fileName().toLower(), f);
	}
	const QString database = files.value(QStringLiteral("manifest.db"));
	if (database.isEmpty()) {
		error = QCoreApplication::translate("EpdzImport", "The archive holds no manifest.db: it is no EPLAN export archive");
		return false;
	}

	const QString connection = QStringLiteral("epdz_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
	{
		QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
		db.setDatabaseName(database);
		db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
		if (!db.open()) {
			error = QCoreApplication::translate("EpdzImport", "manifest.db cannot be read");
		} else {
			auto query = [&](const QString &sql) {QSqlQuery q(db); q.exec(sql); return q;};
			QSqlQuery q = query(QStringLiteral("SELECT name FROM package WHERE type='project' LIMIT 1"));
			if (q.next()) manifest.project_name = q.value(0).toString();

			QHash<qint64, QStringList> locators;
			q = query(QStringLiteral("SELECT packageid, locator FROM item"));
			while (q.next()) locators[q.value(0).toLongLong()] << q.value(1).toString();

				// 11000 page counter, 11011 page description
			QHash<qint64, QString> counter, description;
			q = query(QStringLiteral("SELECT packageid, propid, value FROM property WHERE propid IN (11000, 11011) "
									 "AND value IS NOT NULL AND value != ''"));
			while (q.next()) {
				if (q.value(1).toInt() == 11000) counter.insert(q.value(0).toLongLong(), q.value(2).toString());
				else description.insert(q.value(0).toLongLong(), q.value(2).toString());
			}

				// the columns this manifest has (older ones lack some)
			QSet<QString> columns;
			q = query(QStringLiteral("PRAGMA table_info(page_package)"));
			while (q.next()) columns << q.value(1).toString().toLower();
			auto col = [&](const char *name) {
				return columns.contains(QString::fromLatin1(name))
					? QStringLiteral("COALESCE(%1,'')").arg(QString::fromLatin1(name)) : QStringLiteral("''");
			};
				// ep1340 higher-level function, ep1440 plant, ep1140 site,
				// ep1240 location, ep1640 page number
			q = query(QStringLiteral("SELECT packageid, name, %1, %2, %3, %4, %5 FROM page_package")
						  .arg(col("ep1340"), col("ep1440"), col("ep1140"), col("ep1240"), col("ep1640")));
			while (q.next()) {
				Page p;
				p.id = q.value(0).toLongLong();
				p.name = q.value(1).toString();
				const QString higher = bare(q.value(2).toString()), plant = bare(q.value(3).toString());
				const QString site = bare(q.value(4).toString()), location = bare(q.value(5).toString());
				p.plant = plant.isEmpty() ? higher : plant;
				p.location = location.isEmpty() ? site : location;
				p.folio = bare(q.value(6).toString());
				if (p.folio.isEmpty()) p.folio = counter.value(p.id);
				p.title = description.value(p.id);
				if (p.title.isEmpty()) p.title = p.name;
				for (const QString &l : locators.value(p.id))
					if (l.endsWith(QLatin1String(".svg"), Qt::CaseInsensitive)) {
						p.svg = files.value(QFileInfo(l).fileName().toLower());
						if (QFileInfo(l).completeBaseName().compare(p.name, Qt::CaseInsensitive) == 0) break;
					}
				manifest.pages << p;
			}

			auto first = [](int id) {
				return QStringLiteral("(SELECT p.value FROM property p WHERE p.packageid = k.id AND p.propid = %1 "
									  "AND p.value != '' ORDER BY p.propindex LIMIT 1)").arg(id);
			};
				// the functions: 20001 device tag, 20100 article, 20193 designation
			q = query(QStringLiteral("SELECT k.name, %1, %2, %3 FROM package k WHERE k.type = 'function'")
						  .arg(first(20001), first(20100), first(20193)));
			while (q.next())
				manifest.devices << Device{q.value(0).toString(), q.value(1).toString(),
										   q.value(2).toString(), q.value(3).toString()};

				// the connections: 31019 source, 31020 target, 33003 or 20045
				// potential, 31004 colour, 31007 section, 31003 length
			q = query(QStringLiteral("SELECT k.name, %1, %2, %3, %4, %5, %6, %7 FROM package k WHERE k.type = 'mergedconnection'")
						  .arg(first(31019), first(31020), first(33003), first(20045), first(31004), first(31007), first(31003)));
			while (q.next()) {
				const QString potential = q.value(3).toString().isEmpty() ? q.value(4).toString() : q.value(3).toString();
				manifest.connections << Connection{q.value(1).toString(), q.value(2).toString(), potential,
												   q.value(5).toString(), q.value(6).toString(),
												   q.value(7).toString()};
			}

			q = query(QStringLiteral("SELECT packageid, name FROM installationspace_package"));
			while (q.next())
				for (const QString &l : locators.value(q.value(0).toLongLong()))
					if (l.endsWith(QLatin1String(".e3d"), Qt::CaseInsensitive))
						manifest.spaces << qMakePair(q.value(1).toString(), files.value(QFileInfo(l).fileName().toLower()));
			db.close();
		}
	}
	QSqlDatabase::removeDatabase(connection);
	if (!error.isEmpty()) return false;
	if (manifest.pages.isEmpty()) {
		error = QCoreApplication::translate("EpdzImport", "The manifest lists no page");
		return false;
	}
	return true;
}
