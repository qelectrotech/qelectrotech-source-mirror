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
#include "diagram.h"
#include "qetapptest.h"
#include "qetgraphicsitem/dimensionitem.h"
#include "ui/dimensionpropertieswidget.h"
#include "qetproject.h"
#include "qetunits.h"
#include "TerminalStrip/GraphicsItem/properties/terminalstriplayoutpattern.h"

#include <QSignalSpy>
#include <QLabel>
#include <QtTest>

	/// A dimension that notes the pixels per millimetre it has when it is
	/// asked for its bounds
class BoundsSpy : public DimensionItem
{
	public:
		using DimensionItem::DimensionItem;
		QRectF boundingRect() const override
		{
			asked << pxPerMm();
			return DimensionItem::boundingRect();
		}
		mutable QList<qreal> asked;
};

/**
	Step 7 of docs/mm-scale.md: a dimension reads 2 px/mm in a project in
	pixels, as it always has, and 4 px/mm (QetUnits::pxToMm) in a project
	drawn to scale, also when the item is moved into one.
*/
class tst_mmscaledimension : public QObject
{
	Q_OBJECT

	private slots:
		void pixelsProjectKeepsTwoPxPerMm()
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			auto *dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0));
			diagram->addItem(dim);
			QCOMPARE(dim->dimensionValueMM(), 50.0);
			QCOMPARE(dim->displayText(), QStringLiteral("50,0"));
		}

		void mmProjectUsesFourPxPerMm()
		{
			QETProject project;
			project.setScale(QETProject::Scale::Mm);
			Diagram *diagram = project.addNewDiagram();
			auto *dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0));
			diagram->addItem(dim);
			QCOMPARE(dim->dimensionValueMM(), 25.0);
			QCOMPARE(dim->displayText(), QStringLiteral("25,0"));
			QCOMPARE(dim->dimensionValueMM(), QetUnits::pxToMm(100, QetUnits::defaultPxPerMm));
		}

			//Out of any project it keeps the old value
		void loneItemKeepsTwoPxPerMm()
		{
			DimensionItem dim(QPointF(0, 0), QPointF(100, 0));
			QCOMPARE(dim.dimensionValueMM(), 50.0);
		}

		void movedIntoAnotherProjectFollowsIt()
		{
			QETProject px_project, mm_project;
			mm_project.setScale(QETProject::Scale::Mm);
			Diagram *px_diagram = px_project.addNewDiagram();
			Diagram *mm_diagram = mm_project.addNewDiagram();
			auto *dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0));
			px_diagram->addItem(dim);
			QCOMPARE(dim->dimensionValueMM(), 50.0);
			mm_diagram->addItem(dim);
			QCOMPARE(dim->dimensionValueMM(), 25.0);
			QCOMPARE(dim->displayText(), QStringLiteral("25,0"));
			px_diagram->addItem(dim);
			QCOMPARE(dim->dimensionValueMM(), 50.0);
		}

		void dimensionFollowsTheModule_data()
		{
			QTest::addColumn<qreal>("module");
			QTest::newRow("M 1.8") << 1.8;
			QTest::newRow("M 2.5") << 2.5;
			QTest::newRow("M 5.0") << 5.0;
		}

			//100 px are 100 * M / 10 mm, and a new M changes the text of
			//the item already there
		void dimensionFollowsTheModule()
		{
			QFETCH(qreal, module);
			QETProject project;
			project.setScale(QETProject::Scale::Mm);
			Diagram *diagram = project.addNewDiagram();
			auto *dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0));
			diagram->addItem(dim);
			QCOMPARE(dim->dimensionValueMM(), 25.0);
			project.setModuleMm(module);
			QVERIFY(qAbs(dim->dimensionValueMM() - 100 * module / 10) < 1e-9);
			QCOMPARE(dim->displayText(),
				 QString::number(dim->dimensionValueMM(), 'f', 1).replace('.', ','));
			QVERIFY(qAbs(dim->pxPerMm() - 10 / module) < 1e-9);
				//In pixels the module means nothing
			project.setScale(QETProject::Scale::Px);
			QCOMPARE(dim->dimensionValueMM(), 50.0);
		}

			//A scale or module switched while the item is shown: the item
			//is told, so it measures again and is painted again
		void liveSwitchRefreshesTheItem()
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			auto *dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0));
			diagram->addItem(dim);
			QCOMPARE(dim->displayText(), QStringLiteral("50,0"));
			QCoreApplication::processEvents();

			QSignalSpy spy(diagram, &QGraphicsScene::changed);
			project.setScale(QETProject::Scale::Mm);
			QCOMPARE(dim->displayText(), QStringLiteral("25,0"));
			QCoreApplication::processEvents();
			QVERIFY2(spy.count() > 0, "scale: the item was not painted again");

			spy.clear();
			project.setModuleMm(5.0);
			QCOMPARE(dim->displayText(), QStringLiteral("50,0"));
			QCoreApplication::processEvents();
			QVERIFY2(spy.count() > 0, "module: the item was not painted again");
		}

			//Qt keeps what an item covered when it is told the bounds are
			//about to change (prepareGeometryChange()): it asks for them
			//then, and they have to be the old ones, those of the scale
			//the item was painted in
		void switchAnnouncesTheOldBounds()
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			auto *dim = new BoundsSpy(QPointF(0, 0), QPointF(100, 0));
			diagram->addItem(dim);
			dim->setTextInLine(false);
			dim->setOffset(20);
			dim->setTextDistance(60);
			QCOMPARE(dim->pxPerMm(), 2.0);
			QVERIFY(diagram->items(QPointF(50, 140)).contains(dim));
			QCoreApplication::processEvents();

			dim->asked.clear();
			project.setScale(QETProject::Scale::Mm);
			QCOMPARE(dim->pxPerMm(), 4.0);
			QVERIFY(!dim->asked.isEmpty());
			QCOMPARE(dim->asked.first(), 2.0);
			QCoreApplication::processEvents();

			dim->asked.clear();
			project.setModuleMm(5.0);
			QCOMPARE(dim->pxPerMm(), 2.0);
			QVERIFY(!dim->asked.isEmpty());
			QCOMPARE(dim->asked.first(), 4.0);
				//What the scene finds is the item at its new text
			QVERIFY(diagram->items(QPointF(50, 140)).contains(dim));
		}

		void textAwayFromTheLineIsInTheBounds_data()
		{
			QTest::addColumn<bool>("mm");
			QTest::newRow("pixels") << false;
			QTest::newRow("millimetres") << true;
		}

			//The text sits off the line by the text distance in mm, far
			//beyond the size of the text: the item still covers it
		void textAwayFromTheLineIsInTheBounds()
		{
			QFETCH(bool, mm);
			QETProject project;
			if (mm)
				project.setScale(QETProject::Scale::Mm);
			Diagram *diagram = project.addNewDiagram();
			auto *dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0));
			diagram->addItem(dim);
			dim->setTextInLine(false);
			dim->setOffset(20);
			dim->setTextDistance(60);
			const qreal distance = 60 * dim->pxPerMm();
			QCOMPARE(distance, mm ? 240.0 : 120.0);
				//The line runs along x, so its normal
				//points down
			const QPointF centre(50, 20 + distance);
			const QRectF text(centre - QPointF(10, 5), QSizeF(20, 10));
			QVERIFY2(dim->boundingRect().contains(text), qPrintable(
				QStringLiteral("%1 does not hold the text").arg(dim->boundingRect().height())));
			QVERIFY(dim->shape().contains(centre));
			dim->setTextDistance(0);
			QVERIFY(!dim->shape().contains(centre));
		}

			//An open properties dock shows the length in the new scale
		void propertiesWidgetFollowsTheScale()
		{
			QETProject project;
			project.setScale(QETProject::Scale::Mm);
			Diagram *diagram = project.addNewDiagram();
			auto *dim = new DimensionItem(QPointF(0, 0), QPointF(100, 0));
			diagram->addItem(dim);
			DimensionPropertiesWidget widget(dim);
			auto *label = widget.findChild<QLabel *>(QStringLiteral("m_length_label"));
			QVERIFY(label);
			QCOMPARE(label->text(), QStringLiteral("25.0 mm"));
			project.setModuleMm(5.0);
			QCOMPARE(label->text(), QStringLiteral("50.0 mm"));
		}

		void terminalStripDefaultIsTheModuleScale()
		{
			TerminalStripLayoutPattern pattern;
			QCOMPARE(pattern.m_realistic_px_per_mm, QetUnits::defaultPxPerMm);
		}
};

QET_APP_TEST_MAIN(tst_mmscaledimension)
#include "tst_mmscaledimension.moc"
