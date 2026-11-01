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
	The zoom step (a setting, off by default): zooming in or out goes by
	a fixed number of percent, so the zoom shown stays a whole multiple
	of it; off, by a factor of 1.15 as before.
*/
#include "qetapptest.h"

#include <QWheelEvent>

class tst_zoomstep : public QObject
{
	Q_OBJECT

	private:
		DiagramView *m_view = nullptr;

		qreal percent() const { return m_view->transform().m11() * 100; }

		void setPercent(qreal p)
		{
			m_view->zoom(p / percent());
		}

		void wheel(int delta)
		{
			QWheelEvent event(QPointF(50, 50), m_view->viewport()->mapToGlobal(QPointF(50, 50)),
							  QPoint(), QPoint(0, delta), Qt::NoButton, Qt::NoModifier,
							  Qt::NoScrollPhase, false);
			QApplication::sendEvent(m_view->viewport(), &event);
		}

	private slots:
		void initTestCase()
		{
			m_view = QetAppTest::view();
			QVERIFY(m_view);
				//Zooming out is otherwise refused once the folio fits
			QSettings().setValue(QStringLiteral("diagrameditor/zoom-out-beyond-of-folio"), true);
		}

			//Off by default: a factor of 1.15, as before
		void offByDefault()
		{
			QCOMPARE(DiagramView::zoomStep(), 0);
			setPercent(100);
			m_view->zoomBy(1);
			QVERIFY(qAbs(percent() - 115) < 1e-6);
		}

		void stepsInAndOut()
		{
			DiagramView::setZoomStep(5);
			setPercent(100);
			m_view->zoomBy(1);
			QVERIFY2(qAbs(percent() - 105) < 1e-6, qPrintable(QString::number(percent())));
			m_view->zoomBy(-3);
			QVERIFY2(qAbs(percent() - 90) < 1e-6, qPrintable(QString::number(percent())));
		}

			//Between two steps (after "fit", say): to the next one first
		void betweenStepsGoesToTheNext()
		{
			setPercent(77.6);
			m_view->zoomBy(1);
			QVERIFY2(qAbs(percent() - 80) < 1e-6, qPrintable(QString::number(percent())));
			setPercent(77.6);
			m_view->zoomBy(-1);
			QVERIFY2(qAbs(percent() - 75) < 1e-6, qPrintable(QString::number(percent())));
		}

			//One step per notch; a smooth wheel's small turns add up
		void wheelNotches()
		{
			setPercent(100);
			wheel(120);
			QVERIFY2(qAbs(percent() - 105) < 1e-6, qPrintable(QString::number(percent())));
			wheel(60);
			QVERIFY2(qAbs(percent() - 105) < 1e-6, qPrintable(QString::number(percent())));
			wheel(60);
			QVERIFY2(qAbs(percent() - 110) < 1e-6, qPrintable(QString::number(percent())));
			wheel(-240);
			QVERIFY2(qAbs(percent() - 100) < 1e-6, qPrintable(QString::number(percent())));
		}

			//The editor's zoom buttons and Ctrl++ go the same way
		void editorActions()
		{
			setPercent(100);
			QAction *in = QetAppTest::action(QStringLiteral("zoom_in"));
			QVERIFY(in);
			in->trigger();
			QVERIFY2(qAbs(percent() - 105) < 1e-6, qPrintable(QString::number(percent())));
			DiagramView::setZoomStep(0);
		}
};

QET_APP_TEST_MAIN(tst_zoomstep)
#include "tst_zoomstep.moc"
