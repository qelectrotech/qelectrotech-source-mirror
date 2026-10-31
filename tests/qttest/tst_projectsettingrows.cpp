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
#include <QRegularExpression>
#include <QTemporaryDir>
#include "qettesttimeout.h"

// The project's settings (title, project properties, new-folio defaults)
// are in the project database, one row per value (project_setting), read
// from the file and kept by every change. QET-SQL-FORMAT S3b.

namespace {

const QString project_xml = QStringLiteral(
	"<project title=\"Plant\" version=\"0.100.1\">"
	"<properties><property name=\"site\" show=\"1\">Plant 2</property></properties>"
	"<terminal_labelling split=\"true\"/>"
	"<chained_reports enabled=\"true\"/>"
	"<style origin=\"bottom-right\"/>"
	"<newdiagrams>"
	"<border cols=\"17\" rows=\"8\" colsize=\"60\" rowsize=\"80\" displaycols=\"true\" displayrows=\"true\"/>"
	"<inset titleblocktemplate=\"A3_cartouche\" titleblocktemplateCollection=\"custom\" folio=\"%id/%total\">"
	"<properties><property name=\"drawn\" show=\"0\">JD</property></properties></inset>"
	"<conductors type=\"multi\" function=\"\"/>"
	"<report label=\"%f-%l%c\"/>"
	"<xrefs><xref type=\"coil\" displayhas=\"cross\"/><xref type=\"protection\" displayhas=\"cross\"/></xrefs>"
	"<guides><guide orientation=\"1\" position=\"120\" color=\"#00ff00\"/></guides>"
	"</newdiagrams>"
	"<diagram title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\"><elements/><conductors/></diagram>"
	"</project>");

} // namespace

class tst_projectsettingrows : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// The rows of project_setting after running @p script on the project,
	// as "section|ord|name|value".
	QStringList rows(const QString &script, const QString &project_path = QString())
	{
		const QString in = m_dir.filePath(QStringLiteral("in%1.qet").arg(m_run));
		const QString js = m_dir.filePath(QStringLiteral("script%1.js").arg(m_run));
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		if (project_path.isEmpty()) {
			QFile f(in);
			if (!f.open(QIODevice::WriteOnly)) return {};
			f.write(project_xml.toUtf8());
			f.close();
		}
		QFile s(js);
		if (!s.open(QIODevice::WriteOnly)) return {};
		s.write((script + QStringLiteral(
			"\nqet.log('ROWS ' + JSON.stringify(qet.query("
			"'SELECT section, ord, name, value FROM project_setting')));")).toUtf8());
		s.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), js, project_path.isEmpty() ? in : project_path});
		if (!proc.waitForFinished(QetTest::timeout(60000))) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		QStringList result;
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(QStringLiteral("ROWS "));
			if (i < 0) continue;
			for (const QJsonValue &v : QJsonDocument::fromJson(line.mid(i + 5).toUtf8()).array()) {
				const QJsonObject o = v.toObject();
				result << QStringLiteral("%1|%2|%3|%4").arg(o.value(QStringLiteral("section")).toString())
						  .arg(o.value(QStringLiteral("ord")).toInt())
						  .arg(o.value(QStringLiteral("name")).toString(), o.value(QStringLiteral("value")).toString());
			}
		}
		return result;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// Each value read from the file is a row
	void settingsAreRows()
	{
		const QStringList r = rows(QString());
		QVERIFY2(r.contains(QStringLiteral("project|0|title|Plant")), qPrintable(r.join('\n')));
		QVERIFY(r.contains(QStringLiteral("properties|0|site|Plant 2")));
		QVERIFY(r.contains(QStringLiteral("properties|0|site#show|1")));
		QVERIFY(r.contains(QStringLiteral("newdiagrams/border|0|cols|17")));
		QVERIFY(r.contains(QStringLiteral("newdiagrams/inset|0|titleblocktemplate|A3_cartouche")));
		QVERIFY(r.contains(QStringLiteral("newdiagrams/inset|0|titleblocktemplateCollection|custom")));
		QVERIFY(r.contains(QStringLiteral("newdiagrams/inset/properties|0|drawn|JD")));
		QVERIFY(r.contains(QStringLiteral("newdiagrams/report|0|label|%f-%l%c")));
		QVERIFY(r.contains(QStringLiteral("terminal_labelling|0|split|true")));
		QVERIFY(r.contains(QStringLiteral("chained_reports|0|enabled|true")));
		QVERIFY(r.contains(QStringLiteral("style|0|origin|bottom-right")));
		QVERIFY(r.contains(QStringLiteral("newdiagrams/guides/guide|0|color|#00ff00")));
			// one ord per xref, whatever other kinds the defaults add
		const QRegularExpression coil(QStringLiteral("^newdiagrams/xrefs/xref\\|(\\d+)\\|type\\|coil$"));
		const QRegularExpression protection(QStringLiteral("^newdiagrams/xrefs/xref\\|(\\d+)\\|type\\|protection$"));
		const QStringList coils = r.filter(coil), protections = r.filter(protection);
		QCOMPARE(coils.size(), 1);
		QCOMPARE(protections.size(), 1);
		QVERIFY(coil.match(coils.first()).captured(1) != protection.match(protections.first()).captured(1));
			// the numbering has tables of its own
		for (const QString &row : r)
			QVERIFY2(!row.contains(QStringLiteral("autonums")), qPrintable(row));
	}

	// A change of a setting is written through
	void changeIsWrittenThrough()
	{
		const QStringList before = rows(QString());
		QVERIFY(before.contains(QStringLiteral("newdiagrams/conductors|0|function|")));
		const QStringList after = rows(QStringLiteral("qet.setConductorDefault(-1, 'function', 'PE');"));
		QVERIFY2(after.contains(QStringLiteral("newdiagrams/conductors|0|function|PE")), qPrintable(after.join('\n')));
		QVERIFY(!after.contains(QStringLiteral("newdiagrams/conductors|0|function|")));
	}

	// Every setting the saved project holds has its rows: a setting added
	// to the file and not to QETProject::storeSettings() fails here
	void everySavedSettingIsRows()
	{
		const QString saved = m_dir.filePath(QStringLiteral("saved.qet"));
		const QStringList r = rows(QStringLiteral("qet.save('%1');").arg(saved));
		QFile f(saved);
		QVERIFY(f.open(QIODevice::ReadOnly));
		QDomDocument document;
		QVERIFY(document.setContent(&f));
		QStringList sections;
		for (const QString &row : r)
			sections << row.section(QLatin1Char('|'), 0, 0);
			// an element with no attribute, child or text holds no value
		auto holds_value = [](const QDomElement &e) {
			return e.attributes().count() || !e.firstChildElement().isNull() || !e.text().isEmpty();
		};
		auto has_rows = [&sections](const QString &section) {
			for (const QString &s : std::as_const(sections))
				if (s == section || s.startsWith(section + QLatin1Char('/')))
					return true;
			return false;
		};
		QStringList checked;
		for (QDomElement child = document.documentElement().firstChildElement() ; !child.isNull()
			 ; child = child.nextSiblingElement()) {
			const QString tag = child.tagName();
				// the folios, the embedded collections and the usage time
				// (counted while the project is open) are not settings
			if (tag == QLatin1String("diagram") || tag == QLatin1String("collection")
				|| tag == QLatin1String("titleblocktemplates") || tag == QLatin1String("usage"))
				continue;
			if (tag == QLatin1String("newdiagrams")) {
				for (QDomElement d = child.firstChildElement() ; !d.isNull() ; d = d.nextSiblingElement()) {
						// the numbering has tables of its own
					if (d.tagName().endsWith(QLatin1String("_autonums")) || !holds_value(d))
						continue;
					checked << QStringLiteral("newdiagrams/") + d.tagName();
				}
				continue;
			}
			if (holds_value(child))
				checked << tag;
		}
		QVERIFY(checked.contains(QStringLiteral("style")));
		QVERIFY(checked.contains(QStringLiteral("newdiagrams/guides")));
		for (const QString &section : std::as_const(checked))
			QVERIFY2(has_rows(section), qPrintable(section));
	}

	// The default layout of the terminal strips is rows, as a save writes it:
	// only for a project with strips
	void layoutPatternIsRows()
	{
		const QStringList none = rows(QString());
		for (const QString &row : none)
			QVERIFY2(!row.startsWith(QStringLiteral("terminal_strip_layout")), qPrintable(row));

		const QString fixture = QStringLiteral(QET_FIXTURES_DIR "/every_feature.qet");
		QFile f(fixture);
		QVERIFY(f.open(QIODevice::ReadOnly));
		QDomDocument doc;
		QVERIFY(doc.setContent(&f));
		QDomElement layout;
		for (QDomElement c = doc.documentElement().firstChildElement(); !c.isNull(); c = c.nextSiblingElement())
			if (c.tagName() == QLatin1String("terminal_strip_layout")) layout = c;
		QVERIFY(!layout.isNull());

		const QStringList r = rows(QString(), fixture);
		const QStringList layout_rows = r.filter(QRegularExpression(QStringLiteral("^terminal_strip_layout")));
		QVERIFY2(!layout_rows.isEmpty(), qPrintable(r.join('\n')));
		for (const QString &name : {QStringLiteral("header_h"), QStringLiteral("terminal_2_y"),
									QStringLiteral("font_pixel_size"), QStringLiteral("cable_end_length"),
									QStringLiteral("xref_text_orientation")}) {
			const QString row = QStringLiteral("terminal_strip_layout|0|%1|%2").arg(name, layout.attribute(name));
			QVERIFY2(r.contains(row), qPrintable(row));
		}
	}
};

QTEST_GUILESS_MAIN(tst_projectsettingrows)
#include "tst_projectsettingrows.moc"
