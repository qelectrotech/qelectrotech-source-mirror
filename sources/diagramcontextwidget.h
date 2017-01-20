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
#ifndef DIAGRAMCONTEXTWIDGET_H
#define DIAGRAMCONTEXTWIDGET_H
#include <QWidget>
#include "diagramcontext.h"
class QLabel;
class QTableWidget;
/**
	This class provides a table which enables end users to edit the key/value
	pairs of a DiagamContext.
*/
class DiagramContextWidget : public QWidget {
	Q_OBJECT
	// Constructor, destructor
	public:
	DiagramContextWidget(QWidget *parent = 0);
	virtual ~DiagramContextWidget();
	private:
	DiagramContextWidget(const DiagramContextWidget &);
	
	// methods
	public:
	bool isReadOnly();
	DiagramContext context() const;
	void setContext(const DiagramContext &);
	int nameLessRowsCount() const;
	void setSorting(bool enable, int column, Qt::SortOrder order );
	
	signals:
	
	public slots:
	void setReadOnly(bool);
	void clear();
	int highlightNonAcceptableKeys();
	void refreshFormatLabel();
	
	private slots:
	void checkTableRows();
	
	private:
	void initWidgets();
	void initLayout();
	
	// attributes
	private:
	QLabel *format_label; ///< label used to detail keys format
	QTableWidget *table_; ///< table used to enter key/value pairs
};

#endif
