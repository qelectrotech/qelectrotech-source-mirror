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
#include <QDomDocument>
#include <QDomElement>
#include "qetgraphicsitem/dimensionitem.h"

class tst_DimensionItem : public QObject
{
	Q_OBJECT

private slots:
	void testDefaultValues();
	void testScaleFactorAndFormatting();
	void testCustomText();
	void testOffsetAndGeometry();
	void testXmlSerializationRoundtrip();
};

void tst_DimensionItem::testDefaultValues()
{
	DimensionItem dim;
	QCOMPARE(dim.type(), DimensionItem::Type);
	QCOMPARE(dim.isAutoValue(), true);
	QCOMPARE(dim.offset(), 20.0);
	QCOMPARE(dim.mode(), DimensionItem::Linear);
	QCOMPARE(dim.terminationStyle(), DimensionItem::Arrows);
}

void tst_DimensionItem::testScaleFactorAndFormatting()
{
	// Strictly 2 px = 1.0 mm
	// 200 px = 100.0 mm
	DimensionItem dim(QPointF(0, 0), QPointF(200, 0));
	QCOMPARE(dim.dimensionValueMM(), 100.0);
	QCOMPARE(dim.displayText(), QStringLiteral("100.0"));

	// 160 px = 80.0 mm
	dim.setP2(QPointF(0, 160));
	QCOMPARE(dim.dimensionValueMM(), 80.0);
	QCOMPARE(dim.displayText(), QStringLiteral("80.0"));

	// 3380 px = 1690.0 mm
	dim.setP1(QPointF(100, 100));
	dim.setP2(QPointF(3480, 100));
	QCOMPARE(dim.dimensionValueMM(), 1690.0);
	QCOMPARE(dim.displayText(), QStringLiteral("1690.0"));
}

void tst_DimensionItem::testCustomText()
{
	DimensionItem dim(QPointF(0, 0), QPointF(200, 0));
	QCOMPARE(dim.displayText(), QStringLiteral("100.0"));

	dim.setAutoValue(false);
	dim.setCustomText(QStringLiteral("CUSTOM_100"));
	QCOMPARE(dim.displayText(), QStringLiteral("CUSTOM_100"));

	dim.setAutoValue(true);
	QCOMPARE(dim.displayText(), QStringLiteral("100.0"));
}

void tst_DimensionItem::testOffsetAndGeometry()
{
	DimensionItem dim(QPointF(0, 0), QPointF(100, 0), 30.0);
	QCOMPARE(dim.offset(), 30.0);

	dim.setOffset(-45.0);
	QCOMPARE(dim.offset(), -45.0);

	QRectF br = dim.boundingRect();
	QVERIFY(!br.isEmpty());
	QVERIFY(br.contains(QPointF(50, 0)));

	QPainterPath sh = dim.shape();
	QVERIFY(!sh.isEmpty());
}

void tst_DimensionItem::testXmlSerializationRoundtrip()
{
	DimensionItem original(QPointF(50, 100), QPointF(250, 100), 35.0, DimensionItem::Chain);
	original.setAutoValue(false);
	original.setCustomText(QStringLiteral("TEST_DIM"));
	original.setTerminationStyle(DimensionItem::Slashes);

	QDomDocument doc;
	QDomElement xml = original.toXml(doc);

	QCOMPARE(xml.tagName(), QStringLiteral("dimension"));
	QCOMPARE(xml.attribute(QStringLiteral("x1")), QStringLiteral("50.00"));
	QCOMPARE(xml.attribute(QStringLiteral("y1")), QStringLiteral("100.00"));
	QCOMPARE(xml.attribute(QStringLiteral("x2")), QStringLiteral("250.00"));
	QCOMPARE(xml.attribute(QStringLiteral("y2")), QStringLiteral("100.00"));
	QCOMPARE(xml.attribute(QStringLiteral("offset")), QStringLiteral("35.00"));
	QCOMPARE(xml.attribute(QStringLiteral("auto")), QStringLiteral("false"));
	QCOMPARE(xml.attribute(QStringLiteral("text")), QStringLiteral("TEST_DIM"));
	QCOMPARE(xml.attribute(QStringLiteral("mode")), QStringLiteral("chain"));
	QCOMPARE(xml.attribute(QStringLiteral("style")), QStringLiteral("slashes"));

	// Deserialize into a new item
	DimensionItem restored;
	restored.fromXml(xml);

	QCOMPARE(restored.p1(), QPointF(50, 100));
	QCOMPARE(restored.p2(), QPointF(250, 100));
	QCOMPARE(restored.offset(), 35.0);
	QCOMPARE(restored.isAutoValue(), false);
	QCOMPARE(restored.customText(), QStringLiteral("TEST_DIM"));
	QCOMPARE(restored.mode(), DimensionItem::Chain);
	QCOMPARE(restored.terminationStyle(), DimensionItem::Slashes);
	QCOMPARE(restored.dimensionValueMM(), 100.0);
	QCOMPARE(restored.displayText(), QStringLiteral("TEST_DIM"));
}

QTEST_MAIN(tst_DimensionItem)
#include "tst_dimensionitem.moc"

