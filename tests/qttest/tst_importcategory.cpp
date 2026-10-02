// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// Placing the same symbol twice in lmdg.qet: its embedded collection has a
// "k_elem" category before "import". Imported symbols were filed under the
// first category while every lookup is under "import/", so the second
// placement could not find the first copy, tried to import it again, and
// failed -- in a script (qet.addElement() returned "") and in the editor,
// where the drop silently placed nothing.
class tst_importcategory : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	// Run @p script on @p project in a sandbox of its own and return the
	// JSON object it logged.
	QJsonObject run(const QString &script, const QString &project)
	{
		const QString path = m_dir.filePath(QStringLiteral("probe.js"));
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home + QStringLiteral("/config/QElectroTech"));
			//The shipped collection, so common:// paths resolve; forward
			//slashes, as Qt reads a backslash here as an escape.
		QFile conf(home + QStringLiteral("/config/QElectroTech/QElectroTech.conf"));
		if (!conf.open(QIODevice::WriteOnly)) return {};
		conf.write("[elements-collections]\ncommon-collection-path="
				   QET_ELEMENTS_DIR "\n");
		conf.close();
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
		env.insert(QStringLiteral("TMPDIR"), m_dir.path());
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), path, project});
		if (!proc.waitForFinished(120000)) return {};
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

	void sameSymbolTwiceInLmdg()
	{
		const QString project = m_dir.filePath(QStringLiteral("lmdg.qet"));
		QVERIFY(QFile::copy(QStringLiteral(QET_EXAMPLES_DIR "/lmdg.qet"), project));

		const QJsonObject r = run(QStringLiteral(
			"var p = 'common://10_electric/10_allpole/310_relays_contactors_contacts/"
			"01_coils/bobine_ka_a_remanence.elmt';\n"
			"var a = qet.addElement(0, p, 100, 100);\n"
			"var b = qet.addElement(0, p, 300, 100);\n"
			"qet.log('PROBE ' + JSON.stringify({first: a, second: b}));\n"),
			project);
		QVERIFY2(!r.isEmpty(), "the script logged nothing");
		QVERIFY2(!r.value(QStringLiteral("first")).toString().isEmpty(),
				 "the first placement failed");
		QVERIFY2(!r.value(QStringLiteral("second")).toString().isEmpty(),
				 "the second placement of the same symbol failed");
	}
};

QTEST_GUILESS_MAIN(tst_importcategory)

#include "tst_importcategory.moc"
