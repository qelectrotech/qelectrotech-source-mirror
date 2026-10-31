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
	The status bar of the diagram editor: the grid step with its menu, a
	magnet for snapping, and the zoom with a few fixed ones.
*/
#include "qetapptest.h"
#include "diagram.h"
#include "diagramview.h"
#include "projectview.h"

#include <QMenu>
#include <QStatusBar>
#include <QToolButton>

class TstStatusGridZoom : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	QList<QToolButton *> buttons() const
	{
		return QetAppTest::editor()->statusBar()->findChildren<QToolButton *>();
	}

	private slots:
		void initTestCase()
		{
			QETDiagramEditor *editor = QetAppTest::editor();
			QVERIFY(editor);
			editor->resize(1400, 1000);
			editor->show();
			QVERIFY(QTest::qWaitForWindowExposed(editor));
			const QString copy = m_dir.filePath("p.qet");
			QVERIFY(QFile::copy(QFINDTESTDATA("fixtures/every_feature.qet"), copy));
			QVERIFY(editor->openAndAddProject(copy, false));
		}

		void gridStepShown()
		{
			bool found = false;
			for (QToolButton *b : buttons())
				if (b->text().startsWith(QETDiagramEditor::tr("Grid %1").section(QLatin1Char(' '), 0, 0))
				    && b->menu())
					found = true;
			QVERIFY2(found, "no grid step button with a menu in the status bar");
		}

		void magnetSwitchesSnapping()
		{
			QToolButton *magnet = nullptr;
			for (QToolButton *b : buttons())
				if (b->isCheckable())
					magnet = b;
			QVERIFY(magnet);
			const bool before = Diagram::gridSnap();
			QCOMPARE(magnet->isChecked(), before);
			magnet->click();
			QCOMPARE(Diagram::gridSnap(), !before);
			QCOMPARE(magnet->isChecked(), !before);
			magnet->click();
			QCOMPARE(Diagram::gridSnap(), before);
		}

		void fixedZoom()
		{
			QToolButton *zoom = nullptr;
			for (QToolButton *b : buttons())
				if (b->text().startsWith(QStringLiteral("Zoom")))
					zoom = b;
			QVERIFY(zoom);
			QVERIFY(zoom->menu());
			QAction *hundred = nullptr;
			for (QAction *a : zoom->menu()->actions())
				if (a->text().startsWith(QStringLiteral("100")))
					hundred = a;
			QVERIFY(hundred);
			hundred->trigger();
			DiagramView *view = QetAppTest::editor()->currentProjectView()->currentDiagram();
			QVERIFY(qAbs(view->transform().m11() - 1.0) < 1e-6);
		}
};

QET_APP_TEST_MAIN(TstStatusGridZoom)

#include "tst_statusgridzoom.moc"
