// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

// The wiring list (--export-cables, and the wiring plan CSV export of the
// GUI) joins the two halves of a wire drawn to a folio report. The fixture
// has three pairs of folios, each joined by a linked going/coming report:
//   P: one wire on each side             -> one row, both folios
//   E: two wires on the first side, one  -> three rows, none dropped
//   L: one wire, two on the second side  -> three rows
// With several wires on a side the diagram does not say which terminal is
// wired to which, so those wires are not joined. The Page column is the
// folio number as the folio shows it, not its "%id/%total" template, and
// rows come in folio order. The last column is the wire's cable; the two
// halves of P both say W1, which is written once.
class tst_wiringlistexport : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	// The rows of the wiring list, header left out. Sets @p ok to whether
	// the export ran, so a project with no wires is not mistaken for a
	// failed run.
	QStringList exportCables(const QString &project, bool *ok)
	{
		*ok = false;
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run));
		const QString tmp = m_dir.filePath(QStringLiteral("tmp%1").arg(m_run));
		const QString out = m_dir.filePath(QStringLiteral("out%1.csv").arg(m_run++));
		QDir().mkpath(home);
		QDir().mkpath(tmp);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/.config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/.local/share"));
		env.insert(QStringLiteral("TMPDIR"), tmp);

		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--export-cables"), project, out});
		if (!proc.waitForFinished(60000) || proc.exitCode() != 0)
			return {};

		QFile file(out);
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return {};
		*ok = true;
		QStringList rows = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'),
																   Qt::SkipEmptyParts);
		if (!rows.isEmpty())
			rows.removeFirst(); // header, translated
		return rows;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void reportPairs()
	{
		const QString fixture = QFINDTESTDATA("fixtures/wiring_list_arrows.qet");
		QVERIFY2(!fixture.isEmpty(), "fixture project not found");

		const QStringList expected {
			QStringLiteral("1/6, 2/6;PA;A1;PB;A1;;;;;W1"),
			QStringLiteral("3/6;EA;A1;;1;;;;;W2"),
			QStringLiteral("3/6;EA2;A1;;1;;;;;"),
			QStringLiteral("4/6;EB;A1;;1;;;;;"),
			QStringLiteral("5/6;LA;A1;;1;;;;;"),
			QStringLiteral("6/6;LB;A1;;1;;;;;"),
			QStringLiteral("6/6;LD;A1;;1;;;;;"),
		};
		bool ok;
		QCOMPARE(exportCables(fixture, &ok), expected);
		QVERIFY(ok);
	}

	void pageIsTheFolioNumber_data()
	{
		QTest::addColumn<QString>("project");
		const QDir examples(QStringLiteral(QET_EXAMPLES_DIR));
		for (const QString &f : examples.entryList({QStringLiteral("*.qet")}, QDir::Files, QDir::Name))
			QTest::newRow(f.toUtf8().constData()) << examples.filePath(f);
	}

	void pageIsTheFolioNumber()
	{
		QFETCH(QString, project);
		bool ok;
		const QStringList rows = exportCables(project, &ok);
		QVERIFY2(ok, "--export-cables failed");
		for (const QString &row : rows) {
			const QString page = row.section(QLatin1Char(';'), 0, 0);
			QVERIFY2(!page.contains(QLatin1Char('%')), qPrintable(row));
		}
	}
};

QTEST_APPLESS_MAIN(tst_wiringlistexport)

#include "tst_wiringlistexport.moc"
