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
#include "pdfpagesetupdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

namespace {
		/// What the margins must leave free on the page, in mm
	constexpr qreal minimum_printable_mm = 10.0;

	QDoubleSpinBox *newMillimetreBox(const QString &name, QWidget *parent)
	{
		auto sb = new QDoubleSpinBox(parent);
		sb->setObjectName(name);
		sb->setDecimals(1);
		sb->setSingleStep(1.0);
		sb->setSuffix(QStringLiteral(" mm"));
		return sb;
	}
}

/**
	@brief PdfPageSetupDialog::PdfPageSetupDialog
	@param layout : the page layout of the PDF printer
	@param parent : parent widget
*/
PdfPageSetupDialog::PdfPageSetupDialog(const QPageLayout &layout,
				       QWidget *parent) :
	QDialog(parent),
	m_layout(layout)
{
	setWindowTitle(tr("PDF page layout"));

		//Paper
	auto paper_gb = new QGroupBox(tr("Paper"), this);
	m_size_cb = new QComboBox(paper_gb);
	m_size_cb->setObjectName(QStringLiteral("size"));
	const QList<QPageSize::PageSizeId> sizes {
		QPageSize::A0, QPageSize::A1, QPageSize::A2, QPageSize::A3,
		QPageSize::A4, QPageSize::A5, QPageSize::A6,
		QPageSize::B3, QPageSize::B4, QPageSize::B5,
		QPageSize::Letter, QPageSize::Legal, QPageSize::Tabloid,
		QPageSize::Ledger};
	for (auto id : sizes)
		m_size_cb->addItem(QPageSize::name(id), int(id));

	const QPageSize page_size = m_layout.pageSize();
	const auto page_id = page_size.isValid() ? page_size.id()
						 : QPageSize::A4;
	if (page_id != QPageSize::Custom && !sizes.contains(page_id))
		m_size_cb->addItem(QPageSize::name(page_id), int(page_id));
	m_size_cb->addItem(tr("Custom"), int(QPageSize::Custom));
	m_size_cb->setCurrentIndex(m_size_cb->findData(int(page_id)));

	m_width_sb = newMillimetreBox(QStringLiteral("width"), paper_gb);
	m_height_sb = newMillimetreBox(QStringLiteral("height"), paper_gb);
	for (auto sb : {m_width_sb, m_height_sb})
		sb->setRange(2 * minimum_printable_mm, 10000.0);
	const QSizeF size_mm = page_size.isValid()
			? page_size.size(QPageSize::Millimeter)
			: QPageSize(QPageSize::A4).size(QPageSize::Millimeter);
	m_width_sb->setValue(size_mm.width());
	m_height_sb->setValue(size_mm.height());

	auto paper_layout = new QFormLayout(paper_gb);
	paper_layout->addRow(tr("Size:"), m_size_cb);
	paper_layout->addRow(tr("Width:"), m_width_sb);
	paper_layout->addRow(tr("Height:"), m_height_sb);

		//Orientation
	auto orientation_gb = new QGroupBox(tr("Orientation"), this);
	m_portrait_rb = new QRadioButton(tr("Portrait"), orientation_gb);
	m_portrait_rb->setObjectName(QStringLiteral("portrait"));
	m_landscape_rb = new QRadioButton(tr("Landscape"), orientation_gb);
	m_landscape_rb->setObjectName(QStringLiteral("landscape"));
	(m_layout.orientation() == QPageLayout::Portrait ? m_portrait_rb
							   : m_landscape_rb)
			->setChecked(true);
	auto orientation_layout = new QHBoxLayout(orientation_gb);
	orientation_layout->addWidget(m_portrait_rb);
	orientation_layout->addWidget(m_landscape_rb);
	orientation_layout->addStretch();

		//Margins
	m_margins_gb = new QGroupBox(tr("Margins"), this);
	m_margins_gb->setObjectName(QStringLiteral("margins"));
	m_left_sb = newMillimetreBox(QStringLiteral("left"), m_margins_gb);
	m_top_sb = newMillimetreBox(QStringLiteral("top"), m_margins_gb);
	m_right_sb = newMillimetreBox(QStringLiteral("right"), m_margins_gb);
	m_bottom_sb = newMillimetreBox(QStringLiteral("bottom"), m_margins_gb);

	auto margins_layout = new QGridLayout(m_margins_gb);
	margins_layout->addWidget(new QLabel(tr("Left:"), m_margins_gb), 0, 0);
	margins_layout->addWidget(m_left_sb, 0, 1);
	margins_layout->addWidget(new QLabel(tr("Right:"), m_margins_gb), 0, 2);
	margins_layout->addWidget(m_right_sb, 0, 3);
	margins_layout->addWidget(new QLabel(tr("Top:"), m_margins_gb), 1, 0);
	margins_layout->addWidget(m_top_sb, 1, 1);
	margins_layout->addWidget(new QLabel(tr("Bottom:"), m_margins_gb), 1, 2);
	margins_layout->addWidget(m_bottom_sb, 1, 3);

		//The page must be known before the margins can be limited to it
	updateCustomSize();
	updateMarginLimits();
	const QMarginsF margins = m_layout.margins(QPageLayout::Millimeter);
	m_left_sb->setValue(margins.left());
	m_top_sb->setValue(margins.top());
	m_right_sb->setValue(margins.right());
	m_bottom_sb->setValue(margins.bottom());
	updateMarginLimits();

	if (m_layout.mode() == QPageLayout::FullPageMode)
	{
		m_margins_gb->setEnabled(false);
		m_margins_gb->setToolTip(
			tr("Margins are not used while \"Use the whole page\" is ticked."));
	}

	auto button_box = new QDialogButtonBox(
		QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
	connect(button_box, &QDialogButtonBox::accepted,
		this, &QDialog::accept);
	connect(button_box, &QDialogButtonBox::rejected,
		this, &QDialog::reject);

	auto main_layout = new QVBoxLayout(this);
	main_layout->addWidget(paper_gb);
	main_layout->addWidget(orientation_gb);
	main_layout->addWidget(m_margins_gb);
	main_layout->addWidget(button_box);

	connect(m_size_cb, &QComboBox::currentIndexChanged, this, [this]() {
		updateCustomSize();
		updateMarginLimits();
	});
	for (auto sb : {m_width_sb, m_height_sb, m_left_sb, m_top_sb,
			m_right_sb, m_bottom_sb})
		connect(sb, &QDoubleSpinBox::valueChanged,
			this, &PdfPageSetupDialog::updateMarginLimits);
	connect(m_landscape_rb, &QRadioButton::toggled,
		this, &PdfPageSetupDialog::updateMarginLimits);
}

/**
	@brief PdfPageSetupDialog::pageLayout
	@return the layout given to the constructor, with the paper size,
	orientation and margins chosen in this dialog. Margins are in mm.
*/
QPageLayout PdfPageSetupDialog::pageLayout() const
{
	QPageLayout layout = m_layout;
	layout.setUnits(QPageLayout::Millimeter);
	layout.setPageSize(chosenPageSize());
	layout.setOrientation(m_landscape_rb->isChecked()
				      ? QPageLayout::Landscape
				      : QPageLayout::Portrait);
	layout.setMargins(QMarginsF(m_left_sb->value(), m_top_sb->value(),
				    m_right_sb->value(), m_bottom_sb->value()));
	return layout;
}

QPageSize PdfPageSetupDialog::chosenPageSize() const
{
	const auto id = QPageSize::PageSizeId(m_size_cb->currentData().toInt());
	if (id != QPageSize::Custom)
		return QPageSize(id);

	return QPageSize(QSizeF(m_width_sb->value(), m_height_sb->value()),
			 QPageSize::Millimeter, QString(),
			 QPageSize::ExactMatch);
}

/**
	@brief PdfPageSetupDialog::orientedSizeMM
	@return the size of the page as it lies, in mm
*/
QSizeF PdfPageSetupDialog::orientedSizeMM() const
{
	const QPageLayout layout(chosenPageSize(),
				 m_landscape_rb->isChecked()
					 ? QPageLayout::Landscape
					 : QPageLayout::Portrait,
				 QMarginsF(), QPageLayout::Millimeter);
	return layout.fullRect(QPageLayout::Millimeter).size();
}

/**
	@brief PdfPageSetupDialog::updateCustomSize
	Width and height can be typed for a custom size; for a standard one
	they show its size.
*/
void PdfPageSetupDialog::updateCustomSize()
{
	const bool custom =
		m_size_cb->currentData().toInt() == int(QPageSize::Custom);
	m_width_sb->setEnabled(custom);
	m_height_sb->setEnabled(custom);
	if (custom)
		return;

	const QSizeF size = chosenPageSize().size(QPageSize::Millimeter);
	const QSignalBlocker width_blocker(m_width_sb);
	const QSignalBlocker height_blocker(m_height_sb);
	m_width_sb->setValue(size.width());
	m_height_sb->setValue(size.height());
}

/**
	@brief PdfPageSetupDialog::updateMarginLimits
	Two opposite margins together leave at least
	minimum_printable_mm of the page free.
*/
void PdfPageSetupDialog::updateMarginLimits()
{
	const QSizeF page = orientedSizeMM();
	const auto limit = [](QDoubleSpinBox *sb, qreal max) {
		const QSignalBlocker blocker(sb);
		sb->setRange(0.0, qMax(0.0, max));
	};
	limit(m_left_sb, page.width() - m_right_sb->value()
				 - minimum_printable_mm);
	limit(m_right_sb, page.width() - m_left_sb->value()
				  - minimum_printable_mm);
	limit(m_top_sb, page.height() - m_bottom_sb->value()
				- minimum_printable_mm);
	limit(m_bottom_sb, page.height() - m_top_sb->value()
				   - minimum_printable_mm);
}
