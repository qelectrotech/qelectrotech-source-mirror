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
	A static text with anchor="alignment" has its x at the edge or centre
	its Halignment selects, instead of its left edge (#1251). The symbol in
	fixtures/static_text_anchor.qet has the same text five times, all at
	x = 0: left, right and centre anchored, right aligned without anchor,
	and anchored without Halignment. --export-dxf writes where each is drawn.
*/
class tst_statictextanchor : public QObject
{
	Q_OBJECT

private slots:
	void anchoredX()
	{
		const QString project = QFINDTESTDATA("fixtures/static_text_anchor.qet");
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

			//x (group code 10) of each TEXT entity of the symbol, in order
		QList<qreal> xs;
		for (int i = 0 ; i + 1 < values.size() ; i += 2) {
			if (values.at(i).trimmed() != QLatin1String("0") || values.at(i + 1).trimmed() != QLatin1String("TEXT"))
				continue;
			qreal x = 0;
			QString text;
			for (int j = i + 2 ; j + 1 < values.size() && values.at(j).trimmed() != QLatin1String("0") ; j += 2) {
				if (values.at(j).trimmed() == QLatin1String("10"))
					x = values.at(j + 1).trimmed().toDouble();
				else if (values.at(j).trimmed() == QLatin1String("1"))
					text = values.at(j + 1).trimmed();
			}
			if (text == QLatin1String("GPIO21 PCM_DOUT"))
				xs << x;
		}
		QCOMPARE(xs.size(), 5);

		const qreal left = xs.at(0), right = xs.at(1), centre = xs.at(2);
			//anchored on the right edge: drawn one text width to the left
		QVERIFY2(left - right > 20, qPrintable(QStringLiteral("left %1 right %2").arg(left).arg(right)));
			//anchored on the centre: half as far
		QVERIFY(qAbs(2 * (left - centre) - (left - right)) < 0.01);
			//without anchor, x stays the left edge whatever the alignment
		QCOMPARE(xs.at(3), left);
			//anchor without Halignment is the left edge too
		QCOMPARE(xs.at(4), left);
	}
};

QTEST_APPLESS_MAIN(tst_statictextanchor)
#include "tst_statictextanchor.moc"
