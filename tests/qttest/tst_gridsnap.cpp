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
#include "foliogrid.h"

#include <QAction>
#include <QSettings>

/**
	View > Snap to the grid switches the snap off and on for everything
	that snaps (snapToGrid(), snapToTextGrid()), as Ctrl held does, and is
	kept in the settings. View > Grid step sets the step, fine, medium or
	coarse, for both directions.
*/
class tst_gridsnap : public QObject
{
	Q_OBJECT

	static QAction *action(const QString &text)
	{
		for (QAction *a : QetAppTest::editor()->findChildren<QAction *>())
			if (a->text() == text) return a;
		return nullptr;
	}

	private slots:
		void snapCanBeSwitchedOff()
		{
			QSettings().setValue(FolioGrid::x_key, 10);
			QSettings().setValue(FolioGrid::y_key, 10);
			QAction *snap = action(QStringLiteral("Snap to the grid"));
			QVERIFY(snap && snap->isCheckable());
			QVERIFY(snap->isChecked());
			QCOMPARE(snap->shortcut(), QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_G));
			QCOMPARE(Diagram::snapToGrid(QPointF(13.4, 26.7)), QPointF(10, 30));

			snap->trigger();
			QVERIFY(!Diagram::gridSnap());
			QCOMPARE(Diagram::snapToGrid(QPointF(13.4, 26.7)), QPointF(13, 27));
			QCOMPARE(Diagram::snapToTextGrid(QPointF(13.4, 26.7)), QPointF(13, 27));
			QVERIFY(!QSettings().value(QStringLiteral("diagrameditor/grid_snap")).toBool());

			snap->trigger();
			QVERIFY(Diagram::gridSnap());
			QCOMPARE(Diagram::snapToGrid(QPointF(13.4, 26.7)), QPointF(10, 30));
		}

		void gridSteps()
		{
			const struct { const char *text; int step; Qt::Key key; } steps[] = {
				{"Fine (5)", 5, Qt::Key_1}, {"Medium (10)", 10, Qt::Key_2}, {"Coarse (20)", 20, Qt::Key_3}};
			for (const auto &s : steps) {
				QAction *a = action(QString::fromLatin1(s.text));
				QVERIFY2(a, s.text);
				QCOMPARE(a->shortcut(), QKeySequence(Qt::CTRL | Qt::ALT | s.key));
				a->trigger();
				QCOMPARE(QSettings().value(FolioGrid::x_key).toInt(), s.step);
				QCOMPARE(QSettings().value(FolioGrid::y_key).toInt(), s.step);
				QVERIFY(a->isChecked());
				QCOMPARE(Diagram::snapToGrid(QPointF(23, 23)),
					 QPointF(qRound(23.0 / s.step) * s.step, qRound(23.0 / s.step) * s.step));
			}
			action(QStringLiteral("Medium (10)"))->trigger();
		}
};

QET_APP_TEST_MAIN(tst_gridsnap)

#include "tst_gridsnap.moc"
