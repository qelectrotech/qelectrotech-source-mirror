// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// Saving a project that was just saved must change nothing. Two things
// made the second save differ from the first, both cleanup done on save
// but not on load:
//  - symbol information whose values were all empty was written as an
//    empty <elementInformations/> block, which the next load read as no
//    information and the next save dropped (Projet_vierge.qet);
//  - information values were trimmed on save but not on load, so a label
//    with stray spaces kept them in its displayed copy until the project
//    was opened again (m_000.qet).
// Runs the real binary's --resave twice on each example.
class tst_resaveunchanged : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// --resave @p in to a new file, in a sandbox of its own (so a running
	// QElectroTech cannot answer instead); returns the new file's path.
	QString resave(const QString &in)
	{
		const QString out = m_dir.filePath(QStringLiteral("out%1.qet").arg(m_run));
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--resave"), in, out});
		if (!proc.waitForFinished(120000) || proc.exitCode() != 0) return {};
		return out;
	}

	static QByteArray read(const QString &path)
	{
		QFile f(path);
		return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void secondSaveChangesNothing_data()
	{
		QTest::addColumn<QString>("project");
		QTest::newRow("empty information values") << QStringLiteral("Projet_vierge.qet");
		QTest::newRow("information values with stray spaces") << QStringLiteral("m_000.qet");
	}

	void secondSaveChangesNothing()
	{
		QFETCH(QString, project);
		const QString first = resave(QStringLiteral(QET_EXAMPLES_DIR "/") + project);
		QVERIFY2(!first.isEmpty(), "first --resave failed");
		const QString second = resave(first);
		QVERIFY2(!second.isEmpty(), "second --resave failed");
		const QByteArray a = read(first), b = read(second);
		QVERIFY(!a.isEmpty());
		QVERIFY2(a == b, "the second save changed the file");
	}
};

QTEST_APPLESS_MAIN(tst_resaveunchanged)

#include "tst_resaveunchanged.moc"
