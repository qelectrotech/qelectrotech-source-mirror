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
	--export-dxf on a folio whose title block is at the right (issue #1339):
	a symbol's circle is one CIRCLE and its arc one ARC with the symbol's own
	angles, which needs the same scale on both axes; the title block is
	turned a quarter, as it is on screen.
*/
class tst_dxfcurves : public QObject
{
	Q_OBJECT

		/// The entities on @a layer, each as its group codes and values.
	static QList<QMultiHash<int, QString>> entities(const QStringList &values,
													const QString &type,
													const QString &layer)
	{
		QList<QMultiHash<int, QString>> found;
		for (int i = 0 ; i + 1 < values.size() ; i += 2)
		{
			if (values.at(i).trimmed() != QLatin1String("0")
				|| values.at(i + 1).trimmed() != type)
				continue;
			QMultiHash<int, QString> entity;
			for (int j = i + 2 ; j + 1 < values.size()
				 && values.at(j).trimmed() != QLatin1String("0") ; j += 2)
				entity.insert(values.at(j).trimmed().toInt(), values.at(j + 1).trimmed());
			if (entity.value(8) == layer)
				found << entity;
		}
		return found;
	}

private slots:
	void curvesAndTitleBlock()
	{
		const QString project = QFINDTESTDATA("fixtures/dxf_curves.qet");
		QVERIFY2(!project.isEmpty(), "fixture project not found");
		QTemporaryDir dir;
		QVERIFY(dir.isValid());

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH), {QStringLiteral("--export-dxf"), project, dir.path()});
		QVERIFY2(proc.waitForFinished(60000), "--export-dxf timed out");
		QCOMPARE(proc.exitCode(), 0);

		QFile file(dir.filePath(QStringLiteral("01_diagram.dxf")));
		QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
		const QStringList values = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
		const QString symbols = QStringLiteral("QET_SYMBOLS");

			//The 20 px circle: one CIRCLE, nothing approximated
		const auto circles = entities(values, QStringLiteral("CIRCLE"), symbols);
		QCOMPARE(circles.size(), 1);
		QVERIFY(entities(values, QStringLiteral("POLYLINE"), symbols).isEmpty());

			//The arc from 30 to 150 degrees, same radius as the circle
		const auto arcs = entities(values, QStringLiteral("ARC"), symbols);
		QCOMPARE(arcs.size(), 1);
		QCOMPARE(arcs.first().value(50).toDouble(), 30.0);
		QCOMPARE(arcs.first().value(51).toDouble(), 150.0);
		QCOMPARE(arcs.first().value(40).toDouble(), circles.first().value(40).toDouble());

			//Every title block text turned a quarter, as drawn on screen
		const auto texts = entities(values, QStringLiteral("TEXT"), QStringLiteral("QET_TITLEBLOCK"));
		QVERIFY(!texts.isEmpty());
		for (const auto &text : texts)
			QCOMPARE(text.value(50).toDouble(), 90.0);
	}
};

QTEST_APPLESS_MAIN(tst_dxfcurves)
#include "tst_dxfcurves.moc"
