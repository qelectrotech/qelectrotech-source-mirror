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
#include <QtTest>
#include <QGraphicsTextItem>

#include "textanchor.h"

static bool samePoint(const QPointF &a, const QPointF &b)
{
	return qAbs(a.x() - b.x()) < 1e-9 && qAbs(a.y() - b.y()) < 1e-9;
}

class tst_textanchor : public QObject
{
	Q_OBJECT

private slots:
	// The texts of issue #1155: Position X/Y is the point chosen by the
	// alignment, so a right-aligned label keeps its right edge at X.
	void pointOfTheText_data()
	{
		QTest::addColumn<int>("alignment");
		QTest::addColumn<qreal>("fx");	// fraction of the width
		QTest::addColumn<qreal>("fy");	// fraction of the height

		QTest::newRow("top left")      << int(Qt::AlignTop | Qt::AlignLeft)         << 0.0 << 0.0;
		QTest::newRow("top right")     << int(Qt::AlignTop | Qt::AlignRight)        << 1.0 << 0.0;
		QTest::newRow("centre")        << int(Qt::AlignVCenter | Qt::AlignHCenter)  << 0.5 << 0.5;
		QTest::newRow("middle left")   << int(Qt::AlignVCenter | Qt::AlignLeft)     << 0.0 << 0.5;
		QTest::newRow("middle right")  << int(Qt::AlignVCenter | Qt::AlignRight)    << 1.0 << 0.5;
		QTest::newRow("bottom centre") << int(Qt::AlignBottom | Qt::AlignHCenter)   << 0.5 << 1.0;
	}

	void pointOfTheText()
	{
		QFETCH(int, alignment);
		QFETCH(qreal, fx);
		QFETCH(qreal, fy);

		QGraphicsTextItem text(QStringLiteral("XT2:12"));
		text.setPos(-52, -3);
		const QRectF r = text.boundingRect();

		const QPointF expected(-52 + fx * r.width(), -3 + fy * r.height());
		QVERIFY(samePoint(TextAnchor::pos(&text, Qt::Alignment(alignment)), expected));
	}

	// Top-left alignment keeps today's numbers: the anchor is pos(),
	// whatever the rotation or the rotation point.
	void topLeftIsPos()
	{
		QGraphicsTextItem text(QStringLiteral("-K1"));
		text.setPos(12.5, -40);
		for (qreal angle : {0.0, 90.0, 180.0, 270.0, 30.0}) {
			text.setRotation(angle);
			for (bool centre : {false, true}) {
				text.setTransformOriginPoint(centre ? text.boundingRect().center() : QPointF());
				QVERIFY(samePoint(TextAnchor::pos(&text, Qt::AlignTop | Qt::AlignLeft), text.pos()));
			}
		}
	}

	// A rotated text: the anchor is the point of the text where it is
	// drawn, e.g. the right edge of a text turned by 90° is below pos().
	void rotatedTextAnchorIsDrawnPoint()
	{
		QGraphicsTextItem text(QStringLiteral("XT2:12"));
		text.setPos(100, 100);
		text.setRotation(90);
		const QRectF r = text.boundingRect();

		const QPointF anchor = TextAnchor::pos(&text, Qt::AlignTop | Qt::AlignRight);
		QVERIFY(samePoint(anchor, text.mapToParent(QPointF(r.right(), 0))));
		QVERIFY(samePoint(anchor, QPointF(100, 100 + r.right())));
	}

	// Typing an anchor puts that point of the text there, for every
	// alignment, rotation and rotation point.
	void itemPosForRoundTrips()
	{
		const QList<Qt::Alignment> horizontal {Qt::AlignLeft, Qt::AlignHCenter, Qt::AlignRight};
		const QList<Qt::Alignment> vertical   {Qt::AlignTop, Qt::AlignVCenter, Qt::AlignBottom};

		QGraphicsTextItem text(QStringLiteral("Right."));
		text.setPos(-81, -13);
		const QPointF anchor(-52, -3);

		for (Qt::Alignment h : horizontal)
			for (Qt::Alignment v : vertical)
				for (qreal angle : {0.0, 90.0, 270.0, 45.0})
					for (bool centre : {false, true}) {
						text.setRotation(angle);
						text.setTransformOriginPoint(centre ? text.boundingRect().center() : QPointF());
						text.setPos(TextAnchor::itemPosFor(&text, h | v, anchor));
						QVERIFY2(samePoint(TextAnchor::pos(&text, h | v), anchor),
								 qPrintable(QStringLiteral("h %1 v %2 angle %3 centre %4")
											.arg(int(h)).arg(int(v)).arg(angle).arg(centre)));
					}
	}

	// A longer text with the same anchor keeps its right edge in place:
	// the gap between a label and its symbol does not change (#1155).
	void rightEdgeStaysWhenTextGrows()
	{
		QGraphicsTextItem text(QStringLiteral("XT2:2"));
		const QPointF anchor(-10, 0);
		const Qt::Alignment right = Qt::AlignTop | Qt::AlignRight;
		text.setPos(TextAnchor::itemPosFor(&text, right, anchor));
		const qreal short_left = text.pos().x();

		text.setPlainText(QStringLiteral("XT2:12"));
		text.setPos(TextAnchor::itemPosFor(&text, right, anchor));

		QVERIFY(text.pos().x() < short_left);
		QCOMPARE(text.pos().x() + text.boundingRect().right(), anchor.x());
	}
};

QTEST_MAIN(tst_textanchor)
#include "tst_textanchor.moc"
