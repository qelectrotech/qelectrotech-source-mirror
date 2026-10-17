// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// A folio report shows where its partner is. Changing any other
// information of the report wrote back the label stored in its
// information, the partner's position when the file was last saved, so
// the report pointed at the wrong place until the partner moved again
// (FINDINGS F116). Runs a script through the real binary's --run on
// examples/iso_sfc_example.qet: the "jump to" of folio 1 moves from
// column 4 to column 9, then the "jump from" on folio 2 gets a comment.
class tst_reporttextafterinfo : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

		// The text a symbol @p uuid of @p saved shows for its label
	static QString labelText(const QString &saved, const QString &uuid)
	{
		QFile file(saved);
		if (!file.open(QIODevice::ReadOnly)) return QStringLiteral("(not saved)");
		QDomDocument doc;
		if (!doc.setContent(&file)) return QStringLiteral("(not read)");
		const QDomNodeList elements = doc.elementsByTagName(QStringLiteral("element"));
		for (int i = 0 ; i < elements.size() ; ++i) {
			const QDomElement e = elements.at(i).toElement();
			if (e.attribute(QStringLiteral("uuid")) != uuid) continue;
			const QDomNodeList texts = e.elementsByTagName(QStringLiteral("dynamic_elmt_text"));
			for (int j = 0 ; j < texts.size() ; ++j) {
				const QDomElement t = texts.at(j).toElement();
				if (t.firstChildElement(QStringLiteral("info_name")).text() == QLatin1String("label"))
					return t.firstChildElement(QStringLiteral("text")).text();
			}
		}
		return QStringLiteral("(no such text)");
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void reportFollowsItsPartner_data()
	{
		QTest::addColumn<bool>("every_folio_built");
		QTest::newRow("folios built when needed") << false;
		QTest::newRow("every folio built") << true;
	}
	void reportFollowsItsPartner()
	{
		QFETCH(bool, every_folio_built);
		const QString tag = every_folio_built ? QStringLiteral("eager") : QStringLiteral("lazy");
		const QString saved = m_dir.filePath(tag + QStringLiteral(".qet"));
		const QString script = m_dir.filePath(tag + QStringLiteral(".js"));
		QFile f(script);
		QVERIFY(f.open(QIODevice::WriteOnly));
		f.write("qet.moveElement(0, '{bcf3d2e9-7190-48ed-bbb5-57bcc9ba8cfa}', 300, 0);\n"
				"qet.setElementInfo(1, '{016d0266-33ac-4255-8fe2-4d9629a12d86}', 'comment', 'edited');\n"
				"qet.save('" + saved.toUtf8() + "');\n");
		f.close();

			// Columns numbered from 1, as QElectroTech's settings page
			// shows by default. Written to an INI file QET_SETTINGS_DIR
			// points at: on Windows the settings are in the registry
			// otherwise, and the columns would count from 0
		const QString home = m_dir.filePath(tag + QStringLiteral("-home"));
		QVERIFY(QDir().mkpath(home + QStringLiteral("/settings/QElectroTech")));
		QFile conf(home + QStringLiteral("/settings/QElectroTech/QElectroTech.ini"));
		QVERIFY(conf.open(QIODevice::WriteOnly));
		conf.write("[General]\nborder-columns_0=false\n");
		conf.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("QET_SETTINGS_DIR"), home + QStringLiteral("/settings"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("TMPDIR"), m_dir.path());
		if (every_folio_built)
			env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		else
			env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), script,
					QStringLiteral(QET_EXAMPLES_DIR "/iso_sfc_example.qet")});
		QVERIFY(proc.waitForFinished(120000));

			// The partner, moved, and the report that shows where it is
		QCOMPARE(labelText(saved, QStringLiteral("{016d0266-33ac-4255-8fe2-4d9629a12d86}")),
				 QStringLiteral("1-C9"));
	}
};

QTEST_APPLESS_MAIN(tst_reporttextafterinfo)

#include "tst_reporttextafterinfo.moc"
