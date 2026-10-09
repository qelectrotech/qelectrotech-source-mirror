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
#include "toolbarsettings.h"
#include "ui/configpage/toolbarsconfigpage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QSettings>
#include <QStandardPaths>
#include <QTest>
#include <QToolBar>

/**
	ToolbarSettings and its configuration page: defaults change nothing,
	saved values reach every open window that took them, and a toolbar
	inside a panel or a window that never took them is left alone.
*/
class tst_toolbarsettings : public QObject
{
	Q_OBJECT

	QMainWindow *m_window = nullptr;
	QToolBar *m_top = nullptr;
	QToolBar *m_left = nullptr;
	QToolBar *m_in_dock = nullptr;
	QMainWindow *m_other = nullptr;
	QToolBar *m_other_bar = nullptr;
	QSize m_default_size;

private slots:
	void initTestCase()
	{
		QStandardPaths::setTestModeEnabled(true);
			//A scope of this test's own. Without an organization name,
			//QSettings on Windows has no registry key: it drops every
			//write and reads only defaults.
		QCoreApplication::setOrganizationName(QStringLiteral("QElectroTech-tst_toolbarsettings"));
		QCoreApplication::setApplicationName(QStringLiteral("tst_toolbarsettings"));
		QSettings().remove(QStringLiteral("toolbars"));

		m_window = new QMainWindow();
		m_default_size = m_window->iconSize();
		m_top = new QToolBar(QStringLiteral("top"), m_window);
		m_window->addToolBar(Qt::TopToolBarArea, m_top);
			//Created without a parent, as some QET toolbars are
		m_left = new QToolBar(QStringLiteral("left"));
		m_window->addToolBar(Qt::LeftToolBarArea, m_left);
		auto *dock = new QDockWidget(m_window);
		m_in_dock = new QToolBar(dock);
		dock->setWidget(m_in_dock);
		m_window->addDockWidget(Qt::LeftDockWidgetArea, dock);
		m_window->show();

			//A window that never takes the settings, like the print window
		m_other = new QMainWindow();
		m_other_bar = new QToolBar(QStringLiteral("other"), m_other);
		m_other->addToolBar(Qt::TopToolBarArea, m_other_bar);
		m_other->show();
	}

	void cleanupTestCase()
	{
		delete m_window;
		delete m_other;
	}

	void defaultsChangeNothing()
	{
		QCOMPARE(ToolbarSettings::iconSize(), 0);
		QCOMPARE(ToolbarSettings::buttonStyle(), Qt::ToolButtonIconOnly);
		QCOMPARE(ToolbarSettings::locked(), false);
		ToolbarSettings::applyTo(m_window);
		QCOMPARE(m_window->iconSize(), m_default_size);
		QCOMPARE(m_top->iconSize(), m_default_size);
		QCOMPARE(m_top->toolButtonStyle(), Qt::ToolButtonIconOnly);
		QVERIFY(m_top->isMovable());
		QVERIFY(m_left->isMovable());
	}

	void pageSavesAndAppliesToOpenWindows()
	{
		{
			ToolbarsConfigPage page;
			auto *size = page.findChild<QComboBox *>(QStringLiteral("iconSizeCombo"));
			auto *style = page.findChild<QComboBox *>(QStringLiteral("buttonStyleCombo"));
			size->setCurrentIndex(size->findData(32));
			style->setCurrentIndex(style->findData(int(Qt::ToolButtonTextUnderIcon)));
			page.findChild<QCheckBox *>(QStringLiteral("lockedCheck"))->setChecked(true);
			page.applyConf();
		}
		QCOMPARE(ToolbarSettings::iconSize(), 32);
		QCOMPARE(ToolbarSettings::buttonStyle(), Qt::ToolButtonTextUnderIcon);
		QVERIFY(ToolbarSettings::locked());

		QCOMPARE(m_top->iconSize(), QSize(32, 32));
		QCOMPARE(m_left->iconSize(), QSize(32, 32));
		QCOMPARE(m_top->toolButtonStyle(), Qt::ToolButtonTextUnderIcon);
		QVERIFY(!m_top->isMovable());
		QVERIFY(!m_left->isMovable());
			//A toolbar inside a panel is not one of the window's toolbars
		QVERIFY(m_in_dock->isMovable());
			//Nor is a window that did not take the settings when it opened
		QCOMPARE(m_other_bar->iconSize(), m_default_size);
		QCOMPARE(m_other_bar->toolButtonStyle(), Qt::ToolButtonIconOnly);
		QVERIFY(m_other_bar->isMovable());

			//A page opened now shows what was saved
		ToolbarsConfigPage page;
		QCOMPARE(page.findChild<QComboBox *>(QStringLiteral("iconSizeCombo"))->currentData().toInt(), 32);
		QVERIFY(page.findChild<QCheckBox *>(QStringLiteral("lockedCheck"))->isChecked());
	}

	void backToDefaultsRemovesTheKeys()
	{
		ToolbarSettings::save(0, Qt::ToolButtonIconOnly, false);
		QVERIFY(!QSettings().contains(QStringLiteral("toolbars/icon_size")));
		QVERIFY(!QSettings().contains(QStringLiteral("toolbars/button_style")));
		QVERIFY(!QSettings().contains(QStringLiteral("toolbars/locked")));
		ToolbarSettings::applyToAll();
		QCOMPARE(m_top->iconSize(), m_default_size);
		QCOMPARE(m_top->toolButtonStyle(), Qt::ToolButtonIconOnly);
		QVERIFY(m_top->isMovable());
	}

	void unknownStyleFallsBack()
	{
		QSettings().setValue(QStringLiteral("toolbars/button_style"), 99);
		QCOMPARE(ToolbarSettings::buttonStyle(), Qt::ToolButtonIconOnly);
		QSettings().remove(QStringLiteral("toolbars/button_style"));
	}
};

QTEST_MAIN(tst_toolbarsettings)
#include "tst_toolbarsettings.moc"
