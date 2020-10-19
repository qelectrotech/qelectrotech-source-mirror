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
#include "addtabledialog.h"
#include "ui_addtabledialog.h"
#include "elementquerywidget.h"
#include "marginseditdialog.h"
#include "qetutils.h"

#include <QFontDialog>

/**
	@brief AddTableDialog::AddTableDialog
	@param content_widget : the widget to display in the "content" tab.
	This dialog take ownership of content_widget.
	@param parent : parent widget.
*/
AddTableDialog::AddTableDialog(QWidget *content_widget, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::AddTableDialog)
{
	ui->setupUi(this);
	ui->m_header_font_pb->setText(m_header_font.family());
	ui->m_table_font_pb->setText(m_table_font.family());
	m_content_widget = content_widget;
	content_widget->setParent(this);
	ui->m_tab->addTab(content_widget, tr("Contenu"));
	fillSavedQuery();
	
	connect(ui->m_config_gb, &ConfigSaveLoaderWidget::saveClicked, this, &AddTableDialog::saveConfig);
	connect(ui->m_config_gb, &ConfigSaveLoaderWidget::loadClicked, this, &AddTableDialog::loadConfig);
}

/**
	@brief AddTableDialog::~AddNomenclatureDialog
*/
AddTableDialog::~AddTableDialog()
{
	delete ui;
}

/**
	@brief AddTableDialog::setQueryWidget
	Not implemented yet
	@param widget
*/
void AddTableDialog::setQueryWidget(QWidget *widget) {
	Q_UNUSED(widget)
}

/**
	@brief AddTableDialog::adjustTableToFolio
	@return
*/
bool AddTableDialog::adjustTableToFolio() const
{
	return ui->m_adjust_table_size_cb->isChecked();
}

/**
	@brief AddTableDialog::addNewTableToNewDiagram
	@return
*/
bool AddTableDialog::addNewTableToNewDiagram() const
{
	return ui->m_add_table_and_folio->isChecked();
}

/**
	@brief AddTableDialog::tableName
	@return
*/
QString AddTableDialog::tableName() const
{
	return ui->m_table_name_le->text();
}

/**
	@brief AddTableDialog::headerMargins
	@return
*/
QMargins AddTableDialog::headerMargins() const
{
	return  m_header_margins;
}

/**
	@brief AddTableDialog::headerAlignment
	@return
*/
Qt::Alignment AddTableDialog::headerAlignment() const
{
	switch (ui->m_header_alignment_cb->currentIndex()) {
		case 0 :
			return  Qt::AlignLeft;
		case 1:
			return Qt::AlignHCenter;
		default:
			return Qt::AlignRight;
	}
}

/**
	@brief AddTableDialog::headerFont
	@return
*/
QFont AddTableDialog::headerFont() const
{
	return m_header_font;
}

/**
	@brief AddTableDialog::tableMargins
	@return
*/
QMargins AddTableDialog::tableMargins() const
{
	return m_table_margins;
}

/**
	@brief AddTableDialog::tableAlignment
	@return
*/
Qt::Alignment AddTableDialog::tableAlignment() const
{
	switch (ui->m_table_alignment_cb->currentIndex()) {
		case 0 :
			return  Qt::AlignLeft;
		case 1:
			return Qt::AlignHCenter;
		default:
			return Qt::AlignRight;
	}
}

/**
	@brief AddTableDialog::tableFont
	@return
*/
QFont AddTableDialog::tableFont() const
{
	return m_table_font;
}

QWidget *AddTableDialog::contentWidget() const
{
	return m_content_widget;
}

void AddTableDialog::on_m_header_font_pb_clicked()
{
	bool b;
	auto font = QFontDialog::getFont(&b, m_header_font, this, tr("Sélectionner la police des en tête du tableau"));
	if (b) {
		m_header_font = font;
		ui->m_header_font_pb->setText(font.family());
	}

}

void AddTableDialog::on_m_table_font_pb_clicked()
{
	bool b;
	auto font = QFontDialog::getFont(&b, m_table_font, this, tr("Sélectionner la police des cellules du tableau"));
	if (b) {
		m_table_font = font;
		ui->m_table_font_pb->setText(font.family());
	}
}

void AddTableDialog::on_m_edit_header_margins_pb_clicked()
{
	bool accept;
	auto margins_ = MarginsEditDialog::getMargins(m_header_margins, &accept, this);
	if (accept)
		m_header_margins = margins_;
}

void AddTableDialog::on_m_table_margins_pb_clicked()
{
	bool accept;
	auto margins_ = MarginsEditDialog::getMargins(m_table_margins, &accept, this);
	if (accept)
		m_table_margins = margins_;
}

void AddTableDialog::saveConfig()
{
	QFile file_(QETApp::configDir() + "/graphics_table.json");
	
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
		
		QJsonObject header_object;
		header_object.insert("margins", QETUtils::marginsToString(this->headerMargins()));
		auto me = QMetaEnum::fromType<Qt::Alignment>();
		header_object.insert("alignment", me.valueToKey(int(this->headerAlignment())));
		header_object.insert("font", this->headerFont().toString());

		QJsonObject table_object;
		table_object.insert("margins", QETUtils::marginsToString(this->tableMargins()));
		table_object.insert("alignment", me.valueToKey(int(this->tableAlignment())));
		table_object.insert("font", this->tableFont().toString());

		QJsonObject config_object;
		config_object.insert("header", header_object);
		config_object.insert("table", table_object);

		root_object.insert(ui->m_config_gb->text(), config_object);

		doc_.setObject(root_object);
		file_.resize(0);
		file_.write(doc_.toJson());
	}
}

void AddTableDialog::loadConfig()
{
	auto name = ui->m_config_gb->selectedText();
	if (name.isEmpty()) {
		return;
	}

	QFile file_(QETApp::configDir() + "/graphics_table.json");
	if (!file_.open(QFile::ReadOnly)) {
		return;
	}

	QJsonDocument doc_(QJsonDocument::fromJson(file_.readAll()));
	QJsonObject root_object(doc_.object());
	auto config_object = root_object.value(name).toObject();
	if (config_object.isEmpty()) {
		return;
	}

	auto me = QMetaEnum::fromType<Qt::Alignment>();

		//Header
	auto header_object = config_object.value("header").toObject();
	m_header_margins = QETUtils::marginsFromString(header_object.value("margins").toString());
	switch (me.keyToValue(header_object.value("alignment").toString().toStdString().data())) {
		case Qt::AlignLeft :
			ui->m_header_alignment_cb->setCurrentIndex(0);
			break;
		case Qt::AlignCenter :
			ui->m_header_alignment_cb->setCurrentIndex(1);
			break;
		default:
			ui->m_header_alignment_cb->setCurrentIndex(2);
	}
	m_header_font.fromString(header_object.value("font").toString());
	ui->m_header_font_pb->setText(m_header_font.family());

		//Table
	auto table_object = config_object.value("table").toObject();
	m_table_margins = QETUtils::marginsFromString(table_object.value("margins").toString());
	switch (me.keyToValue(table_object.value("alignment").toString().toStdString().data())) {
		case Qt::AlignLeft :
			ui->m_table_alignment_cb->setCurrentIndex(0);
			break;
		case Qt::AlignCenter :
			ui->m_table_alignment_cb->setCurrentIndex(1);
			break;
		default:
			ui->m_table_alignment_cb->setCurrentIndex(2);
	}
	m_table_font.fromString(table_object.value("font").toString());
	ui->m_table_font_pb->setText(m_table_font.family());

}

void AddTableDialog::fillSavedQuery()
{
	QFile file(QETApp::configDir() + "/graphics_table.json");
	if (file.open(QFile::ReadOnly))
	{
		QJsonDocument jsd(QJsonDocument::fromJson(file.readAll()));
		QJsonObject jso = jsd.object();

		for (auto it = jso.begin() ; it != jso.end() ; ++it) {
			ui->m_config_gb->addItem(it.key());
		}
	}
}
