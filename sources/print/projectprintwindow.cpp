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
#include "projectprintwindow.h"

#include "../diagram.h"
#include "../qeticons.h"
#include "../qetproject.h"
#include "../qetversion.h"
#include "../qetgraphicsitem/crossrefitem.h"
#include "../qetgraphicsitem/dynamicelementtextitem.h"
#include "../qetgraphicsitem/elementtextitemgroup.h"

#include "ui_projectprintwindow.h"

// Private Qt PDF engine for drawHyperlink() — not public API, stable since Qt4
// Requires QT += gui-private in qelectrotech.pro
#include <private/qpdf_p.h>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0) // ### Qt 6: remove
#	include <QDesktopWidget>
#else
#	if TODO_LIST
#		pragma message("@TODO remove code for QT 6 or later")
#	endif
#endif
#include <QMarginsF>
#include <QPageSetupDialog>
#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewWidget>
#include <QScreen>
#include <QFile>
#include <QRegularExpression>
#include <QMap>
#include <QTimer>
#include <QVector>

/**
 * @brief ProjectPrintWindow::ProjectPrintWindow
 * Use this static function to properly launch the print dialog.
 * @param project : project to print
 * @param format : native format to print in physical printer, or pdf format to export in pdf
 * @param parent : parent widget
 */
void ProjectPrintWindow::launchDialog(QETProject *project, QPrinter::OutputFormat format, QWidget *parent)
{
	auto printer_ = new QPrinter();
	QPrinter printer(QPrinter::HighResolution);
	printer_->setDocName(ProjectPrintWindow::docName(project));
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 1) // ### Qt 6: remove
	printer_->setOrientation(QPrinter::Landscape);
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#	endif
	printer_->setPageOrientation(QPageLayout::Landscape);
#endif

	if (format == QPrinter::NativeFormat) //To physical printer
	{
		QPrintDialog print_dialog(printer_, parent);
#ifdef Q_OS_MACOS
		print_dialog.setWindowFlags(Qt::Sheet);
#endif
		print_dialog.setWindowTitle(tr("Options d'impression", "window title"));
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)	// ### Qt 6: remove
		print_dialog.setEnabledOptions(QAbstractPrintDialog::PrintShowPageSize);
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
		qDebug()<<"Help code for QT 6 or later";
#endif
		if (print_dialog.exec() == QDialog::Rejected) {
			delete  printer_;
			return;
		}
	}
	else //To pdf file
	{
		auto dir_path = project->currentDir();
		QString file_name = QDir::toNativeSeparators(QDir::cleanPath(dir_path + "/" + printer_->docName()));
		if (!file_name.endsWith(".pdf")) {
			file_name.append(".pdf");
		}
		printer_->setCreator(QString("QElectroTech %1").arg(QetVersion::displayedVersion()));
		printer_->setOutputFileName(file_name);
		printer_->setOutputFormat(QPrinter::PdfFormat);
	}

	auto w = new ProjectPrintWindow(project, printer_, parent);
	w->showMaximized();
}

QString ProjectPrintWindow::docName(QETProject *project)
{
	QString doc_name;
	if (!project->filePath().isEmpty()) {
		doc_name = QFileInfo(project->filePath()).baseName();
	} else if (!project->title().isEmpty()) {
		doc_name = project->title();
		doc_name = QET::stringToFileName(doc_name);
	}

	if (doc_name.isEmpty()) {
		doc_name = tr("projet", "string used to generate a filename");
	}

	return doc_name;
}

/**
 * @brief ProjectPrintWindow::ProjectPrintWindow
 * Constructor, don't use this class directly, instead use ProjectPrintWindow::launchDialog static function.
 * @param project
 * @param printer : QPrinter to use. Note that ProjectPrintWindow take ownerchip of @printer
 * @param parent
 */
ProjectPrintWindow::ProjectPrintWindow(QETProject *project, QPrinter *printer, QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::ProjectPrintWindow),
	m_project(project),
	m_printer(printer)
{
	ui->setupUi(this);

	loadPageSetupForCurrentPrinter();

	m_preview = new QPrintPreviewWidget(m_printer);
	connect(m_preview, &QPrintPreviewWidget::paintRequested, this, &ProjectPrintWindow::requestPaint);
	ui->m_vertical_layout->addWidget(m_preview);

	setUpDiagramList();

	if (m_printer->outputFormat() == QPrinter::NativeFormat) //Print to physical printer
	{
		auto print_button = new QPushButton(QET::Icons::DocumentPrint, tr("Imprimer"));
		ui->m_button_box->addButton(print_button, QDialogButtonBox::ActionRole);
		connect(print_button, &QPushButton::clicked, this, &ProjectPrintWindow::print);
	}
	else	//export to pdf
	{
		auto pdf_button = new QPushButton(QET::Icons::PDF, tr("Exporter en pdf"));
		ui->m_button_box->addButton(pdf_button, QDialogButtonBox::ActionRole);
		connect(pdf_button, &QPushButton::clicked, this, &ProjectPrintWindow::exportToPDF);
	}

	auto exp = ExportProperties::defaultPrintProperties();
	ui->m_draw_border_cb->setChecked(exp.draw_border);
	ui->m_draw_titleblock_cb->setChecked(exp.draw_titleblock);
	ui->m_draw_terminal_cb->setChecked(exp.draw_terminals);
	ui->m_keep_conductor_color_cb->setChecked(exp.draw_colored_conductors);

	ui->m_date_cb->blockSignals(true);
	ui->m_date_cb->setDate(QDate::currentDate());
	ui->m_date_cb->blockSignals(false);

#ifdef Q_OS_WINDOWS
	/*
	 * On windows, the QPageSetupDialog use the native dialog.
	 * This dialog can only manage physical printer ("native printer")
	 */
	if (m_printer->outputFormat() == QPrinter::PdfFormat)
	{
		ui->m_page_setup->setDisabled(true);
		ui->m_page_setup->setText(tr("Mise en page (non disponible sous Windows pour l'export PDF)"));
	}
#endif

	m_backup_diagram_background_color = Diagram::background_color;
	Diagram::background_color = Qt::white;
}

/**
 * @brief ProjectPrintWindow::~ProjectPrintWindow
 */
ProjectPrintWindow::~ProjectPrintWindow()
{
	delete ui;
	delete m_printer;
	Diagram::background_color = m_backup_diagram_background_color;
}

/**
 * @brief ProjectPrintWindow::requestPaint
 * @param slot called when m_preview emit paintRequested
 */
/**
 * @brief ProjectPrintWindow::pdfConvertUriToGoTo
 * Post-processes a Qt-generated PDF to replace URI link annotations
 * (file:///path/to/file.pdf#page=N) with native PDF GoTo actions
 * ([pageObj 0 R /Fit]).  This makes cross-reference links work in all
 * PDF viewers regardless of where the file is stored.
 *
 * The function:
 *   1. Reads the PDF as raw bytes.
 *   2. Collects page object numbers in document order by scanning for
 *      objects that contain "/Type /Page" (but not "/Type /Pages").
 *   3. Replaces every annotation action block
 *        /S /URI\n/URI (file://...#page=N)
 *      with
 *        /S /GoTo\n/D [<pageObj> 0 R /Fit]
 *   4. Rebuilds the cross-reference table (offsets change because the
 *      replacement strings have different lengths).
 *   5. Writes the result back to the same file.
 *
 * The function is intentionally conservative: if any step fails (file
 * not found, malformed PDF, no URI annotations) it returns silently
 * without corrupting the file.
 */
static void pdfConvertUriToGoTo(const QString &pdfPath)
{
	// --- 1. Read raw bytes ---
	QFile f(pdfPath);
	if (!f.open(QIODevice::ReadOnly)) return;
	QByteArray data = f.readAll();
	f.close();

	// --- 2. Collect page object numbers in document order ---
	// Read them from the page tree (/Type /Pages -> /Kids [ N 0 R ... ]).
	// This is reliable; scanning raw bytes for "/Type /Page" is NOT: that
	// marker also occurs inside content streams, and a forward lookahead
	// wrongly tags neighbouring objects (it found 280 "pages" for a 137-page
	// document). Qt writes a single, flat /Kids array listing every page.
	QVector<int> pageObjs;
	{
		int pagesPos = data.indexOf("/Type /Pages");
		int kidsPos  = (pagesPos == -1) ? -1 : data.indexOf("/Kids", pagesPos);
		int lb       = (kidsPos  == -1) ? -1 : data.indexOf('[', kidsPos);
		int rb       = (lb       == -1) ? -1 : data.indexOf(']', lb);
		if (lb != -1 && rb != -1 && rb > lb) {
			const QString kids =
				QString::fromLatin1(data.mid(lb + 1, rb - lb - 1));
			QRegularExpression re(QStringLiteral("(\\d+)\\s+\\d+\\s+R"));
			auto it = re.globalMatch(kids);
			while (it.hasNext()) {
				int objNum = it.next().captured(1).toInt();
				if (objNum > 0) pageObjs.append(objNum);
			}
		}
	}

	if (pageObjs.isEmpty()) return;  // nothing to do

	// --- 3. Replace URI annotations with GoTo ---
	// Pattern (Qt always writes exactly this):
	//   /S /URI\n/URI (file:///...<anything>#page=N)\n
	// or (older patches without file://):
	//   /S /URI\n/URI (page=N)\n
	bool changed = false;
	{
		// We do a manual scan to handle variable-length replacements.
		QByteArray out;
		out.reserve(data.size());

		const QByteArray sUri  = "/S /URI\n/URI (";
		const QByteArray sGoTo = "/S /GoTo\n/D [";
		int pos = 0;

		while (pos < data.size()) {
			int found = data.indexOf(sUri, pos);
			if (found == -1) {
				out.append(data.mid(pos));
				break;
			}

			// Copy everything up to the match
			out.append(data.mid(pos, found - pos));

			// Find closing ')' of the URI value
			int uriStart = found + sUri.size();
			int closeParen = data.indexOf(')', uriStart);
			if (closeParen == -1) {
				// Malformed — copy rest verbatim
				out.append(data.mid(found));
				pos = data.size();
				break;
			}

			QByteArray uriVal = data.mid(uriStart, closeParen - uriStart);

			// Extract page number: look for #page=N or bare page=N
			int pageNum = -1;
			int hashPos = uriVal.lastIndexOf("#page=");
			int digitStart = -1;
			if (hashPos != -1) {
				digitStart = hashPos + 6;
			} else if (uriVal.startsWith("page=")) {
				digitStart = 5;
			}
			if (digitStart != -1) {
				// Take only the leading digits: the fragment may carry extra
				// parameters after the page number (e.g. "22&fitr=15_489_..."),
				// and QByteArray::toInt() would fail on the whole remainder.
				int e = digitStart;
				while (e < uriVal.size()
				       && uriVal[e] >= '0' && uriVal[e] <= '9')
					++e;
				if (e > digitStart)
					pageNum = uriVal.mid(digitStart, e - digitStart).toInt();
			}

			if (pageNum >= 1 && pageNum <= pageObjs.size()) {
				// Valid page reference — emit GoTo action.
				int pageObjNum = pageObjs[pageNum - 1];

				// Optional precise destination: &fitr=Left_Bottom_Right_Top
				// (integer PDF points). If present -> /FitR (frame the element);
				// otherwise -> /Fit (whole page, top).
				QByteArray dest = " /Fit]";
				int fr = uriVal.indexOf("fitr=");
				if (fr != -1) {
					QByteArray rest = uriVal.mid(fr + 5);
					// stop at first char that is not part of the number list
					int end = 0;
					while (end < rest.size()
					       && ((rest[end] >= '0' && rest[end] <= '9')
					           || rest[end] == '_' || rest[end] == '-'))
						++end;
					QList<QByteArray> parts = rest.left(end).split('_');
					if (parts.size() == 4) {
						dest = " /FitR " + parts[0] + " " + parts[1] + " "
						       + parts[2] + " " + parts[3] + "]";
					}
				}

				QByteArray goTo = sGoTo
					+ QByteArray::number(pageObjNum)
					+ " 0 R" + dest;
				out.append(goTo);
				changed = true;
			} else {
				// Unknown page — keep original URI
				out.append(sUri);
				out.append(uriVal);
				out.append(')');
			}

			pos = closeParen + 1;  // skip past ')'
		}

		if (!changed) return;  // nothing was replaced
		data = out;
	}

	// --- 4. Rebuild xref table ---
	// Find start of existing xref (last occurrence)
	int xrefStart = data.lastIndexOf("\nxref\n");
	if (xrefStart == -1) xrefStart = data.lastIndexOf("\nxref ");
	if (xrefStart == -1) return;  // malformed PDF
	++xrefStart;  // skip the leading '\n'

	QByteArray body = data.left(xrefStart);

	// Collect all object offsets from the body
	QMap<int, int> offsets;  // objNum -> byte offset
	{
		const QByteArray objMarker = " 0 obj";
		int pos = 0;
		while ((pos = body.indexOf(objMarker, pos)) != -1) {
			int numStart = pos - 1;
			while (numStart > 0 && body[numStart-1] != '\n' && body[numStart-1] != '\r')
				--numStart;
			QByteArray numStr = body.mid(numStart, pos - numStart).trimmed();
			bool ok = false;
			int objNum = numStr.toInt(&ok);
			if (ok && objNum > 0)
				offsets[objNum] = numStart;
			++pos;
		}
	}

	if (offsets.isEmpty()) return;

	int maxObj = offsets.lastKey();

	// Build xref table
	QByteArray xref;
	xref += "xref\n";
	xref += "0 " + QByteArray::number(maxObj + 1) + "\n";
	xref += "0000000000 65535 f \n";
	for (int i = 1; i <= maxObj; ++i) {
		if (offsets.contains(i)) {
			xref += QByteArray::number(offsets[i]).rightJustified(10, '0')
				+ " 00000 n \n";
		} else {
			xref += "0000000000 65535 f \n";
		}
	}

	// Find trailer dict from the original xref section
	int trailerPos = data.indexOf("trailer", xrefStart);
	int trailerEnd = -1;
	if (trailerPos != -1) {
		trailerEnd = data.indexOf("%%EOF", trailerPos);
		if (trailerEnd != -1) trailerEnd += 5;
	}

	QByteArray trailer;
	if (trailerPos != -1 && trailerEnd != -1)
		trailer = data.mid(trailerPos, trailerEnd - trailerPos);
	else
		trailer = "trailer\n<<>>\n%%EOF";

	int newXrefOffset = body.size();

	QByteArray result;
	result.reserve(body.size() + xref.size() + trailer.size() + 30);
	result += body;
	result += xref;
	result += trailer;
	result += "\nstartxref\n";
	result += QByteArray::number(newXrefOffset);
	result += "\n%%EOF\n";

	// --- 5. Write back ---
	QFile out(pdfPath);
	if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
	out.write(result);
	out.close();
}

void ProjectPrintWindow::requestPaint()
{
	#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
		#ifdef Q_OS_WIN
			#ifdef QT_DEBUG
			qDebug() << "--";
			qDebug() << "DiagramPrintDialog::print  printer_->resolution() before " << m_printer->resolution();
			qDebug() << "DiagramPrintDialog::print  screennumber " << QApplication::desktop()->screenNumber();
			#endif

			QScreen *srn = QApplication::screens().at(QApplication::desktop()->screenNumber());
			qreal dotsPerInch = (qreal)srn->logicalDotsPerInch();
			m_printer->setResolution(dotsPerInch);

			#ifdef QT_DEBUG
				qDebug() << "DiagramPrintDialog::print  dotsPerInch " << dotsPerInch;
				qDebug() << "DiagramPrintDialog::print  printer_->resolution() after" << m_printer->resolution();
			qDebug() << "--";
			#endif
		#endif
	#endif

	if (!m_project->diagrams().count()) {
		return;
	}

	// Build diagram -> first physical PDF page number map (1-based)
	// Must be done before the print loop since page numbers depend on order
	QMap<Diagram*, int> diagramPageMap;
	{
		int pageNum = 1;
		for (auto diagram : selectedDiagram()) {
			diagramPageMap.insert(diagram, pageNum);
			// Each diagram may span multiple pages if not fit_page
			if (!ui->m_fit_in_page_cb->isChecked()) {
				auto option = exportProperties();
				bool full_page = m_printer->fullPage();
				int h = horizontalPagesCount(diagram, option, full_page);
				int v = verticalPagesCount(diagram, option, full_page);
				pageNum += h * v;
			} else {
				pageNum += 1;
			}
		}
	}

	bool first = true;
	QPainter painter(m_printer);

	// A real PDF export uses the QPdfEngine; the on-screen preview uses a
	// preview paint engine. We only post-process when actually writing a PDF.
	const bool pdfExport =
		(m_printer->outputFormat() == QPrinter::PdfFormat)
		&& (dynamic_cast<QPdfEngine*>(painter.paintEngine()) != nullptr);

	for (auto diagram : selectedDiagram())
	{
		first ? first = false : m_printer->newPage();
		printDiagram(diagram, ui->m_fit_in_page_cb->isChecked(), &painter, m_printer, diagramPageMap);
	}

	// Note: do NOT call painter.end() or pdfConvertUriToGoTo() here.
	// We are inside the paintRequested slot: the QPrintPreviewWidget still
	// owns the paint cycle.  On macOS arm64 (Metal/CALayer compositor),
	// closing the QPainter manually inside this slot leaves the backing
	// store in an undefined state, producing a black screen after export.
	// pdfConvertUriToGoTo() is deferred to print() via QTimer::singleShot(0).
}

/**
 * @brief ProjectPrintWindow::printDiagram
 * Print @diagram on the @printer
 * @param diagram
 * @param fit_page
 * @param printer
 */
void ProjectPrintWindow::printDiagram(Diagram *diagram, bool fit_page, QPainter *painter, QPrinter *printer, const QMap<Diagram*, int> &diagramPageMap)
{

	////Prepare the print////
	// Deselect all
	diagram->deselectAll();
	// Disable focus flags
	QList<QGraphicsItem *> focusable_items;
	for (auto qgi : diagram->items()) {
		if (qgi->flags() & QGraphicsItem::ItemIsFocusable) {
			focusable_items << qgi;
			qgi->setFlag(QGraphicsItem::ItemIsFocusable, false);
		}
	}
	// Disable interaction
	for (auto view : diagram->views()) {
		view->setInteractive(false);
	}
	auto option = exportProperties();
	saveReloadDiagramParameters(diagram, option, true);
	////Prepare end////


	auto full_page = printer->fullPage();
	auto diagram_rect = QRectF(diagramRect(diagram, option));
	if (fit_page) {
		diagram->render(painter, QRectF(), diagram_rect, Qt::KeepAspectRatio);
	} else {
		// Print on one or several pages
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 1) // ### Qt 6: remove
		auto printed_rect = full_page ? printer->paperRect() : printer->pageRect();
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
	qDebug()<<"Help code for QT 6 or later";
	auto printed_rect = full_page ? printer->paperRect(QPrinter::Millimeter) :
									printer->pageRect(QPrinter::Millimeter);
#endif
		auto used_width  = printed_rect.width();
		auto used_height = printed_rect.height();
		auto h_pages_count = horizontalPagesCount(diagram, option, full_page);
		auto v_pages_count = verticalPagesCount(diagram, option, full_page);

		QVector<QVector<QRectF>> page_grid;
		// The diagram is printed on a matrix of sheet
		// scrolls through the rows of the matrix
		auto y_offset = 0;
		for (auto i = 0; i < v_pages_count; ++i)
		{
			page_grid << QVector<QRectF>();
			// scrolls through the lines of sheet
			auto x_offset = 0;
			for (auto j=0 ; j<h_pages_count ; ++j)
			{
				page_grid.last() << QRectF(
					QPoint(x_offset, y_offset),
					QSize(
						qMin(used_width, diagram_rect.width() - x_offset),
						qMin(used_height, diagram_rect.height() - y_offset)));

				x_offset += used_width;
			}
			y_offset += used_height;
		}

		// Retains only the pages to be printed
		QVector<QRectF> page_to_print;
		for (auto i=0 ; i < v_pages_count ; ++i) {
			for (int j=0 ; j < h_pages_count ; ++j) {
				page_to_print << page_grid.at(i).at(j);
			}
		}

		// Scrolls through the page for print
		bool first_ = true;
		for (auto& page : page_to_print)
		{
			first_ ? first_ = false : m_printer->newPage();
			diagram->render(
				painter,
				QRectF(QPoint(0, 0), page.size()),
				page.translated(diagram_rect.topLeft()),
				Qt::KeepAspectRatio);
		}
	}

	////Inject PDF cross-reference links////
	if (printer->outputFormat() == QPrinter::PdfFormat && fit_page) {
		auto *pdfEngine = dynamic_cast<QPdfEngine*>(painter->paintEngine());
		if (pdfEngine) {

			// QGraphicsScene::render() fait save()/restore() : worldTransform()
			// est revenu a l'identite ici. On reconstruit DONC explicitement la
			// transform appliquee par :
			//   diagram->render(painter, QRectF(), diagram_rect, KeepAspectRatio)
			// cible vide => painter->viewport() ; source = diagram_rect ; centre.
			const QRectF target = QRectF(painter->viewport());
			const QRectF source = QRectF(diagram_rect); // meme source que render()

			// render() ANCRE en haut-gauche (pas de centrage) :
			//   translate(target.topLeft) . scale(s,s) . translate(-source.topLeft)
			// On reproduit EXACTEMENT ca — surtout PAS de (target-source*s)/2.
			const qreal s = qMin(target.width()  / source.width(),
			                     target.height() / source.height());

			QTransform fit;
			fit.translate(target.x(), target.y());
			fit.scale(s, s);
			fit.translate(-source.x(), -source.y());   // scene -> pixels device

			// IMPORTANT : QPdfEngine::drawHyperlink() applique lui-meme
			// pageMatrix() (echelle 72/resolution + inversion de Y + marges).
			// On lui passe donc le rectangle en PIXELS DEVICE, sans aucune
			// conversion en points ni flip de notre cote.
			const QRectF pageBounds(0, 0, target.width(), target.height());

			// ---- Device-pixels -> PDF points, replicating QPdfEnginePrivate::pageMatrix()
			// (same geometry for every page: same printer, page size and margins). ----
			const qreal pt_scale = 72.0 / printer->resolution();
			const qreal fullH_pt = printer->pageLayout().fullRectPoints().height();
			const bool  fullPageMode =
				(printer->pageLayout().mode() == QPageLayout::FullPageMode);
			const QRect paintPx =
				printer->pageLayout().paintRectPixels(printer->resolution());
			auto devToPdf = [=](const QPointF &d) -> QPointF {
				qreal dx = d.x(), dy = d.y();
				if (!fullPageMode) { dx += paintPx.left(); dy += paintPx.top(); }
				return QPointF(pt_scale * dx, fullH_pt - pt_scale * dy);
			};

			// Compute, in PDF points on its OWN page, the rectangle to frame for a
			// target element (used as a /FitR destination so the link zooms onto it).
			auto destRectPdf = [&](Element *tgt) -> QRectF {
				Diagram *dg = tgt ? tgt->diagram() : nullptr;
				if (!dg) return QRectF();
				const QRectF srcT = QRectF(diagramRect(dg, exportProperties()));
				if (srcT.width() <= 0.0 || srcT.height() <= 0.0) return QRectF();
				const qreal sT = qMin(target.width()  / srcT.width(),
				                      target.height() / srcT.height());
				QTransform fitT;
				fitT.translate(target.x(), target.y());
				fitT.scale(sT, sT);
				fitT.translate(-srcT.x(), -srcT.y());

				QRectF elemScene = tgt->mapRectToScene(tgt->boundingRect());
				// Frame the element with a little context, and enforce a minimum
				// framed size so tiny contacts don't zoom in extremely.
				const qreal pad = 25.0;
				elemScene.adjust(-pad, -pad, pad, pad);
				const qreal minSide = 160.0;
				if (elemScene.width()  < minSide)
					elemScene.adjust(-(minSide - elemScene.width())  / 2.0, 0,
					                  (minSide - elemScene.width())  / 2.0, 0);
				if (elemScene.height() < minSide)
					elemScene.adjust(0, -(minSide - elemScene.height()) / 2.0,
					                 0,  (minSide - elemScene.height()) / 2.0);

				const QRectF devT = fitT.mapRect(elemScene);
				const QPointF a = devToPdf(devT.topLeft());
				const QPointF b = devToPdf(devT.bottomRight());
				return QRectF(QPointF(qMin(a.x(), b.x()), qMin(a.y(), b.y())),
				              QPointF(qMax(a.x(), b.x()), qMax(a.y(), b.y())));
			};

			auto injectLink = [&](const QRectF &sceneRect, Element *targetElmt) {
				if (!targetElmt || !targetElmt->diagram()) return;
				const int targetPage =
					diagramPageMap.value(targetElmt->diagram(), -1);
				if (targetPage < 1) return;
				const QRectF devRect = fit.mapRect(sceneRect);
				if (!devRect.isValid() || !pageBounds.intersects(devRect)) return;

				QString frag = QString("page=%1").arg(targetPage);
				const QRectF d = destRectPdf(targetElmt);   // /FitR L_B_R_T
				if (d.isValid())
					frag += QString("&fitr=%1_%2_%3_%4")
						.arg(qRound(d.left())).arg(qRound(d.top()))
						.arg(qRound(d.right())).arg(qRound(d.bottom()));

				QUrl url = QUrl::fromLocalFile(printer->outputFileName());
				url.setFragment(frag);
				pdfEngine->drawHyperlink(devRect, url);
			};

			for (auto *item : diagram->items()) {

				// --- CrossRefItem links ---
				if (auto *xref = dynamic_cast<CrossRefItem*>(item)) {
					for (auto it = xref->hoveredContactsMap().begin();
					     it != xref->hoveredContactsMap().end(); ++it)
					{
						Element *targetElmt = it.key();
						if (!targetElmt || !targetElmt->diagram()) continue;
						// it.value() est en coords LOCALES du CrossRefItem -> scene
						injectLink(xref->mapRectToScene(it.value()), targetElmt);
					}
					continue;
				}

				// --- Folio report links (DynamicElementTextItem) ---
				if (auto *deti = dynamic_cast<DynamicElementTextItem*>(item)) {
					Element *parent = deti->parentElement();
					if (!parent) continue;

					// (a) Report element : label -> linked report on another folio
					if (parent->linkType() & Element::AllReport) {
						if (parent->linkedElements().isEmpty()) continue;

						bool showsLabel =
							(deti->textFrom() == DynamicElementTextItem::ElementInfo
							 && deti->infoName() == QLatin1String("label")) ||
							(deti->textFrom() == DynamicElementTextItem::CompositeText
							 && deti->compositeText().contains(QStringLiteral("%{label}")));
						if (!showsLabel) continue;

						Element *targetElmt = parent->linkedElements().first();
						if (!targetElmt || !targetElmt->diagram()) continue;

						injectLink(deti->mapRectToScene(deti->boundingRect()), targetElmt);
						continue;
					}

					// (b) Slave element : the "(folio-pos)" text -> master element
					if (parent->linkType() == Element::Slave) {
						QGraphicsTextItem *sx = deti->slaveXrefItem();
						Element *master = deti->masterElement();
						if (sx && master && master->diagram()) {
							injectLink(sx->mapRectToScene(sx->boundingRect()), master);
						}
						continue;
					}
					continue;
				}

				// --- Slave cross-reference carried by a grouped text ---
				if (auto *grp = dynamic_cast<ElementTextItemGroup*>(item)) {
					Element *parent = grp->parentElement();
					if (!parent || parent->linkType() != Element::Slave) continue;
					if (parent->linkedElements().isEmpty()) continue;
					QGraphicsTextItem *sx = grp->slaveXrefItem();
					if (!sx) continue;
					Element *master = parent->linkedElements().first();
					if (!master || !master->diagram()) continue;
					injectLink(sx->mapRectToScene(sx->boundingRect()), master);
					continue;
				}
			}
		}
	}
	////PDF links end////

	////Print is finished, restore diagram and graphics item properties
	for (auto view : diagram->views()) {
		view->setInteractive(true);
	}
	for (auto qgi : focusable_items) {
		qgi->setFlag(QGraphicsItem::ItemIsFocusable, true);
	}
	saveReloadDiagramParameters(diagram, option, false);
}

/**
 * @brief ProjectPrintWindow::diagramRect
 * @param diagram
 * @param option
 * @return The rectangle of diagram to be printed
 */
QRect ProjectPrintWindow::diagramRect(Diagram *diagram, const ExportProperties &option) const
{
	auto diagram_rect = diagram->border_and_titleblock.borderAndTitleBlockRect();
	if (!option.draw_titleblock) {
		auto titleblock_height = diagram->border_and_titleblock.titleBlockRect().height();
		diagram_rect.setHeight(diagram_rect.height() - titleblock_height);
	}

		//Adjust the border of diagram to 1px (width of the line)
	diagram_rect.adjust(0,0,1,1);

	return (diagram_rect.toAlignedRect());
}

/**
 * @brief ProjectPrintWindow::horizontalPagesCount
 * @param diagram : diagram to print
 * @param option : option used to render
 * @param full_page : full page or not
 * @return The width of the "poster" in number of page for print the diagram
 * with the orientation and the paper format used by the actual printer
 */
int ProjectPrintWindow::horizontalPagesCount(
		Diagram *diagram, const ExportProperties &option, bool full_page) const
{
	QRect printable_area;
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 1) // ### Qt 6: remove
	printable_area = full_page ? m_printer->paperRect() : m_printer->pageRect();
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#	endif
	printable_area =
		full_page ?
			m_printer->pageLayout().fullRectPixels(m_printer->resolution()) :
			m_printer->pageLayout().paintRectPixels(m_printer->resolution());
#endif
	QRect diagram_rect = diagramRect(diagram, option);

	int h_pages_count = int(ceil(qreal(diagram_rect.width()) / qreal(printable_area.width())));
	return(h_pages_count);
}

/**
 * @brief ProjectPrintWindow::verticalPagesCount
 * @param diagram : diagram to print
 * @param option : option used to render
 * @param full_page : full page or not
 * @return The height of the "poster" in number of pages for print the diagram
 * with the orientation and paper format used by the actual printer
 */
int ProjectPrintWindow::verticalPagesCount(
		Diagram *diagram, const ExportProperties &option, bool full_page) const
{
	QRect printable_area;
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 1) // ### Qt 6: remove
	printable_area = full_page ? m_printer->paperRect() : m_printer->pageRect();
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#	endif
	printable_area =
		full_page ?
			m_printer->pageLayout().fullRectPixels(m_printer->resolution()) :
			m_printer->pageLayout().paintRectPixels(m_printer->resolution());
#endif
	QRect diagram_rect = diagramRect(diagram, option);

	int v_pages_count = int(ceil(qreal(diagram_rect.height()) / qreal(printable_area.height())));
	return(v_pages_count);
}

ExportProperties ProjectPrintWindow::exportProperties() const
{
	ExportProperties exp;
	exp.draw_border             = ui->m_draw_border_cb->isChecked();
	exp.draw_titleblock         = ui->m_draw_titleblock_cb->isChecked();
	exp.draw_terminals          = ui->m_draw_terminal_cb->isChecked();
	exp.draw_colored_conductors = ui->m_keep_conductor_color_cb->isChecked();
	exp.draw_grid = false;

	return exp;
}

void ProjectPrintWindow::setUpDiagramList()
{
	auto layout = new QVBoxLayout();
	auto widget = new QWidget();
	widget->setLayout(layout);
	widget->setMinimumSize(170, 0);
	widget->setMaximumSize(470, 10000);
	widget->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
	ui->m_diagram_list->setWidget(widget);

	for (auto diagram : m_project->diagrams())
	{
		auto title = diagram->title();
		if (title.isEmpty()) {
			title = tr("Folio sans titre");
		}

		auto checkbox = new QCheckBox(title);
		checkbox->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum));
		checkbox->setChecked(true);
		layout->addWidget(checkbox, 0, Qt::AlignLeft | Qt::AlignTop);
		connect(checkbox, &QCheckBox::clicked, m_preview, &QPrintPreviewWidget::updatePreview);
		m_diagram_list_hash.insert(diagram, checkbox);
	}
	layout->addStretch();
}

QString ProjectPrintWindow::settingsSectionName(const QPrinter *printer)
{
	QPrinter::OutputFormat printer_format = printer -> outputFormat();
	if (printer_format == QPrinter::NativeFormat) {
		return(printer -> printerName().replace(" ", "_"));
	} else if (printer_format == QPrinter::PdfFormat) {
		return("QET_PDF_Printing");
	}
	return(QString());
}

void ProjectPrintWindow::loadPageSetupForCurrentPrinter()
{
	QSettings settings;
	QString	  printer_section = settingsSectionName(m_printer);

	while (! settings.group().isEmpty()) settings.endGroup();
	settings.beginGroup("printers");
	if (! settings.childGroups().contains(printer_section))
	{
		settings.endGroup();
		return;
	}

	settings.beginGroup(printer_section);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 1) // ### Qt 6: remove
	if (settings.contains("orientation")) {
		QString value = settings.value("orientation", "landscape").toString();
		m_printer -> setOrientation(value == "landscape" ? QPrinter::Landscape : QPrinter::Portrait);
	}
	if (settings.contains("papersize")) {
		int value = settings.value("papersize", QPrinter::A4).toInt();
		if (value == QPrinter::Custom) {
			bool w_ok, h_ok;
			int w = settings.value("customwidthmm", -1).toInt(&w_ok);
			int h = settings.value("customheightmm", -1).toInt(&h_ok);
			if (w_ok && h_ok && w != -1 && h != -1) {
				m_printer -> setPaperSize(QSizeF(w, h), QPrinter::Millimeter);
			}
		} else if (value < QPrinter::Custom) {
			m_printer -> setPaperSize(static_cast<QPrinter::PaperSize>(value));
		}
	}

	qreal margins[4];
	m_printer -> getPageMargins(&margins[0], &margins[1], &margins[2], &margins[3], QPrinter::Millimeter);
	QStringList margins_names(QStringList() << "left" << "top" << "right" << "bottom");
	for (int i = 0 ; i < 4 ; ++ i) {
		bool conv_ok;
		qreal value = settings.value("margin" + margins_names.at(i), -1.0).toReal(&conv_ok);
		if (conv_ok && value != -1.0) margins[i] = value;
	}
	m_printer->setPageMargins(
		margins[0],
		margins[1],
		margins[2],
		margins[3],
		QPrinter::Millimeter);
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#	endif
	if (settings.contains("orientation"))
	{
		QString value = settings.value("orientation", "landscape").toString();
		m_printer->setPageOrientation(
			value == "landscape" ? QPageLayout::Landscape :
								   QPageLayout::Portrait);
	}
	if (settings.contains("papersize"))
	{
		int value = settings.value("papersize", QPageSize::A4).toInt();
		if (value == QPageSize::Custom)
		{
			bool w_ok, h_ok;
			int	 w = settings.value("customwidthmm", -1).toInt(&w_ok);
			int	 h = settings.value("customheightmm", -1).toInt(&h_ok);
			if (w_ok && h_ok && w != -1 && h != -1)
			{
				m_printer->setPageSize(QPageSize(
					QSizeF(w, h),
					QPageSize::Millimeter,
					"Custom",
					QPageSize::FuzzyMatch));
			}
		}
		else if (value < QPageSize::Custom)
		{
			QPageSize var;
			var.id(value);
			m_printer->setPageSize(var);
		}
	}

	qreal margins[4];
	margins[0] = m_printer->pageLayout().margins().left();
	margins[1] = m_printer->pageLayout().margins().top();
	margins[2] = m_printer->pageLayout().margins().right();
	margins[3] = m_printer->pageLayout().margins().bottom();
	QStringList margins_names(
		QStringList() << "left"
					  << "top"
					  << "right"
					  << "bottom");
	for (int i = 0; i < 4; ++i)
	{
		bool  conv_ok;
		qreal value = settings.value("margin" + margins_names.at(i), -1.0)
						  .toReal(&conv_ok);
		if (conv_ok && value != -1.0) margins[i] = value;
	}
	m_printer->setPageMargins(
		QMarginsF(margins[0], margins[1], margins[2], margins[3]),
		QPageLayout::Millimeter);
#endif
	m_printer->setFullPage(
		settings.value("fullpage", "false").toString() == "true");

	settings.endGroup();
	settings.endGroup();
}

void ProjectPrintWindow::savePageSetupForCurrentPrinter()
{
	QSettings settings;
	QString printer_section = settingsSectionName(m_printer);

	while (!settings.group().isEmpty()) settings.endGroup();
	settings.beginGroup("printers");
	settings.beginGroup(printer_section);

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 1) // ### Qt 6: remove
	settings.setValue("orientation", m_printer -> orientation() == QPrinter::Portrait ? "portrait" : "landscape");
	settings.setValue("papersize", int(m_printer -> paperSize()));
	if (m_printer -> paperSize() == QPrinter::Custom) {
		QSizeF size = m_printer -> paperSize(QPrinter::Millimeter);
		settings.setValue("customwidthmm", size.width());
		settings.setValue("customheightmm", size.height());
	} else {
		settings.remove("customwidthmm");
		settings.remove("customheightmm");
	}
	qreal left, top, right, bottom;
	m_printer
		->getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
	settings.setValue("marginleft", left);
	settings.setValue("margintop", top);
	settings.setValue("marginright", right);
	settings.setValue("marginbottom", bottom);
	settings.setValue("fullpage", m_printer->fullPage() ? "true" : "false");
	settings.endGroup();
	settings.endGroup();
	settings.sync();

#else
#	if TODO_LIST
#		pragma message("@TODO remove code for QT 6 or later")
#	endif
	qDebug() << "Help code for QT 6 or later";

	settings.setValue(
		"orientation",
		m_printer->pageLayout().orientation() == QPageLayout::Portrait ?
			"portrait" :
			"landscape");
	settings.setValue(
		"papersize",
		int(m_printer->pageLayout().pageSize().id()));
	if (m_printer->pageLayout().pageSize().id() == QPageSize::Custom)
	{
		QSizeF size =
			m_printer->pageLayout().pageSize().size(QPageSize::Millimeter);
		settings.setValue("customwidthmm", size.width());
		settings.setValue("customheightmm", size.height());
	}
	else
	{
		settings.remove("customwidthmm");
		settings.remove("customheightmm");
	}
	settings.setValue("marginleft", m_printer->pageLayout().margins().left());
	settings.setValue("margintop", m_printer->pageLayout().margins().top());
	settings.setValue("marginright", m_printer->pageLayout().margins().right());
	settings.setValue(
		"marginbottom",
		m_printer->pageLayout().margins().bottom());
	settings.setValue("fullpage", m_printer->fullPage() ? "true" : "false");
	settings.endGroup();
	settings.endGroup();
	settings.sync();
#endif
}

/**
 * @brief ProjectPrintWindow::saveReloadDiagramParameters
 * Save or restore the parameter of @diagram
 * @param diagram
 * @param options
 * @param save
 */
void ProjectPrintWindow::saveReloadDiagramParameters(Diagram *diagram, const ExportProperties &options, bool save)
{
	static ExportProperties state_exportProperties;

	if (save) {
		state_exportProperties = diagram -> applyProperties(options);
	} else {
		diagram -> applyProperties(state_exportProperties);
	}
}

QList<Diagram *> ProjectPrintWindow::selectedDiagram() const
{
	QList<Diagram *> selected_diagram;
	for (auto diagram : m_project->diagrams()) {
		auto cb = m_diagram_list_hash[diagram];
		if (cb && cb->isChecked()) {
			selected_diagram << diagram;
		}
	}

	return selected_diagram;
}

void ProjectPrintWindow::exportToPDF()
{
	auto file_name = QFileDialog::getSaveFileName(this, tr("Exporter sous : "), m_printer->outputFileName(), tr("Fichier (*.pdf)"));
	if (file_name.isEmpty()) {
		return;
	}
	m_printer->setOutputFileName(file_name);
	m_printer->setOutputFormat(QPrinter::PdfFormat);
	print();
}

void ProjectPrintWindow::on_m_draw_border_cb_clicked()          { m_preview->updatePreview(); }
void ProjectPrintWindow::on_m_draw_titleblock_cb_clicked()      { m_preview->updatePreview(); }
void ProjectPrintWindow::on_m_keep_conductor_color_cb_clicked() { m_preview->updatePreview(); }
void ProjectPrintWindow::on_m_draw_terminal_cb_clicked()        { m_preview->updatePreview(); }
void ProjectPrintWindow::on_m_fit_in_page_cb_clicked()          { m_preview->updatePreview(); }
void ProjectPrintWindow::on_m_use_full_page_cb_clicked()
{
	m_printer->setFullPage(ui->m_use_full_page_cb->isChecked());
	m_preview->updatePreview();
}

void ProjectPrintWindow::on_m_zoom_out_action_triggered() {
	m_preview->zoomOut(4.0/3.0);
}

void ProjectPrintWindow::on_m_zoom_in_action_triggered() {
	m_preview->zoomIn(4.0/3.0);
}

void ProjectPrintWindow::on_m_adjust_width_action_triggered() {
	m_preview->fitToWidth();
}

void ProjectPrintWindow::on_m_adjust_page_action_triggered() {
	m_preview->fitInView();
}

void ProjectPrintWindow::on_m_landscape_action_triggered() {
	m_preview->setLandscapeOrientation();
}

void ProjectPrintWindow::on_m_portrait_action_triggered() {
	m_preview->setPortraitOrientation();
}

void ProjectPrintWindow::on_m_first_page_action_triggered() {
	m_preview->setCurrentPage(1);
}

void ProjectPrintWindow::on_m_previous_page_action_triggered()
{
	auto previous_page = m_preview->currentPage() - 1;
	m_preview->setCurrentPage(std::max(previous_page, 0));
}

void ProjectPrintWindow::on_m_next_page_action_triggered()
{
	auto next_page = m_preview->currentPage() + 1;
	m_preview->setCurrentPage(std::min(next_page, m_preview->pageCount()));
}

void ProjectPrintWindow::on_m_last_page_action_triggered() {
	m_preview->setCurrentPage(m_preview->pageCount());
}

void ProjectPrintWindow::on_m_display_single_page_action_triggered() {
	m_preview->setSinglePageViewMode();
}

void ProjectPrintWindow::on_m_display_two_page_action_triggered() {
	m_preview->setFacingPagesViewMode();
}

void ProjectPrintWindow::on_m_display_all_page_action_triggered() {
	m_preview->setAllPagesViewMode();
}

void ProjectPrintWindow::on_m_page_setup_triggered()
{
	QPageSetupDialog d(m_printer, this);
	if (d.exec() == QDialog::Accepted) {
		m_preview->updatePreview();
	}
}

void ProjectPrintWindow::on_m_check_all_pb_clicked()
{
	for (auto cb : m_diagram_list_hash.values()) {
		cb->setChecked(true);
	}
	m_preview->updatePreview();
}

void ProjectPrintWindow::on_m_uncheck_all_clicked()
{
	for (auto cb : m_diagram_list_hash.values()) {
		cb->setChecked(false);
	}
	m_preview->updatePreview();
}

void ProjectPrintWindow::print()
{
	const bool isPdf = (m_printer->outputFormat() == QPrinter::PdfFormat);
	const QString pdfFile = isPdf ? m_printer->outputFileName() : QString();

	m_preview->print();   // triggers requestPaint() synchronously; painter
	                      // is created/destroyed inside that call

	savePageSetupForCurrentPrinter();

	if (isPdf && !pdfFile.isEmpty()) {
		// Defer post-processing and window close to the next event-loop
		// iteration.  This lets the macOS arm64 Metal compositor finish
		// compositing the backing store before the window is destroyed,
		// which prevents the black screen observed on Apple Silicon under
		// macOS Sequoia (QPrintPreviewWidget + CALayer timing issue).
		QTimer::singleShot(0, this, [this, pdfFile]() {
			// Convert URI link annotations into native internal GoTo/FitR
			// actions so cross-references jump inside the document.
			pdfConvertUriToGoTo(pdfFile);
			this->close();
		});
	} else {
		this->close();
	}
}

void ProjectPrintWindow::on_m_date_cb_userDateChanged(const QDate &date)
{
	auto index = ui->m_date_from_cb->currentIndex();
		// 0 = all date
		// 1 = from the date
		// 2 = at the date

	if (index) { on_m_uncheck_all_clicked();  }
	else       { on_m_check_all_pb_clicked(); }


	for (auto diagram : m_diagram_list_hash.keys())
	{
		auto diagram_date = diagram->border_and_titleblock.date();
		if ( (index == 1 && diagram_date >= date) ||
			 (index == 2 && diagram_date == date) )
			m_diagram_list_hash.value(diagram)->setChecked(true);
	}

	m_preview->updatePreview();
}

void ProjectPrintWindow::on_m_date_from_cb_currentIndexChanged(int index)
{
	Q_UNUSED(index)

	ui->m_date_cb->setEnabled(index);
	ui->m_apply_date_pb->setEnabled(index);
	on_m_date_cb_userDateChanged(ui->m_date_cb->date());

}

void ProjectPrintWindow::on_m_apply_date_pb_clicked() {
	on_m_date_cb_userDateChanged(ui->m_date_cb->date());
}
