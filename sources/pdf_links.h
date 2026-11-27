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
#ifndef PDF_LINKS_H
#define PDF_LINKS_H

#include <QDateTime>
#include <QMap>
#include <QPageLayout>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QTransform>
#include <QUuid>
#include <functional>

class QPdfEngine;
class Diagram;

/**
	Shared helper that turns a project's cross-references and folio reports
	into clickable internal hyperlinks in a Qt-generated PDF.  Used by both the
	GUI print path (ProjectPrintWindow) and the headless CLI export, each of
	which builds its own page geometry and passes it in — this code never
	computes the scene-to-page mapping itself.
*/
namespace PdfLinks {

	/**
		Geometry mapping for one rendered PDF page.  Each caller builds this
		from its OWN page setup (printer page layout vs QPdfWriter), since the
		device-pixel and point conversions differ between them.
	*/
	struct PageGeometry {
		/// scene coordinates -> device pixels (the same "fit" render() applied)
		QTransform sceneToDevice;
		/// device paint rectangle, in pixels (the page area)
		QRectF target;
		/// links whose rectangle falls outside this are dropped
		QRectF pageBounds;
		/// device pixels -> PDF points (replicates the engine's page matrix)
		std::function<QPointF(const QPointF &)> devToPdf;
		/// optional: a diagram (a link's target) -> device pixels to PDF
		/// points on the page that diagram is on, when its page is not
		/// the size of this one. Not set = every diagram is on a page
		/// like this one, and \a devToPdf serves.
		std::function<std::function<QPointF(const QPointF &)>(Diagram *)> devToPdfOf;
		/// a diagram -> its source rectangle in scene pixels (for /FitR framing)
		std::function<QRectF(Diagram *)> sourceRectOf;
		/// optional: where a diagram (a link's target) is on its page, as
		/// scene -> device pixels; false = it cannot be framed. Not set =
		/// every diagram is fitted to \a target.
		std::function<bool(Diagram *, QTransform *)> sceneToDeviceOf;
	};

	/**
		Device pixels -> PDF points on a page of @p layout written at
		@p resolution dpi, as the PDF engine's page matrix does: the scale
		of 72 / resolution, the flip of Y on the page's height and, unless
		the layout is in full page mode, the margins. For the page of a
		link's target, when it is not the size of the page of the link
		(PageGeometry::devToPdfOf).
	*/
	std::function<QPointF(const QPointF &)> devToPdfOnPage(
			const QPageLayout &layout, int resolution);

	/**
		The transform of a folio of @p source scene pixels onto @p target
		device pixels at one scale for both directions, anchored at the top
		left of @p target: what QetPrint::render() does with
		Qt::KeepAspectRatio.
	*/
	QTransform fitToPage(const QRectF &source, const QRectF &target);

	/**
		Inject clickable cross-reference / folio-report hyperlinks for @p diagram
		into the current page of @p engine.  Each link is emitted as a URI
		annotation encoding the target page and a /FitR rectangle;
		convertUriToGoTo() then rewrites those into native internal GoTo actions.
	*/
	void injectCrossRefLinks(QPdfEngine *engine, Diagram *diagram,
							 const PageGeometry &geom,
							 const QMap<Diagram *, int> &pageMap,
							 const QString &outputFileName);

	/**
		Post-process a Qt-generated PDF file: rewrite every "/S /URI" link
		annotation into a native internal "/S /GoTo" action (page + /FitR or
		/Fit destination) and rebuild the xref table.  No-op if the file has no
		such annotations.
	*/
	void convertUriToGoTo(const QString &pdfPath);

	/**
		Post-process a Qt-generated PDF file: blank out the PDF/X namespace
		declaration Qt 6 writes into the XMP metadata of every PDF.  Adobe
		Acrobat draws small text too bold when it is present (bugtracker #340).
		Replaced in place with spaces, so no offset changes.  No-op for a real
		PDF/X file or when the declaration is absent.
	*/
	void removeUnusedPdfxNamespace(const QString &pdfPath);

	/**
		Post-process a Qt-generated PDF file: replace the creation and
		modification dates Qt wrote (the time of the export, in local time)
		with @p when, in UTC, in the document information and the XMP
		metadata, and shift the xref table to match.  Used for a repeatable
		export (SOURCE_DATE_EPOCH).  Leaves the file alone if it is not laid
		out as Qt writes it.
	*/
	void setDocumentDate(const QString &pdfPath, const QDateTime &when);

	/// The document id to give QPdfWriter before a repeatable export, for
	/// setDocumentIdFromContent() to find and replace afterwards.
	QUuid placeholderDocumentId();

	/**
		Post-process a PDF written with placeholderDocumentId(): replace that
		id, in the XMP metadata and in the trailer's /ID, with one derived
		from the bytes of the file.  The same pages give the same id, however
		the project was made.  Same length, so no offset changes.  No-op when
		the placeholder is absent.
	*/
	void setDocumentIdFromContent(const QString &pdfPath);

	struct ComponentInfo {
		QString contents;
	};

	/**
		Post-process a Qt-generated PDF file: convert component-info placeholder
		link annotations (http://componentinfo.local/<N>) into invisible text
		annotations with the actual component info as /Contents.
	*/
	void convertComponentInfoAnnotations(const QString &pdfPath,
										const QList<ComponentInfo> &annotations);

}

#endif // PDF_LINKS_H
