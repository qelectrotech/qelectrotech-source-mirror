// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// Saving a project replaced the file whatever had happened to it since it was
// opened: another user's save, or another program's, was lost without a word.
// QETProject now remembers the file as it read or wrote it and write() refuses
// when the file on disk is no longer that one. Driven through the real
// binary's --run: qet.save(<the project's own path>) writes the file behind
// the open project without updating what it remembers, which is exactly "someone
// else saved"; qet.save('') is the project's own save.
class tst_changedondisk : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	QString m_project; // a .qetz made from examples/741.qet

	QString run(const QString &script, const QString &project, bool overwrite = false)
	{
		const QString path = m_dir.filePath(QStringLiteral("probe.js"));
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home);
		QFile f(path);
		if (!f.open(QIODevice::WriteOnly)) return QString();
		f.write(script.toUtf8());
		f.close();

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
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), path, project});
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
		// a .qet is saved as a .qetz beside it; that .qetz is the project below
		const QString out = run(QStringLiteral("qet.log('R ' + qet.save(''));\n"), qet);
		QVERIFY2(out.contains(QStringLiteral("R true")), qPrintable(out));
		m_project = m_dir.filePath(QStringLiteral("p.qetz"));
		QVERIFY(QFile::exists(m_project));
	}

	void unchangedFileSaves()
	{
		const QString out = run(QStringLiteral(
			"qet.log('R1 ' + qet.save(''));\nqet.log('R2 ' + qet.save(''));\n"), m_project);
		QVERIFY2(out.contains(QStringLiteral("R1 true")), qPrintable(out));
		// the project's own save updates what it remembers: a second one is no change
		QVERIFY2(out.contains(QStringLiteral("R2 true")), qPrintable(out));
	}

	void changedFileIsNotOverwritten()
	{
		const QString script = QStringLiteral(
			"qet.save(%1);\nqet.log('R ' + qet.save(''));\n")
			.arg(QStringLiteral("'") + m_project + QStringLiteral("'"));
		const QString out = run(script, m_project);
		QVERIFY2(out.contains(QStringLiteral("R false")), qPrintable(out));
		QVERIFY2(out.contains(QStringLiteral("changed the file on disk")), qPrintable(out));
	}

	void overrideOverwrites()
	{
		const QString script = QStringLiteral(
			"qet.save(%1);\nqet.log('R ' + qet.save(''));\n")
			.arg(QStringLiteral("'") + m_project + QStringLiteral("'"));
		const QString out = run(script, m_project, true);
		QVERIFY2(out.contains(QStringLiteral("R true")), qPrintable(out));
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
