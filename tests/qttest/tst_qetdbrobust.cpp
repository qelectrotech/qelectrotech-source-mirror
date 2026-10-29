// SPDX-License-Identifier: GPL-2.0-or-later
// A .qetdb survives what happens to files (QET-SQL-FORMAT S5): QElectroTech
// killed during a save, a sync tool copying the file during a save, a full
// disk, a missing folder, and damaged files opened. The file is always the
// previous project or the new one, complete; a damaged one is refused or
// opened, never a crash. Runs the real binary; QET_QETDB_KILLS=100 for the
// longer run (25 by default).

#include <QtTest>

#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include <atomic>
#include <thread>

#ifdef Q_OS_UNIX
#	include <csignal>
#	include <sys/resource.h>
#endif

class tst_qetdbrobust : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	QString m_old, m_new;   // the same file name saved from two projects
	QByteArray m_old_bytes, m_new_bytes;
	qint64 m_save_ms = 0;
	qint64 m_writing_ms = 0;   // from the new file appearing to the end

	QProcessEnvironment env() const
	{
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QProcessEnvironment e = QProcessEnvironment::systemEnvironment();
		e.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		e.insert(QStringLiteral("HOME"), home);
		e.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		e.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		e.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		return e;
	}

	QStringList resaveArgs(const QString &in, const QString &out) const
	{
		return {QStringLiteral("--common-elements-dir=" QET_ELEMENTS_DIR "/"),
				QStringLiteral("--resave"), in, out};
	}

	int resave(const QString &in, const QString &out, QString *log = nullptr, bool limit_file_size = false)
	{
		QProcess proc;
		proc.setProcessEnvironment(env());
#ifdef Q_OS_UNIX
		if (limit_file_size) {
				//A disk that fills up: no file of the process may grow past
				//512 kB; the write fails (EFBIG) instead of killing it
			proc.setChildProcessModifier([] {
				::signal(SIGXFSZ, SIG_IGN);
				struct rlimit limit{512 * 1024, 512 * 1024};
				::setrlimit(RLIMIT_FSIZE, &limit);
			});
		}
#else
		Q_UNUSED(limit_file_size)
#endif
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), resaveArgs(in, out));
		if (!proc.waitForFinished(300000)) return -1;
		if (log) *log = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		return proc.exitStatus() == QProcess::NormalExit ? proc.exitCode() : -1;
	}

	static QByteArray read(const QString &path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}

	// A complete .qetdb: passes SQLite's check and is ours
	static bool complete(const QString &path)
	{
		bool ok = false;
		{
			QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("look"));
			db.setDatabaseName(path);
			db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
			if (db.open()) {
				QSqlQuery q(db);
				ok = q.exec(QStringLiteral("PRAGMA quick_check")) && q.next() && q.value(0).toString() == QLatin1String("ok")
						&& q.exec(QStringLiteral("PRAGMA application_id")) && q.next() && q.value(0).toInt() == 0x51455444;
			}
			db.close();
		}
		QSqlDatabase::removeDatabase(QStringLiteral("look"));
		return ok;
	}

	void putOld(const QString &target)
	{
			//And no new file left by a save killed before
		for (const char *suffix : {"", ".saving", ".saving-journal"})
			QFile::remove(target + QLatin1String(suffix));
			//Written beside it and put in place at once, as a save does:
			//copiedDuringSave() copies the file while this runs
		const QString beside = target + QStringLiteral(".old");
		QFile f(beside);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write(m_old_bytes);
		f.close();
		QVERIFY(QFile::rename(beside, target));
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
		QDir().mkpath(m_dir.filePath(QStringLiteral("old")));
		QDir().mkpath(m_dir.filePath(QStringLiteral("new")));
		m_old = m_dir.filePath(QStringLiteral("old/p.qetdb"));
		m_new = m_dir.filePath(QStringLiteral("new/p.qetdb"));
		QCOMPARE(resave(QStringLiteral(QET_EXAMPLES_DIR "/grafcet.qet"), m_old), 0);
		QElapsedTimer timer;
		timer.start();
		QCOMPARE(resave(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), m_new), 0);
		m_save_ms = timer.elapsed();
			//How long the save itself lasts, from its new file appearing
		{
			const QString target = m_dir.filePath(QStringLiteral("timed.qetdb"));
			QProcess proc;
			proc.setProcessEnvironment(env());
			proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
					   resaveArgs(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), target));
			QVERIFY(proc.waitForStarted());
			while (!QFile::exists(target + QStringLiteral(".saving")) && proc.state() == QProcess::Running)
				QThread::usleep(200);
			QElapsedTimer writing;
			writing.start();
			QVERIFY(proc.waitForFinished(300000));
			m_writing_ms = writing.elapsed();
			QVERIFY(m_writing_ms > 0);
		}
		m_old_bytes = read(m_old);
		m_new_bytes = read(m_new);
		QVERIFY(!m_old_bytes.isEmpty() && !m_new_bytes.isEmpty());
	}

	// Killed at any moment of a save over the previous file (once the new
	// file is being written beside it): the file is
	// the previous project or the new one, never a broken file. (The new
	// file is the new project's, saved in its own run: equal bytes but for
	// the save time in qet_format, so it is checked whole, not compared.)
	void killedDuringSave()
	{
		const int kills = qEnvironmentVariableIsSet("QET_QETDB_KILLS")
				? qEnvironmentVariableIntValue("QET_QETDB_KILLS") : 25;
		const QString target = m_dir.filePath(QStringLiteral("killed.qetdb"));
		int old_kept = 0, new_written = 0;
		for (int i = 0 ; i < kills ; ++i) {
			putOld(target);
			QProcess proc;
			proc.setProcessEnvironment(env());
			proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
					   resaveArgs(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), target));
			QVERIFY(proc.waitForStarted());
				//Most of a run is opening the project: the kill is aimed at
				//the save, from when its new file appears beside the old
			QElapsedTimer waited;
			waited.start();
			while (!QFile::exists(target + QStringLiteral(".saving")) && proc.state() == QProcess::Running
				   && waited.elapsed() < m_save_ms * 3)
				QThread::usleep(500);
			QThread::msleep(QRandomGenerator::global()->bounded(int(m_writing_ms * 1.1) + 1));
			proc.kill();
			proc.waitForFinished();
			const QByteArray now = read(target);
			if (now == m_old_bytes) {
				++old_kept;
			} else {
				QVERIFY2(complete(target), qPrintable(QStringLiteral("kill %1 left a broken file").arg(i)));
				++new_written;
			}
		}
		qInfo().noquote() << kills << "kills over the" << m_writing_ms << "ms of a save:" << old_kept << "previous file," << new_written << "new file";
	}

	// A sync tool copying the file while it is saved over: every copy is a
	// complete file
	void copiedDuringSave()
	{
		const QString target = m_dir.filePath(QStringLiteral("copied.qetdb"));
		putOld(target);
		std::atomic<bool> stop{false};
		std::atomic<int> copies{0}, broken{0};
		const QString copy_dir = m_dir.filePath(QStringLiteral("copies"));
		QDir().mkpath(copy_dir);
		std::thread copier([&]() {
			while (!stop) {
				const QByteArray bytes = read(target);
				if (bytes.isEmpty()) continue;
				const QString copy = copy_dir + QStringLiteral("/%1.qetdb").arg(copies.load());
				QFile f(copy);
				if (f.open(QIODevice::WriteOnly)) f.write(bytes);
				f.close();
				++copies;
				QThread::msleep(5);
			}
		});
		for (int i = 0 ; i < 3 ; ++i) {
			QCOMPARE(resave(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), target), 0);
			putOld(target);
		}
		stop = true;
		copier.join();
		for (int i = 0 ; i < copies ; ++i)
			if (!complete(copy_dir + QStringLiteral("/%1.qetdb").arg(i))) ++broken;
		qInfo().noquote() << copies.load() << "copies taken during 3 saves," << broken.load() << "broken";
		QVERIFY(copies > 0);
		QCOMPARE(broken.load(), 0);
	}

	// A disk that fills up during the save, and a folder that is not there:
	// the save fails, says so, and the previous file is as it was
	void saveCannotFinish()
	{
#ifdef Q_OS_UNIX
		const QString target = m_dir.filePath(QStringLiteral("full.qetdb"));
		putOld(target);
		QString log;
		QVERIFY(resave(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), target, &log, true) != 0);
		QVERIFY2(log.contains(QStringLiteral("Cannot write")), qPrintable(log.right(600)));
		QVERIFY(read(target) == m_old_bytes);
		QVERIFY(!QFile::exists(target + QStringLiteral(".saving")));
#endif
		const QString nowhere = m_dir.filePath(QStringLiteral("not-there/p.qetdb"));
		QVERIFY(resave(QStringLiteral(QET_EXAMPLES_DIR "/grafcet.qet"), nowhere) != 0);
		QVERIFY(!QFile::exists(nowhere));
	}

	// Damaged files: bytes changed at random places in a copy of a
	// .qetdb. Opening one is refused or gives a project; it never crashes.
	void damagedNeverCrash()
	{
		auto *random = QRandomGenerator::global();
		int refused = 0, opened = 0;
		for (int i = 0 ; i < 40 ; ++i) {
			QByteArray bytes = m_old_bytes;
			const int changes = 1 + random->bounded(64);
			for (int c = 0 ; c < changes ; ++c)
				bytes[random->bounded(int(bytes.size()))] = char(random->bounded(256));
			const QString damaged = m_dir.filePath(QStringLiteral("damaged%1.qetdb").arg(i));
			QFile f(damaged);
			QVERIFY(f.open(QIODevice::WriteOnly));
			f.write(bytes);
			f.close();
			const int code = resave(damaged, damaged + QStringLiteral(".qet"));
			QVERIFY2(code >= 0, qPrintable(QStringLiteral("damaged file %1 crashed QElectroTech").arg(i)));
			code == 0 ? ++opened : ++refused;
		}
		qInfo().noquote() << "40 damaged files:" << refused << "refused," << opened << "opened";
	}
};

QTEST_GUILESS_MAIN(tst_qetdbrobust)
#include "tst_qetdbrobust.moc"
