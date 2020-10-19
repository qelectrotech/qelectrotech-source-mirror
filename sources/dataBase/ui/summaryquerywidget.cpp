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
	@brief SummaryQueryWidget::SummaryQueryWidget
	@param parent
*/
SummaryQueryWidget::SummaryQueryWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SummaryQueryWidget)
{
	ui->setupUi(this);

	setUpItems();
	fillSavedQuery();

	connect(ui->m_config_gb, &ConfigSaveLoaderWidget::saveClicked, this, &SummaryQueryWidget::saveConfig);
	connect(ui->m_config_gb, &ConfigSaveLoaderWidget::loadClicked, this, &SummaryQueryWidget::loadConfig);
}

/**
	@brief SummaryQueryWidget::~SummaryQueryWidget
*/
SummaryQueryWidget::~SummaryQueryWidget()
{
	delete ui;
}

/**
	@brief SummaryQueryWidget::queryStr
	@return The current query string
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
	@brief SummaryQueryWidget::setQuery
	@param query
	set the current query to query.
	If it's possible, rebuild the state of the widget from the query
*/
void SummaryQueryWidget::setQuery(const QString &query)
{
	if (query.startsWith("SELECT"))
	{
		reset();
		ui->m_user_query_le->setText(query);

		QString select = query;
		select.remove(0,7); //remove SELECT
		select.truncate(select.indexOf("FROM")); //Truncate at FROM
		select.replace(" ",""); //Remove white

			//Get the select -> the item in the right list
		QStringList split = select.split(",");
		for (auto str : split)
		{
			for (auto item : m_items_list)
			{
				if (item->data(Qt::UserRole).toString() == str) {
					ui->m_available_list->takeItem(ui->m_available_list->row(item));
					ui->m_choosen_list->addItem(item);
					continue;
				}
			}
		}
	}
}

/**
	@brief SummaryQueryWidget::setUpItems
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

/**
	@brief SummaryQueryWidget::fillSavedQuery
	Fill the combo box of the saved query
*/
void SummaryQueryWidget::fillSavedQuery()
{
	QFile file(QETApp::configDir() + "/summary.json");
	if (file.open(QFile::ReadOnly))
	{
		QJsonDocument jsd(QJsonDocument::fromJson(file.readAll()));
		QJsonObject jso = jsd.object();

		for (auto it = jso.begin() ; it != jso.end() ; ++it) {
			ui->m_config_gb->addItem(it.key());
		}
	}
}

/**
	@brief SummaryQueryWidget::updateQueryLine
*/
void SummaryQueryWidget::updateQueryLine()
{
	ui->m_user_query_le->setText(queryStr());
}

/**
	@brief SummaryQueryWidget::selectedKeys
	@return
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

/**
	@brief SummaryQueryWidget::on_m_available_list_itemDoubleClicked
	@param item
*/
void SummaryQueryWidget::on_m_available_list_itemDoubleClicked(QListWidgetItem *item)
{
	Q_UNUSED(item)
	on_m_add_pb_clicked();
}

/**
	@brief SummaryQueryWidget::on_m_choosen_list_itemDoubleClicked
	@param item
*/
void SummaryQueryWidget::on_m_choosen_list_itemDoubleClicked(QListWidgetItem *item)
{
	Q_UNUSED(item)
	on_m_remove_pb_clicked();
}

/**
	@brief SummaryQueryWidget::on_m_up_pb_clicked
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
	@brief SummaryQueryWidget::on_m_add_pb_clicked
*/
void SummaryQueryWidget::on_m_add_pb_clicked()
{
	if (auto *item = ui->m_available_list->takeItem(ui->m_available_list->currentRow())) {
		ui->m_choosen_list->addItem(item);
	}

	updateQueryLine();
}

/**
	@brief SummaryQueryWidget::on_m_remove_pb_clicked
*/
void SummaryQueryWidget::on_m_remove_pb_clicked()
{
	if (auto *item = ui->m_choosen_list->takeItem(ui->m_choosen_list->currentRow())) {
		ui->m_available_list->addItem(item);
	}

	updateQueryLine();
}

/**
	@brief SummaryQueryWidget::on_m_down_pb_clicked
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

/**
	@brief SummaryQueryWidget::on_m_edit_sql_query_cb_clicked
*/
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

/**
	@brief SummaryQueryWidget::reset
	Clear this widget aka set to initial state
*/
void SummaryQueryWidget::reset()
{
		//Ugly hack to force to remove all selected infos
	while (auto item = ui->m_choosen_list->takeItem(0)) {
		ui->m_available_list->addItem(item);
	}
	ui->m_user_query_le->clear();
}

/**
	@brief SummaryQueryWidget::saveConfig
*/
void SummaryQueryWidget::saveConfig()
{
	QFile file_(QETApp::configDir() + "/summary.json");

	if (file_.open(QFile::ReadWrite))
	{
		QJsonDocument doc_(QJsonDocument::fromJson(file_.readAll()));
		QJsonObject root_object;

		if (!doc_.isEmpty())
		{
			root_object = doc_.object();
			if (root_object.contains(ui->m_config_gb->text())) {
				root_object.remove(ui->m_config_gb->text());
			}
		}

		QJsonObject object_;
		object_.insert("query", queryStr());
		root_object[ui->m_config_gb->text()] = object_;

		doc_.setObject(root_object);
		file_.resize(0);
		file_.write(doc_.toJson());
	}
}

/**
	@brief SummaryQueryWidget::loadConfig
*/
void SummaryQueryWidget::loadConfig()
{
	auto name = ui->m_config_gb->selectedText();
	if (name.isEmpty()) {
		return;
	}

	QFile file_(QETApp::configDir() + "/summary.json");
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
