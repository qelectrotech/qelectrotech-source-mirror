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
/*
	The Device Navigator lists under a device with a device type the parts
	of its type not drawn yet, and places them: a placed part is listed as
	any symbol, with its folio, and is free again once deleted. The filter
	"Only devices with free parts" keeps the devices with one.
*/
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "devicetype/devicetypecheck.h"
#include "diagram.h"
#include "diagramcontent.h"
#include "diagramevent/diagrameventaddelement.h"
#include "factory/elementfactory.h"
#include "qetapp.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "undocommand/deleteqgraphicsitemcommand.h"
#include "ui/devicenavigatordockwidget.h"
#include "ui/devicenavigatorfilterdialog.h"

#include <QComboBox>
#include <QDomDocument>
#include <QGraphicsSceneMouseEvent>
#include <QScopeGuard>
#include <QSettings>
#include <QTreeWidget>
#include <QtTest>

class tst_devicenavigatorparts : public QObject
{
	Q_OBJECT

	int m_position = 0;

		//An element import/<file>.elmt named @p name, with terminals @p pins
	static bool define(QETProject *project, const QString &file, const QString &name,
					   const QString &link_type, const QStringList &pins)
	{
		QString terminals;
		for (int i = 0 ; i < pins.size() ; ++i)
			terminals += QStringLiteral("<terminal x=\"%1\" y=\"20\" orientation=\"s\" name=\"%2\" uuid=\"%3\"/>")
					.arg(10 * i).arg(pins.at(i), QUuid::createUuid().toString());
		QDomDocument definition;
		if (!definition.setContent(QString(
				R"(<definition type="element" link_type="%1" uuid="%2" width="60" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">%3</name></names>
<kindInformations><kindInformation name="type">%4</kindInformation></kindInformations>
<description><line x1="0" y1="-10" x2="0" y2="10"/>%5</description></definition>)")
				.arg(link_type, QUuid::createUuid().toString(), name,
					 link_type == QLatin1String("slave") ? QStringLiteral("simple") : QStringLiteral("coil"),
					 terminals)))
			return false;
		return project->embeddedElementCollection()->addElementDefinition(
					QStringLiteral("import"), file, definition.documentElement());
	}

	Element *place(Diagram *diagram, const QString &file, const QString &label,
				   const QString &model = QString())
	{
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/") + file + QStringLiteral(".elmt"),
							 diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		const int position = m_position++;
		element->setPos(100 + 80 * position, 150);
		auto info = element->elementInformations();
		info.addValue(QStringLiteral("label"), label);
		info.remove(QStringLiteral("formula"));
		if (!model.isEmpty()) info.addValue(QStringLiteral("device_type"), model);
		element->setElementInformations(info);
		return element;
	}

		//CONTR10: a coil, three power contacts, an auxiliary NO
	static DeviceType contactor()
	{
		DeviceType type;
		type.name = QStringLiteral("CONTR10");
		DeviceType::Part coil{QStringLiteral("master"), QStringLiteral("coil"), {},
			{{QStringLiteral("embed://import/coil.elmt"), {QStringLiteral("A1"), QStringLiteral("A2")}, {}}}};
		DeviceType::Part power{QStringLiteral("slave"), QStringLiteral("power"), {},
			{{QStringLiteral("embed://import/power.elmt"),
			  QStringLiteral("1 2 3 4 5 6").split(QLatin1Char(' ')), {}}}};
		DeviceType::Part aux{QStringLiteral("slave"), QStringLiteral("simple"), {},
			{{QStringLiteral("embed://import/aux.elmt"), {QStringLiteral("13"), QStringLiteral("14")},
			  QStringLiteral("NO")}}};
		type.parts = {coil, power, aux};
		return type;
	}

	void defineAll(QETProject *project)
	{
		QVERIFY(define(project, QStringLiteral("coil"), QStringLiteral("CONTR-COIL"),
					   QStringLiteral("master"), {QStringLiteral("A1"), QStringLiteral("A2")}));
		QVERIFY(define(project, QStringLiteral("power"), QStringLiteral("CONTR-3PH"), QStringLiteral("slave"),
					   QStringLiteral("1 2 3 4 5 6").split(QLatin1Char(' '))));
		QVERIFY(define(project, QStringLiteral("aux"), QStringLiteral("CONTR-AUX-NO"),
					   QStringLiteral("slave"), {QStringLiteral("13"), QStringLiteral("14")}));
		project->setDeviceType(contactor());
	}

	static QTreeWidget *treeOf(DeviceNavigatorDockWidget &navigator)
	{
		return navigator.findChild<QTreeWidget *>(QStringLiteral("m_tree_widget"));
	}

		//The device node labelled @p label
	static QTreeWidgetItem *device(DeviceNavigatorDockWidget &navigator, const QString &label)
	{
		QTreeWidget *tree = treeOf(navigator);
		if (!tree) return nullptr;
		for (QTreeWidgetItemIterator it(tree); *it; ++it)
			if ((*it)->data(0, DeviceNavigatorDockWidget::NodeTypeRole).toInt()
					== DeviceNavigatorDockWidget::DeviceNode
				&& (*it)->text(0) == label)
				return *it;
		return nullptr;
	}

	static QList<QTreeWidgetItem *> children(QTreeWidgetItem *item, int type)
	{
		QList<QTreeWidgetItem *> list;
		for (int i = 0; item && i < item->childCount(); ++i)
			if (item->child(i)->data(0, DeviceNavigatorDockWidget::NodeTypeRole).toInt() == type)
				list << item->child(i);
		return list;
	}

	static QStringList texts(const QList<QTreeWidgetItem *> &items)
	{
		QStringList list;
		for (QTreeWidgetItem *item : items) list << item->text(0);
		return list;
	}

	static void drop(DiagramEventAddElement &placing)
	{
		QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseRelease);
		event.setButton(Qt::LeftButton);
		placing.mouseReleaseEvent(&event);
	}

	private slots:
		void cleanup()
		{
			QSettings().remove(QStringLiteral("devicenavigator"));
		}

			//The coil alone: its power contacts and its NO are free
		void theFreePartsAreListedUnderTheDevice()
		{
			QETProject project;
			Diagram *folio = project.addNewDiagram();
			defineAll(&project);
			QVERIFY(place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10")));

			DeviceNavigatorDockWidget navigator;
			navigator.setProject(&project);
			navigator.reload();
			QTreeWidgetItem *k1 = device(navigator, QStringLiteral("K1"));
			QVERIFY(k1);
			QVERIFY(k1->data(0, DeviceNavigatorDockWidget::HasFreePartsRole).toBool());
			QCOMPARE(children(k1, DeviceNavigatorDockWidget::FunctionNode).size(), 1);
			const auto free = children(k1, DeviceNavigatorDockWidget::FreePartNode);
			QCOMPARE(free.size(), 2);
			QCOMPARE(texts(free).filter(QStringLiteral("free: ")).size(), 2);
			QVERIFY(texts(free).at(1).contains(QStringLiteral("13 14")));
				//After the symbols drawn
			QCOMPARE(k1->indexOfChild(free.first()), 1);
		}

			//A device without a type, and a project without types: as before
		void noTypeNoFreeParts()
		{
			QETProject project;
			Diagram *folio = project.addNewDiagram();
			QVERIFY(define(&project, QStringLiteral("coil"), QStringLiteral("CONTR-COIL"),
						   QStringLiteral("master"), {QStringLiteral("A1"), QStringLiteral("A2")}));
			QVERIFY(place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10")));
			QVERIFY(place(folio, QStringLiteral("coil"), QStringLiteral("K2")));

			DeviceNavigatorDockWidget navigator;
			navigator.setProject(&project);
			navigator.reload();
			for (const QString &label : {QStringLiteral("K1"), QStringLiteral("K2")}) {
				QTreeWidgetItem *item = device(navigator, label);
				QVERIFY(item);
				QVERIFY(children(item, DeviceNavigatorDockWidget::FreePartNode).isEmpty());
				QVERIFY(!item->data(0, DeviceNavigatorDockWidget::HasFreePartsRole).toBool());
			}
		}

			//Placed from the navigator: linked to the coil, listed with
			//its folio; deleted: free again
		void aFreePartIsPlacedAndFreeAgainOnceDeleted()
		{
			QETProject project;
			QETApp::registerProject(&project);
			const auto registered = qScopeGuard([&project]() {QETApp::unregisterProject(&project);});
			Diagram *folio = project.addNewDiagram();
			defineAll(&project);
			Element *coil = place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10"));
			QVERIFY(coil);

			DeviceNavigatorDockWidget navigator;
			navigator.setProject(&project);
			navigator.reload();
			QTreeWidgetItem *k1 = device(navigator, QStringLiteral("K1"));
			QTreeWidgetItem *aux = children(k1, DeviceNavigatorDockWidget::FreePartNode).at(1);
			const int elements = folio->elements().size();
			QVERIFY(navigator.placeFreePart(aux, folio, 0));
			auto *placing = qobject_cast<DiagramEventAddElement *>(folio->eventInterface());
			QVERIFY(placing);
			drop(*placing);
			QCOMPARE(folio->elements().size(), elements + 1);

			Element *contact = nullptr;
			for (Element *e : folio->elements()) if (e != coil) contact = e;
			QVERIFY(contact);
			QCOMPARE(contact->linkedElements(), QList<Element *>({coil}));

			navigator.reload();
			k1 = device(navigator, QStringLiteral("K1"));
			QCOMPARE(children(k1, DeviceNavigatorDockWidget::FunctionNode).size(), 2);
			QVERIFY(!device(navigator, QStringLiteral("<unnamed>")));
			const auto free = children(k1, DeviceNavigatorDockWidget::FreePartNode);
			QCOMPARE(free.size(), 1);
			QVERIFY(!free.first()->text(0).contains(QStringLiteral("13 14")));

			folio->clearSelection();
			contact->setSelected(true);
			folio->undoStack().push(new DeleteQGraphicsItemCommand(folio, DiagramContent(folio, true)));
			QVERIFY(!folio->elements().contains(contact));
			navigator.reload();
			k1 = device(navigator, QStringLiteral("K1"));
			QCOMPARE(children(k1, DeviceNavigatorDockWidget::FreePartNode).size(), 2);
		}

			//A part of another project's folio, or no folio: not placed
		void notPlacedWithoutAFolioOfTheProject()
		{
			QETProject project, other;
			Diagram *folio = project.addNewDiagram();
			Diagram *elsewhere = other.addNewDiagram();
			defineAll(&project);
			QVERIFY(place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10")));

			DeviceNavigatorDockWidget navigator;
			navigator.setProject(&project);
			navigator.reload();
			QTreeWidgetItem *aux = children(device(navigator, QStringLiteral("K1")),
											DeviceNavigatorDockWidget::FreePartNode).at(1);
			QVERIFY(!navigator.placeFreePart(aux, elsewhere, 0));
			QVERIFY(!navigator.placeFreePart(aux));
			QVERIFY(!folio->eventInterface());
		}

			//The filter keeps the devices with a free part
		void theFilterKeepsTheDevicesWithFreeParts()
		{
			const auto defaults = DeviceNavigatorFilterScheme::defaultSchemes();
			QCOMPARE(defaults.last().id, QStringLiteral("free_parts"));
			QVERIFY(defaults.last().onlyFreeParts);
			QCOMPARE(defaults.last().displayName(), QStringLiteral("Devices with free parts"));
			for (int i = 0; i < defaults.size() - 1; ++i)
				QVERIFY(!defaults.at(i).onlyFreeParts);

			QETProject project;
			Diagram *folio = project.addNewDiagram();
			defineAll(&project);
			QVERIFY(place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10")));
			QVERIFY(place(folio, QStringLiteral("coil"), QStringLiteral("K2")));

			DeviceNavigatorDockWidget navigator;
			navigator.setProject(&project);
			navigator.reload();
			auto *combo = navigator.findChild<QComboBox *>(QStringLiteral("m_filter_combo"));
			QVERIFY(combo);
			QVERIFY(!device(navigator, QStringLiteral("K2"))->isHidden());
			combo->setCurrentIndex(combo->findText(QStringLiteral("Devices with free parts")));
			QVERIFY(combo->currentIndex() > 0);
			QVERIFY(!device(navigator, QStringLiteral("K1"))->isHidden());
			QVERIFY(device(navigator, QStringLiteral("K2"))->isHidden());
		}

			//Kept with the other filters
		void theFilterIsSaved()
		{
			auto schemes = DeviceNavigatorFilterScheme::defaultSchemes();
			schemes[1].onlyFreeParts = true;
			DeviceNavigatorFilterScheme::saveSchemes(schemes);
			const auto loaded = DeviceNavigatorFilterScheme::loadSchemes();
			QVERIFY(loaded.at(1).onlyFreeParts);
			QVERIFY(!loaded.at(2).onlyFreeParts);
		}
};

QET_APP_TEST_MAIN(tst_devicenavigatorparts)
#include "tst_devicenavigatorparts.moc"
