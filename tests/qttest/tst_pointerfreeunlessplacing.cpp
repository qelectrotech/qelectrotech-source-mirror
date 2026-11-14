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
	"Pointer on the grid only while placing" (a setting, off by default):
	off, the crosshair keeps to the grid as before; on, it follows the
	mouse freely until something is placed or dragged.
*/
#include "qetapptest.h"
#include "diagram.h"

#include <QMouseEvent>

class tst_pointerfreeunlessplacing : public QObject
{
	Q_OBJECT

	DiagramView *m_view = nullptr;

	void move(const QPoint &at)
	{
		QMouseEvent event(QEvent::MouseMove, at, m_view->viewport()->mapToGlobal(at),
				  Qt::NoButton, Qt::NoButton, Qt::NoModifier);
		QApplication::sendEvent(m_view->viewport(), &event);
	}

		/// A point of the viewport that is not on a grid point
	QPoint offGrid() const
	{
		const QPointF scene(203, 207);
		return m_view->mapFromScene(scene);
	}

	private slots:
		void initTestCase()
		{
			m_view = QetAppTest::view();
			QVERIFY(m_view);
			m_view->resize(900, 700);
			m_view->centerOn(250, 250);
			DiagramView::setCrosshairShown(true);
		}

		void cleanupTestCase()
		{
			DiagramView::setGridOnlyWhilePlacing(false);
			DiagramView::setCrosshairShown(false);
		}

		void offByDefault()
		{
			QVERIFY(!DiagramView::gridOnlyWhilePlacing());
		}

		void offKeepsToTheGrid()
		{
			DiagramView::setGridOnlyWhilePlacing(false);
			move(offGrid());
			QCOMPARE(m_view->crosshairPos(), Diagram::snapToGrid(m_view->mapToScene(offGrid())));
		}

		void onFollowsTheMouse()
		{
			DiagramView::setGridOnlyWhilePlacing(true);
			move(offGrid());
			const QPointF at = m_view->mapToScene(offGrid());
			QCOMPARE(m_view->crosshairPos(), at);
			QVERIFY(Diagram::snapToGrid(at) != at);
		}
};

QET_APP_TEST_MAIN(tst_pointerfreeunlessplacing)
#include "tst_pointerfreeunlessplacing.moc"
