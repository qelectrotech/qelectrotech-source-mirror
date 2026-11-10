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
#include "autoNum/assignvariables.h"
#include "autoNum/numerotationcontext.h"
#include "conductorautonumerotation.h"
#include "conductorproperties.h"
#include "diagram.h"
#include "qetapptest.h"
#include "qetproject.h"
#include "qetresult.h"
#include "titleblockproperties.h"

#include <QTemporaryDir>
#include <QtTest>

/**
	"number format 1 - Plant" (unitplant): a conductor numbering part that
	counts each plant (=) on its own, as projects numbered per installation
	do. A new wire on a =SILO folio continues SILO's count, one on =FACTORY
	FACTORY's; other parts are numbered as before. The counts are kept with
	the project.
*/
class tst_plantcounter : public QObject
{
	Q_OBJECT
	QTemporaryDir m_dir;

	static Diagram *folio(QETProject &project, const QString &plant)
	{
		Diagram *diagram = project.addNewDiagram();
		TitleBlockProperties tb = diagram->border_and_titleblock.exportTitleBlock();
		tb.plant = plant;
		diagram->border_and_titleblock.importTitleBlock(tb);
		diagram->setConductorsAutonumName(QStringLiteral("wires"));
		return diagram;
	}

		//"W" then a plant counter starting at 1, the numbering in use
	static void plantNumbering(QETProject &project, bool with_unit = false)
	{
		NumerotationContext rule;
		rule.addValue(QStringLiteral("string"), QStringLiteral("W"));
		rule.addValue(QStringLiteral("unitplant"), 1, 1, 1);
		if (with_unit) {
			rule.addValue(QStringLiteral("string"), QStringLiteral("."));
			rule.addValue(QStringLiteral("unit"), 1, 1, 1);
		}
		project.addConductorAutoNum(QStringLiteral("wires"), rule);
		project.setCurrentConductorAutoNum(QStringLiteral("wires"));
	}

		//The text a new wire drawn on @p diagram gets
	static QString newWire(Diagram *diagram)
	{
		ConductorProperties cp;
		autonum::sequentialNumbers seq;
		ConductorAutoNumerotation::newProperties(diagram, cp, seq);
		return autonum::AssignVariables::formulaToLabel(cp.m_formula, seq, diagram);
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
	}

	void formulaIsAUnit()
	{
		NumerotationContext rule;
		rule.addValue(QStringLiteral("string"), QStringLiteral("W"));
		rule.addValue(QStringLiteral("unitplant"), 1, 1, 1);
		rule.addValue(QStringLiteral("unit"), 1, 1, 1);
		QCOMPARE(autonum::numerotationContextToFormula(rule),
				 QStringLiteral("W%sequ_1%sequ_2"));
	}

	void eachPlantCountsOnItsOwn()
	{
		QETProject project;
		plantNumbering(project);
		Diagram *factory = folio(project, QStringLiteral("FACTORY"));
		Diagram *silo = folio(project, QStringLiteral("SILO"));
		Diagram *factory2 = folio(project, QStringLiteral("FACTORY"));

		QCOMPARE(newWire(factory), QStringLiteral("W1"));
		QCOMPARE(newWire(factory), QStringLiteral("W2"));
		QCOMPARE(newWire(silo), QStringLiteral("W1"));
			//Another folio of the same plant goes on with its count
		QCOMPARE(newWire(factory2), QStringLiteral("W3"));
		QCOMPARE(newWire(silo), QStringLiteral("W2"));
		QCOMPARE(project.conductorPlantCounter(QStringLiteral("wires"), QStringLiteral("FACTORY")),
				 QStringList({QStringLiteral("3")}));
	}

	void otherPartsCountAsBefore()
	{
			//A unit part still counts over the whole project
		QETProject project;
		plantNumbering(project, true);
		Diagram *factory = folio(project, QStringLiteral("FACTORY"));
		Diagram *silo = folio(project, QStringLiteral("SILO"));
		QCOMPARE(newWire(factory), QStringLiteral("W1.1"));
		QCOMPARE(newWire(silo), QStringLiteral("W1.2"));
		QCOMPARE(newWire(factory), QStringLiteral("W2.3"));
	}

	void startAndStepComeFromTheRule()
	{
		QETProject project;
		NumerotationContext rule;
		rule.addValue(QStringLiteral("unitplant"), 100, 10, 100);
		project.addConductorAutoNum(QStringLiteral("wires"), rule);
		project.setCurrentConductorAutoNum(QStringLiteral("wires"));
		Diagram *silo = folio(project, QStringLiteral("SILO"));
		QCOMPARE(newWire(silo), QStringLiteral("100"));
		QCOMPARE(newWire(silo), QStringLiteral("110"));
	}

	void projectKeepsTheCounts_data()
	{
		QTest::addColumn<QString>("file");
		QTest::newRow("qet") << QStringLiteral("plants.qet");
		QTest::newRow("qetdb") << QStringLiteral("plants.qetdb");
	}

	void projectKeepsTheCounts()
	{
		QFETCH(QString, file);
		const QString path = m_dir.filePath(file);
		{
			QETProject project;
			plantNumbering(project);
			Diagram *silo = folio(project, QStringLiteral("SILO"));
			newWire(silo);
			newWire(silo);
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QETProject again(path);
		QCOMPARE(again.conductorPlantCounter(QStringLiteral("wires"), QStringLiteral("SILO")),
				 QStringList({QStringLiteral("2")}));
		QVERIFY(again.conductorPlantCounter(QStringLiteral("wires"), QStringLiteral("FACTORY")).isEmpty());
	}

	void projectWithoutPlantCountersSavesAsBefore()
	{
		QETProject project;
		folio(project, QStringLiteral("SILO"));
		const QString path = m_dir.filePath(QStringLiteral("none.qet"));
		project.setFilePath(path);
		QVERIFY(project.write().isOk());
		QFile f(path);
		QVERIFY(f.open(QIODevice::ReadOnly));
		QVERIFY(!f.readAll().contains("conductor_plant_counters"));
	}
};

QET_APP_TEST_MAIN(tst_plantcounter)
#include "tst_plantcounter.moc"
