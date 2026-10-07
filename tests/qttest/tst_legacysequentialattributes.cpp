/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <QtTest>
#include <QDomDocument>
#include <QProcess>
#include <QTemporaryDir>

/**
	Projects saved before the <sequentialNumbers> element keep an element's
	or conductor's sequential numbers as attributes: sequ_1, sequf_1, seqt_1,
	seqtf_1, seqh_1 and seqhf_1. The loaders look for one of them to take
	the old route; the list named sequf_1 twice and seqhf_1 never, so a
	file whose only sequence was the hundred-folio one lost it. Runs the
	real binary's --resave on fixtures/legacy_sequential_attributes.qet
	and reads the <sequentialNumbers> it writes.
*/
class tst_legacysequentialattributes : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	QDomDocument m_resaved;

	static bool runQet(const QStringList &arguments)
	{
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), arguments);
		return proc.waitForFinished(60000)
				&& proc.exitStatus() == QProcess::NormalExit
				&& proc.exitCode() == 0;
	}

	/// The first @p tag element of the saved project with this uuid.
	QDomElement item(const QString &tag, const QString &uuid) const
	{
		const QDomNodeList list = m_resaved.elementsByTagName(tag);
		for (int i = 0; i < list.size(); ++i) {
			const QDomElement e = list.at(i).toElement();
			if (e.attribute(QStringLiteral("uuid")) == uuid) return e;
		}
		return QDomElement();
	}

	/// The text of <sequentialNumbers><@p part> under @p e.
	static QString sequence(const QDomElement &e, const QString &part)
	{
		return e.firstChildElement(QStringLiteral("sequentialNumbers"))
				.firstChildElement(part).text();
	}

private slots:
	void initTestCase()
	{
		QVERIFY2(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)), "qelectrotech binary not found");
		QVERIFY(m_dir.isValid());
		const QString fixture = QFINDTESTDATA("fixtures/legacy_sequential_attributes.qet");
		QVERIFY2(!fixture.isEmpty(), "fixture project not found");
		const QString out = m_dir.filePath(QStringLiteral("resaved.qet"));
		QVERIFY2(runQet({QStringLiteral("--resave"), fixture, out}), "--resave failed");
		QFile file(out);
		QVERIFY(file.open(QIODevice::ReadOnly));
		QVERIFY(m_resaved.setContent(&file));
	}

	// The element and the conductor whose only old attribute is seqhf_1.
	void hundredFolioSequenceIsKept_data()
	{
		QTest::addColumn<QString>("tag");
		QTest::addColumn<QString>("uuid");
		QTest::addColumn<QString>("value");
		QTest::newRow("element")   << "element"   << "{6c58a5a1-aa4e-40c4-9e9d-a76524f76e52}" << "5";
		QTest::newRow("conductor") << "conductor" << "{f5162c59-0c59-4e94-b6a6-4cf259a52d91}" << "7";
	}

	void hundredFolioSequenceIsKept()
	{
		QFETCH(QString, tag);
		QFETCH(QString, uuid);
		QFETCH(QString, value);
		const QDomElement e = item(tag, uuid);
		QVERIFY2(!e.isNull(), "item not found in the resaved project");
		QVERIFY2(!e.hasAttribute(QStringLiteral("seqhf_1")), "the old attribute is not written back");
		QCOMPARE(sequence(e, QStringLiteral("hundredFolio")), value);
	}

	// The unit sequence, whose attribute was already in the list, as before.
	void unitSequenceIsKept_data()
	{
		QTest::addColumn<QString>("tag");
		QTest::addColumn<QString>("uuid");
		QTest::addColumn<QString>("value");
		QTest::newRow("element")   << "element"   << "{29b32d7b-5ec3-42c8-afda-788e9614c0fd}" << "3";
		QTest::newRow("conductor") << "conductor" << "{89cacce1-b12a-4ec3-aadf-d3f8feb7494d}" << "4";
	}

	void unitSequenceIsKept()
	{
		QFETCH(QString, tag);
		QFETCH(QString, uuid);
		QFETCH(QString, value);
		const QDomElement e = item(tag, uuid);
		QVERIFY2(!e.isNull(), "item not found in the resaved project");
		QCOMPARE(sequence(e, QStringLiteral("unit")), value);
	}
};

QTEST_MAIN(tst_legacysequentialattributes)
#include "tst_legacysequentialattributes.moc"
