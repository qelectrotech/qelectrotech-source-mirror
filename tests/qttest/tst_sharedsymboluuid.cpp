// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>
#include <QSet>

#include <QDomDocument>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// Symbols copied in old versions can share one uuid on a folio. Since every
// terminal got a uuid (#1118), the wires on them were saved by symbol uuid
// and reopened on the first symbol carrying it (#1408).
//
// Runs the real binary on examples/Habitat-Schemas_developpes.qet, whose
// folio 1 has three lamps L1, L2, L3, and compares the nets QElectroTech
// exports with those of the unedited example:
// - the lamps given one uuid, saved twice;
// - fixtures/shared_symbol_uuid_0200.qet: that folio as a build with #1118
//   saved it, its wires naming the lamps by the shared uuid.
namespace {

const QString example = QStringLiteral(QET_EXAMPLES_DIR "/Habitat-Schemas_developpes.qet");

QDomDocument load(const QString &path)
{
	QDomDocument doc;
	QFile file(path);
	if (file.open(QIODevice::ReadOnly))
		doc.setContent(&file);
	return doc;
}

QDomElement firstDiagram(const QDomDocument &doc)
{
	return doc.documentElement().firstChildElement(QStringLiteral("diagram"));
}

QList<QDomElement> childElements(const QDomElement &diagram, const QString &block,
							const QString &tag)
{
	QList<QDomElement> out;
	for (QDomElement e = diagram.firstChildElement(block).firstChildElement(tag);
		 !e.isNull(); e = e.nextSiblingElement(tag))
		out << e;
	return out;
}

QList<QDomElement> lamps(const QDomDocument &doc)
{
	QList<QDomElement> out;
	for (const QDomElement &e : childElements(firstDiagram(doc), QStringLiteral("elements"),
										 QStringLiteral("element")))
		if (e.attribute(QStringLiteral("type")).endsWith(QStringLiteral("lampe_pe.elmt")))
			out << e;
	return out;
}

} // namespace

class tst_sharedsymboluuid : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// Run the binary in a sandbox of its own, so a running QElectroTech
	// cannot answer instead.
	bool run(const QStringList &args)
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), args);
		return proc.waitForFinished(120000) && proc.exitCode() == 0;
	}

	QString write(const QDomDocument &doc)
	{
		const QString path = m_dir.filePath(QStringLiteral("in%1.qet").arg(m_run));
		QFile f(path);
		if (f.open(QIODevice::WriteOnly))
			f.write(doc.toByteArray());
		return path;
	}

	QString resave(const QString &in)
	{
		const QString out = m_dir.filePath(QStringLiteral("out%1.qet").arg(m_run));
		return run({QStringLiteral("--resave"), in, out}) ? out : QString();
	}

	// Folio 1's nets, each as its sorted symbol labels, sorted: the order
	// the export numbers them in follows the file and does not matter.
	QStringList nets(const QString &project)
	{
		const QString out = m_dir.filePath(QStringLiteral("nets%1.json").arg(m_run));
		if (!run({QStringLiteral("--export-nets"), project, out}))
			return {};
		QFile f(out);
		if (!f.open(QIODevice::ReadOnly))
			return {};
		QStringList result;
		const QJsonArray list = QJsonDocument::fromJson(f.readAll())
				.object().value(QStringLiteral("list")).toArray();
		for (const QJsonValue &net : list) {
			QStringList ends;
			for (const QJsonValue &t : net.toObject().value(QStringLiteral("terminals")).toArray()) {
				const QJsonObject end = t.toObject();
				if (end.value(QStringLiteral("folio")).toInt() == 1)
					ends << end.value(QStringLiteral("element")).toString();
			}
			ends.sort();
			if (!ends.isEmpty())
				result << ends.join(QLatin1Char(','));
		}
		result.sort();
		return result;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
		QCOMPARE(lamps(load(example)).size(), 3);
	}

	void sharedUuidSurvivesSaves()
	{
		const QStringList expected = nets(example);
		QVERIFY(expected.contains(QStringLiteral("L2,L3,S2")));

		QDomDocument doc = load(example);
		const QList<QDomElement> l = lamps(doc);
		const QString shared = l.first().attribute(QStringLiteral("uuid"));
		for (QDomElement e : l)
			e.setAttribute(QStringLiteral("uuid"), shared);

		const QString twice = resave(resave(write(doc)));
		QVERIFY(!twice.isEmpty());
		QCOMPARE(nets(twice), expected);

			//Each lamp has a uuid of its own after a save: one keeps the
			//shared uuid, the others get one derived from it
		QSet<QString> uuids;
		for (const QDomElement &e : lamps(load(twice)))
			uuids.insert(e.attribute(QStringLiteral("uuid")));
		QCOMPARE(uuids.size(), 3);
		QVERIFY(uuids.contains(shared));
	}

	// A linked contact and an unlinked symbol carrying the same uuid (a copy
	// made by an older version): the linked one keeps the uuid, the other
	// gets its own, and every link and wire stays where it was.
	void linkedCopyKeepsUuid()
	{
		const QString source = QStringLiteral(QET_EXAMPLES_DIR "/affuteuse_250h.qet");
		QDomDocument doc = load(source);
		QList<QDomElement> all;
		const QDomNodeList folios = doc.documentElement().elementsByTagName(QStringLiteral("diagram"));
		for (int i = 0; i < folios.size(); ++i)
			all << childElements(folios.at(i).toElement(), QStringLiteral("elements"),
								 QStringLiteral("element"));
		auto linked = [](const QDomElement &e) {
			return !e.firstChildElement(QStringLiteral("links_uuids"))
					.firstChildElement(QStringLiteral("link_uuid")).isNull();
		};
		QDomElement keeper, copy;
		for (const QDomElement &e : std::as_const(all)) {
			if (keeper.isNull() && linked(e)) keeper = e;
			else if (copy.isNull() && !linked(e)) copy = e;
		}
		QVERIFY(!keeper.isNull() && !copy.isNull());
		const QString shared = keeper.attribute(QStringLiteral("uuid"));
		copy.setAttribute(QStringLiteral("uuid"), shared);
		const int wires = doc.elementsByTagName(QStringLiteral("conductor")).size();

		const QString saved = resave(write(doc));
		QVERIFY(!saved.isEmpty());
		const QDomDocument out = load(saved);
		QHash<QString, QDomElement> by_uuid;
		QList<std::pair<QString, QString>> links;
		const QDomNodeList saved_folios = out.documentElement().elementsByTagName(QStringLiteral("diagram"));
		for (int i = 0; i < saved_folios.size(); ++i)
			for (const QDomElement &e : childElements(saved_folios.at(i).toElement(),
													  QStringLiteral("elements"),
													  QStringLiteral("element"))) {
				const QString u = e.attribute(QStringLiteral("uuid"));
				QVERIFY2(!by_uuid.contains(u), qPrintable(QStringLiteral("uuid still shared: ") + u));
				by_uuid.insert(u, e);
				for (QDomElement l = e.firstChildElement(QStringLiteral("links_uuids"))
						 .firstChildElement(QStringLiteral("link_uuid"));
					 !l.isNull(); l = l.nextSiblingElement(QStringLiteral("link_uuid")))
					links.append({u, l.attribute(QStringLiteral("uuid"))});
			}
		QVERIFY(by_uuid.contains(shared));
		QVERIFY(linked(by_uuid.value(shared)));
		for (const auto &link : std::as_const(links)) {
			QVERIFY2(by_uuid.contains(link.second), "a link names a symbol not in the project");
			QVERIFY2(links.contains(std::make_pair(link.second, link.first)), "a link only one side has");
		}
		QCOMPARE(out.elementsByTagName(QStringLiteral("conductor")).size(), wires);
	}

	void savedByUuidReopensRight()
	{
		const QString fixture = QFINDTESTDATA("fixtures/shared_symbol_uuid_0200.qet");
		QVERIFY(!fixture.isEmpty());
		QCOMPARE(nets(fixture), nets(example));
	}
};

QTEST_GUILESS_MAIN(tst_sharedsymboluuid)
#include "tst_sharedsymboluuid.moc"
