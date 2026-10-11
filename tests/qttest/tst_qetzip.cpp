// SPDX-License-Identifier: GPL-2.0-or-later
#include <QtTest>

#include <QTemporaryDir>

#include "container/qetzip.h"
#include "container/miniz/miniz.h"

// The zip a .qetz project is stored in (sources/container/qetzip.*):
// entries come back as written, in order, stored ones stored; the same
// entries give the same bytes; unsafe names are refused both ways.
class tst_qetzip : public QObject
{
	Q_OBJECT

	static QList<QetZip::Entry> sample()
	{
		QList<QetZip::Entry> entries;
		entries.append({QStringLiteral("mimetype"), QByteArray("application/x-qelectrotech-project+zip"), false});
		QByteArray xml("<project>");
		for (int i = 0 ; i < 2000 ; ++i) xml += "<element name=\"K1\"/>";
		xml += "</project>";
		entries.append({QStringLiteral("project.xml"), xml, true});
		QByteArray picture;
		for (int i = 0 ; i < 4096 ; ++i) picture += char((i * 7919) & 0xff);
		entries.append({QStringLiteral("images/00.png"), picture, false});
		entries.append({QStringLiteral("folios/é ü.xml"), QByteArray("<diagram/>"), true});
		entries.append({QStringLiteral("empty"), QByteArray(), true});
		return entries;
	}

	// a zip holding one entry named @p name, written past QetZip's checks
	static QByteArray rawZip(const char *name)
	{
		mz_zip_archive zip;
		mz_zip_zero_struct(&zip);
		mz_zip_writer_init_heap(&zip, 0, 0);
		mz_zip_writer_add_mem(&zip, name, "x", 1, MZ_DEFAULT_LEVEL);
		void *buffer = nullptr;
		size_t size = 0;
		mz_zip_writer_finalize_heap_archive(&zip, &buffer, &size);
		const QByteArray bytes(static_cast<const char *>(buffer), qsizetype(size));
		mz_zip_writer_end(&zip);
		return bytes;
	}

private slots:
	void roundTrip()
	{
		const QList<QetZip::Entry> in = sample();
		QString error;
		const QByteArray zip = QetZip::toBytes(in, &error);
		QVERIFY2(!zip.isEmpty(), qPrintable(error));
		QVERIFY(zip.startsWith("PK"));
		QVERIFY2(zip.size() < in.at(1).data.size() / 4, "the XML entry was not compressed");

		QList<QetZip::Entry> out;
		QVERIFY2(QetZip::fromBytes(zip, &out, &error), qPrintable(error));
		QCOMPARE(out.size(), in.size());
		for (int i = 0 ; i < in.size() ; ++i) {
			QCOMPARE(out.at(i).name, in.at(i).name);
			QCOMPARE(out.at(i).data, in.at(i).data);
			if (!in.at(i).data.isEmpty())
				QCOMPARE(out.at(i).compress, in.at(i).compress);
		}
		// mimetype first and stored: its bytes appear as is after the header
		QCOMPARE(zip.indexOf("application/x-qelectrotech-project+zip"), 30 + int(qstrlen("mimetype")));
	}

	void sameBytesTwice()
	{
		QCOMPARE(QetZip::toBytes(sample()), QetZip::toBytes(sample()));
	}

	void refusesUnsafeNamesOnWrite_data()
	{
		QTest::addColumn<QString>("name");
		QTest::newRow("parent") << QStringLiteral("../x.xml");
		QTest::newRow("inner parent") << QStringLiteral("folios/../../x.xml");
		QTest::newRow("absolute") << QStringLiteral("/etc/x");
		QTest::newRow("drive") << QStringLiteral("C:/x");
		QTest::newRow("backslash") << QStringLiteral("folios\\x.xml");
		QTest::newRow("empty") << QString();
	}
	void refusesUnsafeNamesOnWrite()
	{
		QFETCH(QString, name);
		QString error;
		QVERIFY(QetZip::toBytes({{name, QByteArray("x"), true}}, &error).isEmpty());
		QVERIFY(!error.isEmpty());
	}

	void refusesRepeatedNames()
	{
		QString error;
		QVERIFY(QetZip::toBytes({{QStringLiteral("a"), "1", true}, {QStringLiteral("a"), "2", true}}, &error).isEmpty());
	}

	void refusesUnsafeNamesOnRead()
	{
		QList<QetZip::Entry> out;
		QString error;
		QVERIFY(QetZip::fromBytes(rawZip("ok/name.xml"), &out, &error));
		QVERIFY(!QetZip::fromBytes(rawZip("../evil.xml"), &out, &error));
		QVERIFY(out.isEmpty());
		// miniz will not write a name starting with '/': write "Xabs.xml"
		// and turn the X into '/' in both of its headers
		QByteArray absolute = rawZip("Xabs.xml");
		QCOMPARE(absolute.count("Xabs.xml"), 2);
		absolute.replace("Xabs.xml", "/abs.xml");
		QVERIFY(!QetZip::fromBytes(absolute, &out, &error));
	}

	void refusesGarbage()
	{
		QList<QetZip::Entry> out;
		QString error;
		QVERIFY(!QetZip::fromBytes(QByteArray("not a zip at all"), &out, &error));
		QVERIFY(!error.isEmpty());
		QByteArray cut = QetZip::toBytes(sample());
		cut.chop(cut.size() / 2);
		QVERIFY(!QetZip::fromBytes(cut, &out, &error));
	}

	void writeAndRead()
	{
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const QString path = dir.filePath(QStringLiteral("p.qetz"));
		QString error;
		QVERIFY2(QetZip::write(path, sample(), &error), qPrintable(error));
		QList<QetZip::Entry> out;
		QVERIFY2(QetZip::read(path, &out, &error), qPrintable(error));
		QCOMPARE(out.size(), sample().size());
		QCOMPARE(out.at(1).data, sample().at(1).data);
		// a refused write leaves the file as it was
		QVERIFY(!QetZip::write(path, {{QStringLiteral("../x"), "x", true}}, &error));
		QVERIFY(QetZip::read(path, &out, &error));
		QCOMPARE(out.size(), sample().size());
	}
};

QTEST_APPLESS_MAIN(tst_qetzip)

#include "tst_qetzip.moc"
