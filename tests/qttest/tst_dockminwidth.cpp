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
#include "qetgraphicsitem/element.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QFile>
#include <QLabel>
#include <QRadioButton>
#include <QTemporaryDir>
#include <QVBoxLayout>

/**
	The docks of one side share one width, at least the widest minimum of
	them all, so one long text held the whole right side wide: the
	Assistant dock's check box, the fields of the Auto numbering dock, the
	properties of a selected symbol. The texts of every dock now wrap
	(DockTextWrap): labels wrap, a check box or radio button has its text
	in a wrapping label beside it, a combo box no longer widens to its
	longest entry, and the Auto numbering fields scroll.
*/
class tst_dockminwidth : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	static void checkNarrow(QDockWidget *dock, const QString &name)
	{
		QVERIFY2(dock->minimumSizeHint().width() <= 200,
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
				//Its fields (a numbering list with its button, the text
				//editor's) still need a little more than the others: it
				//needed 322 px before
			QVERIFY2(panel->minimumSizeHint().width() <= 240,
				 qPrintable(QString("the Selection properties of a symbol need at least %1 px")
					    .arg(panel->minimumSizeHint().width())));
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
			layout->addWidget(label);
			layout->addWidget(box);
			layout->addWidget(radio);
			layout->addWidget(combo);
			dock->setWidget(content);
			editor->addDockWidget(Qt::RightDockWidgetArea, dock);
			dock->show();
			QTest::qWait(100);

			QVERIFY(label->wordWrap());
			checkNarrow(dock, QStringLiteral("a dock of long texts"));

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

			editor->removeDockWidget(dock);
			delete dock;
		}
};

QET_APP_TEST_MAIN(tst_dockminwidth)

#include "tst_dockminwidth.moc"
