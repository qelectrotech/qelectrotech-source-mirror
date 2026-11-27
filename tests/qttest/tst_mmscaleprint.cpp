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
#include "print/printdrawing.h"
#include "ui/borderpropertieswidget.h"
#include "qetapptest.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetproject.h"
#include "qetresult.h"
#include "qetunits.h"

#include <QComboBox>
#include <QPageLayout>
#include <QPageSize>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QtTest>

/**
	Step 3 of docs/mm-scale.md: a project drawn in millimetres prints and
	exports to PDF with 1 px = 0.25 mm. A project in pixels keeps 1 px at
	96 dpi.
*/
class tst_mmscaleprint : public QObject
{
	Q_OBJECT
	QTemporaryDir m_dir;
	QString m_pdf;	///< what exportedPage() wrote

	QString makeProject(const QString &name, QETProject::Scale scale, qreal module = 2.5)
	{
		QETProject project;
		project.addNewDiagram();
		project.setScale(scale);
		project.setModuleMm(module);
		const QString path = m_dir.filePath(name);
		project.setFilePath(path);
		const QETResult written = project.write();
		if (!written.isOk())
			return {};
		return path;
	}

		//The MediaBox of the first page, in points
	QSizeF exportedPage(const QString &project)
	{
		const QString home = m_dir.filePath(QStringLiteral("home"));
		const QString tmp = m_dir.filePath(QStringLiteral("tmp"));
		const QString out = m_dir.filePath(QStringLiteral("out.pdf"));
		QDir().mkpath(home);
		QDir().mkpath(tmp);
		QFile::remove(out);
		QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
		env.insert(QStringLiteral("QT_QPA_PLATFORM"), QStringLiteral("offscreen"));
		env.insert(QStringLiteral("HOME"), home);
		env.insert(QStringLiteral("XDG_CONFIG_HOME"), home + QStringLiteral("/.config"));
		env.insert(QStringLiteral("XDG_DATA_HOME"), home + QStringLiteral("/.local/share"));
		env.insert(QStringLiteral("TMPDIR"), tmp);
		QProcess proc;
		proc.setProcessEnvironment(env);
		proc.start(QStringLiteral(QET_TEST_BINARY_PATH),
			   {QStringLiteral("--export-pdf"), project, out});
		if (!proc.waitForFinished(60000) || proc.exitCode() != 0)
			return {};
		QFile file(out);
		if (!file.open(QIODevice::ReadOnly))
			return {};
		const QString pdf = QString::fromLatin1(file.readAll());
		m_pdf = pdf;
		static const QRegularExpression box(
			QStringLiteral(R"(/MediaBox \[0 0 ([0-9.]+) ([0-9.]+)\])"));
		const auto m = box.match(pdf);
		return m.hasMatch() ? QSizeF(m.captured(1).toDouble(), m.captured(2).toDouble())
				    : QSizeF();
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
		QETProject::setBackupEnabled(false);
		QVERIFY(QFile::exists(QStringLiteral(QET_TEST_BINARY_PATH)));
	}

		//1 scene px = 0.25 mm on paper, whatever the printer's dpi
	void pixelsPerUnit()
	{
		QCOMPARE(QetPrint::pixelsPerUnit(600, QetUnits::defaultPxPerMm), 600 / 101.6);
		QCOMPARE(QetPrint::pixelsPerUnit(1200, QetUnits::defaultPxPerMm), 1200 / 101.6);
			//One unit is 0.25 mm: 4 units make a millimetre of device pixels
		QVERIFY(qFuzzyCompare(4 * QetPrint::pixelsPerUnit(600, QetUnits::defaultPxPerMm),
				      600 / 25.4));
			//Not to scale: as before
		QCOMPARE(QetPrint::pixelsPerUnit(600), 600.0 / QetPrint::drawingDpi);
		QCOMPARE(QetPrint::pixelsPerUnit(96), 1.0);
	}

		//10 px are M on paper: a unit is M / 10 mm of the printer's pixels
	void pixelsPerUnitFollowsTheModule_data()
	{
		QTest::addColumn<qreal>("module");
		QTest::newRow("M 1.8") << 1.8;
		QTest::newRow("M 2.5") << 2.5;
		QTest::newRow("M 5.0") << 5.0;
	}

	void pixelsPerUnitFollowsTheModule()
	{
		QFETCH(qreal, module);
		const qreal px_per_mm = QetUnits::pxPerMmFor(module);
		QVERIFY(qFuzzyCompare(QetPrint::pixelsPerUnit(600, px_per_mm),
				      600 / (25.4 * 10 / module)));
		QVERIFY(qFuzzyCompare(QetPrint::unitsPerMm(px_per_mm), 10 / module));
			//An A4 landscape page holds 297 / M * 10 units
		const QPageLayout layout(QPageSize(QPageSize::A4),
					 QPageLayout::Landscape, QMarginsF());
		const QSizeF page = QetPrint::pageSizeInDrawingUnits(layout, true, px_per_mm);
		QVERIFY(qAbs(page.width() - 297 * 10 / module) < 0.01);
		QVERIFY(qAbs(page.height() - 210 * 10 / module) < 0.01);
	}

		//A4 landscape, no margins: 297 x 210 mm
	void pageSizeInUnits()
	{
		const QPageLayout layout(QPageSize(QPageSize::A4),
					 QPageLayout::Landscape, QMarginsF());
		const QSizeF to_scale = QetPrint::pageSizeInDrawingUnits(layout, true, QetUnits::defaultPxPerMm);
		QVERIFY(qAbs(to_scale.width() - 297 * 4) < 0.01);
		QVERIFY(qAbs(to_scale.height() - 210 * 4) < 0.01);
		const QSizeF before = QetPrint::pageSizeInDrawingUnits(layout, true);
		QCOMPARE(before, layout.fullRect(QPageLayout::Millimeter).size()
				 * (QetPrint::drawingDpi / 25.4));
			//The default folio, 1040 x 660 px = 260 x 165 mm, fits
		QVERIFY(1040 <= to_scale.width() && 660 <= to_scale.height());
	}

		//The same folio exported in both ways. Qt writes whole points, so
		//the page is within 1 pt of its size.
	void exportedPageSizes()
	{
		const QString px_path = makeProject(QStringLiteral("px.qet"), QETProject::Scale::Px);
		const QString mm_path = makeProject(QStringLiteral("mm.qet"), QETProject::Scale::Mm);
		QVERIFY(!px_path.isEmpty() && !mm_path.isEmpty());
		const QSizeF px_page = exportedPage(px_path);
		const QSizeF mm_page = exportedPage(mm_path);
		QVERIFY(!px_page.isEmpty() && !mm_page.isEmpty());

			//Pixels: 1 px = 0.75 pt as before
		const QSizeF folio(px_page.width() / 0.75, px_page.height() / 0.75);
		QVERIFY(folio.width() > 500 && folio.height() > 300);

			//Millimetres: 1 px = 0.25 mm
		const qreal mm_width = mm_page.width() * 25.4 / 72;
		const qreal mm_height = mm_page.height() * 25.4 / 72;
		QVERIFY2(qAbs(mm_width - QetUnits::pxToMm(folio.width(), QetUnits::defaultPxPerMm)) < 0.6, qPrintable(QString::number(mm_width)));
		QVERIFY2(qAbs(mm_height - QetUnits::pxToMm(folio.height(), QetUnits::defaultPxPerMm)) < 0.6, qPrintable(QString::number(mm_height)));
	}

		//The CLI page is the folio at 10 / M px per mm: 1040 px wide is
		//1040 * M / 10 mm
	void exportedPageFollowsTheModule_data()
	{
		QTest::addColumn<qreal>("module");
		QTest::newRow("M 1.8") << 1.8;
		QTest::newRow("M 2.5") << 2.5;
		QTest::newRow("M 5.0") << 5.0;
	}

	void exportedPageFollowsTheModule()
	{
		QFETCH(qreal, module);
		const QString path = makeProject(QStringLiteral("module.qet"), QETProject::Scale::Mm, module);
		const QString px_path = makeProject(QStringLiteral("modulepx.qet"), QETProject::Scale::Px, module);
		QVERIFY(!path.isEmpty() && !px_path.isEmpty());
		const QSizeF page = exportedPage(path);
		const QSizeF px_page = exportedPage(px_path);
		QVERIFY(!page.isEmpty() && !px_page.isEmpty());
		const QSizeF folio(px_page.width() / 0.75, px_page.height() / 0.75);
		const qreal mm_width = page.width() * 25.4 / 72;
		const qreal mm_height = page.height() * 25.4 / 72;
		QVERIFY2(qAbs(mm_width - folio.width() * module / 10) < 0.6, qPrintable(QString::number(mm_width)));
		QVERIFY2(qAbs(mm_height - folio.height() * module / 10) < 0.6, qPrintable(QString::number(mm_height)));
			//A project in pixels ignores the module
		QVERIFY(qAbs(folio.width() - 1040) < 2);
	}

		//A folio given the size of A3 at M = 5 by the folio properties
		//is an A3 page in the PDF: what it leaves of the sheet is under the
		//3 pt the page size snaps by
	void a3FolioIsAnA3PageAtLargeModule()
	{
		const QString path = m_dir.filePath(QStringLiteral("a3.qet"));
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			project.setScale(QETProject::Scale::Mm);
			project.setModuleMm(5.0);
			BorderPropertiesWidget widget(diagram->border_and_titleblock.exportBorder());
			widget.setPxPerMm(project.pxPerMm());
			widget.setTitleBlockSize(
				qRound(diagram->border_and_titleblock.titleBlockRect().height()),
				diagram->border_and_titleblock.titleBlockEdge(), Qt::AlignTop);
			auto *paper = widget.findChild<QComboBox *>("m_paper_cb");
			QVERIFY(paper);
			widget.findChild<QSpinBox *>("m_columns_width_sp")->setValue(20);
			widget.findChild<QSpinBox *>("m_rows_height_sp")->setValue(20);
			paper->setCurrentIndex(paper->findText("A3"));
			emit paper->activated(paper->currentIndex());
			diagram->border_and_titleblock.importBorder(widget.properties());
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		const QSizeF page = exportedPage(path);
		QVERIFY(!page.isEmpty());
		const QSize a3 = QPageSize(QPageSize::A3).sizePoints();
		QVERIFY2(qAbs(page.width() - a3.height()) < 3
					 && qAbs(page.height() - a3.width()) < 3,
				 qPrintable(QStringLiteral("%1 x %2").arg(page.width()).arg(page.height())));
	}

		//A folio fitted up to A3 by the folio properties is the A3 sheet
		//in the PDF, not a page a few points short of it
	void a3FolioIsExactlyA3_data()
	{
		QTest::addColumn<qreal>("module");
		QTest::newRow("M 2.5") << 2.5;
		QTest::newRow("M 5.0") << 5.0;
	}

	void a3FolioIsExactlyA3()
	{
		QFETCH(qreal, module);
		const QString path = m_dir.filePath(QStringLiteral("a3exact.qet"));
		qreal folio_pt = 0;
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			project.setScale(QETProject::Scale::Mm);
			project.setModuleMm(module);
			BorderPropertiesWidget widget(diagram->border_and_titleblock.exportBorder());
			widget.setPxPerMm(project.pxPerMm());
			widget.setTitleBlockSize(
				qRound(diagram->border_and_titleblock.titleBlockRect().height()),
				diagram->border_and_titleblock.titleBlockEdge(), Qt::AlignTop);
			auto *paper = widget.findChild<QComboBox *>("m_paper_cb");
			QVERIFY(paper);
			widget.findChild<QSpinBox *>("m_columns_width_sp")->setValue(20);
			widget.findChild<QSpinBox *>("m_rows_height_sp")->setValue(20);
			paper->setCurrentIndex(paper->findText("A3"));
			emit paper->activated(paper->currentIndex());
			diagram->border_and_titleblock.importBorder(widget.properties());
			const QRectF rect = QRectF(diagram->border_and_titleblock
				.borderAndTitleBlockRect()).adjusted(0, 0, 1, 1).toAlignedRect();
			folio_pt = rect.width() * 72.0 / (project.pxPerMm() * 25.4);
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		const QSizeF page = exportedPage(path);
		QVERIFY(!page.isEmpty());
		const QSize a3 = QPageSize(QPageSize::A3).sizePoints();
		QVERIFY2(folio_pt < a3.height(),
				 qPrintable(QStringLiteral("the folio is not short of A3: %1 pt").arg(folio_pt)));
		QVERIFY2(qAbs(page.width() - a3.height()) <= 0.5
					 && qAbs(page.height() - a3.width()) <= 0.5,
				 qPrintable(QStringLiteral("%1 x %2 pt, folio %3 pt wide")
					 .arg(page.width()).arg(page.height()).arg(folio_pt)));
	}

		//The /FitR of a link points at its target on the page of the
		//target: the page of a tall folio and the page of a short one are
		//not as high, so the Y flip is that of the target's page
	void fitRectOnAnotherPageSize()
	{
		const QString path = m_dir.filePath(QStringLiteral("links.qet"));
		QRectF framed;
		QRectF source;
		qreal pt_per_px = 0;
		{
			QETProject project;
			Diagram *first = project.addNewDiagram();
			Diagram *second = project.addNewDiagram();
			project.setScale(QETProject::Scale::Mm);
			first->border_and_titleblock.setRowsCount(12);
			second->border_and_titleblock.setRowsCount(4);
			second->border_and_titleblock.setColumnsCount(10);
			auto *text = new IndependentTextItem(QStringLiteral("see 2"));
			first->addItem(text);
			text->setPos(300, 200);
			text->setDestination(second->uuid());
			source = QRectF(second->border_and_titleblock
				.borderAndTitleBlockRect()).adjusted(0, 0, 1, 1).toAlignedRect();
			framed = second->border_and_titleblock.insideBorderRect();
			framed.adjust(-25, -25, 25, 25);
			pt_per_px = 72.0 / (project.pxPerMm() * 25.4);
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		QVERIFY(!exportedPage(path).isEmpty());
		static const QRegularExpression boxes(
			QStringLiteral(R"(/MediaBox \[0 0 ([0-9.]+) ([0-9.]+)\])"));
		QList<QSizeF> pages;
		for (auto it = boxes.globalMatch(m_pdf); it.hasNext();) {
			const auto b = it.next();
			pages << QSizeF(b.captured(1).toDouble(), b.captured(2).toDouble());
		}
		QCOMPARE(pages.size(), 2);
		QVERIFY(pages.at(0).height() > pages.at(1).height() + 20);

		static const QRegularExpression fitr(QStringLiteral(
			R"(/FitR\s+([-0-9.]+)\s+([-0-9.]+)\s+([-0-9.]+)\s+([-0-9.]+))"));
		const auto m = fitr.match(m_pdf);
		QVERIFY2(m.hasMatch(), "no /FitR destination");
		const QRectF got(QPointF(m.captured(1).toDouble(), m.captured(2).toDouble()),
				 QPointF(m.captured(3).toDouble(), m.captured(4).toDouble()));
			//The frame has 25 px of context around the border
		const qreal slack = 25 * pt_per_px + 1;
		QVERIFY2(QRectF(QPointF(), pages.at(1)).adjusted(-slack, -slack, slack, slack).contains(got),
			 qPrintable(QStringLiteral("%1,%2 %3,%4 is not on page 2, %5 x %6")
				 .arg(got.left()).arg(got.top()).arg(got.right()).arg(got.bottom())
				 .arg(pages.at(1).width()).arg(pages.at(1).height())));
		const qreal height = pages.at(1).height();
		const QRectF expected(
			QPointF((framed.left() - source.left()) * pt_per_px,
				height - (framed.bottom() - source.top()) * pt_per_px),
			QPointF((framed.right() - source.left()) * pt_per_px,
				height - (framed.top() - source.top()) * pt_per_px));
		QVERIFY2(qAbs(got.left() - expected.left()) < 1.5
				 && qAbs(got.right() - expected.right()) < 1.5
				 && qAbs(got.top() - expected.top()) < 1.5
				 && qAbs(got.bottom() - expected.bottom()) < 1.5,
			 qPrintable(QStringLiteral("got %1,%2 %3,%4 expected %5,%6 %7,%8")
				 .arg(got.left()).arg(got.top()).arg(got.right()).arg(got.bottom())
				 .arg(expected.left()).arg(expected.top())
				 .arg(expected.right()).arg(expected.bottom())));
	}

		//A folio 2 pt wider than A3 is not cut down to the A3 sheet: the
		//page is as wide as the folio
	void pageIsNotSmallerThanTheFolio()
	{
		const QString path = m_dir.filePath(QStringLiteral("wide.qet"));
		qreal folio_pt = 0;
		{
			QETProject project;
			Diagram *diagram = project.addNewDiagram();
			project.setScale(QETProject::Scale::Mm);
				//A3 high, and 3 px (2.1 pt) wider than A3
			BorderPropertiesWidget widget(diagram->border_and_titleblock.exportBorder());
			widget.setPxPerMm(project.pxPerMm());
			widget.setTitleBlockSize(
				qRound(diagram->border_and_titleblock.titleBlockRect().height()),
				diagram->border_and_titleblock.titleBlockEdge(), Qt::AlignTop);
			auto *paper = widget.findChild<QComboBox *>("m_paper_cb");
			QVERIFY(paper);
			paper->setCurrentIndex(paper->findText("A3"));
			emit paper->activated(paper->currentIndex());
			BorderProperties props = widget.properties();
			props.columns_count = 14;
			props.columns_width = (420 * project.pxPerMm() + 3.0 - 21) / 14;
			diagram->border_and_titleblock.importBorder(props);
			const QRectF rect = QRectF(diagram->border_and_titleblock
				.borderAndTitleBlockRect()).adjusted(0, 0, 1, 1).toAlignedRect();
			folio_pt = rect.width() * 72.0 / (project.pxPerMm() * 25.4);
			project.setFilePath(path);
			QVERIFY(project.write().isOk());
		}
		const QSizeF page = exportedPage(path);
		QVERIFY(!page.isEmpty());
		const qreal a3_long = QPageSize(QPageSize::A3).sizePoints().height();
		QVERIFY2(folio_pt > a3_long + 1.5 && folio_pt < a3_long + 3,
				 qPrintable(QString::number(folio_pt)));
		QVERIFY2(page.width() >= folio_pt - 0.5,
				 qPrintable(QStringLiteral("page %1 pt, folio %2 pt").arg(page.width()).arg(folio_pt)));
	}
};

QET_APP_TEST_MAIN(tst_mmscaleprint)
#include "tst_mmscaleprint.moc"
