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
#include <QRegularExpression>
#include <QTemporaryDir>
#include <QTextDocument>

/// Run the qelectrotech binary with arguments, without a display.
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

/// The number of lines the words starting with prefix are drawn on.
static int drawnLines(const QString &svg, const QString &prefix)
{
	const QRegularExpression text_re(
				QStringLiteral("<text[^>]*\\by=\"([^\"]+)\"[^>]*>\\s*%1").arg(prefix));
	QSet<QString> lines;
	auto it = text_re.globalMatch(svg);
	while (it.hasNext())
		lines.insert(it.next().captured(1));
	return lines.size();
}

/**
	A free text with a width (text_width on its <input>) keeps it when the
	project is saved again, and wraps to it. A text without a width is saved
	without the attribute, as before, so older versions read it unchanged.
*/
class tst_freetextwidth : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase()
	{
		QVERIFY2(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)), "qelectrotech binary not found");
		m_fixture = QFINDTESTDATA("fixtures/free_text_width.qet");
		QVERIFY2(!m_fixture.isEmpty(), "fixture project not found");
		QVERIFY(m_dir.isValid());
	}

	void widthIsSavedOnlyWhenSet()
	{
		const QString out = m_dir.filePath(QStringLiteral("resaved.qet"));
		QVERIFY2(runQet({QStringLiteral("--resave"), m_fixture, out}), "--resave failed");

		QFile file(out);
		QVERIFY(file.open(QIODevice::ReadOnly));
		QDomDocument document;
		QVERIFY(document.setContent(&file));

		QMap<QString, QString> widths;	// first word -> text_width
		const QDomNodeList inputs = document.elementsByTagName(QStringLiteral("input"));
		for (int i = 0 ; i < inputs.size() ; ++i) {
			const QDomElement input = inputs.at(i).toElement();
			QTextDocument text;
			text.setHtml(input.attribute(QStringLiteral("text")));
			const QString first_word = text.toPlainText().section(QLatin1Char(' '), 0, 0);
			widths.insert(first_word, input.hasAttribute(QStringLiteral("text_width"))
						  ? input.attribute(QStringLiteral("text_width"))
						  : QStringLiteral("none"));
		}

		QCOMPARE(widths.value(QStringLiteral("FreeAlpha")), QStringLiteral("70"));
		QCOMPARE(widths.value(QStringLiteral("OpenAlpha")), QStringLiteral("none"));
			//Centred lines: the width of the user, not the one setHtml() gives
		QCOMPARE(widths.value(QStringLiteral("CentAlpha")), QStringLiteral("120"));
	}

	void textWrapsToItsWidth()
	{
		QVERIFY2(runQet({QStringLiteral("--export-svg"), m_fixture, m_dir.path()}), "--export-svg failed");
		QFile file(m_dir.filePath(QStringLiteral("01_diagram.svg")));
		QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
		const QString svg = QString::fromUtf8(file.readAll());

		QVERIFY(drawnLines(svg, QStringLiteral("Free")) > 1);
		QCOMPARE(drawnLines(svg, QStringLiteral("Open")), 1);
	}

private:
	QString m_fixture;
	QTemporaryDir m_dir;
};

QTEST_MAIN(tst_freetextwidth)
#include "tst_freetextwidth.moc"
