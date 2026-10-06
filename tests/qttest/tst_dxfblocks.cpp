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
#include <QProcess>
#include <QTemporaryDir>

/**
	--export-dxf --dxf-blocks (issue #1339): a symbol placed twice is one
	BLOCK and two INSERTs, the second turned a quarter, and each symbol's
	information is a hidden attribute of it; with --dxf-attributes its
	label is a visible one. Without the switches there is neither.
*/
class tst_dxfblocks : public QObject
{
	Q_OBJECT

		/// The group code / value pairs of the exported folio
	static QList<QPair<QString, QString>> exportPairs(const QStringList &extra)
	{
		const QString project = QFINDTESTDATA("fixtures/dxf_blocks.qet");
		QTemporaryDir dir;
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   QStringList{QStringLiteral("--export-dxf"), project, dir.path()} + extra);
		if (!proc.waitForFinished(60000) || proc.exitCode() != 0)
			return {};
		QFile file(dir.filePath(QStringLiteral("01_diagram.dxf")));
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			return {};
		const QStringList values = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
		QList<QPair<QString, QString>> pairs;
		for (int i = 0 ; i + 1 < values.size() ; i += 2)
			pairs << qMakePair(values.at(i).trimmed(), values.at(i + 1).trimmed());
		return pairs;
	}

		/// The values of code @a code in each entity of type @a type
	static QStringList values(const QList<QPair<QString, QString>> &pairs,
							  const QString &type, const QString &code)
	{
		QStringList found;
		for (int i = 0 ; i < pairs.size() ; ++i) {
			if (pairs.at(i).first != QLatin1String("0") || pairs.at(i).second != type)
				continue;
			for (int j = i + 1 ; j < pairs.size() && pairs.at(j).first != QLatin1String("0") ; ++j)
				if (pairs.at(j).first == code)
					found << pairs.at(j).second;
		}
		return found;
	}

private slots:
	void blocks()
	{
		const auto pairs = exportPairs({QStringLiteral("--dxf-blocks")});
		QVERIFY2(!pairs.isEmpty(), "--export-dxf --dxf-blocks failed");

		QCOMPARE(values(pairs, QStringLiteral("BLOCK"), QStringLiteral("2")),
				 QStringList{QStringLiteral("QET_CURVES")});
		QCOMPARE(values(pairs, QStringLiteral("INSERT"), QStringLiteral("2")),
				 (QStringList{QStringLiteral("QET_CURVES"), QStringLiteral("QET_CURVES")}));
		QCOMPARE(values(pairs, QStringLiteral("INSERT"), QStringLiteral("50")),
				 (QStringList{QStringLiteral("0"), QStringLiteral("270")}));
			//The circle is drawn once, inside the block
		QCOMPARE(values(pairs, QStringLiteral("CIRCLE"), QStringLiteral("8")).size(), 1);
	}

	void labelsAsAttributes()
	{
		const auto pairs = exportPairs({QStringLiteral("--dxf-attributes")});
		QVERIFY2(!pairs.isEmpty(), "--export-dxf --dxf-attributes failed");

		QCOMPARE(values(pairs, QStringLiteral("ATTRIB"), QStringLiteral("2"))
				 .count(QStringLiteral("LABEL")), 2);
		const QStringList texts = values(pairs, QStringLiteral("ATTRIB"), QStringLiteral("1"));
		QVERIFY(texts.contains(QStringLiteral("K1")));
		QVERIFY(texts.contains(QStringLiteral("K2")));
			//Defined once in the block, and no longer a loose text
		QCOMPARE(values(pairs, QStringLiteral("ATTDEF"), QStringLiteral("2"))
				 .count(QStringLiteral("LABEL")), 1);
		QVERIFY(!values(pairs, QStringLiteral("TEXT"), QStringLiteral("1")).contains(QStringLiteral("K1")));
	}

	void labelsStayTextsWithBlocksOnly()
	{
		const auto pairs = exportPairs({QStringLiteral("--dxf-blocks")});
		QVERIFY2(!pairs.isEmpty(), "--export-dxf --dxf-blocks failed");
			//Only hidden attributes (70 = 1); the labels are texts
		QVERIFY(!values(pairs, QStringLiteral("ATTRIB"), QStringLiteral("70")).contains(QStringLiteral("0")));
		QVERIFY(values(pairs, QStringLiteral("TEXT"), QStringLiteral("1")).contains(QStringLiteral("K1")));
	}

	void dataAsHiddenAttributes()
	{
			//The label is a text, so it is part data too: K1's label and
			//manufacturer, K2's label, all hidden (70 = 1)
		auto pairs = exportPairs({QStringLiteral("--dxf-blocks")});
		QVERIFY2(!pairs.isEmpty(), "--export-dxf --dxf-blocks failed");
		QCOMPARE(values(pairs, QStringLiteral("ATTRIB"), QStringLiteral("2")),
				 (QStringList{QStringLiteral("LABEL"), QStringLiteral("MANUFACTURER"),
							  QStringLiteral("LABEL")}));
		QCOMPARE(values(pairs, QStringLiteral("ATTRIB"), QStringLiteral("70")),
				 (QStringList{QStringLiteral("1"), QStringLiteral("1"), QStringLiteral("1")}));
		QVERIFY(values(pairs, QStringLiteral("ATTRIB"), QStringLiteral("1"))
				.contains(QStringLiteral("Schneider")));

			//With the labels as attributes, the label is not repeated
		pairs = exportPairs({QStringLiteral("--dxf-attributes")});
		QVERIFY2(!pairs.isEmpty(), "--export-dxf --dxf-attributes failed");
		QCOMPARE(values(pairs, QStringLiteral("ATTRIB"), QStringLiteral("2")),
				 (QStringList{QStringLiteral("LABEL"), QStringLiteral("MANUFACTURER"),
							  QStringLiteral("LABEL")}));
		QCOMPARE(values(pairs, QStringLiteral("ATTRIB"), QStringLiteral("70")),
				 (QStringList{QStringLiteral("0"), QStringLiteral("1"), QStringLiteral("0")}));
	}

	void flatWithoutSwitch()
	{
		const auto pairs = exportPairs({});
		QVERIFY2(!pairs.isEmpty(), "--export-dxf failed");
		QVERIFY(values(pairs, QStringLiteral("BLOCK"), QStringLiteral("2")).isEmpty());
		QVERIFY(values(pairs, QStringLiteral("INSERT"), QStringLiteral("2")).isEmpty());
		QCOMPARE(values(pairs, QStringLiteral("CIRCLE"), QStringLiteral("8")).size(), 2);
		QVERIFY(values(pairs, QStringLiteral("TEXT"), QStringLiteral("1")).contains(QStringLiteral("K1")));
	}
};

QTEST_APPLESS_MAIN(tst_dxfblocks)
#include "tst_dxfblocks.moc"
