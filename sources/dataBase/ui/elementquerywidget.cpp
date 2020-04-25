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

	m_export_info.insert("position", tr("Position"));
	m_export_info.insert("title", tr("Titre du folio"));
	m_export_info.insert("diagram_position", tr("Position du folio"));
	m_export_info.insert("folio", tr("Numéro du folio"));

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
	QString filter_;

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
		
		auto f = FilterFor(key);
		switch (f.first)
		{
			case 0: //No filter
				break;
			case 1: //Not empty
				filter_ += QString(" AND ") += key += " IS NOT NULL"; 
				break;
			case 2: //empty
				filter_ += QString(" AND ") += key += " IS NULL";
				break;
			case 3: // contain
				filter_ += QString(" AND ") += key += QString(" LIKE'%") += f.second += "%'";
				break;
			case 4: // not contain
				filter_ += QString(" AND ") += key += QString(" NOT LIKE'%") += f.second += "%'";
				break;
			case 5: // is equal
				filter_ += QString(" AND ") += key += QString("='") += f.second += "'";
				break;
			case 6: // is not equal
				filter_ += QString(" AND ") += key += QString("!='") += f.second += "'";
				break;
		}
	}

	QString from = " FROM element_nomenclature_view";

	QString where;
	if (ui->m_all_cb->checkState() == Qt::PartiallyChecked)
	{
		where = " WHERE ";
		bool b = false;
		if (ui->m_terminal_cb->isChecked()) {
			if (b) where +=" OR";
			where += " element_type = 'Terminale'";
			b = true;
		}
		if (ui->m_simple_cb->isChecked()) {
			if (b) where +=" OR";
			where += " element_type = 'Simple'";
			b = true;
		}
		if (ui->m_button_cb->isChecked())     {
			if (b) where +=" OR";
			where += " element_sub_type = 'commutator'";
			b = true;
		}
		if (ui->m_coil_cb->isChecked()) {
			if (b) where +=" OR";
			where += " element_sub_type = 'coil'";
			b = true;
		}
		if (ui->m_protection_cb->isChecked()) {
			if (b) where +=" OR";
			where += " element_sub_type = 'protection'";
		}
	}

	QString q(select + column + from + where + filter_ + order_by);
	return q;
}

QStringList ElementQueryWidget::header() const
{
		//Made a string list with the colomns (keys) choosen by the user
	QStringList headers;
	int row = 0;
	while (auto *item = ui->m_choosen_list->item(row))
	{
		headers.append(item->data(Qt::DisplayRole).toString());
		++row;
	}

	return headers;
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
		if (key == "formula")
			continue;

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
 * @brief ElementQueryWidget::FilterFor
 * @param key
 * @return the filter associated to key
 */
QPair<int, QString> ElementQueryWidget::FilterFor(const QString &key) const {
	return m_filter.value(key, qMakePair(0, QString()));
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

void ElementQueryWidget::on_m_plant_textChanged(const QString &arg1) {
	Q_UNUSED(arg1)
	updateQueryLine();
}

void ElementQueryWidget::on_m_location_textChanged(const QString &arg1) {
	Q_UNUSED(arg1)
	updateQueryLine();
}

void ElementQueryWidget::on_m_filter_le_textEdited(const QString &arg1)
{
	if (auto item = ui->m_choosen_list->currentItem())
	{
		auto key =  item->data(Qt::UserRole).toString();
		auto type = ui->m_filter_type_cb->currentIndex();
		auto value = arg1;

		m_filter.insert(key, qMakePair(type, value));
		updateQueryLine();
	}
}

void ElementQueryWidget::on_m_filter_type_cb_activated(int index)
{
	if (auto item = ui->m_choosen_list->currentItem())
	{
		auto key =  item->data(Qt::UserRole).toString();
		auto type = index;
		auto value = ui->m_filter_le->text();

		m_filter.insert(key, qMakePair(type, value));
		ui->m_filter_le->setDisabled(index <= 2);
		updateQueryLine();
	}
}

void ElementQueryWidget::on_m_choosen_list_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
	Q_UNUSED(previous)

	if (!current)
		return;

	auto key = current->data(Qt::UserRole).toString();
	auto p = FilterFor(key);
	ui->m_filter_type_cb->setCurrentIndex(p.first);
	ui->m_filter_le->setText(p.second);
	ui->m_filter_le->setEnabled(p.first>=3);
}

void ElementQueryWidget::on_m_var_list_itemDoubleClicked(QListWidgetItem *item) {
	Q_UNUSED(item)
	on_m_add_pb_clicked();
}

void ElementQueryWidget::on_m_choosen_list_itemDoubleClicked(QListWidgetItem *item) {
	Q_UNUSED(item)
	on_m_remove_pb_clicked();
}
