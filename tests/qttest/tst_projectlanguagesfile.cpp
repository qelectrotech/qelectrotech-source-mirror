// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDomDocument>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include "qettesttimeout.h"

// The texts of a project in other languages are a <languages> child of the
// project: saved as read, only when there are any, and held by the project
// database (project_setting), whether the folios are built or read from the
// document. The texts offered for translation come from the database too.
//
// Runs the real binary on a small project written here.
namespace {

const QString languages_xml = QStringLiteral(
		"<languages source=\"en\" active=\"de\"><language code=\"de\"/>"
		"<phrase source=\"Motor protection\"><text language=\"de\" value=\"Motorschutz\"/></phrase>"
		"</languages>");

QString project(const QString &languages)
{
	return QStringLiteral(
				"<project title=\"lang\" version=\"0.100.1\">"
				"<diagram title=\"f\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<elements/><conductors/>"
				"<inputs><input x=\"40\" y=\"40\" text=\"Motor protection\" rotation=\"0\"/>"
				"<input x=\"40\" y=\"90\" text=\"-K1\" rotation=\"0\"/></inputs>"
				"</diagram>%1</project>").arg(languages);
}

} // namespace

class tst_projectlanguagesfile : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	QProcessEnvironment environment(bool eager)
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		if (eager) env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		else env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		return env;
	}

	QString writeProject(const QString &languages)
	{
		const QString in = m_dir.filePath(QStringLiteral("in%1.qet").arg(m_run));
		QFile f(in);
		if (!f.open(QIODevice::WriteOnly)) return {};
		f.write(project(languages).toUtf8());
		return in;
	}

	// The <languages> element after the project is opened and saved again
	QDomElement savedLanguages(const QString &languages, bool eager, bool *saved = nullptr)
	{
		const QString in = writeProject(languages);
		const QString out = m_dir.filePath(QStringLiteral("out%1.qet").arg(m_run));
		QProcess proc;
		proc.setProcessEnvironment(environment(eager));
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--resave"), in, out});
		const bool finished = proc.waitForFinished(QetTest::timeout(120000));
		QFile f(out);
		if (saved) *saved = finished && f.open(QIODevice::ReadOnly);
		else if (!finished || !f.open(QIODevice::ReadOnly)) return {};
		static QDomDocument document;
		if (!document.setContent(&f)) return {};
		return document.documentElement().firstChildElement(QStringLiteral("languages"));
	}

	// What a script prints after "TAG " for the project written here
	QJsonArray scriptRows(const QString &languages, bool eager, const QString &sql)
	{
		const QString in = writeProject(languages);
		const QString js = m_dir.filePath(QStringLiteral("script%1.js").arg(m_run));
		QFile s(js);
		if (!s.open(QIODevice::WriteOnly)) return {};
		s.write("qet.log('ROWS ' + JSON.stringify(qet.query(\"" + sql.toUtf8() + "\")));");
		s.close();
		QProcess proc;
		proc.setProcessEnvironment(environment(eager));
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), js, in});
		if (!proc.waitForFinished(QetTest::timeout(120000))) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(QStringLiteral("ROWS "));
			if (i >= 0) return QJsonDocument::fromJson(line.mid(i + 5).toUtf8()).array();
		}
		return {};
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void languagesAreSaved_data()
	{
		QTest::addColumn<bool>("eager");
		QTest::newRow("built") << true;
		QTest::newRow("from the document") << false;
	}

	// What the project was given is saved again as it was
	void languagesAreSaved()
	{
		QFETCH(bool, eager);
		const QDomElement languages = savedLanguages(languages_xml, eager);
		QVERIFY(!languages.isNull());
		QCOMPARE(languages.attribute(QStringLiteral("active")), QStringLiteral("de"));
		QCOMPARE(languages.firstChildElement(QStringLiteral("language")).attribute(QStringLiteral("code")),
				 QStringLiteral("de"));
		const QDomElement text = languages.firstChildElement(QStringLiteral("phrase"))
				.firstChildElement(QStringLiteral("text"));
		QCOMPARE(text.attribute(QStringLiteral("value")), QStringLiteral("Motorschutz"));
	}

	// A project with none saves exactly as before
	void nothingIsWrittenWhenUnused()
	{
		bool saved = false;
		const QDomElement languages = savedLanguages(QString(), true, &saved);
		QVERIFY(saved);
		QVERIFY(languages.isNull());
	}

	// The project database holds it, built or not
	void databaseHoldsTheLanguages()
	{
		const QString sql = QStringLiteral("SELECT section, name, value FROM project_setting "
										   "WHERE section LIKE 'languages%' ORDER BY section, ord, name");
		const QJsonArray built = scriptRows(languages_xml, true, sql);
		QVERIFY(!built.isEmpty());
		QVERIFY(QJsonDocument(built).toJson().contains("Motorschutz"));
		QCOMPARE(scriptRows(languages_xml, false, sql), built);
		QVERIFY(scriptRows(QString(), true, sql).isEmpty());
	}
};

QTEST_GUILESS_MAIN(tst_projectlanguagesfile)
#include "tst_projectlanguagesfile.moc"
