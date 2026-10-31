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
#include "qetapptest.h"
#include "ui/dockfold.h"

#include <QAction>
#include <QDockWidget>
#include <QMenu>
#include <QToolButton>

/**
	An arrow beside the drawing folds the docks of its side away, as in
	LibreOffice's sidebar, and a second click brings them back at the
	width they had. A dock opened from the Windows menu ends the fold.
*/
class tst_dockfold : public QObject
{
	Q_OBJECT

	static QList<QDockWidget *> openDocks(QMainWindow *window, Qt::DockWidgetArea area)
	{
		QList<QDockWidget *> docks;
		for (QDockWidget *dock : window->findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly))
			if (window->dockWidgetArea(dock) == area && !dock->isFloating()
			    && dock->toggleViewAction()->isChecked())
				docks << dock;
		return docks;
	}

	private slots:
		void initTestCase()
		{
				//Off by default: no bars until the setting is on
			QVERIFY(!DockFold::enabled());
			DockFold::setEnabled(true);
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			editor->resize(1600, 1000);
			editor->show();
			QVERIFY(QTest::qWaitForWindowExposed(editor));
			QTest::qWait(100);
		}

		void foldsAndUnfoldsTheRightDocks()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			auto *button = editor->findChild<QToolButton *>(QStringLiteral("qet_dock_fold_right"));
			QVERIFY(button);
			QVERIFY(button->isVisible());
			const QList<QDockWidget *> docks = openDocks(editor, Qt::RightDockWidgetArea);
			QVERIFY(!docks.isEmpty());
			QDockWidget *front = nullptr;
			for (QDockWidget *dock : docks)
				if (dock->isVisible())
					front = dock;
			QVERIFY(front);
			editor->resizeDocks({front}, {380}, Qt::Horizontal);
			QTest::qWait(100);
			const int width = front->width();
			const int drawing = editor->centralWidget()->width();

			button->click();
			QTest::qWait(100);
			QVERIFY(openDocks(editor, Qt::RightDockWidgetArea).isEmpty());
			QVERIFY(editor->centralWidget()->width() > drawing + 300);
				//The bar stays at the edge, in a strip of its own
			QVERIFY(button->isVisible());
			QCOMPARE(editor->centralWidget()->contentsMargins().right(), button->width());
			QVERIFY(button->geometry().right() <= editor->centralWidget()->geometry().right());
			QVERIFY(button->geometry().right() > editor->centralWidget()->geometry().right() - 40);

			button->click();
			QTest::qWait(200);
			QCOMPARE(openDocks(editor, Qt::RightDockWidgetArea).size(), docks.size());
			QVERIFY(front->isVisible());
			QVERIFY2(qAbs(front->width() - width) <= 4,
				 qPrintable(QString("%1 px wide, %2 before").arg(front->width()).arg(width)));
		}

		void aDockOpenedEndsTheFold()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			auto *button = editor->findChild<QToolButton *>(QStringLiteral("qet_dock_fold_right"));
			QVERIFY(button);
			const QList<QDockWidget *> docks = openDocks(editor, Qt::RightDockWidgetArea);
			button->click();
			QTest::qWait(100);
			docks.first()->toggleViewAction()->trigger();
			QTest::qWait(100);
				//A second click folds again rather than unfolding
			QVERIFY(docks.first()->isVisible());
			button->click();
			QTest::qWait(100);
			QVERIFY(openDocks(editor, Qt::RightDockWidgetArea).isEmpty());
			button->click();
			QTest::qWait(100);
			QVERIFY(!openDocks(editor, Qt::RightDockWidgetArea).isEmpty());
		}

		void theLeftDocksFoldToo()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			auto *button = editor->findChild<QToolButton *>(QStringLiteral("qet_dock_fold_left"));
			QVERIFY(button);
			QVERIFY(button->isVisible());
			const int count = openDocks(editor, Qt::LeftDockWidgetArea).size();
			QVERIFY(count > 0);
			button->click();
			QTest::qWait(100);
			QVERIFY(openDocks(editor, Qt::LeftDockWidgetArea).isEmpty());
			QCOMPARE(button->geometry().left(), editor->centralWidget()->geometry().left());
			button->click();
			QTest::qWait(100);
			QCOMPARE(openDocks(editor, Qt::LeftDockWidgetArea).size(), count);
		}

			//The Display menu folds too, bars or not
		void theMenuFoldsWithoutBars()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			DockFold::setEnabled(false);
			QAction *fold = QetAppTest::action(QStringLiteral("fold_right_docks"));
			QVERIFY(fold);
			auto *menu = editor->findChild<QMenu *>(QStringLiteral("display_menu"));
			QVERIFY(menu && menu->actions().contains(fold));
			fold->trigger();
			QTest::qWait(100);
			QVERIFY(openDocks(editor, Qt::RightDockWidgetArea).isEmpty());
			fold->trigger();
			QTest::qWait(200);
			QVERIFY(!openDocks(editor, Qt::RightDockWidgetArea).isEmpty());
			DockFold::setEnabled(true);
		}

			//Switched off, the bars and their strips go at once
		void noBarsWithTheSettingOff()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			DockFold::setEnabled(false);
			for (const QString &name : {QStringLiteral("qet_dock_fold_right"),
						    QStringLiteral("qet_dock_fold_left")})
				QVERIFY(!editor->findChild<QToolButton *>(name)->isVisible());
			QCOMPARE(editor->centralWidget()->contentsMargins(), QMargins());
			DockFold::setEnabled(true);
		}
};

QET_APP_TEST_MAIN(tst_dockfold)

#include "tst_dockfold.moc"
