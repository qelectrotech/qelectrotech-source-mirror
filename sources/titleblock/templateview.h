/*
	Copyright 2006-2013 The QElectroTech Team
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
#ifndef TITLEBLOCK_SLASH_TEMPLATE_VIEW_H
#define TITLEBLOCK_SLASH_TEMPLATE_VIEW_H
#include <QGraphicsView>
#include "titleblocktemplate.h"
class HelperCell;
class SplittedHelperCell;
class TitleBlockTemplateCommand;
class TitleBlockTemplateCellsSet;

/**
	This QGraphicsView subclass is used in the title block template editor to
	offer a graphical preview of the template being edited, but also to handle
	cell selection and various actions.
*/
class TitleBlockTemplateView : public QGraphicsView {
	Q_OBJECT
	
	// constructors, destructor
	public:
	TitleBlockTemplateView(QWidget * = 0);
	TitleBlockTemplateView(QGraphicsScene *, QWidget * = 0);
	virtual ~TitleBlockTemplateView();
	private:
	TitleBlockTemplateView(const TitleBlockTemplateView &);
	
	// methods and slots
	public:
	TitleBlockTemplate *titleBlockTemplate() const;
	virtual QList<TitleBlockCell *> selectedCells() const;
	virtual TitleBlockTemplateCellsSet selectedCellsSet() const;
	virtual TitleBlockTemplateCellsSet cells(const QRectF &) const;
	virtual void analyzeSelectedCells(bool *, bool *, int *);
	virtual QSizeF templateSize() const;
	virtual qreal templateWidth() const;
	virtual qreal templateHeight() const;
	
	public slots:
	void setTitleBlockTemplate(TitleBlockTemplate *);
	void selectionChanged();
	void zoomIn();
	void zoomOut();
	void zoomFit();
	void zoomReset();
	QList<TitleBlockCell *> cut();
	QList<TitleBlockCell *> copy();
	bool mayPaste();
	QList<TitleBlockCell> pastedCells();
	void paste();
	void addColumnAtEnd();
	void addRowAtEnd();
	void addColumnBefore();
	void addRowBefore();
	void addColumnAfter();
	void addRowAfter();
	void editColumn(HelperCell * = 0);
	void editRow(HelperCell * = 0);
	void deleteColumn();
	void deleteRow();
	void mergeSelectedCells();
	void splitSelectedCell();
	void refresh();
	void changePreviewWidth();
	void setPreviewWidth(int);
	void updateLayout();
	void rowsDimensionsChanged();
	void columnsDimensionsChanged();
	void updateDisplayedMinMaxWidth();
	void setReadOnly(bool);
	
	protected slots:
	virtual void applyColumnsWidths(bool = true);
	virtual void applyRowsHeights(bool = true);
	virtual void updateRowsHelperCells();
	virtual void updateColumnsHelperCells();
	
	protected:
	virtual void drawBackground(QPainter *, const QRectF &);
	virtual void addCells();
	virtual void loadTemplate(TitleBlockTemplate *);
	virtual void init();
	virtual void wheelEvent(QWheelEvent *);
	virtual qreal zoomFactor() const;
	virtual void fillWithEmptyCells();
	virtual bool event(QEvent *);
	virtual void normalizeCells(QList<TitleBlockCell> &, int x = 0, int y = 0) const;
	
	signals:
	void selectedCellsChanged(QList<TitleBlockCell *>);
	void gridModificationRequested(TitleBlockTemplateCommand *);
	void previewWidthChanged(int, int);
	
	private:
	QList<QAction *> rowsActions() const;
	QList<QAction *> columnsActions() const;
	void updateTotalWidthLabel();
	void requestGridModification(TitleBlockTemplateCommand *);
	int lastContextMenuCellIndex() const;
	int indexOf(QGraphicsLayoutItem *);
	void removeItem(QGraphicsLayoutItem *);
	TitleBlockTemplateCellsSet makeCellsSetFromGraphicsItems(const QList<QGraphicsItem *> &) const;
	QString makePrettyToolTip(const QString &);
	
	private slots:
	void updateLastContextMenuCell(HelperCell *);
	void adjustSceneRect();
	
	// attributes
	private:
	TitleBlockTemplate *tbtemplate_;
	QGraphicsGridLayout *tbgrid_;
	QGraphicsWidget *form_;
	int preview_width_;
	SplittedHelperCell *total_width_helper_cell_;
	HelperCell *extra_cells_width_helper_cell_;
	QAction *add_column_before_, *add_row_before_;
	QAction *add_column_after_, *add_row_after_;
	QAction *edit_column_dim_, *edit_row_dim_;
	QAction *delete_column_, *delete_row_;
	QAction *change_preview_width_;
	HelperCell *last_context_menu_cell_;
	int apply_columns_widths_count_;
	int apply_rows_heights_count_;
	bool first_activation_;                ///< Boolean used to detect the first display of this widget
	bool read_only_;                       ///< Boolean stating whether this view allows template edition
};
#endif
