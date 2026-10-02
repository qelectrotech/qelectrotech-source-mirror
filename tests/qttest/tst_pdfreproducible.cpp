// SPDX-License-Identifier: GPL-2.0-or-later
/*
	With SOURCE_DATE_EPOCH set, --export-pdf writes the same bytes for the
	same project in every run: the dates are the ones it names, the document
	id comes from the project file, and the fonts come in a fixed order.
	Without it the dates are the time of the export, as before. Exported in
	separate processes, since what used to differ changed between runs.
*/
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QTemporaryDir>
#include <QtTest>

class tst_pdfreproducible : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	QByteArray exportPdf(const QString &project, const QByteArray &epoch)
	{
		const int run = m_run++;
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
		env.remove(QStringLiteral("QT_HASH_SEED"));
		env.remove(QStringLiteral("SOURCE_DATE_EPOCH"));
		if (!epoch.isEmpty())
			env.insert(QStringLiteral("SOURCE_DATE_EPOCH"), QString::fromLatin1(epoch));

		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--export-pdf"), project, out});
		if (!proc.waitForFinished(120000) || proc.exitCode() != 0)
			return {};
		QFile file(out);
		if (!file.open(QIODevice::ReadOnly))
			return {};
		return file.readAll();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

	void sameBytesEveryRun()
	{
		// A project with no cross-reference links: their order is fixed
		// separately.
		const QString project =
			QStringLiteral(QET_EXAMPLES_DIR) + QStringLiteral("/741.qet");
		QVERIFY2(QFile::exists(project), "examples/741.qet not found");

		const QByteArray first = exportPdf(project, "1700000000");
		QVERIFY(!first.isEmpty());
		QVERIFY(first.contains("/CreationDate (D:20231114221320Z)"));
		QVERIFY(first.contains("xmp:CreateDate=\"2023-11-14T22:13:20Z\""));
		for (int i = 0; i < 3; ++i)
			QVERIFY2(exportPdf(project, "1700000000") == first,
					 "two exports of the same project differ");
	}

	void nowWithoutTheVariable()
	{
		const QString project =
			QStringLiteral(QET_EXAMPLES_DIR) + QStringLiteral("/741.qet");
		const QByteArray pdf = exportPdf(project, QByteArray());
		QVERIFY(!pdf.isEmpty());
		const QByteArray year =
			"/CreationDate (D:" + QByteArray::number(QDate::currentDate().year());
		QVERIFY(pdf.contains(year));
	}
};

QTEST_MAIN(tst_pdfreproducible)
#include "tst_pdfreproducible.moc"
