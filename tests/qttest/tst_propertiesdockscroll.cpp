// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include "PropertiesEditor/propertieseditordockwidget.h"
#include "PropertiesEditor/propertieseditorwidget.h"

#include <QMainWindow>
#include <QScrollArea>

// The selection properties dock scrolls its editors when it is lower than
// they are: before, its smallest height was theirs, and in a low window the
// dock below was drawn over its last rows. It still shrinks to its title
// bar without an editor and is no higher than its editors with one.
class tst_propertiesdockscroll : public QObject
{
	Q_OBJECT

	private slots:
		void scrollsWhenLow()
		{
			QMainWindow window;
			auto *dock = new PropertiesEditorDockWidget(&window);
			auto *below = new QDockWidget(QStringLiteral("below"), &window);
			below->setWidget(new QWidget(below));
			below->widget()->setMinimumHeight(100);
			window.addDockWidget(Qt::RightDockWidgetArea, dock);
			window.addDockWidget(Qt::RightDockWidgetArea, below);
			window.setCentralWidget(new QWidget(&window));
			window.resize(600, 400);
			window.show();
			QVERIFY(QTest::qWaitForWindowExposed(&window));

			auto *editor = new PropertiesEditorWidget;
			editor->setMinimumHeight(800);
			dock->addEditor(editor);
			QApplication::processEvents();
			QTest::qWait(50);

			QVERIFY2(dock->minimumSizeHint().height() < 300,
					 qPrintable(QString::number(dock->minimumSizeHint().height())));
			QVERIFY(dock->geometry().bottom() < below->geometry().top());
			auto *scroll = dock->findChild<QScrollArea *>(QStringLiteral("m_scroll_area"));
			QVERIFY(scroll);
			QVERIFY(scroll->height() < editor->minimumHeight());
			QCOMPARE(scroll->maximumHeight(), scroll->widget()->sizeHint().height());

				// Without an editor, nothing is left to scroll
			dock->removeEditor(editor);
			delete editor;
			QApplication::processEvents();
			QCOMPARE(scroll->maximumHeight(), 0);
		}
};

QET_APP_TEST_MAIN(tst_propertiesdockscroll)

#include "tst_propertiesdockscroll.moc"
