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
/*
	The Edit menu stays short enough to read: families of commands sit
	one level down, every action keeps its shortcut.
*/
#include "qetapptest.h"

#include <QMenuBar>

class tst_tidyeditmenu : public QObject
{
	Q_OBJECT

	private:
		QMenu *editMenu()
		{
			QETDiagramEditor *e = QetAppTest::editor();
			if (!e) return nullptr;
			for (QAction *a : e->menuBar()->actions())
				if (a->menu() && a->text() == QETDiagramEditor::tr("&Edit"))
					return a->menu();
			return nullptr;
		}

		static void collect(QMenu *menu, QList<QAction *> &into)
		{
			for (QAction *a : menu->actions()) {
				if (a->menu()) collect(a->menu(), into);
				else if (!a->isSeparator()) into << a;
			}
		}

	private slots:
		void initTestCase()
		{
			QVERIFY(QetAppTest::view());
		}

		void editMenuShort()
		{
			QMenu *edit = editMenu();
			QVERIFY(edit);
			int entries = 0;
			for (QAction *a : edit->actions())
				if (!a->isSeparator()) ++entries;
			QVERIFY2(entries <= 30, qPrintable(QString::number(entries)));
		}

		void movedActionsStillThere()
		{
			QList<QAction *> all;
			collect(editMenu(), all);
			for (const QString &name : {QStringLiteral("rotate_selection"),
						    QStringLiteral("rotate_texts"),
						    QStringLiteral("paste_origin")}) {
				QAction *a = QetAppTest::action(name);
				if (!a) continue;
				QVERIFY2(all.contains(a), qPrintable(name));
			}
			QAction *rotate = QetAppTest::action(QStringLiteral("rotate_selection"));
			QVERIFY(rotate);
			QVERIFY(!editMenu()->actions().contains(rotate));
			QVERIFY(!rotate->shortcut().isEmpty());
		}
};

QET_APP_TEST_MAIN(tst_tidyeditmenu)

#include "tst_tidyeditmenu.moc"
