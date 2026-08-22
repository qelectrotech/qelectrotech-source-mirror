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

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QPdfDocument>
#include <QScrollArea>
#include <QPainter>

// Preview rendering DPI for the thumbnail in the dialog.
// Lower than the final render DPI to keep the dialog fast and compact.
static const int PREVIEW_DPI = 96;

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
	setMinimumHeight(400);

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

	// Update preview when spinbox value changes
	connect(m_page_spinbox, QOverload<int>::of(&QSpinBox::valueChanged),
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
	@brief PdfPagesDialog::updatePreview
	Renders the currently selected page as a thumbnail and displays it
	in the preview label. Uses a fixed preview width of 400 pixels
	while preserving the page aspect ratio.
*/
void PdfPagesDialog::updatePreview()
{
	int pageIndex = m_page_spinbox->value() - 1; // Convert to 0-based

	// Calculate preview size: fixed width, proportional height
	QSizeF pageSize = m_document.pagePointSize(pageIndex);
	if (pageSize.isEmpty()) return;

	const int previewWidth = 400;
	qreal ratio = pageSize.height() / pageSize.width();
	int previewHeight = qRound(previewWidth * ratio);

	// Render the page to QImage
	QImage preview = m_document.render(pageIndex, QSize(previewWidth, previewHeight));
	if (preview.isNull()) return;

	// Fill white background to handle transparent PDFs
	QImage background(preview.size(), QImage::Format_ARGB32_Premultiplied);
	background.fill(Qt::white);
	QPainter painter(&background);
	painter.drawImage(0, 0, preview);
	painter.end();

	m_preview_label->setPixmap(QPixmap::fromImage(background).scaled(
		previewWidth, previewHeight,
		Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
