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
#include "diagram.h"
#include "diagramview.h"
#include "qetapp.h"
#include "qetproject.h"
#include "diagramevent/diagrameventaddtext.h"
#include "richtext/richtexteditor_p.h"

#include <QAction>
#include <QApplication>
#include <QFontComboBox>
#include <QKeyEvent>
#include <QStandardPaths>
#include <QTextCursor>
#include <QToolBar>
#include <QtTest>

class tst_TextFieldCursorAndFont : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase();
	void testTextCursorProperties();
	void testAddTextEventCursorAndEscape();
	void testRichTextEditorFontControls();
	void testDefaultProjectFontUnification();
};

void tst_TextFieldCursorAndFont::initTestCase()
{
	QStandardPaths::setTestModeEnabled(true);
}

void tst_TextFieldCursorAndFont::testTextCursorProperties()
{
	QCursor cursor = DiagramEventAddText::textCursor();
	QCOMPARE(cursor.hotSpot(), QPoint(0, 0));
	QPixmap pixmap = cursor.pixmap();
	QVERIFY(!pixmap.isNull());
	QCOMPARE(pixmap.width(), 32);
	QCOMPARE(pixmap.height(), 32);
}

void tst_TextFieldCursorAndFont::testAddTextEventCursorAndEscape()
{
	DiagramView *view = QetAppTest::view();
	QVERIFY(view);
	Diagram *diagram = view->diagram();
	QVERIFY(diagram);

	// Initially, viewport cursor is unset
	QVERIFY(view->viewport()->cursor().shape() == Qt::ArrowCursor);

	// Arm the textfield tool
	auto *event = new DiagramEventAddText(diagram);
	diagram->setEventInterface(event);
	QVERIFY(diagram->eventInterfaceIsRunning());

	// Viewport cursor must now be the custom text cursor
	QCOMPARE(view->viewport()->cursor().hotSpot(), QPoint(0, 0));
	QCOMPARE(view->viewport()->cursor().pixmap().size(), QSize(32, 32));

	// Pressing Escape should cancel the tool and reset the cursor
	QKeyEvent escapeEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
	QCoreApplication::sendEvent(view, &escapeEvent);

	QVERIFY(!diagram->eventInterfaceIsRunning());
}

void tst_TextFieldCursorAndFont::testRichTextEditorFontControls()
{
	qdesigner_internal::RichTextEditorDialog dialog;

	auto *fontCombo = dialog.findChild<QFontComboBox *>();
	QVERIFY(fontCombo != nullptr);

	auto *toolbar = dialog.findChild<QToolBar *>();
	QVERIFY(toolbar != nullptr);

	QAction *defaultFontAction = nullptr;
	for (QAction *action : toolbar->actions()) {
		if (action->text() == tr("Default project font") ||
			action->toolTip() == tr("Default project font")) {
			defaultFontAction = action;
			break;
		}
	}
	QVERIFY(defaultFontAction != nullptr);
}

void tst_TextFieldCursorAndFont::testDefaultProjectFontUnification()
{
	qdesigner_internal::RichTextEditorDialog dialog;

	QFont defaultFont = QETApp::indiTextsItemFont();

	// Set initial rich text with disparate fonts (e.g. copied from external document)
	QString initialHtml = QString(
		"<h1><span style=\"font-family:'Courier New'; font-size:18pt;\">Heading</span></h1>"
		"<p><span style=\"font-family:'Times New Roman'; font-size:10pt;\">Body text</span></p>");
	dialog.setText(initialHtml);

	auto *toolbar = dialog.findChild<QToolBar *>();
	QVERIFY(toolbar != nullptr);

	QAction *defaultFontAction = nullptr;
	for (QAction *action : toolbar->actions()) {
		if (action->text() == tr("Default project font") ||
			action->toolTip() == tr("Default project font")) {
			defaultFontAction = action;
			break;
		}
	}
	QVERIFY(defaultFontAction != nullptr);

	// Trigger default font action without selection -> unifies whole document
	defaultFontAction->trigger();

	QString updatedText = dialog.text(Qt::RichText);
	// Disparate font families should no longer be present because they were unified
	QVERIFY(!updatedText.contains("Courier New"));
	QVERIFY(!updatedText.contains("Times New Roman"));

	// Heading and size formatting are preserved
	QVERIFY(updatedText.contains("font-size:18pt"));
	QVERIFY(updatedText.contains("font-size:10pt"));

	// Now select text and set a specific font family via fontCombo
	const auto textEdits = dialog.findChildren<QTextEdit *>();
	QTextEdit *editor = nullptr;
	for (auto *te : textEdits) {
		if (te->acceptRichText()) {
			editor = te;
			break;
		}
	}
	QVERIFY(editor != nullptr);
	QTextCursor cursor = editor->textCursor();
	cursor.select(QTextCursor::Document);
	editor->setTextCursor(cursor);

	auto *fontCombo = dialog.findChild<QFontComboBox *>();
	QVERIFY(fontCombo != nullptr);
	if (fontCombo->count() > 1) {
		int targetIndex = (fontCombo->currentIndex() == 0) ? 1 : 0;
		fontCombo->setCurrentIndex(targetIndex);
		QString chosenFamily = fontCombo->currentFont().family();
		QString textWithChosenFont = dialog.text(Qt::RichText);
		QVERIFY(textWithChosenFont.contains(chosenFamily));
	}
}

QET_APP_TEST_MAIN(tst_TextFieldCursorAndFont)
#include "tst_textfieldcursorandfont.moc"

