/*
	Copyright 2006-2018 The QElectroTech Team
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
#include "ui_diagramcontextwidget.h"

DiagramContextWidget::DiagramContextWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::DiagramContextWidget)
{
	ui->setupUi(this);
	connect(ui->m_table, SIGNAL(itemChanged(QTableWidgetItem *)), this, SLOT(checkTableRows()));
}

DiagramContextWidget::~DiagramContextWidget()
{
	delete ui;
}

/**
 * @brief DiagramContextWidget::context
 * @return The diagram context object edited by this widget
 */
DiagramContext DiagramContextWidget::context() const
{
	DiagramContext context;
	
	for (int i = 0 ; i < ui->m_table-> rowCount() ; ++ i)
	{
		QTableWidgetItem *qtwi_name  = ui->m_table-> item(i, 0);
		QTableWidgetItem *qtwi_value = ui->m_table-> item(i, 1);
		if (!qtwi_name || !qtwi_value) {
			continue;
		}
		
		QString key = qtwi_name -> text();
		if (key.isEmpty()) {
			continue;
		}
		
		QString value = qtwi_value -> text();
		context.addValue(key, value);
	}
	
	return(context);
}

/**
 * @brief DiagramContextWidget::setContext
 * Load the content from @context into this widget
 * @param context
 */
void DiagramContextWidget::setContext (const DiagramContext &context)
{
	clear();
	
	int i = 0;
	for (QString key : context.keys(DiagramContext::Alphabetical))
	{
		ui->m_table->setItem(i, 0, new QTableWidgetItem(key));
		ui->m_table->setItem(i, 1, new QTableWidgetItem(context[key].toString()));
		++ i;
	}
	
	checkTableRows();
}

/**
 * @brief DiagramContextWidget::nameLessRowsCount
 * @return The count of name-less rows in the table
 */
int DiagramContextWidget::nameLessRowsCount() const
{
	int name_less_rows_count = 0;
	for (int i = 0 ; i < ui->m_table->rowCount() ; ++ i)
	{
		QTableWidgetItem *qtwi_name  = ui->m_table->item(i, 0);
		if (qtwi_name && qtwi_name -> text().isEmpty()) {
			++ name_less_rows_count;
		}
	}
	
	return(name_less_rows_count);
}

/**
 * @brief DiagramContextWidget::clear
 * Clear any values entered within this widget
 */
void DiagramContextWidget::clear()
{
	ui->m_table->clearContents();
	for (int i = 1 ; i < ui->m_table->rowCount() ; ++ i) {
		ui->m_table->removeRow(i);
	}
	
	refreshFormatLabel();
}

/**
 * @brief DiagramContextWidget::highlightNonAcceptableKeys
 * Highlight keys that would not be accepted by a DiagramContext object.
 * @return the number of highlighted keys.
 */
int DiagramContextWidget::highlightNonAcceptableKeys()
{
	static QRegExp re(DiagramContext::validKeyRegExp());
	
	QBrush fg_brush = ui->m_table->palette().brush(QPalette::WindowText);
	
	int invalid_keys = 0;
	for (int i = 0 ; i < ui->m_table->rowCount() ; ++ i)
	{
		QTableWidgetItem *qtwi_name  = ui->m_table->item(i, 0);
		if (!qtwi_name) {
			continue;
		}
		
		bool highlight = false;
		if (!qtwi_name -> text().isEmpty())
		{
			if (!re.exactMatch(qtwi_name -> text()))
			{
				highlight = true;
				++ invalid_keys;
			}
		}
		qtwi_name -> setForeground(highlight ? Qt::red : fg_brush);
	}
	
	return(invalid_keys);
}

/**
 * @brief DiagramContextWidget::refreshFormatLabel
 * Sets the text describing the acceptable format for keys when adding extra
 * key/value pairs.
 */
void DiagramContextWidget::refreshFormatLabel()
{
	QString format_text = tr(
		"Les noms ne peuvent contenir que des lettres minuscules, des "
		"chiffres et des tirets."
	);
	
	if (highlightNonAcceptableKeys()) {
		format_text = QString("<span style=\"color: red;\">%1</span>").arg(format_text);
	}
	ui->m_label->setText(format_text);
}

/**
 * @brief DiagramContextWidget::checkTableRows
 * Adds a row in the additional fields table if needed
 */
void DiagramContextWidget::checkTableRows()
{
	refreshFormatLabel();
	if (!nameLessRowsCount())
	{
		int new_idx = ui->m_table->rowCount();
		ui->m_table->setRowCount(new_idx + 1);
		ui->m_table->setItem(new_idx, 0, new QTableWidgetItem(""));
		ui->m_table->setItem(new_idx, 1, new QTableWidgetItem(""));
	}
}
