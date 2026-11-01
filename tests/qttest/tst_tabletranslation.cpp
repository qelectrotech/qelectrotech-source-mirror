// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include "qettesttimeout.h"

// A table shows its column titles and cells in the project's active
// language when the project has a translation for them. The text the table
// holds is never changed, so the exports of the table follow the same rule.
//
// Runs the real binary: opens a small project, places a table of the folio
// titles, saves it (the export reads the file) and exports the folio as SVG,
// where every text stays a text.
namespace {

QString languages(const QString &active)
{
	return QStringLiteral(
			"<languages source=\"en\" active=\"%1\"><language code=\"de\"/>"
			"<phrase source=\"Motor protection\"><text language=\"de\" value=\"Motorschutz\"/></phrase>"
			"<phrase source=\"Title\"><text language=\"de\" value=\"Titelspalte\"/></phrase>"
			"</languages>").arg(active);
}

QString project(const QString &languages)
{
	return QStringLiteral(
				"<project title=\"tt\" version=\"0.100.1\">"
				"<diagram title=\"Motor protection\" cols=\"8\" rows=\"4\" colsize=\"60\" rowsize=\"80\">"
				"<elements/><conductors/></diagram>%1</project>").arg(languages);
}

} // namespace

class tst_tabletranslation : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// The SVG of the folio after a table of the folio titles is placed
	QString svgWith(const QString &languages)
	{
		const QString run = QString::number(m_run++);
		const QString home = m_dir.filePath(QStringLiteral("home") + run);
		const QString out = m_dir.filePath(QStringLiteral("svg") + run);
		QDir().mkpath(home);
		QDir().mkpath(out);
		const QString in = m_dir.filePath(QStringLiteral("in") + run + QStringLiteral(".qet"));
		QFile f(in);
		if (!f.open(QIODevice::WriteOnly)) return {};
		f.write(project(languages).toUtf8());
		f.close();
		const QString js = m_dir.filePath(QStringLiteral("script") + run + QStringLiteral(".js"));
		QFile s(js);
		if (!s.open(QIODevice::WriteOnly)) return {};
		s.write("qet.addTable(0, 'summary', 't', 'SELECT title FROM project_summary_view');"
				"qet.save('" + in.toUtf8() + "');"
				"qet.exportSvg('" + out.toUtf8() + "');");
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
		if (!proc.waitForFinished(QetTest::timeout(120000))) return {};
		QString svg;
		const QStringList files = QDir(out).entryList({QStringLiteral("*.svg")});
		for (const QString &name : files) {
			QFile svg_file(QDir(out).filePath(name));
			if (svg_file.open(QIODevice::ReadOnly)) svg += QString::fromUtf8(svg_file.readAll());
		}
		return svg;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	// The control: the table is drawn, with the words as they are held
	void tableShowsTheSourceWords()
	{
		const QString svg = svgWith(QString());
		QVERIFY(!svg.isEmpty());
		QVERIFY2(svg.contains(QStringLiteral("Motor protection")), "the table cell is not in the export");
		QVERIFY(!svg.contains(QStringLiteral("Motorschutz")));
	}

	// The same project, drawn in German: the cell and the column title
	void tableFollowsTheActiveLanguage()
	{
		const QString svg = svgWith(languages(QStringLiteral("de")));
		QVERIFY(!svg.isEmpty());
		QVERIFY(svg.contains(QStringLiteral("Motorschutz")));
		QVERIFY(svg.contains(QStringLiteral("Titelspalte")));
	}

	// Translations that are not in use change nothing
	void sourceLanguageShowsTheSourceWords()
	{
		const QString svg = svgWith(languages(QStringLiteral("en")));
		QVERIFY(svg.contains(QStringLiteral("Motor protection")));
		QVERIFY(!svg.contains(QStringLiteral("Motorschutz")));
	}
};

QTEST_GUILESS_MAIN(tst_tabletranslation)
#include "tst_tabletranslation.moc"
