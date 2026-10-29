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
#include "addtabledialog.h"

#include "../../dataBase/ui/elementquerywidget.h"
#include "../../ui/marginseditdialog.h"
#include "../../utils/qetutils.h"
#include "ui_addtabledialog.h"

#include "../../qet.h"
#include <QFontDialog>
#include "../../ui/tabledefaults18.h"
#include "../../dataBase/ui/summaryquerywidget.h"
#include <QMessageBox>

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
	QET::trackDialogGeometry(this);
	ui->m_header_font_pb->setText(m_header_font.family());
	ui->m_table_font_pb->setText(m_table_font.family());
	m_content_widget = content_widget;
	content_widget->setParent(this);
	ui->m_tab->addTab(content_widget, tr("Content"));
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
	@brief AddTableDialog::setAdjustTableToFolio
	@param set
*/
void AddTableDialog::setAdjustTableToFolio(bool set)
{
	ui->m_adjust_table_size_cb->setChecked(set);
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
	@brief AddTableDialog::setAddNewTableToNewDiagram
	@param set
*/
void AddTableDialog::setAddNewTableToNewDiagram(bool set)
{
	ui->m_add_table_and_folio->setChecked(set);
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
	@brief AddTableDialog::setTableName
	Set the name field directly, so a caller that builds this dialog to
	read from (never shows or execs it -- the scripting API's addTable())
	does not need a name typed by a user who was never there to type one.
	@param name
*/
void AddTableDialog::setTableName(const QString &name)
{
	ui->m_table_name_le->setText(name);
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

void AddTableDialog::setHeaderFont(const QFont &font)
{
    m_header_font=font;ui->m_header_font_pb->setText(font.family());
}
void AddTableDialog::setTableFont(const QFont &font)
{
    m_table_font=font;ui->m_table_font_pb->setText(font.family());
}
void AddTableDialog::setHeaderAlignment(Qt::Alignment alignment)
{
    ui->m_header_alignment_cb->setCurrentIndex(alignment&Qt::AlignRight?2:alignment&Qt::AlignHCenter?1:0);
}
void AddTableDialog::setTableAlignment(Qt::Alignment alignment)
{
    ui->m_table_alignment_cb->setCurrentIndex(alignment&Qt::AlignRight?2:alignment&Qt::AlignHCenter?1:0);
}
void AddTableDialog::setHeaderMargins(const QMargins &margins) {m_header_margins=margins;}
void AddTableDialog::setTableMargins(const QMargins &margins) {m_table_margins=margins;}

QWidget *AddTableDialog::contentWidget() const
{
	return m_content_widget;
}

void AddTableDialog::on_m_header_font_pb_clicked()
{
	bool b;
	auto font = QFontDialog::getFont(&b, m_header_font, this, tr("Select the font at the top of the table"));
	if (b) {
		m_header_font = font;
		ui->m_header_font_pb->setText(font.family());
	}

}

void AddTableDialog::on_m_table_font_pb_clicked()
{
	bool b;
	auto font = QFontDialog::getFont(&b, m_table_font, this, tr("Select the font of the table cells"));
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
	QFile file_(QETApp::configDir() % "/graphics_table.json");
	
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
		header_object.insert("font", QETUtils::fontToString(this->headerFont()));

		QJsonObject table_object;
		table_object.insert("margins", QETUtils::marginsToString(this->tableMargins()));
		table_object.insert("alignment", me.valueToKey(int(this->tableAlignment())));
		table_object.insert("font", QETUtils::fontToString(this->tableFont()));

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

	QFile file_(QETApp::configDir() % "/graphics_table.json");
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
		case Qt::AlignHCenter :
		case Qt::AlignCenter :  // accept AlignCenter in case it was hand-edited by the user
			ui->m_header_alignment_cb->setCurrentIndex(1);
			break;
		default:
			ui->m_header_alignment_cb->setCurrentIndex(2);
	}
	QETUtils::fontFromString(m_header_font, header_object.value("font").toString());
	ui->m_header_font_pb->setText(m_header_font.family());

		//Table
	auto table_object = config_object.value("table").toObject();
	m_table_margins = QETUtils::marginsFromString(table_object.value("margins").toString());
	switch (me.keyToValue(table_object.value("alignment").toString().toStdString().data())) {
		case Qt::AlignLeft :
			ui->m_table_alignment_cb->setCurrentIndex(0);
			break;
		case Qt::AlignHCenter :
		case Qt::AlignCenter :  // accept AlignCenter in case it was hand-edited by the user
			ui->m_table_alignment_cb->setCurrentIndex(1);
			break;
		default:
			ui->m_table_alignment_cb->setCurrentIndex(2);
	}
	QETUtils::fontFromString(m_table_font, table_object.value("font").toString());
	ui->m_table_font_pb->setText(m_table_font.family());

}

void AddTableDialog::fillSavedQuery()
{
	QFile file(QETApp::configDir() % "/graphics_table.json");
	if (file.open(QFile::ReadOnly))
	{
		QJsonDocument jsd(QJsonDocument::fromJson(file.readAll()));
		QJsonObject jso = jsd.object();

		for (auto it = jso.begin() ; it != jso.end() ; ++it) {
			ui->m_config_gb->addItem(it.key());
		}
	}
}

void AddTableDialog::setUpDefaults20(const QString &kind){
 if(!QetTableDefaults18::supportedKind18(kind))return;
 auto *box=new QGroupBox(tr("Defaults for new tables"),this);auto *layout=new QVBoxLayout(box);auto *row=new QHBoxLayout;
 auto *fonts=new QPushButton(tr("Use fonts as default"));fonts->setObjectName("addTableFontsDefault20");row->addWidget(fonts);
 auto *full=new QPushButton(tr("Use configuration as default"));full->setObjectName("addTableFullDefault20");row->addWidget(full);layout->addLayout(row);
 auto *remember=new QCheckBox(tr("Remember last used formatting"));remember->setObjectName("addTableRemember20");remember->setChecked(QetTableDefaults18::remember20(kind));layout->addWidget(remember);
 auto *status=new QLabel;status->setWordWrap(true);layout->addWidget(status);
 auto *displayLayout=qobject_cast<QVBoxLayout*>(ui->m_tab->widget(0)->layout());if(displayLayout)displayLayout->insertWidget(displayLayout->count()-1,box);
 auto capture=[this](){auto format=property("newTableDefault18Configuration").toMap().value("format").toMap();auto body=format.value("bodyStyle").toMap(),header=format.value("headerStyle").toMap();body["font"]=tableFont().toString();header["font"]=headerFont().toString();format["bodyStyle"]=body;format["headerStyle"]=header;QString query;if(auto *w=qobject_cast<ElementQueryWidget*>(contentWidget()))query=w->queryStr();else if(auto *w=qobject_cast<SummaryQueryWidget*>(contentWidget()))query=w->queryStr();const int columns=QetTableFields18::outputNames18(query).size();QVariantList bodyAlign,headAlign;for(int i=0;i<columns;++i){bodyAlign<<int(tableAlignment());headAlign<<int(headerAlignment());}format["alignments"]=bodyAlign;format["headerAlignments"]=headAlign;const auto margins=tableMargins();format["paddingX"]=qMax(margins.left(),margins.right());format["paddingY"]=qMax(margins.top(),margins.bottom());return format;};
 connect(fonts,&QPushButton::clicked,this,[=](){QetTableDefaults18::saveFonts20(kind,capture());status->setText(tr("Fonts saved for new tables of this type."));});
 connect(full,&QPushButton::clicked,this,[=](){auto configuration=property("newTableDefault18Configuration").toMap();configuration["format"]=capture();if(auto *w=qobject_cast<ElementQueryWidget*>(contentWidget()))configuration["query"]=w->queryStr();else if(auto *w=qobject_cast<SummaryQueryWidget*>(contentWidget()))configuration["query"]=w->queryStr();configuration["columns18"]=QetTableFields18::outputNames18(configuration.value("query").toString());QString error;status->setText(QetTableDefaults18::saveDefault18(kind,configuration,&error)?tr("Configuration saved for new tables of this type."):error);});
 connect(remember,&QCheckBox::toggled,this,[=](bool checked){QSettings().setValue(QetTableDefaults18::preferenceKey20(kind,"remember"),checked);});
}
