// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDomDocument>
#include <QFile>
#include <QHash>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSet>
#include <QTemporaryDir>
#include <QUuid>

// A symbol saved without a uuid must get the same one on every load of the
// same file, and inserting a folio in front of it must not change it. Until
// this was fixed it got a random uuid each time (Element::fromXml), which
// the next save wrote out.
//
// Runs the real binary (--resave) on the fixture with its symbols' uuids
// stripped, and reads the uuids back from the saved file.
namespace {

QDomDocument load(const QString &path)
{
	QDomDocument doc;
	QFile file(path);
	if (file.open(QIODevice::ReadOnly))
		doc.setContent(&file);
	return doc;
}

QList<QDomElement> symbols(const QDomElement &diagram)
{
	QList<QDomElement> out;
	const QDomNodeList nodes = diagram.firstChildElement(QStringLiteral("elements"))
			.elementsByTagName(QStringLiteral("element"));
	for (int i = 0; i < nodes.size(); ++i) {
		const QDomElement e = nodes.at(i).toElement();
		if (e.parentNode().parentNode() == diagram)
			out << e;
	}
	return out;
}

QList<QDomElement> diagrams(const QDomDocument &doc)
{
	QList<QDomElement> out;
	for (QDomElement d = doc.documentElement().firstChildElement(QStringLiteral("diagram"));
		 !d.isNull(); d = d.nextSiblingElement(QStringLiteral("diagram")))
		out << d;
	return out;
}

// Symbols without uuids, and no wires (they refer to symbols by uuid).
void stripUuids(QDomDocument &doc)
{
	for (QDomElement d : diagrams(doc)) {
		for (QDomElement e : symbols(d))
			e.removeAttribute(QStringLiteral("uuid"));
		d.removeChild(d.firstChildElement(QStringLiteral("conductors")));
	}
}

// type|x|y|orientation -> uuid, for every symbol in the file
QMultiHash<QString, QString> symbolUuids(const QDomDocument &doc)
{
	QMultiHash<QString, QString> out;
	for (const QDomElement &d : diagrams(doc))
		for (const QDomElement &e : symbols(d))
			out.insert(QStringList{e.attribute(QStringLiteral("type")),
								   e.attribute(QStringLiteral("x")),
								   e.attribute(QStringLiteral("y")),
								   e.attribute(QStringLiteral("orientation"))}
					   .join(QLatin1Char('|')),
					   e.attribute(QStringLiteral("uuid")));
	return out;
}

} // namespace

class tst_derivedsymboluuid : public QObject
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
		if (!proc.waitForFinished(60000) || proc.exitCode() != 0) return {};
		return load(out);
	}

	QDomDocument fixture()
	{
		QDomDocument doc = load(QFINDTESTDATA("fixtures/qet_bug_repro_resaved.qet"));
		stripUuids(doc);
		return doc;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
		QVERIFY(!fixture().isNull());
	}

	void sameUuidsOnEveryLoad()
	{
		const QMultiHash<QString, QString> a = symbolUuids(resave(fixture()));
		const QMultiHash<QString, QString> b = symbolUuids(resave(fixture()));
		QCOMPARE(a.size(), 9);      // the fixture's placed symbols
		QCOMPARE(a, b);
		for (const QString &u : a)
			QVERIFY2(!QUuid(u).isNull(), qPrintable(u));
		QCOMPARE(QSet<QString>(a.begin(), a.end()).size(), a.size());
	}

	void insertingAFolioChangesNothing()
	{
		QDomDocument moved = fixture();
		QDomElement first = diagrams(moved).first();
		QDomElement blank = first.cloneNode(false).toElement();
		blank.setAttribute(QStringLiteral("title"), QStringLiteral("new"));
		blank.appendChild(moved.createElement(QStringLiteral("elements")));
		moved.documentElement().insertBefore(blank, first);

		const QMultiHash<QString, QString> after = symbolUuids(resave(moved));
		QCOMPARE(after.size(), 9);
		QCOMPARE(after, symbolUuids(resave(fixture())));
	}

	void savedUuidsAreKept()
	{
		QDomDocument doc = load(QFINDTESTDATA("fixtures/qet_bug_repro_resaved.qet"));
		QCOMPARE(symbolUuids(resave(doc)), symbolUuids(doc));
	}

	// A symbol keeps its derived uuid once saved, even when moved. A symbol
	// saved without a uuid that later turns up on the spot it left (a hand
	// edit, an older version, another tool) would derive the same uuid: it
	// must get another one instead.
	void newcomerOnAMovedSymbolsSpotGetsAnotherUuid()
	{
		QDomDocument doc = resave(fixture());
		QDomElement moved = symbols(diagrams(doc).first()).first();
		QVERIFY(!QUuid(moved.attribute(QStringLiteral("uuid"))).isNull());
		QDomElement newcomer = moved.cloneNode(true).toElement();
		newcomer.removeAttribute(QStringLiteral("uuid"));
		const QDomNodeList terminals = newcomer.elementsByTagName(QStringLiteral("terminal"));
		for (int i = 0; i < terminals.size(); ++i)
			terminals.at(i).toElement().setAttribute(QStringLiteral("id"), 90000 + i);
		moved.setAttribute(QStringLiteral("x"), moved.attribute(QStringLiteral("x")).toInt() + 500);
		moved.parentNode().appendChild(newcomer);

		const QMultiHash<QString, QString> after = symbolUuids(resave(doc));
		QCOMPARE(after.size(), 10);
		QCOMPARE(QSet<QString>(after.begin(), after.end()).size(), 10);
	}

	void stackedIdenticalSymbolsDiffer()
	{
		QDomDocument doc = fixture();
		QDomElement one = symbols(diagrams(doc).first()).first();
		QDomElement copy = one.cloneNode(true).toElement();
			//A copy's terminals carry their own file ids; a symbol whose
			//terminal ids are already taken is not loaded at all.
		const QDomNodeList terminals = copy.elementsByTagName(QStringLiteral("terminal"));
		for (int i = 0; i < terminals.size(); ++i)
			terminals.at(i).toElement().setAttribute(QStringLiteral("id"), 90000 + i);
		one.parentNode().appendChild(copy);

		const QStringList both = symbolUuids(resave(doc)).values(
					QStringList{one.attribute(QStringLiteral("type")),
								one.attribute(QStringLiteral("x")),
								one.attribute(QStringLiteral("y")),
								one.attribute(QStringLiteral("orientation"))}
					.join(QLatin1Char('|')));
		QCOMPARE(both.size(), 2);
		QVERIFY(both.at(0) != both.at(1));
	}
};

QTEST_APPLESS_MAIN(tst_derivedsymboluuid)

#include "tst_derivedsymboluuid.moc"
