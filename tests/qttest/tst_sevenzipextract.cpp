// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QDirIterator>
#include <QTemporaryDir>

#include "import/edz/edzsevenzip.h"
#include "import/edz/lzma/7zCrc.h"

// sevenZipExtract() keeps every entry inside the folder it extracts to: a
// name that is absolute or climbs out with ".." stops the extraction, and
// nothing is written outside that folder.
//
// The archives are made here from fixtures/sevenzip_template.7z, one
// stored entry of 120 characters with an uncompressed header: the test
// writes another name of the same length over it and recomputes the two
// header checksums, as a crafted archive would carry them.
class tst_sevenzipextract : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	QByteArray m_template;
	int m_name_length = 0;

	static constexpr char kFill = 'n';

		// the template with its entry named @p name, written as @p file
	bool archiveNamed(QString name, const QString &file) const
	{
		const QString tail = QStringLiteral(".txt");
		if (name.size() + tail.size() > m_name_length) return false;
		name += QString(m_name_length - name.size() - tail.size(), QLatin1Char('p')) + tail;

		QByteArray data = m_template;
		const QString old_name = QString(m_name_length - tail.size(), QLatin1Char(kFill)) + tail;
		const QByteArray old_utf16(reinterpret_cast<const char *>(old_name.utf16()), old_name.size() * 2);
		const QByteArray new_utf16(reinterpret_cast<const char *>(name.utf16()), name.size() * 2);
		const int at = data.indexOf(old_utf16);
		if (at < 0) return false;
		data.replace(at, old_utf16.size(), new_utf16);

			// 7z start header: CRC at 8 over bytes 12..31; next header
			// offset (12) and size (20) from byte 32, its CRC at 28
		auto u64 = [&data](int pos) {
			quint64 v = 0;
			for (int i = 7; i >= 0; --i) v = (v << 8) | quint8(data.at(pos + i));
			return v;
		};
		auto put32 = [&data](int pos, quint32 v) {
			for (int i = 0; i < 4; ++i) data[pos + i] = char((v >> (8 * i)) & 0xFF);
		};
		const quint64 offset = u64(12), size = u64(20);
		if (32 + offset + size > quint64(data.size())) return false;
		put32(28, CrcCalc(data.constData() + 32 + offset, size_t(size)));
		put32(8, CrcCalc(data.constData() + 12, 20));

		QFile f(file);
		return f.open(QIODevice::WriteOnly) && f.write(data) == data.size();
	}

		// every file under @p base, this row's folder, except the archive
		// and what is inside @p dest
	static QStringList filesOutside(const QString &base, const QString &dest)
	{
		QStringList found;
		QDirIterator it(base, QDir::Files, QDirIterator::Subdirectories);
		while (it.hasNext()) {
			const QString f = it.next();
			if (!f.endsWith(QStringLiteral(".7z")) && !f.startsWith(dest + QLatin1Char('/')))
				found << f;
		}
		return found;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QFile f(QFINDTESTDATA("fixtures/sevenzip_template.7z"));
		QVERIFY(f.open(QIODevice::ReadOnly));
		m_template = f.readAll();
		m_name_length = 120;
		const QString name = QString(m_name_length - 4, QLatin1Char(kFill)) + QStringLiteral(".txt");
		QVERIFY2(m_template.contains(QByteArray(reinterpret_cast<const char *>(name.utf16()),
												name.size() * 2)),
				 "the template's entry is not the 120-character name the test writes over");
		CrcGenerateTable();
	}

	void extract_data()
	{
		QTest::addColumn<QString>("name");
		QTest::addColumn<bool>("inside");

		QTest::newRow("plain name") << QStringLiteral("ok_") << true;
		QTest::newRow("folder, then back into it") << QStringLiteral("sub/../ok_") << true;
		QTest::newRow("one level up") << QStringLiteral("../up_") << false;
		QTest::newRow("two levels up") << QStringLiteral("../../up_") << false;
		QTest::newRow("up from a folder") << QStringLiteral("sub/../../up_") << false;
		QTest::newRow("backslashes up") << QStringLiteral("..\\..\\up_") << false;
		QTest::newRow("drive letter") << QStringLiteral("C:/drive_") << false;
		QTest::newRow("network share") << QStringLiteral("//host/share/unc_") << false;
			// Windows drops a trailing space or dot: ".. " would become ".."
		QTest::newRow("up, with a trailing space") << QStringLiteral(".. /up_") << false;
		QTest::newRow("folder ending in a dot") << QStringLiteral("sub./dot_") << false;
		QTest::newRow("hidden stream") << QStringLiteral("ok_stream:x") << false;
		QTest::newRow("dot folder") << QStringLiteral("./sub/ok_") << true;
			// an absolute name pointing into this test's own folder
		QTest::newRow("absolute") << QStringLiteral("ABSOLUTE") << false;
	}

	void extract()
	{
		QFETCH(QString, name);
		QFETCH(bool, inside);
		static int run = 0;
		const QString base = m_dir.filePath(QStringLiteral("run%1").arg(++run));
		const QString dest = base + QStringLiteral("/a/b/dest");
		QVERIFY(QDir().mkpath(dest));
		if (name == QLatin1String("ABSOLUTE")) {
			name = base + QStringLiteral("/abs_");
			if (name.size() + 4 > m_name_length)
				QSKIP("the temporary folder's path is too long for the template's name");
		}

		const QString archive = base + QStringLiteral("/test.7z");
		QVERIFY2(archiveNamed(name, archive), qPrintable(name));

		QString error;
		const bool ok = sevenZipExtract(archive, dest, error);
		QCOMPARE(ok, inside);
		QCOMPARE(filesOutside(base, dest), QStringList());
		if (inside) {
			QVERIFY(error.isEmpty());
			QDirIterator it(dest, QDir::Files, QDirIterator::Subdirectories);
			QVERIFY(it.hasNext());
			it.next();
			QVERIFY(!it.hasNext());
		} else {
			QVERIFY2(error.contains(QStringLiteral("outside")), qPrintable(error));
		}
	}
};

QTEST_GUILESS_MAIN(tst_sevenzipextract)
#include "tst_sevenzipextract.moc"
