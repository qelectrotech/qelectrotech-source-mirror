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
#include "ElementsCollection/elementslocation.h"
#include "ElementsCollection/xmlelementcollection.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/terminal.h"
#include "qetgraphicsitem/terminalelement.h"
#include "qetproject.h"

#include <QDomDocument>
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

		/// A click with the tool for @a location at @a at: one placed
		void placeAt(ElementsLocation location, Diagram *diagram, const QPointF &at)
		{
			DiagramEventAddElement tool(location, diagram);
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

		/// A click with the corner's tool at @a at: one placed
		void placeCorner(Diagram *diagram, const QPointF &at)
		{
			placeAt(ConnectionSymbols::location(ConnectionSymbols::Corner), diagram, at);
		}

		/**
			@brief ownJunction
			@return a junction symbol of this test's own, in the project's
			own collection.

			The shipped connection symbols are retyped by the elements
			collection, which lives in another repository and is not part
			of this change, so what is checked here is what QElectroTech
			does with a junction wherever the symbol came from (#1500).
		*/
		ElementsLocation ownJunction()
		{
			Diagram *diagram = QetAppTest::diagram();
			auto *collection = diagram->project()->embeddedElementCollection();
			const QString name = QStringLiteral("test_junction");
			if (!collection->exist(QStringLiteral("import/%1.elmt").arg(name))) {
				QDomDocument definition;
				definition.setContent(QStringLiteral(
					R"(<definition type="element" link_type="junction" width="20" height="20" hotspot_x="10" hotspot_y="10">
<names><name lang="en">Test junction</name></names>
<description>
<line x1="0" y1="10" x2="20" y2="10" style="line-style:normal;line-weight:normal;filling:none;color:black"/>
<terminal orientation="w" uuid="%1" x="0" y="10"/><terminal orientation="e" uuid="%2" x="20" y="10"/>
</description></definition>)")
						.arg(QUuid::createUuid().toString(), QUuid::createUuid().toString()));
				if (!collection->addElementDefinition(QStringLiteral("import"), name,
								 definition.documentElement())) {
					return ElementsLocation();
				}
			}
			return ElementsLocation(QStringLiteral("embed://import/%1.elmt").arg(name),
						diagram->project());
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

			//A junction placed on the folio is not a terminal: it is
			//never numbered, carries no label, and is no terminal the
			//terminal strip manager could pick up as a free one. The
			//symbol is this test's own, because the shipped ones are
			//retyped by the elements collection, which lives in another
			//repository and is not part of this change (#1500)
		void aPlacedJunctionIsNotATerminal()
		{
			Diagram *diagram = QetAppTest::diagram();
			const QPointF at(500, 300);
			const ElementsLocation junction = ownJunction();
			QVERIFY(!junction.isNull());
			placeAt(junction, diagram, at);
			const QList<Element *> placed = cornersAt(diagram, at);
			QCOMPARE(placed.size(), 1);
			Element *placed_junction = placed.first();

			QCOMPARE(int(placed_junction->elementData().m_type), int(ElementData::Junction));
			QCOMPARE(int(placed_junction->linkType()), int(Element::Junction));

				//No numbering scheme may reach it, and it holds no label to give
			QVERIFY(!placed_junction->takesSchemeNumber());
			QVERIFY(placed_junction->actualLabel().isEmpty());

				//The type the terminal strip code casts to, and the type the
				//free-terminal and terminal-numbering queries filter on
			QVERIFY2(!dynamic_cast<TerminalElement *>(placed_junction),
				 "a junction is no terminal, so the terminal plan stays free of it");

			diagram->undoStack().undo();
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
