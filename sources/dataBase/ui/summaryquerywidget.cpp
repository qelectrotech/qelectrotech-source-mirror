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
#include "summaryquerywidget.h"
#include "../../ui/tablequery15.h"
#include "../../ui/tablefields18.h"
#include "../../ui/tablefields19.h"
#include "../../ui/formulabuilder19.h"

#include "../../qetapp.h"
#include "../../qetinformation.h"
#include "../projectdatabase.h"
#include "ui_summaryquerywidget.h"

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
    QetTableFields18::installFields18("summary",ui->m_available_list,m_items_list);QetTableQuery15::addItem(ui->m_available_list,m_items_list,QetTableFields19::numberExpression("summary",m_number_start19),tr("No. - sequential item number"));
    m_custom_fields=QetTableQuery15::savedFields("summary");
    QetTableQuery15::installFields(applicableFields18(),ui->m_available_list,m_items_list);
    auto *custom=new QPushButton(tr("Add custom field..."),this);custom->setObjectName("table15AddCustomField");layout()->addWidget(custom);
    connect(custom,&QPushButton::clicked,this,[this](){QList<QetFormula19::Field> catalog;for(const auto &f:sourceFields19())catalog<<QetFormula19::Field{f.first,f.second};QVariantMap field;if(QetTableQuery15::editField(this,field,"summary",m_source_query18.isEmpty()?QStringList():QetTableFields18::outputNames18(m_source_query18),catalog))if(!addCustomField(field.value("name").toString(),field.value("heading").toString(),field.value("expression").toString(),field.value("saveForReuse").toBool(),field.value("formulaTree19").toMap()))QMessageBox::information(this,tr("Calculated field"),tr("A field with that name already exists, or its heading is already used by a selected column. Choose a unique field name and column heading."));});


    auto *recipes=new QWidget(this);auto *recipeLayout=new QHBoxLayout(recipes);recipeLayout->setContentsMargins(0,0,0,0);
    auto *recipe=new QComboBox(recipes);recipe->setObjectName("table18Recipe");
    recipe->addItem(tr("Choose a predefined table..."),QString());
    recipe->addItem(tr("Contents - Page, Page content, Comments, Date"),"contents");
    recipe->addItem(tr("Contents - include Author and Revision"),"contents-full");
    auto *useRecipe=new QPushButton(tr("Use predefined table"),recipes);useRecipe->setObjectName("table18UseRecipe");
    recipeLayout->addWidget(recipe,1);recipeLayout->addWidget(useRecipe);
    static_cast<QVBoxLayout*>(layout())->insertWidget(0,recipes);
    auto *mode=new QLabel(this);mode->setObjectName("table18QueryMode");mode->setWordWrap(true);mode->hide();static_cast<QVBoxLayout*>(layout())->insertWidget(1,mode);
    connect(useRecipe,&QPushButton::clicked,this,[this,recipe](){if(!recipe->currentData().toString().isEmpty())usePredefined18(recipe->currentData().toString());});
    setupControls19();
    cleanFields19();
    connect(ui->m_user_query_le,&QLineEdit::textChanged,this,[this](){if(!m_loading19&&ui->m_edit_sql_query_cb->isChecked())emit definitionChanged19();});
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
QString SummaryQueryWidget::rawQueryStr() const
{
		//User define is own query
	if (ui->m_edit_sql_query_cb->isChecked()) {
		return ui->m_user_query_le->text();
	}

		//A saved nested query stays the source; graphical column changes project
        //its output rather than flattening derived aliases into the base view.
    if(!m_source_query18.isEmpty()) {
     if(m_filter19.isEmpty())return QetTableFields18::projectOutputs18(m_source_query18,selectedKeys());
     return "WITH \"qet_table_filtered19\" AS ("+QetTableQuery15::withoutState(m_source_query18)+") SELECT "+selectedKeys().join(", ")+" FROM \"qet_table_filtered19\" WHERE ("+m_filter19+")";
    }

        //Made a string list with the columns (keys) choosen by the user
	QStringList keys = QetTableFields19::formatSelected("summary",selectedKeys(),m_number_start19,m_date_format19,selectedFieldIds19());

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

		//Always ordered by the folio's own position, never by the columns
		//the user happened to choose to display. This is a table of
		//contents: it lists the folios of the project and its order is the
		//project's order. Ordering by the displayed columns instead meant
		//that choosing, say, Title as the first column silently sorted the
		//summary alphabetically (bugtracker #238).
		//
		//project_summary_view exposes the position as "pos", from
		//diagram.pos, which is an INTEGER -- so this sorts numerically and
		//folio 10 does not land between folio 1 and folio 2. One row per
		//folio means pos fully determines the order and no secondary key is
		//needed.
		//
		//A user who wants a different order can still write the query by
		//hand; that path returns above, untouched.
	QString order_by = " ORDER BY pos";

	QString q(select + column + from + (m_filter19.isEmpty()?QString():" WHERE ("+m_filter19+")") + order_by);
	return q;
}

/**
	@brief SummaryQueryWidget::setQuery
	@param query
	set the current query to query.
	If it's possible, rebuild the state of the widget from the query
*/
void SummaryQueryWidget::setLegacyQuery(const QString &query)
{
	if (query.startsWith("SELECT"))
	{
		reset();
		ui->m_edit_sql_query_cb->setChecked(false);
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

			//If the query this widget would build from the columns just
			//parsed above does not match the query as loaded byte-for-byte,
			//the user wrote it by hand (a join, a subquery, a different
			//view) and accepting the dialog unmodified must not silently
			//replace it with a generated one (bugtracker #885).
		const bool custom_query = query != rawQueryStr();
		m_custom_query = custom_query ? query : QString();
		ui->m_edit_sql_query_cb->setChecked(custom_query);
		ui->m_user_query_le->setEnabled(custom_query);
		ui->m_info_widget->setDisabled(custom_query);
	}
}

/**
	@brief SummaryQueryWidget::setUpItems
*/
void SummaryQueryWidget::setUpItems()
{
	for (auto key : QETInformation::diagramInfoKeys())
	{
		if (key == "filename" || key == "display_folio") {
			continue;
		}
		auto item = new QListWidgetItem(QETInformation::translatedInfoKey(key), ui->m_available_list);
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
	QFile file(QETApp::configDir() % "/summary.json");
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
    if(m_loading19)return;
    
    cleanFields19();
    ui->m_user_query_le->setText(QetTableQuery15::withoutState(queryStr()));
    emit definitionChanged19();
}

/**
	@brief SummaryQueryWidget::selectedKeys
	@return
*/
QStringList SummaryQueryWidget::selectedKeys() const
{
		//Made a string list with the columns (keys) choosen by the user
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
    const QString current=QetTableQuery15::withoutState(ui->m_user_query_le->text());
    if(ui->m_edit_sql_query_cb->isChecked())m_custom_query=current;
    else if(!m_source_query18.isEmpty()&&current.trimmed()==QetTableFields18::projectOutputs18(m_source_query18,selectedKeys()).trimmed()) {
        // Existing graphical projection remains valid when SQL is only viewed.
    } else if(current.trimmed()!=rawQueryStr().trimmed()) {
        const bool supported=QetTableFields18::canProjectOutputs18(current);
        if(supported)setDerivedSource18(current);
        else {ui->m_edit_sql_query_cb->setChecked(true);if(auto *notice=findChild<QLabel*>("table18QueryMode")){notice->setText(tr("This SQL query needs unique named output columns for graphical editing (no duplicate headings or wildcard *). It is preserved. Add unique AS headings in SQL, or choose a predefined table."));notice->show();}return;}
    } else m_source_query18.clear();
    refreshMode18();
    if(!ui->m_edit_sql_query_cb->isChecked())updateQueryLine();
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
	QFile file_(QETApp::configDir() % "/summary.json");

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

	QFile file_(QETApp::configDir() % "/summary.json");
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

// Table 1.5 stores editor state beside the SELECT as an inert SQL comment.
// Unknown SQL remains Advanced and is never regenerated implicitly.

QStringList SummaryQueryWidget::selectedFieldExpressions() const {return selectedKeys();}

bool SummaryQueryWidget::addCustomField(const QString &name,const QString &heading,const QString &expression,bool saveForReuse,const QVariantMap &formulaTree19)
{
 if(!QetTableQuery15::fieldValid(name,heading,expression))return false;
 QVariantMap field{{"name",name.trimmed()},{"heading",heading.trimmed()},{"expression",expression.trimmed()},{"context","summary"}};if(!formulaTree19.isEmpty())field["formulaTree19"]=formulaTree19;
 if(!m_source_query18.isEmpty())field["sourceColumns18"]=QetTableFields18::outputNames18(m_source_query18);
 const QString sql=QetTableQuery15::fieldExpression(field);
 for(const auto &f:m_custom_fields)if(QetTableQuery15::fieldExpression(f.toMap())==sql&&f.toMap().value("sourceColumns18")==field.value("sourceColumns18"))return true;
 for(const auto &f:applicableFields18())if(f.toMap().value("name").toString().compare(name.trimmed(),Qt::CaseInsensitive)==0)return false;
 for(const auto &selected:QetTableFields18::outputNames18(rawQueryStr()))if(selected.compare(heading.trimmed(),Qt::CaseInsensitive)==0)return false;
 m_custom_fields<<field;QetTableQuery15::installFields(applicableFields18(),ui->m_available_list,m_items_list);cleanFields19();
 if(saveForReuse)QetTableQuery15::saveFields("summary",m_custom_fields);return true;
}

QVariantMap SummaryQueryWidget::editorState() const
{
 QVariantMap result{{"version",15},{"context","summary"},{"columns",selectedKeys()},{"customFields",m_custom_fields},{"advanced",ui->m_edit_sql_query_cb->isChecked()}};
 result["sourceSql18"]=m_source_query18;result["columnIds19"]=selectedFieldIds19();result["filterTree19"]=m_filter_tree19;
 result["numberStart19"]=m_number_start19;result["dateFormat19"]=m_date_format19;result["filter19"]=m_filter19;

 return result;
}

void SummaryQueryWidget::restoreEditorState(const QVariantMap &state)
{
 for(const auto &f:state.value("customFields").toList()){auto field=f.toMap();if(QetTableQuery15::fieldValid(field.value("name").toString(),field.value("heading").toString(),field.value("expression").toString())){bool found=false;for(const auto &existing:m_custom_fields)if(existing.toMap()==field)found=true;if(!found)m_custom_fields<<field;}}
 QetTableQuery15::installFields(applicableFields18(),ui->m_available_list,m_items_list);
 QetTableQuery15::restoreSelected(state.value("columns").toStringList(),ui->m_available_list,ui->m_choosen_list,m_items_list);

}

QString SummaryQueryWidget::queryStr() const
{
 const QString sql=QetTableQuery15::withoutState(rawQueryStr());
 if(ui->m_edit_sql_query_cb->isChecked())return sql;
 auto state=editorState();state["sql"]=sql;return QetTableQuery15::withState(sql,state);
}

void SummaryQueryWidget::setQuery(const QString &query)
{
 m_loading19=true;
 const QString sql=QetTableQuery15::withoutState(query);auto state=QetTableQuery15::state(query);m_filter_tree19=state.value("filterTree19").toMap();
 m_number_start19=qMax(1,state.value("numberStart19",1).toInt());m_date_format19=state.value("dateFormat19").toString();m_filter19=state.value("filter19").toString();
 m_source_query18.clear();rebuildFields18();reset();m_custom_query.clear();
 ui->m_edit_sql_query_cb->setChecked(false);
 const bool saved=state.value("context").toString()=="summary"&&state.value("sql").toString()==sql;
 if(saved&&!state.value("sourceSql18").toString().isEmpty()){m_source_query18=state.value("sourceSql18").toString();rebuildFields18();}
 if(!saved)setLegacyQuery(sql);
 if(saved) {
  if(state.value("columns").toStringList().isEmpty()&&!QetTableQuery15::selectColumns(sql).isEmpty())state["columns"]=QetTableQuery15::selectColumns(sql);
  restoreEditorState(state);
 }else QetTableQuery15::restoreSelected(QetTableQuery15::selectColumns(sql),ui->m_available_list,ui->m_choosen_list,m_items_list);
 ui->m_edit_sql_query_cb->setChecked(false);
 bool advanced=saved?state.value("advanced").toBool():sql.trimmed()!=rawQueryStr().trimmed();
 // A saved graphical state must describe the executable query, not silently erase it.
 if(saved&&!advanced&&rawQueryStr().trimmed()!=sql.trimmed()) {
  QetTableQuery15::restoreSelected(QetTableQuery15::selectColumns(sql),ui->m_available_list,ui->m_choosen_list,m_items_list);
  advanced=sql.trimmed()!=rawQueryStr().trimmed();
 }
 ui->m_edit_sql_query_cb->setChecked(advanced);ui->m_user_query_le->setText(sql);m_custom_query=advanced?sql:QString();
 if(saved){const auto ids=state.value("columnIds19").toStringList();for(int i=0;i<ui->m_choosen_list->count()&&i<ids.size();++i)ui->m_choosen_list->item(i)->setData(Qt::UserRole+19,ids[i]);}
 if(auto *spin=findChild<QSpinBox*>("table19NumberStart")){QSignalBlocker block(spin);spin->setValue(m_number_start19);}
 if(auto *combo=findChild<QComboBox*>("table19DateFormat")){QSignalBlocker block(combo);combo->setCurrentIndex(qMax(0,combo->findData(m_date_format19)));}
 cleanFields19();refreshMode18();m_loading19=false;emit definitionChanged19();
}

QVariantList SummaryQueryWidget::applicableFields18() const
{
 QVariantList result;const auto names=m_source_query18.isEmpty()?QStringList():QetTableFields18::outputNames18(m_source_query18);
 for(const auto &entry:m_custom_fields){const auto columns=entry.toMap().value("sourceColumns18").toStringList();if(columns==names)result<<entry;}
 return result;
}

void SummaryQueryWidget::rebuildFields18()
{
 QSignalBlocker a(ui->m_available_list),b(ui->m_choosen_list);
 ui->m_available_list->clear();ui->m_choosen_list->clear();m_items_list.clear();
 if(m_source_query18.isEmpty()){setUpItems();QetTableFields18::installFields18("summary",ui->m_available_list,m_items_list);QetTableQuery15::addItem(ui->m_available_list,m_items_list,QetTableFields19::numberExpression("summary",m_number_start19),tr("No. - sequential item number"));}
 else for(const auto &name:QetTableFields18::outputNames18(m_source_query18))QetTableQuery15::addItem(ui->m_available_list,m_items_list,QetTableQuery15::quotedName(name),name);
 QetTableQuery15::installFields(applicableFields18(),ui->m_available_list,m_items_list);
}

void SummaryQueryWidget::setDerivedSource18(const QString &query)
{
 m_source_query18=QetTableQuery15::withoutState(query);rebuildFields18();
 QetTableQuery15::restoreSelected(QetTableFields18::quotedOutputs18(m_source_query18),ui->m_available_list,ui->m_choosen_list,m_items_list);
}

void SummaryQueryWidget::refreshMode18()
{
 const bool advanced=ui->m_edit_sql_query_cb->isChecked();
 ui->m_user_query_le->setEnabled(advanced);ui->m_info_widget->setDisabled(advanced);
 for(const auto &name:QStringList{"table19NumberStart","table19AddNumber","table19DateFormat"})if(auto *control=findChild<QWidget*>(name)){control->setEnabled(!advanced&&m_source_query18.isEmpty());control->setToolTip(tr("Use a predefined Contents table for graphical date formatting and automatic numbering. Custom SQL keeps its own formats."));}
 if(auto *notice=findChild<QLabel*>("table18QueryMode")){
  notice->setText(advanced?tr("Advanced SQL is preserved. Unchecking SQL edits its result columns without replacing its source query."):!m_source_query18.isEmpty()?tr("Saved query source: edit the columns and order above. Use the grouped-BOM controls or a predefined table to change its data selection."):QString());
  notice->setVisible(!notice->text().isEmpty());
 }
}

void SummaryQueryWidget::usePredefined18(const QString &recipe)
{
 if(recipe!="contents"&&recipe!="contents-full")return;
 QString sql=QetTableFields18::defaultQuery18("summary");
 if(recipe=="contents-full")sql.replace(" FROM project_summary_view",", author AS \"Author\", indexrev AS \"Revision\" FROM project_summary_view");
 setQuery(sql);
}

QStringList SummaryQueryWidget::selectedFieldIds19() const {QStringList result;for(int i=0;i<ui->m_choosen_list->count();++i){auto *item=ui->m_choosen_list->item(i);QString id=item->data(Qt::UserRole+19).toString();for(const auto &v:m_custom_fields){const auto field=v.toMap();if(QetTableQuery15::fieldExpression(field)==item->data(Qt::UserRole).toString())id="custom:"+field.value("context").toString()+":"+field.value("name").toString();}result<< (id.isEmpty()?QetTableFields19::semanticId("summary",item->data(Qt::UserRole).toString()):id);}return result;}

QString SummaryQueryWidget::validationMessage19() const {
 if(ui->m_edit_sql_query_cb->isChecked())return {};
 if(selectedKeys().isEmpty())return tr("Choose at least one column. The previous table is kept until a valid definition is applied.");
 QSet<QString> headings;for(const auto &name:QetTableFields18::outputNames18(rawQueryStr())){const auto folded=name.toCaseFolded();if(headings.contains(folded))return tr("Choose unique column headings. Two selected columns currently have the same heading: %1").arg(name);headings.insert(folded);}
 return {};
}
void SummaryQueryWidget::cleanFields19() {
 for(auto *item:m_items_list) {
  const QString expression=item->data(Qt::UserRole).toString();
  for(const auto &v:m_custom_fields){const auto field=v.toMap();if(QetTableQuery15::fieldExpression(field)==expression)item->setData(Qt::UserRole+19,"custom:"+field.value("context").toString()+":"+field.value("name").toString());}
  
 }
 QetTableFields19::deduplicate("summary",ui->m_available_list,ui->m_choosen_list,m_items_list);
 const auto query=findChild<QLineEdit*>("table19SearchFields");const QString text=query?query->text():QString();const auto categoryBox=findChild<QComboBox*>("table19FieldCategory");const QString category=categoryBox?categoryBox->currentData().toString():QStringLiteral("all");
 for(int i=0;i<ui->m_available_list->count();++i){auto *item=ui->m_available_list->item(i);item->setHidden((!text.isEmpty()&&!item->text().contains(text,Qt::CaseInsensitive)&&!item->toolTip().contains(text,Qt::CaseInsensitive))||(category!="all"&&QetTableFields19::category("summary",item)!=category));}
 for(int i=0;i<ui->m_choosen_list->count();++i)ui->m_choosen_list->item(i)->setHidden(false);
}
QList<QPair<QString,QString>> SummaryQueryWidget::sourceFields19() const {
 QList<QPair<QString,QString>> result;
 if(!m_source_query18.isEmpty()){for(const auto &name:QetTableFields18::outputNames18(m_source_query18))result<<qMakePair(name,QetTableQuery15::quotedName(name));return result;}
 for(auto *item:m_items_list)result<<qMakePair(item->text(),QetTableQuery15::baseExpression(item->data(Qt::UserRole).toString()));return result;
}
void SummaryQueryWidget::setNumberStart19(int start) {
 m_number_start19=qMax(1,start);
 if(m_source_query18.isEmpty())for(int i=0;i<ui->m_choosen_list->count();++i){auto *item=ui->m_choosen_list->item(i);if(selectedFieldIds19().value(i)=="summary:number")item->setData(Qt::UserRole,QetTableFields19::formatSelected("summary",{item->data(Qt::UserRole).toString()},m_number_start19).first());}
 updateQueryLine();
}
void SummaryQueryWidget::enableNumber19(bool enabled) {
 for(int i=0;i<ui->m_choosen_list->count();++i){auto *item=ui->m_choosen_list->item(i);if(selectedFieldIds19().value(i)=="summary:number"){
  if(!enabled){ui->m_choosen_list->takeItem(i);ui->m_available_list->addItem(item);updateQueryLine();}return;
 }}
 if(!enabled)return;
 
 if(!m_source_query18.isEmpty()&&!false){QMessageBox::information(this,tr("Automatic numbering"),tr("Choose a predefined table to add ordered automatic numbering. Custom SQL keeps its own ordering."));return;}
 for(int i=0;i<ui->m_available_list->count();++i)if(QetTableFields19::semanticId("summary",ui->m_available_list->item(i)->data(Qt::UserRole).toString())=="summary:number"){
  auto *item=ui->m_available_list->takeItem(i);ui->m_choosen_list->insertItem(0,item);updateQueryLine();return;
 }
 auto *item=QetTableQuery15::addItem(ui->m_available_list,m_items_list,QetTableFields19::numberExpression("summary",m_number_start19),tr("No."));ui->m_available_list->takeItem(ui->m_available_list->row(item));ui->m_choosen_list->insertItem(0,item);updateQueryLine();
}
void SummaryQueryWidget::setFilter19(const QString &condition) {m_filter_tree19.clear();m_filter19=condition;updateQueryLine();}
void SummaryQueryWidget::setupControls19() {
 auto *controls=new QWidget(this);auto *box=new QVBoxLayout(controls);box->setContentsMargins(0,0,0,0);
 auto *search=new QLineEdit;search->setObjectName("table19SearchFields");search->setPlaceholderText(tr("Search available fields..."));auto *searchRow=new QHBoxLayout;searchRow->addWidget(search,1);auto *category=new QComboBox;category->setObjectName("table19FieldCategory");category->addItem(tr("All fields"),"all");category->addItem(tr("Common fields"),"common"); category->addItem(tr("Sheet / placement"),"sheet");category->addItem(tr("Calculated fields"),"calculated");searchRow->addWidget(category);box->addLayout(searchRow);connect(category,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](){cleanFields19();});
 connect(search,&QLineEdit::textChanged,this,[this](){cleanFields19();});
 auto *row=new QHBoxLayout;auto *more=new QHBoxLayout;auto *number=new QPushButton(tr("Add No. column"));number->setObjectName("table19AddNumber");row->addWidget(number);connect(number,&QPushButton::clicked,this,[this](){enableNumber19();});
 row->addWidget(new QLabel(tr("Starting number")));auto *start=new QSpinBox;start->setObjectName("table19NumberStart");start->setRange(1,1000000000);start->setValue(1);row->addWidget(start);connect(start,qOverload<int>(&QSpinBox::valueChanged),this,&SummaryQueryWidget::setNumberStart19);
 row->addWidget(new QLabel(tr("Date format")));auto *date=new QComboBox;date->setObjectName("table19DateFormat");date->addItem(tr("Keep stored format"),"");date->addItem("DD.MM.YYYY","%d.%m.%Y");date->addItem("YYYY-MM-DD","%Y-%m-%d");date->addItem("DD/MM/YYYY","%d/%m/%Y");date->addItem("MM/DD/YYYY","%m/%d/%Y");row->addWidget(date);connect(date,qOverload<int>(&QComboBox::currentIndexChanged),this,[this,date](){setDateFormat19(date->currentData().toString());});
 auto *heading=new QPushButton(tr("Column heading..."));heading->setObjectName("table19ColumnHeading");row->addWidget(heading);connect(heading,&QPushButton::clicked,this,[this](){auto *item=ui->m_choosen_list->currentItem();if(!item)return;bool ok=false;const auto old=item->data(Qt::UserRole).toString();QString name=QInputDialog::getText(this,tr("Column heading"),tr("Displayed heading"),QLineEdit::Normal,item->text(),&ok).trimmed();if(!ok||name.isEmpty())return;for(int i=0;i<ui->m_choosen_list->count();++i)if(ui->m_choosen_list->item(i)!=item&&ui->m_choosen_list->item(i)->text().compare(name,Qt::CaseInsensitive)==0){QMessageBox::information(this,tr("Column heading"),tr("Choose a unique heading."));return;}for(int i=0;i<m_custom_fields.size();++i){auto field=m_custom_fields[i].toMap();if(QetTableQuery15::fieldExpression(field)==old){field["heading"]=name;m_custom_fields[i]=field;}}item->setData(Qt::UserRole,QetTableQuery15::baseExpression(old)+" AS "+QetTableQuery15::quotedName(name));item->setText(name);updateQueryLine();});
 auto *editFormula=new QPushButton(tr("Edit calculated field..."));editFormula->setObjectName("table19EditCalculatedField");more->addWidget(editFormula);
 connect(editFormula,&QPushButton::clicked,this,[this](){
  auto *item=ui->m_choosen_list->currentItem();if(!item)item=ui->m_available_list->currentItem();if(!item)return;
  const QString oldExpression=item->data(Qt::UserRole).toString();int index=-1;for(int i=0;i<m_custom_fields.size();++i)if(QetTableQuery15::fieldExpression(m_custom_fields[i].toMap())==oldExpression){index=i;break;}
  if(index<0){QMessageBox::information(this,tr("Calculated field"),tr("Select a calculated/custom field to edit. Use Column heading for a normal field."));return;}
  QVariantMap field=m_custom_fields[index].toMap();
  QList<QetFormula19::Field> catalog;for(const auto &f:sourceFields19())catalog<<QetFormula19::Field{f.first,f.second};if(!QetTableQuery15::editField(this,field,"summary",m_source_query18.isEmpty()?QStringList():QetTableFields18::outputNames18(m_source_query18),catalog))return;
  if(!m_source_query18.isEmpty())field["sourceColumns18"]=QetTableFields18::outputNames18(m_source_query18);
  bool conflict=false;
  for(int i=0;i<m_custom_fields.size();++i)if(i!=index){const auto other=m_custom_fields[i].toMap();if(other.value("sourceColumns18").toStringList()!=field.value("sourceColumns18").toStringList())continue;if(other.value("name").toString().compare(field.value("name").toString(),Qt::CaseInsensitive)==0||other.value("heading").toString().compare(field.value("heading").toString(),Qt::CaseInsensitive)==0)conflict=true;}
  for(int i=0;i<ui->m_choosen_list->count();++i){auto *other=ui->m_choosen_list->item(i);if(other==item)continue;const auto headings=QetTableFields18::outputNames18("SELECT "+other->data(Qt::UserRole).toString());if(!headings.isEmpty()&&headings.first().compare(field.value("heading").toString(),Qt::CaseInsensitive)==0)conflict=true;}
  if(conflict){QMessageBox::information(this,tr("Calculated field"),tr("Another field already uses this name or column heading. Choose unique values; the existing definition has been kept."));return;}
  m_custom_fields[index]=field;item->setData(Qt::UserRole,QetTableQuery15::fieldExpression(field));item->setText(field.value("name").toString()+tr(" (custom)"));item->setData(Qt::UserRole+19,"custom:summary:"+field.value("name").toString());
  if(field.value("saveForReuse").toBool())QetTableQuery15::saveFields("summary",m_custom_fields);updateQueryLine();
 });
 auto *filter=new QPushButton(tr("Build filter..."));filter->setObjectName("table19BuildFilter");more->addWidget(filter);connect(filter,&QPushButton::clicked,this,[this](){
  QList<QetFormula19::Field> fields;for(const auto &field:sourceFields19())if(!field.second.contains("ROW_NUMBER",Qt::CaseInsensitive))fields<<QetFormula19::Field{field.first,field.second};
  QString condition=m_filter19;
  QetFormula19::Dialog dialog(this,fields,condition,true,false);if(!m_filter_tree19.isEmpty())dialog.setExpressionTree(m_filter_tree19);if(dialog.exec()==QDialog::Accepted){setFilter19(dialog.expression());m_filter_tree19=dialog.expressionTree();emit definitionChanged19();}
 });
 auto *clear=new QPushButton(tr("Clear filter"));clear->setObjectName("table19ClearFilter");more->addWidget(clear);connect(clear,&QPushButton::clicked,this,[this](){setFilter19({});});
 row->addStretch();more->addStretch();box->addLayout(row);box->addLayout(more);static_cast<QVBoxLayout*>(layout())->insertWidget(2,controls);
}

void SummaryQueryWidget::setDateFormat19(const QString &format) {
 m_date_format19=format;
 if(m_source_query18.isEmpty()&&!format.isEmpty())for(int i=0;i<ui->m_choosen_list->count();++i){auto *item=ui->m_choosen_list->item(i);QString e=item->data(Qt::UserRole).toString();if(selectedFieldIds19().value(i)=="summary:date"){item->setData(Qt::UserRole,QetTableFields19::formatSelected("summary",{e},m_number_start19,format).first());}}
 updateQueryLine();
}
