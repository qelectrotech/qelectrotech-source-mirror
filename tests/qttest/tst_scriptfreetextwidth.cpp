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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>

/**
	The project database keeps the width of a free text (text_width in the
	independent_text table, NULL for the automatic width), read here with
	qet.query() from a script run on fixtures/free_text_width.qet.
*/
class tst_scriptfreetextwidth : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	/// The rows of the independent_text table, by the first word of the text.
	QHash<QString, QJsonObject> rows()
	{
		const QString path = m_dir.filePath(QStringLiteral("probe.js"));
		const QString home = m_dir.filePath(QStringLiteral("home"));
		QDir().mkpath(home);
		QFile f(path);
		if (!f.open(QIODevice::WriteOnly)) return {};
		f.write("qet.log('PROBE ' + JSON.stringify(qet.query("
				"'SELECT text, text_width, width, height FROM independent_text')));\n");
		f.close();

		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("QET_ENABLE_SCRIPTING"), QStringLiteral("1"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/data"));
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
				   {QStringLiteral("--run"), path, QFINDTESTDATA("fixtures/free_text_width.qet")});
		if (!proc.waitForFinished(60000)) return {};

		const QString out = QString::fromUtf8(proc.readAllStandardOutput() + proc.readAllStandardError());
		const QString mark = QStringLiteral("PROBE ");
		QHash<QString, QJsonObject> result;
		for (const QString &line : out.split(QLatin1Char('\n'))) {
			const int i = line.indexOf(mark);
			if (i < 0) continue;
			const QJsonArray array = QJsonDocument::fromJson(line.mid(i + mark.size()).toUtf8()).array();
			for (const QJsonValue &row : array)
				result.insert(row.toObject().value(QStringLiteral("text")).toString().section(QLatin1Char(' '), 0, 0),
							  row.toObject());
		}
		return result;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
		QVERIFY(!QFINDTESTDATA("fixtures/free_text_width.qet").isEmpty());
	}

	void widthIsInTheDatabase()
	{
		const QHash<QString, QJsonObject> r = rows();
		QCOMPARE(r.size(), 3);

		QCOMPARE(r.value(QStringLiteral("FreeAlpha")).value(QStringLiteral("text_width")).toDouble(), 70.0);
		QCOMPARE(r.value(QStringLiteral("CentAlpha")).value(QStringLiteral("text_width")).toDouble(), 120.0);
			//The automatic width is NULL
		const QJsonValue open = r.value(QStringLiteral("OpenAlpha")).value(QStringLiteral("text_width"));
		QVERIFY2(open.isNull() || open.toString().isEmpty(), qPrintable(open.toVariant().toString()));

			//The box of the row is the wrapped one: taller than one line
		QVERIFY(r.value(QStringLiteral("FreeAlpha")).value(QStringLiteral("height")).toDouble()
				> 2 * r.value(QStringLiteral("OpenAlpha")).value(QStringLiteral("height")).toDouble());
	}
};

QTEST_APPLESS_MAIN(tst_scriptfreetextwidth)
#include "tst_scriptfreetextwidth.moc"
