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
#include "diagram.h"
#include "diagramcontent.h"
#include "qetproject.h"
#include "qetgraphicsitem/dimensionitem.h"
#include "undocommand/addgraphicsobjectcommand.h"
#include "undocommand/deleteqgraphicsitemcommand.h"
#include "QPropertyUndoCommand/qpropertyundocommand.h"

class tst_DimensionItem : public QObject
{
	Q_OBJECT

private slots:
	void testDefaultValues();
	void testScaleFactorAndFormatting();
	void testCustomText();
	void testOffsetAndGeometry();
	void testPropertiesStruct();
	void testXmlSerializationRoundtrip();
	void testUndoRedoAdd();
	void testUndoRedoDelete();
	void testUndoRedoProperties();
};

void tst_DimensionItem::testDefaultValues()
{
	DimensionItem dim;
	QCOMPARE(dim.type(), DimensionItem::Type);
	QCOMPARE(dim.isAutoValue(), true);
	QCOMPARE(dim.offset(), 20.0);
	QCOMPARE(dim.mode(), DimensionItem::Linear);
	QCOMPARE(dim.terminationStyle(), DimensionItem::Arrows);
	QCOMPARE(dim.isFramed(), true);
	QCOMPARE(dim.useComma(), true);
	QCOMPARE(dim.precision(), 1);
}

void tst_DimensionItem::testScaleFactorAndFormatting()
{
	// Strictly 2 px = 1.0 mm
	// 200 px = 100.0 mm
	DimensionItem dim(QPointF(0, 0), QPointF(200, 0));
	QCOMPARE(dim.dimensionValueMM(), 100.0);
	QCOMPARE(dim.displayText(), QStringLiteral("100,0"));

	dim.setUseComma(false);
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

	// Formatting with prefix, suffix and unit
	dim.setPrefix(QStringLiteral("DM"));
	dim.setSuffix(QStringLiteral(" max"));
	dim.setUnit(QStringLiteral("mm"));
	dim.setShowUnit(true);
	dim.setUseComma(true);
	QCOMPARE(dim.displayText(), QStringLiteral("DM1690,0 mm max"));
}

void tst_DimensionItem::testCustomText()
{
	DimensionItem dim(QPointF(0, 0), QPointF(200, 0));
	dim.setUseComma(false);
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

void tst_DimensionItem::testPropertiesStruct()
{
	DimensionItem dim;
	DimensionProperties p = dim.properties();
	QCOMPARE(p.isFramed, true);
	QCOMPARE(p.useComma, true);
	QCOMPARE(p.precision, 1);

	p.prefix = QStringLiteral("M");
	p.customText = QStringLiteral("12");
	p.isAutoValue = false;
	p.precision = 2;
	p.isFramed = false;
	dim.setProperties(p);

	DimensionProperties p2 = dim.properties();
	QCOMPARE(p2.prefix, QStringLiteral("M"));
	QCOMPARE(p2.customText, QStringLiteral("12"));
	QCOMPARE(p2.isAutoValue, false);
	QCOMPARE(p2.precision, 2);
	QCOMPARE(p2.isFramed, false);
	QCOMPARE(p, p2);
}

void tst_DimensionItem::testXmlSerializationRoundtrip()
{
	DimensionItem original(QPointF(50, 100), QPointF(250, 100), 35.0, DimensionItem::Chain);
	original.setAutoValue(false);
	original.setCustomText(QStringLiteral("TEST_DIM"));
	original.setTerminationStyle(DimensionItem::Slashes);
	original.setPrefix(QStringLiteral("L="));
	original.setFramed(true);

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
	QCOMPARE(xml.attribute(QStringLiteral("prefix")), QStringLiteral("L="));
	QCOMPARE(xml.attribute(QStringLiteral("framed")), QStringLiteral("true"));
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
	QCOMPARE(restored.prefix(), QStringLiteral("L="));
	QCOMPARE(restored.isFramed(), true);
	QCOMPARE(restored.mode(), DimensionItem::Chain);
	QCOMPARE(restored.terminationStyle(), DimensionItem::Slashes);
	QCOMPARE(restored.dimensionValueMM(), 100.0);
	QCOMPARE(restored.displayText(), QStringLiteral("L=TEST_DIM"));
}

void tst_DimensionItem::testUndoRedoAdd()
{
	QETProject project;
	Diagram *diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);
	auto *dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0), 20.0);

	// Initial: not yet committed via undo stack
	QCOMPARE(diagram->items().contains(dim), false);

	// Add via command
	diagram->undoStack().push(new AddGraphicsObjectCommand(dim, diagram));
	QCOMPARE(diagram->items().contains(dim), true);
	QCOMPARE(dim->scene(), diagram);

	// Undo: should remove item from diagram
	diagram->undoStack().undo();
	QCOMPARE(diagram->items().contains(dim), false);
	QCOMPARE(dim->scene(), nullptr);

	// Redo: should put item back into diagram
	diagram->undoStack().redo();
	QCOMPARE(diagram->items().contains(dim), true);
	QCOMPARE(dim->scene(), diagram);
}

void tst_DimensionItem::testUndoRedoDelete()
{
	QETProject project;
	Diagram *diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);
	auto *dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0), 20.0);
	diagram->undoStack().push(new AddGraphicsObjectCommand(dim, diagram));
	QCOMPARE(diagram->items().contains(dim), true);

	// Select and build DiagramContent
	dim->setSelected(true);
	DiagramContent content(diagram, true);
	QCOMPARE(content.m_dimensions.count(), 1);

	// Delete item via command
	diagram->undoStack().push(new DeleteQGraphicsItemCommand(diagram, content));
	QCOMPARE(diagram->items().contains(dim), false);

	// Undo deletion: restored!
	diagram->undoStack().undo();
	QCOMPARE(diagram->items().contains(dim), true);
	QCOMPARE(dim->scene(), diagram);

	// Redo deletion: removed again!
	diagram->undoStack().redo();
	QCOMPARE(diagram->items().contains(dim), false);
	QCOMPARE(dim->scene(), nullptr);
}

void tst_DimensionItem::testUndoRedoProperties()
{
	QETProject project;
	Diagram *diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);
	auto *dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0), 20.0);
	diagram->undoStack().push(new AddGraphicsObjectCommand(dim, diagram));

	// Test QPropertyUndoCommand for auto value
	QCOMPARE(dim->isAutoValue(), true);
	diagram->undoStack().push(new QPropertyUndoCommand(dim, "isAutoValue", true, false));
	QCOMPARE(dim->isAutoValue(), false);

	diagram->undoStack().undo();
	QCOMPARE(dim->isAutoValue(), true);

	diagram->undoStack().redo();
	QCOMPARE(dim->isAutoValue(), false);

	// Test QPropertyUndoCommand for customText
	diagram->undoStack().push(new QPropertyUndoCommand(dim, "customText", QString(), QStringLiteral("L=500")));
	QCOMPARE(dim->customText(), QStringLiteral("L=500"));

	diagram->undoStack().undo();
	QCOMPARE(dim->customText(), QString());

	diagram->undoStack().redo();
	QCOMPARE(dim->customText(), QStringLiteral("L=500"));

	// Test offset undo/redo
	QCOMPARE(dim->offset(), 20.0);
	diagram->undoStack().push(new QPropertyUndoCommand(dim, "offset", 20.0, 45.0));
	QCOMPARE(dim->offset(), 45.0);

	diagram->undoStack().undo();
	QCOMPARE(dim->offset(), 20.0);

	diagram->undoStack().redo();
	QCOMPARE(dim->offset(), 45.0);
}

QTEST_MAIN(tst_DimensionItem)
#include "tst_dimensionitem.moc"
