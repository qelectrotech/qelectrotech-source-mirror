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
#include <QRegularExpression>
#include <QTemporaryDir>

/**
	A free text (<input>) whose x, y or rotation is "nan", "inf" or "-inf"
	is loaded at 0 for that number, as conductors and element texts are.
	Those strings parse as numbers, so the text used to keep them: a resave
	wrote them back, and the SVG export drew the text at translate(nan,...).
	Runs the real binary's --resave and --export-svg on
	fixtures/free_text_nonfinite.qet.
*/
class tst_freetextnonfinite : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;
	QString m_fixture;

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

	/// The saved <input> whose text contains @p word.
	static QDomElement input(const QDomDocument &doc, const QString &word)
	{
		const QDomNodeList list = doc.elementsByTagName(QStringLiteral("input"));
		for (int i = 0; i < list.size(); ++i) {
			const QDomElement e = list.at(i).toElement();
			if (e.attribute(QStringLiteral("text")).contains(word)) return e;
		}
		return QDomElement();
	}

private slots:
	void initTestCase()
	{
		QVERIFY2(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)), "qelectrotech binary not found");
		QVERIFY(m_dir.isValid());
		m_fixture = QFINDTESTDATA("fixtures/free_text_nonfinite.qet");
		QVERIFY2(!m_fixture.isEmpty(), "fixture project not found");
	}

	void savedNumbersAreFinite_data()
	{
		QTest::addColumn<QString>("word");
		QTest::addColumn<QString>("x");
		QTest::addColumn<QString>("y");
		QTest::addColumn<QString>("rotation");
		QTest::newRow("x nan")        << "NanX"        << "0"    << "100"  << "0";
		QTest::newRow("y inf")        << "InfY"        << "200"  << "0"    << "0";
		QTest::newRow("x -inf")       << "MinusInfX"   << "0"    << "300"  << "0";
		QTest::newRow("rotation nan") << "NanRotation" << "400"  << "400"  << "0";
		// A finite text, with values that round differently, is kept as it is.
		QTest::newRow("finite")       << "Plain"       << "61.7" << "61.3" << "37";
	}

	void savedNumbersAreFinite()
	{
		QFETCH(QString, word);
		QFETCH(QString, x);
		QFETCH(QString, y);
		QFETCH(QString, rotation);

		const QString out = m_dir.filePath(QStringLiteral("resaved-%1.qet").arg(word));
		QVERIFY2(runQet({QStringLiteral("--resave"), m_fixture, out}), "--resave failed");
		QFile file(out);
		QVERIFY(file.open(QIODevice::ReadOnly));
		QDomDocument doc;
		QVERIFY(doc.setContent(&file));

		const QDomElement e = input(doc, word);
		QVERIFY2(!e.isNull(), "text not found in the resaved project");
		QCOMPARE(e.attribute(QStringLiteral("x")), x);
		QCOMPARE(e.attribute(QStringLiteral("y")), y);
		QCOMPARE(e.attribute(QStringLiteral("rotation")), rotation);
	}

	// No "nan" or "inf" reaches the exported drawing.
	void exportHasNoNonFiniteNumbers()
	{
		const QString dir = m_dir.filePath(QStringLiteral("svg"));
		QVERIFY(QDir().mkpath(dir));
		QVERIFY2(runQet({QStringLiteral("--export-svg"), m_fixture, dir}), "--export-svg failed");
		const QStringList files = QDir(dir).entryList({QStringLiteral("*.svg")}, QDir::Files);
		QCOMPARE(files.size(), 1);
		QFile svg(QDir(dir).filePath(files.first()));
		QVERIFY(svg.open(QIODevice::ReadOnly));
		const QString content = QString::fromUtf8(svg.readAll());
		const QRegularExpression non_finite(QStringLiteral("[(,\\s=\"]-?(nan|inf)\\b"),
						    QRegularExpression::CaseInsensitiveOption);
		const QRegularExpressionMatch match = non_finite.match(content);
		QVERIFY2(!match.hasMatch(),
			 qPrintable(QStringLiteral("found \"%1\" in the SVG").arg(
					    content.mid(qMax(0, int(match.capturedStart()) - 30), 60))));
		QVERIFY(content.contains(QStringLiteral("NanX")));
	}
};

QTEST_MAIN(tst_freetextnonfinite)
#include "tst_freetextnonfinite.moc"
