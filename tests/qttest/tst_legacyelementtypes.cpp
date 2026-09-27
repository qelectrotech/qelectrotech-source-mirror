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
#include "dataBase/legacyelementtypes.h"

#include <QTest>

class TestLegacyElementTypes : public QObject
{
	Q_OBJECT

	private slots:
	void upgradesOldNames_data()
	{
		QTest::addColumn<QString>("saved");
		QTest::addColumn<QString>("expected");

		// The WHERE clause industrial.qet's parts list was saved with.
		QTest::newRow("simple, with sub types")
				<< "WHERE ( element_type = 'Simple' OR element_sub_type = 'coil') ORDER BY label"
				<< "WHERE ( element_type = 'simple' OR element_sub_type = 'coil') ORDER BY label";
		QTest::newRow("terminal and master, lengths change")
				<< "element_type = 'Terminale' OR element_type = 'Master' OR element_type = 'Simple'"
				<< "element_type = 'terminal' OR element_type = 'master' OR element_type = 'simple'";
		QTest::newRow("reports and thumbnail")
				<< "element_type='NextReport' OR element_type =  'PreviousReport' OR element_type = 'Thumbnail'"
				<< "element_type='next_report' OR element_type =  'previous_report' OR element_type = 'thumbnail'";
		QTest::newRow("current names unchanged")
				<< "element_type = 'simple' OR element_type = 'terminal'"
				<< "element_type = 'simple' OR element_type = 'terminal'";
		QTest::newRow("the word elsewhere unchanged")
				<< "WHERE label = 'Simple' OR designation = 'Master'"
				<< "WHERE label = 'Simple' OR designation = 'Master'";
		QTest::newRow("empty") << "" << "";
	}

	void upgradesOldNames()
	{
		QFETCH(QString, saved);
		QFETCH(QString, expected);
		QCOMPARE(LegacyElementTypes::upgradeQuery(saved), expected);
	}
};

QTEST_APPLESS_MAIN(TestLegacyElementTypes)
#include "tst_legacyelementtypes.moc"
