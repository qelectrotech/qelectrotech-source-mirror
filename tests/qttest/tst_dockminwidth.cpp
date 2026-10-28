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
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTabWidget>
#include <QHeaderView>
#include <QTemporaryDir>
#include <QTreeWidget>
#include <QVBoxLayout>

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
};

static void narrowDocks()
{
	QSettings().setValue(QStringLiteral("diagrameditor/narrow_docks"), true);
	QSettings().setValue(QStringLiteral("diagrameditor/conductor_properties_panel"), true);
}

QET_APP_TEST_MAIN_PREPARED(tst_dockminwidth, narrowDocks)

#include "tst_dockminwidth.moc"
