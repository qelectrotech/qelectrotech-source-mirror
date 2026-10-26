// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// A symbol's kind information (NO/NC/SW/power and the like, baked into its
// own definition and never edited after it is built -- the data
// CrossRefItem::NOElements()/NCElements() read to classify a linked contact)
// is queryable from the project database (element_kind_information), not
// only from a live Element*. QETZ-DB-FIRST 2d-2a.

namespace {

// One embedded symbol, carrying the given kind-information rows, placed once.
QString project(const QString &kind_informations_xml)
{
	return QStringLiteral(
		"<project title=\"kind\" version=\"0.100.1\">"
		"<diagram title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
		"<elements>"
		"<element orientation=\"0\" type=\"embed://import/t/box.elmt\" z=\"10\""
		" uuid=\"{11111111-1111-4111-8111-111111111111}\" x=\"100\" y=\"200\"/>"
		"</elements><conductors/></diagram>"
		"<collection><category name=\"import\"><category name=\"t\">"
		"<element name=\"box.elmt\">"
		"<definition width=\"30\" hotspot_y=\"21\" version=\"0.100.0\" type=\"element\""
		" link_type=\"simple\" hotspot_x=\"15\" height=\"42\">"
		"<uuid uuid=\"{33333333-3333-4333-8333-333333333333}\"/>"
		"<names><name lang=\"en\">box</name></names>"
		"<kindInformations>%1</kindInformations><informations/>"
		"<description><rect x=\"-5\" y=\"-6\" width=\"10\" height=\"12\""
		" style=\"line-style:normal;line-weight:normal;filling:none;color:black\"/></description>"
		"</definition></element></category></category></collection>"
		"</project>").arg(kind_informations_xml);
}

QString kindInformation(const QString &name, const QString &value, bool show)
{
	return QStringLiteral("<kindInformation name=\"%1\" show=\"%2\">%3</kindInformation>")
			.arg(name).arg(show ? QStringLiteral("1") : QStringLiteral("0"), value);
}

} // namespace

class tst_elementkindinformation : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// Runs the real binary's --run on a project written here with @p xml,
	// executing @p script; returns its stdout+stderr.
	QString run(const QString &xml, const QString &script)
	{
		const QString in = m_dir.filePath(QStringLiteral("in%1.qet").arg(m_run));
		const QString js = m_dir.filePath(QStringLiteral("script%1.js").arg(m_run));
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QFile f(in);
		if (!f.open(QIODevice::WriteOnly)) return {};
		f.write(xml.toUtf8());
		f.close();
		QFile s(js);
		if (!s.open(QIODevice::WriteOnly)) return {};
		s.write(script.toUtf8());
		s.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), js, in});
		if (!proc.waitForFinished(60000)) return {};
		return QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
	}

	// The ROWS line of a --run output, as a sorted list of its rows.
	static QJsonArray rowsIn(const QString &out)
	{
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(QStringLiteral("ROWS "));
			if (i < 0) continue;
			QJsonArray rows = QJsonDocument::fromJson(line.mid(i + 5).toUtf8()).array();
			return rows;
		}
		return {};
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// A placed symbol's kind information is in the database once it is
	// placed, in the order its definition gives it, each key with its
	// value and whether it is shown.
	void kindInformationIsStored()
	{
		const QString xml = project(kindInformation(QStringLiteral("state"), QStringLiteral("NO"), false)
								   + kindInformation(QStringLiteral("type"), QStringLiteral("contact"), true));
		const QString script = QStringLiteral(
			"qet.log('ROWS ' + JSON.stringify(qet.query("
			"'SELECT element_uuid, ord, name, value, show FROM element_kind_information ORDER BY ord')));");
		const QJsonArray rows = rowsIn(run(xml, script));
		QCOMPARE(rows.size(), 2);
		const QJsonObject r0 = rows.at(0).toObject(), r1 = rows.at(1).toObject();
		QCOMPARE(r0.value(QStringLiteral("element_uuid")).toString(),
				 QStringLiteral("{11111111-1111-4111-8111-111111111111}"));
		QCOMPARE(r0.value(QStringLiteral("name")).toString(), QStringLiteral("state"));
		QCOMPARE(r0.value(QStringLiteral("value")).toString(), QStringLiteral("NO"));
		QCOMPARE(r0.value(QStringLiteral("show")).toInt(), 0);
		QCOMPARE(r1.value(QStringLiteral("name")).toString(), QStringLiteral("type"));
		QCOMPARE(r1.value(QStringLiteral("value")).toString(), QStringLiteral("contact"));
		QCOMPARE(r1.value(QStringLiteral("show")).toInt(), 1);
	}

	// A symbol with no kind information at all stores no rows (not a
	// crash, not a row of empty strings).
	void noKindInformationMeansNoRows()
	{
		const QString xml = project(QString());
		const QString script = QStringLiteral(
			"qet.log('ROWS ' + JSON.stringify(qet.query("
			"'SELECT * FROM element_kind_information')));");
		QCOMPARE(rowsIn(run(xml, script)).size(), 0);
	}
};

QTEST_GUILESS_MAIN(tst_elementkindinformation)
#include "tst_elementkindinformation.moc"
