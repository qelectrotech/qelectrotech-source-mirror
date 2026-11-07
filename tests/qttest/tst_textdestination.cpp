// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

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

// A free text (a reference such as /2.1) can lead to a folio. It is a
// "destination" attribute of the <input>, written only when set, the project
// database holds it (the drawing_item rows), whether the folio was built or
// read from the document, and the PDF export makes the text a link.
//
// Runs the real binary on a small project written here: two folios, one text
// which leads to the second, one which leads nowhere.
namespace {

const QString folio_one = QStringLiteral("{aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa}");
const QString folio_two = QStringLiteral("{bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbbb}");
const QString uuid_linked = QStringLiteral("{11111111-1111-4111-8111-111111111111}");
const QString uuid_plain = QStringLiteral("{22222222-2222-4222-8222-222222222222}");

QString text(const QString &uuid, int x, const QString &label, const QString &destination)
{
	return QStringLiteral("<input uuid=\"%1\" x=\"%2\" y=\"60\" text=\"%3\" rotation=\"0\"%4/>")
			.arg(uuid).arg(x).arg(label,
					destination.isEmpty() ? QString() : QStringLiteral(" destination=\"%1\"").arg(destination));
}

QString project(const QString &destination)
{
	return QStringLiteral(
				"<project title=\"dest\" version=\"0.100.1\">"
				"<diagram uuid=\"%1\" title=\"one\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<elements/><conductors/><inputs>%3%4</inputs></diagram>"
				"<diagram uuid=\"%2\" title=\"two\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<elements/><conductors/></diagram></project>")
			.arg(folio_one, folio_two, text(uuid_linked, 100, QStringLiteral("/2.1"), destination),
				 text(uuid_plain, 300, QStringLiteral("/9.9"), QString()));
}

} // namespace

class tst_textdestination : public QObject
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

	QString writeProject(const QString &destination)
	{
		const QString in = m_dir.filePath(QStringLiteral("in%1.qet").arg(m_run));
		QFile f(in);
		if (!f.open(QIODevice::WriteOnly)) return {};
		f.write(project(destination).toUtf8());
		return in;
	}

	// The <input> of the text @p uuid after the project is opened and saved
	QDomElement savedText(const QString &destination, bool eager, const QString &uuid, bool *saved = nullptr)
	{
		const QString in = writeProject(destination);
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
		const QDomNodeList inputs = document.elementsByTagName(QStringLiteral("input"));
		for (int i = 0 ; i < inputs.size() ; ++i) {
			const QDomElement input = inputs.at(i).toElement();
			if (input.attribute(QStringLiteral("uuid")) == uuid) return input;
		}
		return {};
	}

	// The drawing_item rows named "destination", as "item|value"
	QStringList databaseRows(const QString &destination, bool eager)
	{
		const QString in = writeProject(destination);
		const QString js = m_dir.filePath(QStringLiteral("script%1.js").arg(m_run));
		QFile s(js);
		if (!s.open(QIODevice::WriteOnly)) return {};
		s.write("qet.log('ROWS ' + JSON.stringify(qet.query("
				"\"SELECT item_uuid, value FROM drawing_item "
				"WHERE name = 'destination' ORDER BY item_uuid\")));");
		s.close();
		QProcess proc;
		proc.setProcessEnvironment(environment(eager));
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), js, in});
		if (!proc.waitForFinished(QetTest::timeout(120000))) return {};
		const QString out = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		QStringList rows;
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(QStringLiteral("ROWS "));
			if (i < 0) continue;
			for (const QJsonValue &v : QJsonDocument::fromJson(line.mid(i + 5).toUtf8()).array()) {
				const QJsonObject o = v.toObject();
				rows << o.value(QStringLiteral("item_uuid")).toString() + QLatin1Char('|')
						+ o.value(QStringLiteral("value")).toString();
			}
		}
		return rows;
	}

	// How many link annotations the exported PDF holds, -1 if it failed
	int pdfLinks(const QString &destination)
	{
		const QString in = writeProject(destination);
		const QString out = m_dir.filePath(QStringLiteral("out%1.pdf").arg(m_run));
		QProcess proc;
		proc.setProcessEnvironment(environment(false));
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--export-pdf"), in, out});
		if (!proc.waitForFinished(QetTest::timeout(120000)) || proc.exitCode() != 0) return -1;
		QFile f(out);
		if (!f.open(QIODevice::ReadOnly)) return -1;
		return QString::fromLatin1(f.readAll()).count(QStringLiteral("/Subtype /Link"));
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void destinationIsSaved_data()
	{
		QTest::addColumn<bool>("eager");
		QTest::newRow("built") << true;
		QTest::newRow("from the document") << false;
	}

	// What the text was given is saved again as it was
	void destinationIsSaved()
	{
		QFETCH(bool, eager);
		const QDomElement input = savedText(folio_two, eager, uuid_linked);
		QVERIFY(!input.isNull());
		QCOMPARE(input.attribute(QStringLiteral("destination")), folio_two);
	}

	// A text which leads nowhere saves exactly as before
	void nothingIsWrittenWhenUnset()
	{
		bool saved = false;
		const QDomElement input = savedText(folio_two, true, uuid_plain, &saved);
		QVERIFY(saved);
		QVERIFY(!input.isNull());
		QVERIFY(!input.hasAttribute(QStringLiteral("destination")));
	}

	// The project database holds it, built or not
	void databaseHoldsTheDestination()
	{
		const QStringList built = databaseRows(folio_two, true);
		QCOMPARE(built, QStringList() << uuid_linked + QStringLiteral("|") + folio_two);
		QCOMPARE(databaseRows(folio_two, false), built);
		QVERIFY(databaseRows(QString(), true).isEmpty());
	}

	// The PDF export makes the text a link; without a destination there is none
	void pdfHasTheLink()
	{
		QCOMPARE(pdfLinks(QString()), 0);
		QVERIFY(pdfLinks(folio_two) >= 1);
	}
};

QTEST_GUILESS_MAIN(tst_textdestination)
#include "tst_textdestination.moc"
