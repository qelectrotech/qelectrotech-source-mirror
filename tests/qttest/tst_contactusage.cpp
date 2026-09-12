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

#include "contactusage.h"

class tst_contactusage : public QObject
{
	Q_OBJECT

private slots:
	// An empty master uses nothing.
	void emptyUsesNothing()
	{
		ContactUsage usage;

		QCOMPARE(usage.no, 0);
		QCOMPARE(usage.nc, 0);
		QCOMPARE(usage.sw, 0);
		QCOMPARE(usage.other, 0);
		QCOMPARE(usage.total(), 0);
	}

	// Each type accumulates into its own field only.
	void countsEachTypeSeparately()
	{
		ContactUsage usage;
		usage.addSlave(ContactUsage::NO, 1);
		usage.addSlave(ContactUsage::NO, 1);
		usage.addSlave(ContactUsage::NC, 1);
		usage.addSlave(ContactUsage::SW, 1);
		usage.addSlave(ContactUsage::Other, 1);

		QCOMPARE(usage.no, 2);
		QCOMPARE(usage.nc, 1);
		QCOMPARE(usage.sw, 1);
		QCOMPARE(usage.other, 1);
		QCOMPARE(usage.total(), 5);
	}

	// A slave standing for several contacts counts once per contact.
	// Counting elements rather than contacts made a 4 pole contact
	// consume a single contact from the master's budget.
	void countsContactsNotElements()
	{
		ContactUsage usage;
		usage.addSlave(ContactUsage::NO, 4);

		QCOMPARE(usage.no, 4);
		QCOMPARE(usage.total(), 4);
	}

	// A changeover is one contact of its own kind, never one NO plus
	// one NC. CrossRefItem::NOElements() and NCElements() both return
	// changeovers, so a count built by adding those two lists would
	// report a single changeover as two contacts.
	void changeoverIsCountedOnce()
	{
		ContactUsage usage;
		usage.addSlave(ContactUsage::SW, 1);

		QCOMPARE(usage.sw, 1);
		QCOMPARE(usage.no, 0);
		QCOMPARE(usage.nc, 0);
		QCOMPARE(usage.total(), 1);
	}

	// An element which declares no contact count, or a nonsensical one,
	// is still a contact.
	void missingContactCountIsOneContact_data()
	{
		QTest::addColumn<int>("declared");

		QTest::newRow("zero")     << 0;
		QTest::newRow("negative") << -1;
	}

	void missingContactCountIsOneContact()
	{
		QFETCH(int, declared);

		ContactUsage usage;
		usage.addSlave(ContactUsage::NO, declared);

		QCOMPARE(usage.no, 1);
		QCOMPARE(usage.total(), 1);
	}

	// A declared capacity is summed across groups, so two NO groups of two
	// contacts each declare four NO contacts, not two groups.
	void capacitySumsAcrossGroups()
	{
		ContactUsage capacity;
		capacity.addSlave(ContactUsage::NO, 2);
		capacity.addSlave(ContactUsage::NO, 2);
		capacity.addSlave(ContactUsage::NC, 1);

		QCOMPARE(capacity.no, 4);
		QCOMPARE(capacity.nc, 1);
		QCOMPARE(capacity.total(), 5);
	}

	// The mix a coil would actually carry: two single NO, one 4 pole NO,
	// one NC and one changeover.
	void tallysARealisticMix()
	{
		ContactUsage usage;
		usage.addSlave(ContactUsage::NO, 1);
		usage.addSlave(ContactUsage::NO, 1);
		usage.addSlave(ContactUsage::NO, 4);
		usage.addSlave(ContactUsage::NC, 1);
		usage.addSlave(ContactUsage::SW, 1);

		QCOMPARE(usage.no, 6);
		QCOMPARE(usage.nc, 1);
		QCOMPARE(usage.sw, 1);
		QCOMPARE(usage.total(), 8);
	}
};

QTEST_APPLESS_MAIN(tst_contactusage)
#include "tst_contactusage.moc"
