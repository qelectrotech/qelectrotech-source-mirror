/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "templatecellsset.h"
#include "templatevisualcell.h"
#include "templateview.h"
#include "titleblockcell.h"

/**
	Constructor
	@param parent_view View this set of cells are rattached to
*/
TitleBlockTemplateCellsSet::TitleBlockTemplateCellsSet(const TitleBlockTemplateView *parent_view) :
	parent_view_(parent_view)
{
}

/**
	Copy constructor
	@param copy TitleBlockTemplateCellsSet object to copy
*/
TitleBlockTemplateCellsSet::TitleBlockTemplateCellsSet(const TitleBlockTemplateCellsSet &copy) :
	QList<TitleBlockTemplateVisualCell *>(copy),
	parent_view_(copy.parent_view_)
{
}

/**
	Destructor
*/
TitleBlockTemplateCellsSet::~TitleBlockTemplateCellsSet() {
}

/**
	@return a QPainterPath composed of the rectangles from cells within this set
*/
QPainterPath TitleBlockTemplateCellsSet::painterPath() const {
	QPainterPath cells_path;
	foreach (TitleBlockTemplateVisualCell *cell, *this) {
		cells_path.addRect(cell -> geometry());
	}
	return(cells_path);
}

/**
	@return true if the cells within this set are composing a rectangle shape,
	false otherwise.
*/
bool TitleBlockTemplateCellsSet::isRectangle() const {
	if (!count()) return(false);
	if (count() == 1) return(true);
	
	QPolygonF points = painterPath().simplified().toFillPolygon();
	if (points.isClosed()) points.pop_back();
	
	return(points.count() == 4 || points.count() == 5);
}

/**
	@return true if all cells within this set are selected
*/
bool TitleBlockTemplateCellsSet::allCellsAreSelected() const {
	foreach (TitleBlockTemplateVisualCell *cell, *this) {
		if (!cell -> isSelected()) {
			return(false);
		}
	}
	return(true);
}

/**
	@return true if this set includes at least one cell which is spanned by a
	cell not present in this set, false otherwise.
*/
bool TitleBlockTemplateCellsSet::hasExternalSpan() const {
	// fetches all cells concerned by this set
	QSet<TitleBlockCell *> all_cells = cells(true);
	
	// look for cells spanned by cells that do not belong to this set
	foreach (TitleBlockCell *cell, all_cells) {
		if (cell -> spanner_cell && !all_cells.contains(cell -> spanner_cell)) {
			return(true);
		}
	}
	return(false);
}

/**
	@return the top left cell within this set, or 0 if this set is empty
*/
TitleBlockTemplateVisualCell *TitleBlockTemplateCellsSet::topLeftCell() const {
	if (empty()) return(0);
	if (count() == 1) return(first());
	
	// look for cells at the top
	QMultiMap<int, TitleBlockTemplateVisualCell *> top_cells;
	foreach (TitleBlockTemplateVisualCell *cell_view, *this) {
		if (TitleBlockCell *cell = cell_view -> cell()) {
			top_cells.insertMulti(cell -> num_row, cell_view);
		}
	}
	QList<TitleBlockTemplateVisualCell *> candidates = top_cells.values(top_cells.keys().first());
	if (candidates.count() == 1) return(candidates.first());
	
	// look for the cell at the left
	int lowest_num_col = 100000;
	TitleBlockTemplateVisualCell *candidate = 0;
	foreach (TitleBlockTemplateVisualCell *cell_view, candidates) {
		if (TitleBlockCell *cell = cell_view -> cell()) {
			if (cell -> num_col < lowest_num_col) {
				lowest_num_col = cell -> num_col;
				candidate = cell_view;
			}
		}
	}
	return(candidate);
}

/**
	@return the bottom right cell within this set, or 0 if this set is empty
*/
TitleBlockTemplateVisualCell *TitleBlockTemplateCellsSet::bottomRightCell() const {
	if (empty()) return(0);
	if (count() == 1) return(first());
	
	// look for cells at the bottom
	QMultiMap<qreal, TitleBlockTemplateVisualCell *> bottom_cells;
	foreach (TitleBlockTemplateVisualCell *cell_view, *this) {
		bottom_cells.insertMulti(cell_view -> geometry().bottom(), cell_view);
	}
	QList<TitleBlockTemplateVisualCell *> candidates = bottom_cells.values(bottom_cells.keys().last());
	if (candidates.count() == 1) return(candidates.first());
	
	// look for the cell at the right
	qreal highest_right = -100000;
	TitleBlockTemplateVisualCell *candidate = 0;
	foreach (TitleBlockTemplateVisualCell *cell_view, candidates) {
		qreal right = cell_view -> geometry().right();
		if (right > highest_right) {
			highest_right = right;
			candidate = cell_view;
		}
	}
	return(candidate);
}

/**
	@return the merge area, i.e. the rectangle delimited by the top left cell
	and the bottom right cell within this cells set.
*/
QRectF TitleBlockTemplateCellsSet::mergeAreaRect() const {
	QRectF merge_area;
	if (!parent_view_) return(merge_area);
	
	TitleBlockTemplateVisualCell *top_left_cell = topLeftCell();
	if (!top_left_cell) return(merge_area);
	TitleBlockTemplateVisualCell *bottom_right_cell = bottomRightCell();
	if (!bottom_right_cell) return(merge_area);
	
	merge_area.setTopLeft(top_left_cell -> geometry().topLeft());
	merge_area.setBottomRight(bottom_right_cell -> geometry().bottomRight());
	return(merge_area);
}

/**
	@param rect (Optional) The merge area to be considered; if a null QRectF is
	provided, this method will use mergeAreaRect().
	@return the cells contained in the merge area of this cells set
*/
TitleBlockTemplateCellsSet TitleBlockTemplateCellsSet::mergeArea(const QRectF &rect) const {
	TitleBlockTemplateCellsSet merge_area(parent_view_);
	if (!parent_view_) return(merge_area);
	
	QRectF merge_area_rect = rect.isNull() ? mergeAreaRect() : rect;
	
	merge_area = parent_view_ -> cells(merge_area_rect);
	return(merge_area);
}

/**
	@return the list of cells rendered by the current selection
	@param include_spanned whether to include spanned cells or not
*/
QSet<TitleBlockCell *> TitleBlockTemplateCellsSet::cells(bool include_spanned) const {
	QSet<TitleBlockCell *> set;
	foreach (TitleBlockTemplateVisualCell *cell_view, *this) {
		if (TitleBlockCell *cell = cell_view -> cell()) {
			if (include_spanned) {
				foreach (TitleBlockCell *cell, cell_view -> cells()) {
					set << cell;
				}
			} else {
				set << cell;
			}
		}
	}
	return(set);
}
