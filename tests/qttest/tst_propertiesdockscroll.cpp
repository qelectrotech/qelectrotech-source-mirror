// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"

#include "ElementsCollection/elementslocation.h"
#include "PropertiesEditor/propertieseditordockwidget.h"
#include "PropertiesEditor/propertieseditorwidget.h"
#include "qetgraphicsitem/element.h"
#include "ui/elementpropertieswidget.h"

#include <QMainWindow>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWheelEvent>

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

			// Tabbed with another dock, the scroll area keeps no maximum:
			// the tab stack would take it for all its tabs
		void noMaximumWhenTabbed()
		{
			QMainWindow window;
			auto *dock = new PropertiesEditorDockWidget(&window);
			auto *other = new QDockWidget(QStringLiteral("other"), &window);
			other->setWidget(new QWidget(other));
			window.addDockWidget(Qt::RightDockWidgetArea, dock);
			window.addDockWidget(Qt::RightDockWidgetArea, other);
			window.tabifyDockWidget(other, dock);
			window.setCentralWidget(new QWidget(&window));
			window.resize(600, 400);
			window.show();
			QVERIFY(QTest::qWaitForWindowExposed(&window));
			auto *editor = new PropertiesEditorWidget;
			editor->setMinimumHeight(100);
			dock->addEditor(editor);
			QApplication::processEvents();
			QTest::qWait(50);
			auto *scroll = dock->findChild<QScrollArea *>(QStringLiteral("m_scroll_area"));
			QVERIFY(scroll);
			QCOMPARE(scroll->maximumHeight(), QWIDGETSIZE_MAX);
			QVERIFY(dock->height() > 200);
		}

			// An editor that grows (the wire editor, #500) or has a list of
			// its own that scrolls grows with a tall dock: no maximum then
		void growingEditorGrows()
		{
			QFETCH(bool, own_scroll_area);
			QMainWindow window;
			auto *dock = new PropertiesEditorDockWidget(&window);
			window.addDockWidget(Qt::RightDockWidgetArea, dock);
			window.setCentralWidget(new QWidget(&window));
			window.resize(600, 900);
			window.show();
			QVERIFY(QTest::qWaitForWindowExposed(&window));
			auto *editor = new PropertiesEditorWidget;
			auto *layout = new QVBoxLayout(editor);
			if (own_scroll_area) {
				auto *list = new QScrollArea(editor);
				list->setWidget(new QWidget(list));
				list->widget()->setMinimumHeight(2000);
				layout->addWidget(list);
			} else {
				editor->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
				layout->addWidget(new QSpinBox(editor));
			}
			dock->addEditor(editor);
			QApplication::processEvents();
			window.resizeDocks({dock}, {800}, Qt::Vertical);
			QApplication::processEvents();
			QTest::qWait(50);
			auto *scroll = dock->findChild<QScrollArea *>(QStringLiteral("m_scroll_area"));
			QVERIFY(scroll);
			QCOMPARE(scroll->maximumHeight(), QWIDGETSIZE_MAX);
			QVERIFY2(editor->height() > 500, qPrintable(QString::number(editor->height())));
		}
		void growingEditorGrows_data()
		{
			QTest::addColumn<bool>("own_scroll_area");
			QTest::newRow("expanding") << false;
			QTest::newRow("own scroll area") << true;
		}

			// The panel and the element editor's information tab both
			// carry a bar for the same rows: two scroll bars, and the
			// wheel over those rows went to the panel's while the one
			// beside the rows stayed dead to it. Only the editor's own
			// bar is to be left -- it has the way down its rows, and
			// the panel's is out of the way then.
		void onlyTheEditorsOwnBar()
		{
			Diagram *diagram = QetAppTest::diagram();
			QVERIFY(diagram);
			ElementsLocation location;
			auto *element = new Element(location);
			element->setPos(100, 100);
			diagram->addItem(element);

			QMainWindow window;
			auto *dock = new PropertiesEditorDockWidget(&window);
			auto *below = new QDockWidget(QStringLiteral("below"), &window);
			below->setWidget(new QWidget(below));
			below->widget()->setMinimumHeight(80);
			window.addDockWidget(Qt::RightDockWidgetArea, dock);
			window.addDockWidget(Qt::RightDockWidgetArea, below);
			window.setCentralWidget(new QWidget(&window));
			window.resize(600, 1000);
			window.show();
			QVERIFY(QTest::qWaitForWindowExposed(&window));

			auto *editor = new ElementPropertiesWidget(element, dock);
			dock->addEditor(editor);
			QApplication::processEvents();
			QTest::qWait(100);

			auto *scroll = dock->findChild<QScrollArea *>(QStringLiteral("m_scroll_area"));
			QVERIFY(scroll);
			auto *info = editor->findChild<QScrollArea *>(QStringLiteral("scrollArea"));
			QVERIFY2(info, "no scrollArea in the ElementInfoWidget");

				// The panel made lower than the rows stand: where both
				// bars used to show at once
			window.resizeDocks({dock}, {280}, Qt::Vertical);
			QApplication::processEvents();
			QTest::qWait(100);

				// The editor's own bar keeps the way down its rows
			QVERIFY2(info->verticalScrollBar()->maximum() > 0,
					 qPrintable(QStringLiteral(
								 "the information tab has no way down: %1")
								.arg(info->verticalScrollBar()->maximum())));

				// and the panel's is out of the way: one bar, not two
			QVERIFY2(!scroll->verticalScrollBar()->isVisible(),
					 qPrintable(QStringLiteral(
								 "two scroll bars: the panel still offers %1 of way down "
									"(the tab: %2)")
							.arg(scroll->verticalScrollBar()->maximum())
							.arg(info->verticalScrollBar()->maximum())));

			// The wheel over that tab reaches its bar, not the panel's
			const int info_before = info->verticalScrollBar()->value();
			const int panel_before = scroll->verticalScrollBar()->value();
			const QPoint at = info->viewport()->mapToGlobal(QPoint(20, 60));
			QWheelEvent wheel(QPointF(20, 60), at, QPoint(), QPoint(0, -120),
					  Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
			QCoreApplication::sendEvent(info->viewport(), &wheel);
			QTest::qWait(30);
			QVERIFY2(info->verticalScrollBar()->value() != info_before,
					 qPrintable(QStringLiteral(
								 "the wheel over the tab does not move the tab (%1 -> %2) "
									"but the panel (%3 -> %4)")
								.arg(info_before).arg(info->verticalScrollBar()->value())
								.arg(panel_before).arg(scroll->verticalScrollBar()->value())));

				// the panel still shrinks below its editors
			QVERIFY2(dock->minimumSizeHint().height() < 200,
					 qPrintable(QStringLiteral(
								 "the panel grew a minimum of %1 and is no longer smaller "
									"than its editors")
								.arg(dock->minimumSizeHint().height())));

			delete element;
		}

			// The wheel over a spin box without focus scrolls the dock
		void wheelScrollsOverFields()
		{
			QMainWindow window;
			auto *dock = new PropertiesEditorDockWidget(&window);
			window.addDockWidget(Qt::RightDockWidgetArea, dock);
			window.setCentralWidget(new QWidget(&window));
			window.resize(600, 300);
			window.show();
			QVERIFY(QTest::qWaitForWindowExposed(&window));
			auto *editor = new PropertiesEditorWidget;
			auto *layout = new QVBoxLayout(editor);
			auto *spin = new QSpinBox(editor);
			spin->setRange(0, 100);
			spin->setValue(50);
			layout->addWidget(spin);
			editor->setMinimumHeight(800);
			dock->addEditor(editor);
			QApplication::processEvents();
			QTest::qWait(50);
			QCOMPARE(spin->focusPolicy(), Qt::StrongFocus);
			QWheelEvent wheel(QPointF(5, 5), spin->mapToGlobal(QPointF(5, 5)), QPoint(),
					  QPoint(0, -120), Qt::NoButton, Qt::NoModifier,
					  Qt::NoScrollPhase, false);
			QCoreApplication::sendEvent(spin, &wheel);
			QCOMPARE(spin->value(), 50);
			auto *scroll = dock->findChild<QScrollArea *>(QStringLiteral("m_scroll_area"));
			QVERIFY(scroll->verticalScrollBar()->value() > 0);
		}
};

QET_APP_TEST_MAIN(tst_propertiesdockscroll)

#include "tst_propertiesdockscroll.moc"
