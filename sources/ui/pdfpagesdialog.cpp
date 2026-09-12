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
#include "pdfpagesdialog.h"

// Whole file is a no-op unless QtPdf is available (see pdfpagesdialog.h).
#ifdef QET_HAS_QTPDF

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QPdfDocument>
#include <QScrollArea>
#include <QPainter>

// Preview rendering DPI for the thumbnail in the dialog.
// Fixed at 96 DPI to keep the preview fast and compact regardless
// of the final render DPI selected by the user.
static const int PREVIEW_DPI = 96;
static const int PREVIEW_WIDTH = 400;

/**
	@brief PdfPagesDialog::PdfPagesDialog
	Constructor
	@param document the loaded PDF document to preview pages from
	@param parent parent widget
*/
PdfPagesDialog::PdfPagesDialog(QPdfDocument &document, QWidget *parent)
	: QDialog(parent),
	  m_document(document)
{
	setWindowTitle(tr("Sélectionner une page"));
	setMinimumWidth(500);
	setMinimumHeight(500);

	int pageCount = document.pageCount();

	// Main layout
	QVBoxLayout *main_layout = new QVBoxLayout(this);

	// Info label: "Ce document PDF contient X page(s)"
	m_info_label = new QLabel(
		tr("Ce document PDF contient %n page(s)", "", pageCount),
		this
	);
	m_info_label->setWordWrap(true);
	main_layout->addWidget(m_info_label);

	// Page selection layout
	QHBoxLayout *page_layout = new QHBoxLayout();

	QLabel *select_label = new QLabel(tr("Page à importer :"), this);
	page_layout->addWidget(select_label);

	// Spinbox: min=1, max=pageCount
	m_page_spinbox = new QSpinBox(this);
	m_page_spinbox->setMinimum(1);
	m_page_spinbox->setMaximum(pageCount);
	m_page_spinbox->setValue(1);
	page_layout->addWidget(m_page_spinbox);

	page_layout->addStretch();

	// DPI selection
	QLabel *dpi_label = new QLabel(tr("Résolution :"), this);
	page_layout->addWidget(dpi_label);

	m_dpi_combo = new QComboBox(this);
	m_dpi_combo->addItem(tr("150 DPI (écran)"), 150);
	m_dpi_combo->addItem(tr("300 DPI (impression)"), 300);
	m_dpi_combo->addItem(tr("600 DPI (haute qualité)"), 600);
	m_dpi_combo->setCurrentIndex(0);
	page_layout->addWidget(m_dpi_combo);

	main_layout->addLayout(page_layout);

	// Preview area
	QLabel *preview_title = new QLabel(tr("Aperçu :"), this);
	main_layout->addWidget(preview_title);

	QScrollArea *scroll_area = new QScrollArea(this);
	scroll_area->setWidgetResizable(true);
	scroll_area->setAlignment(Qt::AlignCenter);
	scroll_area->setMinimumHeight(250);

	m_preview_label = new QLabel(this);
	m_preview_label->setAlignment(Qt::AlignCenter);
	m_preview_label->setMinimumSize(200, 200);
	m_preview_label->setStyleSheet("QLabel { background-color: white; border: 1px solid #cccccc; }");
	scroll_area->setWidget(m_preview_label);
	main_layout->addWidget(scroll_area, 1);

	// OK/Cancel buttons
	QDialogButtonBox *button_box = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
		this
	);
	connect(button_box, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected, this, &QDialog::reject);
	main_layout->addWidget(button_box);

	// Update preview when spinbox or DPI changes
	connect(m_page_spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
		this, &PdfPagesDialog::updatePreview);
	connect(m_dpi_combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
		this, &PdfPagesDialog::updatePreview);

	// Render initial preview
	updatePreview();
}

/**
	@brief PdfPagesDialog::selectedPage
	@return the selected page number (1-based)
*/
int PdfPagesDialog::selectedPage() const
{
	return m_page_spinbox->value();
}

/**
	@brief PdfPagesDialog::selectedDpi
	@return the selected DPI value (150, 300, or 600)
*/
int PdfPagesDialog::selectedDpi() const
{
	return m_dpi_combo->currentData().toInt();
}

/**
	@brief PdfPagesDialog::updatePreview
	Renders the currently selected page as a thumbnail and displays it
	in the preview label. The preview size scales with the selected DPI
	to give a visual hint of the output quality.
*/
void PdfPagesDialog::updatePreview()
{
	int pageIndex = m_page_spinbox->value() - 1; // Convert to 0-based

	// Calculate preview size: fixed width, proportional height
	QSizeF pageSize = m_document.pagePointSize(pageIndex);
	if (pageSize.isEmpty()) return;

	qreal ratio = pageSize.height() / pageSize.width();
	int previewHeight = qRound(PREVIEW_WIDTH * ratio);

	// Render the page to QImage at preview DPI
	QImage preview = m_document.render(pageIndex, QSize(PREVIEW_WIDTH, previewHeight));
	if (preview.isNull()) return;

	// Fill white background to handle transparent PDFs
	QImage background(preview.size(), QImage::Format_ARGB32_Premultiplied);
	background.fill(Qt::white);
	QPainter painter(&background);
	painter.drawImage(0, 0, preview);
	painter.end();

	m_preview_label->setPixmap(QPixmap::fromImage(background).scaled(
		PREVIEW_WIDTH, previewHeight,
		Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

#endif // QET_HAS_QTPDF
