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
#include <QDir>
#include <QDomDocument>
#include <QProcess>
#include <QTemporaryDir>

/**
	A picture on a folio (<image>, without a <transform> child) whose size
	attribute is missing, unreadable, "nan" or 0 is loaded at size 1. It
	used to be loaded at size 0, so it was invisible, not exported, and a
	resave wrote size="0" back. Runs the real binary's --resave and
	--export-svg on fixtures/image_missing_size.qet: six 20x10 pictures,
	one per size value, told apart by their y position.
*/
class tst_imagemissingsize : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	QString m_fixture;
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

private slots:
	void initTestCase()
	{
		QVERIFY2(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)), "qelectrotech binary not found");
		QVERIFY(m_dir.isValid());
		m_fixture = QFINDTESTDATA("fixtures/image_missing_size.qet");
		QVERIFY2(!m_fixture.isEmpty(), "fixture project not found");
		const QString out = m_dir.filePath(QStringLiteral("resaved.qet"));
		QVERIFY2(runQet({QStringLiteral("--resave"), m_fixture, out}), "--resave failed");
		QFile file(out);
		QVERIFY(file.open(QIODevice::ReadOnly));
		QVERIFY(m_resaved.setContent(&file));
	}

	void savedSize_data()
	{
		QTest::addColumn<QString>("y");
		QTest::addColumn<QString>("size");
		QTest::newRow("missing")       << "100" << "1";
		QTest::newRow("0")             << "160" << "1";
		QTest::newRow("nan")           << "220" << "1";
		QTest::newRow("not a number")  << "280" << "1";
		// Usable sizes are kept, below and above 1.
		QTest::newRow("0.5")           << "340" << "0.5";
		QTest::newRow("2")             << "400" << "2";
	}

	void savedSize()
	{
		QFETCH(QString, y);
		QFETCH(QString, size);
		const QDomNodeList list = m_resaved.elementsByTagName(QStringLiteral("image"));
		QDomElement found;
		for (int i = 0; i < list.size(); ++i) {
			const QDomElement e = list.at(i).toElement();
			if (e.attribute(QStringLiteral("y")) == y) found = e;
		}
		QVERIFY2(!found.isNull(), "picture not found in the resaved project");
		QCOMPARE(found.attribute(QStringLiteral("size")), size);
	}

	// All six pictures are drawn in the export.
	void allPicturesAreExported()
	{
		const QString dir = m_dir.filePath(QStringLiteral("svg"));
		QVERIFY(QDir().mkpath(dir));
		QVERIFY2(runQet({QStringLiteral("--export-svg"), m_fixture, dir}), "--export-svg failed");
		const QStringList files = QDir(dir).entryList({QStringLiteral("*.svg")}, QDir::Files);
		QCOMPARE(files.size(), 1);
		QFile svg(QDir(dir).filePath(files.first()));
		QVERIFY(svg.open(QIODevice::ReadOnly));
		QCOMPARE(QString::fromUtf8(svg.readAll()).count(QStringLiteral("<image")), 6);
	}
};

QTEST_MAIN(tst_imagemissingsize)
#include "tst_imagemissingsize.moc"
