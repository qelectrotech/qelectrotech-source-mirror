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
#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "diagram.h"
#include "diagramcontent.h"
#include "qetgraphicsitem/dimensionitem.h"
#include "qetproject.h"
#include "ui/dimensionpropertiesdialog.h"
#include "editor/esevent/elementmeasureitem.h"
#include "editor/esevent/eseventmeasure.h"
#include "editor/elementscene.h"
#include <QImage>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include "undocommand/addgraphicsobjectcommand.h"
#include "undocommand/deleteqgraphicsitemcommand.h"
#include "undocommand/movegraphicsitemcommand.h"

#include <QCheckBox>
#include "diagramevent/diagrameventadddimension.h"
#include <QComboBox>
#include <QDomDocument>
#include <QDomElement>
#include <QtTest>

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
	void testUndoRedoDialog();
	void testEscapeCancelsTool();
	void testFormatPainterDimension();
	void testDialogButtons();
	void testChainGroupIdAndSegments();
	void testChainSelectionSync();
	void testIntermediatePointMoveAndUndo();
	void testChainOffsetMoveAndUndo();
	void testChainAxisConstraintAndMoveAll();
	void testDialogMultiSegmentPropertyEdit();
	void testFramedBoxPainting();
	void testElementMeasureItemZoomScaling();
	void testElementEditorMeasureTeardown();
	void testTextRotationAndArrowSizeMax();
};

void tst_DimensionItem::testDefaultValues()
{
	DimensionItem dim;
	QCOMPARE(dim.type(), DimensionItem::Type);
	QCOMPARE(dim.isAutoValue(), true);
	QCOMPARE(dim.offset(), 20.0);
	QCOMPARE(dim.mode(), DimensionItem::Linear);
	QCOMPARE(dim.terminationStyle(), DimensionItem::Arrows);
	QCOMPARE(dim.isFramed(), false);
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
	QVERIFY(! br.isEmpty());
	QVERIFY(br.contains(QPointF(50, 0)));

	QPainterPath sh = dim.shape();
	QVERIFY(! sh.isEmpty());
}

void tst_DimensionItem::testPropertiesStruct()
{
	DimensionItem		dim;
	DimensionProperties p = dim.properties();
	QCOMPARE(p.isFramed, false);
	QCOMPARE(p.useComma, true);
	QCOMPARE(p.precision, 1);

	p.prefix	  = QStringLiteral("M");
	p.customText  = QStringLiteral("12");
	p.isAutoValue = false;
	p.precision	  = 2;
	p.isFramed	  = true;
	dim.setProperties(p);

	DimensionProperties p2 = dim.properties();
	QCOMPARE(p2.prefix, QStringLiteral("M"));
	QCOMPARE(p2.customText, QStringLiteral("12"));
	QCOMPARE(p2.isAutoValue, false);
	QCOMPARE(p2.precision, 2);
	QCOMPARE(p2.isFramed, true);
	QCOMPARE(p, p2);
}

void tst_DimensionItem::testXmlSerializationRoundtrip()
{
	DimensionItem original(
		QPointF(50, 100),
		QPointF(250, 100),
		35.0,
		DimensionItem::Chain);
	original.setAutoValue(false);
	original.setCustomText(QStringLiteral("TEST_DIM"));
	original.setTerminationStyle(DimensionItem::Slashes);
	original.setPrefix(QStringLiteral("L="));
	original.setFramed(true);

	QDomDocument doc;
	QDomElement	 xml = original.toXml(doc);

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
	Diagram*   diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);
	auto* dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0), 20.0);

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
	Diagram*   diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);
	auto* dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0), 20.0);
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
	Diagram*   diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);
	auto* dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0), 20.0);
	diagram->undoStack().push(new AddGraphicsObjectCommand(dim, diagram));

	// Test QPropertyUndoCommand for auto value
	QCOMPARE(dim->isAutoValue(), true);
	diagram->undoStack().push(
		new QPropertyUndoCommand(dim, "isAutoValue", true, false));
	QCOMPARE(dim->isAutoValue(), false);

	diagram->undoStack().undo();
	QCOMPARE(dim->isAutoValue(), true);

	diagram->undoStack().redo();
	QCOMPARE(dim->isAutoValue(), false);

	// Test QPropertyUndoCommand for customText
	diagram->undoStack().push(new QPropertyUndoCommand(
		dim,
		"customText",
		QString(),
		QStringLiteral("L=500")));
	QCOMPARE(dim->customText(), QStringLiteral("L=500"));

	diagram->undoStack().undo();
	QCOMPARE(dim->customText(), QString());

	diagram->undoStack().redo();
	QCOMPARE(dim->customText(), QStringLiteral("L=500"));

	// Test offset undo/redo
	QCOMPARE(dim->offset(), 20.0);
	diagram->undoStack().push(
		new QPropertyUndoCommand(dim, "offset", 20.0, 45.0));
	QCOMPARE(dim->offset(), 45.0);

	diagram->undoStack().undo();
	QCOMPARE(dim->offset(), 20.0);

	diagram->undoStack().redo();
	QCOMPARE(dim->offset(), 45.0);
}

void tst_DimensionItem::testUndoRedoDialog()
{
	QETProject project;
	Diagram*   diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);
	auto* dim = new DimensionItem(QPointF(0, 0), QPointF(200, 0), 20.0);
	diagram->undoStack().push(new AddGraphicsObjectCommand(dim, diagram));

	int initialUndoCount = diagram->undoStack().count();
	QCOMPARE(dim->prefix(), QString());
	QCOMPARE(dim->isFramed(), false);

	{
		DimensionPropertiesDialog dialog(dim);
		auto* prefixCb = dialog.findChild<QComboBox*>("m_prefix_cb");
		auto* framedCb = dialog.findChild<QCheckBox*>("m_framed_cb");
		QVERIFY(prefixCb != nullptr);
		QVERIFY(framedCb != nullptr);

		prefixCb->setCurrentText(QStringLiteral("⌀"));
		framedCb->setChecked(true);

		// Accept dialog -> commits ChangeDimensionPropertiesCommand
		dialog.accept();
	}

	QCOMPARE(diagram->undoStack().count(), initialUndoCount + 1);
	QCOMPARE(dim->prefix(), QStringLiteral("⌀"));
	QCOMPARE(dim->isFramed(), true);

	// Undo dialog changes
	diagram->undoStack().undo();
	QCOMPARE(dim->prefix(), QString());
	QCOMPARE(dim->isFramed(), false);

	// Redo dialog changes
	diagram->undoStack().redo();
	QCOMPARE(dim->prefix(), QStringLiteral("⌀"));
	QCOMPARE(dim->isFramed(), true);
}

void tst_DimensionItem::testEscapeCancelsTool()
{
	QETProject project;
	Diagram *diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);

	auto *tool = new DiagramEventAddDimension(diagram, DimensionItem::Linear);
	diagram->setEventInterface(tool);

	QCOMPARE(diagram->eventInterfaceIsRunning(), true);

	// Send Escape key event to diagram
	QKeyEvent escEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
	QCoreApplication::sendEvent(diagram, &escEvent);

	// The event should be accepted and the tool aborted/cleared
	QCOMPARE(escEvent.isAccepted(), true);
	QCOMPARE(diagram->eventInterfaceIsRunning(), false);
}

void tst_DimensionItem::testFormatPainterDimension()
{
	QETProject project;
	Diagram *diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);

	auto *sourceDim = new DimensionItem(QPointF(0, 0), QPointF(100, 0), 20.0);
	sourceDim->setPrefix(QStringLiteral("⌀"));
	sourceDim->setPrecision(2);
	sourceDim->setLineWidth(2.0);
	sourceDim->setTerminationStyle(DimensionItem::Slashes);
	diagram->undoStack().push(new AddGraphicsObjectCommand(sourceDim, diagram));

	auto *targetDim1 = new DimensionItem(QPointF(0, 100), QPointF(200, 100), 25.0);
	auto *targetDim2 = new DimensionItem(QPointF(0, 200), QPointF(300, 200), 30.0);
	diagram->undoStack().push(new AddGraphicsObjectCommand(targetDim1, diagram));
	diagram->undoStack().push(new AddGraphicsObjectCommand(targetDim2, diagram));

	// Copy format from source
	DimensionFormatPattern::setCurrentPattern(sourceDim->properties());
	QCOMPARE(DimensionFormatPattern::hasPattern(), true);

	// Apply format to targets via ApplyDimensionFormatCommand
	QList<DimensionItem *> targets{targetDim1, targetDim2};
	diagram->undoStack().push(new ApplyDimensionFormatCommand(targets, DimensionFormatPattern::currentPattern()));

	// Verify targets received source format
	QCOMPARE(targetDim1->prefix(), QStringLiteral("⌀"));
	QCOMPARE(targetDim1->precision(), 2);
	QCOMPARE(targetDim1->lineWidth(), 2.0);
	QCOMPARE(targetDim1->terminationStyle(), DimensionItem::Slashes);

	QCOMPARE(targetDim2->prefix(), QStringLiteral("⌀"));
	QCOMPARE(targetDim2->precision(), 2);
	QCOMPARE(targetDim2->lineWidth(), 2.0);
	QCOMPARE(targetDim2->terminationStyle(), DimensionItem::Slashes);

	// Verify length & offset of targetDim1 were NOT overwritten
	QCOMPARE(targetDim1->dimensionValueMM(), 100.0);
	QCOMPARE(targetDim1->offset(), 25.0);

	// Test Undo
	diagram->undoStack().undo();
	QCOMPARE(targetDim1->prefix(), QString());
	QCOMPARE(targetDim1->precision(), 1);
	QCOMPARE(targetDim1->terminationStyle(), DimensionItem::Arrows);

	// Test Redo
	diagram->undoStack().redo();
	QCOMPARE(targetDim1->prefix(), QStringLiteral("⌀"));
	QCOMPARE(targetDim1->precision(), 2);
	QCOMPARE(targetDim1->terminationStyle(), DimensionItem::Slashes);
}

void tst_DimensionItem::testDialogButtons()
{
	QETProject project;
	Diagram *diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);

	auto *dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0), 20.0);
	diagram->undoStack().push(new AddGraphicsObjectCommand(dim, diagram));

	int undoCountBefore = diagram->undoStack().count();

	// Test Ok button via m_buttonBox
	{
		DimensionPropertiesDialog dialog(dim);
		auto *buttonBox = dialog.findChild<QDialogButtonBox*>("m_buttonBox");
		QVERIFY(buttonBox != nullptr);
		auto *prefixCb = dialog.findChild<QComboBox*>("m_prefix_cb");
		QVERIFY(prefixCb != nullptr);

		prefixCb->setCurrentText(QStringLiteral("L="));

		// Click OK button
		buttonBox->button(QDialogButtonBox::Ok)->click();
	}

	QCOMPARE(diagram->undoStack().count(), undoCountBefore + 1);
	QCOMPARE(dim->prefix(), QStringLiteral("L="));

	// Test Cancel button via m_buttonBox
	{
		DimensionPropertiesDialog dialog(dim);
		auto *buttonBox = dialog.findChild<QDialogButtonBox*>("m_buttonBox");
		auto *prefixCb = dialog.findChild<QComboBox*>("m_prefix_cb");

		prefixCb->setCurrentText(QStringLiteral("TEST_DISCARD"));

		// Click Cancel button
		buttonBox->button(QDialogButtonBox::Cancel)->click();
	}

	// Should not have pushed anything new, and prefix should remain L=
	QCOMPARE(diagram->undoStack().count(), undoCountBefore + 1);
	QCOMPARE(dim->prefix(), QStringLiteral("L="));
}

void tst_DimensionItem::testChainGroupIdAndSegments()
{
	QETProject project;
	Diagram *diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);

	QUuid chainId = QUuid::createUuid();

	auto *s0 = new DimensionItem(QPointF(0, 0), QPointF(100, 0), 20.0, DimensionItem::Chain);
	s0->setChainId(chainId);
	auto *s1 = new DimensionItem(QPointF(100, 0), QPointF(250, 0), 20.0, DimensionItem::Chain);
	s1->setChainId(chainId);
	auto *s2 = new DimensionItem(QPointF(250, 0), QPointF(400, 0), 20.0, DimensionItem::Chain);
	s2->setChainId(chainId);

	diagram->addItem(s0);
	diagram->addItem(s1);
	diagram->addItem(s2);

	QList<DimensionItem *> segments = s1->chainSegments();
	QCOMPARE(segments.size(), 3);
	QCOMPARE(segments[0], s0);
	QCOMPARE(segments[1], s1);
	QCOMPARE(segments[2], s2);

	QCOMPARE(s0->dimensionValueMM(), 50.0);
	QCOMPARE(s1->dimensionValueMM(), 75.0);
	QCOMPARE(s2->dimensionValueMM(), 75.0);
}

void tst_DimensionItem::testChainSelectionSync()
{
	QETProject project;
	Diagram *diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);

	QUuid chainId = QUuid::createUuid();

	auto *s0 = new DimensionItem(QPointF(0, 0), QPointF(100, 0), 20.0, DimensionItem::Chain);
	s0->setChainId(chainId);
	auto *s1 = new DimensionItem(QPointF(100, 0), QPointF(250, 0), 20.0, DimensionItem::Chain);
	s1->setChainId(chainId);
	auto *s2 = new DimensionItem(QPointF(250, 0), QPointF(400, 0), 20.0, DimensionItem::Chain);
	s2->setChainId(chainId);

	diagram->addItem(s0);
	diagram->addItem(s1);
	diagram->addItem(s2);

	// Select s1: s0 and s2 must become selected too (unified chain selection)
	s1->setSelected(true);
	QCOMPARE(s0->isSelected(), true);
	QCOMPARE(s1->isSelected(), true);
	QCOMPARE(s2->isSelected(), true);

	// Deselect s1: s0 and s2 must become deselected
	s1->setSelected(false);
	QCOMPARE(s0->isSelected(), false);
	QCOMPARE(s1->isSelected(), false);
	QCOMPARE(s2->isSelected(), false);
}

void tst_DimensionItem::testIntermediatePointMoveAndUndo()
{
	QETProject project;
	Diagram *diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);

	QUuid chainId = QUuid::createUuid();

	auto *s0 = new DimensionItem(QPointF(0, 0), QPointF(100, 0), 20.0, DimensionItem::Chain);
	s0->setChainId(chainId);
	auto *s1 = new DimensionItem(QPointF(100, 0), QPointF(250, 0), 20.0, DimensionItem::Chain);
	s1->setChainId(chainId);
	auto *s2 = new DimensionItem(QPointF(250, 0), QPointF(400, 0), 20.0, DimensionItem::Chain);
	s2->setChainId(chainId);

	diagram->addItem(s0);
	diagram->addItem(s1);
	diagram->addItem(s2);

	// Intermediate point P1 connects s0->p2 and s1->p1 at (100, 0)
	// Move intermediate point from (100, 0) to (140, 0)
	QList<MoveDimensionPointCommand::PointChange> changes;
	changes.append({ s0, s0->p1(), s0->p2(), s0->p1(), QPointF(140, 0) });
	changes.append({ s1, s1->p1(), s1->p2(), QPointF(140, 0), s1->p2() });

	diagram->undoStack().push(new MoveDimensionPointCommand(changes));

	// Verify both adjacent segments dynamically updated
	QCOMPARE(s0->p2(), QPointF(140, 0));
	QCOMPARE(s1->p1(), QPointF(140, 0));
	QCOMPARE(s0->dimensionValueMM(), 70.0);
	QCOMPARE(s1->dimensionValueMM(), 55.0);
	// s2 must remain untouched
	QCOMPARE(s2->p1(), QPointF(250, 0));
	QCOMPARE(s2->dimensionValueMM(), 75.0);

	// Verify displayText live updates
	QCOMPARE(s0->displayText(), QStringLiteral("70,0"));
	QCOMPARE(s1->displayText(), QStringLiteral("55,0"));

	// Undo
	diagram->undoStack().undo();
	QCOMPARE(s0->p2(), QPointF(100, 0));
	QCOMPARE(s1->p1(), QPointF(100, 0));
	QCOMPARE(s0->dimensionValueMM(), 50.0);
	QCOMPARE(s1->dimensionValueMM(), 75.0);
	QCOMPARE(s0->displayText(), QStringLiteral("50,0"));
	QCOMPARE(s1->displayText(), QStringLiteral("75,0"));

	// Redo
	diagram->undoStack().redo();
	QCOMPARE(s0->p2(), QPointF(140, 0));
	QCOMPARE(s1->p1(), QPointF(140, 0));
	QCOMPARE(s0->dimensionValueMM(), 70.0);
	QCOMPARE(s1->dimensionValueMM(), 55.0);
}

void tst_DimensionItem::testChainOffsetMoveAndUndo()
{
	QETProject project;
	Diagram *diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);

	QUuid chainId = QUuid::createUuid();

	auto *s0 = new DimensionItem(QPointF(0, 0), QPointF(100, 0), 20.0, DimensionItem::Chain);
	s0->setChainId(chainId);
	auto *s1 = new DimensionItem(QPointF(100, 0), QPointF(250, 0), 20.0, DimensionItem::Chain);
	s1->setChainId(chainId);

	diagram->addItem(s0);
	diagram->addItem(s1);

	// Change offset of whole chain to 35.0
	diagram->undoStack().push(new ChangeDimensionOffsetCommand({ s0, s1 }, 20.0, 35.0));

	QCOMPARE(s0->offset(), 35.0);
	QCOMPARE(s1->offset(), 35.0);

	// Undo
	diagram->undoStack().undo();
	QCOMPARE(s0->offset(), 20.0);
	QCOMPARE(s1->offset(), 20.0);

	// Redo
	diagram->undoStack().redo();
	QCOMPARE(s0->offset(), 35.0);
	QCOMPARE(s1->offset(), 35.0);
}

void tst_DimensionItem::testChainAxisConstraintAndMoveAll()
{
	QETProject project;
	Diagram *diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);

	QUuid chainId = QUuid::createUuid();

	// Vertical chain
	auto *v0 = new DimensionItem(QPointF(50, 0), QPointF(50, 100), 20.0, DimensionItem::Chain);
	v0->setChainId(chainId);
	auto *v1 = new DimensionItem(QPointF(50, 100), QPointF(50, 250), 20.0, DimensionItem::Chain);
	v1->setChainId(chainId);

	diagram->addItem(v0);
	diagram->addItem(v1);

	// Verify vertical chain orientation is detected
	QCOMPARE(v0->isVerticalChain(), true);
	QCOMPARE(v1->isVerticalChain(), true);

	// Horizontal chain
	QUuid hChainId = QUuid::createUuid();
	auto *h0 = new DimensionItem(QPointF(0, 80), QPointF(120, 80), 20.0, DimensionItem::Chain);
	h0->setChainId(hChainId);
	auto *h1 = new DimensionItem(QPointF(120, 80), QPointF(300, 80), 20.0, DimensionItem::Chain);
	h1->setChainId(hChainId);

	diagram->addItem(h0);
	diagram->addItem(h1);

	QCOMPARE(h0->isVerticalChain(), false);
	QCOMPARE(h1->isVerticalChain(), false);

	// Test whole-chain move via MoveGraphicsItemCommand (as generated by ElementsMover)
	v0->setSelected(true);
	QCOMPARE(v0->isSelected(), true);
	QCOMPARE(v1->isSelected(), true);

	DiagramContent content(diagram);
	QPointF delta(30.0, 40.0);

	// In QET, ElementsMover moves all selected items during the drag,
	// then pushes MoveGraphicsItemCommand to undoStack
	v0->setPos(v0->pos() + delta);
	v1->setPos(v1->pos() + delta);
	QCOMPARE(v0->pos(), delta);
	QCOMPARE(v1->pos(), delta);

	diagram->undoStack().push(new MoveGraphicsItemCommand(diagram, content, delta));

	// Undo: moves back to (0, 0)
	diagram->undoStack().undo();
	QCOMPARE(v0->pos(), QPointF(0, 0));
	QCOMPARE(v1->pos(), QPointF(0, 0));

	// Redo: moves back to delta
	diagram->undoStack().redo();
	QCOMPARE(v0->pos(), delta);
	QCOMPARE(v1->pos(), delta);
}

void tst_DimensionItem::testDialogMultiSegmentPropertyEdit()
{
	QETProject project;
	Diagram *diagram = project.addNewDiagram();
	QVERIFY(diagram != nullptr);

	QUuid chainId = QUuid::createUuid();

	auto *s0 = new DimensionItem(QPointF(0, 0), QPointF(100, 0), 20.0, DimensionItem::Chain);
	s0->setChainId(chainId);
	s0->setPrefix(QStringLiteral("L="));
	s0->setLineWidth(1.0);
	s0->setPrecision(1);
	s0->setUnit(QStringLiteral("mm"));

	auto *s1 = new DimensionItem(QPointF(100, 0), QPointF(250, 0), 20.0, DimensionItem::Chain);
	s1->setChainId(chainId);
	s1->setPrefix(QStringLiteral("B="));
	s1->setLineWidth(1.0);
	s1->setPrecision(2);
	s1->setUnit(QStringLiteral("mm"));

	auto *s2 = new DimensionItem(QPointF(250, 0), QPointF(400, 0), 20.0, DimensionItem::Chain);
	s2->setChainId(chainId);
	s2->setPrefix(QStringLiteral("H="));
	s2->setLineWidth(1.0);
	s2->setPrecision(3);
	s2->setUnit(QStringLiteral("mm"));

	diagram->addItem(s0);
	diagram->addItem(s1);
	diagram->addItem(s2);

	DimensionPropertiesDialog dialog(s0);
	QCOMPARE(dialog.selectedSegments().size(), 1);

	// Select all segments via selectAll checkbox
	auto *selectAllCb = dialog.findChild<QCheckBox *>("m_select_all_cb");
	QVERIFY(selectAllCb != nullptr);
	selectAllCb->click();

	QCOMPARE(dialog.selectedSegments().size(), 3);

	// Change only lineWidth and unit
	auto *lineWidthSb = dialog.findChild<QDoubleSpinBox *>("m_line_width_sb");
	QVERIFY(lineWidthSb != nullptr);
	lineWidthSb->setValue(2.5);

	auto *unitCb = dialog.findChild<QComboBox *>("m_unit_cb");
	QVERIFY(unitCb != nullptr);
	unitCb->setCurrentText(QStringLiteral("cm"));

	// Accept dialog
	dialog.accept();

	// Verify changed fields are updated on all segments
	QCOMPARE(s0->lineWidth(), 2.5);
	QCOMPARE(s1->lineWidth(), 2.5);
	QCOMPARE(s2->lineWidth(), 2.5);

	QCOMPARE(s0->unit(), QStringLiteral("cm"));
	QCOMPARE(s1->unit(), QStringLiteral("cm"));
	QCOMPARE(s2->unit(), QStringLiteral("cm"));

	// Verify untouched fields remain at their individual states
	QCOMPARE(s0->prefix(), QStringLiteral("L="));
	QCOMPARE(s1->prefix(), QStringLiteral("B="));
	QCOMPARE(s2->prefix(), QStringLiteral("H="));

	QCOMPARE(s0->precision(), 1);
	QCOMPARE(s1->precision(), 2);
	QCOMPARE(s2->precision(), 3);

	// Test Undo
	diagram->undoStack().undo();
	QCOMPARE(s0->lineWidth(), 1.0);
	QCOMPARE(s1->lineWidth(), 1.0);
	QCOMPARE(s2->lineWidth(), 1.0);
	QCOMPARE(s0->unit(), QStringLiteral("mm"));
	QCOMPARE(s1->unit(), QStringLiteral("mm"));
	QCOMPARE(s2->unit(), QStringLiteral("mm"));

	// Test Redo
	diagram->undoStack().redo();
	QCOMPARE(s0->lineWidth(), 2.5);
	QCOMPARE(s1->lineWidth(), 2.5);
	QCOMPARE(s2->lineWidth(), 2.5);
	QCOMPARE(s0->unit(), QStringLiteral("cm"));
	QCOMPARE(s1->unit(), QStringLiteral("cm"));
	QCOMPARE(s2->unit(), QStringLiteral("cm"));
}


void tst_DimensionItem::testFramedBoxPainting()
{
	DimensionItem item(QPointF(0, 0), QPointF(100, 0), 25.0);
	item.setFramed(true);
	item.setMaskBackground(true);
	QVERIFY(item.isFramed());
	QVERIFY(item.maskBackground());

	// Test painting on QImage
	QImage img(200, 200, QImage::Format_ARGB32_Premultiplied);
	img.fill(Qt::white);
	QPainter painter(&img);
	QStyleOptionGraphicsItem option;
	item.paint(&painter, &option);
	painter.end();

	// Verify bounding rect contains text area
	QRectF br = item.boundingRect();
	QVERIFY(br.width() > 100);
	QVERIFY(br.height() > 25);
}

void tst_DimensionItem::testElementMeasureItemZoomScaling()
{
	ElementMeasureItem item(QPointF(10, 20), QPointF(110, 70));
	QCOMPARE(item.deltaXMM(), 50.0);
	QCOMPARE(item.deltaYMM(), 25.0);
	qreal expectedLen = std::hypot(100.0, 50.0) / 2.0;
	QVERIFY(qAbs(item.lengthMM() - expectedLen) < 1e-4);

	QImage img(300, 300, QImage::Format_ARGB32_Premultiplied);
	img.fill(Qt::white);
	QPainter painter(&img);
	QStyleOptionGraphicsItem opt;

	// Normal 1:1
	item.paint(&painter, &opt);

	// Zoomed in 5x
	painter.save();
	painter.scale(5.0, 5.0);
	item.paint(&painter, &opt);
	painter.restore();

	// Zoomed out 0.2x
	painter.save();
	painter.scale(0.2, 0.2);
	item.paint(&painter, &opt);
	painter.restore();

	painter.end();

	QRectF br = item.boundingRect();
	QVERIFY(!br.isEmpty());
}

void tst_DimensionItem::testElementEditorMeasureTeardown()
{
	// Verify that setting and clearing ESEventMeasure or destroying scene causes NO crash
	ElementScene scene(nullptr, nullptr);
	auto *measureTool = new ESEventMeasure(&scene);
	scene.setEventInterface(measureTool);

	// Normal clear
	scene.clearEventInterface();
	QVERIFY(scene.behavior() == ElementScene::Behavior::Normal);

	// Second instance destroyed alongside scene destructor
	auto *measureTool2 = new ESEventMeasure(&scene);
	scene.setEventInterface(measureTool2);
	// scene goes out of scope here: destructor cleans up interface safely
}

void tst_DimensionItem::testTextRotationAndArrowSizeMax()
{
	DimensionItem dim;
	QCOMPARE(dim.textRotation(), -1);
	QCOMPARE(dim.isTextHorizontal(), false);

	// Rotation settings
	dim.setTextRotation(0);
	QCOMPARE(dim.textRotation(), 0);
	QCOMPARE(dim.isTextHorizontal(), true);

	dim.setTextRotation(90);
	QCOMPARE(dim.textRotation(), 90);
	QCOMPARE(dim.isTextHorizontal(), false);

	dim.setTextRotation(180);
	QCOMPARE(dim.textRotation(), 180);

	dim.setTextRotation(270);
	QCOMPARE(dim.textRotation(), 270);

	// Arrow size up to 100.0
	dim.setArrowSize(100.0);
	QCOMPARE(dim.arrowSize(), 100.0);

	// XML serialization
	QDomDocument doc;
	QDomElement xml = dim.toXml(doc);
	QCOMPARE(xml.attribute("text_rotation"), QStringLiteral("270"));
	QCOMPARE(xml.attribute("arrow_size"), QStringLiteral("100.00"));

	// Backward compatibility XML (has horizontal="true", no text_rotation)
	QDomDocument docOld;
	QDomElement oldXml = docOld.createElement("dimension");
	oldXml.setAttribute("horizontal", "true");
	DimensionItem dimOld;
	dimOld.fromXml(oldXml);
	QCOMPARE(dimOld.isTextHorizontal(), true);
	QCOMPARE(dimOld.textRotation(), 0);

	// Unified vertical line orientation in Auto mode (-1)
	// Line 1: top to bottom (0, 0) to (0, 100)
	DimensionItem vDim1(QPointF(0, 0), QPointF(0, 100));
	vDim1.setTextRotation(-1);
	QPointF p1_a, p2_a, p1_ext_a, p2_ext_a, mid_a, udir_a, ndir_a;
	qreal angle1 = 0.0;
	vDim1.calculateGeometry(p1_a, p2_a, p1_ext_a, p2_ext_a, mid_a, angle1, udir_a, ndir_a);

	// Line 2: bottom to top (0, 100) to (0, 0)
	DimensionItem vDim2(QPointF(0, 100), QPointF(0, 0));
	vDim2.setTextRotation(-1);
	QPointF p1_b, p2_b, p1_ext_b, p2_ext_b, mid_b, udir_b, ndir_b;
	qreal angle2 = 0.0;
	vDim2.calculateGeometry(p1_b, p2_b, p1_ext_b, p2_ext_b, mid_b, angle2, udir_b, ndir_b);

	// Both should face the exact same direction (-90°)
	QCOMPARE(angle1, -90.0);
	QCOMPARE(angle2, -90.0);

	// When explicitly rotated to 270, both should be 270
	vDim1.setTextRotation(270);
	vDim1.calculateGeometry(p1_a, p2_a, p1_ext_a, p2_ext_a, mid_a, angle1, udir_a, ndir_a);
	QCOMPARE(angle1, 270.0);

	// Verify Dialog sets rotation and max arrow size
	DimensionPropertiesDialog dlg(&dim);
	auto *rotCb = dlg.findChild<QComboBox*>("m_text_rotation_cb");
	QVERIFY(rotCb != nullptr);
	QCOMPARE(rotCb->count(), 5); // Auto (-1), 0, 90, 180, 270

	auto *arrowSb = dlg.findChild<QDoubleSpinBox*>("m_arrow_size_sb");
	QVERIFY(arrowSb != nullptr);
	QCOMPARE(arrowSb->maximum(), 100.0);

	// Explicit Undo/Redo test for textRotation and arrowSize via Dialog
	QETProject prj;
	Diagram *diag = prj.addNewDiagram();
	QVERIFY(diag != nullptr);
	auto *testDim = new DimensionItem(QPointF(0, 0), QPointF(200, 0), 20.0);
	diag->undoStack().push(new AddGraphicsObjectCommand(testDim, diag));

	QCOMPARE(testDim->textRotation(), -1);
	QCOMPARE(testDim->arrowSize(), 8.0);

	{
		DimensionPropertiesDialog dialog(testDim);
		auto *rotBox = dialog.findChild<QComboBox*>("m_text_rotation_cb");
		auto *arrBox = dialog.findChild<QDoubleSpinBox*>("m_arrow_size_sb");
		QVERIFY(rotBox != nullptr);
		QVERIFY(arrBox != nullptr);

		// Select 90°
		int idx90 = rotBox->findData(90);
		QVERIFY(idx90 >= 0);
		rotBox->setCurrentIndex(idx90);
		arrBox->setValue(85.0);

		dialog.accept();
	}

	QCOMPARE(testDim->textRotation(), 90);
	QCOMPARE(testDim->arrowSize(), 85.0);

	// Perform Undo
	diag->undoStack().undo();
	QCOMPARE(testDim->textRotation(), -1);
	QCOMPARE(testDim->arrowSize(), 8.0);

	// Perform Redo
	diag->undoStack().redo();
	QCOMPARE(testDim->textRotation(), 90);
	QCOMPARE(testDim->arrowSize(), 85.0);

	// Test Apply + Reject (Preview cancel)
	{
		DimensionPropertiesDialog dialog(testDim);
		auto *rotBox = dialog.findChild<QComboBox*>("m_text_rotation_cb");
		auto *arrBox = dialog.findChild<QDoubleSpinBox*>("m_arrow_size_sb");

		int idx180 = rotBox->findData(180);
		QVERIFY(idx180 >= 0);
		rotBox->setCurrentIndex(idx180);
		arrBox->setValue(95.0);

		// Simulate clicking Apply
		dialog.apply();
		QCOMPARE(testDim->textRotation(), 180);
		QCOMPARE(testDim->arrowSize(), 95.0);

		// Cancel (Reject) -> must rollback to 90 and 85.0
		dialog.reject();
	}
	QCOMPARE(testDim->textRotation(), 90);
	QCOMPARE(testDim->arrowSize(), 85.0);
}

QTEST_MAIN(tst_DimensionItem)
#include "tst_dimensionitem.moc"
