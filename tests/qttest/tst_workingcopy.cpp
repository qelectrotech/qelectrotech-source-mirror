// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QCryptographicHash>
#include <QDir>
#include <QDomDocument>
#include <QElapsedTimer>
#include <QFile>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTextStream>

// The working copy (QET-SQL-FORMAT S3z2, QETDB-S3-SCOPE.md 7.4.5): the
// project database in a file, committed after each edit, so that a
// QElectroTech killed at any moment leaves every edit it finished.
//
// Runs the real binary on industrial.qet with QET_WORKING_COPY_DIR. A
// script makes twelve edits that make no new uuids, logging "DONE k"
// after each; a reference run saves the project after each. The test
// kills the edit run at random moments with SIGKILL, writes the copy left
// behind with --from-database, and compares it, saved with every folio
// built, with the reference save k or k+1, k the last DONE before the
// kill, each folio's wires compared as a set. 25 kills by default, about
// 4 min; QET_WORKING_COPY_KILLS=100 for the step's full check (13 min). A run killed before DONE 0 (still opening) leaves no ready copy.
class tst_workingcopy : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	QString m_source, m_script;
	QList<QByteArray> m_reference;   // hash of the eager save after edit k
	qint64 m_open_ms = 0, m_edits_ms = 0;
	static constexpr int LAST = 12;   // the edits the script makes
	int m_run = 0;

	QProcessEnvironment environment(const QString &home, const QString &working_copies = QString()) const
	{
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		// a dozen edits take longer than a script's 30 s under AddressSanitizer
		env.insert(QStringLiteral("QET_SCRIPT_TIMEOUT_MS"), QStringLiteral("240000"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		env.insert(QStringLiteral("TMPDIR"), m_dir.path());
		env.remove(QStringLiteral("QET_EAGER_FOLIOS"));
		env.remove(QStringLiteral("QET_WORKING_COPY_DIR"));
		if (!working_copies.isEmpty())
			env.insert(QStringLiteral("QET_WORKING_COPY_DIR"), working_copies);
		return env;
	}

	QString newHome()
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(m_run++));
		QDir().mkpath(home);
		return home;
	}

	QString runQet(const QStringList &arguments, const QString &working_copies = QString(),
				   bool eager = false, int *exit_code = nullptr)
	{
		QProcessEnvironment env = environment(newHome(), working_copies);
		if (eager) env.insert(QStringLiteral("QET_EAGER_FOLIOS"), QStringLiteral("1"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), arguments);
		if (!proc.waitForFinished(300000)) return QStringLiteral("TIMEOUT");
		if (exit_code) *exit_code = proc.exitCode();
		return QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
	}

	// The hash of @p project saved with every folio built: written under
	// its own file name, which a project without a title takes as one.
	QByteArray savedHash(const QString &project)
	{
		const QString dir = m_dir.filePath(QStringLiteral("saved%1").arg(m_run++));
		QDir().mkpath(dir);
		const QString saved = dir + QLatin1Char('/') + QFileInfo(m_source).fileName();
		runQet({QStringLiteral("--resave"), project, saved}, QString(), true);
		QFile file(saved);
		QDomDocument document;
		if (!file.open(QIODevice::ReadOnly) || !document.setContent(&file)) return {};
			//A folio's wires as a set: the database keeps them in its own
			//order, the scene in another (QETDB-S3-SCOPE.md 7.4.5)
		const QDomNodeList lists = document.elementsByTagName(QStringLiteral("conductors"));
		for (int i = 0; i < lists.size(); ++i) {
			QDomNode list = lists.at(i);
			QMap<QString, QDomNode> sorted;
			for (QDomNode wire = list.firstChild(); !wire.isNull(); wire = wire.nextSibling()) {
				QString key;
				QTextStream stream(&key);
				wire.save(stream, 0);
				sorted.insert(key + QString::number(sorted.size()), wire);
			}
			for (const QDomNode &wire : std::as_const(sorted))
				list.appendChild(wire);   // moves it to the end, in key order
		}
		return QCryptographicHash::hash(document.toByteArray(), QCryptographicHash::Sha256);
	}

	static int lastDone(const QString &log)
	{
		int done = -1;
		const QRegularExpression re(QStringLiteral("DONE (\\d+) true"));
		auto it = re.globalMatch(log);
		while (it.hasNext()) done = it.next().captured(1).toInt();
		return done;
	}

	QStringList workingCopies(const QString &dir, const QString &suffix = QStringLiteral("*")) const
	{
		return QDir(dir).entryList({suffix}, QDir::Files);
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		m_source = m_dir.filePath(QStringLiteral("source/industrial.qet"));
		QDir().mkpath(QFileInfo(m_source).path());
			//Saved once, so that its folios open not built
		runQet({QStringLiteral("--resave"), QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"), m_source});
		QVERIFY(QFile::exists(m_source));

		m_script = m_dir.filePath(QStringLiteral("edits.js"));
		QFile js(m_script);
		QVERIFY(js.open(QIODevice::WriteOnly));
		js.write(QByteArrayLiteral(
				 "function first(f) { return qet.elementUuids(f).slice().sort()[0]; }\n"
				 "function wire(f) { return qet.conductorUuids(f).slice().sort()[0]; }\n"
					//A label a formula does not make: setting one is saved
				 "function plain(f) {\n"
				 "  var u = qet.elementUuids(f).slice().sort();\n"
				 "  for (var i = 0; i < u.length; ++i)\n"
				 "    if (!qet.elementInfo(f, u[i], 'formula')) return u[i];\n"
				 "}\n"
				 "function master() {\n"
				 "  for (var f = 0; f < qet.folioCount(); ++f) {\n"
				 "    var u = qet.elementUuids(f).slice().sort();\n"
				 "    for (var i = 0; i < u.length; ++i)\n"
				 "      if (qet.elementLinkType(f, u[i]) == 'master' && qet.linkedElements(f, u[i]).length)\n"
				 "        return [f, u[i]];\n"
				 "  }\n"
				 "}\n"
				 "var steps = [\n"
				 "  function () { return qet.setElementLabel(3, plain(3), 'QX1'); },\n"
				 "  function () { return qet.setElementInfo(3, first(3), 'comment', 'edited'); },\n"
				 "  function () { return qet.moveElement(4, first(4), 0, 100); },\n"
				 "  function () { return qet.rotateElement(4, first(4), 90); },\n"
				 "  function () { return qet.deleteElement(5, first(5)); },\n"
				 "  function () { var m = master(); return !!m && qet.unlinkElement(m[0], m[1]); },\n"
				 "  function () { return qet.setConductorPropertyByUuid(6, wire(6), 'function', 'edited'); },\n"
				 "  function () { return qet.setFolioProperty(7, 'title', 'Edited'); },\n"
				 "  function () { return qet.undo(); },\n"
				 "  function () { return qet.redo(); },\n"
				 "  function () { return qet.removeFolio(8); },\n"
					//Folio 8 is not built: its rows are put back (F137)
				 "  function () { return qet.undo(); }\n"
				 "];\n"
				 "function save(k) { if (typeof SAVEDIR !== 'undefined') qet.save(SAVEDIR + '/' + k + '/industrial.qet'); }\n"
				 "save(0);\n"
				 "qet.log('DONE 0 true');\n"
				 "for (var k = 0; k < steps.length; ++k) {\n"
				 "  var ok = steps[k]();\n"
				 "  save(k + 1);\n"
				 "  qet.log('DONE ' + (k + 1) + ' ' + ok);\n"
				 "}\n"));
		js.close();

			//The reference: the project saved after each edit
		const QString saves = m_dir.filePath(QStringLiteral("reference"));
		const QString reference = m_dir.filePath(QStringLiteral("reference.js"));
		QFile ref(reference);
		QVERIFY(ref.open(QIODevice::WriteOnly));
		QFile edits(m_script);
		QVERIFY(edits.open(QIODevice::ReadOnly));
		ref.write(QStringLiteral("var SAVEDIR = '%1';\n").arg(saves).toUtf8() + edits.readAll());
		ref.close();
		for (int k = 0; k <= LAST; ++k) QDir().mkpath(saves + QStringLiteral("/%1").arg(k));
		const QString log = runQet({QStringLiteral("--run"), reference, m_source});
		QVERIFY2(lastDone(log) == LAST, qPrintable(log.right(600)));
		for (int k = 0; k <= LAST; ++k) {
			m_reference << savedHash(saves + QStringLiteral("/%1/industrial.qet").arg(k));
			QVERIFY(!m_reference.last().isEmpty());
			if (k) QVERIFY2(m_reference.at(k) != m_reference.at(k - 1),
							qPrintable(QStringLiteral("edit %1 changes nothing saved").arg(k)));
		}
	}

	// Run to the end: every edit, and no file left in the folder. Times
	// the open (to DONE 0) and the edits (DONE 0 to DONE LAST).
	void cleanExitLeavesNothing()
	{
		const QString copies = m_dir.filePath(QStringLiteral("clean"));
		QDir().mkpath(copies);
		QProcess proc;
		proc.setProcessEnvironment(environment(newHome(), copies));
		proc.setProcessChannelMode(QProcess::MergedChannels);
		QElapsedTimer timer;
		timer.start();
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), m_script, m_source});
		QString log;
		while (proc.state() != QProcess::NotRunning || proc.bytesAvailable()) {
			proc.waitForReadyRead(5);
			log += QString::fromUtf8(proc.readAll());
			if (!m_open_ms && lastDone(log) >= 0) m_open_ms = timer.elapsed();
			if (!m_edits_ms && lastDone(log) == LAST) m_edits_ms = timer.elapsed() - m_open_ms;
		}
		QVERIFY2(lastDone(log) == LAST, qPrintable(log.right(600)));
		QVERIFY2(workingCopies(copies).isEmpty(), qPrintable(workingCopies(copies).join(QLatin1Char(' '))));
	}

	// Killed at random moments: the copy holds every edit finished. One
	// run in ten is killed while it opens the project, one as soon as it
	// logs DONE LAST, the others between its DONE 0 and a little after
	// its DONE LAST.
	void killedKeepsEveryEdit()
	{
		QVERIFY(m_open_ms > 0 && m_edits_ms > 0);
		const int kills = qEnvironmentVariableIsSet("QET_WORKING_COPY_KILLS")
				? qEnvironmentVariableIntValue("QET_WORKING_COPY_KILLS") : 25;
		const int parallel = 8;
		int not_ready = 0;
		QMap<int, int> at_edit;
		for (int batch = 0; batch < kills; batch += parallel) {
			QList<QProcess *> runs;
			QStringList dirs;
			QList<qint64> kill_at;   // from the start, or (-delay) from DONE 0
			QList<bool> at_last;     // or as soon as the last edit is done
			for (int i = batch; i < qMin(kills, batch + parallel); ++i) {
				const QString copies = m_dir.filePath(QStringLiteral("killed%1").arg(i));
				QDir().mkpath(copies);
				auto *proc = new QProcess(this);
				proc->setProcessEnvironment(environment(newHome(), copies));
				proc->setProcessChannelMode(QProcess::MergedChannels);
				proc->start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), m_script, m_source});
				QVERIFY(proc->waitForStarted());
				runs << proc;
				dirs << copies;
				kill_at << (i % 10 == 0 ? QRandomGenerator::global()->bounded(m_open_ms)
										: -1 - QRandomGenerator::global()->bounded(m_edits_ms * 3 / 2));
				at_last << (i % 10 == 5);
			}
			QElapsedTimer timer;
			timer.start();
			QList<qint64> opened(runs.size(), -1);
			QList<int> done(runs.size(), -1);
			QStringList pending(runs.size());
			int running = runs.size();
			while (running) {
				running = 0;
				for (int i = 0; i < runs.size(); ++i) {
					QProcess *proc = runs.at(i);
					if (proc->state() == QProcess::NotRunning) continue;
					++running;
						//Whole lines only: a DONE line can come in two reads
					pending[i] += QString::fromUtf8(proc->readAll());
					const int end = pending.at(i).lastIndexOf(QLatin1Char('\n')) + 1;
					done[i] = qMax(done.at(i), lastDone(pending.at(i).left(end)));
					pending[i].remove(0, end);
					if (opened.at(i) < 0 && done.at(i) >= 0) opened[i] = timer.elapsed();
					const qint64 at = kill_at.at(i) >= 0 ? kill_at.at(i)
							: opened.at(i) < 0 ? -1 : opened.at(i) - kill_at.at(i) - 1;
					if ((at >= 0 && timer.elapsed() >= at) || (at_last.at(i) && done.at(i) == LAST)) proc->kill();
				}
					//Every pipe read as it comes: one left full would stop its run
				if (running) QTest::qWait(2);
			}
			for (int i = 0; i < runs.size(); ++i) {
				QProcess *proc = runs.at(i);
				proc->waitForFinished(10000);
				const int last = qMax(done.at(i), lastDone(pending.at(i) + QString::fromUtf8(proc->readAll())));
				delete proc;
				const QStringList copies = workingCopies(dirs.at(i), QStringLiteral("*.sqlite"));
				if (last < 0) {
						//Killed while opening: nothing to recover
					++not_ready;
					if (copies.isEmpty()) continue;
				}
				if (last == LAST && copies.isEmpty()) {
					++at_edit[LAST];   // ended before the kill: nothing left
					continue;
				}
				QCOMPARE(copies.size(), 1);
				const QString written_dir = dirs.at(i) + QStringLiteral("/written");
				QDir().mkpath(written_dir);
				const QString written = written_dir + QStringLiteral("/industrial.qet");
				int exit_code = -1;
				const QString log = runQet({QStringLiteral("--from-database"),
											dirs.at(i) + QLatin1Char('/') + copies.first(), written},
										   QString(), false, &exit_code);
				if (last < 0) {
					QVERIFY2(exit_code != 0 && log.contains(QStringLiteral("not ready")), qPrintable(log));
					continue;
				}
				QVERIFY2(exit_code == 0, qPrintable(log));
				const QByteArray hash = savedHash(written);
				const bool k = hash == m_reference.at(last);
				const bool k1 = last < LAST && hash == m_reference.at(last + 1);
				QVERIFY2(k || k1, qPrintable(QStringLiteral("killed after DONE %1: the copy is neither "
															"save %1 nor save %2 (%3)")
											 .arg(last).arg(last + 1).arg(dirs.at(i))));
				++at_edit[k ? last : last + 1];
			}
		}
		QString seen;
		for (auto it = at_edit.cbegin(); it != at_edit.cend(); ++it)
			seen += QStringLiteral(" %1:%2").arg(it.key()).arg(it.value());
		qInfo().noquote() << kills << "kills," << not_ready << "while opening; copies at edit" << seen;
	}

	// A project opened read-only never makes its copy ready
	void readOnlyNeverReady()
	{
		const QString read_only = m_dir.filePath(QStringLiteral("readonly/industrial.qet"));
		QDir().mkpath(QFileInfo(read_only).path());
		QVERIFY(QFile::copy(m_source, read_only));
		QVERIFY(QFile::setPermissions(read_only, QFile::ReadOwner | QFile::ReadGroup | QFile::ReadOther));
		const QString copies = m_dir.filePath(QStringLiteral("readonly-copies"));
		QDir().mkpath(copies);
		QProcess proc;
		proc.setProcessEnvironment(environment(newHome(), copies));
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--run"), m_script, read_only});
		QVERIFY(proc.waitForStarted());
		QString log;
		while (lastDone(log) < 5 && proc.state() == QProcess::Running) {
			proc.waitForReadyRead(100);
			log += QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		}
		QVERIFY2(lastDone(log) >= 0, qPrintable(log.right(600)));
		proc.kill();
		proc.waitForFinished();
		const QStringList left = workingCopies(copies, QStringLiteral("*.sqlite"));
		if (left.isEmpty()) return;   // ended before the kill
		int exit_code = 0;
		const QString written = runQet({QStringLiteral("--from-database"), copies + QLatin1Char('/') + left.first(),
										m_dir.filePath(QStringLiteral("readonly/written.qet"))},
									   QString(), false, &exit_code);
		QVERIFY2(exit_code != 0 && written.contains(QStringLiteral("not ready")), qPrintable(written));
	}
};

QTEST_GUILESS_MAIN(tst_workingcopy)
#include "tst_workingcopy.moc"
