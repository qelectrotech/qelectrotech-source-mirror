// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QThread>

#include <functional>

// Saving a project replaced the file whatever had happened to it since it was
// opened: another user's save, or another program's, was lost without a word.
// QETProject now remembers the file as it read or wrote it and write() refuses
// when the file on disk is no longer that one. Driven through the real
// binary's --run: qet.save(<the project's own path>) writes the file behind
// the open project without updating what it remembers, which is exactly "someone
// else saved"; qet.save('') is the project's own save. Only a different content
// counts: the same bytes with a new time are what a network share reports
// right after our own save (#1579).
class tst_changedondisk : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	QString m_project; // a .qetz made from examples/741.qet

	QString m_database; // the same project as a .qetdb

	QString run(const QString &script, const QString &project, bool overwrite = false,
				const std::function<void()> &meanwhile = {})
	{
		return launch({QStringLiteral("--run"), scriptFile(script), project}, overwrite, meanwhile);
	}

	QString scriptFile(const QString &script)
	{
		const QString path = m_dir.filePath(QStringLiteral("probe.js"));
		QFile f(path);
		if (!f.open(QIODevice::WriteOnly)) return QString();
		f.write(script.toUtf8());
		return path;
	}

	QString launch(const QStringList &args, bool overwrite = false,
				   const std::function<void()> &meanwhile = {})
	{
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home);

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("TMPDIR"), m_dir.path());
		env.remove(QStringLiteral("QET_OVERWRITE_CHANGED"));
		if (overwrite)
			env.insert(QStringLiteral("QET_OVERWRITE_CHANGED"), QStringLiteral("1"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), args);
		if (meanwhile) meanwhile();
		if (!proc.waitForFinished(120000)) return QString();
		return QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
		const QString qet = m_dir.filePath(QStringLiteral("p.qet"));
		QVERIFY(QFile::copy(QStringLiteral(QET_EXAMPLES_DIR "/741.qet"), qet));
		// the .qet saved as a .qetz is the project below
		m_project = m_dir.filePath(QStringLiteral("p.qetz"));
		const QString out = run(QStringLiteral("qet.log('R ' + qet.save('%1'));\n").arg(m_project), qet);
		QVERIFY2(out.contains(QStringLiteral("R true")), qPrintable(out));
		QVERIFY(QFile::exists(m_project));
		m_database = m_dir.filePath(QStringLiteral("p.qetdb"));
		launch({QStringLiteral("--resave"), qet, m_database});
		QVERIFY(QFile::exists(m_database));
	}

	void unchangedFileSaves()
	{
		const QString out = run(QStringLiteral(
			"qet.log('R1 ' + qet.save(''));\nqet.log('R2 ' + qet.save(''));\n"), m_project);
		QVERIFY2(out.contains(QStringLiteral("R1 true")), qPrintable(out));
		// the project's own save updates what it remembers: a second one is no change
		QVERIFY2(out.contains(QStringLiteral("R2 true")), qPrintable(out));
	}

	void sameContentNewTimeSaves()
	{
		// the same bytes written behind the project (an unchanged project
		// saves to an identical file, F124): only the time differs
		const QString script = QStringLiteral(
			"qet.save(%1);\nqet.log('R ' + qet.save(''));\n")
			.arg(QStringLiteral("'") + m_project + QStringLiteral("'"));
		const QString out = run(script, m_project);
		QVERIFY2(out.contains(QStringLiteral("R true")), qPrintable(out));
	}

	void changedFileIsNotOverwritten()
	{
		const QString script = QStringLiteral(
			"qet.moveElement(0, qet.elementUuids(0)[0], 10, 0);\n"
			"qet.save(%1);\nqet.log('R ' + qet.save(''));\n")
			.arg(QStringLiteral("'") + m_project + QStringLiteral("'"));
		const QString out = run(script, m_project);
		QVERIFY2(out.contains(QStringLiteral("R false")), qPrintable(out));
		QVERIFY2(out.contains(QStringLiteral("changed the file on disk")), qPrintable(out));
	}

	void overrideOverwrites()
	{
		const QString script = QStringLiteral(
			"qet.moveElement(0, qet.elementUuids(0)[0], 10, 0);\n"
			"qet.save(%1);\nqet.log('R ' + qet.save(''));\n")
			.arg(QStringLiteral("'") + m_project + QStringLiteral("'"));
		const QString out = run(script, m_project, true);
		QVERIFY2(out.contains(QStringLiteral("R true")), qPrintable(out));
	}

	// A .qetdb: its save remembers the file it wrote (the hash made while
	// writing it), so a second save is no conflict and another's is
	void qetdbUnchangedSaves()
	{
		const QString out = run(QStringLiteral(
			"qet.log('R1 ' + qet.save(''));\nqet.log('R2 ' + qet.save(''));\n"), m_database);
		QVERIFY2(out.contains(QStringLiteral("R1 true")), qPrintable(out));
		QVERIFY2(out.contains(QStringLiteral("R2 true")), qPrintable(out));
	}

	// The same bytes with a new time after its own save (a network share
	// settling it, #1579): the file is then hashed, so the hash the save
	// handed over must be the file's, or the next save refuses
	void qetdbSameContentNewTimeSaves()
	{
		const QDateTime before = QFileInfo(m_database).lastModified();
		bool touched = false;
		const auto touch = [&]() {
			QElapsedTimer timer;
			timer.start();
			while (QFileInfo(m_database).lastModified() == before && timer.elapsed() < 60000)
				QThread::msleep(50);
				// past the save's own look at the file
			QThread::msleep(1000);
			QFile file(m_database);
			touched = file.open(QIODevice::ReadWrite)
					&& file.setFileTime(before.addSecs(-3600), QFileDevice::FileModificationTime);
		};
		const QString out = run(QStringLiteral(
			"qet.log('R1 ' + qet.save(''));\n"
			"var t = Date.now(); while (Date.now() - t < 5000) {}\n"
			"qet.log('R2 ' + qet.save(''));\n"), m_database, false, touch);
		QVERIFY(touched);
		QVERIFY2(out.contains(QStringLiteral("R1 true")), qPrintable(out));
		QVERIFY2(out.contains(QStringLiteral("R2 true")), qPrintable(out));
	}

	void qetdbChangedFileIsNotOverwritten()
	{
		const QString script = QStringLiteral(
			"qet.moveElement(0, qet.elementUuids(0)[0], 10, 0);\n"
			"qet.save(%1);\nqet.log('R ' + qet.save(''));\n")
			.arg(QStringLiteral("'") + m_database + QStringLiteral("'"));
		const QString out = run(script, m_database);
		QVERIFY2(out.contains(QStringLiteral("R false")), qPrintable(out));
		QVERIFY2(out.contains(QStringLiteral("changed the file on disk")), qPrintable(out));
	}

	void saveAsTargetIsNotAConflict()
	{
		const QString other = m_dir.filePath(QStringLiteral("other.qetz"));
		const QString script = QStringLiteral(
			"qet.log('R1 ' + qet.save(%1));\nqet.log('R2 ' + qet.save(%1));\n")
			.arg(QStringLiteral("'") + other + QStringLiteral("'"));
		const QString out = run(script, m_project);
		QVERIFY2(out.contains(QStringLiteral("R1 true")), qPrintable(out));
		QVERIFY2(out.contains(QStringLiteral("R2 true")), qPrintable(out));
	}
};

QTEST_APPLESS_MAIN(tst_changedondisk)

#include "tst_changedondisk.moc"
