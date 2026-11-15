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
#include "diagram.h"
#include "diagramcommands.h"
#include "diagramevent/diagrameventaddpaste.h"
#include "diagramview.h"
#include "qetapptest.h"
#include "qetdiagrameditor.h"
#include "qetgraphicsitem/qetshapeitem.h"
#include "qetproject.h"
#include "shortcutmanager.h"

#include <QGraphicsLineItem>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QTest>

class tst_pasteaxislock : public QObject
{
	Q_OBJECT

  private slots:
	void testShortcutRegistration();
	void testAxisLockAndOriginalPositionSnap();
	void testCustomShortcut();
	void testMenuPasteFocusAndKeyRouting();
	void testInPlaceAxisLock();
	void testConflictingShortcutOverride();
};

void tst_pasteaxislock::testShortcutRegistration()
{
	QETDiagramEditor* editor = QetAppTest::editor();
	QVERIFY(editor != nullptr);
	const QKeySequence x_seq = ShortcutManager::instance().currentSequence(
		QStringLiteral("diagrameditor.paste_lock_x"));
	const QKeySequence y_seq = ShortcutManager::instance().currentSequence(
		QStringLiteral("diagrameditor.paste_lock_y"));
	QCOMPARE(x_seq, QKeySequence(Qt::Key_X));
	QCOMPARE(y_seq, QKeySequence(Qt::Key_Y));
}

void tst_pasteaxislock::testAxisLockAndOriginalPositionSnap()
{
	Diagram* diagram = QetAppTest::diagram();
	QVERIFY(diagram != nullptr);
	DiagramView* view = QetAppTest::view();
	QVERIFY(view != nullptr);

	// Create an item at an exact known position: (100, 100)
	auto* rect = new QetShapeItem(
		QPointF(0, 0),
		QPointF(40, 40),
		QetShapeItem::Rectangle);
	rect->setPos(100, 100);
	diagram->addItem(rect);
	QCOMPARE(rect->pos(), QPointF(100, 100));

	// Copy to clipboard
	diagram->clearSelection();
	rect->setSelected(true);
	view->copy();
	QVERIFY(DiagramEventAddPaste::clipboardHasDiagram());

	// Start paste in UnderCursor placement at (200, 200)
	const QPointF start_pos(200, 200);
	auto*		  paste = new DiagramEventAddPaste(
		diagram,
		start_pos,
		DiagramEventAddPaste::UnderCursor);
	diagram->setEventInterface(paste);

	// Get the pasted item (the one that is currently selected and not the
	// original)
	QList<QGraphicsItem*> selected = diagram->selectedItems();
	QVERIFY(! selected.isEmpty());
	QGraphicsItem* pasted = nullptr;
	for (QGraphicsItem* item : selected)
	{
		if (item != rect)
		{
			pasted = item;
			break;
		}
	}
	QVERIFY(pasted != nullptr);
	const QPointF base_pos = pasted->pos();

	auto hasDashedLine = [diagram](Qt::Orientation orientation) -> bool {
		for (auto *item : diagram->items()) {
			if (auto *line = qgraphicsitem_cast<QGraphicsLineItem *>(item)) {
				if (line->isVisible() && line->pen().style() == Qt::DashLine) {
					const QLineF l = line->line();
					if (orientation == Qt::Vertical && qFuzzyCompare(l.x1(), l.x2())) {
						return true;
					}
					if (orientation == Qt::Horizontal && qFuzzyCompare(l.y1(), l.y2())) {
						return true;
					}
				}
			}
		}
		return false;
	};

	// 1. Move to (300, 400) without locks -> no dashed lines
	QGraphicsSceneMouseEvent move1(QEvent::GraphicsSceneMouseMove);
	move1.setScenePos(QPointF(300, 400));
	paste->mouseMoveEvent(&move1);
	// Delta from start_pos (200, 200) is (+100, +200)
	QCOMPARE(pasted->pos(), base_pos + QPointF(100, 200));
	QVERIFY(!hasDashedLine(Qt::Vertical));
	QVERIFY(!hasDashedLine(Qt::Horizontal));

	// 2. Press X: locks X axis to original X (100) -> dashed line along Y axis (vertical)
	QKeyEvent key_x(QEvent::KeyPress, Qt::Key_X, Qt::NoModifier);
	paste->keyPressEvent(&key_x);
	QVERIFY(key_x.isAccepted());
	QCOMPARE(pasted->pos().x(), 100.0);
	QVERIFY(hasDashedLine(Qt::Vertical));
	QVERIFY(!hasDashedLine(Qt::Horizontal));

	// Move mouse to (500, 600) -> X must stay at original 100, Y moves to 600 (deltaY = +400)
	QGraphicsSceneMouseEvent move2(QEvent::GraphicsSceneMouseMove);
	move2.setScenePos(QPointF(500, 600));
	paste->mouseMoveEvent(&move2);
	QCOMPARE(pasted->pos().x(), 100.0);
	QCOMPARE(pasted->pos().y(), base_pos.y() + 400);
	QVERIFY(hasDashedLine(Qt::Vertical));

	// 3. Press Y: now BOTH X and Y are locked! Must snap to original position (100, 100)
	// and show dashed lines in both directions (cross)
	QKeyEvent key_y(QEvent::KeyPress, Qt::Key_Y, Qt::NoModifier);
	paste->keyPressEvent(&key_y);
	QVERIFY(key_y.isAccepted());
	QCOMPARE(pasted->pos(), QPointF(100, 100));
	QVERIFY(hasDashedLine(Qt::Vertical));
	QVERIFY(hasDashedLine(Qt::Horizontal));

	// Moving mouse while both are locked should NOT move the pasted item away
	// from original position
	QGraphicsSceneMouseEvent move3(QEvent::GraphicsSceneMouseMove);
	move3.setScenePos(QPointF(800, 900));
	paste->mouseMoveEvent(&move3);
	QCOMPARE(pasted->pos(), QPointF(100, 100));
	QVERIFY(hasDashedLine(Qt::Vertical));
	QVERIFY(hasDashedLine(Qt::Horizontal));

	// 4. Press X again to unlock X: Y remains locked at original Y (100), X
	// moves to mouse (800 - 200 = +600) -> only horizontal line visible
	QKeyEvent key_x_unlock(QEvent::KeyPress, Qt::Key_X, Qt::NoModifier);
	paste->keyPressEvent(&key_x_unlock);
	QCOMPARE(pasted->pos().x(), base_pos.x() + 600);
	QCOMPARE(pasted->pos().y(), 100.0);
	QVERIFY(!hasDashedLine(Qt::Vertical));
	QVERIFY(hasDashedLine(Qt::Horizontal));

	// Move mouse to (850, 950) -> X follows to (850 - 200 = +650), Y stays at 100
	QGraphicsSceneMouseEvent move4(QEvent::GraphicsSceneMouseMove);
	move4.setScenePos(QPointF(850, 950));
	paste->mouseMoveEvent(&move4);
	QCOMPARE(pasted->pos().x(), base_pos.x() + 650);
	QCOMPARE(pasted->pos().y(), 100.0);

	// 5. Press Y again to unlock Y: both axes are free again -> no dashed lines
	QKeyEvent key_y_unlock(QEvent::KeyPress, Qt::Key_Y, Qt::NoModifier);
	paste->keyPressEvent(&key_y_unlock);
	QCOMPARE(pasted->pos(), base_pos + QPointF(650, 750));
	QVERIFY(!hasDashedLine(Qt::Vertical));
	QVERIFY(!hasDashedLine(Qt::Horizontal));

	// 6. Test Y then X:
	// Press Y to lock Y to original Y (100) -> horizontal dashed line visible
	QKeyEvent key_y2(QEvent::KeyPress, Qt::Key_Y, Qt::NoModifier);
	paste->keyPressEvent(&key_y2);
	QCOMPARE(pasted->pos().y(), 100.0);
	QVERIFY(!hasDashedLine(Qt::Vertical));
	QVERIFY(hasDashedLine(Qt::Horizontal));

	QGraphicsSceneMouseEvent move5(QEvent::GraphicsSceneMouseMove);
	move5.setScenePos(QPointF(600, 400));
	paste->mouseMoveEvent(&move5);
	// deltaX = 600 - 200 = +400, Y locked at original 100
	QCOMPARE(pasted->pos().x(), base_pos.x() + 400);
	QCOMPARE(pasted->pos().y(), 100.0);

	// Now press X -> both locked -> snap to original position (100, 100) -> both dashed lines visible
	QKeyEvent key_x2(QEvent::KeyPress, Qt::Key_X, Qt::NoModifier);
	paste->keyPressEvent(&key_x2);
	QCOMPARE(pasted->pos(), QPointF(100, 100));
	QVERIFY(hasDashedLine(Qt::Vertical));
	QVERIFY(hasDashedLine(Qt::Horizontal));

	// Commit paste using Enter -> dashed lines must disappear
	QKeyEvent key_enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
	paste->keyPressEvent(&key_enter);
	diagram->clearEventInterface();

	// Item must be landed at (100, 100) and dashed lines must be gone
	QCOMPARE(pasted->pos(), QPointF(100, 100));
	QCOMPARE(pasted->scene(), diagram);
	QVERIFY(!hasDashedLine(Qt::Vertical));
	QVERIFY(!hasDashedLine(Qt::Horizontal));
}

void tst_pasteaxislock::testCustomShortcut()
{
	Diagram *diagram = QetAppTest::diagram();
	QVERIFY(diagram != nullptr);
	DiagramView *view = QetAppTest::view();
	QVERIFY(view != nullptr);

	auto *rect = new QetShapeItem(QPointF(0, 0), QPointF(40, 40), QetShapeItem::Rectangle);
	rect->setPos(150, 150);
	diagram->addItem(rect);
	rect->setSelected(true);
	view->copy();

	// Set custom shortcut for X axis lock to 'H'
	ShortcutManager::instance().setSequence(
		QStringLiteral("diagrameditor.paste_lock_x"), QKeySequence(Qt::Key_H));
	QCOMPARE(ShortcutManager::instance().currentSequence(
		QStringLiteral("diagrameditor.paste_lock_x")), QKeySequence(Qt::Key_H));

	auto *paste = new DiagramEventAddPaste(diagram, QPointF(200, 200), DiagramEventAddPaste::UnderCursor);
	diagram->setEventInterface(paste);

	// Press X: should NOT lock because shortcut was changed to H
	QKeyEvent key_x(QEvent::KeyPress, Qt::Key_X, Qt::NoModifier);
	key_x.setAccepted(false);
	paste->keyPressEvent(&key_x);
	QVERIFY(!key_x.isAccepted());

	// Press H: SHOULD lock X axis
	QKeyEvent key_h(QEvent::KeyPress, Qt::Key_H, Qt::NoModifier);
	key_h.setAccepted(false);
	paste->keyPressEvent(&key_h);
	QVERIFY(key_h.isAccepted());

	// Reset to default
	ShortcutManager::instance().resetToDefault(QStringLiteral("diagrameditor.paste_lock_x"));
	QCOMPARE(ShortcutManager::instance().currentSequence(
		QStringLiteral("diagrameditor.paste_lock_x")), QKeySequence(Qt::Key_X));

	QKeyEvent cancel(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
	paste->keyPressEvent(&cancel);
	diagram->clearEventInterface();
}

void tst_pasteaxislock::testMenuPasteFocusAndKeyRouting()
{
	QETDiagramEditor *editor = QetAppTest::editor();
	QVERIFY(editor != nullptr);
	Diagram *diagram = QetAppTest::diagram();
	QVERIFY(diagram != nullptr);
	DiagramView *view = QetAppTest::view();
	QVERIFY(view != nullptr);

	editor->show();
	QVERIFY(QTest::qWaitForWindowExposed(editor));

	auto *rect = new QetShapeItem(QPointF(0, 0), QPointF(40, 40), QetShapeItem::Rectangle);
	rect->setPos(120, 140);
	diagram->addItem(rect);
	diagram->clearSelection();
	rect->setSelected(true);
	view->copy();

	// Simulate focus being elsewhere (e.g. tab bar or project panel) before Edit -> Paste
	view->clearFocus();
	editor->setFocus();
	QVERIFY(!view->hasFocus());

	// Trigger the Edit -> Paste action directly (as if clicked from the menu)
	QAction *paste_action = ShortcutManager::instance().action(QStringLiteral("diagrameditor.paste"), editor);
	QVERIFY(paste_action != nullptr);
	QVERIFY(paste_action->isEnabled());
	paste_action->trigger();

	// DiagramView must now have keyboard focus and DiagramEventAddPaste must be running
	QVERIFY(view->hasFocus());
	QVERIFY(diagram->eventInterfaceIsRunning());

	QList<QGraphicsItem *> selected = diagram->selectedItems();
	QVERIFY(!selected.isEmpty());
	QGraphicsItem *pasted = nullptr;
	for (QGraphicsItem *item : selected) {
		if (item != rect) {
			pasted = item;
			break;
		}
	}
	QVERIFY(pasted != nullptr);

	// Send X and Y key events to whichever widget currently has focus
	QWidget *focused = QApplication::focusWidget();
	QVERIFY(focused == view || view->isAncestorOf(focused));
	QTest::keyClick(focused, Qt::Key_X);
	QCOMPARE(pasted->pos().x(), 120.0);
	QTest::keyClick(focused, Qt::Key_Y);
	QCOMPARE(pasted->pos(), QPointF(120.0, 140.0));

	QTest::keyClick(focused, Qt::Key_Escape);
	QCoreApplication::processEvents();
	QVERIFY(!diagram->eventInterfaceIsRunning());
}

void tst_pasteaxislock::testInPlaceAxisLock()
{
	Diagram *diagram = QetAppTest::diagram();
	QVERIFY(diagram != nullptr);
	DiagramView *view = QetAppTest::view();
	QVERIFY(view != nullptr);

	auto *rect = new QetShapeItem(QPointF(0, 0), QPointF(40, 40), QetShapeItem::Rectangle);
	rect->setPos(100, 100);
	diagram->addItem(rect);
	diagram->clearSelection();
	rect->setSelected(true);

	const QString fragment = diagram->toXml(false, true).toString(4);
	auto *paste = new DiagramEventAddPaste(
		diagram,
		QPointF(300, 300),
		DiagramEventAddPaste::InPlace,
		fragment);
	diagram->setEventInterface(paste);

	QList<QGraphicsItem *> selected = diagram->selectedItems();
	QVERIFY(!selected.isEmpty());
	QGraphicsItem *pasted = nullptr;
	for (QGraphicsItem *item : selected) {
		if (item != rect) {
			pasted = item;
			break;
		}
	}
	QVERIFY(pasted != nullptr);
	QCOMPARE(pasted->pos(), QPointF(100, 100));

	// Drag to (340, 360): delta = (+40, +60) -> (140, 160)
	QGraphicsSceneMouseEvent move1(QEvent::GraphicsSceneMouseMove);
	move1.setScenePos(QPointF(340, 360));
	paste->mouseMoveEvent(&move1);
	QCOMPARE(pasted->pos(), QPointF(140, 160));

	// Press X: X locks to original (100), Y remains at 160
	QKeyEvent key_x(QEvent::KeyPress, Qt::Key_X, Qt::NoModifier);
	paste->keyPressEvent(&key_x);
	QVERIFY(key_x.isAccepted());
	QCOMPARE(pasted->pos(), QPointF(100, 160));

	// Press Y: both locked -> snaps to original (100, 100)
	QKeyEvent key_y(QEvent::KeyPress, Qt::Key_Y, Qt::NoModifier);
	paste->keyPressEvent(&key_y);
	QVERIFY(key_y.isAccepted());
	QCOMPARE(pasted->pos(), QPointF(100, 100));

	// Commit with Return -> remains at (100, 100)
	QKeyEvent key_enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
	paste->keyPressEvent(&key_enter);
	diagram->clearEventInterface();
	QCOMPARE(pasted->pos(), QPointF(100, 100));
}

void tst_pasteaxislock::testConflictingShortcutOverride()
{
	QETDiagramEditor *editor = QetAppTest::editor();
	QVERIFY(editor != nullptr);
	Diagram *diagram = QetAppTest::diagram();
	QVERIFY(diagram != nullptr);
	DiagramView *view = QetAppTest::view();
	QVERIFY(view != nullptr);

	editor->show();
	QVERIFY(QTest::qWaitForWindowExposed(editor));

	auto *rect = new QetShapeItem(QPointF(0, 0), QPointF(40, 40), QetShapeItem::Rectangle);
	rect->setPos(100, 100);
	diagram->addItem(rect);
	diagram->clearSelection();
	rect->setSelected(true);
	view->copy();

	// Rebind lock X to 'M', which collides with a global QAction in QETDiagramEditor
	ShortcutManager::instance().setSequence(
		QStringLiteral("diagrameditor.paste_lock_x"), QKeySequence(Qt::Key_M));

	QAction *paste_action = ShortcutManager::instance().action(QStringLiteral("diagrameditor.paste"), editor);
	QVERIFY(paste_action != nullptr);
	paste_action->trigger();
	QVERIFY(diagram->eventInterfaceIsRunning());

	QList<QGraphicsItem *> selected = diagram->selectedItems();
	QVERIFY(!selected.isEmpty());
	QGraphicsItem *pasted = nullptr;
	for (QGraphicsItem *item : selected) {
		if (item != rect) {
			pasted = item;
			break;
		}
	}
	QVERIFY(pasted != nullptr);

	// Pressing M while paste placement is active must override the global shortcut
	// and toggle the X axis lock instead
	QWidget *focused = QApplication::focusWidget();
	QVERIFY(focused != nullptr);
	QTest::keyClick(focused, Qt::Key_M);
	QVERIFY(diagram->eventInterfaceIsRunning());
	QCOMPARE(pasted->pos().x(), 100.0);

	QTest::keyClick(focused, Qt::Key_Escape);
	QCoreApplication::processEvents();
	QVERIFY(!diagram->eventInterfaceIsRunning());

	ShortcutManager::instance().resetToDefault(QStringLiteral("diagrameditor.paste_lock_x"));
}

QET_APP_TEST_MAIN(tst_pasteaxislock)
#include "tst_pasteaxislock.moc"
