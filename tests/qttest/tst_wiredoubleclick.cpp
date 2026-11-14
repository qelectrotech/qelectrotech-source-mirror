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
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "conductorproperties.h"
#include "diagram.h"
#include "factory/elementfactory.h"
#include "qetapptest.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "QetGraphicsItemModeler/qetgraphicshandleritem.h"
#include "qetproject.h"
#include <QDomDocument>
#include <QApplication>
#include <QDialog>
#include <QGraphicsSceneMouseEvent>
#include <QTimer>
#include <QtTest>

/**
	A double-click on the middle of a wire opens its properties. The first
	click selects the wire and puts a handle under the pointer, and the
	second click went to the handle as a plain press, which started moving
	the segment instead.
*/
class tst_wiredoubleclick : public QObject
{
	Q_OBJECT

		/// A symbol with two terminals, top and bottom
	static Element *symbol(Diagram *diagram, const QPointF &pos)
	{
		auto *collection = diagram->project()->embeddedElementCollection();
		const QString name = QStringLiteral("test_simple");
		if (!collection->exist(QStringLiteral("import/%1.elmt").arg(name))) {
			QDomDocument definition;
			definition.setContent(
				QString(R"(<definition type="element" link_type="simple" uuid="%1" width="20" height="40" hotspot_x="10" hotspot_y="20">
<names><name lang="en">Test</name></names>
<description><line x1="0" y1="-10" x2="0" y2="10"/>
<terminal x="0" y="-20" orientation="n" uuid="%2"/><terminal x="0" y="20" orientation="s" uuid="%3"/>
</description></definition>)")
					.arg(QUuid::createUuid().toString(), QUuid::createUuid().toString(),
						 QUuid::createUuid().toString()));
			if (!collection->addElementDefinition(QStringLiteral("import"), name,
												  definition.documentElement()))
				return nullptr;
		}
		int state = 0;
		Element *element = ElementFactory::Instance()->createElement(
			ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name),
							 diagram->project()),
			nullptr, &state);
		if (state || !element) return nullptr;
		diagram->addItem(element);
		element->setPos(pos);
		return element;
	}

	static Terminal *top(Element *element)
	{
		Terminal *found = element->terminals().value(0);
		for (Terminal *t : element->terminals())
			if (t->scenePos().y() < found->scenePos().y()) found = t;
		return found;
	}

	static void mouse(QGraphicsScene *scene, QGraphicsItem *item, QEvent::Type type,
					  const QPointF &pos)
	{
		QGraphicsSceneMouseEvent event(type);
		event.setScenePos(pos);
		event.setPos(item->mapFromScene(pos));
		event.setButton(Qt::LeftButton);
		event.setButtons(type == QEvent::GraphicsSceneMouseRelease ? Qt::NoButton
																	 : Qt::LeftButton);
		scene->sendEvent(item, &event);
	}

	static QList<QetGraphicsHandlerItem *> handlers(QGraphicsScene *scene)
	{
		QList<QetGraphicsHandlerItem *> found;
		for (QGraphicsItem *item : scene->items())
			if (auto *handler = dynamic_cast<QetGraphicsHandlerItem *>(item))
				found << handler;
		return found;
	}

private slots:
	void initTestCase() { QETProject::setBackupEnabled(false); }

	void aDoubleClickOnTheMiddleOpensTheProperties()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		Element *k1 = symbol(diagram, {100, 300});
		Element *k2 = symbol(diagram, {400, 100});
		QVERIFY(k1 && k2);
		auto *wire = new Conductor(top(k1), top(k2));
		diagram->addItem(wire);

			//Where the handle of the middle segment stands once selected
		wire->setSelected(true);
		QList<QetGraphicsHandlerItem *> shown = handlers(diagram);
		QVERIFY(!shown.isEmpty());
		const QPointF middle = shown.at(shown.size() / 2)->scenePos();
		wire->setSelected(false);
		QVERIFY(handlers(diagram).isEmpty());

			//First click: on the wire, which selects it and shows the handle
		mouse(diagram, wire, QEvent::GraphicsSceneMousePress, middle);
		mouse(diagram, wire, QEvent::GraphicsSceneMouseRelease, middle);
		QVERIFY(wire->isSelected());
		QetGraphicsHandlerItem *under = nullptr;
		for (QetGraphicsHandlerItem *handler : handlers(diagram))
			if (QLineF(handler->scenePos(), middle).length() < 1.0) under = handler;
		QVERIFY(under);

			//Second click, at once: on the handle now under the pointer
		QString opened;
		QTimer::singleShot(0, [&opened]() {
			if (QWidget *dialog = QApplication::activeModalWidget()) {
				opened = QString::fromLatin1(dialog->metaObject()->className());
				if (auto *d = qobject_cast<QDialog *>(dialog)) d->reject();
			}
		});
		mouse(diagram, under, QEvent::GraphicsSceneMousePress, middle);
		QCoreApplication::processEvents();
		QCOMPARE(opened, QStringLiteral("ConductorPropertiesDialog"));
	}
};

QET_APP_TEST_MAIN(tst_wiredoubleclick)
#include "tst_wiredoubleclick.moc"
