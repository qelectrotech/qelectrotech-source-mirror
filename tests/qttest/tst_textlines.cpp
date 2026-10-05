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
};

QTEST_MAIN(tst_textlines)
#include "tst_textlines.moc"
