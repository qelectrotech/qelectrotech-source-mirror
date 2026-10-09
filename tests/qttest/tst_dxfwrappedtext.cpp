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
	--export-dxf writes a symbol text wrapped to its width line by line,
	as it is drawn, rather than as one long line. A text without a width
	is still written on one line.
*/
class tst_dxfwrappedtext : public QObject
{
	Q_OBJECT

private slots:
	void wrappedLines()
	{
		const QString project = QFINDTESTDATA("fixtures/dxf_wrapped_text.qet");
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

			//One text entity per word of the text 70 px wide...
		for (const QString &word : {QStringLiteral("WrapAlpha"), QStringLiteral("WrapGamma"), QStringLiteral("WrapEpsilon")})
			QVERIFY2(values.contains(word), qPrintable(word));
			//...one for the whole text without a width
		QVERIFY(values.contains(QStringLiteral("LineAlpha LineBeta LineGamma LineDelta LineEpsilon")));
	}
};

QTEST_APPLESS_MAIN(tst_dxfwrappedtext)
#include "tst_dxfwrappedtext.moc"
