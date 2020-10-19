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

#include <QRegularExpression>

/**
	@brief ElementQueryWidget::ElementQueryWidget
	@param parent
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
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)	// ### Qt 6: remove
	connect(&m_button_group, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [this](int id)
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 5.15 or later")
#endif
	connect(&m_button_group, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::idClicked), [this](int id)
#endif
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
			for (int i=1 ; i<6 ; ++i) {
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
	fillSavedQuery();
}

/**
	@brief ElementQueryWidget::~ElementQueryWidget
*/
ElementQueryWidget::~ElementQueryWidget()
{
	delete ui;
}

/**
	@brief ElementQueryWidget::setQuery
	@param query
	Set the current query to query.
	If it's possible, rebuild the state of the widget from the query
*/
void ElementQueryWidget::setQuery(const QString &query)
{
	if (query.startsWith("SELECT"))
	{
		reset();

		ui->m_sql_query->setText(query);

		QString select = query;
		select.remove(0,7); //Remove SELECT
		select.truncate(select.indexOf("FROM")); //Truncate at FROM
		select.replace(" ", ""); //Remove white space

			//Get the select -> the item in the right list
		QStringList split = select.split(",");
		for (auto str : split)
		{
			for (auto item : m_items_list)
			{
				if (item->data(Qt::UserRole).toString() == str) {
					ui->m_var_list->takeItem(ui->m_var_list->row(item));
					ui->m_choosen_list->addItem(item);
					continue;
				}
			}
		}

			//There is not filter return now.
		if (!query.contains("WHERE"))
		{
			ui->m_all_cb->setCheckState(Qt::Checked);
			for (auto b : m_button_group.buttons())
				b->setChecked(true);
			return;
		}

			//Get the filter
		auto where = query;
		where.remove(0, where.indexOf("WHERE") + 6);
		where.truncate(where.indexOf("ORDER BY"));

			//Element type filter
		if (where.contains("element_sub_type") || where.contains("element_type"))
		{
			int c=0;
			ui->m_simple_cb->setChecked    (where.contains("Simple")     ? true : false);
			if (ui->m_simple_cb->isChecked()) {
				++c;
				where.remove("element_type = 'Simple' ");
			}
			ui->m_terminal_cb->setChecked  (where.contains("Terminale")  ? true : false);
			if (ui->m_terminal_cb->isChecked()) {
				++c;
				where.remove("element_type = 'Terminale'");
			}
			ui->m_coil_cb->setChecked      (where.contains("coil")       ? true : false);
			if (ui->m_coil_cb->isChecked()) {
				++c;
				where.remove("element_sub_type = 'coil' ");
			}
			ui->m_button_cb->setChecked    (where.contains("commutator") ? true : false);
			if (ui->m_button_cb->isChecked()) {
				++c;
				where.remove("element_sub_type = 'commutator' ");
			}
			ui->m_protection_cb->setChecked(where.contains("protection") ? true : false);
			if (ui->m_protection_cb) {
				++c;
				where.remove("element_sub_type = 'protection'");
			}

			if (c == 5) {
				ui->m_all_cb->setCheckState(Qt::Checked);
			} else if (c > 0) {
				ui->m_all_cb->setCheckState(Qt::PartiallyChecked);
			}

			where.remove("OR");
		}
		else // There is not "element_sub_type" or "element_type" that mean every element are selected
		{
			ui->m_all_cb->setCheckState(Qt::Checked);
			for (auto b : m_button_group.buttons())
				b->setChecked(true);
		}


			//Filter for selected data
		QStringList strl;
		for (auto item : m_items_list) {
			strl.append(item->data(Qt::UserRole).toString());
		}

		QString beginning_rx;
		beginning_rx.append(QString("^(").append(strl.join("|")));
		beginning_rx.append(")");

		QRegularExpression rx_is_not_null(beginning_rx + " IS NOT NULL$");
		QRegularExpression rx_is_null (beginning_rx + " IS NULL$");
		QRegularExpression rx_like (beginning_rx + QString(" LIKE'%(.+)%'$"));
		QRegularExpression rx_not_like (beginning_rx + QString(" NOT LIKE'%(.+)%'$"));
		QRegularExpression rx_equal (beginning_rx + QString("='(.+)'$"));
		QRegularExpression rx_not_equal (beginning_rx + QString("!='(.+)'$"));


		QStringList split_where;

			//Remove the white space at begin and end of each string
		for (auto str : where.split("AND "))
		{
			if (str.startsWith(" "))
				str.remove(0,1);
			if (str.endsWith(" "))
				str.remove(str.size()-1, 1);

			split_where.append(str);
		}


		QRegularExpressionMatch rxm;
		for (auto str : split_where)
		{
			rxm = rx_is_not_null.match(str);
			if (rxm.hasMatch()) {
				m_filter.insert(rxm.captured(1), qMakePair(1, QString()));
				continue;
			}

			rxm = rx_is_null.match(str);
			if (rxm.hasMatch()) {
				m_filter.insert(rxm.captured(1), qMakePair(2, QString()));
				continue;
			}

			rxm = rx_like.match(str);
			if (rxm.hasMatch()) {
				m_filter.insert(rxm.captured(1), qMakePair(3, rxm.captured(2)));
				continue;
			}

			rxm = rx_not_like.match(str);
			if (rxm.hasMatch()) {
				m_filter.insert(rxm.captured(1), qMakePair(4, rxm.captured(2)));
				continue;
			}

			rxm = rx_equal.match(str);
			if (rxm.hasMatch()) {
				m_filter.insert(rxm.captured(1), qMakePair(5, rxm.captured(2)));
				continue;
			}

			rxm = rx_not_equal.match(str);
			if (rxm.hasMatch()) {
				m_filter.insert(rxm.captured(1), qMakePair(6, rxm.captured(2)));
				continue;
			}

		}
	}
}

/**
	@brief ElementQueryWidget::queryStr
	@return The current query
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
	where = " WHERE (";
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
	where.append(")");

	if (where == " WHERE ()") {
		where.clear();
	}

	if (where.isEmpty() && !filter_.isEmpty()) {
		filter_.remove(0, 4); //Remove the first " AND" of filter.
		filter_.prepend( " WHERE");
	}

	QString q(select + column + m_count + from + where + filter_ + m_group_by + order_by);
	return q;
}

/**
	@brief ElementQueryWidget::setGroupBy
	Add the query instruction GROUP BY.
	@param text : the text of the GROUP BY instruction:
	ex : if text = designation,
	the query will contain "GROUP BY designation"
	@param set :
	true by default -> GROUP BY will be used.
	false -> GROUP BY will be not used
*/
void ElementQueryWidget::setGroupBy(QString text, bool set)
{
	if (set) {
		m_group_by = QString(" GROUP BY ") + text;
	} else {
		m_group_by.clear();
	}
	updateQueryLine();
}

/**
	@brief ElementQueryWidget::setCount
	Add the query instruction COUNT.
	Unlike setGroupBy, you have to write the entire sentance.
	ex : text = "COUNT(*) AS designation_qty".
	the query will contain what you write.
	@param text : the count instruction
	@param set :
	true by default -> count will be used.
	false -> count will be not used.
*/
void ElementQueryWidget::setCount(QString text, bool set)
{
	if (set) {
		m_count = QString(", " + text + " ");
	} else {
		m_count.clear();
	}
	updateQueryLine();
}

/**
	@brief ElementQueryWidget::updateQueryLine
*/
void ElementQueryWidget::updateQueryLine()
{
	ui->m_sql_query->setText(queryStr());
}

/**
	@brief ElementQueryWidget::selectedKeys
	@return the current keys of selected infos to be exported
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

/**
	@brief ElementQueryWidget::setUpItems
*/
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
	@brief ElementQueryWidget::FilterFor
	@param key
	@return the filter associated to key
*/
QPair<int, QString> ElementQueryWidget::FilterFor(const QString &key) const
{
	return m_filter.value(key, qMakePair(0, QString()));
}

/**
	@brief ElementQueryWidget::fillSavedQuery
	Fill the combobox of saved queries
*/
void ElementQueryWidget::fillSavedQuery()
{
	QFile file(QETApp::configDir() + "/nomenclature.json");
	if (file.open(QFile::ReadOnly))
	{
		QJsonDocument jsd(QJsonDocument::fromJson(file.readAll()));
		QJsonObject jso = jsd.object();

		for (auto it = jso.begin() ; it != jso.end() ; ++it) {
			ui->m_conf_cb->addItem(it.key());
		}
	}
}

/**
	@brief ElementQueryWidget::on_m_up_pb_clicked
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
	@brief ElementQueryWidget::on_m_add_pb_clicked
*/
void ElementQueryWidget::on_m_add_pb_clicked()
{
	if (auto *item = ui->m_var_list->takeItem(ui->m_var_list->currentRow())) {
		ui->m_choosen_list->addItem(item);
	}

	updateQueryLine();
}

/**
	@brief ElementQueryWidget::on_m_remove_pb_clicked
*/
void ElementQueryWidget::on_m_remove_pb_clicked()
{
	if (auto *item = ui->m_choosen_list->takeItem(ui->m_choosen_list->currentRow())) {
		ui->m_var_list->addItem(item);
	}

	updateQueryLine();
}

/**
	@brief ElementQueryWidget::on_m_down_pb_clicked
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
	@brief ElementQueryWidget::on_m_edit_sql_query_cb_clicked
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

/**
	@brief ElementQueryWidget::on_m_filter_le_textEdited
	@param arg1
*/
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

/**
	@brief ElementQueryWidget::on_m_filter_type_cb_activated
	@param index
*/
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

/**
	@brief ElementQueryWidget::on_m_load_pb_clicked
	Load a query from nomenclature.json file
*/
void ElementQueryWidget::on_m_load_pb_clicked()
{
	auto name = ui->m_conf_cb->currentText();
	if (name.isEmpty()) {
		return;
	}

	QFile file_(QETApp::configDir() + "/nomenclature.json");
	if (!file_.open(QFile::ReadOnly)) {
		return;
	}

	QJsonDocument doc_(QJsonDocument::fromJson(file_.readAll()));
	QJsonObject object_ = doc_.object();

	auto value = object_.value(name);
	if (!value.isObject()) {
		return;
	}

	auto value_object = value.toObject();
	if (value_object.value("query").isString()) {
		setQuery(value_object.value("query").toString());
	}
}

/**
	@brief ElementQueryWidget::on_m_save_current_conf_pb_clicked
	Save the actual query to nomenclature.json file
*/
void ElementQueryWidget::on_m_save_current_conf_pb_clicked()
{
	QFile file_(QETApp::configDir() + "/nomenclature.json");

	if (file_.open(QFile::ReadWrite))
	{
		QJsonDocument doc_(QJsonDocument::fromJson(file_.readAll()));
		QJsonObject root_object;

		if (!doc_.isEmpty())
		{
			root_object = doc_.object();
			if (root_object.contains(ui->m_save_name_le->text())) {
				root_object.remove(ui->m_save_name_le->text());
			}
		}

		QJsonObject object_;
		object_.insert("query", queryStr());
		root_object[ui->m_save_name_le->text()] = object_;

		doc_.setObject(root_object);
		file_.resize(0);
		file_.write(doc_.toJson());
	}
}

void ElementQueryWidget::on_m_save_name_le_textChanged(const QString &arg1) {
	ui->m_save_current_conf_pb->setDisabled(arg1.isEmpty());
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

/**
	@brief ElementQueryWidget::reset
	Clear this widget aka set to initial state
*/
void ElementQueryWidget::reset()
{
		//Ugly hack to force to remove all selected infos
	while (auto item = ui->m_choosen_list->takeItem(0)) {
		ui->m_var_list->addItem(item);
	}

	ui->m_all_cb->setChecked(true);
	ui->m_sql_query->clear();
	m_filter.clear();
}

