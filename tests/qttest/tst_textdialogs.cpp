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
#include "ui/dynamicelementtextitemeditor.h"
#include "ui/dynamicelementtextmodel.h"

#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalSpy>
#include <QTimer>
#include <QTreeView>
#include <QUndoStack>
#include <QWindow>

#include <functional>
#include <memory>

/**
	The text alignment dialog applies a point as soon as it is picked, and
	stays open to try another (issue #1973). It has no OK or Cancel: its
	Close button, Esc and the close button of its title bar close it and
	keep the point. In the symbol editor, from the Alignment button of a
	text, a text field and a terminal, Undo returns to where the dialog
	started, in one step for each property changed. In the Texts tab of a
	symbol, the point is saved at once.

	The composite text dialog of the Texts tab saves the text only with
	OK. A click elsewhere in the tab, Ctrl+Return or Ctrl+Tab leaves it
	open.
*/
class tst_textdialogs : public QObject
{
	Q_OBJECT

	static constexpr Qt::Alignment bottomRight =
			Qt::AlignBottom | Qt::AlignRight;
	static constexpr Qt::Alignment top = Qt::AlignTop | Qt::AlignHCenter;
	static constexpr Qt::Alignment topLeft = Qt::AlignTop | Qt::AlignLeft;

		//The ways out of a dialog
	enum class Exit { Ok, Cancel, Close, Escape, TitleBar };

		//The ways to close the alignment dialog
	static QList<QPair<const char *, Exit>> closes()
	{
		return {{"Close", Exit::Close},
				{"Esc", Exit::Escape},
				{"title bar", Exit::TitleBar}};
	}

	static const QStringList &partKinds()
	{
		static const QStringList kinds{QStringLiteral("text"),
									   QStringLiteral("text field"),
									   QStringLiteral("terminal")};
		return kinds;
	}

		//Leave @p dialog by @p way_out, as the user does. False if the
		//dialog has no button for it.
	static bool leave(QDialog *dialog, Exit way_out)
	{
		auto *box = dialog->findChild<QDialogButtonBox *>();
		switch (way_out) {
			case Exit::Ok:
			case Exit::Cancel:
			case Exit::Close:
			{
				const QDialogButtonBox::StandardButton which =
						way_out == Exit::Ok ? QDialogButtonBox::Ok
						: way_out == Exit::Cancel ? QDialogButtonBox::Cancel
												  : QDialogButtonBox::Close;
				QPushButton *button = box ? box->button(which) : nullptr;
				if (!button)
					return false;
				button->click();
				return true;
			}
			case Exit::Escape:
			{
					//To the widget with the focus, as a key press goes
				QWidget *target = QApplication::focusWidget();
				if (!target || !dialog->isAncestorOf(target))
					target = dialog;
				QTest::keyClick(target, Qt::Key_Escape);
				return true;
			}
			case Exit::TitleBar:
					//What the close button of the title bar does
				dialog->close();
				return true;
		}
		return false;
	}

		//Pick the point @p name of the alignment dialog @p dialog
	static void pick(QDialog *dialog, const char *name)
	{
		if (auto *point = dialog->findChild<QRadioButton *>(
				QLatin1String(name)))
			point->click();
	}

		//Click @p button, which opens the alignment dialog with exec(), and
		//run @p act on the dialog once it shows. False if it did not show.
	static bool whileOpen(QPushButton *button,
						  const std::function<void(QDialog *)> &act)
	{
		bool shown = false;
		QElapsedTimer waited;
		waited.start();
		QTimer poll;
		QObject::connect(&poll, &QTimer::timeout, [&]() {
			auto *dialog = qobject_cast<AlignmentTextDialog *>(
						QApplication::activeModalWidget());
			if (!dialog)
				return;
			shown = dialog->windowHandle()
					&& dialog->windowHandle()->isExposed();
			if (!shown && waited.elapsed() < 5000)
				return;
			poll.stop();
			if (shown)
				act(dialog);
			if (dialog->isVisible())
				dialog->reject();
		});
		poll.start(10);
		button->click();
		return shown;
	}

		//The Undo steps from @p from to @p to: a terminal keeps the
		//horizontal and the vertical alignment of its label apart
	static int steps(const QString &kind, Qt::Alignment from,
					 Qt::Alignment to)
	{
		if (kind != QLatin1String("terminal"))
			return from != to ? 1 : 0;
		const auto h = Qt::AlignHorizontal_Mask;
		const auto v = Qt::AlignVertical_Mask;
		return ((from & h) != (to & h) ? 1 : 0)
				+ ((from & v) != (to & v) ? 1 : 0);
	}

		//The symbol editor with a part of @p kind, and the editor of that
		//part, which has the Alignment button. With @p two_parts, a text
		//or a text field is edited with a second one aligned bottom right.
	struct SymbolEditor
	{
		QETElementEditor window;
		std::unique_ptr<QWidget> editor;
		QPushButton *button = nullptr;
		std::function<Qt::Alignment()> alignment;
		std::function<Qt::Alignment()> second;

		explicit SymbolEditor(const QString &kind, bool two_parts = false)
		{
			ElementScene *scene = window.elementScene();
			if (kind == QLatin1String("text field")) {
				QList<CustomElementPart *> parts;
				QList<PartDynamicTextField *> fields;
				for (int i = 0; i < (two_parts ? 2 : 1); ++i) {
					auto *field = new PartDynamicTextField(&window);
					field->setText(QStringLiteral("Field"));
					field->setAlignment(i ? bottomRight : topLeft);
					scene->addItems({field});
					parts << field;
					fields << field;
				}
				auto *e = new DynamicTextFieldEditor(&window);
				editor.reset(e);
				if (e->setParts(parts))
					button = e->findChild<QPushButton *>(
								QStringLiteral("m_alignment_pb"));
				alignment = [fields] { return fields.first()->alignment(); };
				second = [fields] { return fields.last()->alignment(); };
			} else if (kind == QLatin1String("text")) {
				QList<CustomElementPart *> parts;
				QList<PartText *> texts;
				for (int i = 0; i < (two_parts ? 2 : 1); ++i) {
					auto *text = new PartText(&window);
					text->setPlainText(QStringLiteral("Text"));
					text->setAlignment(i ? bottomRight : topLeft);
					scene->addItems({text});
					parts << text;
					texts << text;
				}
				auto *e = new TextEditor(&window);
				editor.reset(e);
				if (e->setParts(parts))
					for (QPushButton *b : e->findChildren<QPushButton *>())
						if (b->text() == TextEditor::tr("Alignment"))
							button = b;
				alignment = [texts] { return texts.first()->alignment(); };
				second = [texts] { return texts.last()->alignment(); };
			} else {
				auto *terminal = new PartTerminal(&window);
				terminal->setShowName(true);
				scene->addItems({terminal});
				auto *e = new TerminalEditor(&window);
				editor.reset(e);
				if (e->setPart(terminal))
					button = e->findChild<QPushButton *>(
								QStringLiteral("m_align_pb"));
				alignment = [terminal] {
					return terminal->labelHAlignment()
							| terminal->labelVAlignment();
				};
			}
		}

		~SymbolEditor()
		{
			editor.reset();
				//As closing the window does: a scene destroyed with its
				//undo history still in it updates the half-destroyed window
			window.elementScene()->reset();
		}

		QUndoStack &undoStack() { return window.elementScene()->undoStack(); }
	};

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

		//The dialog of class @p name that @p view opened to edit a cell
	static QDialog *editorOf(QTreeView &view, const char *name)
	{
		for (QDialog *d : view.findChildren<QDialog *>())
			if (d->isVisible()
				&& qstrcmp(d->metaObject()->className(), name) == 0)
				return d;
		return nullptr;
	}

		//Let the view close and delete its editor
	static void settle()
	{
		QCoreApplication::processEvents();
		QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
	}

	private slots:
		void symbolEditor_data()
		{
			QTest::addColumn<QString>("kind");
			QTest::addColumn<int>("way_out");
			for (const QString &kind : partKinds())
				for (const auto &[name, way_out] : closes())
					QTest::addRow("%s, %s", qPrintable(kind), name)
							<< kind << int(way_out);
		}
		void symbolEditor()
		{
			QFETCH(QString, kind);
			QFETCH(int, way_out);
			SymbolEditor symbol(kind);
			QVERIFY(symbol.button);
			QVERIFY(symbol.button->isEnabled());
			const Qt::Alignment start = symbol.alignment();
			QVERIFY(start != bottomRight);
			const int commands = symbol.undoStack().count();

			Qt::Alignment while_open;
			bool left = false;
			QVERIFY(whileOpen(symbol.button, [&](QDialog *dialog) {
				pick(dialog, "bottom_right");
				while_open = symbol.alignment();
				left = leave(dialog, Exit(way_out));
			}));
			QVERIFY(left);
				//Applied at the click, with the dialog still open
			QCOMPARE(while_open, bottomRight);
			QCOMPARE(symbol.alignment(), bottomRight);
			QCOMPARE(symbol.undoStack().count() - commands,
					 steps(kind, start, bottomRight));
			symbol.undoStack().setIndex(commands);
			QCOMPARE(symbol.alignment(), start);
		}

		void symbolEditorSeveralPoints_data()
		{
			QTest::addColumn<QString>("kind");
			for (const QString &kind : partKinds())
				QTest::newRow(qPrintable(kind)) << kind;
		}
		void symbolEditorSeveralPoints()
		{
			QFETCH(QString, kind);
			SymbolEditor symbol(kind);
			QVERIFY(symbol.button);
			const Qt::Alignment start = symbol.alignment();
			const int commands = symbol.undoStack().count();

			QList<Qt::Alignment> seen;
			bool left = false;
			QVERIFY(whileOpen(symbol.button, [&](QDialog *dialog) {
				pick(dialog, "bottom_right");
				seen << symbol.alignment();
				pick(dialog, "top");
				seen << symbol.alignment();
				left = leave(dialog, Exit::Close);
			}));
			QVERIFY(left);
			QCOMPARE(seen, (QList<Qt::Alignment>{bottomRight, top}));
				//The steps of one change, from the start to the last point
			QCOMPARE(symbol.undoStack().count() - commands,
					 steps(kind, start, top));
			symbol.undoStack().setIndex(commands);
			QCOMPARE(symbol.alignment(), start);
		}

		void symbolEditorWithoutPick_data()
		{
			symbolEditorSeveralPoints_data();
		}
		void symbolEditorWithoutPick()
		{
			QFETCH(QString, kind);
			SymbolEditor symbol(kind);
			QVERIFY(symbol.button);
			const Qt::Alignment start = symbol.alignment();
			const int commands = symbol.undoStack().count();

			bool left = false;
			QVERIFY(whileOpen(symbol.button, [&](QDialog *dialog) {
				left = leave(dialog, Exit::Close);
			}));
			QVERIFY(left);
			QCOMPARE(symbol.alignment(), start);
			QCOMPARE(symbol.undoStack().count(), commands);
		}

			//Parts aligned apart stay so when no point is picked
		void symbolEditorWithoutPickTwoParts_data()
		{
			QTest::addColumn<QString>("kind");
			QTest::newRow("text") << QStringLiteral("text");
			QTest::newRow("text field") << QStringLiteral("text field");
		}
		void symbolEditorWithoutPickTwoParts()
		{
			QFETCH(QString, kind);
			SymbolEditor symbol(kind, true);
			QVERIFY(symbol.button);
			QCOMPARE(symbol.alignment(), topLeft);
			QCOMPARE(symbol.second(), bottomRight);
			const int commands = symbol.undoStack().count();

			bool left = false;
			QVERIFY(whileOpen(symbol.button, [&](QDialog *dialog) {
				left = leave(dialog, Exit::Close);
			}));
			QVERIFY(left);
			QCOMPARE(symbol.alignment(), topLeft);
			QCOMPARE(symbol.second(), bottomRight);
			QCOMPARE(symbol.undoStack().count(), commands);
		}

			//A click on the checked point aligns parts that differ
		void symbolEditorCheckedPointTwoParts_data()
		{
			symbolEditorWithoutPickTwoParts_data();
		}
		void symbolEditorCheckedPointTwoParts()
		{
			QFETCH(QString, kind);
			SymbolEditor symbol(kind, true);
			QVERIFY(symbol.button);
			const int commands = symbol.undoStack().count();

			bool checked = false;
			bool left = false;
			QVERIFY(whileOpen(symbol.button, [&](QDialog *dialog) {
				auto *point = dialog->findChild<QRadioButton *>(
							QStringLiteral("top_left"));
				checked = point && point->isChecked();
				pick(dialog, "top_left");
				left = leave(dialog, Exit::Close);
			}));
			QVERIFY(checked);
			QVERIFY(left);
			QCOMPARE(symbol.alignment(), topLeft);
			QCOMPARE(symbol.second(), topLeft);
			QCOMPARE(symbol.undoStack().count(), commands + 1);
			symbol.undoStack().undo();
			QCOMPARE(symbol.second(), bottomRight);
		}

		void arrowKeysMoveFromTheCheckedPoint()
		{
			AlignmentTextDialog dialog(Qt::AlignCenter);
			QSignalSpy picked(&dialog, &AlignmentTextDialog::alignmentChanged);
			dialog.show();
			QVERIFY(QTest::qWaitForWindowExposed(&dialog));
			auto *center = dialog.findChild<QRadioButton *>(
						QStringLiteral("center"));
			QVERIFY(center);
			QTRY_COMPARE(dialog.focusWidget(), center);

				//Qt moves between the points only in the active window
			dialog.activateWindow();
			if (!QTest::qWaitForWindowActive(&dialog))
				QSKIP("needs an active window, as in CTest");
			QTest::keyClick(center, Qt::Key_Left);
			QCOMPARE(picked.count(), 1);
			QCOMPARE(picked.at(0).at(0).value<Qt::Alignment>(),
					 Qt::AlignVCenter | Qt::AlignLeft);
			QVERIFY(dialog.isVisible());
		}

		void textsTabAlignment_data()
		{
			QTest::addColumn<int>("way_out");
			for (const auto &[name, way_out] : closes())
				QTest::newRow(name) << int(way_out);
		}
		void textsTabAlignment()
		{
			QFETCH(int, way_out);
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			Element *element = firstSymbol(project);
			QVERIFY(element);
			auto *text = new DynamicElementTextItem(element);
			element->addDynamicTextItem(text);
			text->setAlignment(topLeft);

			DynamicElementTextModel model(element);
			DynamicTextItemDelegate delegate;
			QTreeView view;
			view.setModel(&model);
			view.setItemDelegate(&delegate);
			view.show();
			QVERIFY(QTest::qWaitForWindowExposed(&view));

			const QModelIndex cell = cellOf(
						model, text, DynamicElementTextModel::txtAlignment);
			QVERIFY(cell.isValid());
			view.expandAll();
			view.edit(cell);
			QPointer<QDialog> dialog = editorOf(view, "AlignmentTextDialog");
			QVERIFY(dialog);
			QVERIFY(QTest::qWaitForWindowExposed(dialog));
			pick(dialog, "bottom_right");
				//Saved at the click, with the dialog still open
			QVERIFY(dialog);
			QVERIFY(dialog->isVisible());
			QCOMPARE(model.data(cell, Qt::UserRole + 2).value<Qt::Alignment>(),
					 bottomRight);

			const bool left = leave(dialog, Exit(way_out));
			if (!left)
				dialog->reject();
			settle();
			QVERIFY(left);
			QCOMPARE(model.data(cell, Qt::UserRole + 2).value<Qt::Alignment>(),
					 bottomRight);
		}

			//In the properties dialog, a point waits in the tab for
			//Apply. Opened again, the dialog shows that point, and closing
			//it keeps the point.
		void textsTabAlignmentReopened()
		{
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			Element *element = firstSymbol(project);
			QVERIFY(element);
			auto *text = new DynamicElementTextItem(element);
			element->addDynamicTextItem(text);
			text->setAlignment(topLeft);

			DynamicElementTextModel model(element);
			DynamicTextItemDelegate delegate;
			QTreeView view;
			view.setModel(&model);
			view.setItemDelegate(&delegate);
			view.show();
			QVERIFY(QTest::qWaitForWindowExposed(&view));

			const QModelIndex cell = cellOf(
						model, text, DynamicElementTextModel::txtAlignment);
			QVERIFY(cell.isValid());
			view.expandAll();
			view.edit(cell);
			QPointer<QDialog> dialog = editorOf(view, "AlignmentTextDialog");
			QVERIFY(dialog);
			QVERIFY(QTest::qWaitForWindowExposed(dialog));
			pick(dialog, "bottom_right");
			QVERIFY(leave(dialog, Exit::Close));
			settle();

			view.edit(cell);
			dialog = editorOf(view, "AlignmentTextDialog");
			QVERIFY(dialog);
			QVERIFY(QTest::qWaitForWindowExposed(dialog));
			auto *point = dialog->findChild<QRadioButton *>(
						QStringLiteral("bottom_right"));
			QVERIFY(point);
			QVERIFY(point->isChecked());
			QVERIFY(leave(dialog, Exit::Close));
			settle();
			QCOMPARE(model.data(cell, Qt::UserRole + 2).value<Qt::Alignment>(),
					 bottomRight);
				//The folio changes only at Apply
			QCOMPARE(text->alignment(), topLeft);
		}

			//The selection properties panel applies each change to the
			//folio at once: the text moves while the dialog is open
		void selectionPanelAlignment()
		{
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			Element *element = firstSymbol(project);
			QVERIFY(element);
			QVERIFY(element->diagram());
			auto *text = new DynamicElementTextItem(element);
			element->addDynamicTextItem(text);
			text->setAlignment(topLeft);

			DynamicElementTextItemEditor editor(element);
			editor.setLiveEdit(true);
			editor.show();
			QVERIFY(QTest::qWaitForWindowExposed(&editor));
			auto *view = editor.findChild<QTreeView *>();
			QVERIFY(view);
			auto *model = qobject_cast<DynamicElementTextModel *>(
						view->model());
			QVERIFY(model);
			const QModelIndex cell = cellOf(
						*model, text, DynamicElementTextModel::txtAlignment);
			QVERIFY(cell.isValid());
			QUndoStack &stack = element->diagram()->undoStack();
			const int commands = stack.count();

			view->expandAll();
			view->edit(cell);
			QPointer<QDialog> dialog = editorOf(*view, "AlignmentTextDialog");
			QVERIFY(dialog);
			QVERIFY(QTest::qWaitForWindowExposed(dialog));
			pick(dialog, "bottom_right");
			QCOMPARE(text->alignment(), bottomRight);
			QVERIFY(dialog);
			pick(dialog, "top");
			QCOMPARE(text->alignment(), top);
			QVERIFY(dialog);
			QVERIFY(dialog->isVisible());

			QVERIFY(leave(dialog, Exit::Close));
			settle();
			QCOMPARE(text->alignment(), top);
				//Both points in one Undo step
			QCOMPARE(stack.count(), commands + 1);
			stack.undo();
			QCOMPARE(text->alignment(), topLeft);
		}

			//Undo also brings the panel back to the old point, so that a
			//change to another field does not apply the undone one again
		void selectionPanelAlignmentUndo()
		{
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			Element *element = firstSymbol(project);
			QVERIFY(element);
			QVERIFY(element->diagram());
			auto *text = new DynamicElementTextItem(element);
			element->addDynamicTextItem(text);
			text->setAlignment(topLeft);

			DynamicElementTextItemEditor editor(element);
			editor.setLiveEdit(true);
			editor.show();
			QVERIFY(QTest::qWaitForWindowExposed(&editor));
			auto *view = editor.findChild<QTreeView *>();
			QVERIFY(view);
			auto *model = qobject_cast<DynamicElementTextModel *>(
						view->model());
			QVERIFY(model);
			const QModelIndex cell = cellOf(
						*model, text, DynamicElementTextModel::txtAlignment);
			const QModelIndex size = cellOf(
						*model, text, DynamicElementTextModel::size);
			QVERIFY(cell.isValid() && size.isValid());
			QUndoStack &stack = element->diagram()->undoStack();

			view->expandAll();
			view->edit(cell);
			QPointer<QDialog> dialog = editorOf(*view, "AlignmentTextDialog");
			QVERIFY(dialog);
			QVERIFY(QTest::qWaitForWindowExposed(dialog));
			pick(dialog, "bottom_right");
			QVERIFY(leave(dialog, Exit::Close));
			settle();
			QCOMPARE(text->alignment(), bottomRight);

			stack.undo();
			QCOMPARE(text->alignment(), topLeft);
			QCOMPARE(model->data(cell, Qt::UserRole + 2)
						 .value<Qt::Alignment>(),
					 topLeft);

			const int index = stack.index();
			model->setData(size, model->data(size, Qt::EditRole).toInt() + 2,
						   Qt::EditRole);
			QCOMPARE(stack.index(), index + 1);
			QCOMPARE(text->alignment(), topLeft);
		}

		void textsTabCompositeText_data()
		{
			QTest::addColumn<int>("way_out");
			QTest::addColumn<bool>("applied");
			QTest::newRow("OK") << int(Exit::Ok) << true;
			QTest::newRow("Cancel") << int(Exit::Cancel) << false;
			QTest::newRow("Esc") << int(Exit::Escape) << false;
			QTest::newRow("title bar") << int(Exit::TitleBar) << false;
		}
		void textsTabCompositeText()
		{
			QFETCH(int, way_out);
			QFETCH(bool, applied);
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			Element *element = firstSymbol(project);
			QVERIFY(element);
			auto *text = new DynamicElementTextItem(element);
			element->addDynamicTextItem(text);
			text->setCompositeText(QStringLiteral("before"));
			text->setTextFrom(DynamicElementTextItem::CompositeText);

			DynamicElementTextModel model(element);
			DynamicTextItemDelegate delegate;
			QTreeView view;
			view.setModel(&model);
			view.setItemDelegate(&delegate);
			view.show();
			QVERIFY(QTest::qWaitForWindowExposed(&view));

			const QModelIndex cell = cellOf(
						model, text, DynamicElementTextModel::compositeText);
			QVERIFY(cell.isValid());
			view.expandAll();
			view.edit(cell);
			QDialog *dialog = editorOf(view, "CompositeTextEditDialog");
			QVERIFY(dialog);
			QVERIFY(QTest::qWaitForWindowExposed(dialog));
			auto *edit = dialog->findChild<QPlainTextEdit *>();
			QVERIFY(edit);
			edit->setPlainText(QStringLiteral("after"));
			const bool left = leave(dialog, Exit(way_out));
			if (!left)
				dialog->reject();
			settle();
			QVERIFY(left);

			QCOMPARE(model.data(cell, Qt::UserRole + 2).toString(),
					 applied ? QStringLiteral("after")
							 : QStringLiteral("before"));
		}

			//Keys that close an editor of the view leave this dialog open
		void textsTabCompositeTextKeys_data()
		{
			QTest::addColumn<int>("key");
			QTest::newRow("Ctrl+Return") << int(Qt::Key_Return);
			QTest::newRow("Ctrl+Tab") << int(Qt::Key_Tab);
		}
		void textsTabCompositeTextKeys()
		{
			QFETCH(int, key);
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			Element *element = firstSymbol(project);
			QVERIFY(element);
			auto *text = new DynamicElementTextItem(element);
			element->addDynamicTextItem(text);
			text->setCompositeText(QStringLiteral("before"));
			text->setTextFrom(DynamicElementTextItem::CompositeText);

			DynamicElementTextModel model(element);
			DynamicTextItemDelegate delegate;
			QTreeView view;
			view.setModel(&model);
			view.setItemDelegate(&delegate);
			view.show();
			QVERIFY(QTest::qWaitForWindowExposed(&view));

			const QModelIndex cell = cellOf(
						model, text, DynamicElementTextModel::compositeText);
			QVERIFY(cell.isValid());
			view.expandAll();
			view.edit(cell);
			QPointer<QDialog> dialog =
					editorOf(view, "CompositeTextEditDialog");
			QVERIFY(dialog);
			QVERIFY(QTest::qWaitForWindowExposed(dialog));
			auto *edit = dialog->findChild<QPlainTextEdit *>();
			QVERIFY(edit);
			edit->setPlainText(QStringLiteral("after"));
			QTest::keyClick(edit, Qt::Key(key), Qt::ControlModifier);
			settle();
			QVERIFY(dialog);
			QVERIFY(dialog->isVisible());
			QCOMPARE(model.data(cell, Qt::UserRole + 2).toString(),
					 QStringLiteral("before"));

			QVERIFY(leave(dialog, Exit::Ok));
			settle();
			QCOMPARE(model.data(cell, Qt::UserRole + 2).toString(),
					 QStringLiteral("after"));
		}

		void textsTabCompositeTextIsModal()
		{
			QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
			Element *element = firstSymbol(project);
			QVERIFY(element);
			auto *text = new DynamicElementTextItem(element);
			element->addDynamicTextItem(text);
			text->setCompositeText(QStringLiteral("before"));
			text->setTextFrom(DynamicElementTextItem::CompositeText);

			DynamicElementTextModel model(element);
			DynamicTextItemDelegate delegate;
			QTreeView view;
			view.setModel(&model);
			view.setItemDelegate(&delegate);
			view.resize(500, 600);
			view.show();
			QVERIFY(QTest::qWaitForWindowExposed(&view));

			const QModelIndex cell = cellOf(
						model, text, DynamicElementTextModel::compositeText);
			const QModelIndex other = cellOf(
						model, text, DynamicElementTextModel::size);
			QVERIFY(cell.isValid() && other.isValid());
			view.expandAll();
			view.setCurrentIndex(cell);
			view.edit(cell);
			QPointer<QDialog> dialog =
					editorOf(view, "CompositeTextEditDialog");
			QVERIFY(dialog);
			QVERIFY(QTest::qWaitForWindowExposed(dialog));
			auto *edit = dialog->findChild<QPlainTextEdit *>();
			QVERIFY(edit);
			edit->setPlainText(QStringLiteral("after"));

				//Before, a click on another cell closed the dialog. Now it
				//cannot reach the view, and the dialog waits for an answer.
				//A click on the window, unlike one on a widget, goes
				//through the window system, which blocks it as for a user.
			QCOMPARE(QApplication::activeModalWidget(), dialog.data());
			QTest::mouseClick(view.windowHandle(), Qt::LeftButton, {},
							  view.viewport()->mapTo(
								  &view, view.visualRect(other).center()));
			settle();
			QVERIFY(dialog);
			QVERIFY(dialog->isVisible());
			QCOMPARE(view.currentIndex(), cell);
			QCOMPARE(model.data(cell, Qt::UserRole + 2).toString(),
					 QStringLiteral("before"));

			QVERIFY(leave(dialog, Exit::Ok));
			settle();
			QCOMPARE(model.data(cell, Qt::UserRole + 2).toString(),
					 QStringLiteral("after"));
		}
};

QET_APP_TEST_MAIN(tst_textdialogs)
#include "tst_textdialogs.moc"
