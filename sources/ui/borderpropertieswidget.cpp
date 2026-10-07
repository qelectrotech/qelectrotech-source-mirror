/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "borderpropertieswidget.h"

#include "../diagram.h"
#include "../qetapp.h"
#include "../titleblocktemplate.h"
#include "ui_borderpropertieswidget.h"

#include <iterator>
#include <tuple>

namespace {
	/// ISO 216 A sizes in mm, landscape
	struct PaperSize { const char *name; int width_mm; int height_mm; };
	const PaperSize paper_sizes[] = {
		{"A0", 1189, 841},
		{"A1",  841, 594},
		{"A2",  594, 420},
		{"A3",  420, 297},
		{"A4",  297, 210},
		{"A5",  210, 148}
	};

	/// The PDF export draws one scene unit as one pixel at 96 dpi
	int mmToPx(int mm) { return qRound(mm * 96.0 / 25.4); }

	/// The PDF export rounds a page within 3 pt of a paper size to that
	/// size, and 4 px left over already rounds to 3.0 pt and misses
	const int exact_fit = 3;

	/// Smallest column width or row height a paper size may give
	const int min_cell = 10;

	/// Fewest columns or rows a folio may have: BorderTitleBlock raises
	/// any lower count to 3 (MIN_COLUMN_COUNT, MIN_ROW_COUNT)
	const int min_count = 3;

	/**
		@brief fitCells
		Split @p available pixels into cells close to @p current pixels.
		First choice: cells within 15 %, then 30 %, of the current size
		that leave at most exact_fit pixels, the closest to the current
		size. Failing that, cells within 15 % that leave the fewest pixels.
		@return the number of cells and their size
	*/
	QPair<int, int> fitCells(int available, int current, int min_size, int max_count)
	{
		current = qMax(current, min_size);
		auto size_for = [&](int count, double spread) {
			const int size = available / count;
			return (size < min_size || qAbs(size - current) > current * spread)
					? 0 : size;
		};

		for (double spread : {0.15, 0.30})
		{
			QPair<int, int> best(0, 0);
			for (int count = min_count ; count <= max_count ; ++count)
			{
				const int size = size_for(count, spread);
				if (size && available - count * size <= exact_fit &&
					(!best.first || qAbs(size - current) < qAbs(best.second - current)))
					best = qMakePair(count, size);
			}
			if (best.first) return best;
		}

		QPair<int, int> best(0, 0);
		std::tuple<int, int> best_score(available, available);
		for (int count = min_count ; count <= max_count ; ++count)
		{
			const int size = size_for(count, 0.15);
			const std::tuple<int, int> score(available - count * size,
											 qAbs(size - current));
			if (size && score < best_score) {
				best_score = score;
				best = qMakePair(count, size);
			}
		}
		if (best.first) return best;

			//No size near the current one when the cell count is capped
		const int count = qBound(min_count, qRound(double(available) / current), max_count);
		return qMakePair(count, available / count);
	}
}

/**
	@brief BorderPropertiesWidget::BorderPropertiesWidget
	default constructor
	@param bp properties
	@param parent paretn widget
*/
BorderPropertiesWidget::BorderPropertiesWidget(const BorderProperties &bp, QWidget *parent) :
	QWidget(parent),
	ui(new Ui::BorderPropertiesWidget)
{
	ui->setupUi(this);

	ui->m_paper_cb->addItem(tr("Personnalisé"));
	for (const PaperSize &paper : paper_sizes)
		ui->m_paper_cb->addItem(QString::fromLatin1(paper.name));
	ui->m_orientation_cb->addItem(tr("Paysage"));
	ui->m_orientation_cb->addItem(tr("Portrait"));

	if (TitleBlockTemplate *tbt = QETApp::defaultTitleBlockTemplate())
		m_titleblock_height = tbt->height();

		//activated() is only emitted for a choice the user makes
	auto paper_picked = [this]() {
		m_paper_picked = ui->m_paper_cb->currentIndex() > 0;
		applyPaperSize();
	};
	connect(ui->m_paper_cb, qOverload<int>(&QComboBox::activated),
			this, paper_picked);
	connect(ui->m_orientation_cb, qOverload<int>(&QComboBox::activated),
			this, paper_picked);
	for (QSpinBox *sb : {ui->m_colums_count_sp, ui->m_columns_width_sp,
						 ui->m_rows_count_sp, ui->m_rows_height_sp})
		connect(sb, qOverload<int>(&QSpinBox::valueChanged),
				this, &BorderPropertiesWidget::showMatchingPaperSize);

	setProperties(bp);
}

/**
	@brief BorderPropertiesWidget::~BorderPropertiesWidget
	default destructor
*/
BorderPropertiesWidget::~BorderPropertiesWidget()
{
	delete ui;
}

/**
	@brief BorderPropertiesWidget::setProperties
	Set the current properties to edit
	@param bp properties to edit
*/
void BorderPropertiesWidget::setProperties(const BorderProperties &bp)
{
	m_properties = bp;
	ui -> m_colums_count_sp    ->setValue   (m_properties.columns_count);
	ui -> m_columns_width_sp   ->setValue   (m_properties.columns_width);
	ui -> m_display_columns_cb ->setChecked (m_properties.display_columns);
	ui -> m_rows_count_sp      ->setValue   (m_properties.rows_count);
	ui -> m_rows_height_sp     ->setValue   (m_properties.rows_height);
	ui -> m_display_rows_cb    ->setChecked (m_properties.display_rows);
	showMatchingPaperSize();
}

/**
	@brief BorderPropertiesWidget::properties
	@return the edited border properties
*/
const BorderProperties &BorderPropertiesWidget::properties ()
{
	m_properties.columns_count   = ui -> m_colums_count_sp    -> value();
	m_properties.columns_width   = ui -> m_columns_width_sp   -> value();
	m_properties.display_columns = ui -> m_display_columns_cb -> isChecked();
	m_properties.rows_count      = ui -> m_rows_count_sp      -> value();
	m_properties.rows_height     = ui -> m_rows_height_sp     -> value();
	m_properties.display_rows    = ui -> m_display_rows_cb    -> isChecked();
	return m_properties;
}

/**
	@brief BorderPropertiesWidget::setReadOnly
	Enable or disable this widget
	@param ro true-disable / false-enable
*/
void BorderPropertiesWidget::setReadOnly(const bool &ro)
{
	ui->border_gb->setDisabled(ro);
}

/**
	@brief BorderPropertiesWidget::setTitleBlockSize
	Tell the widget the title block the folio uses: its height counts in
	the size of the page. If the user picked a paper size in this widget,
	the columns and rows are fitted again so the folio keeps that size;
	otherwise they are left alone.
	@param height : height of the title block template
	@param edge : edge of the folio the title block sits on
*/
void BorderPropertiesWidget::setTitleBlockSize(int height, Qt::Edge edge)
{
	m_titleblock_height = height;
	m_titleblock_edge = edge;
	if (m_paper_picked)
		applyPaperSize();
	else
		showMatchingPaperSize();
}

/**
	@brief BorderPropertiesWidget::applyPaperSize
	Set the columns and rows so the folio, title block included, has the
	size of the chosen paper. The column width and row height stay close
	to the current ones.
*/
void BorderPropertiesWidget::applyPaperSize()
{
	const int index = ui->m_paper_cb->currentIndex() - 1;
	ui->m_orientation_cb->setEnabled(index >= 0);
	if (index < 0) return;

	int columns, column_width, rows, row_height;
	if (!fitPaperSize(index, ui->m_orientation_cb->currentIndex() == 0,
					  columns, column_width, rows, row_height))
	{
			//The title block leaves no room on this paper
		showMatchingPaperSize();
		return;
	}

	m_updating = true;
	ui->m_colums_count_sp->setValue(columns);
	ui->m_columns_width_sp->setValue(column_width);
	ui->m_rows_count_sp->setValue(rows);
	ui->m_rows_height_sp->setValue(row_height);
	m_updating = false;
}

/**
	@brief BorderPropertiesWidget::fitPaperSize
	Compute the columns and rows that give the folio, title block
	included, the size of a paper, with cells close to the current ones.
	@param index : index of the paper in paper_sizes
	@param landscape : true for landscape, false for portrait
	@return false if the paper is too small for the title block
*/
bool BorderPropertiesWidget::fitPaperSize(int index, bool landscape,
										  int &columns, int &column_width,
										  int &rows, int &row_height) const
{
	const PaperSize &paper = paper_sizes[index];
	const int page_width  = mmToPx(landscape ? paper.width_mm  : paper.height_mm);
	const int page_height = mmToPx(landscape ? paper.height_mm : paper.width_mm);

		//The page is one pixel larger than the border, for its line
	int width  = page_width  - 1 - qRound(m_properties.rows_header_width);
	int height = page_height - 1 - qRound(m_properties.columns_header_height);
	if (m_titleblock_edge == Qt::BottomEdge)
		height -= m_titleblock_height;
	else
		width -= m_titleblock_height;
	if (width < min_cell * min_count || height < min_cell * min_count)
		return false;

	const QPair<int, int> c = fitCells(width,
									   ui->m_columns_width_sp->value(),
									   min_cell,
									   ui->m_colums_count_sp->maximum());
	const QPair<int, int> r = fitCells(height,
									   ui->m_rows_height_sp->value(),
									   min_cell,
									   ui->m_rows_count_sp->maximum());
	columns = c.first;
	column_width = c.second;
	rows = r.first;
	row_height = r.second;
	return true;
}

/**
	@brief BorderPropertiesWidget::showMatchingPaperSize
	Show the paper size the folio has, or "Custom" if it has none: the
	folio has a paper size when choosing it would change nothing.
*/
void BorderPropertiesWidget::showMatchingPaperSize()
{
	if (m_updating) return;
	m_paper_picked = false;

	int found = 0;
	int orientation = ui->m_orientation_cb->currentIndex();
	for (int i = 0 ; i < int(std::size(paper_sizes)) && !found ; ++i)
	{
		for (int o = 0 ; o < 2 ; ++o)
		{
			int columns, column_width, rows, row_height;
			if (fitPaperSize(i, o == 0, columns, column_width, rows, row_height) &&
				columns      == ui->m_colums_count_sp->value() &&
				column_width == ui->m_columns_width_sp->value() &&
				rows         == ui->m_rows_count_sp->value() &&
				row_height   == ui->m_rows_height_sp->value())
			{
				found = i + 1;
				orientation = o;
				break;
			}
		}
	}
	ui->m_paper_cb->setCurrentIndex(found);
	ui->m_orientation_cb->setCurrentIndex(orientation);
	ui->m_orientation_cb->setEnabled(found > 0);
}
