// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"
#include "qetproject.h"
#include "qetresult.h"
#include "scripting/qetscriptapi.h"

#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QTemporaryDir>

// The crash-recovery backup is written soon after the user stops editing,
// not only every 20 minutes: QET_BACKUP_IDLE_MS (idle delay) and
// QET_BACKUP_MIN_GAP_MS (least time between two backups) shorten the two
// delays, which are 30 s and 2 minutes in a real run.
// QET_BACKUP_MEASURE_FILES (paths separated by ':') switches the test to
// measuring: for each project, the GUI-thread time of the backup after
// opening, after building one folio and with every folio built, printed
// as "MEASURE".
class tst_backupidle : public QObject
{
	Q_OBJECT

	QTemporaryDir m_data;

	// Backup files KAutoSaveFile has written, by size
	QList<qint64> backups() const
	{
		QList<qint64> sizes;
		QDirIterator it(m_data.filePath("data"), QDir::Files, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			it.next();
			sizes << it.fileInfo().size();
		}
		return sizes;
	}

	bool waitForBackup(int ms) const
	{
		QElapsedTimer t;
		t.start();
			// The backup itself can block this thread past @p ms, so look
			// once more after the last wait
		do {
			QTest::qWait(20);
			for (qint64 s : backups())
				if (s > 0)
					return true;
		} while (t.elapsed() < ms);
		return false;
	}

	// The files of earlier cases are no evidence for this one
	void forgetBackups()
	{
		QDirIterator it(m_data.filePath("data"), QDir::Files, QDirIterator::Subdirectories);
		while (it.hasNext())
			QFile::remove(it.next());
	}

	// For "no backup": the write runs on another thread after the GUI
	// thread has made the snapshot, so give it time to appear
	bool backupEverAppears(int ms) const
	{
		if (waitForBackup(ms))
			return true;
		return waitForBackup(3000);
	}

	QString copyOfExample(const QString &name)
	{
		const QString target = m_data.filePath(name + QStringLiteral(".copy.qet"));
		QFile::remove(target);
		QFile::copy(QStringLiteral(QET_EXAMPLES_DIR "/") + name, target);
		return target;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_data.isValid());
		qputenv("XDG_DATA_HOME", m_data.filePath("data").toUtf8());
		qputenv("QET_BACKUP_IDLE_MS", "300");
		qputenv("QET_BACKUP_MIN_GAP_MS", "300");
	}

	// An edit is backed up once the idle delay has passed
	void backupAfterEdit()
	{
		QETProject project(copyOfExample(QStringLiteral("industrial.qet")));
		QCOMPARE(project.state(), QETProject::Ok);
		qInfo() << "after open:" << backups();
		QetScriptApi api(&project);
		QVERIFY(api.setFolioProperty(0, QStringLiteral("title"), QStringLiteral("edited")));
		QVERIFY2(waitForBackup(5000), "no backup after the idle delay");
	}

	// A project that was only opened has nothing to back up
	void noBackupWithoutEdit()
	{
		forgetBackups();
		QETProject project(copyOfExample(QStringLiteral("industrial.qet")));
		QCOMPARE(project.state(), QETProject::Ok);
		QVERIFY2(!backupEverAppears(1500), "a backup of an unchanged project");
	}

	// Edits that keep coming push the backup back: none while the user is
	// still working
	void noBackupWhileEditing()
	{
		forgetBackups();
		QETProject project(copyOfExample(QStringLiteral("industrial.qet")));
		QetScriptApi api(&project);
		QElapsedTimer t;
		t.start();
		for (int i = 0; t.elapsed() < 1500; ++i) {
			QVERIFY(api.setFolioProperty(0, QStringLiteral("title"), QString::number(i)));
			QTest::qWait(100);
		}
		for (qint64 s : backups())
			QCOMPARE(s, qint64(0));
	}

	// A project opened with its folios not built is left to the 20 minute
	// timer: backing it up builds every folio
	void leftToTimerWhileFoliosUnbuilt()
	{
		forgetBackups();
		const QString saved = m_data.filePath(QStringLiteral("lazy.qet"));
		QFile::remove(saved);
		{
			QETProject source(copyOfExample(QStringLiteral("industrial.qet")));
			source.setFilePath(saved);
			QVERIFY(source.write().isOk());
		}
		forgetBackups();
		qunsetenv("QET_EAGER_FOLIOS");
		QETProject project(saved);
		QCOMPARE(project.state(), QETProject::Ok);
		QVERIFY2(project.unloadedFolioCount() > 0, "the saved project did not open lazily");
		project.setModified(true);
		QVERIFY2(!backupEverAppears(1500), "idle backup of a project with unbuilt folios");
	}

	void measure()
	{
		const QString files = qEnvironmentVariable("QET_BACKUP_MEASURE_FILES");
		if (files.isEmpty())
			QSKIP("QET_BACKUP_MEASURE_FILES not set");
		qputenv("QET_BACKUP_TIMING", "1");
		const bool eager = qEnvironmentVariableIsSet("QET_EAGER_FOLIOS");
		for (const QString &path : files.split(':')) {
			forgetBackups();
			// A project saved by this QET has the folio uuids lazy opening
			// needs; an older file is opened with every folio built
			const QString saved = m_data.filePath(QStringLiteral("saved.qet"));
			QFile::remove(saved);
			{
				QETProject source(path);
				QCOMPARE(source.state(), QETProject::Ok);
				source.setFilePath(saved);
				QVERIFY(source.write().isOk());
			}
			for (int built = 0 ; built < 2 ; ++built) {
				forgetBackups();
				QElapsedTimer open;
				open.start();
				QETProject project(saved);
				QCOMPARE(project.state(), QETProject::Ok);
				const qint64 open_ms = open.elapsed();
				const int unbuilt = project.unloadedFolioCount();
				if (built)
					(void)project.diagrams().at(0);
				qInfo().noquote() << "MEASURE" << QFileInfo(path).fileName() << (eager ? "eager" : "lazy")
					<< "folios" << project.folios().size() << "unbuilt at open" << unbuilt
					<< "unbuilt at backup" << project.unloadedFolioCount()
					<< "open ms" << open_ms << "<- next backup line is the GUI-thread cost";
				project.setModified(true);
				waitForBackup(60000);
				QTest::qWait(300);
			}
		}
	}
};

QET_APP_TEST_MAIN(tst_backupidle)
#include "tst_backupidle.moc"
