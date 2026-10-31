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
#include "qettesttimeout.h"

// The terminal strips are in the project database, one terminal_strip_value
// row per value of what each strip's toXml() writes, written through as the
// strips change. QET-SQL-FORMAT S3k1.
//
// Runs the real binary (--run) on tests/qttest/fixtures/every_feature.qet,
// which has one strip of two bridged terminals.
namespace {

const QString strip_uuid = QStringLiteral("{cfad854f-6cd6-4d34-9473-9af35b219174}");

// The rows of an element the way projectDataBase::xmlRows() makes them,
// as "section|ord|name|value".
void flatten(const QDomElement &element, const QString &section, int ord, QStringList &rows,
			 const QString &children_section = QString())
{
	const QString base = children_section.isEmpty() ? section : children_section;
	const QDomNamedNodeMap attributes = element.attributes();
	QStringList names;
	for (int i = 0 ; i < attributes.count() ; ++i) names << attributes.item(i).nodeName();
	names.sort();
	for (const QString &name : std::as_const(names))
		rows << QStringLiteral("%1|%2|%3|%4").arg(section).arg(ord).arg(name, element.attribute(name));
	QHash<QString, int> count;
	for (QDomElement child = element.firstChildElement() ; !child.isNull() ; child = child.nextSiblingElement())
		if (!(child.tagName() == QLatin1String("property") && child.hasAttribute(QStringLiteral("name"))))
			++count[child.tagName()];
	QHash<QString, int> seen;
	bool has_child = false;
	for (QDomElement child = element.firstChildElement() ; !child.isNull() ; child = child.nextSiblingElement()) {
		has_child = true;
		if (child.tagName() == QLatin1String("property") && child.hasAttribute(QStringLiteral("name"))) {
			const QString name = child.attribute(QStringLiteral("name"));
			rows << QStringLiteral("%1|%2|%3|%4").arg(section).arg(ord).arg(name, child.text());
			if (child.hasAttribute(QStringLiteral("show")))
				rows << QStringLiteral("%1|%2|%3#show|%4").arg(section).arg(ord).arg(name, child.attribute(QStringLiteral("show")));
			continue;
		}
		const QString child_section = base + QLatin1Char('/') + child.tagName();
		const int child_ord = seen[child_section]++;
		flatten(child, child_section, child_ord, rows, count.value(child.tagName()) > 1
				? child_section + QStringLiteral("[%1]").arg(child_ord) : child_section);
	}
	if (!has_child && !element.text().isEmpty())
		rows << QStringLiteral("%1|%2|#text|%3").arg(section).arg(ord).arg(element.text());
}

} // namespace

class tst_terminalstriprows : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	QProcessEnvironment environment(const QString &home) const
	{
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		return env;
	}

	// Run @p script on the fixture; the lines the script logs, by tag
	// ("TAG {json}").
	QString runScript(const QString &script, const QString &project)
	{
		const QString js = m_dir.filePath(QStringLiteral("script%1.js").arg(m_run));
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QFile s(js);
		if (!s.open(QIODevice::WriteOnly)) return {};
		s.write(script.toUtf8());
		s.close();
		QProcess proc;
		proc.setProcessEnvironment(environment(home));
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), js, project});
		if (!proc.waitForFinished(QetTest::timeout(120000))) return {};
		return QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
	}

	// The rows logged after @p tag, as "strip|section|ord|name|value".
	static QStringList logged(const QString &out, const QString &tag)
	{
		QStringList result;
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(tag + QLatin1Char(' '));
			if (i < 0) continue;
			for (const QJsonValue &v : QJsonDocument::fromJson(line.mid(i + tag.size() + 1).toUtf8()).array()) {
				const QJsonObject o = v.toObject();
				const QJsonValue value = o.value(QStringLiteral("value"));
				result << QStringLiteral("%1|%2|%3|%4|%5").arg(
							o.value(QStringLiteral("strip_uuid")).toString(),
							o.value(QStringLiteral("section")).toString(),
							QString::number(o.value(QStringLiteral("ord")).toInt()),
							o.value(QStringLiteral("name")).toString(),
							value.isNull() ? QStringLiteral("NULL") : value.toString());
			}
		}
		return result;
	}

	static QString fixture() {return QStringLiteral(QET_FIXTURES_DIR "/every_feature.qet");}

	static QString rowsQuery()
	{
		return QStringLiteral("JSON.stringify(qet.query('SELECT strip_uuid, section, ord, name, value "
							  "FROM terminal_strip_value ORDER BY rowid'))");
	}

	static int distinctStrips(const QStringList &rows)
	{
		QSet<QString> set;
		for (const QString &r : rows) set << r.section(QLatin1Char('|'), 0, 0);
		return set.size();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// Opening a project gives rows for every strip of the file
	void openedStripsAreRows()
	{
		const QString out = runScript(QStringLiteral("qet.log('ROWS ' + %1);").arg(rowsQuery()), fixture());
		const QStringList rows = logged(out, QStringLiteral("ROWS"));
		QVERIFY2(!rows.isEmpty(), qPrintable(out.right(2000)));
		QFile f(fixture());
		QVERIFY(f.open(QIODevice::ReadOnly));
		QDomDocument doc;
		QVERIFY(doc.setContent(&f));
		const int in_file = doc.elementsByTagName(QStringLiteral("terminal_strip")).count();
		QCOMPARE(distinctStrips(rows), in_file);
		qInfo() << "terminal_strip_value rows:" << rows.size() << "strips:" << in_file;
		const QString p = strip_uuid + QLatin1Char('|');
		QVERIFY(rows.contains(p + "terminal_strip/terminal_strip_data|0|uuid|" + strip_uuid));
		QVERIFY(rows.contains(p + "terminal_strip/terminal_strip_data/informations/information|2|#text|X1"));
		QVERIFY(rows.contains(p + "terminal_strip/terminal_strip_data/informations/information|0|#text|Plant 0"));
			// each terminal's real terminal is under its own terminal's index
		QVERIFY(rows.contains(p + "terminal_strip/layout/physical_terminal[0]/real_terminal|0|element_uuid|{891b70f2-2822-41c3-9f94-28efb448b5c4}"));
		QVERIFY(rows.contains(p + "terminal_strip/layout/physical_terminal[1]/real_terminal|0|element_uuid|{4861e268-d344-413e-a86f-034ea5ada16c}"));
		QVERIFY(rows.contains(p + "terminal_strip/terminal_strip_bridge|0|color|#808080"));
	}

	// A strip added or removed through the script API is written through
	void changesAreWrittenThrough()
	{
		const QString script = QStringLiteral(
					"qet.log('BEFORE ' + %1);"
					"var i = qet.addTerminalStrip('Plant 9', 'Cabinet 9', 'X9');"
					"qet.log('ADDED ' + %1);"
					"qet.removeTerminalStrip(i);"
					"qet.log('REMOVED ' + %1);").arg(rowsQuery());
		const QString out = runScript(script, fixture());
		const QStringList before = logged(out, QStringLiteral("BEFORE"));
		const QStringList added = logged(out, QStringLiteral("ADDED"));
		const QStringList removed = logged(out, QStringLiteral("REMOVED"));
		QVERIFY2(!before.isEmpty(), qPrintable(out.right(2000)));
		QCOMPARE(distinctStrips(before), 1);
		QCOMPARE(distinctStrips(added), 2);
		QVERIFY(added.size() > before.size());
		QStringList extra;
		for (const QString &r : added) if (!before.contains(r)) extra << r;
		bool named = false;
		for (const QString &r : std::as_const(extra))
			if (r.endsWith(QStringLiteral("|terminal_strip/terminal_strip_data/informations/information|2|#text|X9"))) named = true;
		QVERIFY2(named, qPrintable(extra.join('\n')));
		qInfo() << "rows before:" << before.size() << "added:" << added.size() << "removed:" << removed.size();
		QCOMPARE(removed, before);
	}

	// The rows are what a save writes
	void rowsEqualWhatASaveWrites()
	{
		const QString saved = m_dir.filePath(QStringLiteral("saved.qet"));
		QProcess proc;
		proc.setProcessEnvironment(environment(m_dir.filePath(QStringLiteral("home-resave"))));
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--resave"), fixture(), saved});
		QVERIFY(proc.waitForFinished(QetTest::timeout(180000)));
		QVERIFY(QFile::exists(saved));

		QFile f(saved);
		QVERIFY(f.open(QIODevice::ReadOnly));
		QDomDocument doc;
		QVERIFY(doc.setContent(&f));
		const QDomNodeList strips = doc.elementsByTagName(QStringLiteral("terminal_strip"));
		QCOMPARE(strips.count(), 1);
		QStringList expected;
		flatten(strips.at(0).toElement(), QStringLiteral("terminal_strip"), 0, expected);
		QVERIFY(!expected.isEmpty());

		const QString out = runScript(QStringLiteral("qet.log('ROWS ' + %1);").arg(rowsQuery()), saved);
		QStringList got;
		for (const QString &r : logged(out, QStringLiteral("ROWS"))) {
			QVERIFY(r.startsWith(strip_uuid + QLatin1Char('|')));
			got << r.section(QLatin1Char('|'), 1);
		}
		qInfo() << "saved strip rows:" << expected.size();
		QCOMPARE(got, expected);
	}
};

QTEST_GUILESS_MAIN(tst_terminalstriprows)
#include "tst_terminalstriprows.moc"
