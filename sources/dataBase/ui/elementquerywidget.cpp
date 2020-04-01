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
#include "elementquerywidget.h"
#include "ui_elementquerywidget.h"
#include "qetapp.h"

/**
 * @brief ElementQueryWidget::ElementQueryWidget
 * @param parent
 */
ElementQueryWidget::ElementQueryWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ElementQueryWidget)
{
    ui->setupUi(this);

	m_export_info.insert("pos", tr("Position"));
//	m_export_info.insert("folio_title", tr("Titre du folio"));
//	m_export_info.insert("folio_pos", tr("Position de folio"));
//	m_export_info.insert("folio_num", tr("Numéro de folio"));
//	m_export_info.insert("designation_qty", tr("Quantité (Numéro d'article)"));

	m_button_group.setExclusive(false);
	m_button_group.addButton(ui->m_all_cb, 0);
	m_button_group.addButton(ui->m_terminal_cb, 1);
	m_button_group.addButton(ui->m_simple_cb, 2);
	m_button_group.addButton(ui->m_button_cb, 3);
	m_button_group.addButton(ui->m_coil_cb, 4);
	m_button_group.addButton(ui->m_protection_cb, 5);
	connect(&m_button_group, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [this](int id)
	{
		auto check_box = static_cast<QCheckBox *>(m_button_group.button(0));
		if (id == 0)
		{
			switch (check_box->checkState()) {
				case Qt::Checked :
					for (auto button : m_button_group.buttons()) {
						button->setChecked(true);
					}
					break;
				case Qt::Unchecked :
					for (auto button : m_button_group.buttons()) {
						button->setChecked(false);
					}
					break;
				default: break;
			}
		}
		else
		{
			int checked = 0;
			for (int i=1 ; i<5 ; ++i) {
				if (m_button_group.button(i)->isChecked()) {++checked;}
			}

			switch (checked)
			{
				case 0 :
					check_box->setCheckState(Qt::Unchecked);
					break;
				case 5:
					check_box->setCheckState(Qt::Checked);
					break;
				default:
					check_box->setCheckState(Qt::PartiallyChecked);
					break;
			}
		}

		updateQueryLine();
	});

	setUpItems();
}

/**
 * @brief ElementQueryWidget::~ElementQueryWidget
 */
ElementQueryWidget::~ElementQueryWidget() {
	delete ui;
}

/**
 * @brief ElementQueryWidget::queryStr
 * @return The current query
 */
QString ElementQueryWidget::queryStr() const
{
		//User define is own query
	if (ui->m_edit_sql_query_cb->isChecked()) {
		return ui->m_sql_query->text();
	}
		//Made a string list with the colomns (keys) choosen by the user
	QStringList keys = selectedKeys();

	QString select ="SELECT ";
	QString order_by = " ORDER BY ";

	QString column;
	bool first = true;
	for (auto key: keys) {
		if (first) {
			first = false;
		} else {
			column += ", ";
			order_by += ", ";
		}
		column += key;
		order_by += key;
	}

	QString from = " FROM element_info";
	QString where;
	if (ui->m_all_cb->checkState() == Qt::PartiallyChecked)
	{
		if (ui->m_terminal_cb->isChecked()) {
			where = " WHERE element_type = 'Terminale'";
		}
		if (ui->m_simple_cb->isChecked()) {
			auto str = where.isEmpty() ? " WHERE element_type = 'Simple'" : " AND element_type = 'Simple'";
			where += str;
		}
		if (ui->m_button_cb->isChecked()) {
			auto str = where.isEmpty() ? " WHERE element_subtype = 'commutator'" : " AND element_subtype = 'commutator'";
			where += str;
		}
		if (ui->m_coil_cb->isChecked()) {
			auto str = where.isEmpty() ? " WHERE element_subtype = 'coil'" : " AND element_subtype = 'coil'";
			where += str;
		}
		if (ui->m_protection_cb->isChecked()) {
			auto str = where.isEmpty() ? " WHERE element_subtype = 'protection'" : " AND element_subtype = 'protection'";
			where += str;
		}
	}

	QString q(select + column + from + where + order_by);
	return q;
}

/**
 * @brief ElementQueryWidget::updateQueryLine
 */
void ElementQueryWidget::updateQueryLine() {
	ui->m_sql_query->setText(queryStr());
}

/**
 * @brief ElementQueryWidget::selectedKeys
 * @return the current keys of selected infos to be exported
 */
QStringList ElementQueryWidget::selectedKeys() const
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

void ElementQueryWidget::setUpItems()
{
	for(QString key : QETApp::elementInfoKeys())
	{
		auto item = new QListWidgetItem(QETApp::elementTranslatedInfoKey(key), ui->m_var_list);
		item->setData(Qt::UserRole, key);
		m_items_list << item;
	}

	for (auto key : m_export_info.keys())
	{
		auto item = new QListWidgetItem(m_export_info.value(key), ui->m_var_list);
		item->setData(Qt::UserRole, key);
		m_items_list << item;
	}
}

/**
 * @brief ElementQueryWidget::on_m_up_pb_clicked
 */
void ElementQueryWidget::on_m_up_pb_clicked()
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
 * @brief ElementQueryWidget::on_m_add_pb_clicked
 */
void ElementQueryWidget::on_m_add_pb_clicked()
{
	if (auto *item = ui->m_var_list->takeItem(ui->m_var_list->currentRow())) {
		ui->m_choosen_list->addItem(item);
	}

	updateQueryLine();
}

/**
 * @brief ElementQueryWidget::on_m_remove_pb_clicked
 */
void ElementQueryWidget::on_m_remove_pb_clicked()
{
	if (auto *item = ui->m_choosen_list->takeItem(ui->m_choosen_list->currentRow())) {
		ui->m_var_list->addItem(item);
	}

	updateQueryLine();
}

/**
 * @brief ElementQueryWidget::on_m_down_pb_clicked
 */
void ElementQueryWidget::on_m_down_pb_clicked()
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

/**
 * @brief ElementQueryWidget::on_m_edit_sql_query_cb_clicked
 */
void ElementQueryWidget::on_m_edit_sql_query_cb_clicked()
{
	ui->m_sql_query->setEnabled(ui->m_edit_sql_query_cb->isChecked());
	ui->m_info_widget->setDisabled(ui->m_edit_sql_query_cb->isChecked());
	ui->m_parametre_widget->setDisabled(ui->m_edit_sql_query_cb->isChecked());

	if (ui->m_edit_sql_query_cb->isChecked() && !m_custom_query.isEmpty())
	{
		ui->m_sql_query->setText(m_custom_query);
	}
	else if (!ui->m_edit_sql_query_cb->isChecked())
	{
		m_custom_query = ui->m_sql_query->text();
		updateQueryLine();
	}
}
