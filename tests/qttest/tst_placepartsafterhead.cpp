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
	With the setting on, placing the head of a device with a type goes on
	with its parts not drawn yet: all side by side to the right of the
	head, as one undoable step, or one by one, each following the mouse
	until the placing is given up. Off by default.
*/
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "devicetype/devicetypecheck.h"
#include "devicetype/devicetypeplacement.h"
#include "diagram.h"
#include "diagramevent/diagrameventaddelement.h"
#include "factory/elementfactory.h"
#include "qetapp.h"
#include "qetapptest.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"

#include <QApplication>
#include <QDomDocument>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QScopeGuard>
#include <QSettings>
#include <QTimer>
#include <QtTest>

class tst_placepartsafterhead : public QObject
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

	static void drop(Diagram *folio)
	{
		auto *placing = qobject_cast<DiagramEventAddElement *>(folio->eventInterface());
		QVERIFY(placing);
		QGraphicsSceneMouseEvent event(QEvent::GraphicsSceneMouseRelease);
		event.setButton(Qt::LeftButton);
		placing->mouseReleaseEvent(&event);
	}

	static QList<Element *> others(Diagram *folio, Element *head)
	{
		QList<Element *> list;
		for (Element *element : folio->elements())
			if (element != head) list << element;
		return list;
	}

	private slots:
		void cleanup()
		{
			QSettings().remove(QStringLiteral("diagrameditor/ask_parts_after_head"));
		}

		void offFromTheStart()
		{
			cleanup();
			QVERIFY(!DeviceTypePlacement::askAfterHead());
			DeviceTypePlacement::setAskAfterHead(true);
			QVERIFY(DeviceTypePlacement::askAfterHead());
		}

			//Off: placing the head asks nothing and places nothing more
		void offAsksNothing()
		{
			QETProject project;
			Diagram *folio = project.addNewDiagram();
			defineAll(&project);
			Element *coil = place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10"));
			QVERIFY(coil);
			bool asked = false;
			QTimer::singleShot(0, [&asked]() {
				if (QWidget *box = QApplication::activeModalWidget()) {
					asked = true;
					box->close();
				}
			});
			DeviceTypePlacement::offerParts(coil);
			QCoreApplication::processEvents();
			QVERIFY(!asked);
			QCOMPARE(folio->elements().size(), 1);
			QVERIFY(!folio->eventInterface());
		}

			//On: asked once the head is placed; "All side by side" places them
		void onAsksAndPlaces()
		{
			DeviceTypePlacement::setAskAfterHead(true);
			QETProject project;
			Diagram *folio = project.addNewDiagram();
			defineAll(&project);
			Element *coil = place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10"));
			QVERIFY(coil);
			QString text;
			QTimer::singleShot(0, [&text]() {
				auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
				if (!box) return;
				text = box->text();
				for (QAbstractButton *button : box->buttons())
					if (button->text() == QStringLiteral("All side by side")) button->click();
			});
			DeviceTypePlacement::offerParts(coil);
			QVERIFY(text.startsWith(QStringLiteral("K1 has parts not drawn yet")));
			QVERIFY(text.contains(QStringLiteral("13 14")));
			QCOMPARE(others(folio, coil).size(), 2);
		}

			//What is drawn is not offered again
		void theFreePartsLeaveOutWhatIsDrawn()
		{
			QETProject project;
			QETApp::registerProject(&project);
			const auto registered = qScopeGuard([&project]() {QETApp::unregisterProject(&project);});
			Diagram *folio = project.addNewDiagram();
			defineAll(&project);
			Element *coil = place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10"));
			QVERIFY(coil);
			QCOMPARE(DeviceTypePlacement::freeParts(DeviceTypeCheck::checkDevice(coil)), QList<int>({1, 2}));
			QVERIFY(DeviceTypePlacement::placeOneByOne(DeviceTypeCheck::checkDevice(coil), folio));
			drop(folio);
			folio->clearEventInterface();
			QCOMPARE(DeviceTypePlacement::freeParts(DeviceTypeCheck::checkDevice(coil)), QList<int>({2}));
		}

			//To the right of the head, side by side, linked, one undo step
		void allSideBySide()
		{
			QETProject project;
			Diagram *folio = project.addNewDiagram();
			defineAll(&project);
			Element *coil = place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10"));
			QVERIFY(coil);
			const int steps = folio->undoStack().count();

			const QList<Element *> placed = DeviceTypePlacement::placeSideBySide(DeviceTypeCheck::checkDevice(coil));
			QCOMPARE(placed.size(), 2);
			QCOMPARE(folio->undoStack().count(), steps + 1);
			qreal right = coil->sceneBoundingRect().right();
			for (Element *element : placed) {
				QCOMPARE(element->diagram(), folio);
				QCOMPARE(element->pos().y(), coil->pos().y());
				QVERIFY(element->sceneBoundingRect().left() > right);
				QCOMPARE(element->pos(), Diagram::snapToGrid(element->pos()));
				QCOMPARE(element->linkedElements(), QList<Element *>({coil}));
				right = element->sceneBoundingRect().right();
			}
			QVERIFY(DeviceTypeCheck::checkDevice(coil).missing.isEmpty());
				//Nothing left to place
			QVERIFY(DeviceTypePlacement::placeSideBySide(DeviceTypeCheck::checkDevice(coil)).isEmpty());

			folio->undoStack().undo();
			QCOMPARE(folio->elements().size(), 1);
			QCOMPARE(DeviceTypeCheck::checkDevice(coil).missing.size(), 2);
		}

			//Each dropped part brings the next one, until none is left
		void oneByOne()
		{
			QETProject project;
			QETApp::registerProject(&project);
			const auto registered = qScopeGuard([&project]() {QETApp::unregisterProject(&project);});
			Diagram *folio = project.addNewDiagram();
			defineAll(&project);
			Element *coil = place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10"));
			QVERIFY(coil);
			QVERIFY(DeviceTypePlacement::placeOneByOne(DeviceTypeCheck::checkDevice(coil), folio, QPointF(300, 300)));
			drop(folio);
			QCOMPARE(others(folio, coil).size(), 1);
			QCoreApplication::processEvents();
			drop(folio);
			QCOMPARE(others(folio, coil).size(), 2);
			QCoreApplication::processEvents();
			QVERIFY(!folio->eventInterface());
			for (Element *element : others(folio, coil))
				QCOMPARE(element->linkedElements(), QList<Element *>({coil}));
			QVERIFY(DeviceTypeCheck::checkDevice(coil).missing.isEmpty());
			QVERIFY(!DeviceTypePlacement::placeOneByOne(DeviceTypeCheck::checkDevice(coil), folio));
		}

			//Escape stops: the rest stay free
		void oneByOneGivenUp()
		{
			QETProject project;
			QETApp::registerProject(&project);
			const auto registered = qScopeGuard([&project]() {QETApp::unregisterProject(&project);});
			Diagram *folio = project.addNewDiagram();
			defineAll(&project);
			Element *coil = place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10"));
			QVERIFY(coil);
			QVERIFY(DeviceTypePlacement::placeOneByOne(DeviceTypeCheck::checkDevice(coil), folio));
			drop(folio);
			QCoreApplication::processEvents();
			QVERIFY(folio->eventInterface());
			QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
			folio->eventInterface()->keyPressEvent(&escape);
			QCoreApplication::processEvents();
			QVERIFY(!folio->eventInterface());
			QCOMPARE(others(folio, coil).size(), 1);
			QCOMPARE(DeviceTypeCheck::checkDevice(coil).missing.size(), 1);
		}

			//Not on another project's folio
		void notOnAnotherProject()
		{
			QETProject project, other;
			Diagram *folio = project.addNewDiagram();
			Diagram *elsewhere = other.addNewDiagram();
			defineAll(&project);
			Element *coil = place(folio, QStringLiteral("coil"), QStringLiteral("K1"), QStringLiteral("CONTR10"));
			QVERIFY(coil);
			QVERIFY(!DeviceTypePlacement::placeOneByOne(DeviceTypeCheck::checkDevice(coil), elsewhere));
			QVERIFY(!elsewhere->eventInterface());
		}
};

QET_APP_TEST_MAIN(tst_placepartsafterhead)
#include "tst_placepartsafterhead.moc"
