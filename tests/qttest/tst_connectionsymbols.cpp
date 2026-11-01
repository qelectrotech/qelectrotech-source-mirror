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
