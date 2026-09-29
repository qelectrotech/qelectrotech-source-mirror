#include <QtTest>
#include <QTemporaryDir>

#include "utils/configprofile.h"

class tst_configprofile : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	QString path(const QString &name) const { return m_dir.filePath(name); }

private slots:
	void localKeys()
	{
		QVERIFY(ConfigProfile::isLocalKey("diagrameditor/geometry"));
		QVERIFY(ConfigProfile::isLocalKey("elementeditor/state"));
		QVERIFY(ConfigProfile::isLocalKey("projects-recentfiles/file1"));
		QVERIFY(ConfigProfile::isLocalKey(ConfigProfile::marker_key));
		QVERIFY(!ConfigProfile::isLocalKey("diagrameditor/Xgrid"));
		QVERIFY(!ConfigProfile::isLocalKey("shortcuts/mainwindow.fullscreen"));
		QVERIFY(!ConfigProfile::isLocalKey("masterpropertieswidget/plc-table-header-state"));
	}

	// Export leaves out window layout and recent files, and marks the file.
	void exportLeavesOutLocalKeys()
	{
		QSettings live(path("live1.conf"), QSettings::IniFormat);
		live.setValue("diagrameditor/Xgrid", 7);
		live.setValue("lang", "de");
		live.setValue("diagrameditor/geometry", QByteArray("xyz"));
		live.setValue("projects-recentfiles/file1", "/home/a/b.qet");

		QSettings file(path("profile1.conf"), QSettings::IniFormat);
		QCOMPARE(ConfigProfile::exportTo(live, file), 2);
		QVERIFY(ConfigProfile::isProfile(file));
		QCOMPARE(file.value("diagrameditor/Xgrid").toInt(), 7);
		QVERIFY(!file.contains("diagrameditor/geometry"));
		QVERIFY(!file.contains("projects-recentfiles/file1"));
	}

	// Import replaces every setting, removes the ones the profile does not
	// have, keeps window layout and recent files, and never copies the marker.
	void importReplacesButKeepsLocal()
	{
		QSettings file(path("profile2.conf"), QSettings::IniFormat);
		file.setValue("diagrameditor/Xgrid", 5);
		file.setValue("diagrameditor/geometry", QByteArray("from-file"));
		file.setValue(ConfigProfile::marker_key, 1);

		QSettings live(path("live2.conf"), QSettings::IniFormat);
		live.setValue("diagrameditor/Xgrid", 10);
		live.setValue("lang", "fr");
		live.setValue("diagrameditor/geometry", QByteArray("mine"));
		live.setValue("projects-recentfiles/file1", "/home/a/b.qet");

		ConfigProfile::importFrom(file, live);
		QCOMPARE(live.value("diagrameditor/Xgrid").toInt(), 5);
		QVERIFY(!live.contains("lang"));
		QCOMPARE(live.value("diagrameditor/geometry").toByteArray(), QByteArray("mine"));
		QCOMPARE(live.value("projects-recentfiles/file1").toString(), QString("/home/a/b.qet"));
		QVERIFY(!live.contains(ConfigProfile::marker_key));
	}

	// Values survive the round trip with their types, including arrays.
	void roundTrip()
	{
		QSettings a(path("a.conf"), QSettings::IniFormat);
		a.setValue("print/default/fitinpage", true);
		a.setValue("diagrameditor/sheet_background_color", QColor(12, 34, 56));
		a.beginWriteArray("diagrameditor/defaultguides", 2);
		a.setArrayIndex(0); a.setValue("pos", 100);
		a.setArrayIndex(1); a.setValue("pos", 250);
		a.endArray();

		QSettings file(path("profile3.conf"), QSettings::IniFormat);
		ConfigProfile::exportTo(a, file);
		QSettings b(path("b.conf"), QSettings::IniFormat);
		b.setValue("stray", 1);
		ConfigProfile::importFrom(file, b);

		QStringList ka = a.allKeys(), kb = b.allKeys();
		ka.sort(); kb.sort();
		QCOMPARE(kb, ka);
		for (const QString &k : std::as_const(ka))
			QCOMPARE(b.value(k), a.value(k));
	}

	void plainFileIsNotAProfile()
	{
		QSettings other(path("other.conf"), QSettings::IniFormat);
		other.setValue("some/key", 1);
		QVERIFY(!ConfigProfile::isProfile(other));
	}
};

QTEST_GUILESS_MAIN(tst_configprofile)

#include "tst_configprofile.moc"
