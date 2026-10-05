// SPDX-License-Identifier: GPL-2.0-or-later
/*
	With SOURCE_DATE_EPOCH set, --export-pdf writes the same bytes for the
	same project in every run: the dates are the ones it names, the document
	id comes from the content of the PDF, and the fonts come in a fixed order.
	Without it the dates are the time of the export, as before. Exported in
	separate processes, since what used to differ changed between runs.
*/
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QtTest>
#include <QUuid>

class tst_pdfreproducible : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	int m_run = 0;

	QByteArray exportPdf(const QString &project, const QByteArray &epoch,
						 const QString &folder = QString())
	{
		const int run = m_run++;
		const QString home = m_dir.filePath(QStringLiteral("home%1").arg(run));
		const QString tmp = m_dir.filePath(QStringLiteral("tmp%1").arg(run));
		const QString out = folder.isEmpty()
			? m_dir.filePath(QStringLiteral("out%1.pdf").arg(run))
			: m_dir.filePath(folder + QStringLiteral("/same.pdf"));
		if (!folder.isEmpty())
			QDir().mkpath(m_dir.filePath(folder));
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

	/// Every object the xref table lists starts where the table says: the
	/// dates are rewritten after Qt writes the file, which moves them.
	static bool xrefMatches(const QByteArray &pdf)
	{
		const int sx = pdf.lastIndexOf("startxref");
		if (sx == -1)
			return false;
		const int xref = pdf.mid(sx + 9).trimmed().split('\n').value(0).toInt();
		if (!pdf.mid(xref).startsWith("xref"))
			return false;
		const QList<QByteArray> lines = pdf.mid(xref).split('\n');
		const QList<QByteArray> header = lines.value(1).split(' ');
		const int first = header.value(0).toInt();
		const int count = header.value(1).toInt();
		for (int i = 0; i < count; ++i) {
			const QList<QByteArray> entry = lines.value(2 + i).split(' ');
			if (entry.value(2) != "n")
				continue;
			const QByteArray obj = QByteArray::number(first + i) + " 0 obj";
			if (pdf.mid(entry.value(0).toInt(), obj.size()) != obj)
				return false;
		}
		return count > 0;
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
		QVERIFY(first.contains("xmp:CreateDate=\"2023-11-14T22:13:20Z\""));
#endif
		QVERIFY2(xrefMatches(first), "the xref table does not match the file");
		for (int i = 0; i < 3; ++i)
			QVERIFY2(exportPdf(project, "1700000000") == first,
					 "two exports of the same project differ");
	}

	void sameBytesWithNewUuids()
	{
		// #1178: a project generated again from the same data has the same
		// drawing but new uuids, and its PDF used to differ in the
		// document id, which came from the project file.
		const QString project =
			QStringLiteral(QET_EXAMPLES_DIR) + QStringLiteral("/741.qet");
		QFile in(project);
		QVERIFY(in.open(QIODevice::ReadOnly));
		QString xml = QString::fromUtf8(in.readAll());
		static const QRegularExpression uuid(QStringLiteral(
			"[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}"));
		QHash<QString, QString> renamed;
		QString regenerated;
		qsizetype last = 0;
		for (auto it = uuid.globalMatch(xml); it.hasNext(); ) {
			const auto match = it.next();
			const QString key = match.captured().toLower();
			if (!renamed.contains(key))
				renamed.insert(key, QUuid::createUuid().toString(QUuid::WithoutBraces));
			regenerated += xml.mid(last, match.capturedStart() - last) + renamed.value(key);
			last = match.capturedEnd();
		}
		regenerated += xml.mid(last);
		QVERIFY(renamed.size() > 10);
		const QString copy = m_dir.filePath(QStringLiteral("regenerated.qet"));
		QFile out(copy);
		QVERIFY(out.open(QIODevice::WriteOnly));
		out.write(regenerated.toUtf8());
		out.close();

		const QByteArray first = exportPdf(project, "1700000000");
		QVERIFY(!first.isEmpty());
		QVERIFY2(!first.contains("6f1c2d4e-9a3b-4c5d-8e7f-0a1b2c3d4e5f"),
				 "the placeholder document id was left in the file");
		QVERIFY2(xrefMatches(first), "the xref table does not match the file");
		QVERIFY2(exportPdf(copy, "1700000000") == first,
				 "the same drawing with new uuids gives a different PDF");
	}

	void sameBytesInAnyFolder()
	{
		// #1178: the links carry the output path, and rewriting them left
		// the old startxref in the file, with the size the file had before.
		const QString project =
			QStringLiteral(QET_EXAMPLES_DIR) + QStringLiteral("/iso_sfc_example.qet");
		QVERIFY2(QFile::exists(project), "examples/iso_sfc_example.qet not found");

		const QByteArray first = exportPdf(project, "1700000000", QStringLiteral("p"));
		QVERIFY(!first.isEmpty());
		QVERIFY(first.contains("/GoTo"));
		QCOMPARE(first.count("startxref"), 1);
		QVERIFY2(xrefMatches(first), "the xref table does not match the file");
		QVERIFY2(exportPdf(project, "1700000000", QStringLiteral("longer-folder-name")) == first,
				 "the same project gives a different PDF in another folder");
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
