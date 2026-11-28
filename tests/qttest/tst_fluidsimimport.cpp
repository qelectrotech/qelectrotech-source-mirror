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
#include "import/fluidsim/fluidsimimport.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QtTest>

/**
	File > Import FluidSIM circuit runs the Python program next to
	QElectroTech: where it is looked for, what it is given and how its
	answer is read. The import itself has its own tests
	(tools/fluidsim_import/test_fluidsim_import.py).
*/
class tst_FluidSimImport : public QObject
{
	Q_OBJECT

	static void touch(const QString &path)
	{
		QDir().mkpath(QFileInfo(path).absolutePath());
		QFile f(path);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write("# test\n");
	}

	private slots:
		void init()
		{
			qunsetenv("QET_FLUIDSIM_IMPORT");
		}

		void scriptNextToAnInstallation()
		{
			QTemporaryDir dir;
			QVERIFY(dir.isValid());
				// Windows: <prefix>/bin/qelectrotech.exe, <prefix>/fluidsim_import
			touch(dir.filePath("fluidsim_import/fluidsim_import.py"));
			QDir(dir.path()).mkpath("bin");
			QCOMPARE(FluidSimImport::findScript(dir.filePath("bin")),
				 QDir::cleanPath(dir.filePath("fluidsim_import/fluidsim_import.py")));
		}

		void scriptInShare()
		{
			QTemporaryDir dir;
			QVERIFY(dir.isValid());
			touch(dir.filePath("share/qelectrotech/fluidsim_import/fluidsim_import.py"));
			QDir(dir.path()).mkpath("bin");
			QCOMPARE(FluidSimImport::findScript(dir.filePath("bin")),
				 QDir::cleanPath(dir.filePath("share/qelectrotech/fluidsim_import/fluidsim_import.py")));
		}

		void scriptInTheSourceTree()
		{
			QTemporaryDir dir;
			QVERIFY(dir.isValid());
				// a build folder in the source tree: <source>/build/qelectrotech
			touch(dir.filePath("tools/fluidsim_import/fluidsim_import.py"));
			QDir(dir.path()).mkpath("build");
			QCOMPARE(FluidSimImport::findScript(dir.filePath("build")),
				 QDir::cleanPath(dir.filePath("tools/fluidsim_import/fluidsim_import.py")));
		}

		void environmentFirst()
		{
			QTemporaryDir dir;
			QVERIFY(dir.isValid());
			touch(dir.filePath("fluidsim_import/fluidsim_import.py"));
			touch(dir.filePath("elsewhere/mine.py"));
			qputenv("QET_FLUIDSIM_IMPORT", dir.filePath("elsewhere/mine.py").toUtf8());
			QCOMPARE(FluidSimImport::findScript(dir.path()),
				 QDir::cleanPath(dir.filePath("elsewhere/mine.py")));
		}

		void noScript()
		{
			QTemporaryDir dir;
			QVERIFY(dir.isValid());
			QVERIFY(FluidSimImport::findScript(dir.path()).isEmpty());
		}

		void arguments()
		{
			const QStringList args = FluidSimImport::arguments(
				"/opt/qet/fluidsim_import/fluidsim_import.py",
				{"/data/a.circ", "/data/b.circ"}, "/data/out.qet", "/opt/qet/qelectrotech");
			QCOMPARE(args.size(), 8);
			QCOMPARE(args.at(0), QStringLiteral("-u"));
			QCOMPARE(QDir::fromNativeSeparators(args.at(1)),
				 QStringLiteral("/opt/qet/fluidsim_import/fluidsim_import.py"));
			QCOMPARE(QDir::fromNativeSeparators(args.at(2)), QStringLiteral("/data/a.circ"));
			QCOMPARE(QDir::fromNativeSeparators(args.at(4)), QStringLiteral("/data/out.qet"));
			QCOMPARE(args.at(5), QStringLiteral("--qet"));
			QCOMPARE(QDir::fromNativeSeparators(args.at(6)), QStringLiteral("/opt/qet/qelectrotech"));
			QCOMPARE(args.at(7), QStringLiteral("--stop-on-eof"));
		}

		void resultIsTheLastDoneLine()
		{
			const QString out = QStringLiteral(
				"/data/out.qet: 1 folio(s), 6 parts, 3 wires\n"
				"  not imported: x\n"
				"done: /data/first.qet\n"
				"done: /data/out.qet  \r\n");
			QCOMPARE(FluidSimImport::resultFromOutput(out), QStringLiteral("/data/out.qet"));
			QVERIFY(FluidSimImport::resultFromOutput("project not saved: {}").isEmpty());
				// "done:" only counts at the start of a line
			QVERIFY(FluidSimImport::resultFromOutput("  FAILED done: x").isEmpty());
		}

		void noStoreShortcut()
		{
			for (const QString &p : FluidSimImport::pythons())
				QVERIFY2(!p.contains("WindowsApps", Qt::CaseInsensitive), qPrintable(p));
		}
};

QTEST_MAIN(tst_FluidSimImport)
#include "tst_fluidsimimport.moc"
