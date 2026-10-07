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
	A label formula with ten or more sequential numbers: %sequ_10 must get
	the tenth value, not the first value followed by a "0". The values are
	put in by AssignVariables::assignSequence(), which the bill of
	materials uses for every label, so the real binary's --export-bom on
	fixtures/ten_sequential_numbers.qet shows the result: its element has
	the formula %sequ_10-%sequ_1 and the unit values A to J.
*/
class tst_tensequentialnumbers : public QObject
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

private slots:
	void initTestCase()
	{
		QVERIFY2(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)), "qelectrotech binary not found");
		QVERIFY(m_dir.isValid());
	}

	void tenthValueIsUsed()
	{
		const QString fixture = QFINDTESTDATA("fixtures/ten_sequential_numbers.qet");
		QVERIFY2(!fixture.isEmpty(), "fixture project not found");
		const QString out = m_dir.filePath(QStringLiteral("bom.csv"));
		QVERIFY2(runQet({QStringLiteral("--export-bom"), fixture, out}), "--export-bom failed");

		QFile file(out);
		QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
		const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
		QStringList labels;
		for (const QString &line : lines) {
			if (line.startsWith(QLatin1Char('"')))
				labels << line.section(QLatin1Char(';'), 0, 0);
		}
		QVERIFY2(labels.contains(QStringLiteral("\"J-A\"")),
			 qPrintable(QStringLiteral("labels: %1").arg(labels.join(QLatin1Char(' ')))));
		QVERIFY2(!labels.contains(QStringLiteral("\"A0-A\"")), "the first value with a 0 appended");
	}
};

QTEST_MAIN(tst_tensequentialnumbers)
#include "tst_tensequentialnumbers.moc"
