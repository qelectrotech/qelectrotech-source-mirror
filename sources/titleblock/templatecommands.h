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
#ifndef TITLEBLOCK_SLASH_TEMPLATE_COMMANDS_H
#define TITLEBLOCK_SLASH_TEMPLATE_COMMANDS_H
#define MODIFY_TITLE_BLOCK_CELL_COMMAND_ID 6378
#include <QtCore>
#include <QUndoCommand>
#include "dimension.h"
#include "templatecellsset.h"
#include "titleblockcell.h"
class TitleBlockTemplateView;
class TitleBlockTemplate;

/**
	This class represents a set of modification applied to a title block
	template cell.
*/
class ModifyTitleBlockCellCommand : public QUndoCommand {
	// constructor, destructor
	public:
	ModifyTitleBlockCellCommand(TitleBlockCell *, QUndoCommand * = 0);
	virtual ~ModifyTitleBlockCellCommand();
	private:
	ModifyTitleBlockCellCommand(const ModifyTitleBlockCellCommand &);
	
	// methods
	public:
	virtual int id() const;
	virtual bool mergeWith(const QUndoCommand *);
	virtual void undo();
	virtual void redo();
	TitleBlockCell *cell() const;
	void setCell(TitleBlockCell *);
	TitleBlockTemplateView *view() const;
	void setView(TitleBlockTemplateView *);
	void clear();
	void addModification(const QString &, const QVariant &, bool = false);
	
	// attributes
	private:
	TitleBlockTemplateView *view_;        ///< This class may trigger a view update
	TitleBlockCell *modified_cell_;       ///< modified cell
	QHash<QString, QVariant> old_values_; ///< values before the cell is modified
	QHash<QString, QVariant> new_values_; ///< values after the cell has been modified
};

/**
	This class is a base class for any UndoCommand that needs to work with a
	title block template.
*/
class TitleBlockTemplateCommand : public QUndoCommand {
	// Constructors, destructor
	public:
	TitleBlockTemplateCommand(TitleBlockTemplate * = 0, QUndoCommand * = 0);
	virtual ~TitleBlockTemplateCommand();
	private:
	TitleBlockTemplateCommand(const TitleBlockTemplateCommand &);
	
	// methods
	public:
	TitleBlockTemplate *titleBlockTemplate() const;
	void setTitleBlockTemplate(TitleBlockTemplate *);
	TitleBlockTemplateView *view() const;
	void setView(TitleBlockTemplateView *);
	void refreshView();
	void refreshLayout();
	
	// attributes
	protected:
	TitleBlockTemplate *tbtemplate_; ///< Modified TitleBlock Template
	TitleBlockTemplateView *view_;   ///< This class may trigger a view update
};

/**
	This class represents the action of adding or deleting a row or column
	within a title block template.
*/
class ModifyTemplateGridCommand : public TitleBlockTemplateCommand {
	// static factory methods
	public:
	static ModifyTemplateGridCommand *addRow(TitleBlockTemplate *, int = -1);
	static ModifyTemplateGridCommand *addColumn(TitleBlockTemplate *, int = -1);
	static ModifyTemplateGridCommand *deleteRow(TitleBlockTemplate *, int = -1);
	static ModifyTemplateGridCommand *deleteColumn(TitleBlockTemplate *, int = -1);
	
	// Constructors, destructor
	public:
	ModifyTemplateGridCommand(TitleBlockTemplate * = 0, QUndoCommand * = 0);
	virtual ~ModifyTemplateGridCommand();
	private:
	ModifyTemplateGridCommand(const ModifyTemplateGridCommand &);
	
	// methods
	public:
	int index() const;
	void setIndex(int);
	QList<TitleBlockCell *> cells() const;
	void setCells(const QList<TitleBlockCell *> &);
	TitleBlockDimension dimension() const;
	void setDimension(const TitleBlockDimension &);
	int type() const;
	void setType(bool);
	bool isInsertion() const;
	void setInsertion(bool);
	virtual void undo();
	virtual void redo();
	
	private:
	void updateText();
	void apply(bool = false);
	
	// attributes
	private:
	int index_;                     ///< Index of the inserted/deleted row/column
	QList<TitleBlockCell *> cells_; ///< Cells composing the inserted/deleted row/column
	bool type_;                     ///< true for a row, false for a column
	TitleBlockDimension dimension_; ///< width/height of the column/row, which interpretation depends on type_
	bool insertion_;                /// true if the row/column is inserted, false if it is deleted
};

/**
	This class represents the action of changing the width/height of a
	specific row/column within a title block template.
*/
class ModifyTemplateDimension : public TitleBlockTemplateCommand {
	// Constructor, destructor
	public:
	ModifyTemplateDimension(TitleBlockTemplate * = 0, QUndoCommand * = 0);
	virtual ~ModifyTemplateDimension();
	private:
	ModifyTemplateDimension(const ModifyTemplateDimension &);
	
	// methods
	public:
	int index() const;
	void setIndex(int);
	int type() const;
	void setType(bool);
	TitleBlockDimension dimensionBefore() const;
	void setDimensionBefore(const TitleBlockDimension &);
	TitleBlockDimension dimensionAfter() const;
	void setDimensionAfter(const TitleBlockDimension &);
	virtual void undo();
	virtual void redo();
	
	private:
	void updateText();
	void apply(const TitleBlockDimension &);
	
	// attributes
	private:
	int index_;                   ///< Index of the resized row/column
	bool type_;                   ///< true for a row, false for a column
	TitleBlockDimension before_;  ///< Size of the row/column before it is changed
	TitleBlockDimension after_;   ///< Size of the row/column after it is changed
};


/**
	This class represents the action of merging 2 to n cells into a single one.
*/
class MergeCellsCommand : public TitleBlockTemplateCommand {
	// Constructor, destructor
	public:
	MergeCellsCommand(const TitleBlockTemplateCellsSet &, TitleBlockTemplate * = 0, QUndoCommand * = 0);
	virtual ~MergeCellsCommand();
	
	// methods
	public:
	static bool canMerge(const TitleBlockTemplateCellsSet &, TitleBlockTemplate *);
	bool isValid() const;
	virtual void undo();
	virtual void redo();
	private:
	static TitleBlockCell *getBottomRightCell(const TitleBlockTemplateCellsSet &);
	
	// attributes
	private:
	/// the cell spanning over the other ones
	TitleBlockCell *spanning_cell_;
	/// hash associating spanned cells with their spanner_cell attribute
	/// before the merge operation
	QHash<TitleBlockCell *, TitleBlockCell *> spanner_cells_before_merge_;
	int row_span_before_; ///< the row_span attribute of the spanning cell before the merge
	int col_span_before_; ///< the col_span attribute of the spanning cell before the merge
	int applied_row_span_before_; ///< the applied_row_span attribute of the spanning cell before the merge
	int applied_col_span_before_; ///< the applied_col_span attribute of the spanning cell before the merge
	int span_state_before_; ///< the span_state attribute of the spanning cell before the merge
	int row_span_after_;  ///< the row_span attribute of the spanning cell after the merge
	int col_span_after_;  ///< the col_span attribute of the spanning cell after the merge
};

/**
	This class represents the action of splitting a visual cell into at least two logical cells
*/
class SplitCellsCommand : public TitleBlockTemplateCommand {
	// Constructor, destructor
	public:
	SplitCellsCommand(const TitleBlockTemplateCellsSet &, TitleBlockTemplate * = 0, QUndoCommand * = 0);
	virtual ~SplitCellsCommand();
	
	// methods
	public:
	static bool canSplit(const TitleBlockTemplateCellsSet &splitted_cells, TitleBlockTemplate *tbtemplate);
	bool isValid() const;
	virtual void undo();
	virtual void redo();
	
	// attributes
	private:
	TitleBlockCell *spanning_cell_;        ///< the cell spanning over the other ones
	QSet<TitleBlockCell *> spanned_cells_; ///< the spanned cells
	int row_span_before_;                  ///< the row_span attribute of the spanning cell before splitting
	int col_span_before_;                  ///< the col_span attribute of the spanning cell before splitting
	int applied_row_span_before_;          ///< the applied_row_span attribute of the spanning cell before splitting
	int applied_col_span_before_;          ///< the applied_col_span attribute of the spanning cell before splitting
	int span_state_before_;                ///< the span_state attribute of the spanning cell before splitting
};

/**
	This class represents the action of changing extra information of a title
	block template.
*/
class ChangeTemplateInformationsCommand : public QUndoCommand {
	// constructors, destructor
	public:
	ChangeTemplateInformationsCommand(TitleBlockTemplate *, const QString &, const QString &, QUndoCommand * = 0);
	virtual ~ChangeTemplateInformationsCommand();
	private:
	ChangeTemplateInformationsCommand(const ChangeTemplateInformationsCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	
	// attributes
	private:
	/// Changed title block template
	TitleBlockTemplate *tbtemplate_;
	/// Informations before they are modified
	QString old_information_;
	/// Informations after they were modified
	QString new_information_;
};

/**
	This class represents the action of cutting a cells set.
*/
class CutTemplateCellsCommand : public TitleBlockTemplateCommand {
	// constructors, destructor
	public:
	CutTemplateCellsCommand(TitleBlockTemplate *, QUndoCommand * = 0);
	virtual ~CutTemplateCellsCommand();
	private:
	CutTemplateCellsCommand(const CutTemplateCellsCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	virtual void setCutCells(const QList<TitleBlockCell *> &);
	protected:
	virtual void updateText();
	
	// attributes
	public:
	/// Cut cells
	QHash<TitleBlockCell *, TitleBlockCell::TemplateCellType> cut_cells_;
};

/**
	This class represents the action of pasting a cells set.
*/
class PasteTemplateCellsCommand : public TitleBlockTemplateCommand {
	// constructors, destructor
	public:
	PasteTemplateCellsCommand(TitleBlockTemplate *, QUndoCommand * = 0);
	virtual ~PasteTemplateCellsCommand();
	private:
	PasteTemplateCellsCommand(const PasteTemplateCellsCommand &);
	
	// methods
	public:
	virtual void undo();
	virtual void redo();
	virtual void addPastedCell(TitleBlockCell *, const TitleBlockCell &);
	virtual void addErasedCell(TitleBlockCell *, const TitleBlockCell &);
	virtual void addCell(TitleBlockCell *, const TitleBlockCell &, const TitleBlockCell &);
	protected:
	virtual void updateText();
	
	// attributes
	public:
	/// Spans before operation
	QHash<TitleBlockCell *, QPair<int, int> > spans_before_;
	/// Pasted cells
	QHash<TitleBlockCell *, TitleBlockCell> pasted_cells_;
	/// Existing cells impacted by the paste operation
	QHash<TitleBlockCell *, TitleBlockCell> erased_cells_;
};
#endif
