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
#include "diagramcontextwidget.h"
#include <QHeaderView>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

/**
	Constructor
	@param parent Parent QWidget
*/
DiagramContextWidget::DiagramContextWidget(QWidget *parent) :
	QWidget(parent)
{
	initWidgets();
	initLayout();
}

/**
	Destructor
*/
DiagramContextWidget::~DiagramContextWidget() {
}

/**
	@return Whether this widget is read-only.
*/
bool DiagramContextWidget::isReadOnly() {
	return(table_ -> isEnabled());
}

/**
	@return the DiagramContext object edited by this widget.
*/
DiagramContext DiagramContextWidget::context() const {
	DiagramContext context;
	for (int i = 0 ; i < table_ -> rowCount() ; ++ i) {
		QTableWidgetItem *qtwi_name  = table_ -> item(i, 0);
		QTableWidgetItem *qtwi_value = table_ -> item(i, 1);
		if (!qtwi_name || !qtwi_value) continue;
		
		QString key = qtwi_name -> text();
		if (key.isEmpty()) continue;
		
		QString value = qtwi_value -> text();
		context.addValue(key, value);
	}
	return(context);
}

/**
	Load the content from \a context into this widget.
*/
void DiagramContextWidget::setContext(const DiagramContext &context) {
	clear();
	int i = 0;
	foreach (QString key, context.keys(DiagramContext::Alphabetical)) {
		table_ -> setItem(i, 0, new QTableWidgetItem(key));
		table_ -> setItem(i, 1, new QTableWidgetItem(context[key].toString()));
		++ i;
	}
	checkTableRows();
}

/**
	@return The count of name-less rows in the table.
*/
int DiagramContextWidget::nameLessRowsCount() const {
	int name_less_rows_count = 0;
	for (int i = 0 ; i < table_ -> rowCount() ; ++ i) {
		QTableWidgetItem *qtwi_name  = table_ -> item(i, 0);
		if (qtwi_name && qtwi_name -> text().isEmpty()) ++ name_less_rows_count;
	}
	return(name_less_rows_count);
}

/**
	@param ro Whether this widget should be read-only.
*/
void DiagramContextWidget::setReadOnly(bool ro) {
	table_ -> setEnabled(!ro);
}

/**
	Clear any value entered within this widget.
*/
void DiagramContextWidget::clear() {
	table_ -> clearContents();
	for (int i = 1 ; i < table_ -> rowCount() ; ++ i) {
		table_ -> removeRow(i);
	}
	refreshFormatLabel();
}


/**
	Highlight keys that would not be accepted by a DiagramContext object.
	@return the number of highlighted keys.
*/
int DiagramContextWidget::highlightNonAcceptableKeys() {
	static QRegExp re(DiagramContext::validKeyRegExp());
	
	QBrush fg_brush = table_ -> palette().brush(QPalette::WindowText);
	
	int invalid_keys = 0;
	for (int i = 0 ; i < table_ -> rowCount() ; ++ i) {
		QTableWidgetItem *qtwi_name  = table_ -> item(i, 0);
		if (!qtwi_name) continue;
		bool highlight = false;
		if (!qtwi_name -> text().isEmpty()) {
			if (!re.exactMatch(qtwi_name -> text())) {
				highlight = true;
				++ invalid_keys;
			}
		}
		qtwi_name -> setForeground(highlight ? Qt::red : fg_brush);
	}
	return(invalid_keys);
}

/**
	Sets the text describing the acceptable format for keys when adding extra
	key/value pairs.
*/
void DiagramContextWidget::refreshFormatLabel() {
	QString format_text = tr(
		"Les noms ne peuvent contenir que des lettres minuscules, des "
		"chiffres et des tirets."
	);
	
	if (highlightNonAcceptableKeys()) {
		format_text = QString("<span style=\"color: red;\">%1</span>").arg(format_text);
	}
	format_label -> setText(format_text);
}

/**
	Adds a row in the additional fields table if needed.
*/
void DiagramContextWidget::checkTableRows() {
	refreshFormatLabel();
	if (!nameLessRowsCount()) {
		int new_idx = table_ -> rowCount();
		table_ -> setRowCount(new_idx + 1);
		table_ -> setItem(new_idx, 0, new QTableWidgetItem(""));
		table_ -> setItem(new_idx, 1, new QTableWidgetItem(""));
	}
}

/**
	Initialize child widgets.
*/
void DiagramContextWidget::initWidgets() {
	format_label = new QLabel();
	format_label -> setWordWrap(true);
	format_label -> setAlignment(Qt::AlignJustify);
	
	table_ = new QTableWidget(1, 2);
	table_ -> setSelectionMode(QAbstractItemView::SingleSelection);
	table_ -> setHorizontalHeaderLabels(QStringList() << tr("Nom", "table header") << tr("Valeur", "table header"));
	table_ -> horizontalHeader() -> setStretchLastSection(true);
	
	connect(table_, SIGNAL(itemChanged(QTableWidgetItem *)), this, SLOT(checkTableRows()));
}

/**
	Initialize the layout of this widget.
*/
void DiagramContextWidget::initLayout() {
	QVBoxLayout *vlayout0 = new QVBoxLayout();
	vlayout0 -> setContentsMargins(0, 0, 0, 0);
	vlayout0 -> addWidget(format_label);
	vlayout0 -> addWidget(table_);
	setLayout(vlayout0);
}

/**
	Set Sorting on the table.
*/
void DiagramContextWidget::setSorting(bool enable, int column, Qt::SortOrder order ) {
	table_ -> setSortingEnabled(enable);
	table_ -> sortByColumn(column, order);
}

