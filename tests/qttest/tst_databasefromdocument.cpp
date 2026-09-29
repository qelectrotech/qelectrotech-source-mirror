// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// A project's database, filled from the file as it is read, holds exactly
// what the same database filled from the built folios holds. Every example
// is saved once (so that it carries the uuids a current QElectroTech
// writes), then opened twice through the real binary's --run: once as is,
// once with QET_DATABASE_FROM_FOLIOS=1, and the six tables compared.
class tst_databasefromdocument : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	QProcessEnvironment env()
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QProcessEnvironment e = QProcessEnvironment::systemEnvironment();
		e.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		e.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		e.insert(QStringLiteral("HOME"), home);
		e.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		e.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		e.insert(QStringLiteral("TMPDIR"), m_dir.path());
		e.remove(QStringLiteral("QET_DATABASE_FROM_FOLIOS"));
		return e;
	}

	QString run(const QStringList &args, bool from_folios = false)
	{
		QProcessEnvironment e = env();
		if (from_folios)
			e.insert(QStringLiteral("QET_DATABASE_FROM_FOLIOS"), QStringLiteral("1"));
		QProcess proc;
		proc.setProcessEnvironment(e);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), args);
		if (!proc.waitForFinished(180000)) return {};
		return QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
	}

	// The six tables, each as a sorted list of its rows, and which way the
	// database was filled.
	QJsonObject dump(const QString &project, bool from_folios, QString *how)
	{
		const QString out = run({QStringLiteral("--run"), m_dir.filePath(QStringLiteral("dump.js")),
								 project}, from_folios);
		QJsonObject tables;
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			if (line.contains(QStringLiteral("Project database filled")))
				*how = line.mid(line.indexOf(QStringLiteral("Project database filled")));
			const int i = line.indexOf(QStringLiteral("DUMP "));
			if (i >= 0) {
				const QJsonObject raw = QJsonDocument::fromJson(line.mid(i + 5).toUtf8()).object();
				for (auto it = raw.begin(); it != raw.end(); ++it) {
					QStringList rows;
					for (const QJsonValue &row : it.value().toArray())
						rows << QString::fromUtf8(QJsonDocument(row.toObject()).toJson(QJsonDocument::Compact));
					rows.sort();
					tables.insert(it.key(), QJsonArray::fromStringList(rows));
				}
			}
		}
		return tables;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
		QFile js(m_dir.filePath(QStringLiteral("dump.js")));
		QVERIFY(js.open(QIODevice::WriteOnly));
		js.write("var out = {};\n"
				 "['diagram', 'diagram_info', 'element', 'element_info', 'terminal', 'conductor']"
				 ".forEach(function (t) { out[t] = qet.query('SELECT * FROM ' + t); });\n"
				 "qet.log('DUMP ' + JSON.stringify(out));\n");
	}

	void sameTablesBothWays_data()
	{
		QTest::addColumn<QString>("project");
		const QDir examples(QStringLiteral(QET_EXAMPLES_DIR));
		for (const QString &f : examples.entryList({QStringLiteral("*.qet")}, QDir::Files, QDir::Name))
			QTest::newRow(f.toUtf8().constData()) << examples.filePath(f);
	}

	void sameTablesBothWays()
	{
		QFETCH(QString, project);
		const QString saved = m_dir.filePath(QStringLiteral("saved%1.qet").arg(m_run));
		run({QStringLiteral("--resave"), project, saved});
		QVERIFY2(QFile::exists(saved), "--resave failed");

		QString how_document, how_folios;
		const QJsonObject document = dump(saved, false, &how_document);
		const QJsonObject folios = dump(saved, true, &how_folios);
		QVERIFY2(how_document == QLatin1String("Project database filled from the document"),
				 qPrintable(how_document));
		QVERIFY2(how_folios.contains(QStringLiteral("QET_DATABASE_FROM_FOLIOS")), qPrintable(how_folios));
		QCOMPARE(document.keys().size(), 6);
		for (const QString &table : folios.keys()) {
			const QJsonArray a = document.value(table).toArray(), b = folios.value(table).toArray();
			QVERIFY2(a == b, qPrintable(QStringLiteral("%1: %2 rows from the document, %3 from the folios")
										.arg(table).arg(a.size()).arg(b.size())));
		}
	}

	// A file whose items carry no saved uuid is filled from the folios,
	// which work those uuids out as they are built, and says why.
	void olderFileFallsBack()
	{
		QString how;
		dump(QStringLiteral(QET_EXAMPLES_DIR "/tremie_vibrante.qet"), false, &how);
		QCOMPARE(how, QStringLiteral("Project database filled from the folios: a folio has no saved uuid"));
	}
};

QTEST_APPLESS_MAIN(tst_databasefromdocument)

#include "tst_databasefromdocument.moc"
