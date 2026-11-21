/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "qetapptest.h"
#include "diagram.h"
#include "editor/elementscene.h"
#include "editor/graphicspart/partdynamictextfield.h"
#include "editor/graphicspart/partterminal.h"
#include "editor/graphicspart/parttext.h"
#include "editor/ui/dynamictextfieldeditor.h"
#include "editor/ui/qetelementeditor.h"
#include "editor/ui/terminaleditor.h"
#include "editor/ui/texteditor.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetproject.h"
#include "ui/alignmenttextdialog.h"
#include "ui/compositetexteditdialog.h"
#include "ui/dynamicelementtextmodel.h"
#include "utils/dialogplacement.h"

#include <QElapsedTimer>
#include <QPushButton>
#include <QScopeGuard>
#include <QScreen>
#include <QSettings>
#include <QTimer>
#include <QTreeView>
#include <QWindow>

/**
	The text alignment dialog and the composite text dialog open next to
	what opened them (issue #1952): under their cell of a symbol's Texts
	tab, under the Alignment button of a text, a text field or a terminal
	in the symbol editor, above a button with no room below it, and
	inside the screen under the button.

	CTest runs it offscreen with two screens side by side
	(fixtures/two_screens.json). Run directly, it uses the platform's
	windows and their title bars.
*/
class tst_dialogplacement : public QObject
{
	Q_OBJECT

	static Element *firstSymbol(QETProject &project)
	{
		for (Diagram *diagram : project.diagrams())
			for (Element *element : diagram->elements())
				return element;
		return nullptr;
	}

		//The cell of @p text that edits @p role
	static QModelIndex cellOf(DynamicElementTextModel &model,
							  DynamicElementTextItem *text,
							  DynamicElementTextModel::ValueType role)
	{
		QStandardItem *row = model.itemFromIndex(model.indexFromText(text));
		if (!row)
			return {};
		for (int r = 0; r < row->rowCount(); ++r) {
			QStandardItem *cell = row->child(r, 1);
			if (cell && cell->data(Qt::UserRole + 1).toInt() == role)
				return cell->index();
		}
		return {};
	}

		//@p area of @p widget, on the screen
	static QRect onScreen(const QWidget *widget, const QRect &area)
	{
		return QRect(widget->mapToGlobal(area.topLeft()), area.size());
	}

	static QRect onScreen(const QWidget *widget)
	{
		return onScreen(widget, widget->rect());
	}

		//Just below @p anchor, or just above it, their left edges lined up
	static bool nextTo(const QRect &frame, const QRect &anchor)
	{
		const bool below = frame.top() == anchor.top() + anchor.height();
		const bool above = frame.top() + frame.height() == anchor.top();
		return frame.left() == anchor.left() && (below || above);
	}

		//A window holding one button, put at @p top_left on the screen
	struct ButtonWindow
	{
		QWidget window;
		QPushButton *button = new QPushButton(&window);

		explicit ButtonWindow(const QPoint &top_left)
		{
			window.setGeometry(QRect(top_left, QSize(120, 40)));
			button->setGeometry(10, 5, 100, 30);
			window.show();
		}
	};

	static QRect screenArea()
	{
		return QGuiApplication::primaryScreen()->availableGeometry();
	}

		//Place @p dialog under @p button and show it, as the editors do
	static bool openUnder(AlignmentTextDialog &dialog, QPushButton *button)
	{
		DialogPlacement::placeBelow(&dialog, button);
		dialog.show();
		return QTest::qWaitForWindowExposed(&dialog);
	}

		//Click @p button, which opens the alignment dialog with exec(),
		//and return where the dialog is once on the screen
	static QRect dialogOpenedBy(QPushButton *button)
	{
		QRect frame;
		QElapsedTimer waited;
		waited.start();
		QTimer poll;
		QObject::connect(&poll, &QTimer::timeout, [&]() {
			auto *dialog = qobject_cast<QDialog *>(
						QApplication::activeModalWidget());
			if (!dialog)
				return;
			const bool shown = dialog->windowHandle()
					&& dialog->windowHandle()->isExposed();
			if (!shown && waited.elapsed() < 5000)
				return;
			if (shown && qobject_cast<AlignmentTextDialog *>(dialog))
				frame = dialog->frameGeometry();
			dialog->reject();
		});
		poll.start(10);
		button->click();
		return frame;
	}

	private slots:
		void editorsOpenUnderTheirCell_data()
		{
			QTest::addColumn<int>("role");
			QTest::addColumn<QString>("dialog");
			QTest::newRow("alignment")
					<< int(DynamicElementTextModel::txtAlignment)
					<< QStringLiteral("AlignmentTextDialog");
			QTest::newRow("composite text")
					<< int(DynamicElementTextModel::compositeText)
					<< QStringLiteral("CompositeTextEditDialog");
		}

		void editorsOpenUnderTheirCell()
		{
			QFETCH(int, role);
			QFETCH(QString, dialog);
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			Element *element = firstSymbol(project);
			QVERIFY(element);
			auto *text = new DynamicElementTextItem(element);
			element->addDynamicTextItem(text);
			text->setTextFrom(DynamicElementTextItem::CompositeText);

			DynamicElementTextModel model(element);
			DynamicTextItemDelegate delegate;
			QTreeView view;
			view.setModel(&model);
			view.setItemDelegate(&delegate);
			view.setGeometry(QRect(screenArea().topLeft() + QPoint(100, 50),
								   QSize(400, 250)));
			view.show();
			QVERIFY(QTest::qWaitForWindowExposed(&view));

			const QModelIndex cell = cellOf(
						model, text, DynamicElementTextModel::ValueType(role));
			QVERIFY(cell.isValid());
			view.expandAll();
			view.scrollTo(cell);
			view.edit(cell);

			QWidget *editor = nullptr;
			for (QWidget *w : view.findChildren<QWidget *>())
				if (w->isWindow() && dialog == w->metaObject()->className())
					editor = w;
			QVERIFY2(editor, qPrintable(dialog + " did not open"));
			QVERIFY(QTest::qWaitForWindowExposed(editor));
				//Before the fix the dialog took the cell's position inside
				//the table as a position on the screen
			const QRect rect = onScreen(view.viewport(), view.visualRect(cell));
			QCOMPARE(editor->frameGeometry().topLeft(),
					 QPoint(rect.left(), rect.top() + rect.height()));
		}

		void textAlignmentButton()
		{
			QETElementEditor window;
			ElementScene *scene = window.elementScene();
			const auto reset = qScopeGuard([scene] { scene->reset(); });
			auto *text = new PartText(&window);
			text->setPlainText(QStringLiteral("Text"));
			scene->addItems({text});

			TextEditor editor(&window);
			QVERIFY(editor.setParts({text}));
			editor.move(screenArea().topLeft() + QPoint(100, 50));
			editor.show();
			QVERIFY(QTest::qWaitForWindowExposed(&editor));
			QPushButton *button = nullptr;
			for (QPushButton *b : editor.findChildren<QPushButton *>())
				if (b->text() == QStringLiteral("Alignment"))
					button = b;
			QVERIFY(button);

			const QRect frame = dialogOpenedBy(button);
			QVERIFY2(nextTo(frame, onScreen(button)),
					 qPrintable(QStringLiteral("dialog at %1,%2")
								.arg(frame.x()).arg(frame.y())));
		}

		void textFieldAlignmentButton()
		{
			QETElementEditor window;
			ElementScene *scene = window.elementScene();
			const auto reset = qScopeGuard([scene] { scene->reset(); });
			auto *field = new PartDynamicTextField(&window);
			field->setText(QStringLiteral("Field"));
			scene->addItems({field});

			DynamicTextFieldEditor editor(&window);
			QVERIFY(editor.setParts({field}));
			editor.move(screenArea().topLeft() + QPoint(100, 50));
			editor.show();
			QVERIFY(QTest::qWaitForWindowExposed(&editor));
			auto *button = editor.findChild<QPushButton *>(
						QStringLiteral("m_alignment_pb"));
			QVERIFY(button);

			const QRect frame = dialogOpenedBy(button);
			QVERIFY2(nextTo(frame, onScreen(button)),
					 qPrintable(QStringLiteral("dialog at %1,%2")
								.arg(frame.x()).arg(frame.y())));
		}

		void terminalAlignmentButton()
		{
			QETElementEditor window;
			ElementScene *scene = window.elementScene();
			const auto reset = qScopeGuard([scene] { scene->reset(); });
			auto *terminal = new PartTerminal(&window);
			terminal->setShowName(true);
			scene->addItems({terminal});

			TerminalEditor editor(&window);
			QVERIFY(editor.setPart(terminal));
			editor.move(screenArea().topLeft() + QPoint(100, 50));
			editor.show();
			QVERIFY(QTest::qWaitForWindowExposed(&editor));
			auto *button = editor.findChild<QPushButton *>(
						QStringLiteral("m_align_pb"));
			QVERIFY(button);
			QVERIFY(button->isEnabled());

			const QRect frame = dialogOpenedBy(button);
			QVERIFY2(nextTo(frame, onScreen(button)),
					 qPrintable(QStringLiteral("dialog at %1,%2")
								.arg(frame.x()).arg(frame.y())));
		}

		void opensAboveTheButtonWithoutRoomBelow()
		{
			const QRect screen = screenArea();
			ButtonWindow w(QPoint(screen.center().x(),
								  screen.top() + screen.height() - 60));
			QVERIFY(QTest::qWaitForWindowExposed(&w.window));
			AlignmentTextDialog dialog(Qt::AlignCenter, &w.window);
			QVERIFY(openUnder(dialog, w.button));

			const QRect button = onScreen(w.button);
			const QRect placed = dialog.frameGeometry();
			QCOMPARE(placed.top() + placed.height(), button.top());
			QCOMPARE(placed.left(), button.left());
		}

		void staysInsideTheScreenAtItsRightEdge()
		{
			const QRect screen = screenArea();
			ButtonWindow w(QPoint(screen.left() + screen.width() - 120,
								  screen.top() + 100));
			QVERIFY(QTest::qWaitForWindowExposed(&w.window));
			AlignmentTextDialog dialog(Qt::AlignCenter, &w.window);
			QVERIFY(openUnder(dialog, w.button));

			const QRect placed = dialog.frameGeometry();
			QVERIFY2(screen.contains(placed),
					 "the dialog goes past the right edge of the screen");
			QCOMPARE(placed.left() + placed.width(),
					 screen.left() + screen.width());
		}

		void rightToLeftLinesUpTheRightEdges()
		{
			ButtonWindow w(screenArea().center() - QPoint(60, 100));
			w.window.setLayoutDirection(Qt::RightToLeft);
			QVERIFY(QTest::qWaitForWindowExposed(&w.window));
			AlignmentTextDialog dialog(Qt::AlignCenter, &w.window);
			QVERIFY(openUnder(dialog, w.button));

			const QRect button = onScreen(w.button);
			const QRect placed = dialog.frameGeometry();
			QCOMPARE(placed.left() + placed.width(),
					 button.left() + button.width());
			QCOMPARE(placed.top(), button.top() + button.height());
		}

			//The window is mostly on the left screen, the button on the
			//right one: the dialog goes under the button, on the right
		void usesTheScreenUnderTheButton()
		{
			const QList<QScreen *> screens = QGuiApplication::screens();
			if (screens.size() != 2
				|| screens.at(1)->geometry().left()
				   != screens.at(0)->geometry().left()
					  + screens.at(0)->geometry().width())
				QSKIP("needs two screens side by side, as in CTest");

			const QRect left = screens.at(0)->availableGeometry();
			QWidget window;
			window.setGeometry(left.left() + left.width() - 300,
							   left.top() + 100, 400, 60);
			auto *button = new QPushButton(&window);
			button->setGeometry(320, 10, 60, 30);
			window.show();
			QVERIFY(QTest::qWaitForWindowExposed(&window));
			QCOMPARE(window.screen(), screens.at(0));

			AlignmentTextDialog dialog(Qt::AlignCenter, &window);
			QVERIFY(openUnder(dialog, button));
			const QRect placed = dialog.frameGeometry();
			QCOMPARE(placed.topLeft(), onScreen(button).bottomLeft()
										   + QPoint(0, 1));
		}

			//The user may have moved it: placing it again changes nothing
		void aShownDialogStaysWhereItIs()
		{
			ButtonWindow w(screenArea().center());
			QVERIFY(QTest::qWaitForWindowExposed(&w.window));
			AlignmentTextDialog dialog(Qt::AlignCenter, &w.window);
			QVERIFY(openUnder(dialog, w.button));
			const QPoint moved = dialog.pos() + QPoint(-40, -40);
			dialog.move(moved);
			DialogPlacement::placeBelow(&dialog, w.button);
			QCOMPARE(dialog.pos(), moved);
		}

			//Where it was closed no longer matters: what opens it decides
		void closingRecordsNoPosition()
		{
			ButtonWindow w(screenArea().center());
			AlignmentTextDialog dialog(Qt::AlignCenter, &w.window);
			DialogPlacement::placeBelow(&dialog, w.button);
			dialog.done(QDialog::Accepted);
			QVERIFY(!QSettings().contains(
						QStringLiteral("dialoggeometry/AlignmentTextDialog")));
		}
};

QET_APP_TEST_MAIN(tst_dialogplacement)
#include "tst_dialogplacement.moc"
