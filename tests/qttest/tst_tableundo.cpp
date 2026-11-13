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
#include "dataBase/ui/summaryquerywidget.h"
#include "diagram.h"
#include "factory/qetgraphicstablefactory.h"
#include "factory/ui/addtabledialog.h"
#include "qetapptest.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "qetproject.h"

/**
	Adding a list table (summary, nomenclature, cable list) is one undo
	step: before, the table went on the sheet without an undo command, so
	Ctrl+Z undid the step before it instead.
*/
class tst_tableundo : public QObject
{
	Q_OBJECT

	static int tables(Diagram *diagram)
	{
		int count = 0;
		for (QGraphicsItem *item : diagram->items())
			if (qgraphicsitem_cast<QetGraphicsTableItem *>(item)) ++count;
		return count;
	}

private slots:
	void addingATableIsOneUndoStep()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		AddTableDialog dialog(new SummaryQueryWidget());
		dialog.setAdjustTableToFolio(false);
		dialog.setAddNewTableToNewDiagram(false);
		const int steps = diagram->undoStack().count();

		QetGraphicsTableFactory::create(diagram, &dialog);
		QCOMPARE(tables(diagram), 1);
		QCOMPARE(diagram->undoStack().count(), steps + 1);

		diagram->undoStack().undo();
		QCOMPARE(tables(diagram), 0);
		diagram->undoStack().redo();
		QCOMPARE(tables(diagram), 1);
	}
};

QET_APP_TEST_MAIN(tst_tableundo)
#include "tst_tableundo.moc"
