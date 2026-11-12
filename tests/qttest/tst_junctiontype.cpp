/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "properties/elementdata.h"

#include <QTest>

/**
	The junction is the connection symbol on a wire: a corner, a T-junction,
	a crossing. It is drawn in the cabinet nowhere, so it must never be given
	a label by a numbering scheme (#1500), and its name must survive the round
_trip through the element definition, which is where the type lives.
*/
class TestJunctionType : public QObject
{
	Q_OBJECT

	private slots:
	void readsAndWritesItsName()
	{
		QCOMPARE(ElementData::typeToString(ElementData::Junction),
				 QStringLiteral("junction"));
		QCOMPARE(ElementData::typeFromString(QStringLiteral("junction")),
				 ElementData::Junction);
	}

	void takesNoSchemeNumber()
	{
		//The point of the type: nothing numbers a junction
		QVERIFY(!ElementData::takesSchemeNumber(ElementData::Junction));
	}

	void leavesTheNumberedKindsAlone()
	{
		//What was numbered before must still be numbered, so that this
		//type does not quietly widen into a change of the other kinds
		QVERIFY(ElementData::takesSchemeNumber(ElementData::Simple));
		QVERIFY(ElementData::takesSchemeNumber(ElementData::Terminal));
		QVERIFY(ElementData::takesSchemeNumber(ElementData::Master));

		//And what was not numbered still is not
		QVERIFY(!ElementData::takesSchemeNumber(ElementData::Slave));
		QVERIFY(!ElementData::takesSchemeNumber(ElementData::NextReport));
		QVERIFY(!ElementData::takesSchemeNumber(ElementData::PreviousReport));
		QVERIFY(!ElementData::takesSchemeNumber(ElementData::ConductorDefinition));
	}

	void isNotAnyOtherKind()
	{
		//The type is a bit of its own: it must not read as a terminal (which
		//would put it back into the terminal strip) nor as any report (which
		//would make it link itself to a partner)
		QVERIFY(!(ElementData::Junction & ElementData::Terminal));
		QVERIFY(!(ElementData::Junction & ElementData::AllReport));
		QVERIFY(!(ElementData::Junction & ElementData::ConductorDefinition));
		QVERIFY(ElementData::Junction & ElementData::Junction);
	}
};

QTEST_GUILESS_MAIN(TestJunctionType)
#include "tst_junctiontype.moc"
