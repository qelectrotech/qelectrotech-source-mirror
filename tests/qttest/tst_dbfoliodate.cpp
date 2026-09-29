// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// The project database holds each folio's own date. It used to hold the
// title block's text read back through the locale's short format, which in a
// locale with a two-digit year (en_US "M/d/yy") turned 2010 into 1910 -- in
// diagram_info and in project_summary_view, which the summary table reads.
// Runs a script through the real binary's --run on examples/741.qet, whose
// folio is dated 20100921, with LC_ALL=en_US.UTF-8.
class tst_dbfoliodate : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void folioDateKeepsItsCentury()
	{
		const QString path = m_dir.filePath(QStringLiteral("probe.js"));
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home);
		QFile f(path);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write("qet.log('PROBE ' + JSON.stringify("
				"qet.query('SELECT date FROM project_summary_view')));\n");
		f.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("TMPDIR"), m_dir.path());
		env.insert(QStringLiteral("LC_ALL"), QStringLiteral("en_US.UTF-8"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), path,
					QStringLiteral(QET_EXAMPLES_DIR "/741.qet")});
		QVERIFY(proc.waitForFinished(120000));
		const QString out = QString::fromUtf8(proc.readAllStandardOutput()
											  + proc.readAllStandardError());
		QJsonArray rows;
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(QStringLiteral("PROBE "));
			if (i >= 0)
				rows = QJsonDocument::fromJson(line.mid(i + 6).toUtf8()).array();
		}
		QCOMPARE(rows.size(), 1);
		QCOMPARE(rows.at(0).toObject().value(QStringLiteral("date")).toString(),
				 QStringLiteral("2010-09-21"));
	}
};

QTEST_APPLESS_MAIN(tst_dbfoliodate)

#include "tst_dbfoliodate.moc"
