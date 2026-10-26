// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "diagram.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"

/**
	Closing a project takes its symbols down with them: their terminals
	say "a wire left" on their way out, and what a symbol shows of its
	wire must not be filled while its texts are being deleted. Found on
	a real drawing which took QElectroTech down with it whenever it was
	closed, however it had been opened.
*/
class tst_symbolteardown : public QObject
{
	Q_OBJECT

private slots:
	//A symbol which shows the values of its wire goes down with its
	//project without taking the application with it
	void closingAProjectTakesItsSymbolsDownWhole()
	{
		const QString fixture = QFINDTESTDATA("fixtures/linked_reports.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		QVERIFY(!project.diagrams().isEmpty());

			//A report symbol with texts of its own and a wire on one of
			//its terminals: the very chain which fills those texts
		Element *report = nullptr;
		for (Diagram *diagram : project.diagrams()) {
			for (Element *element : diagram->elements()) {
				if ((element->linkType() & Element::AllReport)
					&& !element->dynamicTextItems().isEmpty()) {
					report = element;
					break;
				}
			}
			if (report) break;
		}
		if (!report) {
			QSKIP("the fixture has no report symbol with texts");
		}
		QVERIFY(!report->terminals().isEmpty());

		Diagram *diagram = report->diagram();
		Terminal *first = report->terminals().first();
		Terminal *second = nullptr;
		for (Element *element : diagram->elements()) {
			if (element == report) continue;
			for (Terminal *terminal : element->terminals()) {
				if (!terminal->conductors().isEmpty()) {
					second = terminal;
					break;
				}
			}
			if (second) break;
		}
		if (second) {
			auto *conductor = new Conductor(first, second);
			diagram->addItem(conductor);
		}

			//The symbol goes down with its texts still asking about wires
		delete report;
		QVERIFY2(true, "the symbol went down without taking the application with it");
	}

	//And a whole project with wires and texts closes cleanly
	void closingTheWholeProjectGoesQuietly()
	{
		const QString fixture = QFINDTESTDATA("fixtures/linked_reports.qet");
		{
			QETProject project(fixture);
			QCOMPARE(project.state(), QETProject::Ok);
		}
		QVERIFY2(true, "the project closed without taking anything with it");
	}
};

QET_APP_TEST_MAIN(tst_symbolteardown)
#include "tst_symbolteardown.moc"
