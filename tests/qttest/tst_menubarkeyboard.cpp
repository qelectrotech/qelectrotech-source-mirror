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
#include <QtTest>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QShortcut>

#ifdef Q_OS_MACOS
// Exported from QtGui but declared in qkeysequence.h only for the
// documentation build; Qt's own docs say to declare it like this.
Q_GUI_EXPORT void qt_set_sequence_auto_mnemonic(bool b);
#endif

/*
	F10 should open the menu bar, as it does in most applications and as
	someone working without a mouse will expect. Qt provides this on Windows
	but not on X11, so QElectroTech adds it (QETMainWindow::activateMenuBar).

	These tests use QTest rather than driving a real X server. That is not a
	convenience: xdotool on Xvfb delivers every function key with Alt held, so
	the application receives Alt+F10 and never the plain key. Two rounds of
	GUI automation gave confident, wrong answers about F10 before that was
	understood. QTest posts the event directly to the widget, so the key
	arrives exactly as written.
*/
class TstMenuBarKeyboard : public QObject
{
	Q_OBJECT

	private slots:
		void initTestCase();
		void altLetterOpensMenu();      // control -- must pass, or nothing below means anything
		void plainF10DoesNothingInQt(); // the gap being filled
		void shortcutOpensMenuBar();    // the mechanism QETMainWindow uses
};

namespace {

QMainWindow *makeWindow(QMenu **file_menu)
{
	auto *w = new QMainWindow;
#ifdef Q_OS_MACOS
	// A QMenuBar is native on macOS: its menus live in the system menu
	// bar, outside the Qt widget tree, where QTest key events never reach
	// them and QMenu::isVisible() stays false. The in-window bar runs the
	// same QMenuBar code the other platforms use.
	w->menuBar()->setNativeMenuBar(false);
#endif
	*file_menu = w->menuBar()->addMenu(QStringLiteral("&File"));
	(*file_menu)->addAction(QStringLiteral("Quit"));
	w->menuBar()->addMenu(QStringLiteral("&Edit"))->addAction(QStringLiteral("Copy"));
	w->resize(600, 400);
	w->show();
	w->activateWindow();
	w->raise();
	return w;
}

}

/*
	macOS has no Alt+letter menu mnemonics, so Qt does not turn "&File"
	into a shortcut there (qt_set_sequence_auto_mnemonic is off). The
	control below needs one; switch mnemonics on for this process.
*/
void TstMenuBarKeyboard::initTestCase()
{
#ifdef Q_OS_MACOS
	qt_set_sequence_auto_mnemonic(true);
#endif
}

/*
	The control. Alt and a menu's letter is known to work, so if this fails
	the environment cannot open menus at all and the other two tests are
	measuring nothing.
*/
void TstMenuBarKeyboard::altLetterOpensMenu()
{
	QMenu *file = nullptr;
	QScopedPointer<QMainWindow> w(makeWindow(&file));
	QVERIFY(QTest::qWaitForWindowExposed(w.data()));

	QTest::keyClick(w.data(), Qt::Key_F, Qt::AltModifier);
	QTest::qWait(300);

	QVERIFY2(file->isVisible(),
		 "control failed: Alt+F did not open a menu, so this environment "
		 "cannot judge any of the keyboard tests below");
}

/*
	Records why the shortcut in QETMainWindow exists. If a future Qt starts
	handling F10 on this platform, this test fails and the shortcut can go.
*/
void TstMenuBarKeyboard::plainF10DoesNothingInQt()
{
	QMenu *file = nullptr;
	QScopedPointer<QMainWindow> w(makeWindow(&file));
	QVERIFY(QTest::qWaitForWindowExposed(w.data()));

	QTest::keyClick(w.data(), Qt::Key_F10, Qt::NoModifier);
	QTest::qWait(300);

	QVERIFY2(!file->isVisible() && w->menuBar()->activeAction() == nullptr,
		 "Qt now handles F10 by itself -- QETMainWindow's shortcut is "
		 "redundant and can be removed");
}

/*
	The mechanism QETMainWindow uses, exercised on a plain QMainWindow.

	Worth being clear about what this does not cover: it repeats the shortcut
	wiring rather than driving QETMainWindow itself, because
	initCommonActions() calls QETApp::instance() and constructing that pulls
	in the whole application -- element collections and all -- which does not
	belong in a unit test. So this proves the approach works and would catch
	it breaking in a future Qt; it does not prove QETMainWindow is wired up.
	That last step needs someone to press F10 in a running QElectroTech.
*/
void TstMenuBarKeyboard::shortcutOpensMenuBar()
{
	QMenu *file = nullptr;
	QScopedPointer<QMainWindow> holder(makeWindow(&file));
	QMainWindow *w = holder.data();
	QVERIFY(QTest::qWaitForWindowExposed(w));

	auto *shortcut = new QShortcut(QKeySequence(Qt::Key_F10), w);
	shortcut->setContext(Qt::WindowShortcut);
	QObject::connect(shortcut, &QShortcut::activated, w, [w]() {
		for (QAction *action : w->menuBar()->actions()) {
			if (action->isVisible() && action->isEnabled() && action->menu()) {
				w->menuBar()->setActiveAction(action);
				return;
			}
		}
	});

	QTest::keyClick(w, Qt::Key_F10, Qt::NoModifier);
	QTest::qWait(300);

	QVERIFY2(w->menuBar()->activeAction() != nullptr,
		 "F10 did not activate the menu bar");
	QCOMPARE(w->menuBar()->activeAction()->text(), QStringLiteral("&File"));
}

QTEST_MAIN(TstMenuBarKeyboard)
#include "tst_menubarkeyboard.moc"
