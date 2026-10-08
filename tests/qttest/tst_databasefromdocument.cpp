// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QDomDocument>
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

		// tremie_vibrante.qet saved once, as a document to change
	QDomDocument resaved(const QString &saved)
	{
		run({QStringLiteral("--resave"), QStringLiteral(QET_EXAMPLES_DIR "/tremie_vibrante.qet"), saved});
		QFile file(saved);
		QDomDocument document;
		if (file.open(QIODevice::ReadOnly))
			document.setContent(&file);
		return document;
	}

	static bool write(const QString &path, const QDomDocument &document)
	{
		QFile file(path);
		return file.open(QIODevice::WriteOnly | QIODevice::Truncate)
				&& file.write(document.toByteArray()) > 0;
	}

		// Both fills of @p saved, which must come from the document, agree.
	void compareBothWays(const QString &saved)
	{
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
		m_last = document;
	}

	QJsonObject m_last;   // the document's tables, from compareBothWays()

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
		compareBothWays(saved);
	}

	// A label or a conductor text made from a formula is what the formula
	// gives now, not what it gave when the file was saved: every folio's
	// first element and first conductor is given a formula using each kind
	// of variable, and a saved label no formula gives.
	void formulasAreWorkedOut()
	{
		const QString saved = m_dir.filePath(QStringLiteral("formulas-saved.qet"));
		run({QStringLiteral("--resave"), QStringLiteral(QET_EXAMPLES_DIR "/tremie_vibrante.qet"), saved});
		QFile file(saved);
		QVERIFY(file.open(QIODevice::ReadOnly));
		QDomDocument document;
		QVERIFY(document.setContent(&file));
		file.close();

		auto property = [&document](QDomElement parent, const QString &name, const QString &value) {
			QDomElement properties = parent.firstChildElement(QStringLiteral("properties"));
			if (properties.isNull())
				properties = parent.appendChild(document.createElement(QStringLiteral("properties"))).toElement();
			QDomElement p = document.createElement(QStringLiteral("property"));
			p.setAttribute(QStringLiteral("name"), name);
			p.appendChild(document.createTextNode(value));
			properties.appendChild(p);
		};
		auto sequence = [&document](QDomElement item, const QString &unit) {
			item.removeChild(item.firstChildElement(QStringLiteral("sequentialNumbers")));
			QDomElement s = document.createElement(QStringLiteral("sequentialNumbers"));
			QDomElement u = document.createElement(QStringLiteral("unit"));
			u.appendChild(document.createTextNode(unit));
			s.appendChild(u);
			item.appendChild(s);
		};

		property(document.documentElement(), QStringLiteral("site"), QStringLiteral("S"));
		const QDomNodeList diagrams = document.elementsByTagName(QStringLiteral("diagram"));
		QCOMPARE(diagrams.size(), 3);
		for (int i = 0 ; i < diagrams.size() ; ++i)
		{
			QDomElement diagram = diagrams.at(i).toElement();
			diagram.setAttribute(QStringLiteral("folio"), QStringLiteral("F%id"));
			diagram.setAttribute(QStringLiteral("plant"), QStringLiteral("P"));
			diagram.setAttribute(QStringLiteral("locmach"), QStringLiteral("L"));
			property(diagram, QStringLiteral("zone"), QStringLiteral("Z%1").arg(i));

			QDomElement element = diagram.firstChildElement(QStringLiteral("elements"))
									  .firstChildElement(QStringLiteral("element"));
			QVERIFY(!element.isNull());
			element.setAttribute(QStringLiteral("prefix"), QStringLiteral("X"));
			sequence(element, QStringLiteral("7"));
			QDomElement informations = element.firstChildElement(QStringLiteral("elementInformations"));
			if (informations.isNull())
				informations = element.appendChild(document.createElement(QStringLiteral("elementInformations"))).toElement();
			while (!informations.firstChild().isNull())
				informations.removeChild(informations.firstChild());
			for (const auto &info : {std::make_pair(QStringLiteral("formula"),
													QStringLiteral("K%total-%f-%F-%M-%LM-%c%l-%prefix-%{zone}-%{site}-%sequ_1")),
									 std::make_pair(QStringLiteral("label"), QStringLiteral("OLD"))}) {
				QDomElement e = document.createElement(QStringLiteral("elementInformation"));
				e.setAttribute(QStringLiteral("name"), info.first);
				e.setAttribute(QStringLiteral("show"), QStringLiteral("1"));
				e.appendChild(document.createTextNode(info.second));
				informations.appendChild(e);
			}

			QDomElement conductor = diagram.firstChildElement(QStringLiteral("conductors"))
										.firstChildElement(QStringLiteral("conductor"));
			QVERIFY(!conductor.isNull());
			conductor.setAttribute(QStringLiteral("formula"), QStringLiteral("W%total-%id-%wf-%{zone}-%sequ_1"));
			conductor.setAttribute(QStringLiteral("function"), QStringLiteral("N"));
			conductor.setAttribute(QStringLiteral("num"), QStringLiteral("OLD"));
			sequence(conductor, QStringLiteral("3"));
		}
		QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
		file.write(document.toByteArray());
		file.close();

		compareBothWays(saved);
			// ...and they were worked out, not left as saved
		const QString info = QString::fromUtf8(QJsonDocument(m_last.value(QStringLiteral("element_info")).toArray())
												   .toJson(QJsonDocument::Compact));
		const QString wires = QString::fromUtf8(QJsonDocument(m_last.value(QStringLiteral("conductor")).toArray())
													.toJson(QJsonDocument::Compact));
		QVERIFY2(info.contains(QStringLiteral("K3-1-F1-P-L-")), qPrintable(info.left(400)));
		QVERIFY2(info.contains(QStringLiteral("-X-Z0-S-7")), qPrintable(info.left(400)));
		QVERIFY2(wires.contains(QStringLiteral("W3-3-N-Z2-3")), qPrintable(wires.left(400)));
		QVERIFY(!info.contains(QStringLiteral("OLD")));
		QVERIFY(!wires.contains(QStringLiteral("OLD")));
	}

	// An element the folio does not build, and so the conductors ending on
	// it, are left out the same way.
	void unbuiltElementIsLeftOut()
	{
		const QString saved = m_dir.filePath(QStringLiteral("unbuilt.qet"));
		QDomDocument document = resaved(saved);
		QDomElement conductor = document.elementsByTagName(QStringLiteral("conductor")).at(0).toElement();
		QVERIFY(!conductor.isNull());
		const QString uuid = conductor.attribute(QStringLiteral("element1"));
		const QDomNodeList elements = document.elementsByTagName(QStringLiteral("element"));
		bool found = false;
		for (int i = 0 ; i < elements.size() ; ++i) {
			QDomElement e = elements.at(i).toElement();
			if (e.attribute(QStringLiteral("uuid")) == uuid) {
				e.setAttribute(QStringLiteral("x"), QStringLiteral("nan"));
				found = true;
			}
		}
		QVERIFY(found);
		QVERIFY(write(saved, document));
		compareBothWays(saved);
		const QString wires = QString::fromUtf8(QJsonDocument(m_last.value(QStringLiteral("conductor")).toArray())
													.toJson(QJsonDocument::Compact));
		QVERIFY(!wires.contains(conductor.attribute(QStringLiteral("uuid"))));
	}

	// Two elements on a folio numbering their terminals alike: which one the
	// folio then refuses depends on the terminals' geometry, so the folios
	// fill the database.
	void clashingTerminalIdsFallBack()
	{
		const QString saved = m_dir.filePath(QStringLiteral("clash.qet"));
		QDomDocument document = resaved(saved);
		const QDomElement diagram = document.elementsByTagName(QStringLiteral("diagram")).at(0).toElement();
		QDomElement first = diagram.firstChildElement(QStringLiteral("elements")).firstChildElement(QStringLiteral("element"));
		QDomElement second = first.nextSiblingElement(QStringLiteral("element"));
		const QString id = first.firstChildElement(QStringLiteral("terminals"))
							   .firstChildElement(QStringLiteral("terminal")).attribute(QStringLiteral("id"));
		QDomElement terminal = second.firstChildElement(QStringLiteral("terminals"))
								   .firstChildElement(QStringLiteral("terminal"));
		QVERIFY(!id.isEmpty() && !terminal.isNull());
		terminal.setAttribute(QStringLiteral("id"), id);
		QVERIFY(write(saved, document));
		QString how;
		dump(saved, false, &how);
		QCOMPARE(how, QStringLiteral("Project database filled from the folios: "
									 "two elements on a folio number their terminals alike"));
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
