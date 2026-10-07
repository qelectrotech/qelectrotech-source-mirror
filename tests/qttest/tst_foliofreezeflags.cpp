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
	A folio's "freeze new elements" and "freeze new conductors" flags are
	saved as freezeNewElement="true" and freezeNewConductor="true" on its
	<diagram>. Loading must read the words back: the flags used to be read
	with toInt(), which is 0 for "true" as well as for "false", so both were
	off again after every reload. Runs the real binary's --resave on a
	project with one folio per combination and reads the saved attributes.
*/
class tst_foliofreezeflags : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

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

	/// The <diagram> elements of a saved project, in file order.
	static QList<QDomElement> diagrams(const QString &path)
	{
		QFile file(path);
		QDomDocument doc;
		if (!file.open(QIODevice::ReadOnly) || !doc.setContent(&file)) return {};
		QList<QDomElement> list;
		for (QDomElement e = doc.documentElement().firstChildElement(QStringLiteral("diagram"));
		     !e.isNull();
		     e = e.nextSiblingElement(QStringLiteral("diagram")))
			list << e;
		return list;
	}

private slots:
	void initTestCase()
	{
		QVERIFY2(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)), "qelectrotech binary not found");
		QVERIFY(m_dir.isValid());
	}

	// Folio 1 has both flags, folio 2 only the element one, folio 3 none,
	// and folio 4 is an older file without the attributes at all.
	void flagsSurviveASave_data()
	{
		QTest::addColumn<int>("folio");
		QTest::addColumn<QString>("element");
		QTest::addColumn<QString>("conductor");
		QTest::newRow("both frozen")    << 0 << "true"  << "true";
		QTest::newRow("elements frozen") << 1 << "true"  << "false";
		QTest::newRow("not frozen")     << 2 << "false" << "false";
		QTest::newRow("no attributes")  << 3 << "false" << "false";
	}

	void flagsSurviveASave()
	{
		QFETCH(int, folio);
		QFETCH(QString, element);
		QFETCH(QString, conductor);

		const QString fixture = QFINDTESTDATA("fixtures/folio_freeze_flags.qet");
		QVERIFY2(!fixture.isEmpty(), "fixture project not found");
		const QString out = m_dir.filePath(QStringLiteral("resaved%1.qet").arg(folio));
		QVERIFY2(runQet({QStringLiteral("--resave"), fixture, out}), "--resave failed");

		const QList<QDomElement> list = diagrams(out);
		QCOMPARE(list.size(), 4);
		const QDomElement e = list.at(folio);
		QCOMPARE(e.attribute(QStringLiteral("freezeNewElement")), element);
		QCOMPARE(e.attribute(QStringLiteral("freezeNewConductor")), conductor);
	}
};

QTEST_MAIN(tst_foliofreezeflags)
#include "tst_foliofreezeflags.moc"
