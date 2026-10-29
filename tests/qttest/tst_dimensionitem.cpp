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

QTEST_MAIN(tst_DimensionItem)
#include "tst_dimensionitem.moc"
