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
	The Connections toolbar: one button per symbol on the wires, a click
	places its symbol on the folio as one taken from the collection, and
	the symbol a button places can be changed.
*/
#include "qetapptest.h"
#include "connectionsymbols.h"
#include "qetmessagebox.h"
#include "diagramevent/diagrameventaddelement.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"

#include <QGraphicsSceneMouseEvent>
#include <QUndoStack>

#include <QToolBar>
#include <QToolButton>

class TstConnectionSymbols : public QObject
{
	Q_OBJECT

	private:
		QAction *button(ConnectionSymbols::Kind kind)
		{
			return QetAppTest::action(ConnectionSymbols::commandId(kind).section('.', 1));
		}

		/// A click with the corner's tool at @a at: one placed
		void placeCorner(Diagram *diagram, const QPointF &at)
		{
			ElementsLocation corner = ConnectionSymbols::location(ConnectionSymbols::Corner);
			DiagramEventAddElement tool(corner, diagram);
			for (QEvent::Type type : {QEvent::GraphicsSceneMouseMove, QEvent::GraphicsSceneMousePress,
						  QEvent::GraphicsSceneMouseRelease}) {
				QGraphicsSceneMouseEvent event(type);
				event.setButton(Qt::LeftButton);
				event.setButtons(type == QEvent::GraphicsSceneMousePress ? Qt::LeftButton : Qt::NoButton);
				event.setScenePos(at);
				if (type == QEvent::GraphicsSceneMouseMove) tool.mouseMoveEvent(&event);
				else if (type == QEvent::GraphicsSceneMousePress) tool.mousePressEvent(&event);
				else tool.mouseReleaseEvent(&event);
			}
		}

		QList<Element *> cornersAt(Diagram *diagram, const QPointF &at)
		{
			QList<Element *> found;
			for (Element *e : diagram->elements())
				if (QLineF(e->scenePos(), at).length() < 0.5) found << e;
			return found;
		}

	private slots:
		void initTestCase()
		{
			QET::QetMessageBox::setNonInteractive(true);
			QVERIFY(QetAppTest::view());
			QApplication::processEvents();
		}

		void toolbarHasEveryButton()
		{
			auto *toolbar = QetAppTest::editor()->findChild<QToolBar *>(QStringLiteral("connections"));
			QVERIFY(toolbar);
			QVERIFY2(toolbar->isHidden(), "hidden until switched on");
			for (ConnectionSymbols::Kind kind : ConnectionSymbols::kinds()) {
				QAction *action = button(kind);
				QVERIFY2(action, qPrintable(ConnectionSymbols::commandId(kind)));
				QVERIFY(toolbar->actions().contains(action));
				auto *tool = qobject_cast<QToolButton *>(toolbar->widgetForAction(action));
				QVERIFY(tool);
				QCOMPARE(tool->popupMode(), QToolButton::MenuButtonPopup);
			}
		}

		void clickPlacesTheSymbol()
		{
			Diagram *diagram = QetAppTest::diagram();
			QVERIFY(ConnectionSymbols::location(ConnectionSymbols::Corner).exist());
			button(ConnectionSymbols::Corner)->trigger();
			QVERIFY(diagram->eventInterfaceIsRunning());
			diagram->clearEventInterface();
		}

		void noSymbolChosenPlacesNothing()
		{
			Diagram *diagram = QetAppTest::diagram();
			QVERIFY(!ConnectionSymbols::location(ConnectionSymbols::Interruption).exist());
			button(ConnectionSymbols::Interruption)->trigger();
			QVERIFY(!diagram->eventInterfaceIsRunning());
		}

			//Off, as before: a second corner on the first one, both stay
		void secondCornerStaysByDefault()
		{
			Diagram *diagram = QetAppTest::diagram();
			const QPointF at(400, 300);
			placeCorner(diagram, at);
			placeCorner(diagram, at);
			QCOMPARE(cornersAt(diagram, at).size(), 2);
			diagram->undoStack().undo();
			diagram->undoStack().undo();
		}

			//On: the new corner takes the old one's place and its wire
		void newCornerReplacesOld()
		{
			ConnectionSymbols::setNewJunctionReplacesOld(true);
			Diagram *diagram = QetAppTest::diagram();
			const QPointF at(400, 300), far(400, 500);
			placeCorner(diagram, at);
			placeCorner(diagram, far);
			Element *old = cornersAt(diagram, at).value(0);
			Element *other = cornersAt(diagram, far).value(0);
			QVERIFY(old && other);
			auto *wire = new Conductor(old->terminals().first(), other->terminals().first());
			diagram->addItem(wire);
			const QPointF dock = old->terminals().first()->dockConductor();

			placeCorner(diagram, at);
			const QList<Element *> now = cornersAt(diagram, at);
			QCOMPARE(now.size(), 1);
			QVERIFY(now.first() != old);
			bool wired = false;
			for (Terminal *t : now.first()->terminals())
				for (Conductor *c : t->conductors())
					wired |= QLineF(t->dockConductor(), dock).length() < 0.5
						&& (c->terminal1->parentElement() == other || c->terminal2->parentElement() == other);
			QVERIFY2(wired, "the old corner's wire goes to the new one");

				//One undo brings the old one back, with its wire
			diagram->undoStack().undo();
			QCOMPARE(cornersAt(diagram, at), QList<Element *>{old});
			QCOMPARE(old->conductors().size(), 1);
			ConnectionSymbols::setNewJunctionReplacesOld(false);
		}

		void chosenSymbolIsPlaced()
		{
			Diagram *diagram = QetAppTest::diagram();
			ConnectionSymbols::setPath(ConnectionSymbols::Interruption,
						   ConnectionSymbols::defaultPath(ConnectionSymbols::Corner));
			button(ConnectionSymbols::Interruption)->trigger();
			QVERIFY(diagram->eventInterfaceIsRunning());
			diagram->clearEventInterface();
			ConnectionSymbols::resetPath(ConnectionSymbols::Interruption);
			QVERIFY(ConnectionSymbols::path(ConnectionSymbols::Interruption).isEmpty());
		}
};

	//As QET_APP_TEST_MAIN, with the source tree's common collection, where
	//the default symbols come from
int main(int argc, char **argv)
{
	QStandardPaths::setTestModeEnabled(true);
	QApplication app(argc, argv);
	QCoreApplication::setOrganizationName("QElectroTech");
	QCoreApplication::setApplicationName("QElectroTech");
	QTemporaryDir home;
	if (!home.isValid())
		return 1;
	QSettings::setDefaultFormat(QSettings::IniFormat);
	QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, home.filePath("settings"));
	QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, home.filePath("settings"));
	QETApp::applyDirectoryArguments(QETArguments(QList<QString>{
		"--config-dir=" + home.filePath("config"),
		"--data-dir=" + home.filePath("data"),
		QStringLiteral("--common-elements-dir=" QET_ELEMENTS_DIR "/")}));
	QETApp qetapp;
	QCoreApplication::processEvents();
	TstConnectionSymbols test;
	return QTest::qExec(&test, argc, argv);
}

#include "tst_connectionsymbols.moc"
