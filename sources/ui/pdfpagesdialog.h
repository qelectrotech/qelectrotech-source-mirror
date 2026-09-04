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
#ifndef PDFPAGESDIALOG_H
#define PDFPAGESDIALOG_H

// Needs the QtPdf module (QET_HAS_QTPDF, set by CMake only when QtPdf
// is present AND Qt >= 6.4, for QPdfDocument::pagePointSize()).
#ifdef QET_HAS_QTPDF

#include <QDialog>

class QSpinBox;
class QComboBox;
class QLabel;
class QPdfDocument;

/**
	@brief The PdfPagesDialog class
	Dialog for selecting a page from a PDF document.
	Displays the total page count, a live preview of the selected page,
 DPI options (150/300/600), and allows the user to choose which page to import.
*/
class PdfPagesDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit PdfPagesDialog(QPdfDocument &document, QWidget *parent = nullptr);
		int selectedPage() const;
		int selectedDpi() const;

	private slots:
		void updatePreview();

	private:
		QPdfDocument &m_document;
		QLabel *m_info_label;
		QLabel *m_preview_label;
		QSpinBox *m_page_spinbox;
		QComboBox *m_dpi_combo;
};

#endif // QET_HAS_QTPDF

#endif // PDFPAGESDIALOG_H
