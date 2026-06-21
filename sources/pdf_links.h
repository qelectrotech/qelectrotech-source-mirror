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

#include <QMap>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QTransform>
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
		/// a diagram -> its source rectangle in scene pixels (for /FitR framing)
		std::function<QRectF(Diagram *)> sourceRectOf;
	};

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

}

#endif // PDF_LINKS_H
