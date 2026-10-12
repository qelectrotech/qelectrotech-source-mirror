/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "cablequerywidget.h"

#include "cablelistrows.h"

#include "../qetapp.h"
#include "../ui/configsaveloaderwidget.h"

#include <QFile>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

#include <QStringBuilder>

/**
	@brief CableQueryWidget::CableQueryWidget
	@param parent
*/
CableQueryWidget::CableQueryWidget(QWidget *parent) :
	QWidget(parent)
{
	auto *layout = new QVBoxLayout(this);

	auto *lists = new QHBoxLayout;

	auto *left = new QVBoxLayout;
	left->addWidget(new QLabel(tr("Available columns"), this));
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
	auto *add_pb = make_button(QStyle::SP_ArrowRight, tr("Add the selected column"));
	auto *remove_pb = make_button(QStyle::SP_ArrowLeft, tr("Remove the selected column"));
	auto *up_pb = make_button(QStyle::SP_ArrowUp, tr("Move the selected column up"));
	auto *down_pb = make_button(QStyle::SP_ArrowDown, tr("Move the selected column down"));
	buttons->addWidget(add_pb);
	buttons->addWidget(remove_pb);
	buttons->addWidget(up_pb);
	buttons->addWidget(down_pb);
	buttons->addStretch();

	auto *right = new QVBoxLayout;
	right->addWidget(new QLabel(tr("Table columns"), this));
	m_choosen_list = new QListWidget(this);
	right->addWidget(m_choosen_list);

	lists->addLayout(left);
	lists->addLayout(buttons);
	lists->addLayout(right);
	layout->addLayout(lists);

	m_config_gb = new ConfigSaveLoaderWidget(this);
	layout->addWidget(m_config_gb);

	setMinimumSize(QSize(520, 420));

	setUpItems();
	fillSavedQuery();

	connect(m_config_gb, &ConfigSaveLoaderWidget::saveClicked, this, &CableQueryWidget::saveConfig);
	connect(m_config_gb, &ConfigSaveLoaderWidget::loadClicked, this, &CableQueryWidget::loadConfig);

	connect(add_pb, &QPushButton::clicked, this, &CableQueryWidget::addField);
	connect(remove_pb, &QPushButton::clicked, this, &CableQueryWidget::removeField);
	connect(up_pb, &QPushButton::clicked, this, &CableQueryWidget::moveFieldUp);
	connect(down_pb, &QPushButton::clicked, this, &CableQueryWidget::moveFieldDown);

	connect(m_available_list, &QListWidget::itemDoubleClicked,
			this, &CableQueryWidget::on_m_available_list_itemDoubleClicked);
	connect(m_choosen_list, &QListWidget::itemDoubleClicked,
			this, &CableQueryWidget::on_m_choosen_list_itemDoubleClicked);
}

/**
	@brief CableQueryWidget::~CableQueryWidget
*/
CableQueryWidget::~CableQueryWidget()
{
}

/**
	@brief CableQueryWidget::selectedKeys
	@return the chosen columns, in order
*/
QStringList CableQueryWidget::selectedKeys() const
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
	@brief CableQueryWidget::setFields
	Put exactly these columns in the right list, in that order. Keys
	this list does not know are ignored, which is what keeps a file
	coming from a newer program readable here.
	@param fields
*/
void CableQueryWidget::setFields(const QStringList &fields)
{
		//Ugly hack to force to remove all chosen columns, the same way
		//SummaryQueryWidget::reset() does it
	while (auto *item = m_choosen_list->takeItem(0)) {
		m_available_list->addItem(item);
	}

	for (const QString &key : fields)
	{
		for (int row = 0; row < m_available_list->count(); ++row)
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
	@brief CableQueryWidget::setUpItems
	Fill the available list with every column the list offers and
	leave them there: a fresh dialog opens with its entries on the
	left, the same way the material list dialog does. What a table or
	an export starts with when nothing has been picked is the default
	set of columns, applied where the table or the file is made.
*/
void CableQueryWidget::setUpItems()
{
	for (const CableList::Column &column : CableList::allColumns())
	{
		auto *item = new QListWidgetItem(column.label, m_available_list);
		item->setData(Qt::UserRole, column.key);
	}
}

/**
	@brief CableQueryWidget::fillSavedQuery
	Fill the combo box of the saved configurations
*/
void CableQueryWidget::fillSavedQuery()
{
	QFile file(QETApp::configDir() % QStringLiteral("/cable_list.json"));
	if (file.open(QFile::ReadOnly))
	{
		QJsonDocument jsd(QJsonDocument::fromJson(file.readAll()));
		QJsonObject jso = jsd.object();

		for (auto it = jso.begin() ; it != jso.end() ; ++it) {
			m_config_gb->addItem(it.key());
		}
	}
}

/**
	@brief CableQueryWidget::on_m_available_list_itemDoubleClicked
	@param item
*/
void CableQueryWidget::on_m_available_list_itemDoubleClicked(QListWidgetItem *item)
{
	Q_UNUSED(item)
	addField();
}

/**
	@brief CableQueryWidget::on_m_choosen_list_itemDoubleClicked
	@param item
*/
void CableQueryWidget::on_m_choosen_list_itemDoubleClicked(QListWidgetItem *item)
{
	Q_UNUSED(item)
	removeField();
}

/**
	@brief CableQueryWidget::addField
*/
void CableQueryWidget::addField()
{
	if (auto *item = m_available_list->takeItem(m_available_list->currentRow())) {
		m_choosen_list->addItem(item);
	}
}

/**
	@brief CableQueryWidget::removeField
*/
void CableQueryWidget::removeField()
{
	if (auto *item = m_choosen_list->takeItem(m_choosen_list->currentRow())) {
		m_available_list->addItem(item);
	}
}

/**
	@brief CableQueryWidget::moveFieldUp
*/
void CableQueryWidget::moveFieldUp()
{
	auto row = m_choosen_list->currentRow();
	if (row <= 0) {
		return;
	}

	auto *item = m_choosen_list->takeItem(row);
	m_choosen_list->insertItem(row - 1, item);
	m_choosen_list->setCurrentItem(item);
}

/**
	@brief CableQueryWidget::moveFieldDown
*/
void CableQueryWidget::moveFieldDown()
{
	auto row = m_choosen_list->currentRow();
	if (row == -1) {
		return;
	}

	auto *item = m_choosen_list->takeItem(row);
	m_choosen_list->insertItem(row + 1, item);
	m_choosen_list->setCurrentItem(item);
}

/**
	@brief CableQueryWidget::saveConfig
*/
void CableQueryWidget::saveConfig()
{
	QFile file_(QETApp::configDir() % QStringLiteral("/cable_list.json"));

	if (file_.open(QFile::ReadWrite))
	{
		QJsonDocument doc_(QJsonDocument::fromJson(file_.readAll()));
		QJsonObject root_object;

		if (!doc_.isEmpty())
		{
			root_object = doc_.object();
			if (root_object.contains(m_config_gb->text())) {
				root_object.remove(m_config_gb->text());
			}
		}

		QJsonObject object_;
		object_.insert(QStringLiteral("fields"), selectedKeys().join(QLatin1Char(',')));
		root_object[m_config_gb->text()] = object_;

		doc_.setObject(root_object);
		file_.resize(0);
		file_.write(doc_.toJson());
	}
}

/**
	@brief CableQueryWidget::loadConfig
*/
void CableQueryWidget::loadConfig()
{
	auto name = m_config_gb->selectedText();
	if (name.isEmpty()) {
		return;
	}

	QFile file_(QETApp::configDir() % QStringLiteral("/cable_list.json"));
	if (!file_.open(QFile::ReadOnly)) {
		return;
	}

	QJsonDocument doc_(QJsonDocument::fromJson(file_.readAll()));
	QJsonObject object_ = doc_.object();

	auto value = object_.value(name);
	if (!value.isObject()) {
		return;
	}

	auto fields = value.toObject().value(QStringLiteral("fields"));
	if (fields.isString()) {
		setFields(fields.toString().split(QLatin1Char(','), Qt::SkipEmptyParts));
	}
}
