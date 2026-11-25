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
// SPDX-License-Identifier: GPL-2.0-or-later
#include "cable/cable.h"
#include "cable/cablecreatedialog.h"
#include "cablelist/cabletypelist.h"
#include "diagram.h"
#include "qetapptest.h"
#include "qetproject.h"

#include <QAbstractItemModel>
#include <QFile>
#include <QTableView>
#include <QTemporaryDir>
#include <QUndoStack>
#include <QtTest>

/**
	Picking a cable from the cable type file (#2002): the manufacturer and
	the part number its line gives come along with the type.

	 - a new cable takes both from the line it is made from;
	 - changing the type of a cable takes them too, as part of the same
	   undo step;
	 - a line whose cells are empty leaves what the cable already had --
	   a part number typed in by hand is not wiped by a file which says
	   nothing about it, nor by taking the same type again;
	 - the two go as a pair: a line with a part number and no maker
	   does not leave the cable with the maker of its former type.
*/
class tst_cabletypepick : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

		///Make the row naming @p type the current one of the window's list
	static bool pick(CableCreateDialog &dialog, const QString &type)
	{
		const auto tables = dialog.findChildren<QTableView *>();
		for (QTableView *table : tables) {
			QAbstractItemModel *model = table->model();
			if (!model || model->rowCount() == 0) continue;
			const QModelIndexList found = model->match(model->index(0, 0), Qt::DisplayRole,
													   type, 1, Qt::MatchExactly);
			if (found.isEmpty()) continue;
			table->setCurrentIndex(found.first());
			return true;
		}
		return false;
	}

  private slots:
	void initTestCase()
	{
		QETProject::setBackupEnabled(false);
		QVERIFY(m_dir.isValid());
		const QString path = m_dir.filePath(QStringLiteral("cables.csv"));
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
		file.write("Designation;Number of cores;Core colors;Manufacturer;Part number\n"
				   "designation;cores;core_colors;manufacturer;manufacturer_reference\n"
				   "H07RN-F 3G1,5;3;;Lapp;LAPP-0001\n"
				   "NYM-J 3G2,5;3;;;\n"
				   "OLFLEX 4G1;4;;;OLF-0004\n");
		file.close();
		CableTypeList::setConfiguredPath(path);
	}

	void newCableTakesThePartNumber()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		CablePartData part;
		part.uuid = QUuid::createUuid();
		CableCreateDialog dialog(folio, QList<CableCrossing>(), part);
		QVERIFY(pick(dialog, QStringLiteral("H07RN-F 3G1,5")));
		dialog.accept();

		Cable *cable = dialog.cable();
		QVERIFY(cable);
		QCOMPARE(cable->type(), QStringLiteral("H07RN-F 3G1,5"));
		QCOMPARE(cable->manufacturer(), QStringLiteral("Lapp"));
		QCOMPARE(cable->manufacturerReference(), QStringLiteral("LAPP-0001"));
	}

	void changingTheTypeTakesThePartNumber()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Cable *cable = project.newCable();
		cable->setDesignation(QStringLiteral("W1"));
		cable->setType(QStringLiteral("NYM-J 3G2,5"));
		cable->setCoreCount(3);
		cable->setManufacturerReference(QStringLiteral("TYPED-BY-HAND"));

		{
			CableCreateDialog dialog(folio, cable);
			QVERIFY(pick(dialog, QStringLiteral("H07RN-F 3G1,5")));
			dialog.accept();
		}
		QCOMPARE(cable->type(), QStringLiteral("H07RN-F 3G1,5"));
		QCOMPARE(cable->manufacturer(), QStringLiteral("Lapp"));
		QCOMPARE(cable->manufacturerReference(), QStringLiteral("LAPP-0001"));

			//One undo puts the type and the part number back together
		folio->undoStack().undo();
		QCOMPARE(cable->type(), QStringLiteral("NYM-J 3G2,5"));
		QCOMPARE(cable->manufacturer(), QString());
		QCOMPARE(cable->manufacturerReference(), QStringLiteral("TYPED-BY-HAND"));
	}

	void anEmptyCellKeepsWhatWasTyped()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Cable *cable = project.newCable();
		cable->setDesignation(QStringLiteral("W1"));
		cable->setType(QStringLiteral("H07RN-F 3G1,5"));
		cable->setCoreCount(3);
		cable->setManufacturer(QStringLiteral("Maker B"));
		cable->setManufacturerReference(QStringLiteral("TYPED-BY-HAND"));

		CableCreateDialog dialog(folio, cable);
		QVERIFY(pick(dialog, QStringLiteral("NYM-J 3G2,5")));
		dialog.accept();
		QCOMPARE(cable->type(), QStringLiteral("NYM-J 3G2,5"));
		QCOMPARE(cable->manufacturer(), QStringLiteral("Maker B"));
		QCOMPARE(cable->manufacturerReference(), QStringLiteral("TYPED-BY-HAND"));
	}

	void theSameTypeAgainKeepsWhatWasTyped()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Cable *cable = project.newCable();
		cable->setDesignation(QStringLiteral("W1"));
		cable->setType(QStringLiteral("H07RN-F 3G1,5"));
		cable->setCoreCount(3);
		cable->setManufacturerReference(QStringLiteral("TYPED-BY-HAND"));

		CableCreateDialog dialog(folio, cable);
		QVERIFY(pick(dialog, QStringLiteral("H07RN-F 3G1,5")));
		dialog.accept();
		QCOMPARE(cable->manufacturer(), QString());
		QCOMPARE(cable->manufacturerReference(), QStringLiteral("TYPED-BY-HAND"));
	}

	void makerAndNumberGoAsAPair()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Cable *cable = project.newCable();
		cable->setDesignation(QStringLiteral("W1"));
		cable->setType(QStringLiteral("H07RN-F 3G1,5"));
		cable->setCoreCount(3);
		cable->setManufacturer(QStringLiteral("Lapp"));
		cable->setManufacturerReference(QStringLiteral("LAPP-0001"));

		CableCreateDialog dialog(folio, cable);
		QVERIFY(pick(dialog, QStringLiteral("OLFLEX 4G1")));
		dialog.accept();
		QCOMPARE(cable->type(), QStringLiteral("OLFLEX 4G1"));
		QCOMPARE(cable->manufacturer(), QString());
		QCOMPARE(cable->manufacturerReference(), QStringLiteral("OLF-0004"));
	}
};

QET_APP_TEST_MAIN(tst_cabletypepick)
#include "tst_cabletypepick.moc"
