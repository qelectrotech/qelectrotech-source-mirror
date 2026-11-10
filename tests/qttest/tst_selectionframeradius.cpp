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
	The corner radius of the frame around a selected symbol (a setting,
	10 by default as before): 0 draws it square, so its corners reach the
	corners of the symbol's frame.
*/
#include "qetapptest.h"
#include "qetgraphicsitem/qgraphicsitemutility.h"

#include <QGraphicsRectItem>
#include <QImage>
#include <QPainter>
#include <QSettings>

class tst_selectionframeradius : public QObject
{
	Q_OBJECT

	private:
			/// Whether the frame of a 60 x 40 item, drawn with the current
			/// radius, touches the item's top left corner
		static bool cornerDrawn()
		{
			QGraphicsRectItem item(0, 0, 60, 40);
			QImage image(80, 60, QImage::Format_ARGB32);
			image.fill(Qt::white);
			QPainter painter(&image);
			painter.translate(10, 10);
			QGIUtility::drawBoundingRectSelection(&item, &painter);
			painter.end();
			for (int x = 9; x <= 11; ++x)
				for (int y = 9; y <= 11; ++y)
					if (image.pixelColor(x, y) != QColor(Qt::white))
						return true;
			return false;
		}

	private slots:
		void defaultIsRounded()
		{
			QSettings().remove(QStringLiteral("diagrameditor/selection_frame_radius"));
			QGIUtility::setSelectionFrameRadius(10);
			QCOMPARE(QGIUtility::selectionFrameRadius(), 10);
			QVERIFY2(!cornerDrawn(), "with radius 10 the corner is rounded off");
		}

		void squareWithZero()
		{
			QGIUtility::setSelectionFrameRadius(0);
			QCOMPARE(QGIUtility::selectionFrameRadius(), 0);
			QCOMPARE(QSettings().value(QStringLiteral("diagrameditor/selection_frame_radius")).toInt(), 0);
			QVERIFY2(cornerDrawn(), "with radius 0 the frame reaches the corner");
		}

		void bounded()
		{
			QGIUtility::setSelectionFrameRadius(25);
			QCOMPARE(QGIUtility::selectionFrameRadius(), 10);
			QGIUtility::setSelectionFrameRadius(-3);
			QCOMPARE(QGIUtility::selectionFrameRadius(), 0);
			QGIUtility::setSelectionFrameRadius(10);
		}
};

QET_APP_TEST_MAIN(tst_selectionframeradius)
#include "tst_selectionframeradius.moc"
