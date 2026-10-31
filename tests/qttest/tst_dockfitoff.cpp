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

#include <QCheckBox>
#include <QDockWidget>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

/**
	Without the setting "Narrow docks" (the default) the docks look as
	before DockFit: a check box keeps its own text, no dock gets a
	sideways scroll area.
*/
class tst_dockfitoff : public QObject
{
	Q_OBJECT

	private slots:
		void docksAsBefore()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			editor->show();
			QVERIFY(QTest::qWaitForWindowExposed(editor));

			const QString long_text = QStringLiteral("A text far too long to stand on one line of a narrow panel");
			auto *dock = new QDockWidget(QStringLiteral("Test"), editor);
			auto *content = new QWidget(dock);
			auto *layout = new QVBoxLayout(content);
			auto *box = new QCheckBox(long_text, content);
			layout->addWidget(box);
			dock->setWidget(content);
			editor->addDockWidget(Qt::RightDockWidgetArea, dock);
			dock->show();
			QTest::qWait(100);

			QCOMPARE(box->text(), long_text);
			for (QLabel *l : content->findChildren<QLabel *>())
				QVERIFY(l->buddy() != box);
			QVERIFY(!editor->findChild<QScrollArea *>(QStringLiteral("qet_dock_side_scroll")));

			editor->removeDockWidget(dock);
			delete dock;
		}
};

QET_APP_TEST_MAIN(tst_dockfitoff)

#include "tst_dockfitoff.moc"
