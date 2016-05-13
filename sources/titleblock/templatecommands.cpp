/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "templatecommands.h"
#include "templatevisualcell.h"
#include "templateview.h"
#include "titleblockcell.h"
#include "dimension.h"
#define TITLEBLOCK_DEFAULT_ROW_HEIGHT TitleBlockDimension(25)
#define TITLEBLOCK_DEFAULT_COL_WIDTH  TitleBlockDimension(50)

/**
	Constructor
	@param cell Modified cell
	@param parent Parent QUndoCommand
*/
ModifyTitleBlockCellCommand::ModifyTitleBlockCellCommand(TitleBlockCell *cell, QUndoCommand *parent) :
	QUndoCommand(parent),
	view_(0),
	modified_cell_(cell)
{
}

/**
	Destructor
*/
ModifyTitleBlockCellCommand::~ModifyTitleBlockCellCommand() {
}

/**
	@see QUndoCommand::id()
	@return the ID of this command.
*/
int ModifyTitleBlockCellCommand::id() const {
	return(MODIFY_TITLE_BLOCK_CELL_COMMAND_ID);
}

/**
	@see QUndoCommand::mergeWith()
	@param command Command to merge with.
	@return true on success, false otherwise
*/
bool ModifyTitleBlockCellCommand::mergeWith(const QUndoCommand *command) {
	const ModifyTitleBlockCellCommand *other = static_cast<const ModifyTitleBlockCellCommand *>(command);
	if (other) {
		if (other -> modified_cell_ == modified_cell_) {
			if (other -> new_values_.keys() == new_values_.keys()) {
				new_values_ = other -> new_values_;
				return(true);
			}
		}
	}
	return(false);
}

/**
	Undo the change.
*/
void ModifyTitleBlockCellCommand::undo() {
	if (!modified_cell_) return;
	foreach (QString attribute, old_values_.keys()) {
		modified_cell_ -> setAttribute(attribute, old_values_[attribute]);
	}
	if (view_) view_ -> refresh();
}

/**
	Redo the change.
*/
void ModifyTitleBlockCellCommand::redo() {
	if (!modified_cell_) return;
	foreach (QString attribute, new_values_.keys()) {
		modified_cell_ -> setAttribute(attribute, new_values_[attribute]);
	}
	if (view_) view_ -> refresh();
}

/**
	@return the cell modified by this command
*/
TitleBlockCell *ModifyTitleBlockCellCommand::cell() const {
	return(modified_cell_);
}

/**
	Set the cell modified by this command object
	@param modified_cell the cell modified by this command
*/
void ModifyTitleBlockCellCommand::setCell(TitleBlockCell *modified_cell) {
	modified_cell_ = modified_cell;
}

/**
	@return the view to be updated after the cell modification
*/
TitleBlockTemplateView *ModifyTitleBlockCellCommand::view() const {
	return(view_);
}

/**
	Set the view to be updated after the cell modification
	@param view the view to be updated after the cell modification
*/
void ModifyTitleBlockCellCommand::setView(TitleBlockTemplateView *view) {
	view_ = view;
}

/**
	Erase the known old/new values.
*/
void ModifyTitleBlockCellCommand::clear() {
	old_values_.clear();
	new_values_.clear();
}

/**
	Register a new modification on a title block template cell; you may
	indicate either the new value or the old one: this method will
	systematically fetch the other one.
	@param attribute Name of the modified attribute
	@param value Old or new value of the modified attribute, depending on is_old_value
	@param is_old_value (optional, defaults to false) Indicates whether the provided value is the old or the new one.
*/
void ModifyTitleBlockCellCommand::addModification(const QString &attribute, const QVariant &value, bool is_old_value) {
	if (is_old_value) {
		// the provided value is the old one; therefore, the one we fetch is the new one
		old_values_[attribute] = value;
		if (modified_cell_) {
			new_values_[attribute] = modified_cell_ -> attribute(attribute);
		}
	} else {
		// the provided value is the new one; therefore, we fetch the old one
		if (modified_cell_) {
			old_values_[attribute] = modified_cell_ -> attribute(attribute);
		}
		new_values_[attribute] = value;
	}
}

/**
	Constructor
	@param tbtemplate Modified title block template
	@param parent Parent QUndoCommand
*/
TitleBlockTemplateCommand::TitleBlockTemplateCommand(TitleBlockTemplate *tbtemplate, QUndoCommand *parent) :
	QUndoCommand(parent),
	tbtemplate_(tbtemplate),
	view_(0)
{
}

/**
	Destructor
*/
TitleBlockTemplateCommand::~TitleBlockTemplateCommand() {
}

/**
	@return the modified title block template.
*/
TitleBlockTemplate *TitleBlockTemplateCommand::titleBlockTemplate() const {
	return(tbtemplate_);
}

/**
	Set the modified title block template.
	@param tbtemplate New modified title block template.
*/
void TitleBlockTemplateCommand::setTitleBlockTemplate(TitleBlockTemplate *tbtemplate) {
	tbtemplate_ = tbtemplate;
}

/**
	@return the view to be updated after the template modification
*/
TitleBlockTemplateView *TitleBlockTemplateCommand::view() const {
	return(view_);
}

/**
	Set the view to be updated after the template modification
	@param view the view to be updated after the template modification
*/
void TitleBlockTemplateCommand::setView(TitleBlockTemplateView *view) {
	view_ = view;
}

/**
	Refresh the view, if any.
*/
void TitleBlockTemplateCommand::refreshView() {
	if (!view_) return;
	view_ -> refresh();
}

/**
	Refresh the view, including layout reloading, if any.
*/
void TitleBlockTemplateCommand::refreshLayout() {
	if (!view_) return;
	view_ -> updateLayout();
}

/**
	This static method is a convenience to create a ModifyTemplateGridCommand
	that adds a row to \a tbtemplate at \a index.
	@param tbtemplate Modified title block template
	@param index Index where the row should be inserted.
	@return a ModifyTemplateGridCommand object, or 0 if something went wrong.
*/
ModifyTemplateGridCommand *ModifyTemplateGridCommand::addRow(TitleBlockTemplate *tbtemplate, int index) {
	if (!tbtemplate) return(0);
	
	// create the command itself
	ModifyTemplateGridCommand *add_row_command = new ModifyTemplateGridCommand(tbtemplate);
	add_row_command -> setInsertion(true);
	add_row_command -> setType(true);
	add_row_command -> setCells(tbtemplate -> createRow());
	add_row_command -> setDimension(TITLEBLOCK_DEFAULT_ROW_HEIGHT);
	add_row_command -> setIndex(index);
	
	return(add_row_command);
}

/**
	This static method is a convenience to create a ModifyTemplateGridCommand
	that adds a column to \a tbtemplate at \a index.
	@param tbtemplate Modified title block template.
	@param index Index where the column should be inserted.
	@return a ModifyTemplateGridCommand object, or 0 if something went wrong.
*/
ModifyTemplateGridCommand *ModifyTemplateGridCommand::addColumn(TitleBlockTemplate *tbtemplate, int index) {
	if (!tbtemplate) return(0);
	
	// create the command itself
	ModifyTemplateGridCommand *add_column_command = new ModifyTemplateGridCommand(tbtemplate);
	add_column_command -> setInsertion(true);
	add_column_command -> setType(false);
	add_column_command -> setCells(tbtemplate -> createColumn());
	add_column_command -> setDimension(TITLEBLOCK_DEFAULT_COL_WIDTH);
	add_column_command -> setIndex(index);
	
	return(add_column_command);
}

/**
	This static method is a convenience to create a ModifyTemplateGridCommand
	that removes the row at \a index from \a tbtemplate.
	@param tbtemplate Modified title block template.
	@param index Index of the removed row.
	@return a ModifyTemplateGridCommand object, or 0 if something went wrong.
*/
ModifyTemplateGridCommand *ModifyTemplateGridCommand::deleteRow(TitleBlockTemplate *tbtemplate, int index) {
	if (!tbtemplate) return(0);
	
	// create the command itself
	ModifyTemplateGridCommand *del_row_command = new ModifyTemplateGridCommand(tbtemplate);
	del_row_command -> setInsertion(false);
	del_row_command -> setType(true);
	del_row_command -> setIndex(index);
	
	return(del_row_command);
}

/**
	This static method is a convenience to create a ModifyTemplateGridCommand
	that removes the column at \a index from \a tbtemplate.
	@param tbtemplate Modified title block template.
	@param index Index of the removed column.
	@return a ModifyTemplateGridCommand object, or 0 if something went wrong.
*/
ModifyTemplateGridCommand *ModifyTemplateGridCommand::deleteColumn(TitleBlockTemplate *tbtemplate, int index) {
	if (!tbtemplate) return(0);
	
	// create the command itself
	ModifyTemplateGridCommand *del_column_command = new ModifyTemplateGridCommand(tbtemplate);
	del_column_command -> setInsertion(false);
	del_column_command -> setType(false);
	del_column_command -> setIndex(index);
	
	return(del_column_command);
}

/**
	Construct a default ModifyTemplateGridCommand, i.e. a command adding a 25px row at the bottom of the template.
	@param tbtemplate Modified title block template
	@param parent Parent QUndoCommand
*/
ModifyTemplateGridCommand::ModifyTemplateGridCommand(TitleBlockTemplate *tbtemplate, QUndoCommand *parent) :
	TitleBlockTemplateCommand(tbtemplate, parent),
	index_(-1),
	type_(true),
	dimension_(TITLEBLOCK_DEFAULT_ROW_HEIGHT),
	insertion_(true)
{
	updateText();
}

/**
	Destructor
*/
ModifyTemplateGridCommand::~ModifyTemplateGridCommand() {
}

/**
	@return the index of the inserted/deleted row/column
*/
int ModifyTemplateGridCommand::index() const {
	return(index_);
}

/**
	Set the index of the inserted/deleted row/column.
	@param index Index of the inserted/deleted row/column.
*/
void ModifyTemplateGridCommand::setIndex(int index) {
	index_ = index;
}

/**
	@return a list of pointers to cells composing the inserted/deleted row/column.
*/
QList<TitleBlockCell *> ModifyTemplateGridCommand::cells() const {
	return(cells_);
}

/**
	Set the cells composing the inserted/deleted row/column.
	@param cells List of pointers to cells composing the inserted/deleted row/column.
*/
void ModifyTemplateGridCommand::setCells(const QList<TitleBlockCell *> &cells) {
	cells_ = cells;
}

/**
	@return the dimension of the inserted/deleted row/column.
*/
TitleBlockDimension ModifyTemplateGridCommand::dimension() const {
	return dimension_;
}

/**
	Set the dimension of the inserted/deleted row/column
	@param dimension Dimension of the inserted/deleted row/column
*/
void ModifyTemplateGridCommand::setDimension(const TitleBlockDimension &dimension) {
	dimension_ = dimension;
}

/**
	@return true if this object is about inserting/deleting a row, false for a column.
*/
int ModifyTemplateGridCommand::type() const {
	return(type_);
}

/**
	Indicates whether this object inserts/deletes a row or a column.
	@param type true if this object is about inserting/deleting a row, false for a column.
*/
void ModifyTemplateGridCommand::setType(bool type) {
	type_ = type;
	updateText();
}

/**
	@return true if the row/column is inserted, false if it is deleted
*/
bool ModifyTemplateGridCommand::isInsertion() const {
	return(insertion_);
}

/**
	@param insertion true if the row/column is inserted, false if it is deleted
*/
void ModifyTemplateGridCommand::setInsertion(bool insertion) {
	insertion_ = insertion;
	updateText();
}

/**
	Undo the change.
*/
void ModifyTemplateGridCommand::undo() {
	apply(true);
}

/**
	Redo the change.
*/
void ModifyTemplateGridCommand::redo() {
	apply(false);
}

/**
	Update the text describing what the command does.
*/
void ModifyTemplateGridCommand::updateText() {
	if (type_) {
		if (insertion_) {
			setText(QObject::tr("Insertion d'une ligne", "label used in the title block template editor undo list"));
		} else {
			setText(QObject::tr("Suppression d'une ligne", "label used in the title block template editor undo list"));
		}
	} else {
		if (insertion_) {
			setText(QObject::tr("Insertion d'une colonne", "label used in the title block template editor undo list"));
		} else {
			setText(QObject::tr("Suppression d'une colonne", "label used in the title block template editor undo list"));
		}
	}
}

/*
	This method takes care of the actual job when undoing / redoing a
	row/column insertion/removal.
	@param true to undo the change, false to apply it.
*/
void ModifyTemplateGridCommand::apply(bool undo) {
	if (!tbtemplate_ || index_ == -1) return;
	
	if (insertion_ ^ undo) {
		if (type_) {
			tbtemplate_ -> insertRow(dimension_.value, cells_, index_);
		} else {
			tbtemplate_ -> insertColumn(dimension_, cells_, index_);
		}
	} else {
		if (type_) {
			dimension_.value = tbtemplate_ -> rowDimension(index_);
			cells_ = tbtemplate_ -> takeRow(index_);
		} else {
			dimension_ = tbtemplate_ -> columnDimension(index_);
			cells_ = tbtemplate_ -> takeColumn(index_);
		}
	}
	
	// update the view, if any
	if (view_) {
		view_ -> updateLayout();
	}
}

/**
	Construct a default ModifyTemplateDimension.
	@param tbtemplate Modified title block template
	@param parent Parent QUndoCommand
*/
ModifyTemplateDimension::ModifyTemplateDimension(TitleBlockTemplate *tbtemplate, QUndoCommand *parent) :
	TitleBlockTemplateCommand(tbtemplate, parent),
	index_(-1),
	type_(true),
	before_(TitleBlockDimension(-1)),
	after_(TitleBlockDimension(-1))
{
}

/**
	Destructor
*/
ModifyTemplateDimension::~ModifyTemplateDimension() {
}

/**
	@return the index of the resized row/column
*/
int ModifyTemplateDimension::index() const {
	return(index_);
}

/**
	Set the index of the resized row/column.
	@param index Index of the resized row/column.
*/
void ModifyTemplateDimension::setIndex(int index) {
	index_ = index;
}

/**
	@return true if this object is about resizing a row, false for a column.
*/
int ModifyTemplateDimension::type() const {
	return type_;
}

/**
	Indicates whether this object resizes a row or a column.
	@param type true if this object is about resizing a row, false for a column.
*/
void ModifyTemplateDimension::setType(bool type) {
	type_ = type;
	updateText();
}

/**
	@return the dimension of the row/column before it is resized
*/
TitleBlockDimension ModifyTemplateDimension::dimensionBefore() const {
	return(before_);
}

/**
	@param dimension the dimension of the row/column before it is resized
*/
void ModifyTemplateDimension::setDimensionBefore(const TitleBlockDimension &dimension) {
	before_ = dimension;
}

/**
	@return the dimension of the row/column after it is resized
*/
TitleBlockDimension ModifyTemplateDimension::dimensionAfter() const {
	return(after_);
}

/**
	@param dimension the dimension of the row/column after it is resized
*/
void ModifyTemplateDimension::setDimensionAfter(const TitleBlockDimension &dimension) {
	after_ = dimension;
}

/**
	Restore the previous size of the row/column.
*/
void ModifyTemplateDimension::undo() {
	apply(before_);
}

/**
	Resize the row/column.
*/
void ModifyTemplateDimension::redo() {
	apply(after_);
}

/**
	Update the text describing what the command does.
*/
void ModifyTemplateDimension::updateText() {
	if (type_) {
		setText(QObject::tr("Modification d'une ligne", "label used in the title block template editor undo list"));
	} else {
		setText(QObject::tr("Modification d'une colonne", "label used in the title block template editor undo list"));
	}
}

/**
	Applies a given size to the row/column
	@param dimension Size to apply
*/
void ModifyTemplateDimension::apply(const TitleBlockDimension &dimension) {
	if (!tbtemplate_) return;
	if (type_) {
		tbtemplate_ -> setRowDimension(index_, dimension);
	} else {
		tbtemplate_ -> setColumnDimension(index_, dimension);
	}
	if (view_) {
		if (type_) {
			view_ -> rowsDimensionsChanged();
		} else {
			view_ -> columnsDimensionsChanged();
		}
	}
}

/**
	Construct a command object that acts on \a tbtemplate in order to merge \a merged_cells.
	Note: you should check the resulting object is valid using isValid().
	@param merged_cells Cells to be merged together into a single one.
	@param tbtemplate Modified title block template.
	@param parent Parent QUndoCommand.
*/
MergeCellsCommand::MergeCellsCommand(const TitleBlockTemplateCellsSet &merged_cells, TitleBlockTemplate *tbtemplate, QUndoCommand *parent) :
	TitleBlockTemplateCommand(tbtemplate, parent),
	spanning_cell_(0),
	row_span_after_(-1),
	col_span_after_(-1)
{
	if (!canMerge(merged_cells, tbtemplate)) return;
	
	// the spanning cell is the top left cell
	TitleBlockTemplateVisualCell *top_left_cell = merged_cells.topLeftCell();
	if (!top_left_cell) return;
	spanning_cell_ = top_left_cell -> cell();
	if (!spanning_cell_) return;
	
	// store the spanner_cell attribute of each cell implied in the merge
	foreach(TitleBlockCell *cell, merged_cells.cells()) {
		spanner_cells_before_merge_.insert(cell, cell -> spanner_cell);
	}
	
	// store the former values of the row_span and col_span attributes of the spanning cell
	row_span_before_ = spanning_cell_ -> row_span;
	col_span_before_ = spanning_cell_ -> col_span;
	applied_row_span_before_ = spanning_cell_ -> applied_row_span;
	applied_col_span_before_ = spanning_cell_ -> applied_col_span;
	span_state_before_ = spanning_cell_ -> span_state;
	
	// calculate their new values after the merge operation
	TitleBlockCell *bottom_right_cell = getBottomRightCell(merged_cells);
	if (!bottom_right_cell) return;
	row_span_after_ = bottom_right_cell -> num_row - spanning_cell_ -> num_row;
	col_span_after_ = bottom_right_cell -> num_col - spanning_cell_ -> num_col;
	
	setText(
		QString(
			QObject::tr(
				"Fusion de %1 cellules",
				"label used in the title block template editor undo list; %1 is the number of merged cells"
			)
		).arg(merged_cells.count())
	);
}

/**
	Destructor
*/
MergeCellsCommand::~MergeCellsCommand() {
}

/**
	@param merged_cells Cell sto be merged.
	@param tbtemplate Modified title block template.
	@return true if the merge is feasible, false otherwise
*/
bool MergeCellsCommand::canMerge(const TitleBlockTemplateCellsSet &merged_cells, TitleBlockTemplate *tbtemplate) {
	Q_UNUSED(tbtemplate)
	
	// basic checks
	if (!merged_cells.isRectangle()) return(false);
	if (merged_cells.count() < 2) return(false);
	
	// the spanning cell is the top left cell
	TitleBlockTemplateVisualCell *top_left_cell = merged_cells.topLeftCell();
	if (!top_left_cell || !top_left_cell -> cell()) return(false);
	
	if (!getBottomRightCell(merged_cells)) return(false);
	
	return(true);
}

/**
	@return true if this command object is valid and usable, false otherwise.
*/
bool MergeCellsCommand::isValid() const {
	// we consider having a non-zero spanning cell and positive spans makes a MergeCellsCommand valid
	return(spanning_cell_ && row_span_after_ != -1 && col_span_after_ != -1);
}

/**
	Undo the merge operation.
*/
void MergeCellsCommand::undo() {
	if (!isValid()) return;
	
	// restore the original spanning_cell attribute of all impacted cells
	foreach (TitleBlockCell *cell, spanner_cells_before_merge_.keys()) {
		cell -> spanner_cell = spanner_cells_before_merge_[cell];
	}
	
	// restore the span-related attributes of the spanning cell
	spanning_cell_ -> row_span = row_span_before_;
	spanning_cell_ -> col_span = col_span_before_;
	spanning_cell_ -> applied_row_span = applied_row_span_before_;
	spanning_cell_ -> applied_col_span = applied_col_span_before_;
	spanning_cell_ -> span_state = span_state_before_;
	
	if (view_) view_ -> updateLayout();
}

/**
	Apply the merge operation
*/
void MergeCellsCommand::redo() {
	if (!isValid()) return;
	
	// set the spanning_cell attributes of spanned cells to the spanning cell
	foreach (TitleBlockCell *cell, spanner_cells_before_merge_.keys()) {
		if (cell == spanning_cell_) continue;
		cell -> spanner_cell = spanning_cell_;
	}
	
	// set the new values of the row_span and col_span attributes
	spanning_cell_ -> row_span = row_span_after_;
	spanning_cell_ -> col_span = col_span_after_;
	spanning_cell_ -> applied_row_span = row_span_after_;
	spanning_cell_ -> applied_col_span = col_span_after_;
	spanning_cell_ -> span_state = TitleBlockCell::Enabled;
	
	if (view_) view_ -> updateLayout();
}

/**
	@param cells_set Set of title block template visual cells.
	@return the bottom right logical cell within a set of visual cells.
*/
TitleBlockCell *MergeCellsCommand::getBottomRightCell(const TitleBlockTemplateCellsSet &cells_set) {
	// first, we get the visual cell at the bottom right
	TitleBlockTemplateVisualCell *bottom_right_cell = cells_set.bottomRightCell();
	if (!bottom_right_cell) return(0);
	
	// next, we get its logical cells: the painted one and the spanned ones (if any)
	QSet<TitleBlockCell *> logical_cells = bottom_right_cell -> cells();
	if (logical_cells.isEmpty()) return(0);
	if (logical_cells.count() == 1) return(logical_cells.toList().first());
	
	// we then look for the bottom right logical cell
	int max_num_row = -1, max_num_col = -1;
	TitleBlockCell *candidate = 0;
	foreach(TitleBlockCell *cell, logical_cells) {
		if (cell -> num_row > max_num_row) max_num_row = cell -> num_row;
		if (cell -> num_col > max_num_col) max_num_col = cell -> num_col;
		if (cell -> num_row == max_num_row && cell -> num_col == max_num_col) {
			candidate = cell;
		}
	}
	return(candidate);
}

/**
	Construct a command object that acts on \a tbtemplate in order to split
	\a splitted_cells.
	Note: you should check the resulting object is valid using isValid().
	@param splitted_cells Cell to be splitted.
	@param tbtemplate Modified title block template.
	@param parent Parent QUndoCommand.
*/
SplitCellsCommand::SplitCellsCommand(const TitleBlockTemplateCellsSet &splitted_cells, TitleBlockTemplate *tbtemplate, QUndoCommand *parent) :
	TitleBlockTemplateCommand(tbtemplate, parent),
	spanning_cell_(0),
	row_span_before_(-1),
	col_span_before_(-1)
{
	if (!canSplit(splitted_cells, tbtemplate)) return;
	
	// retrieve values necessary for the undo operation
	spanning_cell_ = splitted_cells.first() -> cell();
	spanned_cells_ = tbtemplate_ -> spannedCells(spanning_cell_);
	row_span_before_ = spanning_cell_ -> row_span;
	col_span_before_ = spanning_cell_ -> col_span;
	applied_row_span_before_ = spanning_cell_ -> row_span;
	applied_col_span_before_ = spanning_cell_ -> col_span;
	span_state_before_ = spanning_cell_ -> span_state;
	
	setText(
		QString(
			QObject::tr(
				"Séparation d'une cellule en %1",
				"label used in the title block template editor undo list; %1 is the number of cells after the split"
			)
		).arg(spanned_cells_.count() + 1)
	);
}

/**
	Destructor
*/
SplitCellsCommand::~SplitCellsCommand() {
}

/**
	@param splitted_cells Cell to be splitted.
	@param tbtemplate Modified title block template.
	@return true if the split is feasible, false otherwise
*/
bool SplitCellsCommand::canSplit(const TitleBlockTemplateCellsSet &splitted_cells, TitleBlockTemplate *tbtemplate) {
	Q_UNUSED(tbtemplate)
	
	// basic check: the command applies to a single visual cell only
	if (splitted_cells.count() != 1) return(false);
	
	// fetch the spanning cell
	TitleBlockCell *spanning_cell = splitted_cells.first() -> cell();
	if (!spanning_cell) return(false);
	
	// ensure the cell spans over other cells and therefore can be splitted
	if (!spanning_cell -> spans()) return(false);
	
	return(true);
}

/**
	@return true if this command object is valid and usable, false otherwise.
*/
bool SplitCellsCommand::isValid() const {
	// we consider having a non-zero spanning cell and at least one spanned cell makes a SplitCellsCommand valid
	return(spanning_cell_ && spanned_cells_.count());
}

/**
	Undo the split operation
*/
void SplitCellsCommand::undo() {
	if (!isValid()) return;
	
	// the spanned cells are spanned again
	foreach(TitleBlockCell *cell, spanned_cells_) {
		cell -> spanner_cell = spanning_cell_;
	}
	
	// the spanning cell span again
	spanning_cell_ -> row_span = row_span_before_;
	spanning_cell_ -> col_span = col_span_before_;
	spanning_cell_ -> applied_row_span = applied_row_span_before_;
	spanning_cell_ -> applied_col_span = applied_col_span_before_;
	spanning_cell_ -> span_state = span_state_before_;
	
	if (view_) view_ -> updateLayout();
}

/**
	Apply the split operation
*/
void SplitCellsCommand::redo() {
	if (!isValid()) return;
	
	// the spanned cells are not spanned anymore
	foreach(TitleBlockCell *cell, spanned_cells_) {
		cell -> spanner_cell = 0;
	}
	
	// the spanning cell does not span anymore
	spanning_cell_ -> row_span = 0;
	spanning_cell_ -> col_span = 0;
	tbtemplate_ -> checkCellSpan(spanning_cell_);
	
	if (view_) view_ -> updateLayout();
}


/**
	Constructor
	@param tbt Changed title block template
	@param old_info Former information
	@param new_info New information
	@param parent Parent QUndoCommand
*/
ChangeTemplateInformationsCommand::ChangeTemplateInformationsCommand(TitleBlockTemplate *tbt, const QString &old_info, const QString &new_info, QUndoCommand *parent) :
	QUndoCommand(QObject::tr("modification des informations complémentaires", "undo caption"), parent),
	tbtemplate_(tbt),
	old_information_(old_info),
	new_information_(new_info)
{
}

/**
	Destructor
*/
ChangeTemplateInformationsCommand::~ChangeTemplateInformationsCommand() {
}

/**
	Undo the information change
*/
void ChangeTemplateInformationsCommand::undo() {
	if (!tbtemplate_) return;
	tbtemplate_ -> setInformation(old_information_);
}

/**
	Redo the information change
*/
void ChangeTemplateInformationsCommand::redo() {
	tbtemplate_ -> setInformation(new_information_);
}

/**
	Constructor
*/
CutTemplateCellsCommand::CutTemplateCellsCommand(TitleBlockTemplate *tb_template, QUndoCommand *parent) :
	TitleBlockTemplateCommand(tb_template, parent)
{
}

/**
	Destructor
*/
CutTemplateCellsCommand::~CutTemplateCellsCommand() {
}

/**
	Undo a cut operation
*/
void CutTemplateCellsCommand::undo() {
	foreach (TitleBlockCell *cell, cut_cells_.keys()) {
		cell -> cell_type = cut_cells_.value(cell);
	}
	refreshView();
}

/**
	Redo a cut operation
*/
void CutTemplateCellsCommand::redo() {
	foreach (TitleBlockCell *cell, cut_cells_.keys()) {
		cell -> cell_type = TitleBlockCell::EmptyCell;
	}
	refreshView();
}

void CutTemplateCellsCommand::setCutCells(const QList<TitleBlockCell *> &cells) {
	foreach (TitleBlockCell *cell, cells) {
		cut_cells_.insert(cell, cell -> cell_type);
	}
	updateText();
}

/**
	Update the label describing this command
*/
void CutTemplateCellsCommand::updateText() {
	setText(QObject::tr("Couper %n cellule(s)", "undo caption", cut_cells_.count()));
}


/**
	Constructor
	@param tb_template Changed title block template
	@param parent Parent command
*/
PasteTemplateCellsCommand::PasteTemplateCellsCommand(TitleBlockTemplate *tb_template, QUndoCommand *parent) :
	TitleBlockTemplateCommand(tb_template, parent)
{
}

/**
	Destructor
*/
PasteTemplateCellsCommand::~PasteTemplateCellsCommand() {
}

/**
	Update the label describing this command
*/
void PasteTemplateCellsCommand::updateText() {
	setText(QObject::tr("Coller %n cellule(s)", "undo caption", erased_cells_.count()));
}

/**
	Undo a paste action.
*/
void PasteTemplateCellsCommand::undo() {
	bool span_management = erased_cells_.count() > 1;
	foreach (TitleBlockCell *cell, erased_cells_.keys()) {
		cell -> loadContentFromCell(erased_cells_.value(cell));
	}
	if (span_management) {
		// restore all span parameters as they were before the paste operation.
		tbtemplate_ -> setAllSpans(spans_before_);
		tbtemplate_ -> applyCellSpans();
		refreshLayout();
	} else {
		refreshView();
	}
}

/**
	Redo a paste action.
*/
void PasteTemplateCellsCommand::redo() {
	// we only play with spans when pasting more than one cell.
	bool span_management = erased_cells_.count() > 1;
	
	if (span_management) {
		// When pasting several cells, we may modify the span parameters of existing,
		// non-erased cells. The easiest way to ensure everything can be restored at its
		// initial state consists in saving the span parameters of every cell.
		if (spans_before_.isEmpty()) {
			spans_before_ = tbtemplate_ -> getAllSpans();
		}
	}
	
	// copy data from each pasted cell into each erased cell
	foreach (TitleBlockCell *cell, erased_cells_.keys()) {
		if (span_management) {
			// the erased cell may be spanned by another cell
			if (TitleBlockCell *spanning_cell = cell -> spanner_cell) {
				// for the moment, we simply cancel the whole spanning
				tbtemplate_ -> forgetSpanning(spanning_cell);
			}
		}
		
		// copy non-spans data
		TitleBlockCell pasted_cell = pasted_cells_.value(cell);
		cell -> loadContentFromCell(pasted_cell);
		
		if (span_management) {
			// copy spans data
			if ((pasted_cell.row_span != cell -> row_span) || (pasted_cell.col_span != cell -> col_span)) {
				tbtemplate_ -> forgetSpanning(cell);
				
				// Note: the code below is similar to TitleBlockTemplate::checkCell() but is more aggressive (spans deletion).
				// set the new/pasted span parameters
				cell -> row_span = qBound(0, pasted_cell.row_span, tbtemplate_ -> rowsCount() - 1 - cell -> num_row);
				cell -> col_span = qBound(0, pasted_cell.col_span, tbtemplate_ -> columnsCount() - 1 - cell -> num_col);
				
				if (cell -> row_span || cell -> col_span) {
					// browse newly spanned cells...
					foreach (TitleBlockCell *spanned_cell, tbtemplate_ -> spannedCells(cell, true)) {
						// ... to ensure they are not already spanned by other cells
						if (spanned_cell -> spanner_cell && spanned_cell -> spanner_cell != cell) {
							// if so, simply cancel the whole spanning
							tbtemplate_ -> forgetSpanning(spanned_cell -> spanner_cell);
						}
					}
					
					// set the spanner_cell attribute of newly spanned cells
					tbtemplate_ -> applyCellSpan(cell);
				}
			}
		}
	}
	if (span_management) {
		refreshLayout();
	} else {
		refreshView();
	}
}

/**
	@param cell Pointer to the cell impacted by te paste operation
	@param new_cell_content Content pasted to the cell
*/
void PasteTemplateCellsCommand::addPastedCell(TitleBlockCell *cell, const TitleBlockCell &new_cell_content) {
	pasted_cells_.insert(cell, new_cell_content);
}

/**
	@param cell Pointer to the cell impacted by te paste operation
	@param former_cell_content Content of the cell before the paste operation
*/
void PasteTemplateCellsCommand::addErasedCell(TitleBlockCell *cell, const TitleBlockCell &former_cell_content) {
	erased_cells_.insert(cell, former_cell_content);
	updateText();
}

/**
	This is a convenience function equivalent to:
	addErasedCell(cell, before)
	addPastedCell(cell, after)
*/
void PasteTemplateCellsCommand::addCell(TitleBlockCell *cell, const TitleBlockCell &before, const TitleBlockCell &after) {
	addPastedCell(cell, after);
	addErasedCell(cell, before);
}
