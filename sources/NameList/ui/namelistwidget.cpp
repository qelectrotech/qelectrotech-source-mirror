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
#include "namelistwidget.h"
#include "ui_namelistwidget.h"

#include <QPushButton>
#include <QClipboard>

NameListWidget::NameListWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::NameListWidget)
{
	ui->setupUi(this);
	ui->m_clipboard_cb->setHidden(true);
	connect(ui->m_add_line_pb, &QPushButton::clicked, this, &NameListWidget::addLine);
}

NameListWidget::~NameListWidget()
{
	delete ui;
}

/**
 * @brief NameListWidget::addLine
 * Add a new line to the name list widget
 */
void NameListWidget::addLine()
{
	clean();
	if (m_read_only) {
		return;
	}
	QTreeWidgetItem *qtwi = new QTreeWidgetItem();
	qtwi->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	ui->m_tree->addTopLevelItem(qtwi);
 	ui->m_tree->setCurrentItem(qtwi);
	ui->m_tree->editItem(qtwi);
}

/**
 * @brief NameListWidget::setNames
 * Set the current names of this dialog from @name_list
 * @param name_list
 */
void NameListWidget::setNames(const NamesList &name_list)
{
	for (QString lang : name_list.langs())
	{
		QString value = name_list[lang];
		QStringList values;
		values << lang << value;
		QTreeWidgetItem *qtwi = new QTreeWidgetItem(values);
		if (!m_read_only) {
			qtwi->setFlags(Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		}
		ui->m_tree->addTopLevelItem(qtwi);
		ui->m_tree->sortItems(0, Qt::AscendingOrder);
	}
}

/**
 * @brief NameListWidget::names
 * @return the current name list edited by this dialog
 */
NamesList NameListWidget::names() const
{
	NamesList nl_;
	
	int names_count = ui->m_tree->topLevelItemCount();
	for (int i = 0 ; i < names_count ; ++ i)
	{
		QString lang  = ui->m_tree->topLevelItem(i)->text(0);
		QString value = ui->m_tree->topLevelItem(i)->text(1);
		if (lang != "" && value != "") {
			nl_.addName(lang, value);
		}
	}
	
	return nl_;
}

/**
 * @brief NameListWidget::setReadOnly
 * Set this dialog to read only or not.
 * @param ro
 */
void NameListWidget::setReadOnly(bool ro)
{
	m_read_only = ro;
	
	int names_count = ui->m_tree->topLevelItemCount() - 1;
	for (int i = names_count ; i >= 0 ; -- i)
	{
		Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		if (!m_read_only) {
			flags |= Qt::ItemIsEditable;
		}
		ui->m_tree->topLevelItem(i)->setFlags(flags);
	}
	ui->m_add_line_pb->setEnabled(!ro);
}

/**
 * @brief NameListWidget::isEmpty
 * @return true if empty.
 * An empty dialog, is a dialog without any edited lang. 
 */
bool NameListWidget::isEmpty() const {
	return names().isEmpty();
}

void NameListWidget::setClipboardValue(QHash<QString, QString> value)
{
	if (value.isEmpty()) {
		ui->m_clipboard_cb->setHidden(true);
	}
	else
	{
		ui->m_clipboard_cb->setVisible(true);
		
		QStringList list = value.keys();
		list.sort();
		for (QString key : list) {
			ui->m_clipboard_cb->addItem(key, value.value(key));
		}
	}
}

/**
 * @brief NameListWidget::clean
 * Clean the lists of names by removing the emtpy lines
 */
void NameListWidget::clean()
{
	int names_count = ui->m_tree->topLevelItemCount() - 1;
	for (int i = names_count ; i >= 0 ; -- i)
	{
		if (ui->m_tree->topLevelItem(i)->text(0).isEmpty() &&
			ui->m_tree->topLevelItem(i)->text(1).isEmpty())
		{
			ui->m_tree->takeTopLevelItem(i);
		}
	}
}

void NameListWidget::on_m_clipboard_cb_activated(int index)
{
	Q_UNUSED(index);
	
	QClipboard *clipboard = QApplication::clipboard();
	clipboard->setText(ui->m_clipboard_cb->currentData().toString());
	ui->m_clipboard_cb->setCurrentIndex(0);
}
