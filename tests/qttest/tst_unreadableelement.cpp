// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// qet.addElement() compares the uuid of the symbol it is given with the copy
// already embedded under the same name. A symbol file that exists but cannot
// be read has no uuid, and was reported as "would collide with a different
// element" (#1178 follow-up: on Windows, any symbol whose full path reaches
// 260 characters). Made unreadable here with the file's permissions, which
// gives the same null uuid on Linux.
class tst_unreadableelement : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	QString m_collection;
	QString m_symbol;

	// Runs a script placing common://custom/my_siren.elmt into project and
	// saving it to output; returns everything the run printed.
	QString placeTheSymbol(const QString &project, const QString &output)
	{
		const QString root = m_dir.path();
		const QString home = root + QStringLiteral("/home");
		const QString settings = root + QStringLiteral("/settings");
		QDir().mkpath(settings + QStringLiteral("/QElectroTech"));
		QDir().mkpath(root + QStringLiteral("/tmp"));
		QFile ini(settings + QStringLiteral("/QElectroTech/QElectroTech.ini"));
		if (!ini.open(QIODevice::WriteOnly | QIODevice::Text))
			return QString();
		ini.write("[elements-collections]\ncommon-collection-path=");
		ini.write(m_collection.toUtf8());
		ini.write("\n");
		ini.close();

		const QString script = root + QStringLiteral("/add.js");
		QFile js(script);
		if (!js.open(QIODevice::WriteOnly))
			return QString();
		js.write("var u = qet.addElement(0, 'common://custom/my_siren.elmt', 100, 100);\n"
				 "qet.log('RESULT ' + (u ? 'placed' : 'refused'));\n"
				 "if (u) qet.save('");
		js.write(output.toUtf8());
		js.write("');\n");
		js.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/.config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/.local/share"));
		env.insert(QStringLiteral("TMPDIR"), root + QStringLiteral("/tmp"));
		env.insert(QStringLiteral("QET_SETTINGS_DIR"), settings);

		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.setProcessChannelMode(QProcess::MergedChannels);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), script, project});
		if (!proc.waitForFinished(60000))
			return QString();
		return QString::fromUtf8(proc.readAll());
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
		m_collection = m_dir.filePath(QStringLiteral("collection"));
		m_symbol = m_collection + QStringLiteral("/custom/my_siren.elmt");
		QVERIFY(QDir().mkpath(m_collection + QStringLiteral("/custom")));
		QVERIFY(QFile::copy(QStringLiteral(QET_ELEMENTS_DIR "/10_electric/10_allpole/380_signaling_operating/12_acoustic_signaling/sirene.elmt"),
							m_symbol));
		QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/qet_bug_repro_resaved.qet"),
							m_dir.filePath(QStringLiteral("p.qet"))));
	}

	void anUnreadableSymbolIsNotACollision()
	{
		// Placed once while readable: the project now embeds it.
		const QString embedded = m_dir.filePath(QStringLiteral("embedded.qet"));
		QVERIFY(placeTheSymbol(m_dir.filePath(QStringLiteral("p.qet")), embedded)
				.contains(QStringLiteral("RESULT placed")));
		QVERIFY(QFile::exists(embedded));

#ifdef Q_OS_WIN
		// Qt cannot take read permission away on Windows: it only knows the
		// read-only flag there, so this returns false. The check needs a
		// file the user cannot read, which Windows cannot give it.
		QSKIP("Windows cannot make the file unreadable through QFile");
#endif
		QVERIFY(QFile::setPermissions(m_symbol, QFileDevice::Permissions()));
		QFile probe(m_symbol);
		if (probe.open(QIODevice::ReadOnly))
			QSKIP("permissions do not stop this user reading the file (root?)");

		const QString out = placeTheSymbol(embedded, m_dir.filePath(QStringLiteral("again.qet")));
		QFile::setPermissions(m_symbol, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
		QVERIFY2(out.contains(QStringLiteral("RESULT refused")), qPrintable(out));
		QVERIFY2(out.contains(QStringLiteral("could not read element")), qPrintable(out));
		QVERIFY2(!out.contains(QStringLiteral("would collide")), qPrintable(out));
	}

	// Readable again, the same symbol is placed a second time as before.
	void theSameSymbolIsPlacedAgain()
	{
		const QString out = placeTheSymbol(m_dir.filePath(QStringLiteral("embedded.qet")),
										   m_dir.filePath(QStringLiteral("twice.qet")));
		QVERIFY2(out.contains(QStringLiteral("RESULT placed")), qPrintable(out));
	}
};

QTEST_APPLESS_MAIN(tst_unreadableelement)

#include "tst_unreadableelement.moc"
