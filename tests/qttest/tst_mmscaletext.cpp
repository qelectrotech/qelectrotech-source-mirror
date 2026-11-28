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
#include "qetapptest.h"
#include "qetproject.h"
#include "qetunits.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "ui/conductorpropertieswidget.h"
#include "ui/dynamicelementtextmodel.h"
#include "ui/inditextpropertieswidget.h"
#include "conductorproperties.h"

#include <QDoubleSpinBox>
#include <QFontDatabase>
#include <QStyleOptionViewItem>
#include <QUndoCommand>
#include <QtTest>
#include <memory>

/**
	Step 6 of docs/mm-scale.md: in a project drawn to scale the text-size
	fields show the capital height h in mm and store points. Projects in
	pixels, and widgets without a project, show points as before.
*/
class tst_mmscaletext : public QObject
{
	Q_OBJECT

	static QFont liberation()
	{
		QFont f(QStringLiteral("Liberation Sans"));
		return QFontInfo(f).family() == QLatin1String("Liberation Sans") ? f : QFont();
	}

	static Element *firstSymbol(QETProject &project)
	{
		for (Diagram *d : project.diagrams())
			for (Element *e : d->elements())
				if (e->linkType() == Element::Simple
					&& e->elementData().m_type != ElementData::Terminal)
					return e;
		return nullptr;
	}

private slots:
	void helperRoundTrips()
	{
		QList<QFont> fonts = { QGuiApplication::font() };
		const QFont lib = liberation();
		if (QFontInfo(lib).family() == QLatin1String("Liberation Sans"))
			fonts << lib;
		for (const QFont &f : fonts) {
			for (qreal pt : {4.0, 6.5, 9.0, 10.3, 14.0, 50.0}) {
				const qreal mm = QetUnits::pointsToTextMm(pt, f, QetUnits::defaultPxPerMm);
				QVERIFY(mm > 0);
				QVERIFY(qAbs(QetUnits::textMmToPoints(mm, f, QetUnits::defaultPxPerMm) - pt) < 0.05);
			}
		}
	}

		//The cache gives what a fresh measure gives, for each style of a
		//family, the first time and the next
	void capHeightRatioIsCachedPerStyle()
	{
		const auto measure = [](const QFont &font) {
			QFont f(font);
			f.setPixelSize(1000);
			const qreal ratio = QFontMetricsF(f).capHeight() / 1000;
			return ratio > 0 ? ratio : 0.7;
		};
		QFont plain = liberation();
		QFont bold = plain;
		bold.setBold(true);
		QFont italic = plain;
		italic.setItalic(true);
		QFont other(QStringLiteral("DejaVu Sans"));
		for (int pass = 0; pass < 2; ++pass)
			for (const QFont &font : {plain, bold, italic, other}) {
				QCOMPARE(QetUnits::capHeightRatio(font), measure(font));
					//The point size does not enter it
				QFont big(font);
				big.setPointSize(40);
				QCOMPARE(QetUnits::capHeightRatio(big), measure(font));
			}
	}

	void isoHeightsInLiberation()
	{
		const QFont lib = liberation();
		if (QFontInfo(lib).family() != QLatin1String("Liberation Sans"))
			QSKIP("Liberation Sans is not installed");
		QVERIFY2(qAbs(QetUnits::capHeightRatio(lib) - 1409.0 / 2048.0) < 0.01,
				 qPrintable(QString::number(QetUnits::capHeightRatio(lib))));
			//The design note says 0.73 and 10.3 pt; Liberation Sans's cap
			//height is 1409 / 2048 = 0.688 em, which gives 10.9 pt
		QVERIFY2(qAbs(QetUnits::textMmToPoints(2.5, lib, QetUnits::defaultPxPerMm) - 10.9) < 0.1,
				 qPrintable(QString::number(QetUnits::textMmToPoints(2.5, lib, QetUnits::defaultPxPerMm))));
		QVERIFY(qAbs(QetUnits::pointsToTextMm(9.0, lib, QetUnits::defaultPxPerMm) - 2.07) < 0.05);
	}

		//The capital height of the same points scales by 4 / (10 / M)
	void textHeightFollowsTheModule_data()
	{
		QTest::addColumn<qreal>("module");
		QTest::newRow("M 1.8") << 1.8;
		QTest::newRow("M 2.5") << 2.5;
		QTest::newRow("M 5.0") << 5.0;
	}

	void textHeightFollowsTheModule()
	{
		QFETCH(qreal, module);
		const QFont font = QETApp::diagramTextsFont();
		const qreal at_default = QetUnits::pointsToTextMm(9, font, QetUnits::defaultPxPerMm);
		const qreal px_per_mm = QetUnits::pxPerMmFor(module);
		QVERIFY(qAbs(QetUnits::pointsToTextMm(9, font, px_per_mm)
			     - at_default * QetUnits::defaultPxPerMm / px_per_mm) < 1e-9);
		const qreal pt = QetUnits::textMmToPoints(3.5, font, px_per_mm);
		QVERIFY(qAbs(QetUnits::pointsToTextMm(pt, font, px_per_mm) - 3.5) < 1e-9);

			//and the field of the widget shows it
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		project.setScale(QETProject::Scale::Mm);
		project.setModuleMm(module);
		auto *text = new IndependentTextItem(QStringLiteral("x"));
		diagram->addItem(text);
		QFont f = text->font();
		f.setPointSizeF(9);
		text->setFont(f);
		IndiTextPropertiesWidget widget(text);
		auto *sb = widget.findChild<QDoubleSpinBox *>(QStringLiteral("m_size_sb"));
		QVERIFY(sb);
		QVERIFY(qAbs(sb->value() - QetUnits::pointsToTextMm(9, text->font(), px_per_mm)) < 0.006);
		sb->setValue(3.5);
		QVERIFY(qAbs(QetUnits::pointsToTextMm(text->font().pointSizeF(), text->font(), px_per_mm) - 3.5) < 0.05);
	}

	void independentTextInMmProject()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		project.setScale(QETProject::Scale::Mm);
		auto *text = new IndependentTextItem(QStringLiteral("x"));
		diagram->addItem(text);
		QFont font = text->font();
		font.setPointSizeF(9);
		text->setFont(font);

		IndiTextPropertiesWidget widget(text);
		auto *sb = widget.findChild<QDoubleSpinBox *>(QStringLiteral("m_size_sb"));
		QVERIFY(sb);
		QCOMPARE(sb->suffix(), tr(" mm"));
		QCOMPARE(sb->decimals(), 2);
		QVERIFY(qAbs(sb->value() - QetUnits::pointsToTextMm(9, text->font(), QetUnits::defaultPxPerMm)) < 0.006);

			//Nothing typed: the font is left alone
		widget.apply();
		QCOMPARE(text->font().pointSizeF(), 9.0);

		sb->setValue(3.5);
		QVERIFY(qAbs(text->font().pointSizeF()
					 - QetUnits::textMmToPoints(3.5, text->font(), QetUnits::defaultPxPerMm)) < 0.06);
		QVERIFY(qAbs(QetUnits::pointsToTextMm(text->font().pointSizeF(), text->font(), QetUnits::defaultPxPerMm) - 3.5) < 0.02);
		diagram->undoStack().undo();
		QCOMPARE(text->font().pointSizeF(), 9.0);
	}

	void independentTextInPxProject()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		auto *text = new IndependentTextItem(QStringLiteral("x"));
		diagram->addItem(text);
		QFont font = text->font();
		font.setPointSize(9);
		text->setFont(font);

		IndiTextPropertiesWidget widget(text);
		auto *sb = widget.findChild<QDoubleSpinBox *>(QStringLiteral("m_size_sb"));
		QVERIFY(sb);
		QVERIFY(sb->suffix().isEmpty());
		QCOMPARE(sb->decimals(), 0);
		QCOMPARE(sb->value(), 9.0);
		sb->setValue(12);
		QCOMPARE(text->font().pointSize(), 12);
	}

		//The scale changes while an editor is open: the box keeps the unit it
		//was filled with, so another edit does not read 9 pt as 9 mm
	void independentTextScaleSwitchedWhileOpen()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		auto *text = new IndependentTextItem(QStringLiteral("x"));
		diagram->addItem(text);
		QFont font = text->font();
		font.setPointSize(9);
		text->setFont(font);

		IndiTextPropertiesWidget widget(text);
		auto *sb = widget.findChild<QDoubleSpinBox *>(QStringLiteral("m_size_sb"));
		auto *angle = widget.findChild<QDoubleSpinBox *>(QStringLiteral("m_angle_sb"));
		QVERIFY(sb && angle);
		QCOMPARE(sb->value(), 9.0);

		project.setScale(QETProject::Scale::Mm);
		angle->setValue(30);
		QCOMPARE(text->rotation(), 30.0);
		QCOMPARE(text->font().pointSizeF(), 9.0);
			//The widget shows the new unit once it is filled again
		QVERIFY(sb->suffix().contains(QLatin1String("mm")));

		project.setScale(QETProject::Scale::Px);
		angle->setValue(60);
		QCOMPARE(text->rotation(), 60.0);
		QCOMPARE(text->font().pointSizeF(), 9.0);
	}

	void elementTextModelScaleSwitchedWhileOpen()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QCOMPARE(project.state(), QETProject::Ok);
		Element *e = firstSymbol(project);
		QVERIFY(e);
		auto *deti = new DynamicElementTextItem(e);
		e->addDynamicTextItem(deti);
		QFont font = deti->font();
		font.setPointSize(9);
		deti->setFont(font);

		DynamicElementTextModel model(e);
		QCOMPARE(model.sizePxPerMm(), 0.0);
		project.setScale(QETProject::Scale::Mm);
			//The cells still hold points: 9 is not read as 9 mm
		QCOMPARE(model.sizePxPerMm(), 0.0);
		std::unique_ptr<QUndoCommand> undo(model.undoForEditedText(deti));
		QCOMPARE(undo->childCount(), 0);
		QCOMPARE(deti->font().pointSizeF(), 9.0);
	}

	void conductorScaleSwitchedWhileOpen()
	{
		ConductorProperties props;
		props.text_size = 10;
		props.cond_size = 2;
		QETProject project;
		project.addNewDiagram();
		ConductorPropertiesWidget widget;
		widget.setProperties(props);
		widget.setProject(&project);
		project.setScale(QETProject::Scale::Mm);
		const ConductorProperties out = widget.properties();
		QCOMPARE(out.text_size, 10);
		QCOMPARE(out.cond_size, 2.0);
	}

	void conductorTextSize()
	{
		ConductorProperties props;
		props.text_size = 10;
		ConductorPropertiesWidget px;
		px.setProperties(props);
		auto *sb = px.findChild<QDoubleSpinBox *>(QStringLiteral("m_text_size_sb"));
		QVERIFY(sb);
		QCOMPARE(sb->value(), 10.0);
		QCOMPARE(px.properties().text_size, 10);

		QETProject project;
		project.addNewDiagram();
		project.setScale(QETProject::Scale::Mm);
		ConductorPropertiesWidget mm;
		mm.setProperties(props);
		mm.setProject(&project);
		sb = mm.findChild<QDoubleSpinBox *>(QStringLiteral("m_text_size_sb"));
		QVERIFY(sb->suffix().contains(QLatin1String("mm")));
		QVERIFY(sb->value() < 4 && sb->value() > 1);
			//Untouched: the stored size is kept exactly
		QCOMPARE(mm.properties().text_size, 10);
		sb->setValue(3.5);
		const int pt = mm.properties().text_size;
		QVERIFY(qAbs(QetUnits::pointsToTextMm(pt, QETApp::diagramTextsFont(), QetUnits::defaultPxPerMm) - 3.5) < 0.2);
	}

	void elementTextTable()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QCOMPARE(project.state(), QETProject::Ok);
		Element *e = firstSymbol(project);
		QVERIFY(e);
		auto *deti = new DynamicElementTextItem(e);
		e->addDynamicTextItem(deti);
		QFont font = deti->font();
		font.setPointSizeF(9);
		deti->setFont(font);

		auto sizeCell = [](DynamicElementTextModel &model, DynamicElementTextItem *t) {
			QStandardItem *row = model.itemFromIndex(model.indexFromText(t));
			for (int r = 0; r < row->rowCount(); ++r)
				if (QStandardItem *c = row->child(r, 1))
					if (c->data(Qt::UserRole + 1).toInt() == DynamicElementTextModel::size)
						return c;
			return static_cast<QStandardItem *>(nullptr);
		};

		{
			DynamicElementTextModel model(e);
			QStandardItem *cell = sizeCell(model, deti);
			QVERIFY(cell);
			QCOMPARE(cell->data(Qt::EditRole).toInt(), 9);
			QWidget parent;
			DynamicTextItemDelegate delegate;
			std::unique_ptr<QWidget> editor(delegate.createEditor(&parent, QStyleOptionViewItem(), cell->index()));
			QVERIFY(qobject_cast<QSpinBox *>(editor.get()));
		}

		project.setScale(QETProject::Scale::Mm);
		DynamicElementTextModel model(e);
		QStandardItem *cell = sizeCell(model, deti);
		QVERIFY(cell);
		QVERIFY(qAbs(cell->data(Qt::EditRole).toDouble() - QetUnits::pointsToTextMm(9, deti->font(), QetUnits::defaultPxPerMm)) < 0.006);

		QWidget parent;
		DynamicTextItemDelegate delegate;
		std::unique_ptr<QWidget> editor(delegate.createEditor(&parent, QStyleOptionViewItem(), cell->index()));
		auto *dsb = qobject_cast<QDoubleSpinBox *>(editor.get());
		QVERIFY(dsb);
		QVERIFY(dsb->suffix().contains(QLatin1String("mm")));

			//Untouched, no command
		std::unique_ptr<QUndoCommand> none(model.undoForEditedText(deti));
		QCOMPARE(none->childCount(), 0);

		dsb->setValue(5.0);
		delegate.setModelData(dsb, &model, cell->index());
		std::unique_ptr<QUndoCommand> undo(model.undoForEditedText(deti));
		QCOMPARE(undo->childCount(), 1);
		undo->redo();
		QVERIFY(qAbs(QetUnits::pointsToTextMm(deti->font().pointSizeF(), deti->font(), QetUnits::defaultPxPerMm) - 5.0) < 0.02);
		undo->undo();
		QCOMPARE(deti->font().pointSizeF(), 9.0);
	}

		//The size editor in mm stops at the smallest size a font can have:
		//0.00 mm would be a font of 0 pt, which Qt refuses
	void elementTextSizeCannotReachZero()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QCOMPARE(project.state(), QETProject::Ok);
		Element *e = firstSymbol(project);
		QVERIFY(e);
		auto *deti = new DynamicElementTextItem(e);
		e->addDynamicTextItem(deti);
		QFont font = deti->font();
		font.setPointSizeF(9);
		deti->setFont(font);
		project.setScale(QETProject::Scale::Mm);

		DynamicElementTextModel model(e);
		QStandardItem *row = model.itemFromIndex(model.indexFromText(deti));
		QStandardItem *cell = nullptr;
		for (int r = 0; r < row->rowCount(); ++r)
			if (QStandardItem *c = row->child(r, 1))
				if (c->data(Qt::UserRole + 1).toInt() == DynamicElementTextModel::size)
					cell = c;
		QVERIFY(cell);
		QWidget parent;
		DynamicTextItemDelegate delegate;
		std::unique_ptr<QWidget> editor(delegate.createEditor(&parent, QStyleOptionViewItem(), cell->index()));
		auto *dsb = qobject_cast<QDoubleSpinBox *>(editor.get());
		QVERIFY(dsb);
		QVERIFY2(dsb->minimum() > 0, "the editor allows 0 mm");

		dsb->setValue(0);
		delegate.setModelData(dsb, &model, cell->index());
		std::unique_ptr<QUndoCommand> undo(model.undoForEditedText(deti));
		QCOMPARE(undo->childCount(), 1);
		undo->redo();
		QVERIFY(deti->font().pointSizeF() > 0);
		QVERIFY(deti->font().pointSizeF() < 9);
	}

		//A text already at the smallest size, set to the smallest again,
		//is not an edit: no undo entry that changes nothing
	void elementTextSizeAtTheMinimumPushesNoUndo()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QCOMPARE(project.state(), QETProject::Ok);
		Element *e = firstSymbol(project);
		QVERIFY(e);
		auto *deti = new DynamicElementTextItem(e);
		e->addDynamicTextItem(deti);
		QFont font = deti->font();
		font.setPointSizeF(1);
		deti->setFont(font);
		project.setScale(QETProject::Scale::Mm);

		DynamicElementTextModel model(e);
		QStandardItem *row = model.itemFromIndex(model.indexFromText(deti));
		QStandardItem *cell = nullptr;
		for (int r = 0; r < row->rowCount(); ++r)
			if (QStandardItem *c = row->child(r, 1))
				if (c->data(Qt::UserRole + 1).toInt() == DynamicElementTextModel::size)
					cell = c;
		QVERIFY(cell);
		QWidget parent;
		DynamicTextItemDelegate delegate;
		std::unique_ptr<QWidget> editor(delegate.createEditor(&parent, QStyleOptionViewItem(), cell->index()));
		auto *dsb = qobject_cast<QDoubleSpinBox *>(editor.get());
		QVERIFY(dsb);
		dsb->setValue(0);
		delegate.setModelData(dsb, &model, cell->index());
		std::unique_ptr<QUndoCommand> undo(model.undoForEditedText(deti));
		QCOMPARE(undo->childCount(), 0);
		undo->redo();
		QCOMPARE(deti->font().pointSizeF(), 1.0);
	}

		//A text of 60 pt opened in the size editor and committed as it is
		//stays 60 pt: the editor takes the range of the pixels (0 to 99
		//pt), not a narrower one that cuts the value
	void bigElementTextIsKeptByAnUnchangedEditor()
	{
		QETProject project(QFINDTESTDATA("fixtures/every_feature.qet"));
		QCOMPARE(project.state(), QETProject::Ok);
		Element *e = firstSymbol(project);
		QVERIFY(e);
		auto *deti = new DynamicElementTextItem(e);
		e->addDynamicTextItem(deti);
		QFont font = deti->font();
		font.setPointSizeF(60);
		deti->setFont(font);
		project.setScale(QETProject::Scale::Mm);

		DynamicElementTextModel model(e);
		QStandardItem *row = model.itemFromIndex(model.indexFromText(deti));
		QStandardItem *cell = nullptr;
		for (int r = 0; r < row->rowCount(); ++r)
			if (QStandardItem *c = row->child(r, 1))
				if (c->data(Qt::UserRole + 1).toInt() == DynamicElementTextModel::size)
					cell = c;
		QVERIFY(cell);
		QWidget parent;
		DynamicTextItemDelegate delegate;
		std::unique_ptr<QWidget> editor(delegate.createEditor(&parent, QStyleOptionViewItem(), cell->index()));
		auto *dsb = qobject_cast<QDoubleSpinBox *>(editor.get());
		QVERIFY(dsb);
			//What the editor opens with, as the view does
		delegate.setEditorData(dsb, cell->index());
		delegate.setModelData(dsb, &model, cell->index());
		std::unique_ptr<QUndoCommand> undo(model.undoForEditedText(deti));
		QCOMPARE(undo->childCount(), 0);
		undo->redo();
		QCOMPARE(deti->font().pointSizeF(), 60.0);
			//As in pixels, 99 pt is the largest
		QVERIFY(qAbs(dsb->maximum() - QetUnits::pointsToTextMm(99, deti->font(), QetUnits::defaultPxPerMm)) < 0.02);
	}
};

QET_APP_TEST_MAIN(tst_mmscaletext)
#include "tst_mmscaletext.moc"
