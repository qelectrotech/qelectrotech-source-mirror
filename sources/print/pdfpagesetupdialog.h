// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef PDFPAGESETUPDIALOG_H
#define PDFPAGESETUPDIALOG_H

#include <QDialog>
#include <QPageLayout>

class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QRadioButton;

/**
	@brief The PdfPageSetupDialog class
	Paper size, orientation and margins of a PDF export.

	On Windows and macOS, QPageSetupDialog is the system's own dialog,
	which only works with a physical printer: for a PDF file it returns
	at once without showing anything. This dialog does the same job for
	the PDF export on those systems. It edits a copy of the layout it is
	given, so everything it does not show (the full page mode, for
	example) is kept as it was.
*/
class PdfPageSetupDialog : public QDialog
{
	Q_OBJECT

	public:
		explicit PdfPageSetupDialog(const QPageLayout &layout,
					    QWidget *parent = nullptr);

		QPageLayout pageLayout() const;

	private:
		QPageSize chosenPageSize() const;
		QSizeF orientedSizeMM() const;
		void updateCustomSize();
		void updateMarginLimits();

		QPageLayout m_layout;
		QComboBox *m_size_cb = nullptr;
		QDoubleSpinBox *m_width_sb = nullptr;
		QDoubleSpinBox *m_height_sb = nullptr;
		QRadioButton *m_portrait_rb = nullptr;
		QRadioButton *m_landscape_rb = nullptr;
		QGroupBox *m_margins_gb = nullptr;
		QDoubleSpinBox *m_left_sb = nullptr;
		QDoubleSpinBox *m_top_sb = nullptr;
		QDoubleSpinBox *m_right_sb = nullptr;
		QDoubleSpinBox *m_bottom_sb = nullptr;
};

#endif // PDFPAGESETUPDIALOG_H
