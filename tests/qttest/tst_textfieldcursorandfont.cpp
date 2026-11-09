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
	void testPastedHtmlDarkModeCleaning();
	void testSelectAllChangeFontSize();
	void testMultiLineDefaultProjectFontNoFreeze();
	void testTextColorApplyToAllFragments();
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
	QAction *bgColorAction = nullptr;
	QAction *textColorAction = nullptr;
	for (QAction *action : toolbar->actions()) {
		if (action->text() == tr("Default project font") ||
			action->toolTip() == tr("Default project font")) {
			defaultFontAction = action;
		}
		if (action->text() == tr("Text Background Color") ||
			action->toolTip() == tr("Text Background Color")) {
			bgColorAction = action;
		}
		if (action->text() == tr("Text Color") ||
			action->toolTip() == tr("Text Color")) {
			textColorAction = action;
		}
	}
	QVERIFY(defaultFontAction != nullptr);
	QVERIFY(bgColorAction != nullptr);
	QVERIFY(textColorAction != nullptr);
	QVERIFY(!bgColorAction->icon().isNull());
	QVERIFY(!textColorAction->icon().isNull());
}

void tst_TextFieldCursorAndFont::testDefaultProjectFontUnification()
{
	qdesigner_internal::RichTextEditorDialog dialog;

	QFont defaultFont = QETApp::indiTextsItemFont();

	// Set initial rich text with disparate fonts, heading margins, and background highlight
	QString initialHtml = QString(
		"<h1><span style=\"font-family:'Courier New'; font-size:18pt;\">Heading</span></h1>"
		"<p><span style=\"background-color:#ffff00; font-family:'Times New Roman'; font-size:10pt;\">Body text</span></p>");
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

	// Disparate margins from heading/paragraphs are cleared to standard spacing
	QVERIFY(!updatedText.contains("margin-top:18px"));
	QVERIFY(!updatedText.contains("margin-bottom:12px"));

	// Background color highlight is cleared
	QVERIFY(!updatedText.contains("background-color"));

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

	// Verify all blocks now have 0 margins
	for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next()) {
		QCOMPARE(block.blockFormat().topMargin(), 0.0);
		QCOMPARE(block.blockFormat().bottomMargin(), 0.0);
		QCOMPARE(block.blockFormat().leftMargin(), 0.0);
		QCOMPARE(block.blockFormat().rightMargin(), 0.0);
		QCOMPARE(block.blockFormat().indent(), 0);
	}

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

void tst_TextFieldCursorAndFont::testPastedHtmlDarkModeCleaning()
{
	qdesigner_internal::RichTextEditorDialog dialog;

	const QString darkWebSnippet = QStringLiteral(
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
		"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
		"p, li { white-space: pre-wrap; }\n"
		"hr { height: 1px; border-width: 0; }\n"
		"li.unchecked::marker { content: \"\\2610\"; }\n"
		"li.checked::marker { content: \"\\2612\"; }\n"
		"</style></head><body style=\" font-family:'Liberation Sans'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
		"<h1 style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; line-height:135%; background-color:#333333;\"><span style=\" font-family:'Open Sans','Verdana','Arial','sans-serif'; font-size:xx-large; font-weight:700; color:#ffffff; background-color:#333333;\">E-Rechnung ansehen</span></h1>\n"
		"<h2 style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; line-height:135%; background-color:#333333;\"><a name=\"eRechnung-E-Rechnungauswählen\"></a><span style=\" font-family:'Open Sans','Verdana','Arial','sans-serif'; font-size:x-large; font-weight:700; color:#ffffff; background-color:#333333;\">E</span><span style=\" font-family:'Open Sans','Verdana','Arial','sans-serif'; font-size:x-large; font-weight:700; color:#ffffff; background-color:#333333;\">-Rechnung auswählen</span></h2>\n"
		"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Open Sans','Verdana','Arial','sans-serif'; font-size:16px; color:#ffffff; background-color:#333333;\">Sie können hier Ihre E-Rechnung ansehen. Nutzen Sie dazu das Auswahlfeld.</span></p>\n"
		"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Open Sans','Verdana','Arial','sans-serif'; font-size:16px; color:#ffffff; background-color:#333333;\">Dies ist ein unverbindlicher Service der Steuer</span></p></body></html>"
	);

	dialog.setText(darkWebSnippet);

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

	// Triggering default font action must clean dark background, reset spacing and font color
	defaultFontAction->trigger();

	QString updatedHtml = dialog.text(Qt::RichText);
	QVERIFY(!updatedHtml.contains("#333333"));
	QVERIFY(!updatedHtml.contains("background-color"));
	QVERIFY(!updatedHtml.contains("color:#ffffff"));

	const auto textEdits = dialog.findChildren<QTextEdit *>();
	QTextEdit *editor = nullptr;
	for (auto *te : textEdits) {
		if (te->acceptRichText()) {
			editor = te;
			break;
		}
	}
	QVERIFY(editor != nullptr);

	// All blocks should have margins reset to 0
	for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next()) {
		QCOMPARE(block.blockFormat().topMargin(), 0.0);
		QCOMPARE(block.blockFormat().bottomMargin(), 0.0);
		QCOMPARE(block.blockFormat().headingLevel(), 0);
		QCOMPARE(block.blockFormat().background().style(), Qt::NoBrush);

		// All character fragments must have transparent background and non-white foreground
		for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
			QTextFragment frag = it.fragment();
			if (frag.isValid()) {
				QCOMPARE(frag.charFormat().background().style(), Qt::NoBrush);
				QVERIFY(frag.charFormat().foreground().color() != Qt::white);
			}
		}
	}
}

void tst_TextFieldCursorAndFont::testSelectAllChangeFontSize()
{
	qdesigner_internal::RichTextEditorDialog dialog;

	const QString darkWebSnippet = QStringLiteral(
		"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
		"<html><head><meta name=\"qrichtext\" content=\"1\" /><meta charset=\"utf-8\" /><style type=\"text/css\">\n"
		"p, li { white-space: pre-wrap; }\n"
		"hr { height: 1px; border-width: 0; }\n"
		"li.unchecked::marker { content: \"\\2610\"; }\n"
		"li.checked::marker { content: \"\\2612\"; }\n"
		"</style></head><body style=\" font-family:'Liberation Sans'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
		"<h1 style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; line-height:135%; background-color:#333333;\"><span style=\" font-family:'Open Sans','Verdana','Arial','sans-serif'; font-size:xx-large; font-weight:700; color:#ffffff; background-color:#333333;\">E-Rechnung ansehen</span></h1>\n"
		"<h2 style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; line-height:135%; background-color:#333333;\"><a name=\"eRechnung-E-Rechnungauswählen\"></a><span style=\" font-family:'Open Sans','Verdana','Arial','sans-serif'; font-size:x-large; font-weight:700; color:#ffffff; background-color:#333333;\">E</span><span style=\" font-family:'Open Sans','Verdana','Arial','sans-serif'; font-size:x-large; font-weight:700; color:#ffffff; background-color:#333333;\">-Rechnung auswählen</span></h2>\n"
		"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Open Sans','Verdana','Arial','sans-serif'; font-size:16px; color:#ffffff; background-color:#333333;\">Sie können hier Ihre E-Rechnung ansehen. Nutzen Sie dazu das Auswahlfeld.</span></p>\n"
		"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Open Sans','Verdana','Arial','sans-serif'; font-size:16px; color:#ffffff; background-color:#333333;\">Dies ist ein unverbindlicher Service der Steuer</span></p></body></html>"
	);

	dialog.setText(darkWebSnippet);

	const auto textEdits = dialog.findChildren<QTextEdit *>();
	QTextEdit *editor = nullptr;
	for (auto *te : textEdits) {
		if (te->acceptRichText()) {
			editor = te;
			break;
		}
	}
	QVERIFY(editor != nullptr);

	editor->selectAll();

	auto *toolbar = dialog.findChild<QToolBar *>();
	QVERIFY(toolbar != nullptr);

	auto *sizeCombo = toolbar->findChild<QComboBox *>("fontSizeCombo");
	if (!sizeCombo) {
		for (auto *cb : toolbar->findChildren<QComboBox *>()) {
			if (cb->findText("36") != -1) {
				sizeCombo = cb;
				break;
			}
		}
	}
	QVERIFY(sizeCombo != nullptr);

	int idx36 = sizeCombo->findText("36");
	QVERIFY(idx36 != -1);
	sizeCombo->setCurrentIndex(idx36);
	QMetaObject::invokeMethod(toolbar, "sizeInputActivated", Q_ARG(QString, QStringLiteral("36")));

	for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next()) {
		QCOMPARE(block.blockFormat().headingLevel(), 0);
		for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
			QTextFragment frag = it.fragment();
			if (frag.isValid()) {
				QCOMPARE(frag.charFormat().fontPointSize(), 36.0);
				QCOMPARE(frag.charFormat().font().pointSize(), 36);
				QVERIFY(!frag.charFormat().hasProperty(QTextFormat::FontSizeAdjustment));
				QVERIFY(!frag.charFormat().hasProperty(QTextFormat::FontPixelSize));
			}
		}
	}
}

void tst_TextFieldCursorAndFont::testMultiLineDefaultProjectFontNoFreeze()
{
	qdesigner_internal::RichTextEditorDialog dialog;

	const QString multiLineText = QStringLiteral(
		"<p><span style=\"font-size:36pt;\">TEST – Schrift</span></p>\n"
		"<p><span style=\"font-size:24pt;\">TEST – Schrift</span></p>\n"
		"<p><span style=\"font-size:18pt;\">TEST – Schrift</span></p>\n"
		"<p><span style=\"font-size:12pt;\">TEST – Schrift</span></p>\n"
		"<p><span style=\"font-size:9pt;\">TEST – Schrift</span></p>"
	);

	dialog.setText(multiLineText);

	const auto textEdits = dialog.findChildren<QTextEdit *>();
	QTextEdit *editor = nullptr;
	for (auto *te : textEdits) {
		if (te->acceptRichText()) {
			editor = te;
			break;
		}
	}
	QVERIFY(editor != nullptr);

	editor->selectAll();

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

	// Triggering default font action must complete immediately without hanging or freezing
	defaultFontAction->trigger();

	for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next()) {
		QCOMPARE(block.blockFormat().headingLevel(), 0);
		QCOMPARE(block.blockFormat().topMargin(), 0.0);
		QCOMPARE(block.blockFormat().bottomMargin(), 0.0);
	}
}

void tst_TextFieldCursorAndFont::testTextColorApplyToAllFragments()
{
	qdesigner_internal::RichTextEditorDialog dialog;

	QString snippet = QStringLiteral(
		"<p>Symbol-Editor: Zeichnung wie im Folio</p>\n"
		"<p><a href=\"https://github.com/DaTweety\">DaTweety</a></p>\n"
		"<p><span style=\"color:#1a7f37;\">+ 963</span><span style=\"color:#cf222e;\">- 59</span></p>\n"
		"<ul><li><a href=\"https://github.com/qelectrotech/qelectrotech-source-mirror/pull/1796\">Überblick</a></li></ul>"
	);

	dialog.setText(snippet);

	const auto textEdits = dialog.findChildren<QTextEdit *>();
	QTextEdit *editor = nullptr;
	for (auto *te : textEdits) {
		if (te->acceptRichText()) {
			editor = te;
			break;
		}
	}
	QVERIFY(editor != nullptr);

	editor->selectAll();

	auto *toolbar = dialog.findChild<QToolBar *>();
	QVERIFY(toolbar != nullptr);

	QAction *textColorAction = nullptr;
	for (QAction *action : toolbar->actions()) {
		if (action->text() == tr("Text Color") || action->toolTip() == tr("Text Color")) {
			textColorAction = action;
			break;
		}
	}
	QVERIFY(textColorAction != nullptr);

	QMetaObject::invokeMethod(toolbar, "colorChanged", Q_ARG(QColor, QColor(Qt::black)));

	for (QTextBlock block = editor->document()->begin(); block.isValid(); block = block.next()) {
		for (QTextBlock::iterator it = block.begin(); !it.atEnd(); ++it) {
			QTextFragment frag = it.fragment();
			if (frag.isValid()) {
				QCOMPARE(frag.charFormat().foreground().color(), QColor(Qt::black));
				QVERIFY(!frag.charFormat().isAnchor());
			}
		}
	}
}

QET_APP_TEST_MAIN(tst_TextFieldCursorAndFont)
#include "tst_textfieldcursorandfont.moc"

