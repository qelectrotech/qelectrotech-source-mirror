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
#include <QTextDocument>
#include <QTextOption>

#include "textlines.h"

class tst_textlines : public QObject
{
	Q_OBJECT

private slots:
	// A text without a width gives its own lines.
	void unwrappedTextGivesItsLines()
	{
		QTextDocument document(QStringLiteral("Motor protection\nswitch Q12"));
		QCOMPARE(TextLines::layoutLines(&document),
				 QStringList({QStringLiteral("Motor protection"), QStringLiteral("switch Q12")}));
	}

	// A text without a width gives exactly the lines the DXF export wrote
	// before, toPlainText().split('\n'): trailing spaces and tabs kept,
	// non-breaking spaces as plain spaces, <br> as a line break.
	void unwrappedTextIsUnchanged_data()
	{
		QTest::addColumn<QString>("html");
		QTest::newRow("trailing spaces")      << QStringLiteral("K1  \nswitch\t");
		QTest::newRow("non-breaking spaces")  << QStringLiteral("<p>K1&nbsp;&nbsp;24V&nbsp;DC</p>");
		QTest::newRow("line break")           << QStringLiteral("<p>Wiper to 6<br>contacts</p>");
		QTest::newRow("paragraphs")           << QStringLiteral("<p>relais voyant </p><p> preventa</p>");
		QTest::newRow("empty")                << QString();
	}

	void unwrappedTextIsUnchanged()
	{
		QFETCH(QString, html);
		QTextDocument document;
		if (Qt::mightBeRichText(html))
			document.setHtml(html);
		else
			document.setPlainText(html);

		QCOMPARE(TextLines::layoutLines(&document),
				 document.toPlainText().split(QLatin1Char('\n')));
	}

	// A text wrapped to its width gives one line per drawn line, without
	// the space the line was broken at.
	void wrappedTextGivesTheDrawnLines()
	{
		QTextDocument document(QStringLiteral("Motor protection switch Q12"));
		QTextOption option = document.defaultTextOption();
		option.setWrapMode(QTextOption::WordWrap);
		document.setDefaultTextOption(option);
		document.setTextWidth(1);

		QCOMPARE(TextLines::layoutLines(&document),
				 QStringList({QStringLiteral("Motor"), QStringLiteral("protection"),
							  QStringLiteral("switch"), QStringLiteral("Q12")}));
	}

	// A wrapped paragraph has no trailing spaces left, even on its last
	// line: tabs and spaces that wrap to a line of their own (as in the
	// affuteuse_250h example) give an empty line, which is not written but
	// still keeps the line spacing.
	void wrappedTextHasNoTrailingSpaces()
	{
		QTextDocument document(QStringLiteral("\tMotor\t\t                              "));
		QTextOption option = document.defaultTextOption();
		option.setWrapMode(QTextOption::WordWrap);
		document.setDefaultTextOption(option);
		document.setTextWidth(80);

		const QStringList lines = TextLines::layoutLines(&document);
		QVERIFY(lines.size() > 1);
		QVERIFY(lines.contains(QStringLiteral("Motor")));
		for (const QString &line : lines)
			QVERIFY2(line.isEmpty() || !line.back().isSpace(), qPrintable(line));
	}

	// Non-breaking spaces also become plain spaces in a wrapped text.
	void wrappedTextHasPlainSpaces()
	{
		QTextDocument document;
		document.setHtml(QStringLiteral("<p>K1&nbsp;24V and more words</p>"));
		QTextOption option = document.defaultTextOption();
		option.setWrapMode(QTextOption::WordWrap);
		document.setDefaultTextOption(option);
		document.setTextWidth(1);

		const QStringList lines = TextLines::layoutLines(&document);
		QCOMPARE(lines.first(), QStringLiteral("K1 24V"));
		for (const QString &line : lines)
			QVERIFY(!line.contains(QChar::Nbsp));
	}
};

QTEST_MAIN(tst_textlines)
#include "tst_textlines.moc"
