/*
	Copyright 2006-2025 The QElectroTech Team
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
#include "pdf_links.h"

#include "diagram.h"
#include "qetgraphicsitem/crossrefitem.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/elementtextitemgroup.h"

// Private Qt PDF engine for drawHyperlink() — not public API.
// Qt6: QPdfEngine::drawHyperlink() was kept in Qt6's private API, but verify
// that <private/qpdf_p.h> resolves correctly against your Qt6 installation.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#  warning "Qt6 build: verify that QPdfEngine::drawHyperlink() is still available in <private/qpdf_p.h>"
#endif
#include <private/qpdf_p.h>

#include <QByteArray>
#include <QFile>
#include <QGraphicsTextItem>
#include <QList>
#include <QRegularExpression>
#include <QUrl>
#include <QVector>

namespace PdfLinks {

void injectCrossRefLinks(QPdfEngine *engine, Diagram *diagram,
						 const PageGeometry &geom,
						 const QMap<Diagram *, int> &pageMap,
						 const QString &outputFileName)
{
	if (!engine || !diagram)
		return;

	const QTransform &fit       = geom.sceneToDevice;
	const QRectF      &target   = geom.target;
	const QRectF      &pageBounds = geom.pageBounds;

	// Compute, in PDF points on its OWN page, the rectangle to frame for a
	// target element (used as a /FitR destination so the link zooms onto it).
	auto destRectPdf = [&](Element *tgt) -> QRectF {
		Diagram *dg = tgt ? tgt->diagram() : nullptr;
		if (!dg) return QRectF();
		const QRectF srcT = geom.sourceRectOf(dg);
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
		const QPointF a = geom.devToPdf(devT.topLeft());
		const QPointF b = geom.devToPdf(devT.bottomRight());
		return QRectF(QPointF(qMin(a.x(), b.x()), qMin(a.y(), b.y())),
					  QPointF(qMax(a.x(), b.x()), qMax(a.y(), b.y())));
	};

	auto injectLink = [&](const QRectF &sceneRect, Element *targetElmt) {
		if (!targetElmt || !targetElmt->diagram()) return;
		const int targetPage = pageMap.value(targetElmt->diagram(), -1);
		if (targetPage < 1) return;
		const QRectF devRect = fit.mapRect(sceneRect);
		if (!devRect.isValid() || !pageBounds.intersects(devRect)) return;

		QString frag = QString("page=%1").arg(targetPage);
		const QRectF d = destRectPdf(targetElmt);   // /FitR L_B_R_T
		if (d.isValid())
			frag += QString("&fitr=%1_%2_%3_%4")
				.arg(qRound(d.left())).arg(qRound(d.top()))
				.arg(qRound(d.right())).arg(qRound(d.bottom()));

		QUrl url = QUrl::fromLocalFile(outputFileName);
		url.setFragment(frag);
		engine->drawHyperlink(devRect, url);
	};

	for (auto *item : diagram->items()) {

		// --- CrossRefItem links ---
		if (auto *xref = dynamic_cast<CrossRefItem*>(item)) {
			for (auto it = xref->hoveredContactsMap().begin();
				 it != xref->hoveredContactsMap().end(); ++it)
			{
				Element *targetElmt = it.key();
				if (!targetElmt || !targetElmt->diagram()) continue;
				// it.value() is in the CrossRefItem's LOCAL coords -> scene
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

void convertUriToGoTo(const QString &pdfPath)
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

} // namespace PdfLinks
