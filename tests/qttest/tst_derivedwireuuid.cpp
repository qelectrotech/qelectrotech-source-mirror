// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDomDocument>
#include <QFile>
#include <QHash>
#include <QMap>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QUuid>

// A wire saved without a uuid must get one that is the same on every load,
// is written on save, and survives the edits people make before that save:
// inserting a folio, saving the wires in another order, drawing the wire
// the other way round. Until this was fixed it got a random uuid on every
// load, never saved (#754).
//
// Runs the real binary (--resave) on two fixtures -- one whose wires name
// their ends by symbol and terminal uuid, one older file naming them by
// terminal number -- and reads the wires' uuids back from the saved file.
namespace {

QDomDocument load(const QString &path)
{
	QDomDocument doc;
	QFile file(path);
	if (file.open(QIODevice::ReadOnly))
		doc.setContent(&file);
	return doc;
}

QList<QDomElement> diagrams(const QDomDocument &doc)
{
	QList<QDomElement> out;
	for (QDomElement d = doc.documentElement().firstChildElement(QStringLiteral("diagram"));
		 !d.isNull(); d = d.nextSiblingElement(QStringLiteral("diagram")))
		out << d;
	return out;
}

QList<QDomElement> wires(const QDomElement &diagram)
{
	QList<QDomElement> out;
	const QDomElement block = diagram.firstChildElement(QStringLiteral("conductors"));
	for (QDomElement c = block.firstChildElement(QStringLiteral("conductor"));
		 !c.isNull(); c = c.nextSiblingElement(QStringLiteral("conductor")))
		out << c;
	return out;
}

// Every wire's uuid in the file, sorted: equal lists mean every wire kept
// its uuid.
QStringList wireUuids(const QDomDocument &doc)
{
	QStringList out;
	for (const QDomElement &d : diagrams(doc))
		for (const QDomElement &c : wires(d))
			out << c.attribute(QStringLiteral("uuid"));
	out.sort();
	return out;
}

// Which wire carries which uuid: each wire keyed by its two ends as the
// saved file names them, sorted. An older file names an end by terminal
// number, resolved here to the symbol and the terminal's place on it.
QMap<QString, QString> wireMap(const QDomDocument &doc)
{
	QMap<QString, QString> out;
	for (const QDomElement &d : diagrams(doc)) {
		QHash<QString, QString> by_number;
		const QDomNodeList symbols = d.firstChildElement(QStringLiteral("elements"))
				.elementsByTagName(QStringLiteral("element"));
		for (int i = 0; i < symbols.size(); ++i) {
			const QDomElement e = symbols.at(i).toElement();
			const QDomNodeList ts = e.elementsByTagName(QStringLiteral("terminal"));
			for (int j = 0; j < ts.size(); ++j) {
				const QDomElement t = ts.at(j).toElement();
				by_number.insert(t.attribute(QStringLiteral("id")),
								 e.attribute(QStringLiteral("uuid")) + QLatin1Char('/')
								 + t.attribute(QStringLiteral("x")) + QLatin1Char(',')
								 + t.attribute(QStringLiteral("y")));
			}
		}
		for (const QDomElement &c : wires(d)) {
			QStringList ends;
			for (const QString n : {QStringLiteral("1"), QStringLiteral("2")}) {
				const QString element = c.attribute(QStringLiteral("element") + n);
				const QString terminal = c.attribute(QStringLiteral("terminal") + n);
				ends << (element.isEmpty() ? by_number.value(terminal, QStringLiteral("?"))
										   : element + QLatin1Char('/') + terminal);
			}
			ends.sort();
			out.insert(ends.join(QLatin1Char('|')), c.attribute(QStringLiteral("uuid")));
		}
	}
	return out;
}

} // namespace

class tst_derivedwireuuid : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// Save @p doc, run --resave on it in a sandbox of its own (so a running
	// QElectroTech cannot answer instead), and return what was saved.
	QDomDocument resave(const QDomDocument &doc)
	{
		const QString in = m_dir.filePath(QStringLiteral("in%1.qet").arg(m_run));
		const QString out = m_dir.filePath(QStringLiteral("out%1.qet").arg(m_run));
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QFile f(in);
		if (!f.open(QIODevice::WriteOnly)) return {};
		f.write(doc.toByteArray());
		f.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--resave"), in, out});
		if (!proc.waitForFinished(120000) || proc.exitCode() != 0) return {};
		return load(out);
	}

	// The fixture as an older QElectroTech saved it: wires without uuids.
	QDomDocument fixture()
	{
		QFETCH(QString, path);
		QDomDocument doc = load(path);
		for (const QDomElement &d : diagrams(doc))
			for (QDomElement c : wires(d))
				c.removeAttribute(QStringLiteral("uuid"));
		return doc;
	}

	// The saved uuids of the unedited fixture, checked for sanity.
	QStringList reference()
	{
		QFETCH(int, count);
		const QStringList ids = wireUuids(resave(fixture()));
		if (ids.size() != count) return {};
		for (const QString &u : ids)
			if (QUuid(u).isNull()) return {};
		return ids;
	}

	void fixtures()
	{
		QTest::addColumn<QString>("path");
		QTest::addColumn<int>("count");
		QTest::newRow("ends by uuid")
				<< QFINDTESTDATA("fixtures/qet_bug_repro_resaved.qet") << 7;
		QTest::newRow("ends by terminal number (older file)")
				<< QStringLiteral(QET_EXAMPLES_DIR "/tremie_vibrante.qet") << 77;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void savedAndUnique_data() { fixtures(); }
	void savedAndUnique()
	{
		QFETCH(int, count);
		const QStringList ids = reference();
		QCOMPARE(ids.size(), count);                        // every wire has one
		QCOMPARE(QSet<QString>(ids.begin(), ids.end()).size(), count);  // all different
		QCOMPARE(wireMap(resave(fixture())).size(), count);  // the test's wire key is unique too
	}

	void sameOnEveryLoad_data() { fixtures(); }
	void sameOnEveryLoad()
	{
		const QStringList a = reference();
		QVERIFY(!a.isEmpty());
		QCOMPARE(wireUuids(resave(fixture())), a);
	}

	void savedUuidIsReadBack_data() { fixtures(); }
	void savedUuidIsReadBack()
	{
		const QDomDocument once = resave(fixture());
		QVERIFY(!wireUuids(once).isEmpty());
		QCOMPARE(wireUuids(resave(once)), wireUuids(once));
	}

	void insertingAFolioChangesNothing_data() { fixtures(); }
	void insertingAFolioChangesNothing()
	{
		QVERIFY(!reference().isEmpty());
		const QMap<QString, QString> before = wireMap(resave(fixture()));
		QDomDocument doc = fixture();
		const QDomElement first = diagrams(doc).first();
		QDomElement blank = first.cloneNode(false).toElement();
		blank.setAttribute(QStringLiteral("title"), QStringLiteral("new"));
		blank.removeAttribute(QStringLiteral("uuid"));
		blank.appendChild(doc.createElement(QStringLiteral("elements")));
		doc.documentElement().insertBefore(blank, first);
		QCOMPARE(wireMap(resave(doc)), before);
	}

	void wireOrderDoesNotMatter_data() { fixtures(); }
	void wireOrderDoesNotMatter()
	{
		QVERIFY(!reference().isEmpty());
		const QMap<QString, QString> before = wireMap(resave(fixture()));
		QDomDocument doc = fixture();
		for (const QDomElement &d : diagrams(doc)) {
			QDomElement block = d.firstChildElement(QStringLiteral("conductors"));
			const QList<QDomElement> ws = wires(d);
			for (const QDomElement &w : ws)
				block.removeChild(w);
			for (auto it = ws.crbegin(); it != ws.crend(); ++it)   // reversed
				block.appendChild(*it);
		}
		QCOMPARE(wireMap(resave(doc)), before);
	}

	void directionDoesNotMatter_data() { fixtures(); }
	void directionDoesNotMatter()
	{
		QVERIFY(!reference().isEmpty());
		const QMap<QString, QString> before = wireMap(resave(fixture()));
		QDomDocument doc = fixture();
		static const QRegularExpression pair(
					QStringLiteral("^(element|terminal|terminalname)([12])(.*)$"));
		for (const QDomElement &d : diagrams(doc)) {
			for (QDomElement w : wires(d)) {
				const QDomNamedNodeMap attrs = w.attributes();
				QHash<QString, QString> swapped;
				for (int i = 0; i < attrs.size(); ++i) {
					const QString name = attrs.item(i).nodeName();
					const auto m = pair.match(name);
					if (m.hasMatch())
						swapped.insert(m.captured(1) + (m.captured(2) == QLatin1String("1")
														? QStringLiteral("2") : QStringLiteral("1"))
									   + m.captured(3),
									   attrs.item(i).nodeValue());
				}
				for (auto it = swapped.cbegin(); it != swapped.cend(); ++it)
					w.setAttribute(it.key(), it.value());
			}
		}
		QCOMPARE(wireMap(resave(doc)), before);
	}

	// A wire keeps its uuid once saved, even when re-connected. A wire saved
	// without a uuid that later turns up on the ends it left (a hand edit, an
	// older version, another tool) would derive the same uuid: it must get
	// another one instead.
	void newcomerOnAReconnectedWiresEndsGetsAnotherUuid_data() { fixtures(); }
	void newcomerOnAReconnectedWiresEndsGetsAnotherUuid()
	{
		QFETCH(int, count);
		QDomDocument doc = resave(fixture());
		QVERIFY(!wireUuids(doc).isEmpty());
		const QList<QDomElement> ws = wires(diagrams(doc).first());
		QVERIFY(ws.size() >= 2);
		QDomElement x = ws.at(0);
		const QDomElement y = ws.at(1);
		QDomElement newcomer = x.cloneNode(true).toElement();   // x's old ends
		newcomer.removeAttribute(QStringLiteral("uuid"));
			// re-connect x's second end to y's second end, keeping x's uuid
		static const QRegularExpression end2(QStringLiteral("^(element|terminal|terminalname)2"));
		const QDomNamedNodeMap attrs = y.attributes();
		for (int i = 0; i < attrs.size(); ++i)
			if (end2.match(attrs.item(i).nodeName()).hasMatch())
				x.setAttribute(attrs.item(i).nodeName(), attrs.item(i).nodeValue());
		x.parentNode().appendChild(newcomer);

		const QStringList after = wireUuids(resave(doc));
		QCOMPARE(after.size(), count + 1);
		QCOMPARE(QSet<QString>(after.begin(), after.end()).size(), count + 1);
	}

	// QElectroTech refuses a second wire between two terminals already joined
	// (Terminal::canBeLinkedTo()), so two wires on one folio never share both
	// ends. A file that has one anyway must still load to the same uuids.
	void doubledWireIsDroppedAndOthersKeepTheirs_data() { fixtures(); }
	void doubledWireIsDroppedAndOthersKeepTheirs()
	{
		QVERIFY(!reference().isEmpty());
		const QMap<QString, QString> before = wireMap(resave(fixture()));
		QDomDocument doc = fixture();
		QDomElement w = wires(diagrams(doc).first()).first();
		w.parentNode().appendChild(w.cloneNode(true));
		QCOMPARE(wireMap(resave(doc)), before);
	}
};

QTEST_APPLESS_MAIN(tst_derivedwireuuid)

#include "tst_derivedwireuuid.moc"
