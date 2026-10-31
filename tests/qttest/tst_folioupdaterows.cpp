// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>
#include "qettesttimeout.h"

// An edit rebuilds the project database's rows of the folios it touched,
// not of every folio (projectDataBase::refreshChangedFolios(),
// QET-SQL-FORMAT S3z1b): the tables must still be what a full rebuild
// gives, rows and order.
//
// Runs the real binary with QET_CHECK_DB_REBUILD=1, which rebuilds every
// folio after each such update and logs any table that changes. Each
// script reads the database after every edit, so that every edit is
// checked on its own.
class tst_folioupdaterows : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	static QString script(const QString &edits)
	{
		return QStringLiteral(
				"function t(name, f) {\n"
				"  var r;\n"
				"  try { r = f(); } catch (e) { r = 'threw ' + e; }\n"
				"  qet.log('EDIT ' + name + ' ' + r);\n"
				"  qet.query('SELECT 1');\n"
				"}\n"
				"var n = qet.folioCount();\n"
				"var masters = [], plain = [];\n"
				"for (var f = 0; f < n; ++f) {\n"
				"  var u = qet.elementUuids(f);\n"
				"  for (var i = 0; i < u.length; ++i) {\n"
				"    var k = qet.elementLinkType(f, u[i]);\n"
				"    if (k == 'master') masters.push([f, u[i]]);\n"
				"    else if (k == 'simple') plain.push([f, u[i]]);\n"
				"  }\n"
				"}\n") + edits + QStringLiteral("qet.log('EDITS DONE');\n");
	}

	void run(const QString &edits, bool eager, int edit_count)
	{
		const QString work = m_dir.filePath(QStringLiteral("run%1").arg(m_run++));
		const QString home = work + QStringLiteral("/home");
		QDir().mkpath(home);
		QFile js(work + QStringLiteral("/edits.js"));
		QVERIFY(js.open(QIODevice::WriteOnly));
		js.write(script(edits).toUtf8());
		js.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("QET_CHECK_DB_REBUILD"), QStringLiteral("1"));
		// a dozen edits take longer than a script's 30 s under AddressSanitizer
		env.insert(QStringLiteral("QET_SCRIPT_TIMEOUT_MS"), QString::number(QetTest::timeout(240000)));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		if (eager) env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--common-elements-dir=" QET_ELEMENTS_DIR "/"),
					QStringLiteral("--run"), js.fileName(),
					QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet")});
		QVERIFY(proc.waitForFinished(QetTest::timeout(300000)));
		const QString log = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());

		QVERIFY2(log.contains(QStringLiteral("EDITS DONE")), qPrintable(log.right(2000)));
		const QStringList lines = log.split(QLatin1Char('\n'));
		QStringList edits_done, mismatches;
		for (const QString &line : lines) {
			if (line.contains(QStringLiteral("EDIT ")) && !line.contains(QStringLiteral("EDITS DONE")))
				edits_done << line.mid(line.indexOf(QStringLiteral("EDIT ")));
			if (line.contains(QStringLiteral("a full rebuild changes")))
				mismatches << line.mid(line.indexOf(QStringLiteral("a full rebuild changes")));
		}
		QCOMPARE(edits_done.size(), edit_count);
		for (const QString &edit : std::as_const(edits_done)) {
			QVERIFY2(!edit.endsWith(QStringLiteral(" false")) && !edit.contains(QStringLiteral(" threw "))
					 && !edit.endsWith(QStringLiteral(" -1")), qPrintable(edit));
		}
		QVERIFY2(mismatches.isEmpty(), qPrintable(mismatches.join(QLatin1Char('\n'))));
			//Each edit was checked, not only the last
		QVERIFY2(log.count(QStringLiteral("update checked against a full rebuild")) >= edit_count,
				 qPrintable(log.right(2000)));
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void edits_data()
	{
		QTest::addColumn<QString>("edits");
		QTest::addColumn<int>("count");
		QTest::addColumn<bool>("eager");
		const QString symbols = QStringLiteral(
				// a coil's label, shown by its contacts on other folios
				"for (var i = 0; i < 2; ++i) {\n"
				"  var m = masters[i];\n"
				"  t('master label', function () { return qet.setElementLabel(m[0], m[1], 'KZ' + i); });\n"
				"  t('master info', function () { return qet.setElementInfo(m[0], m[1], 'comment', 'c' + i); });\n"
				"}\n"
				"var p = plain[0], q = plain[1];\n"
				"t('delete symbol', function () { return qet.deleteElement(p[0], p[1]); });\n"
				"t('undo', function () { return qet.undo(); });\n"
				"t('redo', function () { return qet.redo(); });\n"
				"t('undo', function () { return qet.undo(); });\n"
				"var path = 'common://10_electric/10_allpole/130_terminals_terminal_strips/bornedouble.elmt';\n"
				"var a = qet.addElement(3, path, 100, 300), b = qet.addElement(3, path, 400, 300);\n"
				"t('wire', function () { return qet.addConductor(3, a, 0, b, 0); });\n"
				"t('delete wire', function () { return qet.deleteConductor(3, a, 0); });\n"
				"t('undo', function () { return qet.undo(); });\n");
		const QString drawing = QStringLiteral(
				"t('paste', function () { return qet.duplicateElements(plain[1][0], [plain[1][1], plain[2][1]], n - 1, 300, 300).length == 2; });\n"
				"t('text', function () { return qet.addText(0, 'hello', 100, 100) >= 0; });\n"
				"t('shape', function () { return qet.addShape(0, 'rectangle', 50, 50, 150, 120) >= 0; });\n"
				"t('folio title', function () { return qet.setFolioTitle(0, 'Renamed'); });\n"
				"t('folio property', function () { return qet.setFolioProperty(1, 'locmach', 'L9'); });\n");
			//A label made from %id changes on every folio after one inserted
		const QString folios = QStringLiteral(
				"t('insert folio', function () { return qet.insertFolio(1) == 1; });\n"
				"t('label after insert', function () { return qet.setElementLabel(5, qet.elementUuids(5)[0], 'ZI'); });\n"
				"t('add folio', function () { return qet.addFolio() >= 0; });\n"
				"t('remove folio', function () { return qet.removeFolio(2); });\n"
				"t('undo', function () { return qet.undo(); });\n"
				"t('undo', function () { return qet.undo(); });\n"
				"t('redo', function () { return qet.redo(); });\n");
		for (bool eager : {false, true}) {
			const char *how = eager ? "every folio built" : "folios built on demand";
			QTest::newRow(qPrintable(QStringLiteral("symbols and wires, %1").arg(QLatin1String(how)))) << symbols << 11 << eager;
			QTest::newRow(qPrintable(QStringLiteral("paste and drawing, %1").arg(QLatin1String(how)))) << drawing << 5 << eager;
			QTest::newRow(qPrintable(QStringLiteral("folios, %1").arg(QLatin1String(how)))) << folios << 7 << eager;
		}
	}

	void edits()
	{
		QFETCH(QString, edits);
		QFETCH(int, count);
		QFETCH(bool, eager);
		run(edits, eager, count);
	}
};

QTEST_GUILESS_MAIN(tst_folioupdaterows)
#include "tst_folioupdaterows.moc"
