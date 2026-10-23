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

/**
	The docks of the right side share one width: the widest minimum of
	them all. The Assistant dock's check box, which never wraps its long
	text, and the Auto numbering dock's fields held it at about 310 px, so
	the side could not be made narrower. Both now go down to a narrow
	panel: the text wraps, the fields scroll.
*/
class tst_dockminwidth : public QObject
{
	Q_OBJECT

	private slots:
		void rightDocksCanBeNarrow()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			editor->resize(1600, 1000);
			editor->show();
			QVERIFY(QTest::qWaitForWindowExposed(editor));

			for (const QString &name : {QStringLiteral("assistant_dock"),
						    QStringLiteral("AutoNumberingDockWidget")}) {
				auto *dock = editor->findChild<QDockWidget *>(name);
				QVERIFY2(dock, qPrintable(name));
				QVERIFY2(dock->minimumSizeHint().width() <= 200,
					 qPrintable(QString("%1 needs at least %2 px")
						    .arg(name).arg(dock->minimumSizeHint().width())));
			}
		}

		void assistantTextTogglesItsBox()
		{
			auto *dock = QetAppTest::editor()->findChild<QDockWidget *>(QStringLiteral("assistant_dock"));
			QVERIFY(dock);
			auto *box = dock->findChild<QCheckBox *>();
			auto *label = dock->findChild<QLabel *>();
			QVERIFY(box && label);
			const bool before = box->isChecked();
			dock->show();
			QTest::mouseClick(label, Qt::LeftButton);
			QCOMPARE(box->isChecked(), !before);
			QTest::mouseClick(label, Qt::LeftButton);
			QCOMPARE(box->isChecked(), before);
		}
};

QET_APP_TEST_MAIN(tst_dockminwidth)

#include "tst_dockminwidth.moc"
