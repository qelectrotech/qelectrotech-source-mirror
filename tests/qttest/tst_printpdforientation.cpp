// SPDX-License-Identifier: GPL-2.0-or-later
#include "qetapptest.h"
#include "print/projectprintwindow.h"
#include "qetproject.h"

#include <QFile>
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
	QList<Page> exportPdf(QETProject *project, bool fit_in_page)
	{
		const QString out = m_dir.filePath(QStringLiteral("out.pdf"));
		QFile::remove(out);
		QSettings().setValue("print/default/fitinpage", fit_in_page);

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
};

QET_APP_TEST_MAIN(tst_printpdforientation)
#include "tst_printpdforientation.moc"
