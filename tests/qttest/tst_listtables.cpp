// SPDX-License-Identifier: GPL-2.0-or-later
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "factory/qetgraphicstablefactory.h"
#include "factory/ui/addtabledialog.h"
#include "factory/ui/listquerywidget.h"
#include "qetapptest.h"
#include "qetgraphicsitem/ViewItem/projectdbmodel.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetproject.h"
#include "qetresult.h"

#include <QDomDocument>
#include <QTemporaryDir>
#include <QtTest>

/**
	The lists the project database answers, as tables on a folio
	(#1878 B9-B11): the device list, the wiring table, the PLC list and
	the signal list. Each is a database table (identifier + query), so it
	follows the project when a label changes, goes on to further linked
	tables when it is too long for its folio, and is read as an empty table
	-- nothing more -- by a QET that does not know its view.
*/
class tst_listtables : public QObject
{
	Q_OBJECT

	int m_position = 0;

	Element *symbol(Diagram *diagram, const QString &label)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		const QString kind = QStringLiteral("device");
		if (!collection->exist(QStringLiteral("import/") + kind + QStringLiteral(".elmt"))) {
			QDomDocument definition;
			if (!definition.setContent(QString(
					R"(<definition type="element" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test symbol</name></names>
<kindInformations><kindInformation name="type">simple</kindInformation></kindInformations>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%2"/><terminal x="0" y="20" orientation="s" uuid="%3"/>
</description></definition>)")
					.arg(QUuid::createUuid().toString(), QUuid::createUuid().toString(),
						 QUuid::createUuid().toString())))
				return nullptr;
			if (!collection->addElementDefinition(QStringLiteral("import"), kind,
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/") + kind + QStringLiteral(".elmt"),
							 diagram->project()), nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		const int position = m_position++;
		element->setPos(100 + 80 * (position % 8), 150 + 100 * (position / 8));
		setLabel(element, label);
		return element;
	}

	static void setLabel(Element *element, const QString &label)
	{
		auto info = element->elementInformations();
		info.addValue(QStringLiteral("label"), label);
		info.remove(QStringLiteral("formula"));
		element->setElementInformations(info);
	}

	static QetGraphicsTableItem *table(Diagram *diagram)
	{
		for (QGraphicsItem *item : diagram->items())
			if (auto *t = qgraphicsitem_cast<QetGraphicsTableItem *>(item))
				return t;
		return nullptr;
	}

		/// The table of @p kind on @p diagram, made as the menu makes it
	static QetGraphicsTableItem *addList(Diagram *diagram, ListQueryWidget::Kind kind,
										 bool newFolios = false)
	{
		AddTableDialog dialog(new ListQueryWidget(kind));
		dialog.setTableName(QStringLiteral("list"));
		dialog.setAdjustTableToFolio(true);
		dialog.setAddNewTableToNewDiagram(newFolios);
		QetGraphicsTableFactory::create(diagram, &dialog);
		return table(diagram);
	}

	static QStringList column(const QAbstractItemModel *model, int col)
	{
		QStringList list;
		for (int r = 0 ; r < model->rowCount() ; ++r)
			list << model->index(r, col).data().toString();
		return list;
	}

private slots:
	void initTestCase() { QETProject::setBackupEnabled(false); }
	void init() { m_position = 0; }

		//B9: the table is the device list, and it follows a label
	void deviceTableFollowsALabel()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		Element *a = symbol(folio, QStringLiteral("K1"));
		Element *b = symbol(folio, QStringLiteral("K2"));
		QVERIFY(a && b);
		project.dataBase()->updateDB();

		QetGraphicsTableItem *t = addList(folio, ListQueryWidget::Device);
		QVERIFY(t);
		auto *model = qobject_cast<ProjectDBModel *>(t->model());
		QVERIFY(model);
		QCOMPARE(model->identifier(), QStringLiteral("device_list"));
		QCOMPARE(model->headerData(0, Qt::Horizontal).toString(), QStringLiteral("Device"));
		QCOMPARE(column(model, 0), QStringList({QStringLiteral("-K1"), QStringLiteral("-K2")}));

		setLabel(a, QStringLiteral("Q7"));
		project.dataBase()->updateDB();
		QCOMPARE(column(model, 0), QStringList({QStringLiteral("-K2"), QStringLiteral("-Q7")}));
	}

		//The table is saved as its query and read back as the same table
	void deviceTableSurvivesSaveAndOpen()
	{
		QTemporaryDir dir;
		QVERIFY(dir.isValid());
		const QString path = dir.filePath(QStringLiteral("list.qet"));
		{
			QETProject project;
			Diagram *folio = project.addNewDiagram();
			QVERIFY(symbol(folio, QStringLiteral("K1")));
			project.dataBase()->updateDB();
			QVERIFY(addList(folio, ListQueryWidget::Device));
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QETProject project(path);
		project.loadFolios();
		QetGraphicsTableItem *t = table(project.diagrams().first());
		QVERIFY(t);
		auto *model = qobject_cast<ProjectDBModel *>(t->model());
		QVERIFY(model);
		QCOMPARE(model->identifier(), QStringLiteral("device_list"));
		QCOMPARE(model->queryString(), ListQueryWidget::queryStr(ListQueryWidget::Device));
		QCOMPARE(column(model, 0), QStringList({QStringLiteral("-K1")}));
	}

		//Too many rows for the folio: the rest goes on to a linked table
	void longTableContinuesOnFurtherFolios()
	{
		QETProject project;
		Diagram *folio = project.addNewDiagram();
		for (int i = 0 ; i < 60 ; ++i)
			QVERIFY(symbol(folio, QStringLiteral("K%1").arg(i + 100)));
		project.dataBase()->updateDB();
		QetGraphicsTableItem *t = addList(folio, ListQueryWidget::Device, true);
		QVERIFY(t);
		QVERIFY(t->displayNRow() < 60);
		QVERIFY(project.diagrams().size() > 1);
		QVERIFY(t->nextTable());
	}

		//A QET without the view reads the saved query, finds nothing, and
		//shows an empty table: no crash, no row
	void unknownViewIsAnEmptyTable()
	{
		QETProject project;
		project.addNewDiagram();
		ProjectDBModel model(&project);
		model.setIdentifier(QStringLiteral("device_list"));
		model.setQuery(QStringLiteral("SELECT device FROM qet_view_from_the_future"));
		QCOMPARE(model.rowCount(), 0);
		QCOMPARE(model.columnCount(), 0);
	}
};

QET_APP_TEST_MAIN(tst_listtables)
#include "tst_listtables.moc"
