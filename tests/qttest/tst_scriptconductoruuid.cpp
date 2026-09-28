// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include <QUuid>

// qet.conductorUuids(folio) and qet.conductorEnds(folio, uuid): a script can
// list a folio's conductors by uuid and find where each one runs, in the
// "{element} terminal N" form conductors() prints and the conductor calls
// take. Runs a script through the real binary's --run.
class tst_scriptconductoruuid : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	// Run @p script on the fixture in a sandbox of its own and return the
	// JSON object it logged.
	QJsonObject run(const QString &script)
	{
		const QString path = m_dir.filePath(QStringLiteral("probe.js"));
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home);
		QFile f(path);
		if (!f.open(QIODevice::WriteOnly)) return {};
		f.write(script.toUtf8());
		f.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), path,
					QFINDTESTDATA("fixtures/qet_bug_repro_resaved.qet")});
		if (!proc.waitForFinished(60000)) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput()
											  + proc.readAllStandardError());
		const QString mark = QStringLiteral("PROBE ");
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(mark);
			if (i >= 0)
				return QJsonDocument::fromJson(line.mid(i + mark.size()).toUtf8()).object();
		}
		return {};
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void uuidsAndEndsMatchConductors()
	{
		const QJsonObject r = run(QStringLiteral(
			"var uuids = qet.conductorUuids(0);\n"
			"var ends = uuids.map(function (u) { return qet.conductorEnds(0, u); });\n"
			"qet.log('PROBE ' + JSON.stringify({lines: qet.conductors(0), uuids: uuids, ends: ends,\n"
			"  unknown: qet.conductorEnds(0, '{00000000-0000-0000-0000-000000000001}'),\n"
			"  junk: qet.conductorEnds(0, 'not a uuid'),\n"
			"  badFolio: qet.conductorUuids(99)}));\n"));
		QVERIFY2(!r.isEmpty(), "the script logged nothing");

		const QJsonArray lines = r.value(QStringLiteral("lines")).toArray();
		const QJsonArray uuids = r.value(QStringLiteral("uuids")).toArray();
		const QJsonArray ends = r.value(QStringLiteral("ends")).toArray();
		QCOMPARE(lines.size(), 7);                    // the fixture's conductors
		QCOMPARE(uuids.size(), lines.size());
		QSet<QString> distinct;
		for (int i = 0; i < uuids.size(); ++i) {
			const QString u = uuids.at(i).toString();
			QVERIFY2(!QUuid(u).isNull(), qPrintable(u));
			distinct.insert(u);
				// same order as conductors(), and the same two ends it prints
			const QJsonArray e = ends.at(i).toArray();
			QCOMPARE(e.size(), 2);
			const QString expected = e.at(0).toString() + QStringLiteral(" -- ")
					+ e.at(1).toString() + QStringLiteral(" : ");
			QVERIFY2(lines.at(i).toString().startsWith(expected),
					 qPrintable(lines.at(i).toString() + QStringLiteral(" | ") + expected));
		}
		QCOMPARE(distinct.size(), uuids.size());

		QVERIFY(r.value(QStringLiteral("unknown")).toArray().isEmpty());
		QVERIFY(r.value(QStringLiteral("junk")).toArray().isEmpty());
		QVERIFY(r.value(QStringLiteral("badFolio")).toArray().isEmpty());
	}
};

QTEST_APPLESS_MAIN(tst_scriptconductoruuid)

#include "tst_scriptconductoruuid.moc"
