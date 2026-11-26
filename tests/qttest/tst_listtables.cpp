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
#include "cable/cable.h"
#include "qetproject.h"
#include "ui/wiringlistdialog.h"
#include "qetresult.h"

#include <QDomDocument>
#include <QTableView>
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
<terminal x="0" y="-20" orientation="n" uuid="%2" name="A1"/><terminal x="0" y="20" orientation="s" uuid="%3" name="A2"/>
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

		//B10: the wiring table is the wiring list, row for row, and adds
		//the cable and the core. Two folios, wire numbers that a plain text
		//sort would put in the wrong order (2, 10, 9), one wire on a core
	void wiringTableMatchesTheWiringListDialog()
	{
		QETProject project;
		Diagram *first = project.addNewDiagram();
		Diagram *second = project.addNewDiagram();
		QList<Conductor *> wires;
		const auto wire = [&](Diagram *folio, const QString &from, const QString &to,
							  const QString &number) {
			Element *a = symbol(folio, from);
			Element *b = symbol(folio, to);
			QVERIFY(a && b);
			QVERIFY(a->terminals().size() == 2 && b->terminals().size() == 2);
			auto *conductor = new Conductor(a->terminals().last(), b->terminals().first());
			folio->addItem(conductor);
			auto properties = conductor->properties();
			properties.text = number;
			conductor->setProperties(properties);
			wires << conductor;
		};
		wire(second, QStringLiteral("K5"), QStringLiteral("K6"), QStringLiteral("9"));
		wire(first, QStringLiteral("K1"), QStringLiteral("K2"), QStringLiteral("10"));
		wire(first, QStringLiteral("K3"), QStringLiteral("K4"), QStringLiteral("2"));
		QCOMPARE(wires.size(), 3);

		auto *w1 = new Cable(&project);
		w1->setDesignation(QStringLiteral("W1"));
		w1->setCoreCount(3);
		project.addCable(w1);
		CableCore core;
		core.core = 1;
		core.conductor = wires.at(1)->uuid();
		w1->setCore(core);
		auto properties = wires.at(1)->properties();
		properties.m_cable_uuid = w1->uuid();
		properties.m_cable = QStringLiteral("W1");
		wires.at(1)->setProperties(properties);
		project.dataBase()->updateDB();

		QetGraphicsTableItem *t = addList(first, ListQueryWidget::Wiring);
		QVERIFY(t);
		auto *model = qobject_cast<ProjectDBModel *>(t->model());
		QVERIFY(model);
		QCOMPARE(model->identifier(), QStringLiteral("wiring_table"));

		WiringListDialog dialog(&project);
		auto *view = dialog.findChild<QTableView *>();
		QVERIFY(view && view->model());
		const QAbstractItemModel *list = view->model();
		QCOMPARE(model->rowCount(), 3);
		QCOMPARE(model->rowCount(), list->rowCount());
		for (int r = 0 ; r < list->rowCount() ; ++r) {
				//Wire, sheet; each end is the dialog's component and terminal,
				//the component written as a designation (-K3)
			const auto designation = [](const QString &label) {
				return label.startsWith(QLatin1Char('-')) ? label : QLatin1Char('-') + label;
			};
			QCOMPARE(model->index(r, 0).data(), list->index(r, 0).data());
			QCOMPARE(model->index(r, 5).data(), list->index(r, 5).data());
			QCOMPARE(model->index(r, 1).data().toString(),
					 designation(list->index(r, 1).data().toString()) + QLatin1Char(':') + list->index(r, 2).data().toString());
			QCOMPARE(model->index(r, 2).data().toString(),
					 designation(list->index(r, 3).data().toString()) + QLatin1Char(':') + list->index(r, 4).data().toString());
		}
		QCOMPARE(column(model, 0), QStringList({QStringLiteral("2"), QStringLiteral("10"), QStringLiteral("9")}));
		QCOMPARE(model->index(0, 1).data().toString(), QStringLiteral("-K3:A2"));
		QCOMPARE(model->index(0, 2).data().toString(), QStringLiteral("-K4:A1"));
			//Only the wire on a core has a cable and a core (the core as the list writes it, from 1)
		QCOMPARE(column(model, 3), QStringList({QString(), QStringLiteral("W1"), QString()}));
		QCOMPARE(column(model, 4), QStringList({QString(), QStringLiteral("2"), QString()}));
		QCOMPARE(model->headerData(1, Qt::Horizontal).toString(), QStringLiteral("From"));
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
