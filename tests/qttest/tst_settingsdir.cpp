// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>

// QET_SETTINGS_DIR makes QElectroTech keep its settings in an INI file in
// that folder (issue #1178). On Windows and macOS that is the only way to
// give a headless run its own settings: HOME and XDG_CONFIG_HOME move them
// on Linux alone. Checked here on Linux by making the usual settings file
// point at an empty collection and the one in QET_SETTINGS_DIR at a
// collection holding the symbol a script places: the symbol resolves only
// if the folder's file is the one read.
class tst_settingsdir : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	static void writeCollectionSetting(const QString &file, const QString &collection)
	{
		QDir().mkpath(QFileInfo(file).path());
		QFile f(file);
		QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Text));
		f.write("[elements-collections]\ncommon-collection-path=");
		f.write(collection.toUtf8());
		f.write("\n");
	}

	// Runs a script placing common://custom/my_siren.elmt and returns what it
	// logged: "resolved", "not resolved", or empty if the run itself failed.
	QString placeTheSymbol(bool with_settings_dir)
	{
		const QString root = m_dir.filePath(with_settings_dir ? QStringLiteral("with")
															  : QStringLiteral("without"));
		const QString home = root + QStringLiteral("/home");
		const QString settings = root + QStringLiteral("/settings");
		const QString collection = root + QStringLiteral("/collection");
		const QString empty_collection = root + QStringLiteral("/empty");
		QDir().mkpath(collection + QStringLiteral("/custom"));
		QDir().mkpath(empty_collection);
		QDir().mkpath(root + QStringLiteral("/tmp"));
		if (!QFile::copy(QStringLiteral(QET_ELEMENTS_DIR "/10_electric/10_allpole/380_signaling_operating/12_acoustic_signaling/sirene.elmt"),
						 collection + QStringLiteral("/custom/my_siren.elmt")))
			return QString();

		// The usual place (Linux): an empty collection. The folder: the right one.
		writeCollectionSetting(home + QStringLiteral("/.config/QElectroTech/QElectroTech.conf"),
							   empty_collection);
		writeCollectionSetting(settings + QStringLiteral("/QElectroTech/QElectroTech.ini"),
							   collection);

		const QString project = root + QStringLiteral("/p.qet");
		QFile::copy(QFINDTESTDATA("fixtures/qet_bug_repro_resaved.qet"), project);
		const QString script = root + QStringLiteral("/add.js");
		QFile js(script);
		if (!js.open(QIODevice::WriteOnly))
			return QString();
		js.write("var u = qet.addElement(0, 'common://custom/my_siren.elmt', 100, 100);\n"
				 "qet.log('RESULT ' + (u ? 'resolved' : 'not resolved'));\n");
		js.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/.config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/.local/share"));
		env.insert(QStringLiteral("TMPDIR"), root + QStringLiteral("/tmp"));
		if (with_settings_dir)
			env.insert(QStringLiteral("QET_SETTINGS_DIR"), settings);
		else
			env.remove(QStringLiteral("QET_SETTINGS_DIR"));

		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.setProcessChannelMode(QProcess::MergedChannels);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), script, project});
		if (!proc.waitForFinished(60000))
			return QString();
		const QString out = QString::fromUtf8(proc.readAll());
		static const QRegularExpression result(QStringLiteral("RESULT ([a-z ]+)"));
		return result.match(out).captured(1).trimmed();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void theFolderIsRead()
	{
		QCOMPARE(placeTheSymbol(true), QStringLiteral("resolved"));
	}

	// Without the variable nothing changes: the usual file is read, and it
	// points at the empty collection.
	void withoutItTheUsualPlaceIsRead()
	{
		QCOMPARE(placeTheSymbol(false), QStringLiteral("not resolved"));
	}
};

QTEST_APPLESS_MAIN(tst_settingsdir)

#include "tst_settingsdir.moc"
