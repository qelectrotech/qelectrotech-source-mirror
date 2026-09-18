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

#include "logging/qetlogger.h"

#include "qetapp.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

/**
	QetLogger::init() asks QETApp for the data directory, and that is the
	only thing it needs from the application. Standing in for it here is
	what lets the crash-dump bookkeeping be tested without linking (or
	starting) the whole of QElectroTech.
*/
namespace {
	QString g_data_dir;
}

QString QETApp::dataDir()
{
	return g_data_dir;
}

/**
	@brief The tst_CrashDumps class

	Covers the crash-dump bookkeeping added in #905: one file per run
	instead of a single crash_dump.log that each crash overwrote (#898),
	and the rules about which of those files get offered and deleted.
*/
class tst_CrashDumps : public QObject
{
	Q_OBJECT

	private slots:
		void init();

		void listsNothingWhenThereHasBeenNoCrash();
		void listsDumpsNewestFirst();
		void skipsEmptyDumps();
		void excludesThisRunsOwnDump();
		void capsTheListAtTenDumps();
		void concatenatesEveryOfferedDump();
		void clearsOnlyWhatWasOffered();

		void redactsTheHomeDirectory();
		void redactsAnAppImageMountPoint();
		void redactsBothInOneString();

	private:
		QTemporaryDir m_dir;
		QString crashesDir() const {return g_data_dir + QStringLiteral("/crashes");}
		void writeDump(const QString &name, const QByteArray &body,
			       const QDateTime &when = QDateTime());
};

void tst_CrashDumps::init()
{
	QVERIFY(m_dir.isValid());
	// A fresh subdirectory per test: QetLogger is a singleton, so the
	// tests share one instance and must not share its files.
	static int n = 0;
	g_data_dir = m_dir.path() + QStringLiteral("/run") + QString::number(++n);
	QVERIFY(QDir().mkpath(g_data_dir));
	QVERIFY(QDir().mkpath(crashesDir()));
}

void tst_CrashDumps::writeDump(const QString &name, const QByteArray &body,
			       const QDateTime &when)
{
	const QString path = crashesDir() + QStringLiteral("/") + name;
	{
		QFile f(path);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write(body);
		f.close();
	}
	if (!when.isValid()) {
		return;
	}
	// Separately, and only once the write is closed: setFileTime() needs
	// an open handle, but closing a file that was just written sets the
	// modification time to now, which would undo it.
	QFile stamp(path);
	QVERIFY(stamp.open(QIODevice::ReadWrite));
	QVERIFY(stamp.setFileTime(when, QFileDevice::FileModificationTime));
	stamp.close();
}

void tst_CrashDumps::listsNothingWhenThereHasBeenNoCrash()
{
	QetLogger::instance().init();
	QVERIFY(QetLogger::instance().pendingCrashDumpFiles().isEmpty());
	QVERIFY(!QetLogger::instance().hasPendingCrashDump());
}

void tst_CrashDumps::listsDumpsNewestFirst()
{
	const QDateTime base = QDateTime::currentDateTime();
	writeDump(QStringLiteral("crash_a.log"), "oldest", base.addSecs(-300));
	writeDump(QStringLiteral("crash_b.log"), "middle", base.addSecs(-200));
	writeDump(QStringLiteral("crash_c.log"), "newest", base.addSecs(-100));

	QetLogger::instance().init();
	const QStringList files = QetLogger::instance().pendingCrashDumpFiles();

	QCOMPARE(files.size(), 3);
	QVERIFY(files.at(0).endsWith(QStringLiteral("crash_c.log")));
	QVERIFY(files.at(1).endsWith(QStringLiteral("crash_b.log")));
	QVERIFY(files.at(2).endsWith(QStringLiteral("crash_a.log")));
}

/**
	A zero-length dump means the handler opened the file and died before
	writing anything. There is nothing to show, and offering an empty
	report would be worse than offering none.
*/
void tst_CrashDumps::skipsEmptyDumps()
{
	writeDump(QStringLiteral("crash_empty.log"), QByteArray());
	writeDump(QStringLiteral("crash_real.log"), "something");

	QetLogger::instance().init();
	const QStringList files = QetLogger::instance().pendingCrashDumpFiles();

	QCOMPARE(files.size(), 1);
	QVERIFY(files.at(0).endsWith(QStringLiteral("crash_real.log")));
}

/**
	The dump this run would write if it crashed must never appear in the
	list of dumps from *previous* runs -- otherwise a process that crashed
	could be offered its own dump, mid-crash.
*/
void tst_CrashDumps::excludesThisRunsOwnDump()
{
	writeDump(QStringLiteral("crash_previous.log"), "from a previous run");

	QetLogger &logger = QetLogger::instance();
	logger.init();
	logger.installCrashHandler();   // this is what fixes our own path

	// installCrashHandler() picked crash_<yyyyMMdd-hhmmss>_<pid>.log for
	// this process but has not created it -- the handler only writes when
	// the process actually dies. Standing in for that here: fill in every
	// name it could have chosen, so whichever one it picked now exists
	// and is non-empty. The exact second does not have to be guessed.
	const qint64 pid = QCoreApplication::applicationPid();
	const QDateTime now = QDateTime::currentDateTime();
	for (int offset = -2 ; offset <= 0 ; ++offset) {
		writeDump(QStringLiteral("crash_%1_%2.log")
				  .arg(now.addSecs(offset).toString(QStringLiteral("yyyyMMdd-hhmmss")))
				  .arg(pid),
			  "this run's own dump, mid-crash");
	}

	// Exactly one of those three is the path install() actually chose,
	// and exactly that one must be missing from the list. The other two
	// are ordinary files as far as the logger is concerned.
	const QDir dir(crashesDir());
	const int on_disk = dir.entryList({QStringLiteral("crash_*_") + QString::number(pid)
						  + QStringLiteral(".log")},
					  QDir::Files).size();
	QCOMPARE(on_disk, 3);

	const QStringList files = logger.pendingCrashDumpFiles();
	int offered_own = 0;
	for (const QString &path : files) {
		if (path.contains(QString::number(pid))) {
			++offered_own;
		}
	}
	QCOMPARE(offered_own, 2);            // one of the three was excluded
	QCOMPARE(files.size(), 3);           // those two, plus crash_previous.log
	QVERIFY(files.last().endsWith(QStringLiteral("crash_previous.log")));
}

/**
	A crash loop writes one dump per restart. The list is capped so the
	dialog cannot be handed an unbounded amount of text.
*/
void tst_CrashDumps::capsTheListAtTenDumps()
{
	const QDateTime base = QDateTime::currentDateTime();
	for (int i = 0 ; i < 15 ; ++i) {
		writeDump(QStringLiteral("crash_%1.log").arg(i, 2, 10, QChar('0')),
			  QByteArray("dump ") + QByteArray::number(i),
			  base.addSecs(-1000 + i));
	}

	QetLogger::instance().init();
	QCOMPARE(QetLogger::instance().pendingCrashDumpFiles().size(), 10);

	// The ten kept are the newest ten, i.e. 05..14.
	const QStringList files = QetLogger::instance().pendingCrashDumpFiles();
	QVERIFY(files.at(0).endsWith(QStringLiteral("crash_14.log")));
	QVERIFY(files.at(9).endsWith(QStringLiteral("crash_05.log")));
}

/**
	#898: every dump is offered, not just the most recent one. The whole
	point of keeping them is that a repeating crash is where the earlier
	dumps carry the most information.
*/
void tst_CrashDumps::concatenatesEveryOfferedDump()
{
	const QDateTime base = QDateTime::currentDateTime();
	writeDump(QStringLiteral("crash_one.log"), "FIRST CRASH BODY", base.addSecs(-200));
	writeDump(QStringLiteral("crash_two.log"), "SECOND CRASH BODY", base.addSecs(-100));

	QetLogger &logger = QetLogger::instance();
	logger.init();

	const QStringList offered = logger.pendingCrashDumpFiles();
	const QByteArray content = logger.pendingCrashDumpContents(offered);

	QVERIFY(content.contains("FIRST CRASH BODY"));
	QVERIFY(content.contains("SECOND CRASH BODY"));
	QVERIFY(content.contains("crash_one.log"));
	QVERIFY(content.contains("crash_two.log"));
	QVERIFY(content.contains("2 crash dumps pending"));
}

/**
	The reason clearPendingCrashDump() takes the list rather than looking
	the directory up again: the offer sits in a modal dialog, and a dump
	that arrives while it is open has never been seen by anybody.
*/
void tst_CrashDumps::clearsOnlyWhatWasOffered()
{
	writeDump(QStringLiteral("crash_offered.log"), "offered to the user");

	QetLogger &logger = QetLogger::instance();
	logger.init();

	const QStringList offered = logger.pendingCrashDumpFiles();
	QCOMPARE(offered.size(), 1);

	// ... the dialog is open, and a second instance crashes.
	writeDump(QStringLiteral("crash_arrived_later.log"), "nobody has seen this yet");

	logger.clearPendingCrashDump(offered);

	const QStringList left = logger.pendingCrashDumpFiles();
	QCOMPARE(left.size(), 1);
	QVERIFY(left.at(0).endsWith(QStringLiteral("crash_arrived_later.log")));
}

void tst_CrashDumps::redactsTheHomeDirectory()
{
	const QByteArray home = QDir::homePath().toUtf8();
	QVERIFY(!home.isEmpty());

	const QByteArray in = home + "/projects/secret.qet failed to load";
	const QByteArray out = QetLogger::redact(in);

	QVERIFY(!out.contains(home));
	QVERIFY(out.startsWith("~/projects/secret.qet"));
}

/**
	backtrace_symbols_fd() writes absolute module paths, which for an
	AppImage is a per-run mount point under /tmp/.mount_. Raised in review
	on #905.
*/
void tst_CrashDumps::redactsAnAppImageMountPoint()
{
	const QByteArray in =
		"/tmp/.mount_QElect6Yh2Kz/usr/bin/qelectrotech(+0x9b098e) [0x5ecf]\n"
		"/tmp/.mount_QElect6Yh2Kz/usr/lib/libQt6Core.so.6(+0x1234) [0x7cfa]\n";
	const QByteArray out = QetLogger::redact(in);

	QVERIFY(!out.contains(".mount_QElect6Yh2Kz"));
	QVERIFY(out.contains("<appimage>/usr/bin/qelectrotech"));
	QVERIFY(out.contains("<appimage>/usr/lib/libQt6Core.so.6"));
	// The frame offsets are the useful part and must survive.
	QVERIFY(out.contains("(+0x9b098e) [0x5ecf]"));
}

void tst_CrashDumps::redactsBothInOneString()
{
	const QByteArray home = QDir::homePath().toUtf8();
	const QByteArray in = home + "/Documents/a.qet\n/tmp/.mount_AbCdEf/usr/bin/qet\n";
	const QByteArray out = QetLogger::redact(in);

	QVERIFY(!out.contains(home));
	QVERIFY(!out.contains(".mount_AbCdEf"));
	QVERIFY(out.contains("~/Documents/a.qet"));
	QVERIFY(out.contains("<appimage>/usr/bin/qet"));
}

QTEST_MAIN(tst_CrashDumps)
#include "tst_crashdumps.moc"
