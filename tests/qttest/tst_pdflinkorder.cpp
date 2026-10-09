// SPDX-License-Identifier: GPL-2.0-or-later
/*
	--export-pdf writes a project's cross-reference links in the same order
	every time. They used to come out in the order of Element pointers, so
	the same project gave a different PDF in each run. Exported in separate
	processes, since pointers only change between runs.
*/
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest>

class tst_pdflinkorder : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	// The link annotations' rectangles and destinations, in file order.
	QStringList exportLinks(const QString &project, int run)
	{
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(run));
		const QString tmp = m_dir.filePath(QStringLiteral("tmp%1").arg(run));
		const QString out = m_dir.filePath(QStringLiteral("out%1.pdf").arg(run));
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
				   {QStringLiteral("--export-pdf"), project, out});
		if (!proc.waitForFinished(120000) || proc.exitCode() != 0)
			return {};

		QFile file(out);
		if (!file.open(QIODevice::ReadOnly))
			return {};
		const QString pdf = QString::fromLatin1(file.readAll());
		static const QRegularExpression link(QStringLiteral(
			R"(/Subtype /Link\s*/Rect \[([^\]]*)\][\s\S]*?/D \[\d+ 0 R ([^\]]*)\])"));
		QStringList links;
		auto it = link.globalMatch(pdf);
		while (it.hasNext()) {
			const auto m = it.next();
			links << m.captured(1).simplified() + QStringLiteral(" -> ")
					 + m.captured(2).simplified();
		}
		return links;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void sameOrderEveryRun()
	{
		const QString project =
			QStringLiteral(QET_EXAMPLES_DIR) + QStringLiteral("/industrial.qet");
		QVERIFY2(QFile::exists(project), "examples/industrial.qet not found");

		const QStringList first = exportLinks(project, 0);
		QVERIFY2(first.size() >= 2, "expected several cross-reference links");
		for (int run = 1; run < 5; ++run)
			QCOMPARE(exportLinks(project, run), first);
	}
};

QTEST_MAIN(tst_pdflinkorder)
#include "tst_pdflinkorder.moc"
