#include <QtTest>
#include <QTemporaryDir>

#include "utils/configprofile.h"
#include "borderproperties.h"

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
		QVERIFY(ConfigProfile::isLocalKey("dialoggeometry/ConfigDialog"));
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
		live.setValue("dialoggeometry/ConfigDialog", QByteArray("xyz"));

		QSettings file(path("profile1.conf"), QSettings::IniFormat);
		QCOMPARE(ConfigProfile::exportTo(live, file), 2);
		QVERIFY(ConfigProfile::isProfile(file));
		QCOMPARE(file.value("diagrameditor/Xgrid").toInt(), 7);
		QVERIFY(!file.contains("diagrameditor/geometry"));
		QVERIFY(!file.contains("projects-recentfiles/file1"));
		QVERIFY(!file.contains("dialoggeometry/ConfigDialog"));
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

	// Every key belongs to one part; keys nobody listed are Other.
	void partOfKeys()
	{
		using P = ConfigProfile::Part;
		QCOMPARE(ConfigProfile::partOf("shortcuts/mainwindow.fullscreen"), P::Controls);
		QCOMPARE(ConfigProfile::partOf("toolbars/icon_size"), P::Controls);
		QCOMPARE(ConfigProfile::partOf("diagrameditor/toolbars/diagram"), P::Controls);
		QCOMPARE(ConfigProfile::partOf("diagrameditor/custom_toolbars/names"), P::Controls);
		QCOMPARE(ConfigProfile::partOf("diagrameditor/shortcut_bar/canvas"), P::Controls);
		QCOMPARE(ConfigProfile::partOf("diagrameditor/gestures/directions"), P::Controls);
		QCOMPARE(ConfigProfile::partOf("diagrameditor/mouse_gestures"), P::Controls);
		QCOMPARE(ConfigProfile::partOf("diagrameditor/context_toolbar"), P::Controls);
		QCOMPARE(ConfigProfile::partOf("diagramview/gestures"), P::Controls);

		QCOMPARE(ConfigProfile::partOf("diagrameditor/defaultconductortype"), P::NewProject);
		QCOMPARE(ConfigProfile::partOf("diagrameditor/defaultreportlabel"), P::NewProject);
		QCOMPARE(ConfigProfile::partOf("diagrameditor/defaultxrefcoil"), P::NewProject);
		QCOMPARE(ConfigProfile::partOf("diagrameditor/defaultguides/1/position"), P::NewProject);
		QCOMPARE(ConfigProfile::partOf("autonum/conductor/current"), P::NewProject);

		QCOMPARE(ConfigProfile::partOf("elements-collections/custom-collection-path"), P::Folders);
		QCOMPARE(ConfigProfile::partOf("elements-collections/macros-path"), P::Folders);

		QCOMPARE(ConfigProfile::partOf("diagrameditor/Xgrid"), P::Other);
		QCOMPARE(ConfigProfile::partOf("diagrameditor/auto_break_conductor"), P::Other);
		QCOMPARE(ConfigProfile::partOf("lang"), P::Other);
		QCOMPARE(ConfigProfile::partOf("customColors"), P::Other);
	}

	// What the New project page writes for the folio size is the
	// NewProject part, whatever key names BorderProperties uses.
	void folioDefaultsAreNewProject()
	{
		QSettings live(path("live_border.conf"), QSettings::IniFormat);
		BorderProperties().toSettings(live, "diagrameditor/default");
		const QStringList keys = live.allKeys();
		QVERIFY(!keys.isEmpty());
		for (const QString &key : keys)
			QCOMPARE(ConfigProfile::partOf(key), ConfigProfile::Part::NewProject);
	}

	// A file of some parts holds only their keys, never the folders, and
	// is format 2 so that an older QElectroTech refuses it.
	void exportSomeParts()
	{
		QSettings live(path("live4.conf"), QSettings::IniFormat);
		live.setValue("shortcuts/mainwindow.fullscreen", "F11");
		live.setValue("diagrameditor/defaultconductortype", "Multi");
		live.setValue("diagrameditor/Xgrid", 7);
		live.setValue("elements-collections/custom-collection-path", "/home/a/sym");

		QSettings file(path("profile4.conf"), QSettings::IniFormat);
		QCOMPARE(ConfigProfile::exportTo(live, file, {ConfigProfile::Part::NewProject}), 1);
		QCOMPARE(file.value(ConfigProfile::marker_key).toInt(), 2);
		QVERIFY(ConfigProfile::isProfile(file));
		QCOMPARE(ConfigProfile::partsOf(file), QList<ConfigProfile::Part>{ConfigProfile::Part::NewProject});
		QVERIFY(file.contains("diagrameditor/defaultconductortype"));
		QVERIFY(!file.contains("shortcuts/mainwindow.fullscreen"));
		QVERIFY(!file.contains("diagrameditor/Xgrid"));
		QVERIFY(!file.contains("elements-collections/custom-collection-path"));

			//Other without the two others: still no folders
		QSettings file2(path("profile4b.conf"), QSettings::IniFormat);
		ConfigProfile::exportTo(live, file2, {ConfigProfile::Part::Other});
		QVERIFY(file2.contains("diagrameditor/Xgrid"));
		QVERIFY(!file2.contains("elements-collections/custom-collection-path"));
	}

	// Every choosable part is a complete profile: format 1, folders
	// included, readable by the QElectroTech that only knows format 1.
	void exportEveryPartIsComplete()
	{
		QSettings live(path("live5.conf"), QSettings::IniFormat);
		live.setValue("diagrameditor/Xgrid", 7);
		live.setValue("elements-collections/custom-collection-path", "/home/a/sym");

		QSettings file(path("profile5.conf"), QSettings::IniFormat);
		ConfigProfile::exportTo(live, file, ConfigProfile::choosableParts());
		QCOMPARE(file.value(ConfigProfile::marker_key).toInt(), 1);
		QVERIFY(!file.contains(ConfigProfile::parts_key));
		QVERIFY(file.contains("elements-collections/custom-collection-path"));
		QCOMPARE(ConfigProfile::partsOf(file).size(), 4);
	}

	// Loading some parts replaces those parts only, removes their keys the
	// file lacks, and keeps every other setting, folders included.
	void importSomePartsKeepsTheRest()
	{
		QSettings file(path("profile6.conf"), QSettings::IniFormat);
		file.setValue("diagrameditor/defaultconductortype", "Single");
		file.setValue("diagrameditor/Xgrid", 99);	// not in its parts: ignored
		file.setValue(ConfigProfile::marker_key, 2);
		file.setValue(ConfigProfile::parts_key, "newproject");

		QSettings live(path("live6.conf"), QSettings::IniFormat);
		live.setValue("diagrameditor/defaultconductortype", "Multi");
		live.setValue("autonum/folio/current", "mine");
		live.setValue("diagrameditor/Xgrid", 10);
		live.setValue("shortcuts/mainwindow.fullscreen", "F11");
		live.setValue("elements-collections/custom-collection-path", "/home/me/sym");

		ConfigProfile::importFrom(file, live);
		QCOMPARE(live.value("diagrameditor/defaultconductortype").toString(), QString("Single"));
		QVERIFY(!live.contains("autonum/folio/current"));
		QCOMPARE(live.value("diagrameditor/Xgrid").toInt(), 10);
		QCOMPARE(live.value("shortcuts/mainwindow.fullscreen").toString(), QString("F11"));
		QCOMPARE(live.value("elements-collections/custom-collection-path").toString(),
				 QString("/home/me/sym"));
		QVERIFY(!live.contains(ConfigProfile::parts_key));
	}

	void restartOnlyWhenNeeded()
	{
		using P = ConfigProfile::Part;
		QVERIFY(!ConfigProfile::needsRestart({P::NewProject}));
		QVERIFY(ConfigProfile::needsRestart({P::NewProject, P::Controls}));
		QVERIFY(ConfigProfile::needsRestart({P::Other}));
	}

	// A file of format 2 naming no known part is refused by the caller.
	void unknownPartsGiveNothing()
	{
		QSettings file(path("profile7.conf"), QSettings::IniFormat);
		file.setValue(ConfigProfile::marker_key, 2);
		file.setValue(ConfigProfile::parts_key, "colours");
		QVERIFY(ConfigProfile::partsOf(file).isEmpty());
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
