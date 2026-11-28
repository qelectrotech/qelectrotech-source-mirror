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
#include "import/eplanproject/epdzmanifest.h"
#include "import/eplanproject/eplanprojectimport.h"

#include <QDir>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

// File > Import EPLAN project: how the dialog calls the import program and
// reads its output, and what the manifest of an EPLAN export archive (.epdz)
// gives. The manifest is a small SQLite database made here, with the tables
// and EPLAN property numbers an export writes; no real project is involved.
class tst_eplanprojectimport : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	static void exec(QSqlDatabase &db, const QString &sql)
	{
		QSqlQuery q(db);
		QVERIFY2(q.exec(sql), qPrintable(sql));
	}

	// an unpacked archive: manifest.db, one page drawing, one .e3d
	QString makeExport()
	{
		const QString folder = m_dir.filePath(QStringLiteral("export"));
		QDir().mkpath(folder + QStringLiteral("/packages/pages/items/pagesvg"));
		QDir().mkpath(folder + QStringLiteral("/packages/installationspaces/items/installationspacee3d"));
		QFile svg(folder + QStringLiteral("/packages/pages/items/pagesvg/1.svg"));
		if (svg.open(QIODevice::WriteOnly))
			svg.write("<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 420 297\"/>");
		svg.close();
		QFile e3d(folder + QStringLiteral("/packages/installationspaces/items/installationspacee3d/space.e3d"));
		if (e3d.open(QIODevice::WriteOnly)) e3d.write("E3D");
		e3d.close();
		{
			QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("make"));
			db.setDatabaseName(folder + QStringLiteral("/manifest.db"));
			if (!db.open()) return QString();
			exec(db, QStringLiteral("CREATE TABLE package (id INTEGER, name TEXT, type TEXT)"));
			exec(db, QStringLiteral("CREATE TABLE property (packageid INTEGER, propid INTEGER, propindex INTEGER, value TEXT)"));
			exec(db, QStringLiteral("CREATE TABLE item (packageid INTEGER, locator TEXT)"));
			exec(db, QStringLiteral("CREATE TABLE page_package (packageid INTEGER, name TEXT, ep1440 TEXT, ep1240 TEXT)"));
			exec(db, QStringLiteral("CREATE TABLE installationspace_package (packageid INTEGER, name TEXT)"));
			exec(db, QStringLiteral("INSERT INTO package VALUES (1, 'Test project', 'project'), (2, '1', 'page'), "
									"(10, '-K1', 'function'), (20, 'W1', 'mergedconnection'), (30, 'Cabinet', 'installationspace')"));
			exec(db, QStringLiteral("INSERT INTO property VALUES (2, 11000, 0, '7'), (2, 11011, 0, 'Control'), "
									"(10, 20001, 0, '-K1'), (10, 20100, 0, 'SIE.3RT2015'), (10, 20193, 0, 'Contactor; 24 V'), "
									"(20, 31019, 0, '-K1:A1'), (20, 31020, 0, '-S1:13'), (20, 20045, 0, 'L1'), "
									"(20, 31004, 0, 'BK'), (20, 31007, 0, '1.5'), (20, 31003, 0, '0.4 m')"));
			exec(db, QStringLiteral("INSERT INTO page_package VALUES (2, '1', '==A1', '+S1')"));
			exec(db, QStringLiteral("INSERT INTO item VALUES (2, 'packages/pages/items/pagesvg/1.svg'), "
									"(30, 'packages/installationspaces/items/installationspacee3d/space.e3d')"));
			exec(db, QStringLiteral("INSERT INTO installationspace_package VALUES (30, 'Cabinet')"));
			db.close();
		}
		QSqlDatabase::removeDatabase(QStringLiteral("make"));
		return folder;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
	}

	void manifest()
	{
		const QString folder = makeExport();
		QVERIFY(!folder.isEmpty());
		EpdzManifest::Manifest m;
		QString error;
		QVERIFY2(EpdzManifest::read(folder, m, error), qPrintable(error));
		QCOMPARE(m.project_name, QStringLiteral("Test project"));
		QCOMPARE(m.pages.size(), 1);
		const EpdzManifest::Page &p = m.pages.first();
		QCOMPARE(p.title, QStringLiteral("Control"));
		QCOMPARE(p.folio, QStringLiteral("7"));			// the page counter: no ep1640 column
		QCOMPARE(p.plant, QStringLiteral("A1"));		// "==A1" without its prefix
		QCOMPARE(p.location, QStringLiteral("S1"));
		QVERIFY(p.svg.endsWith(QStringLiteral("1.svg")));
		QCOMPARE(m.devices.size(), 1);
		QCOMPARE(m.devices.first().article, QStringLiteral("SIE.3RT2015"));
		QCOMPARE(m.connections.size(), 1);
		const EpdzManifest::Connection &c = m.connections.first();
		QCOMPARE(c.source, QStringLiteral("-K1:A1"));
		QCOMPARE(c.potential, QStringLiteral("L1"));	// 20045 when 33003 is missing
		QCOMPARE(c.length, QStringLiteral("0.4 m"));
		QCOMPARE(m.spaces.size(), 1);
		QVERIFY(m.spaces.first().second.endsWith(QStringLiteral("space.e3d")));
	}

	void noManifest()
	{
		const QString empty = m_dir.filePath(QStringLiteral("empty"));
		QDir().mkpath(empty);
		EpdzManifest::Manifest m;
		QString error;
		QVERIFY(!EpdzManifest::read(empty, m, error));
		QVERIFY(error.contains(QStringLiteral("manifest.db")));
	}

	void csvCell()
	{
		QCOMPARE(EpdzManifest::csvCell(QStringLiteral("plain")), QStringLiteral("plain"));
		QCOMPARE(EpdzManifest::csvCell(QStringLiteral("a;b")), QStringLiteral("\"a;b\""));
		QCOMPARE(EpdzManifest::csvCell(QStringLiteral("say \"x\"")), QStringLiteral("\"say \"\"x\"\"\""));
		QCOMPARE(EpdzManifest::csvCell(QStringLiteral("two\nlines")), QStringLiteral("two lines"));
		QCOMPARE(EpdzManifest::bare(QStringLiteral(" ==EES ")), QStringLiteral("EES"));
	}

	void commandLine()
	{
		const QString archive = m_dir.filePath(QStringLiteral("Plan 1.epdz"));
		QCOMPARE(EplanProjectImport::defaultTarget(archive), m_dir.filePath(QStringLiteral("Plan 1.qet")));
		QVERIFY(EplanProjectImport::defaultTarget(QString()).isEmpty());
		const QStringList args = EplanProjectImport::arguments(QStringLiteral("s.py"), archive,
															   QStringLiteral("t.qet"), QStringLiteral("x"),
															   true, false);
		QCOMPARE(args.mid(0, 2), (QStringList{QStringLiteral("-u"), QStringLiteral("s.py")}));
		QCOMPARE(args.mid(4), (QStringList{QStringLiteral("--unpacked"), QStringLiteral("x"),
										   QStringLiteral("--pdf"), QStringLiteral("--no-collection"),
										   QStringLiteral("--stop-on-eof")}));
		QCOMPARE(EplanProjectImport::arguments(QStringLiteral("s.py"), archive, QStringLiteral("t.qet"),
											   QString(), false, true).size(), 5);
	}

	void output()
	{
		QCOMPARE(EplanProjectImport::resultFromOutput(
					 QStringLiteral("pages 2\ndone: /a/b.qet\nPROGRESS 100 done\ndone: /c/d e.qet  \n")),
				 QStringLiteral("/c/d e.qet"));
		QVERIFY(EplanProjectImport::resultFromOutput(QStringLiteral("failed\n")).isEmpty());
		int percent = -1;
		QCOMPARE(EplanProjectImport::progressText(QStringLiteral("PROGRESS 88 connections"), &percent),
				 QStringLiteral("Drawing connections"));
		QCOMPARE(percent, 88);
		QCOMPARE(EplanProjectImport::progressText(QStringLiteral("PROGRESS 150 newstep"), &percent),
				 QStringLiteral("newstep"));
		QCOMPARE(percent, 100);
		QVERIFY(EplanProjectImport::progressText(QStringLiteral("pages 2, parts 4"), &percent).isEmpty());
	}

	void findScript()
	{
		const QString app = m_dir.filePath(QStringLiteral("install/bin"));
		const QString script = m_dir.filePath(QStringLiteral("install/eplan_import/eplan_zw1/")
											  + EplanProjectImport::scriptName());
		QDir().mkpath(app);
		QDir().mkpath(QFileInfo(script).absolutePath());
		QVERIFY(EplanProjectImport::findScript(app).isEmpty()
				|| !qEnvironmentVariableIsEmpty("QET_EPLAN_IMPORT"));
		QFile f(script);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.close();
		if (qEnvironmentVariableIsEmpty("QET_EPLAN_IMPORT"))
			QCOMPARE(EplanProjectImport::findScript(app), QDir::cleanPath(script));
		QCOMPARE(EplanProjectImport::findScript(QStringLiteral("/nowhere"), script), QDir::cleanPath(script));
	}
};

QTEST_GUILESS_MAIN(tst_eplanprojectimport)

#include "tst_eplanprojectimport.moc"
