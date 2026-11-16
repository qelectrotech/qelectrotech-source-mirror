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
#include "projectview.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/element.h"
#include "ui/diagrampropertieseditordockwidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QFile>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QGroupBox>
#include <QPointer>
#include <QRadioButton>
#include <QAbstractScrollArea>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>
#include <QHeaderView>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QWidgetItemV2>

/**
	A layout item of the kind Qt's layouts make, which its widget knows
	of, that asks its widget for its size hint when measured, and notes
	whether it was deleted meanwhile
*/
class MeasuredItem : public QWidgetItemV2
{
	public:
		using QWidgetItemV2::QWidgetItemV2;
		~MeasuredItem() override { s_deleted = true; }
		QSize sizeHint() const override
		{
			const QSize hint = widget()->sizeHint();
				//Nothing of this item is read once it is deleted
			if (s_deleted)
				s_deleted_while_measured = true;
			return hint;
		}
		static inline bool s_deleted = false;
		static inline bool s_deleted_while_measured = false;
};

/**
	The docks of one side share one width, at least the widest minimum of
	them all, so one long text held the whole right side wide: the
	Assistant dock's check box, the fields of the Auto numbering dock, the
	properties of a selected symbol. The docks can now be made as narrow
	as one wants (DockFit): their texts wrap -- labels, and check boxes and
	radio buttons through a wrapping label beside them --, a combo box no
	longer widens to its longest entry, and what cannot wrap scrolls
	sideways, with a horizontal scroll bar. All of it with the setting
	"Narrow docks" (off by default, tst_dockfitoff), switched on here
	before the application starts.
*/
class tst_dockminwidth : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	static void checkNarrow(QDockWidget *dock, const QString &name)
	{
		QVERIFY2(dock->minimumSizeHint().width() <= 100,
			 qPrintable(QString("%1 needs at least %2 px")
				    .arg(name).arg(dock->minimumSizeHint().width())));
	}

	private slots:
		void initTestCase()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			editor->resize(1600, 1000);
			editor->show();
			QVERIFY(QTest::qWaitForWindowExposed(editor));
		}

		void rightDocksCanBeNarrow()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			for (const QString &name : {QStringLiteral("elements_collection_widget"),
						    QStringLiteral("diagram_properties_editor_dock_widget"),
						    QStringLiteral("AutoNumberingDockWidget"),
						    QStringLiteral("assistant_dock")}) {
				auto *dock = editor->findChild<QDockWidget *>(name);
					//A build without scripting has no Assistant dock
				if (!dock && name == QLatin1String("assistant_dock"))
					continue;
				QVERIFY2(dock, qPrintable(name));
				checkNarrow(dock, name);
			}
		}

		void selectedSymbolKeepsThemNarrow()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			const QString copy = m_dir.filePath("p.qet");
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/ten_sequential_numbers.qet"), copy));
			QVERIFY(editor->openAndAddProject(copy, false));
			Diagram *diagram = editor->currentProjectView()->currentDiagram()->diagram();
			Element *element = nullptr;
			for (QGraphicsItem *qgi : diagram->items())
				if (qgi->type() == Element::Type)
					element = static_cast<Element *>(qgi);
			QVERIFY(element);
			element->setSelected(true);
				// The panel follows the selection through a queued connection
			QTest::qWait(300);
			auto *panel = editor->findChild<QDockWidget *>("diagram_properties_editor_dock_widget");
			QVERIFY(panel);
			checkNarrow(panel, QStringLiteral("the Selection properties of a symbol"));

				//Narrower than its fields: a horizontal scroll bar
			editor->resizeDocks({panel}, {110}, Qt::Horizontal);
			QTest::qWait(200);
			auto *scroll = panel->findChild<QScrollArea *>(QStringLiteral("qet_dock_side_scroll"));
			QVERIFY(scroll);
			QVERIFY2(panel->width() < 140, qPrintable(QString("the panel stays %1 px wide").arg(panel->width())));
			QVERIFY(scroll->horizontalScrollBar()->isVisible());
			QVERIFY(scroll->horizontalScrollBar()->maximum() > 0);
				//Wide enough: no bar, the content takes the whole width
			editor->resizeDocks({panel}, {420}, Qt::Horizontal);
			QTest::qWait(200);
			QVERIFY(!scroll->horizontalScrollBar()->isVisible());
			QCOMPARE(scroll->widget()->width(), scroll->viewport()->width());
		}

		/**
			The properties of a selected wire: in a narrow dock the
			label-field pairs side by side go one below the other, so the
			fields are not cut; a short tab (Appearance) is not as high as
			the highest one (Type), so no empty space is left below it.
		*/
		void wireEditorFitsTheDock()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			ProjectView *view = editor->currentProjectView();
			QVERIFY(view);
			Conductor *conductor = nullptr;
			for (DiagramView *dv : view->diagram_views()) {
				for (QGraphicsItem *qgi : dv->diagram()->items())
					if (qgi->type() == Conductor::Type) {
						view->showDiagram(dv);
						conductor = static_cast<Conductor *>(qgi);
						break;
					}
				if (conductor)
					break;
			}
			if (!conductor)
				QSKIP("the fixture has no wire");
			auto *panel = editor->findChild<DiagramPropertiesEditorDockWidget *>();
			QVERIFY(panel);
			panel->setDiagram(conductor->diagram());
			conductor->diagram()->clearSelection();
			conductor->setSelected(true);
			QTest::qWait(300);
			auto *tabs = panel->findChild<QTabWidget *>(QStringLiteral("tabWidget"));
			QVERIFY(tabs);

			editor->resizeDocks({panel}, {300}, Qt::Horizontal);
			QTest::qWait(200);
			QWidget *form = tabs->parentWidget();
			QVERIFY2(form->minimumSizeHint().width() < 300,
				 qPrintable(QString("the wire's properties need %1 px")
					    .arg(form->minimumSizeHint().width())));

			tabs->setCurrentIndex(0);
			QTest::qWait(100);
			const int type_height = tabs->maximumHeight();
			tabs->setCurrentIndex(1);
			QTest::qWait(100);
			QVERIFY2(tabs->maximumHeight() < type_height,
				 qPrintable(QString("Appearance %1 px, Type %2 px")
					    .arg(tabs->maximumHeight()).arg(type_height)));
			QVERIFY(tabs->height() <= tabs->maximumHeight());
		}

			//A tab page that scrolls (the collection's tree) takes the
			//dock's height: its tab widget is not held to the page's size
			//hint, which left the space below empty
		void tabPageThatScrollsGrows()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			auto *dock = editor->findChild<QDockWidget *>(QStringLiteral("elements_collection_widget"));
			QVERIFY(dock);
			auto *tabs = dock->findChild<QTabWidget *>();
			QVERIFY(tabs);
			auto *tree = qobject_cast<QAbstractScrollArea *>(tabs->currentWidget());
			QVERIFY(tree);
			dock->show();
			editor->resizeDocks({dock}, {900}, Qt::Vertical);
			QTest::qWait(100);
			QCOMPARE(tabs->maximumHeight(), QWIDGETSIZE_MAX);
			QVERIFY2(tree->height() > tree->sizeHint().height(),
				 qPrintable(QStringLiteral("tree %1 px, hint %2 px, dock %3 px")
					    .arg(tree->height()).arg(tree->sizeHint().height()).arg(dock->height())));
		}

			//A field taken out of a group's grid after the grid learned
			//its places (an editor rebuilding its rows): going narrow and
			//wide again uses only what is still in the grid
		void gridReflowAfterAFieldIsGone()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			auto *dock = new QDockWidget(QStringLiteral("Reflow"), editor);
			auto *content = new QWidget(dock);
			auto *outer = new QVBoxLayout(content);
			auto *group = new QGroupBox(QStringLiteral("Group"), content);
			auto *grid = new QGridLayout(group);
			QList<QLineEdit *> fields;
			for (int r = 0; r < 3; ++r)
				for (int c = 0; c < 2; ++c) {
					grid->addWidget(new QLabel(QStringLiteral("Name %1").arg(r * 2 + c), group), r, c * 2);
					fields << new QLineEdit(group);
					fields.last()->setMinimumWidth(120);
					grid->addWidget(fields.last(), r, c * 2 + 1);
				}
			outer->addWidget(group);
			dock->setWidget(content);
			editor->addDockWidget(Qt::RightDockWidgetArea, dock);
			dock->show();
			editor->resizeDocks({dock}, {700}, Qt::Horizontal);
			QTest::qWait(100);
			const int before = grid->count();
				//Its layout item goes with the widget
			delete fields.takeAt(1);
			QCOMPARE(grid->count(), before - 1);
			for (int width : {150, 700, 150}) {
				editor->resizeDocks({dock}, {width}, Qt::Horizontal);
				QTest::qWait(100);
			}
			QCOMPARE(grid->count(), before - 1);
			for (QLineEdit *field : std::as_const(fields))
				QVERIFY(grid->indexOf(field) >= 0);
			editor->removeDockWidget(dock);
			delete dock;
		}

		void everyDockTextWraps()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			const QString long_text = QStringLiteral("A text far too long to stand on one line of a narrow panel");
			auto *dock = new QDockWidget(QStringLiteral("Test"), editor);
			auto *content = new QWidget(dock);
			auto *layout = new QVBoxLayout(content);
			auto *label = new QLabel(long_text, content);
			auto *box = new QCheckBox(long_text, content);
			auto *radio = new QRadioButton(long_text, content);
			auto *combo = new QComboBox(content);
			combo->addItem(long_text);
				//A check box in a layout nested in the content's
			auto *row = new QHBoxLayout;
			auto *nested = new QCheckBox(long_text, content);
			row->addWidget(nested);
			auto *tree = new QTreeWidget(content);
			tree->setHeaderHidden(true);
			tree->addTopLevelItem(new QTreeWidgetItem({long_text}));
			layout->addWidget(label);
			layout->addWidget(box);
			layout->addWidget(radio);
			layout->addWidget(combo);
			layout->addLayout(row);
			layout->addWidget(tree);
			dock->setWidget(content);
			editor->addDockWidget(Qt::RightDockWidgetArea, dock);
			dock->show();
			QTest::qWait(100);

			QVERIFY(label->wordWrap());
			checkNarrow(dock, QStringLiteral("a dock of long texts"));

				//A narrow tree shows its entry whole and scrolls sideways
			editor->resizeDocks({dock}, {150}, Qt::Horizontal);
			QTest::qWait(200);
			const int text_width = tree->fontMetrics().horizontalAdvance(long_text);
			QVERIFY2(tree->header()->sectionSize(0) >= text_width,
				 qPrintable(QString("the entry is cut to %1 px of %2").arg(tree->header()->sectionSize(0)).arg(text_width)));
			QVERIFY(tree->horizontalScrollBar()->isVisible());
			QVERIFY(tree->horizontalScrollBar()->maximum() > 0);

				//The check box's text, beside it, toggles it
			QLabel *box_text = nullptr;
			for (QLabel *l : content->findChildren<QLabel *>())
				if (l != label && l->buddy() == box) box_text = l;
			QVERIFY(box_text);
			QCOMPARE(box_text->text(), long_text);
			QVERIFY(box_text->wordWrap());
			QVERIFY(!box->isChecked());
			QTest::mouseClick(box_text, Qt::LeftButton);
			QVERIFY(box->isChecked());
			QTest::mouseClick(box_text, Qt::LeftButton);
			QVERIFY(!box->isChecked());
				//and follows it: disabled, hidden
			box->setEnabled(false);
			QVERIFY(!box_text->isEnabled());
			QTest::mouseClick(box_text, Qt::LeftButton);
			QVERIFY(!box->isChecked());
			box->setEnabled(true);
			box->hide();
			QVERIFY(box_text->isHidden());
			box->show();
			QVERIFY(!box_text->isHidden());
				//and its text, set later
			box->setText(QStringLiteral("Another text"));
			QTest::qWait(100);
			QCOMPARE(box_text->text(), QStringLiteral("Another text"));
			QVERIFY(box->text().isEmpty());

				//A check box in a nested layout wraps too
			QLabel *nested_text = nullptr;
			for (QLabel *l : content->findChildren<QLabel *>())
				if (l->buddy() == nested) nested_text = l;
			QVERIFY(nested_text);
			QCOMPARE(nested_text->text(), long_text);

			editor->removeDockWidget(dock);
			delete dock;
		}

		/**
			A check box added to a page already polished, as the
			Information tab of a selected symbol adds "Freeze name", is
			polished only when its layout measures it: the layout item
			asks it for its size hint. Its text goes beside it right then,
			but the item being measured used to be deleted under the
			measure, which crashed. It now outlives the measure.
		*/
		void checkBoxPolishedWhileMeasuredKeepsItsItem()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			const QString long_text = QStringLiteral("A text far too long to stand on one line of a narrow panel");
			auto *dock = new QDockWidget(QStringLiteral("Test"), editor);
			auto *page = new QWidget(dock);
			auto *page_layout = new QVBoxLayout(page);
			dock->setWidget(page);
			editor->addDockWidget(Qt::RightDockWidgetArea, dock);
			dock->show();
			QTest::qWait(100);

			auto *box = new QCheckBox(long_text, page);
			auto *item = new MeasuredItem(box);
			page_layout->addItem(item);
			QVERIFY(!box->testAttribute(Qt::WA_WState_Polished));
			MeasuredItem::s_deleted = false;
			MeasuredItem::s_deleted_while_measured = false;
			item->sizeHint();
			QVERIFY(box->testAttribute(Qt::WA_WState_Polished));
			QVERIFY(!MeasuredItem::s_deleted_while_measured);
			QTRY_VERIFY(MeasuredItem::s_deleted);

			QVERIFY(box->text().isEmpty());
			QLabel *box_text = nullptr;
			for (QLabel *l : page->findChildren<QLabel *>())
				if (l->buddy() == box)
					box_text = l;
			QVERIFY(box_text);
			QCOMPARE(box_text->text(), long_text);

			editor->removeDockWidget(dock);
			delete dock;
		}

		/**
			A check box destroyed right after its text went beside it,
			before the event loop runs again, still has the layout item
			it was measured from deleted: the item is not leaked. It is
			deleted while the check box is destroyed, not later, when
			the check box would be gone.
		*/
		void checkBoxDestroyedFirstStillFreesItsItem()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			const QString long_text = QStringLiteral("A text far too long to stand on one line of a narrow panel");
			auto *dock = new QDockWidget(QStringLiteral("Test"), editor);
			auto *page = new QWidget(dock);
			auto *page_layout = new QVBoxLayout(page);
			dock->setWidget(page);
			editor->addDockWidget(Qt::RightDockWidgetArea, dock);
			dock->show();
			QTest::qWait(100);

			auto *box = new QCheckBox(long_text, page);
			auto *item = new MeasuredItem(box);
			page_layout->addItem(item);
			QVERIFY(!box->testAttribute(Qt::WA_WState_Polished));
			MeasuredItem::s_deleted = false;
			item->sizeHint();
			QVERIFY(box->text().isEmpty());
			QVERIFY(!MeasuredItem::s_deleted);
			delete box;
			QVERIFY(MeasuredItem::s_deleted);

			editor->removeDockWidget(dock);
			delete dock;
		}

		/**
			A group's grid of check boxes first shown in a dock too narrow
			for it, one half below the other, goes back side by side when
			the dock widens: its check boxes are wrapped before the grid
			learns where its items stand.
		*/
		void gridOfCheckBoxesWidensAgain()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QDockWidget *other = nullptr;
			for (QDockWidget *d : editor->findChildren<QDockWidget *>())
				if (d->isVisible() && !d->isFloating()
				    && editor->dockWidgetArea(d) == Qt::RightDockWidgetArea) {
					other = d;
					break;
				}
			if (!other)
				QSKIP("no dock on the right");
			editor->resizeDocks({other}, {200}, Qt::Horizontal);
			QTest::qWait(100);

			auto *dock = new QDockWidget(QStringLiteral("Grid"), editor);
			auto *content = new QWidget(dock);
			auto *outer = new QVBoxLayout(content);
			auto *group = new QGroupBox(QStringLiteral("Group"), content);
			auto *grid = new QGridLayout(group);
			QCheckBox *right_box = nullptr;
			for (int r = 0; r < 2; ++r)
				for (int c = 0; c < 2; ++c) {
					auto *box = new QCheckBox(QStringLiteral("Box %1")
								  .arg(r * 2 + c), group);
					grid->addWidget(box, r, c * 2);
					auto *field = new QLineEdit(group);
					field->setMinimumWidth(80);
					grid->addWidget(field, r, c * 2 + 1);
					if (r == 0 && c == 1)
						right_box = box;
				}
			outer->addWidget(group);
			dock->setWidget(content);
			editor->addDockWidget(Qt::RightDockWidgetArea, dock);
			editor->tabifyDockWidget(other, dock);
			dock->show();
			dock->raise();
			QTest::qWait(200);

				//The column of the right box, or of the row it is wrapped in
			auto column = [grid, right_box]() {
				QWidget *w = right_box;
				while (w && grid->indexOf(w) < 0)
					w = w->parentWidget();
				int row = -1, col = -1, row_span = 0, col_span = 0;
				if (w)
					grid->getItemPosition(grid->indexOf(w), &row, &col,
							      &row_span, &col_span);
				return col;
			};
			QVERIFY(right_box->text().isEmpty());
			QCOMPARE(column(), 0);
			editor->resizeDocks({dock}, {900}, Qt::Horizontal);
			QTest::qWait(300);
			QCOMPARE(column(), 2);

			editor->removeDockWidget(dock);
			delete dock;
		}

		/**
			A wrapped check box destroyed on its own takes its text with
			it: no label is left showing the text of a check box that is
			gone, and a click on that text meanwhile reaches nothing.
		*/
		void checkBoxDestroyedTakesItsText()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			const QString long_text = QStringLiteral("A text far too long to stand on one line of a narrow panel");
			auto *dock = new QDockWidget(QStringLiteral("Test"), editor);
			auto *page = new QWidget(dock);
			auto *page_layout = new QVBoxLayout(page);
			dock->setWidget(page);
			editor->addDockWidget(Qt::RightDockWidgetArea, dock);
			dock->show();
			QTest::qWait(100);

			auto *box = new QCheckBox(long_text, page);
			page_layout->addWidget(box);
			box->show();
			QTRY_VERIFY(box->text().isEmpty());
			QPointer<QLabel> box_text;
			for (QLabel *l : page->findChildren<QLabel *>())
				if (l->buddy() == box)
					box_text = l;
			QVERIFY(box_text);
			QCOMPARE(box_text->text(), long_text);

			delete box;
			QTest::mouseClick(box_text, Qt::LeftButton);
			QTRY_VERIFY(box_text.isNull());
			QCOMPARE(page_layout->count(), 0);

			editor->removeDockWidget(dock);
			delete dock;
		}

		/**
			A click on a check box's text whose slot deletes the row the
			text stands in, as a panel that rebuilds its page would, ends
			there: nothing reads the deleted text afterwards.
		*/
		void textClickThatDeletesItsRowIsSafe()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			const QString long_text = QStringLiteral("A text far too long to stand on one line of a narrow panel");
			auto *dock = new QDockWidget(QStringLiteral("Test"), editor);
			auto *page = new QWidget(dock);
			auto *page_layout = new QVBoxLayout(page);
			dock->setWidget(page);
			editor->addDockWidget(Qt::RightDockWidgetArea, dock);
			dock->show();
			QTest::qWait(100);

			auto *box = new QCheckBox(long_text, page);
			page_layout->addWidget(box);
			box->show();
			QTRY_VERIFY(box->text().isEmpty());
			QPointer<QLabel> box_text;
			for (QLabel *l : page->findChildren<QLabel *>())
				if (l->buddy() == box)
					box_text = l;
			QVERIFY(box_text);
			QWidget *row = box_text->parentWidget();
			QObject::connect(box, &QAbstractButton::clicked, page,
					 [row] { delete row; });

			QTest::mouseClick(box_text, Qt::LeftButton);
			QVERIFY(box_text.isNull());
			QCOMPARE(page_layout->count(), 0);

			editor->removeDockWidget(dock);
			delete dock;
		}

		/**
			Qt shows a check box added to a panel already on screen a
			moment after it is added. One polished before then, as asking
			it for its size hint does, gets its text put beside it
			meanwhile: it is still shown, with its text.
		*/
		void checkBoxAddedToShownPageIsShown()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			const QString long_text = QStringLiteral("A text far too long to stand on one line of a narrow panel");
			auto *dock = new QDockWidget(QStringLiteral("Test"), editor);
			auto *page = new QWidget(dock);
			auto *page_layout = new QVBoxLayout(page);
			dock->setWidget(page);
			editor->addDockWidget(Qt::RightDockWidgetArea, dock);
			dock->show();
			QTest::qWait(100);

			auto *box = new QCheckBox(long_text, page);
			page_layout->addWidget(box);
			QVERIFY(box->isHidden());
			box->sizeHint();
			QVERIFY(box->text().isEmpty());
			QLabel *box_text = nullptr;
			for (QLabel *l : page->findChildren<QLabel *>())
				if (l->buddy() == box)
					box_text = l;
			QVERIFY(box_text);
			QCOMPARE(box_text->text(), long_text);
			QTRY_VERIFY(box->isVisible());
			QVERIFY(box_text->isVisible());

			editor->removeDockWidget(dock);
			delete dock;
		}

		void hiddenCheckBoxStaysHidden_data()
		{
			QTest::addColumn<bool>("shown_first");
			QTest::newRow("panel on screen") << true;
			QTest::newRow("panel shown later") << false;
		}

		/**
			A check box hidden with hide() before its text is put beside
			it stays hidden, and its text with it, whether its panel is
			on screen already or is shown later.
		*/
		void hiddenCheckBoxStaysHidden()
		{
			QFETCH(bool, shown_first);
			QETDiagramEditor *editor = QetAppTest::editor();
			const QString long_text = QStringLiteral("A text far too long to stand on one line of a narrow panel");
			auto *dock = new QDockWidget(QStringLiteral("Test"), editor);
			auto *page = new QWidget(dock);
			auto *page_layout = new QVBoxLayout(page);
			dock->setWidget(page);
			editor->addDockWidget(Qt::RightDockWidgetArea, dock);
			if (shown_first) {
				dock->show();
				QTest::qWait(100);
			}

			auto *box = new QCheckBox(long_text, page);
			page_layout->addWidget(box);
			box->hide();
			if (!shown_first)
				dock->show();
			box->sizeHint();
			QVERIFY(box->text().isEmpty());
			QLabel *box_text = nullptr;
			for (QLabel *l : page->findChildren<QLabel *>())
				if (l->buddy() == box)
					box_text = l;
			QVERIFY(box_text);
			QCOMPARE(box_text->text(), long_text);
			QTest::qWait(100);
			QVERIFY(box->isHidden());
			QVERIFY(box_text->isHidden());

			editor->removeDockWidget(dock);
			delete dock;
		}
};

static void narrowDocks()
{
	QSettings().setValue(QStringLiteral("diagrameditor/narrow_docks"), true);
	QSettings().setValue(QStringLiteral("diagrameditor/conductor_properties_panel"), true);
}

QET_APP_TEST_MAIN_PREPARED(tst_dockminwidth, narrowDocks)

#include "tst_dockminwidth.moc"
