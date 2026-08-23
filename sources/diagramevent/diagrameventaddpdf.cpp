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
#include "diagrameventaddpdf.h"

// Whole file is a no-op unless QtPdf is available (see diagrameventaddpdf.h).
#ifdef QET_HAS_QTPDF

#include "../qetapp.h"
#include "../diagram.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../qetgraphicsitem/diagramimageitem.h"
#include "../ui/pdfpagesdialog.h"

#include <QPdfDocument>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>

/**
	@brief DiagramEventAddPdf::DiagramEventAddPdf
	Constructor
	@param diagram the diagram where this event operates
*/
DiagramEventAddPdf::DiagramEventAddPdf(Diagram *diagram) :
	DiagramEventInterface(diagram),
	m_image(nullptr),
	m_is_added(false)
{
	openDialog();
}

/**
	@brief DiagramEventAddPdf::~DiagramEventAddPdf
	Destructor
*/
DiagramEventAddPdf::~DiagramEventAddPdf()
{
	if (m_running || m_abort)
	{
		if (m_is_added) m_diagram->removeItem(m_image);
		delete m_image;
	}

	foreach (QGraphicsView *view, m_diagram->views())
		view->setContextMenuPolicy((Qt::DefaultContextMenu));
}

/**
	@brief DiagramEventAddPdf::mousePressEvent
	Action when mouse is pressed
	@param event event of mouse pressed
*/
void DiagramEventAddPdf::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (m_image && event->button() == Qt::LeftButton)
	{
		QPointF pos = event->scenePos();
		pos.rx() -= m_image->boundingRect().width()/2;
		pos.ry() -= m_image->boundingRect().height()/2;
		m_diagram->undoStack().push(new AddGraphicsObjectCommand(m_image, m_diagram, pos));

		for (QGraphicsView *view : m_diagram->views()) {
			view->setContextMenuPolicy((Qt::DefaultContextMenu));
		}

		m_running = false;
		emit finish();
		event->setAccepted(true);
	}
	else if (m_image && event->button() == Qt::RightButton)
	{
		m_image->setRotation(m_image->rotation() + 90);
		event->setAccepted(true);
	}
}

/**
	@brief DiagramEventAddPdf::mouseMoveEvent
	Action when mouse moves
	@param event event of mouse move
*/
void DiagramEventAddPdf::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	if (!m_image || event->buttons() != Qt::NoButton) {
		return;
	}

	QPointF pos = event->scenePos();

	if (!m_is_added)
	{
		for (QGraphicsView *view : m_diagram->views()) {
			view->setContextMenuPolicy((Qt::NoContextMenu));
		}

		m_diagram->addItem(m_image);
		m_is_added = true;
	}

	m_image->setPos(pos - m_image->boundingRect().center());
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddPdf::mouseDoubleClickEvent
	Overwrite double click to prevent opening properties dialog.
	@param event event of mouse double click
*/
void DiagramEventAddPdf::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
	event->setAccepted(true);
}

/**
	@brief DiagramEventAddPdf::wheelEvent
	Action when mouse wheel is rotated (CTRL+wheel to scale)
	@param event event of mouse wheel
*/
void DiagramEventAddPdf::wheelEvent(QGraphicsSceneWheelEvent *event)
{
	if (!m_is_added || !m_image || event->modifiers() != Qt::CTRL) {
		return;
	}

	qreal scaling = m_image->scale();
	event->delta() > 1 ? scaling += 0.01 : scaling -= 0.01;
	if (scaling > 0.01 && scaling <= 2) {
		m_image->setScale(scaling);
	}

	event->setAccepted(true);
}

/**
	@brief DiagramEventAddPdf::isNull
	@return true if the PDF image couldn't be loaded, false otherwise
*/
bool DiagramEventAddPdf::isNull() const
{
	if (!m_image) return true;
	return false;
}

/**
	@brief DiagramEventAddPdf::openDialog
	Opens a file dialog to select a PDF file, then opens a page selection
	dialog with DPI options. The selected page is rendered to a QImage at
	the chosen DPI and converted to a DiagramImageItem.
*/
void DiagramEventAddPdf::openDialog()
{
	if (m_diagram->isReadOnly()) return;

	// Open file dialog to select a PDF file
	QString pathPDFs = QETApp::documentDir();
	QString fileName = QFileDialog::getOpenFileName(
		m_diagram->views().isEmpty() ? nullptr : m_diagram->views().first(),
		QObject::tr("Sélectionner un fichier PDF..."),
		pathPDFs,
		QObject::tr("Fichiers PDF (*.pdf)")
	);

	if (fileName.isEmpty()) return;

	// Load the PDF document
	QPdfDocument document;
	document.load(fileName);

	if (document.status() != QPdfDocument::Status::Ready)
	{
		QMessageBox::critical(
			m_diagram->views().isEmpty() ? nullptr : m_diagram->views().first(),
			QObject::tr("Erreur"),
			QObject::tr("Impossible de charger le fichier PDF.")
		);
		return;
	}

	int pageCount = document.pageCount();
	if (pageCount <= 0)
	{
		QMessageBox::critical(
			m_diagram->views().isEmpty() ? nullptr : m_diagram->views().first(),
			QObject::tr("Erreur"),
			QObject::tr("Le fichier PDF ne contient aucune page.")
		);
		return;
	}

	// Always show the dialog so the user can choose page and DPI
	PdfPagesDialog dialog(document,
		m_diagram->views().isEmpty() ? nullptr : m_diagram->views().first());
	if (dialog.exec() != QDialog::Accepted) return;
	int pageIndex = dialog.selectedPage() - 1; // Convert to 0-based index
	int dpi = dialog.selectedDpi();

	// Calculate pixel size from PDF points at the user-selected DPI
	// PDF point = 1/72 inch
	QSizeF pageSize = document.pagePointSize(pageIndex);
	int pixelWidth  = qRound((pageSize.width()  / 72.0) * dpi);
	int pixelHeight = qRound((pageSize.height() / 72.0) * dpi);

	if (pixelWidth <= 0 || pixelHeight <= 0)
	{
		QMessageBox::critical(
			m_diagram->views().isEmpty() ? nullptr : m_diagram->views().first(),
			QObject::tr("Erreur"),
			QObject::tr("Impossible de déterminer la taille de la page PDF.")
		);
		return;
	}

	// Render PDF page to QImage
	QImage image = document.render(pageIndex, QSize(pixelWidth, pixelHeight));
	if (image.isNull())
	{
		QMessageBox::critical(
			m_diagram->views().isEmpty() ? nullptr : m_diagram->views().first(),
			QObject::tr("Erreur"),
			QObject::tr("Impossible de rendre la page PDF.")
		);
		return;
	}

	// Fill white background to handle transparent PDFs
	QImage background(image.size(), QImage::Format_ARGB32_Premultiplied);
	background.fill(Qt::white);
	QPainter painter(&background);
	painter.drawImage(0, 0, image);
	painter.end();

	m_image = new DiagramImageItem(QPixmap::fromImage(background));
	m_running = true;
}

#endif // QET_HAS_QTPDF
