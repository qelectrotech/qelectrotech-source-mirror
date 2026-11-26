/*
	Copyright 2006-2026 The QElectroTech Team
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
#include "plcquerywidget.h"

#include <qetapp.h>
#include <qetinformation.h>
#include <ui/configsaveloaderwidget.h>

#include <QCheckBox>
#include <QFile>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include <utility>

/**
	@brief PlcQueryWidget::PlcQueryWidget
	@param parent
*/
PlcQueryWidget::PlcQueryWidget(QWidget *parent) :
	QWidget(parent)
{
	auto *layout = new QVBoxLayout(this);

	auto *lists = new QHBoxLayout;

	auto *left = new QVBoxLayout;
	left->addWidget(new QLabel(tr("Available information"), this));
	m_available_list = new QListWidget(this);
	left->addWidget(m_available_list);

	auto *buttons = new QVBoxLayout;
	buttons->addStretch();
	auto make_button = [this](QStyle::StandardPixmap icon, const QString &tip) {
		auto *button = new QPushButton(this);
		button->setIcon(style()->standardIcon(icon));
		button->setToolTip(tip);
		return button;
	};
	auto *add_pb = make_button(QStyle::SP_ArrowRight, tr("Add selection"));
	auto *remove_pb = make_button(QStyle::SP_ArrowLeft, tr("Delete selection"));
	auto *up_pb = make_button(QStyle::SP_ArrowUp, tr("go up the selection"));
	auto *down_pb = make_button(QStyle::SP_ArrowDown, tr("go down the selection"));
	buttons->addWidget(add_pb);
	buttons->addWidget(remove_pb);
	buttons->addWidget(up_pb);
	buttons->addWidget(down_pb);
	buttons->addStretch();

	auto *right = new QVBoxLayout;
	right->addWidget(new QLabel(tr("Information to display"), this));
	m_choosen_list = new QListWidget(this);
	right->addWidget(m_choosen_list);

	lists->addLayout(left);
	lists->addLayout(buttons);
	lists->addLayout(right);
	layout->addLayout(lists);

	m_custom_cb = new QCheckBox(tr("Custom SQL Query"), this);
	layout->addWidget(m_custom_cb);
	m_query_line = new QLineEdit(this);
	m_query_line->setObjectName(QStringLiteral("m_sql_query"));
	m_query_line->setEnabled(false);
	layout->addWidget(m_query_line);

	m_config_gb = new ConfigSaveLoaderWidget(this);
	layout->addWidget(m_config_gb);

	setMinimumSize(QSize(560, 460));

	setUpItems();
	fillSavedQuery();

	connect(m_config_gb, &ConfigSaveLoaderWidget::saveClicked, this, &PlcQueryWidget::saveConfig);
	connect(m_config_gb, &ConfigSaveLoaderWidget::loadClicked, this, &PlcQueryWidget::loadConfig);

	connect(add_pb, &QPushButton::clicked, this, &PlcQueryWidget::addField);
	connect(remove_pb, &QPushButton::clicked, this, &PlcQueryWidget::removeField);
	connect(up_pb, &QPushButton::clicked, this, &PlcQueryWidget::moveFieldUp);
	connect(down_pb, &QPushButton::clicked, this, &PlcQueryWidget::moveFieldDown);
	connect(m_custom_cb, &QCheckBox::toggled, this, &PlcQueryWidget::customQueryToggled);

	connect(m_available_list, &QListWidget::itemDoubleClicked,
			this, &PlcQueryWidget::onAvailableItemDoubleClicked);
	connect(m_choosen_list, &QListWidget::itemDoubleClicked,
			this, &PlcQueryWidget::onChosenItemDoubleClicked);
}

PlcQueryWidget::~PlcQueryWidget()
{
}

/**
	@brief PlcQueryWidget::queryStr
	@return The current query string
*/
QString PlcQueryWidget::queryStr() const
{
	if (m_custom_cb->isChecked()) {
		return m_query_line->text();
	}

	const QStringList keys = selectedKeys();
	return QStringLiteral("SELECT %1 FROM qet_plc_list "
						  "ORDER BY structure_id, plc_uuid, entry")
			.arg(keys.join(QStringLiteral(", ")));
}

/**
	@brief PlcQueryWidget::groupQueryStr
	@return The query reading the master of each row of queryStr(), one
	value per row in the same order, so that a page can be broken at the
	end of a master. Empty for a custom query, whose rows no query of
	ours can follow.
*/
QString PlcQueryWidget::groupQueryStr() const
{
	if (m_custom_cb->isChecked()) {
		return QString();
	}
	return QStringLiteral("SELECT plc_uuid FROM qet_plc_list "
						  "ORDER BY structure_id, plc_uuid, entry");
}

/**
	@brief PlcQueryWidget::setQuery
	Set the current query to @p query.
	If it's possible, rebuild the state of the widget from the query
*/
void PlcQueryWidget::setQuery(const QString &query)
{
	if (!query.startsWith(QStringLiteral("SELECT"))) {
		return;
	}
	reset();
	m_query_line->setText(query);

	QString select = query;
	select.remove(0, 7); //remove SELECT
	const int from = select.indexOf(QStringLiteral("FROM"));
	if (from >= 0) {
		select.truncate(from); //Truncate at FROM
		select.replace(QStringLiteral(" "), QString()); //Remove white space

			//Get the select -> the item in the right list
		const QStringList split = select.split(QLatin1Char(','));
		for (const QString &str : split)
		{
			for (auto *item : std::as_const(m_items_list))
			{
				if (item->data(Qt::UserRole).toString() == str) {
					m_available_list->takeItem(m_available_list->row(item));
					m_choosen_list->addItem(item);
					break;
				}
			}
		}
	}

		//A query the widget would not build itself is the person's own
		//one: a join, a subquery, or a FROM it cannot be taken apart.
		//Accepting the dialog unmodified must not replace it.
	const bool custom_query = from < 0 || query != queryStr();
	m_custom_query = custom_query ? query : QString();
	m_custom_cb->setChecked(custom_query);
	m_query_line->setEnabled(custom_query);
	m_available_list->setDisabled(custom_query);
	m_choosen_list->setDisabled(custom_query);
}

/**
	@brief PlcQueryWidget::setUpItems
	The columns qet_plc_list offers, all of them on the left: what a
	fresh dialog opens with, as the material list dialog does. Each name
	is the one the table's header is translated with, so a column reads
	the same here and on the sheet.
*/
void PlcQueryWidget::setUpItems()
{
	const QStringList keys {
		QStringLiteral("label"),
		QStringLiteral("plant"),
		QStringLiteral("location"),
		QStringLiteral("manufacturer"),
		QStringLiteral("manufacturer_reference"),
		QStringLiteral("designation"),
		QStringLiteral("model"),
		QStringLiteral("description"),
		QStringLiteral("supplier"),
		QStringLiteral("plc_type"),
		QStringLiteral("plc_address"),
		QStringLiteral("plc_function"),
		QStringLiteral("plc_comment"),
		QStringLiteral("plc_crossref"),
		QStringLiteral("position"),
		QStringLiteral("diagram_position"),
		QStringLiteral("folio")
	};
	for (const QString &key : keys)
	{
			//Names no information key carries, the way the table's
			//header names them (ProjectDBModel::setHeaderString())
		QString label;
		if (key == QLatin1String("position")) label = tr("Position");
		else if (key == QLatin1String("diagram_position")) label = tr("Sheet position");
		else label = QETInformation::translatedInfoKey(key);
		if (label.isEmpty()) label = key;

		auto *item = new QListWidgetItem(label, m_available_list);
		item->setData(Qt::UserRole, key);
		m_items_list << item;
	}
}

/**
	@brief PlcQueryWidget::selectedKeys
	@return the chosen columns, in order
*/
QStringList PlcQueryWidget::selectedKeys() const
{
	QStringList keys;
	int row = 0;
	while (auto *item = m_choosen_list->item(row))
	{
		keys.append(item->data(Qt::UserRole).toString());
		++row;
	}
	return keys;
}

/**
	@brief PlcQueryWidget::setFields
	Put exactly these columns in the right list, in that order. Keys
	this list does not know are ignored, which is what keeps a file
	coming from a newer program readable here.
	@param fields
*/
void PlcQueryWidget::setFields(const QStringList &fields)
{
	reset();

	for (const QString &key : fields)
	{
		for (int row = 0 ; row < m_available_list->count() ; ++row)
		{
			QListWidgetItem *item = m_available_list->item(row);
			if (item->data(Qt::UserRole).toString() == key) {
				m_available_list->takeItem(row);
				m_choosen_list->addItem(item);
				break;
			}
		}
	}
}

/**
	@brief PlcQueryWidget::fillSavedQuery
	Fill the combo box of the saved configurations
*/
void PlcQueryWidget::fillSavedQuery()
{
	QFile file(QETApp::configDir() + QStringLiteral("/plc_list.json"));
	if (file.open(QFile::ReadOnly))
	{
		const QJsonDocument jsd(QJsonDocument::fromJson(file.readAll()));
		const QJsonObject jso = jsd.object();

		for (auto it = jso.begin() ; it != jso.end() ; ++it) {
			m_config_gb->addItem(it.key());
		}
	}
}

/**
	@brief PlcQueryWidget::updateQueryLine
*/
void PlcQueryWidget::updateQueryLine()
{
	if (m_custom_cb->isChecked()) {
		return;
	}
	m_query_line->setText(queryStr());
}

/**
	@brief PlcQueryWidget::reset
	Clear this widget aka set to initial state
*/
void PlcQueryWidget::reset()
{
	while (auto *item = m_choosen_list->takeItem(0)) {
		m_available_list->addItem(item);
	}
	m_query_line->clear();
}

void PlcQueryWidget::onAvailableItemDoubleClicked(QListWidgetItem *item)
{
	Q_UNUSED(item)
	addField();
}

void PlcQueryWidget::onChosenItemDoubleClicked(QListWidgetItem *item)
{
	Q_UNUSED(item)
	removeField();
}

void PlcQueryWidget::addField()
{
	if (auto *item = m_available_list->takeItem(m_available_list->currentRow())) {
		m_choosen_list->addItem(item);
	}
	updateQueryLine();
}

void PlcQueryWidget::removeField()
{
	if (auto *item = m_choosen_list->takeItem(m_choosen_list->currentRow())) {
		m_available_list->addItem(item);
	}
	updateQueryLine();
}

void PlcQueryWidget::moveFieldUp()
{
	auto row = m_choosen_list->currentRow();
	if (row <= 0) {
		return;
	}

	auto *item = m_choosen_list->takeItem(row);
	m_choosen_list->insertItem(row - 1, item);
	m_choosen_list->setCurrentItem(item);
	updateQueryLine();
}

void PlcQueryWidget::moveFieldDown()
{
	auto row = m_choosen_list->currentRow();
	if (row == -1 || row == m_choosen_list->count() - 1) {
		return;
	}

	auto *item = m_choosen_list->takeItem(row);
	m_choosen_list->insertItem(row + 1, item);
	m_choosen_list->setCurrentItem(item);
	updateQueryLine();
}

/**
	@brief PlcQueryWidget::customQueryToggled
	Take the query line over, or give the built one back
*/
void PlcQueryWidget::customQueryToggled(bool checked)
{
	m_query_line->setEnabled(checked);
	m_available_list->setDisabled(checked);
	m_choosen_list->setDisabled(checked);

	if (checked) {
		if (!m_custom_query.isEmpty()) {
			m_query_line->setText(m_custom_query);
		}
	} else {
			//Only what was typed by hand is worth keeping: the built
			//query is built again whenever it is needed
		if (m_query_line->text() != queryStr()) {
			m_custom_query = m_query_line->text();
		}
		updateQueryLine();
	}
}

/**
	@brief PlcQueryWidget::saveConfig
*/
void PlcQueryWidget::saveConfig()
{
	QFile file(QETApp::configDir() + QStringLiteral("/plc_list.json"));

	if (file.open(QFile::ReadWrite))
	{
		const QJsonDocument jsd(QJsonDocument::fromJson(file.readAll()));
		QJsonObject root_object;

		if (!jsd.isEmpty())
		{
			root_object = jsd.object();
			if (root_object.contains(m_config_gb->text())) {
				root_object.remove(m_config_gb->text());
			}
		}

		QJsonObject object_;
		object_.insert(QStringLiteral("fields"), selectedKeys().join(QLatin1Char(',')));
		root_object[m_config_gb->text()] = object_;

		const QJsonDocument doc(object_);
		file.resize(0);
		file.write(doc.toJson());
	}
}

/**
	@brief PlcQueryWidget::loadConfig
*/
void PlcQueryWidget::loadConfig()
{
	const auto name = m_config_gb->selectedText();
	if (name.isEmpty()) {
		return;
	}

	QFile file(QETApp::configDir() + QStringLiteral("/plc_list.json"));
	if (!file.open(QFile::ReadOnly)) {
		return;
	}

	const QJsonDocument jsd(QJsonDocument::fromJson(file.readAll()));
	const QJsonObject object_ = jsd.object();

	const auto value = object_.value(name);
	if (!value.isObject()) {
		return;
	}

	const auto fields = value.toObject().value(QStringLiteral("fields"));
	if (fields.isString()) {
		setFields(fields.toString().split(QLatin1Char(','), Qt::SkipEmptyParts));
	}
	updateQueryLine();
}
