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
#include "qetapptest.h"
#include "autoNum/assignvariables.h"

#include <QStandardPaths>

/**
	%T in a folio report or cross reference formula is the title of the
	folio the reference points to, so a reference can read like the ones
	EPLAN prints: "+UV1/1 Versorgung UG/EG.0". %total and the title block
	and project variables (lower case only) keep their own values, and the
	title itself is free text: variables in it stay as typed.
*/
class tst_reportfoliotitle : public QObject
{
	Q_OBJECT

	static QString label(const QString &formula, const autonum::FormulaContext &c)
	{
		autonum::sequentialNumbers seq;
		return autonum::AssignVariables::formulaToLabel(formula, seq, c);
	}

	static autonum::FormulaContext context()
	{
		autonum::FormulaContext c;
		c.folio = QStringLiteral("1");
		c.folio_title = QStringLiteral("Versorgung UG/EG");
		c.folio_index = 2;
		c.folio_total = 23;
		c.locmach = QStringLiteral("UV1");
		return c;
	}

private slots:
	void titleIsReplaced()
	{
		const QString result = label(QStringLiteral("+%LM/%F %T"), context());
		QCOMPARE(result, QStringLiteral("+UV1/1 Versorgung UG/EG"));
	}

	void otherVariablesKeepTheirValue()
	{
		autonum::FormulaContext c = context();
		c.title_block_fields.addValue(QStringLiteral("typ"), QStringLiteral("SH363N"));
		const QString result = label(QStringLiteral("%T / %total / %typ / %{typ}"), c);
		QCOMPARE(result, QStringLiteral("Versorgung UG/EG / 23 / SH363N / SH363N"));
	}

	void titleKeepsTheVariablesItContains()
	{
		autonum::FormulaContext c = context();
		c.folio_title = QStringLiteral("Schema %c %F %total %{typ}");
		c.title_block_fields.addValue(QStringLiteral("typ"), QStringLiteral("SH363N"));
		const QString result = label(QStringLiteral("%F %T"), c);
		QCOMPARE(result, QStringLiteral("1 Schema %c %F %total %{typ}"));
	}

	void emptyTitleLeavesNothing()
	{
		autonum::FormulaContext c = context();
		c.folio_title.clear();
		const QString result = label(QStringLiteral("%F%T"), c);
		QCOMPARE(result, QStringLiteral("1"));
	}
};

int main(int argc, char **argv)
{
	QStandardPaths::setTestModeEnabled(true);
	return QetAppTest::run<tst_reportfoliotitle>(argc, argv);
}

#include "tst_reportfoliotitle.moc"
