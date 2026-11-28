// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"
#include "print/projectprintwindow.h"
#include "ui/borderpropertieswidget.h"
#include "qetproject.h"
#include "diagram.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetunits.h"

#include <QComboBox>
#include <QFile>
#include <QLabel>
#include <QPrinter>
#include <QRegularExpression>
#include <QTemporaryDir>

// The print window's PDF export turns each page to its folio (#1480). It
// used one orientation for the whole document, landscape unless changed,
// so a portrait folio was shrunk onto a landscape page.
// fixtures/pdf_page_a3.qet has an A3 landscape folio, then an A3 portrait
// one.
class tst_printpdforientation : public QObject
{
	Q_OBJECT

	QTemporaryDir m_dir;

	struct Page
	{
		QSizeF size;
		QList<QRectF> links;
	};

	// Each page's MediaBox and the Rect of each of its links, in page
	// order. Qt writes every object uncompressed, a page's annotations as
	// "/Annots n 0 R" pointing at an array of annotation references.
	static QList<Page> pages(const QString &pdf_path)
	{
		QFile file(pdf_path);
		if (!file.open(QIODevice::ReadOnly))
			return {};
		const QString pdf = QString::fromLatin1(file.readAll());

		QHash<int, QString> objects;
		static const QRegularExpression object(
			QStringLiteral(R"((\d+) 0 obj(.*?)endobj)"),
			QRegularExpression::DotMatchesEverythingOption);
		for (auto it = object.globalMatch(pdf); it.hasNext();) {
			const auto m = it.next();
			objects.insert(m.captured(1).toInt(), m.captured(2));
		}

		static const QRegularExpression ref(QStringLiteral(R"((\d+) 0 R)"));
		static const QRegularExpression kids(
			QStringLiteral(R"(/Type\s*/Pages.*?/Kids\s*\[(.*?)\])"),
			QRegularExpression::DotMatchesEverythingOption);
		static const QRegularExpression box(
			QStringLiteral(R"(/MediaBox \[0 0 ([0-9.]+) ([0-9.]+)\])"));
		static const QRegularExpression annots(QStringLiteral(R"(/Annots (\d+) 0 R)"));
		static const QRegularExpression link(
			QStringLiteral(R"(/Subtype /Link\s*/Rect \[\s*([-0-9.]+)\s+([-0-9.]+)\s+([-0-9.]+)\s+([-0-9.]+)\s*\])"));

		QList<Page> result;
		for (auto k = ref.globalMatch(kids.match(pdf).captured(1)); k.hasNext();) {
			const QString page = objects.value(k.next().captured(1).toInt());
			const auto b = box.match(page);
			Page p;
			p.size = QSizeF(b.captured(1).toDouble(), b.captured(2).toDouble());
			const QString list = objects.value(annots.match(page).captured(1).toInt());
			for (auto a = ref.globalMatch(list); a.hasNext();) {
				const auto l = link.match(objects.value(a.next().captured(1).toInt()));
				if (l.hasMatch())
					p.links << QRectF(QPointF(l.captured(1).toDouble(), l.captured(2).toDouble()),
									  QPointF(l.captured(3).toDouble(), l.captured(4).toDouble()));
			}
			result << p;
		}
		return result;
	}

	// Export @p project as the window's "Export in pdf" button does, from
	// an A3 landscape printer with fixed margins.
	QList<Page> exportPdf(QETProject *project, bool fit_in_page, bool to_scale = false)
	{
		const QString out = m_dir.filePath(QStringLiteral("out.pdf"));
		QFile::remove(out);
		QSettings().setValue("print/default/fitinpage", fit_in_page);
		QSettings().setValue("print/default/toscale", to_scale);
		// Portrait folios leave items beside the sheet: no question first
		QSettings().setValue("diagrameditor/warn_outside_sheet", false);

		auto printer = new QPrinter();
		printer->setPageSize(QPageSize(QPageSize::A3));
		printer->setPageOrientation(QPageLayout::Landscape);
		printer->setOutputFormat(QPrinter::PdfFormat);
		printer->setOutputFileName(out);

		ProjectPrintWindow window(project, printer);
		// After the window loaded the margins an earlier export saved.
		printer->setPageMargins(QMarginsF(7, 7, 7, 7), QPageLayout::Millimeter);
		if (!QMetaObject::invokeMethod(&window, "print"))
			return {};
		QCoreApplication::processEvents(); // the deferred PDF post-processing
		return pages(out);
	}

	// examples/industrial.qet, its folios made portrait (taller rows), all
	// of them or all but the first.
	QString portraitIndustrial(bool first_too)
	{
		QFile file(QStringLiteral(QET_EXAMPLES_DIR "/industrial.qet"));
		if (!file.open(QIODevice::ReadOnly))
			return {};
		QString xml = QString::fromUtf8(file.readAll());
		static const QRegularExpression diagram(QStringLiteral("<diagram [^>]*>"));
		static const QRegularExpression rows(QStringLiteral(R"(\brows="\d+")"));
		QString result;
		qsizetype last = 0;
		int n = 0;
		for (auto it = diagram.globalMatch(xml); it.hasNext();) {
			const auto m = it.next();
			QString tag = m.captured();
			if (n++ > 0 || first_too)
				tag.replace(rows, QStringLiteral("rows=\"20\""));
			result += xml.mid(last, m.capturedStart() - last) + tag;
			last = m.capturedEnd();
		}
		result += xml.mid(last);

		const QString path = m_dir.filePath(
			first_too ? QStringLiteral("portrait.qet") : QStringLiteral("mixed.qet"));
		QFile out(path);
		if (!out.open(QIODevice::WriteOnly))
			return {};
		out.write(result.toUtf8());
		return path;
	}

private slots:
	void initTestCase()
	{
		QVERIFY(m_dir.isValid());
	}

	void pagesFollowFolios()
	{
		const QString fixture = QFINDTESTDATA("fixtures/pdf_page_a3.qet");
		QVERIFY2(!fixture.isEmpty(), "fixture project not found");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);
		QCOMPARE(project.diagrams().size(), 2);

		const QList<Page> result = exportPdf(&project, true);
		QCOMPARE(result.size(), 2);
		const QSize a3 = QPageSize(QPageSize::A3).sizePoints();
		QCOMPARE(result.at(0).size, QSizeF(a3.height(), a3.width())); // landscape
		QCOMPARE(result.at(1).size, QSizeF(a3));                      // portrait
	}

	// Without "Fit in page" a folio is spread over several pages, all in the
	// orientation chosen in the window, as before.
	void tiledPagesKeepChosenOrientation()
	{
		const QString fixture = QFINDTESTDATA("fixtures/pdf_page_a3.qet");
		QETProject project(fixture);
		QCOMPARE(project.state(), QETProject::Ok);

		const QList<Page> result = exportPdf(&project, false);
		QVERIFY(!result.isEmpty());
		for (const Page &page : result)
			QVERIFY2(page.size.width() > page.size.height(), "a tiled page is not landscape");
	}

	// The cross-reference links on a page turned during the export land
	// where they do when the export starts on that orientation: the same
	// project with every folio portrait.
	void linksOnTurnedPages()
	{
		const QString mixed_path = portraitIndustrial(false);
		const QString portrait_path = portraitIndustrial(true);
		QVERIFY(!mixed_path.isEmpty() && !portrait_path.isEmpty());

		QETProject mixed_project(mixed_path);
		QCOMPARE(mixed_project.state(), QETProject::Ok);
		const QList<Page> mixed = exportPdf(&mixed_project, true);
		QETProject portrait_project(portrait_path);
		QCOMPARE(portrait_project.state(), QETProject::Ok);
		const QList<Page> portrait = exportPdf(&portrait_project, true);

		QCOMPARE(mixed.size(), portrait.size());
		QVERIFY(mixed.size() > 1);
		QVERIFY(mixed.first().size.width() > mixed.first().size.height());
		int links = 0;
		for (int i = 1; i < mixed.size(); ++i) {
			QVERIFY(mixed.at(i).size.height() > mixed.at(i).size.width());
			QCOMPARE(mixed.at(i).links.size(), portrait.at(i).links.size());
			for (int j = 0; j < mixed.at(i).links.size(); ++j) {
				const QRectF a = mixed.at(i).links.at(j);
				const QRectF b = portrait.at(i).links.at(j);
				QVERIFY2(qAbs(a.left() - b.left()) < 0.01 && qAbs(a.top() - b.top()) < 0.01
							 && qAbs(a.right() - b.right()) < 0.01
							 && qAbs(a.bottom() - b.bottom()) < 0.01,
						 qPrintable(QStringLiteral("page %1 link %2 moved").arg(i + 1).arg(j)));
				++links;
			}
		}
		QVERIFY2(links > 100, "too few links to test");
	}

	// A project drawn to scale written to PDF to scale keeps its links: where
	// the text is on the paper, without margin and 1 px = M / 10 mm from the
	// folio's corner.
	void linksPrintedToScale()
	{
		QETProject project;
		Diagram *first = project.addNewDiagram();
		Diagram *second = project.addNewDiagram();
		project.setScale(QETProject::Scale::Mm);
		auto *text = new IndependentTextItem(QStringLiteral("see 2"));
		first->addItem(text);
		text->setPos(300, 200);
		text->setDestination(second->uuid());

		const QList<Page> result = exportPdf(&project, false, true);
		QCOMPARE(result.size(), 2);
		QCOMPARE(result.at(0).links.size(), 1);
		QVERIFY(result.at(1).links.isEmpty());

		const QRectF source = QRectF(first->border_and_titleblock
			.borderAndTitleBlockRect()).adjusted(0, 0, 1, 1).toAlignedRect();
		const qreal pt_per_px = 72.0 / (project.pxPerMm() * 25.4);
		const qreal margin = 0;
		const qreal height = result.at(0).size.height();
		const QRectF scene = text->mapRectToScene(text->boundingRect());
		const QRectF expected(
			QPointF(margin + (scene.left() - source.left()) * pt_per_px,
					height - (margin + (scene.bottom() - source.top()) * pt_per_px)),
			QPointF(margin + (scene.right() - source.left()) * pt_per_px,
					height - (margin + (scene.top() - source.top()) * pt_per_px)));
		const QRectF got = result.at(0).links.first();
		QVERIFY2(qAbs(got.left() - expected.left()) < 1 && qAbs(got.right() - expected.right()) < 1
					 && qAbs(got.top() - expected.top()) < 1 && qAbs(got.bottom() - expected.bottom()) < 1,
				 qPrintable(QStringLiteral("got %1,%2 %3x%4 expected %5,%6 %7x%8")
					 .arg(got.left()).arg(got.top()).arg(got.width()).arg(got.height())
					 .arg(expected.left()).arg(expected.top()).arg(expected.width()).arg(expected.height())));
	}

	// A pixel project printed without fitting keeps no links, as before the
	// drawing scale: only printing to scale places them on unfitted pages
	void noLinksUnfittedInPixels()
	{
		QETProject project;
		Diagram *first = project.addNewDiagram();
		Diagram *second = project.addNewDiagram();
		auto *text = new IndependentTextItem(QStringLiteral("see 2"));
		first->addItem(text);
		text->setPos(300, 200);
		text->setDestination(second->uuid());

		const QList<Page> result = exportPdf(&project, false, false);
		QVERIFY(!result.isEmpty());
		for (const Page &page : result)
			QVERIFY(page.links.isEmpty());
	}

	// To scale, a PDF has one page per folio, as large as the folio at 1 px =
	// M / 10 mm, whatever the paper chosen: a folio larger than the paper
	// is not tiled, and its links are there
	void onePagePerFolioToScale()
	{
		QETProject project;
		Diagram *first = project.addNewDiagram();
		Diagram *second = project.addNewDiagram();
		project.setScale(QETProject::Scale::Mm);
		project.setModuleMm(5.0);
		auto *text = new IndependentTextItem(QStringLiteral("see 2"));
		first->addItem(text);
		text->setPos(300, 200);
		text->setDestination(second->uuid());

		const QList<Page> result = exportPdf(&project, false, true);
		QCOMPARE(result.size(), 2);
		const qreal pt_per_px = 72.0 / (project.pxPerMm() * 25.4);
		for (int i = 0; i < 2; ++i) {
			const Diagram *diagram = project.diagrams().at(i);
			const QRectF rect = QRectF(diagram->border_and_titleblock
				.borderAndTitleBlockRect()).adjusted(0, 0, 1, 1).toAlignedRect();
			const QSizeF expected(rect.width() * pt_per_px, rect.height() * pt_per_px);
			QVERIFY2(qAbs(result.at(i).size.width() - expected.width()) < 1.5
						 && qAbs(result.at(i).size.height() - expected.height()) < 1.5,
					 qPrintable(QStringLiteral("page %1 is %2 x %3, folio %4 x %5")
						 .arg(i + 1).arg(result.at(i).size.width()).arg(result.at(i).size.height())
						 .arg(expected.width()).arg(expected.height())));
		}
		QCOMPARE(result.at(0).links.size(), 1);
	}

	// To scale, a PDF gets a page of its own size for each sheet and the
	// preview cannot show pages of different sizes, so the help text of the
	// option says what is written to a PDF and what is done on paper
	void toScaleHelpNamesPdfAndPaper()
	{
		QETProject project;
		project.addNewDiagram();
		project.setScale(QETProject::Scale::Mm);
		auto *printer = new QPrinter();
		printer->setOutputFormat(QPrinter::PdfFormat);
		printer->setOutputFileName(m_dir.filePath(QStringLiteral("help.pdf")));
		ProjectPrintWindow window(&project, printer);
		auto *label = window.findChild<QLabel *>(QStringLiteral("m_to_scale_label"));
		QVERIFY(label);
		const QString text = label->text();
		QVERIFY2(text.contains(QStringLiteral("PDF")), qPrintable(text));
		QVERIFY2(text.contains(QStringLiteral("each sheet gets a page of its own size")),
				 qPrintable(text));
		QVERIFY2(text.contains(QStringLiteral("on paper")), qPrintable(text));
	}

	// The /FitR rectangle of a link points at its target on the target's own
	// page. Page 1 is tall and page 2 short: the Y flip to PDF points has
	// to use the height of page 2, not that of the page the link is on.
	void fitRectOnAnotherPageSize()
	{
		QETProject project;
		Diagram *first = project.addNewDiagram();
		Diagram *second = project.addNewDiagram();
		project.setScale(QETProject::Scale::Mm);
		project.setModuleMm(5.0);
		first->border_and_titleblock.setRowsCount(12);
		second->border_and_titleblock.setRowsCount(4);
		second->border_and_titleblock.setColumnsCount(10);
		auto *text = new IndependentTextItem(QStringLiteral("see 2"));
		first->addItem(text);
		text->setPos(300, 200);
		text->setDestination(second->uuid());

		const QString out = m_dir.filePath(QStringLiteral("out.pdf"));
		const QList<Page> result = exportPdf(&project, false, true);
		QCOMPARE(result.size(), 2);
		QVERIFY(result.at(0).size.height() > result.at(1).size.height() + 20);

		QFile file(out);
		QVERIFY(file.open(QIODevice::ReadOnly));
		const QString pdf = QString::fromLatin1(file.readAll());
		static const QRegularExpression fitr(QStringLiteral(
			R"(/FitR\s+([-0-9.]+)\s+([-0-9.]+)\s+([-0-9.]+)\s+([-0-9.]+))"));
		const auto m = fitr.match(pdf);
		QVERIFY2(m.hasMatch(), "no /FitR destination");
		const QRectF got(QPointF(m.captured(1).toDouble(), m.captured(2).toDouble()),
						 QPointF(m.captured(3).toDouble(), m.captured(4).toDouble()));

		// The frame of the inside of the border of folio 2, as pdf_links does
		const QRectF source = QRectF(second->border_and_titleblock
			.borderAndTitleBlockRect()).adjusted(0, 0, 1, 1).toAlignedRect();
		QRectF framed = second->border_and_titleblock.insideBorderRect();
		framed.adjust(-25, -25, 25, 25);
		const qreal pt_per_px = 72.0 / (project.pxPerMm() * 25.4);
		const qreal height = result.at(1).size.height();
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

	// To scale the page is the folio itself, not the standard sheet within
	// 3 pt of it: a folio 2 pt wider than A3 is not cut down to A3
	void pageIsNotSmallerThanTheFolioToScale()
	{
		QETProject project;
		Diagram *diagram = project.addNewDiagram();
		project.setScale(QETProject::Scale::Mm);
			// A3 high, and 3 px (2.1 pt) wider than A3
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
		const qreal folio_pt = rect.width() * 72.0 / (project.pxPerMm() * 25.4);
		const qreal a3_long = QPageSize(QPageSize::A3).sizePoints().height();
		QVERIFY2(folio_pt > a3_long + 1.5 && folio_pt < a3_long + 3,
				 qPrintable(QString::number(folio_pt)));

		const QList<Page> result = exportPdf(&project, false, true);
		QCOMPARE(result.size(), 1);
		QVERIFY2(result.first().size.width() >= folio_pt - 0.5,
				 qPrintable(QStringLiteral("page %1 pt, folio %2 pt")
					 .arg(result.first().size.width()).arg(folio_pt)));
	}
};

QET_APP_TEST_MAIN(tst_printpdforientation)
#include "tst_printpdforientation.moc"
