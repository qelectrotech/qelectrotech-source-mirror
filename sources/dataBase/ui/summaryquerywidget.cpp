/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "summaryquerywidget.h"
#include "ui_summaryquerywidget.h"

#include "qetapp.h"

#include <QListWidgetItem>

/**
 * @brief SummaryQueryWidget::SummaryQueryWidget
 * @param parent
 */
SummaryQueryWidget::SummaryQueryWidget(QWidget *parent) :
    QWidget(parent),
	ui(new Ui::SummaryQueryWidget)
{
    ui->setupUi(this);
	ui->m_config_gb->setDisabled(true);

	setUpItems();
	fillSavedQuery();
}

/**
 * @brief SummaryQueryWidget::~SummaryQueryWidget
 */
SummaryQueryWidget::~SummaryQueryWidget()
{
	delete ui;
}

/**
 * @brief SummaryQueryWidget::queryStr
 * @return
 */
QString SummaryQueryWidget::queryStr() const
{
		//User define is own query
	if (ui->m_edit_sql_query_cb->isChecked()) {
		return ui->m_user_query_le->text();
	}

		//Made a string list with the colomns (keys) choosen by the user
	QStringList keys = selectedKeys();

	QString select ="SELECT ";

	QString column;
	bool first = true;
	for (auto key: keys) {
		if (first) {
			first = false;
		} else {
			column += ", ";
		}
		column += key;
	}

	QString from = " FROM project_summary_view";

	QString q(select + column + from);
	return q;
}

/**
 * @brief SummaryQueryWidget::setUpItems
 */
void SummaryQueryWidget::setUpItems()
{
	for (auto key : QETApp::diagramInfoKeys())
	{
		if (key == "filename" || key == "display_folio") {
			continue;
		}
		auto item = new QListWidgetItem(QETApp::diagramTranslatedInfoKey(key), ui->m_available_list);
		item->setData(Qt::UserRole, key);
		m_items_list << item;
	}
	auto item = new QListWidgetItem(tr("Position"), ui->m_available_list);
	item->setData(Qt::UserRole, "pos");
	m_items_list << item;
}

void SummaryQueryWidget::fillSavedQuery()
{

}

/**
 * @brief SummaryQueryWidget::updateQueryLine
 */
void SummaryQueryWidget::updateQueryLine() {
	ui->m_user_query_le->setText(queryStr());
}

/**
 * @brief SummaryQueryWidget::selectedKeys
 * @return
 */
QStringList SummaryQueryWidget::selectedKeys() const
{
		//Made a string list with the colomns (keys) choosen by the user
	QStringList keys;
	int row = 0;
	while (auto *item = ui->m_choosen_list->item(row))
	{
		keys.append(item->data(Qt::UserRole).toString());
		++row;
	}

	return keys;
}
void SummaryQueryWidget::on_m_available_list_itemDoubleClicked(QListWidgetItem *item)
{
	Q_UNUSED(item)
	on_m_add_pb_clicked();
}

/**
 * @brief SummaryQueryWidget::on_m_choosen_list_itemDoubleClicked
 * @param item
 */
void SummaryQueryWidget::on_m_choosen_list_itemDoubleClicked(QListWidgetItem *item)
{
	Q_UNUSED(item)
	on_m_remove_pb_clicked();
}

/**
 * @brief SummaryQueryWidget::on_m_up_pb_clicked
 */
void SummaryQueryWidget::on_m_up_pb_clicked()
{
	auto row = ui->m_choosen_list->currentRow();
	if(row <= 0) {
		return;
	}

	auto *item = ui->m_choosen_list->takeItem(row);
	ui->m_choosen_list->insertItem(row-1, item);
	ui->m_choosen_list->setCurrentItem(item);

	updateQueryLine();
}

/**
 * @brief SummaryQueryWidget::on_m_add_pb_clicked
 */
void SummaryQueryWidget::on_m_add_pb_clicked()
{
	if (auto *item = ui->m_available_list->takeItem(ui->m_available_list->currentRow())) {
		ui->m_choosen_list->addItem(item);
	}

	updateQueryLine();
}

/**
 * @brief SummaryQueryWidget::on_m_remove_pb_clicked
 */
void SummaryQueryWidget::on_m_remove_pb_clicked()
{
	if (auto *item = ui->m_choosen_list->takeItem(ui->m_choosen_list->currentRow())) {
		ui->m_available_list->addItem(item);
	}

	updateQueryLine();
}

/**
 * @brief SummaryQueryWidget::on_m_down_pb_clicked
 */
void SummaryQueryWidget::on_m_down_pb_clicked()
{
	auto row = ui->m_choosen_list->currentRow();
	if (row == -1) {
		return;
	}

	auto *item = ui->m_choosen_list->takeItem(row);
	ui->m_choosen_list->insertItem(row+1, item);
	ui->m_choosen_list->setCurrentItem(item);

	updateQueryLine();
}

void SummaryQueryWidget::on_m_edit_sql_query_cb_clicked()
{
	ui->m_user_query_le->setEnabled(ui->m_edit_sql_query_cb->isChecked());
	ui->m_info_widget->setDisabled(ui->m_edit_sql_query_cb->isChecked());

	if (ui->m_edit_sql_query_cb->isChecked() && !m_custom_query.isEmpty())
	{
		ui->m_user_query_le->setText(m_custom_query);
	}
	else if (!ui->m_edit_sql_query_cb->isChecked())
	{
		m_custom_query = ui->m_user_query_le->text();
		updateQueryLine();
	}
}
