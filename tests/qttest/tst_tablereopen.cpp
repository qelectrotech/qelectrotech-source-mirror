// SPDX-License-Identifier: GPL-2.0-or-later
#include "cablelist/cablequerywidget.h"
#include "diagram.h"
#include "factory/qetgraphicstablefactory.h"
#include "factory/ui/addtabledialog.h"
#include "qetapptest.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "qetproject.h"
#include "qetresult.h"

#include <QTemporaryDir>
#include <QtTest>

/**
	A list table adjusted to its sheet reopens at the width it was saved
	with. Before, the minimum width of its columns was taken before the
	header's margins were read back, so a reopened cable list grew past the
	sheet's border and the export warned that it lay outside the sheet.
*/
class tst_tablereopen : public QObject
{
	Q_OBJECT

	static QetGraphicsTableItem *table(Diagram *diagram)
	{
		for (QGraphicsItem *item : diagram->items())
			if (auto *t = qgraphicsitem_cast<QetGraphicsTableItem *>(item))
				return t;
		return nullptr;
	}

private slots:
	void cableListKeepsItsWidth()
	{
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const QString path = dir.filePath(QStringLiteral("table.qet"));
		int width = 0;
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			auto *query = new CableQueryWidget();
				//The columns the walkthrough chose
			query->setFields({QStringLiteral("bmk_cable"), QStringLiteral("type"),
							  QStringLiteral("cores_used"), QStringLiteral("location_start"),
							  QStringLiteral("bmk_start"), QStringLiteral("location_end"),
							  QStringLiteral("bmk_end"), QStringLiteral("folio_start")});
			AddTableDialog dialog(query);
			dialog.setAdjustTableToFolio(true);
			dialog.setAddNewTableToNewDiagram(false);
			QetGraphicsTableFactory::create(diagram, &dialog);
			QetGraphicsTableItem *made = table(diagram);
			QVERIFY(made);
			width = made->size().width();
			QVERIFY(width <= diagram->border_and_titleblock.insideBorderRect().width());
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}

		QETProject project(path);
		project.loadFolios();
		QCOMPARE(project.diagrams().size(), 1);
		QetGraphicsTableItem *reopened = table(project.diagrams().first());
		QVERIFY(reopened);
		QCOMPARE(reopened->size().width(), width);
	}
};

QET_APP_TEST_MAIN(tst_tablereopen)
#include "tst_tablereopen.moc"
