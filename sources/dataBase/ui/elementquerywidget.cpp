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
#include "elementquerywidget.h"
#include "../../ui/tablequery15.h"
#include "../../ui/tablefields18.h"
#include "../../ui/tablefields19.h"
#include "../../ui/formulabuilder19.h"
#include "../../ui/groupedbom13.h"
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLabel>

#include "../../properties/elementdata.h"
#include "../../qetapp.h"
#include "../../qetinformation.h"
#include "../legacyelementtypes.h"
#include "../projectdatabase.h"
#include "ui_elementquerywidget.h"

#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

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
    auto *grouped=new QPushButton(tr("Grouped BOM: manufacturer and article..."),this);grouped->setObjectName("groupedBom13Button");layout()->addWidget(grouped);
    connect(grouped,&QPushButton::clicked,this,[this](){
        QDialog d(this);d.setWindowTitle(tr("Grouped bill of materials"));auto *l=new QVBoxLayout(&d);
        auto *info=new QLabel(tr("Generate a BOM for all eligible components in the project.\nGroups use manufacturer, article and unit. Decorative objects are excluded by default; described items without an article remain separate.\nFor a supported grouped BOM, current fields and filters are retained. For another source, this creates a new grouped query. Excluded BOM items stay excluded."),&d);info->setWordWrap(true);l->addWidget(info);
        auto check=[&](const QString &title,const QString &key,bool fallback){auto *c=new QCheckBox(title,&d);QString q=queryStr();bool checked=q.contains("qet-bom13")&&q.contains(key+"=")?q.contains(key+"=1"):fallback;if(!m_source_query18.isEmpty()&&key=="refs")checked=selectedFieldIds19().contains("nomenclature:references");if(!m_source_query18.isEmpty()&&key=="seq")checked=selectedFieldIds19().contains("nomenclature:number");c->setChecked(checked);l->addWidget(c);return c;};
        auto *qty=check(tr("Sum quantities (unchecked: count occurrences)"),"qty",true);
        auto *refs=check(tr("Combine component references"),"refs",true);
        auto *seq=check(tr("Add sequential item numbers"),"seq",true);
        auto *aux=check(tr("Include auxiliary articles 1–4"),"aux",false);
        auto *data=check(tr("Only components with article / description data"),"data",true);data->setObjectName("table18MeaningfulBom");
        auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,&d);l->addWidget(buttons);connect(buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);
        if(d.exec()==QDialog::Accepted) {
          if(!m_known_group19){setQuery(QetFormatting::groupedBomQuery(qty->isChecked(),refs->isChecked(),seq->isChecked(),aux->isChecked(),data->isChecked()));return;}
          auto selected=selectedKeys();m_group_options19={{"qty",qty->isChecked()},{"refs",refs->isChecked()},{"seq",seq->isChecked()},{"aux",aux->isChecked()},{"data",data->isChecked()}};regenerateGrouped19();
          selected.removeIf([&](const QString &e){const auto base=QetTableQuery15::baseExpression(e);return (base=="\"No.\""&&!seq->isChecked())||(base=="\"References\""&&!refs->isChecked());});
          bool hasNo=false,hasReferences=false;for(const auto &e:selected){const auto id=QetTableFields19::semanticId("nomenclature",e);hasNo|=id=="nomenclature:number";hasReferences|=id=="nomenclature:references";}if(seq->isChecked()&&!hasNo)selected.prepend("\"No.\"");if(refs->isChecked()&&!hasReferences)selected<<"\"References\"";
          rebuildFields18();QetTableQuery15::restoreSelected(selected,ui->m_var_list,ui->m_choosen_list,m_items_list);updateQueryLine();
        }
    });

	m_export_info.insert("position", tr("Position"));
	m_export_info.insert("title", tr("Sheet title"));
	m_export_info.insert("diagram_position", tr("Sheet position"));
	m_export_info.insert("folio", tr("Sheet number"));

	m_button_group.setExclusive(false);
	m_button_group.addButton(ui->m_all_cb, 0);
	m_button_group.addButton(ui->m_terminal_cb, 1);
	m_button_group.addButton(ui->m_simple_cb, 2);
	m_button_group.addButton(ui->m_button_cb, 3);
	m_button_group.addButton(ui->m_coil_cb, 4);
	m_button_group.addButton(ui->m_protection_cb, 5);
	m_button_group.addButton(ui->m_thumbnail_cb, 6);
	m_button_group.addButton(ui->m_plc_cb, 7);
	m_button_group.addButton(ui->m_slave_cb, 8);
	connect(&m_button_group, static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::idClicked), [this](int id)

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
			updateAllCheckState();
		}

        if(!m_source_query18.isEmpty()&&!m_known_group19) {
          ui->m_edit_sql_query_cb->setChecked(true);refreshMode18();
          if(auto *notice=findChild<QLabel*>("table18QueryMode")){notice->setText(tr("This custom SQL result has no known component-type source. Choose a predefined component/BOM table to use element-type filters. The original SQL is preserved."));notice->show();}
          emit definitionChanged19();return;
        }
		updateQueryLine();
	});
	updateAllCheckState();

	setUpItems();
    QetTableFields18::installFields18("nomenclature",ui->m_var_list,m_items_list);
    m_custom_fields=QetTableQuery15::savedFields("nomenclature");
    QetTableQuery15::installFields(applicableFields18(),ui->m_var_list,m_items_list);
    auto *custom=new QPushButton(tr("Add custom field..."),this);custom->setObjectName("table15AddCustomField");layout()->addWidget(custom);
    connect(custom,&QPushButton::clicked,this,[this](){QList<QetFormula19::Field> catalog;for(const auto &f:sourceFields19())catalog<<QetFormula19::Field{f.first,f.second};QVariantMap field;if(QetTableQuery15::editField(this,field,"nomenclature",m_source_query18.isEmpty()?QStringList():QetTableFields18::outputNames18(m_source_query18),catalog))if(!addCustomField(field.value("name").toString(),field.value("heading").toString(),field.value("expression").toString(),field.value("saveForReuse").toBool(),field.value("formulaTree19").toMap()))QMessageBox::information(this,tr("Calculated field"),tr("A field with that name already exists, or its heading is already used by a selected column. Choose a unique field name and column heading."));});


    auto *recipes=new QWidget(this);auto *recipeLayout=new QHBoxLayout(recipes);recipeLayout->setContentsMargins(0,0,0,0);
    auto *recipe=new QComboBox(recipes);recipe->setObjectName("table18Recipe");
    recipe->addItem(tr("Choose a predefined table..."),QString());
    recipe->addItem(tr("Grouped BOM - components with part/description data"),"grouped");
    recipe->addItem(tr("Grouped BOM - include auxiliary articles"),"grouped-aux");
    recipe->addItem(tr("Component list - one row per component"),"components");
    auto *useRecipe=new QPushButton(tr("Use predefined table"),recipes);useRecipe->setObjectName("table18UseRecipe");
    recipeLayout->addWidget(recipe,1);recipeLayout->addWidget(useRecipe);
    static_cast<QVBoxLayout*>(layout())->insertWidget(0,recipes);
    auto *mode=new QLabel(this);mode->setObjectName("table18QueryMode");mode->setWordWrap(true);mode->hide();static_cast<QVBoxLayout*>(layout())->insertWidget(1,mode);
    connect(useRecipe,&QPushButton::clicked,this,[this,recipe](){if(!recipe->currentData().toString().isEmpty())usePredefined18(recipe->currentData().toString());});
    setupControls19();
    cleanFields19();
    connect(ui->m_sql_query,&QLineEdit::textChanged,this,[this](){if(!m_loading19&&ui->m_edit_sql_query_cb->isChecked())emit definitionChanged19();});
	fillSavedQuery();

	connect(ui->m_sql_query, &QLineEdit::textChanged, this, &ElementQueryWidget::checkQueryValidity);
	checkQueryValidity();
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
void ElementQueryWidget::setLegacyQuery(const QString &query)
{
    if(query.contains("/*qet-bom13 ")) {
        m_custom_query=query;ui->m_edit_sql_query_cb->setChecked(true);on_m_edit_sql_query_cb_clicked();ui->m_sql_query->setText(query);return;
    }
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
			QRegularExpression rx("^(\\( .+?\\))");
			auto rxm = rx.match(where);
			if (rxm.hasMatch())
			{
				auto str_type = rxm.captured(1);
				where.remove(str_type);
				const bool all_master_types = str_type.contains(
						ElementData::typeToString(ElementData::Master));

				int c=0;
				ui->m_simple_cb->setChecked    (str_type.contains(ElementData::typeToString(ElementData::Simple)) ? true : false);
				if (ui->m_simple_cb->isChecked()) {
					++c;
				}
				ui->m_terminal_cb->setChecked  (str_type.contains(ElementData::typeToString(ElementData::Terminal)) ? true : false);
				if (ui->m_terminal_cb->isChecked()) {
					++c;
				}
				ui->m_coil_cb->setChecked      (all_master_types || str_type.contains(ElementData::masterTypeToString(ElementData::Coil)));
				if (ui->m_coil_cb->isChecked()) {
					++c;
				}
				ui->m_button_cb->setChecked    (all_master_types || str_type.contains(ElementData::masterTypeToString(ElementData::Commutator)));
				if (ui->m_button_cb->isChecked()) {
					++c;
				}
				ui->m_protection_cb->setChecked(all_master_types || str_type.contains(ElementData::masterTypeToString(ElementData::Protection)));
				if (ui->m_protection_cb->isChecked()) {
					++c;
				}
			ui->m_thumbnail_cb->setChecked  (str_type.contains(ElementData::typeToString(ElementData::Thumbnail)) ? true : false);
			if (ui->m_thumbnail_cb->isChecked()) {
				++c;
			}
			ui->m_plc_cb->setChecked       (all_master_types || str_type.contains(ElementData::masterTypeToString(ElementData::PLC)));
			if (ui->m_plc_cb->isChecked()) {
				++c;
			}
			ui->m_slave_cb->setChecked     (str_type.contains(ElementData::typeToString(ElementData::Slave)));
			if (ui->m_slave_cb->isChecked()) {
				++c;
			}

			if (c == 8) {
					ui->m_all_cb->setCheckState(Qt::Checked);
				} else if (c > 0) {
					ui->m_all_cb->setCheckState(Qt::PartiallyChecked);
				}
			}
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

		QString join_str = strl.join("|");

		QRegularExpression rx_is_not_null(QStringLiteral("^(%1) != ''$").arg(join_str));
		QRegularExpression rx_is_null    (QStringLiteral("^\\((%1) IS NULL OR (%1) = ''\\)").arg(join_str));
		QRegularExpression rx_like       (QStringLiteral("^(%1) LIKE'%(.+)%'$").arg(join_str));
		QRegularExpression rx_not_like   (QStringLiteral("^(%1) NOT LIKE'%(.+)%'$").arg(join_str));
		QRegularExpression rx_equal      (QStringLiteral("^(%1)='(.+)'$").arg(join_str));
		QRegularExpression rx_not_equal  (QStringLiteral("^(%1)!='(.+)'$").arg(join_str));

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
QString ElementQueryWidget::rawQueryStr() const
{
		//User define is own query
	if (ui->m_edit_sql_query_cb->isChecked()) {
		return QetTableQuery15::withoutState(ui->m_sql_query->text());
	}
		//A saved nested query stays the source; graphical column changes project
        //its output rather than flattening derived aliases into the base view.
    if(!m_source_query18.isEmpty()) {
     if(m_known_group19||m_extra_filter.trimmed().isEmpty())return QetTableFields18::projectOutputs18(m_source_query18,selectedKeys());
     return "WITH \"qet_table_filtered19\" AS ("+QetTableQuery15::withoutState(m_source_query18)+") SELECT "+selectedKeys().join(", ")+" FROM \"qet_table_filtered19\" WHERE ("+m_extra_filter+")";
    }

        //Made a string list with the columns (keys) chosen by the user
	QStringList keys = QetTableFields19::formatSelected("nomenclature",selectedKeys(),m_number_start19,QString(),selectedFieldIds19());

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
		const QString alias = QetTableQuery15::alias(key);
		order_by += alias.isEmpty() ? key : alias;
		const QString expression = QetTableQuery15::baseExpression(key);

		auto f = FilterFor(key);
        f.second.replace('\'', "''");
        switch(f.first) {
        case 1: filter_ += " AND " + expression + " != ''"; break;
        case 2: filter_ += QStringLiteral(" AND (%1 IS NULL OR %1 = '')").arg(expression); break;
        case 3: filter_ += " AND " + expression + " LIKE'%" + f.second + "%'"; break;
        case 4: filter_ += " AND " + expression + " NOT LIKE'%" + f.second + "%'"; break;
        case 5: filter_ += " AND " + expression + "='" + f.second + "'"; break;
        case 6: filter_ += " AND " + expression + "!='" + f.second + "'"; break;
        default: break;
        }
    }

	QString from = " FROM element_nomenclature_view";

	QString where;
	where = " WHERE (";
	bool b = false;
	if (ui->m_terminal_cb->isChecked()) {
		if (b) where +=" OR";
		where +=  QStringLiteral(" element_type = '") += ElementData::typeToString(ElementData::Terminal) += "'";
		b = true;
	}
	if (ui->m_thumbnail_cb->isChecked()) {
		if (b) where +=" OR";
		where +=  QStringLiteral(" element_type = '") += ElementData::typeToString(ElementData::Thumbnail) += "'";
		b = true;
	}
	if (ui->m_simple_cb->isChecked()) {
		if (b) where +=" OR";
		where +=  QStringLiteral(" element_type = '") += ElementData::typeToString(ElementData::Simple) += "'";
		b = true;
	}
	const bool all_master_types = ui->m_button_cb->isChecked()
			&& ui->m_coil_cb->isChecked()
			&& ui->m_protection_cb->isChecked()
			&& ui->m_plc_cb->isChecked();
	if (all_master_types) {
		if (b) where += " OR";
		where += QStringLiteral(" element_type = '")
				 += ElementData::typeToString(ElementData::Master) += "'";
		b = true;
	}
	if (!all_master_types && ui->m_button_cb->isChecked()) {
		if (b) where +=" OR";
		where +=  QStringLiteral(" element_sub_type = '") += ElementData::masterTypeToString(ElementData::Commutator) += "'";
		b = true;
	}
	if (!all_master_types && ui->m_coil_cb->isChecked()) {
		if (b) where +=" OR";
		where +=  QStringLiteral(" element_sub_type = '") += ElementData::masterTypeToString(ElementData::Coil) += "'";
		b = true;
	}
	if (!all_master_types && ui->m_protection_cb->isChecked()) {
		if (b) where +=" OR";
		where +=  QStringLiteral(" element_sub_type = '") += ElementData::masterTypeToString(ElementData::Protection) += "'";
		b = true;
	}
	if (!all_master_types && ui->m_plc_cb->isChecked()) {
		if (b) where +=" OR";
		where +=  QStringLiteral(" element_sub_type = '") += ElementData::masterTypeToString(ElementData::PLC) += "'";
		b = true;
	}
	if (ui->m_slave_cb->isChecked()) {
		if (b) where +=" OR";
		where +=  QStringLiteral(" element_type = '") += ElementData::typeToString(ElementData::Slave) += "'";
		b = true;
	}
	where.append(")");

	if (where == " WHERE ()") {where=" WHERE 0";}
 int selectedTypes19=0;for(int id=1;id<=8;++id)selectedTypes19+=m_button_group.button(id)->isChecked();if(selectedTypes19==8)where.clear();

	// exclude_from_bom is already filtered by element_nomenclature_view
	// (see createElementNomenclatureView() in projectdatabase.cpp); this
	// widget's query reads FROM that view, so a flagged element never
	// reaches this point in the first place.

	if (!m_extra_filter.isEmpty()) {
		filter_ += QStringLiteral(" AND (") + m_extra_filter + ")";
	}

	if (where.isEmpty() && !filter_.isEmpty()) {
		filter_.remove(0, 4); //Remove the first " AND" of filter.
		filter_.prepend( " WHERE");
	}

	for(const auto &key:keys)if(QetTableFields19::semanticId("nomenclature",key)=="nomenclature:number") {const auto alias=QetTableQuery15::alias(key);order_by=" ORDER BY "+(alias.isEmpty()?QetTableQuery15::baseExpression(key):alias);break;}
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
	Unlike setGroupBy, you have to write the entire sentence.
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
		m_count = QString(", " % text % " ");
	} else {
		m_count.clear();
	}
	updateQueryLine();
}

/**
	@brief ElementQueryWidget::setSlavesIncluded
	Check or uncheck the contact blocks (slave elements) in the element
	type filter. The box is unchecked by default, which keeps the tables
	made before it existed as they were.
	@param included
*/
void ElementQueryWidget::setSlavesIncluded(bool included)
{
	ui->m_slave_cb->setChecked(included);
	updateAllCheckState();
	updateQueryLine();
}

/**
	@brief ElementQueryWidget::updateAllCheckState
	Set the "all" check box from the element type check boxes.
*/
void ElementQueryWidget::updateAllCheckState()
{
	int checked = 0;
	for (int i=1 ; i<9 ; ++i) {
		if (m_button_group.button(i)->isChecked()) {++checked;}
	}

	switch (checked)
	{
		case 0 :
			ui->m_all_cb->setCheckState(Qt::Unchecked);
			break;
		case 8:
			ui->m_all_cb->setCheckState(Qt::Checked);
			break;
		default:
			ui->m_all_cb->setCheckState(Qt::PartiallyChecked);
			break;
	}
}

/**
	@brief ElementQueryWidget::setExtraFilter
	Add a condition every row of the query must meet, on top of the filters
	chosen in this widget.
	@param text : the condition, ex : "label != ''"
	@param set :
	true by default -> the condition is used.
	false -> the condition is removed.
*/
void ElementQueryWidget::setExtraFilter(const QString &text, bool set)
{
	if (set) {
		m_extra_filter = text;
	} else {
		m_extra_filter.clear();
	}
	updateQueryLine();
}

/**
	@brief ElementQueryWidget::updateQueryLine
*/
void ElementQueryWidget::updateQueryLine()
{
    if(m_loading19)return;
    if(m_known_group19)regenerateGrouped19();
    cleanFields19();
    ui->m_sql_query->setText(QetTableQuery15::withoutState(queryStr()));
    emit definitionChanged19();
}

/**
	@brief ElementQueryWidget::selectedKeys
	@return the current keys of selected infos to be exported
*/
QStringList ElementQueryWidget::selectedKeys() const
{
		//Made a string list with the columns (keys) chosen by the user
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
	for(QString key : QETInformation::elementInfoKeys())
	{
		if (key == "formula" || key == "exclude_from_bom")
			continue;
		// Terminal number is read-only and not a column of the nomenclature
		// query, so offering it as an export/query column would break the
		// export (query fails with "no such column"). It stays available as a
		// text variable (%{terminal_number}) in the element editor.
		if (key == QETInformation::ELMT_TERMINAL_NUMBER)
			continue;

		auto item = new QListWidgetItem(QETInformation::translatedInfoKey(key), ui->m_var_list);
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
	QFile file(QETApp::configDir() % "/nomenclature.json");
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
    if(m_known_group19)if(auto *item=ui->m_var_list->currentItem())if(item->data(Qt::UserRole+20).isValid()) {
      QDialog dialog(this);dialog.setWindowTitle(tr("Add field to grouped BOM"));QFormLayout form(&dialog);
      auto *heading=new QLineEdit(item->text());form.addRow(tr("Column heading"),heading);
      auto *mode=new QComboBox;mode->addItem(tr("List distinct values"),"distinct");mode->addItem(tr("Common value; mark differing values"),"common");mode->addItem(tr("Split groups by this value"),"split");form.addRow(tr("Several values in one group"),mode);
      auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form.addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
      if(dialog.exec()==QDialog::Accepted&&!addGroupedField19(item->data(Qt::UserRole+20).toString(),heading->text(),mode->currentData().toString()))QMessageBox::information(this,tr("Grouped BOM"),tr("Choose a unique non-empty heading and a field from the available catalogue."));
      return;
    }
	if (auto *item = ui->m_var_list->takeItem(ui->m_var_list->currentRow())) {item->setHidden(false);ui->m_choosen_list->addItem(item);}
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
    const QString current=QetTableQuery15::withoutState(ui->m_sql_query->text());
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
    checkQueryValidity();
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

	QFile file_(QETApp::configDir() % "/nomenclature.json");
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
			//A query saved before June 2022 names element types the old way
		setQuery(LegacyElementTypes::upgradeQuery(
					 value_object.value("query").toString()));
	}
}

/**
	@brief ElementQueryWidget::on_m_save_current_conf_pb_clicked
	Save the actual query to nomenclature.json file
*/
void ElementQueryWidget::on_m_save_current_conf_pb_clicked()
{
	if (!projectDataBase::isReadOnlySelect(queryStr())) {
		return;
	}

	QFile file_(QETApp::configDir() % "/nomenclature.json");

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
	Q_UNUSED(arg1)
	checkQueryValidity();
}

/**
	@brief ElementQueryWidget::checkQueryValidity
	Live feedback for the custom-SQL box: only the free-text path can carry
	anything other than a SELECT (the column/filter builder always produces
	one), so this only actually restricts something once "Requête SQL
	personnalisée" is checked. Same rule projectDataBase::isReadOnlySelect()
	enforces at execution time -- this just tells the user *before* they hit
	Preview/Export/OK instead of after (qelectrotech-source-mirror#886).
*/
void ElementQueryWidget::checkQueryValidity()
{
	if (!ui->m_edit_sql_query_cb->isChecked()) {
		ui->m_query_warning_label->clear();
		ui->m_save_current_conf_pb->setEnabled(!ui->m_save_name_le->text().isEmpty());
		return;
	}

	QString reason;
	const bool valid = projectDataBase::isReadOnlySelect(ui->m_sql_query->text(), &reason);
	ui->m_query_warning_label->setText(valid ? QString() : reason);
	ui->m_save_current_conf_pb->setEnabled(valid && !ui->m_save_name_le->text().isEmpty());
}

/**
	@brief ElementQueryWidget::on_m_export_reports_pb_clicked
	Write every saved report in nomenclature.json to a file the user picks,
	so it can be handed to a colleague or another install
	(qelectrotech-source-mirror#886's "import and export report definitions
	for sharing between users or company installations").
*/
void ElementQueryWidget::on_m_export_reports_pb_clicked()
{
	QFile source(QETApp::configDir() % "/nomenclature.json");
	if (!source.open(QFile::ReadOnly)) {
		QMessageBox::information(this, tr("Export"), tr("No saved report to export."));
		return;
	}
	const auto content = source.readAll();
	source.close();

	if (QJsonDocument::fromJson(content).object().isEmpty()) {
		QMessageBox::information(this, tr("Export"), tr("No saved report to export."));
		return;
	}

	const QString file_path = QFileDialog::getSaveFileName(
			this, tr("Export reports"), QStringLiteral("rapports_qet.json"),
			tr("JSON files (*.json)"));
	if (file_path.isEmpty()) {
		return;
	}

	QFile dest(file_path);
	if (!dest.open(QFile::WriteOnly) || dest.write(content) == -1) {
		QMessageBox::critical(this, tr("Error"), tr("Unable to write to %1.").arg(file_path));
	}
}

/**
	@brief ElementQueryWidget::on_m_import_reports_pb_clicked
	Merge a previously-exported reports file into this install's
	nomenclature.json. A name already used locally is not overwritten
	silently -- the user is asked, per report, whether to replace it.
*/
void ElementQueryWidget::on_m_import_reports_pb_clicked()
{
	const QString file_path = QFileDialog::getOpenFileName(
			this, tr("Import reports"), QString(), tr("JSON files (*.json)"));
	if (file_path.isEmpty()) {
		return;
	}

	QFile source(file_path);
	if (!source.open(QFile::ReadOnly)) {
		QMessageBox::critical(this, tr("Error"), tr("Unable to read %1.").arg(file_path));
		return;
	}

	QJsonParseError parse_error;
	const auto incoming_doc = QJsonDocument::fromJson(source.readAll(), &parse_error);
	source.close();

	if (parse_error.error != QJsonParseError::NoError || !incoming_doc.isObject()) {
		QMessageBox::critical(
				this, tr("Error"),
				tr("%1 does not contain valid QElectroTech reports.").arg(file_path));
		return;
	}

	const auto incoming = incoming_doc.object();
	if (incoming.isEmpty()) {
		QMessageBox::information(this, tr("Import"), tr("This file does not contain any report."));
		return;
	}

	QFile dest_file(QETApp::configDir() % "/nomenclature.json");
	QJsonObject existing;
	if (dest_file.open(QFile::ReadOnly)) {
		existing = QJsonDocument::fromJson(dest_file.readAll()).object();
		dest_file.close();
	}

	int imported = 0, skipped = 0;
	for (auto it = incoming.begin(); it != incoming.end(); ++it)
	{
		if (existing.contains(it.key()))
		{
			const auto answer = QMessageBox::question(
					this, tr("Report already exists"),
					tr("A report named \"%1\" already exists. Replace it?").arg(it.key()),
					QMessageBox::Yes | QMessageBox::No);
			if (answer != QMessageBox::Yes) {
				++skipped;
				continue;
			}
		}
		existing[it.key()] = it.value();
		++imported;
	}

	if (dest_file.open(QFile::WriteOnly | QFile::Truncate))
	{
		dest_file.write(QJsonDocument(existing).toJson());
		dest_file.close();
	}
	else
	{
		QMessageBox::critical(this, tr("Error"), tr("Unable to write the local configuration."));
		return;
	}

	ui->m_conf_cb->clear();
	fillSavedQuery();

	QMessageBox::information(
			this, tr("Import"),
			tr("%1 report(s) imported, %2 skipped.").arg(imported).arg(skipped));
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


// Table 1.5 stores editor state beside the SELECT as an inert SQL comment.
// Unknown SQL remains Advanced and is never regenerated implicitly.

QStringList ElementQueryWidget::selectedFieldExpressions() const {return selectedKeys();}

bool ElementQueryWidget::addCustomField(const QString &name,const QString &heading,const QString &expression,bool saveForReuse,const QVariantMap &formulaTree19)
{
 if(!QetTableQuery15::fieldValid(name,heading,expression))return false;
 QVariantMap field{{"name",name.trimmed()},{"heading",heading.trimmed()},{"expression",expression.trimmed()},{"context","nomenclature"}};if(!formulaTree19.isEmpty())field["formulaTree19"]=formulaTree19;
 if(!m_source_query18.isEmpty())field["sourceColumns18"]=QetTableFields18::outputNames18(m_source_query18);
 const QString sql=QetTableQuery15::fieldExpression(field);
 for(const auto &f:m_custom_fields)if(QetTableQuery15::fieldExpression(f.toMap())==sql&&f.toMap().value("sourceColumns18")==field.value("sourceColumns18"))return true;
 for(const auto &f:applicableFields18())if(f.toMap().value("name").toString().compare(name.trimmed(),Qt::CaseInsensitive)==0)return false;
 for(const auto &selected:QetTableFields18::outputNames18(rawQueryStr()))if(selected.compare(heading.trimmed(),Qt::CaseInsensitive)==0)return false;
 m_custom_fields<<field;QetTableQuery15::installFields(applicableFields18(),ui->m_var_list,m_items_list);cleanFields19();
 if(saveForReuse)QetTableQuery15::saveFields("nomenclature",m_custom_fields);return true;
}

QVariantMap ElementQueryWidget::editorState() const
{
 QVariantMap result{{"version",15},{"context","nomenclature"},{"columns",selectedKeys()},{"customFields",m_custom_fields},{"advanced",ui->m_edit_sql_query_cb->isChecked()}};
 result["sourceSql18"]=m_source_query18;result["columnIds19"]=selectedFieldIds19();result["filterTree19"]=m_filter_tree19;
 result["knownGroup19"]=m_known_group19;result["groupOptions19"]=m_group_options19;result["groupFields19"]=m_group_fields19;result["numberStart19"]=m_number_start19;

 QVariantList types;for(int i=1;i<=8;++i)types<<m_button_group.button(i)->isChecked();result["types"]=types;
 QVariantMap filters;for(auto i=m_filter.cbegin();i!=m_filter.cend();++i)filters[i.key()]=QVariantList{i.value().first,i.value().second};result["filters"]=filters;
 result["groupBy"]=m_group_by;result["count"]=m_count;result["extraFilter"]=m_extra_filter;

 return result;
}

void ElementQueryWidget::restoreEditorState(const QVariantMap &state)
{
 for(const auto &f:state.value("customFields").toList()){auto field=f.toMap();if(QetTableQuery15::fieldValid(field.value("name").toString(),field.value("heading").toString(),field.value("expression").toString())){bool found=false;for(const auto &existing:m_custom_fields)if(existing.toMap()==field)found=true;if(!found)m_custom_fields<<field;}}
 QetTableQuery15::installFields(applicableFields18(),ui->m_var_list,m_items_list);
 QetTableQuery15::restoreSelected(state.value("columns").toStringList(),ui->m_var_list,ui->m_choosen_list,m_items_list);

 auto types=state.value("types").toList();for(int i=1;i<=8&&i<=types.size();++i)m_button_group.button(i)->setChecked(types[i-1].toBool());updateAllCheckState();
 m_filter.clear();auto filters=state.value("filters").toMap();for(auto i=filters.cbegin();i!=filters.cend();++i){auto values=i.value().toList();if(values.size()==2)m_filter[i.key()]=qMakePair(values[0].toInt(),values[1].toString());}
 m_group_by=state.value("groupBy").toString();m_count=state.value("count").toString();m_extra_filter=state.value("extraFilter").toString();

}

QString ElementQueryWidget::queryStr() const
{
 const QString sql=QetTableQuery15::withoutState(rawQueryStr());
 if(ui->m_edit_sql_query_cb->isChecked())return sql;
 auto state=editorState();state["sql"]=sql;return QetTableQuery15::withState(sql,state);
}

void ElementQueryWidget::setQuery(const QString &query)
{
 m_loading19=true;
 const QString sql=QetTableQuery15::withoutState(query);auto state=QetTableQuery15::state(query);m_filter_tree19=state.value("filterTree19").toMap();
 m_known_group19=false;m_group_options19.clear();m_group_fields19.clear();m_number_start19=qMax(1,state.value("numberStart19",1).toInt());
 m_source_query18.clear();rebuildFields18();reset();m_custom_query.clear();m_group_by.clear();m_count.clear();m_extra_filter.clear();
 ui->m_edit_sql_query_cb->setChecked(false);
 const bool saved=state.value("context").toString()=="nomenclature"&&state.value("sql").toString()==sql;
 if(saved&&!state.value("sourceSql18").toString().isEmpty()) {
  m_source_query18=state.value("sourceSql18").toString();
  m_known_group19=m_source_query18.contains("/*qet-bom13 ")&&m_source_query18.trimmed()==QetTableFields19::groupedQuery(QetTableFields19::groupedOptions(m_source_query18),{}, {},1).trimmed();
  m_group_options19=state.value("groupOptions19",QetTableFields19::groupedOptions(m_source_query18)).toMap();m_group_fields19=state.value("groupFields19").toList();rebuildFields18();
 }
 if(!saved)setLegacyQuery(sql);
 if(saved) {
  if(state.value("columns").toStringList().isEmpty()&&!QetTableQuery15::selectColumns(sql).isEmpty())state["columns"]=QetTableQuery15::selectColumns(sql);
  restoreEditorState(state);
  if(state.value("knownGroup19").toBool()&&!m_source_query18.isEmpty()) {
   const QString candidate=QetTableFields19::groupedQuery(m_group_options19,preGroupFilter19(),m_group_fields19,m_number_start19);
   m_known_group19=candidate.trimmed()==m_source_query18.trimmed();
   if(m_known_group19){const auto selected=selectedKeys();rebuildFields18();QetTableQuery15::restoreSelected(selected,ui->m_var_list,ui->m_choosen_list,m_items_list);}
  }
  if(m_known_group19&&!state.contains("knownGroup19"))for(auto *button:m_button_group.buttons())button->setChecked(true);
 }else QetTableQuery15::restoreSelected(QetTableQuery15::selectColumns(sql),ui->m_var_list,ui->m_choosen_list,m_items_list);
 ui->m_edit_sql_query_cb->setChecked(false);
 bool advanced=saved?state.value("advanced").toBool():sql.trimmed()!=rawQueryStr().trimmed();
 if(saved&&!advanced&&rawQueryStr().trimmed()!=sql.trimmed()) {
  QetTableQuery15::restoreSelected(QetTableQuery15::selectColumns(sql),ui->m_var_list,ui->m_choosen_list,m_items_list);
  advanced=sql.trimmed()!=rawQueryStr().trimmed();
 }
 ui->m_edit_sql_query_cb->setChecked(advanced);ui->m_sql_query->setText(sql);m_custom_query=advanced?sql:QString();
 if(!saved&&sql.contains("/*qet-bom13 ")&&QetTableFields18::canProjectOutputs18(sql)){setDerivedSource18(sql);ui->m_edit_sql_query_cb->setChecked(!m_known_group19);if(!m_known_group19)m_custom_query=sql;}
 if(saved){const auto ids=state.value("columnIds19").toStringList();for(int i=0;i<ui->m_choosen_list->count()&&i<ids.size();++i)ui->m_choosen_list->item(i)->setData(Qt::UserRole+19,ids[i]);}
 if(auto *spin=findChild<QSpinBox*>("table19NumberStart")){QSignalBlocker block(spin);spin->setValue(m_number_start19);}
 
 cleanFields19();refreshMode18();m_loading19=false;emit definitionChanged19();
}

QVariantList ElementQueryWidget::applicableFields18() const
{
 QVariantList result;const auto names=m_source_query18.isEmpty()?QStringList():QetTableFields18::outputNames18(m_source_query18);
 for(const auto &entry:m_custom_fields){const auto columns=entry.toMap().value("sourceColumns18").toStringList();if(columns==names)result<<entry;}
 return result;
}

void ElementQueryWidget::rebuildFields18()
{
 QSignalBlocker a(ui->m_var_list),b(ui->m_choosen_list);
 ui->m_var_list->clear();ui->m_choosen_list->clear();m_items_list.clear();
 if(m_source_query18.isEmpty()){setUpItems();QetTableFields18::installFields18("nomenclature",ui->m_var_list,m_items_list);}
 else {
  for(const auto &name:QetTableFields18::outputNames18(m_source_query18))QetTableQuery15::addItem(ui->m_var_list,m_items_list,QetTableQuery15::quotedName(name),name);
  if(m_known_group19) {
   int before=m_items_list.size();setUpItems();
   for(int i=before;i<m_items_list.size();++i)m_items_list[i]->setData(Qt::UserRole+20,m_items_list[i]->data(Qt::UserRole));
  }
 }
 QetTableQuery15::installFields(applicableFields18(),ui->m_var_list,m_items_list);
}

void ElementQueryWidget::setDerivedSource18(const QString &query)
{
 m_source_query18=QetTableQuery15::withoutState(query);
 m_known_group19=m_source_query18.contains("/*qet-bom13 ")&&m_source_query18.trimmed()==QetTableFields19::groupedQuery(QetTableFields19::groupedOptions(m_source_query18),{}, {},1).trimmed();
 if(m_known_group19){m_group_options19=QetTableFields19::groupedOptions(m_source_query18);for(auto *button:m_button_group.buttons())button->setChecked(true);updateAllCheckState();}
 rebuildFields18();
 QetTableQuery15::restoreSelected(QetTableFields18::quotedOutputs18(m_source_query18),ui->m_var_list,ui->m_choosen_list,m_items_list);
}

void ElementQueryWidget::refreshMode18()
{
 const bool advanced=ui->m_edit_sql_query_cb->isChecked();
 ui->m_sql_query->setEnabled(advanced);ui->m_info_widget->setDisabled(advanced);
 ui->m_parametre_widget->setDisabled(advanced);
 for(const auto &name:QStringList{"table19NumberStart","table19AddNumber"})if(auto *control=findChild<QWidget*>(name)){control->setEnabled(!advanced&&(m_source_query18.isEmpty()||m_known_group19));control->setToolTip(tr("Automatic numbering is editable for predefined component/BOM tables. Custom SQL keeps its own ordering and numbering."));}
 if(auto *notice=findChild<QLabel*>("table18QueryMode")){
  notice->setText(advanced?tr("Advanced SQL is preserved. Unchecking SQL edits its result columns without replacing its source query."):!m_source_query18.isEmpty()?tr("Saved query source: edit the columns and order above. Use the grouped-BOM controls or a predefined table to change its data selection."):QString());
  notice->setVisible(!notice->text().isEmpty());
 }
}

void ElementQueryWidget::usePredefined18(const QString &recipe)
{
 if(recipe=="components") {
  const QString sql=QetTableFields18::componentListQuery18();setQuery(sql);m_loading19=true;m_source_query18.clear();m_known_group19=false;rebuildFields18();
  for(auto *button:m_button_group.buttons())button->setChecked(true);updateAllCheckState();
  m_extra_filter="TRIM(COALESCE(description,''))<>'' OR TRIM(COALESCE(designation,''))<>'' OR TRIM(COALESCE(manufacturer_reference,''))<>''";
  QetTableQuery15::restoreSelected(QetTableQuery15::selectColumns(sql),ui->m_var_list,ui->m_choosen_list,m_items_list);ui->m_edit_sql_query_cb->setChecked(false);m_loading19=false;refreshMode18();updateQueryLine();
 }
 else if(recipe=="grouped"||recipe=="grouped-aux")setQuery(QetFormatting::groupedBomQuery(true,true,true,recipe=="grouped-aux"));
}

QStringList ElementQueryWidget::selectedFieldIds19() const {QStringList result;for(int i=0;i<ui->m_choosen_list->count();++i){auto *item=ui->m_choosen_list->item(i);QString id=item->data(Qt::UserRole+19).toString();for(const auto &v:m_custom_fields){const auto field=v.toMap();if(QetTableQuery15::fieldExpression(field)==item->data(Qt::UserRole).toString())id="custom:"+field.value("context").toString()+":"+field.value("name").toString();}result<< (id.isEmpty()?QetTableFields19::semanticId("nomenclature",item->data(Qt::UserRole).toString()):id);}return result;}

QString ElementQueryWidget::validationMessage19() const {
 if(ui->m_edit_sql_query_cb->isChecked())return {};
 if(selectedKeys().isEmpty())return tr("Choose at least one column. The previous table is kept until a valid definition is applied.");
 QSet<QString> headings;for(const auto &name:QetTableFields18::outputNames18(rawQueryStr())){const auto folded=name.toCaseFolded();if(headings.contains(folded))return tr("Choose unique column headings. Two selected columns currently have the same heading: %1").arg(name);headings.insert(folded);}
 return {};
}
void ElementQueryWidget::cleanFields19() {
 for(auto *item:m_items_list) {
  const QString expression=item->data(Qt::UserRole).toString();
  for(const auto &v:m_custom_fields){const auto field=v.toMap();if(QetTableQuery15::fieldExpression(field)==expression)item->setData(Qt::UserRole+19,"custom:"+field.value("context").toString()+":"+field.value("name").toString());}
  for(const auto &v:m_group_fields19){const auto field=v.toMap();if(expression==QetTableQuery15::quotedName(field.value("heading").toString()))item->setData(Qt::UserRole+19,"nomenclature:group:"+field.value("key").toString());}
 }
 QetTableFields19::deduplicate("nomenclature",ui->m_var_list,ui->m_choosen_list,m_items_list);
 const auto query=findChild<QLineEdit*>("table19SearchFields");const QString text=query?query->text():QString();const auto categoryBox=findChild<QComboBox*>("table19FieldCategory");const QString category=categoryBox?categoryBox->currentData().toString():QStringLiteral("all");
 for(int i=0;i<ui->m_var_list->count();++i){auto *item=ui->m_var_list->item(i);item->setHidden((!text.isEmpty()&&!item->text().contains(text,Qt::CaseInsensitive)&&!item->toolTip().contains(text,Qt::CaseInsensitive))||(category!="all"&&QetTableFields19::category("nomenclature",item)!=category));}
 for(int i=0;i<ui->m_choosen_list->count();++i)ui->m_choosen_list->item(i)->setHidden(false);
}
QList<QPair<QString,QString>> ElementQueryWidget::sourceFields19() const {
 QList<QPair<QString,QString>> result;
 if(!m_source_query18.isEmpty()){for(const auto &name:QetTableFields18::outputNames18(m_source_query18))result<<qMakePair(name,QetTableQuery15::quotedName(name));return result;}
 for(auto *item:m_items_list)result<<qMakePair(item->text(),QetTableQuery15::baseExpression(item->data(Qt::UserRole).toString()));return result;
}
void ElementQueryWidget::setNumberStart19(int start) {
 m_number_start19=qMax(1,start);
 if(m_source_query18.isEmpty())for(int i=0;i<ui->m_choosen_list->count();++i){auto *item=ui->m_choosen_list->item(i);if(selectedFieldIds19().value(i)=="nomenclature:number")item->setData(Qt::UserRole,QetTableFields19::formatSelected("nomenclature",{item->data(Qt::UserRole).toString()},m_number_start19).first());}
 updateQueryLine();
}
void ElementQueryWidget::enableNumber19(bool enabled) {
 for(int i=0;i<ui->m_choosen_list->count();++i){auto *item=ui->m_choosen_list->item(i);if(selectedFieldIds19().value(i)=="nomenclature:number"){
  if(!enabled){ui->m_choosen_list->takeItem(i);ui->m_var_list->addItem(item);updateQueryLine();}return;
 }}
 if(!enabled)return;
 if(m_known_group19&&!QetTableFields18::outputNames18(m_source_query18).contains("No.")){m_group_options19["seq"]=true;regenerateGrouped19();}
 if(!m_source_query18.isEmpty()&&!m_known_group19){QMessageBox::information(this,tr("Automatic numbering"),tr("Choose a predefined table to add ordered automatic numbering. Custom SQL keeps its own ordering."));return;}
 for(int i=0;i<ui->m_var_list->count();++i)if(QetTableFields19::semanticId("nomenclature",ui->m_var_list->item(i)->data(Qt::UserRole).toString())=="nomenclature:number"){
  auto *item=ui->m_var_list->takeItem(i);ui->m_choosen_list->insertItem(0,item);updateQueryLine();return;
 }
 auto *item=QetTableQuery15::addItem(ui->m_var_list,m_items_list,QetTableFields19::numberExpression("nomenclature",m_number_start19),tr("No."));ui->m_var_list->takeItem(ui->m_var_list->row(item));ui->m_choosen_list->insertItem(0,item);updateQueryLine();
}
void ElementQueryWidget::setFilter19(const QString &condition) {m_filter_tree19.clear();m_extra_filter=condition;updateQueryLine();}
void ElementQueryWidget::setupControls19() {
 auto *controls=new QWidget(this);auto *box=new QVBoxLayout(controls);box->setContentsMargins(0,0,0,0);
 auto *search=new QLineEdit;search->setObjectName("table19SearchFields");search->setPlaceholderText(tr("Search available fields..."));auto *searchRow=new QHBoxLayout;searchRow->addWidget(search,1);auto *category=new QComboBox;category->setObjectName("table19FieldCategory");category->addItem(tr("All fields"),"all");category->addItem(tr("Common fields"),"common");category->addItem(tr("Component properties"),"component");category->addItem(tr("Auxiliary articles"),"auxiliary"); category->addItem(tr("Sheet / placement"),"sheet");category->addItem(tr("Calculated fields"),"calculated");searchRow->addWidget(category);box->addLayout(searchRow);connect(category,qOverload<int>(&QComboBox::currentIndexChanged),this,[this](){cleanFields19();});
 connect(search,&QLineEdit::textChanged,this,[this](){cleanFields19();});
 auto *row=new QHBoxLayout;auto *more=new QHBoxLayout;auto *number=new QPushButton(tr("Add No. column"));number->setObjectName("table19AddNumber");row->addWidget(number);connect(number,&QPushButton::clicked,this,[this](){enableNumber19();});
 row->addWidget(new QLabel(tr("Starting number")));auto *start=new QSpinBox;start->setObjectName("table19NumberStart");start->setRange(1,1000000000);start->setValue(1);row->addWidget(start);connect(start,qOverload<int>(&QSpinBox::valueChanged),this,&ElementQueryWidget::setNumberStart19);
 
 auto *heading=new QPushButton(tr("Column heading..."));heading->setObjectName("table19ColumnHeading");row->addWidget(heading);connect(heading,&QPushButton::clicked,this,[this](){auto *item=ui->m_choosen_list->currentItem();if(!item)return;bool ok=false;const auto old=item->data(Qt::UserRole).toString();QString name=QInputDialog::getText(this,tr("Column heading"),tr("Displayed heading"),QLineEdit::Normal,item->text(),&ok).trimmed();if(!ok||name.isEmpty())return;for(int i=0;i<ui->m_choosen_list->count();++i)if(ui->m_choosen_list->item(i)!=item&&ui->m_choosen_list->item(i)->text().compare(name,Qt::CaseInsensitive)==0){QMessageBox::information(this,tr("Column heading"),tr("Choose a unique heading."));return;}for(int i=0;i<m_custom_fields.size();++i){auto field=m_custom_fields[i].toMap();if(QetTableQuery15::fieldExpression(field)==old){field["heading"]=name;m_custom_fields[i]=field;}}item->setData(Qt::UserRole,QetTableQuery15::baseExpression(old)+" AS "+QetTableQuery15::quotedName(name));item->setText(name);updateQueryLine();});
 auto *groupValues=new QPushButton(tr("Grouped field values..."));groupValues->setObjectName("table19GroupedFieldValues");more->addWidget(groupValues);connect(groupValues,&QPushButton::clicked,this,[this](){
  auto *item=ui->m_choosen_list->currentItem();if(!item||!m_known_group19)return;const auto expression=QetTableQuery15::baseExpression(item->data(Qt::UserRole).toString());int index=-1;for(int i=0;i<m_group_fields19.size();++i)if(QetTableQuery15::quotedName(m_group_fields19[i].toMap().value("heading").toString())==expression){index=i;break;}if(index<0){QMessageBox::information(this,tr("Grouped field values"),tr("Select an additional grouped property column. Qty and References use the main grouped-BOM controls."));return;}
  auto field=m_group_fields19[index].toMap();QDialog dialog(this);dialog.setWindowTitle(tr("Grouped field values"));QFormLayout form(&dialog);auto *mode=new QComboBox;mode->addItem(tr("List distinct values"),"distinct");mode->addItem(tr("Common value; mark differing values"),"common");mode->addItem(tr("Split groups by this value"),"split");mode->setCurrentIndex(qMax(0,mode->findData(field.value("mode"))));form.addRow(tr("Several values in one group"),mode);auto *buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);form.addRow(buttons);connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);if(dialog.exec()==QDialog::Accepted){field["mode"]=mode->currentData();m_group_fields19[index]=field;updateQueryLine();}
 });
 auto *editFormula=new QPushButton(tr("Edit calculated field..."));editFormula->setObjectName("table19EditCalculatedField");more->addWidget(editFormula);
 connect(editFormula,&QPushButton::clicked,this,[this](){
  auto *item=ui->m_choosen_list->currentItem();if(!item)item=ui->m_var_list->currentItem();if(!item)return;
  const QString oldExpression=item->data(Qt::UserRole).toString();int index=-1;for(int i=0;i<m_custom_fields.size();++i)if(QetTableQuery15::fieldExpression(m_custom_fields[i].toMap())==oldExpression){index=i;break;}
  if(index<0){QMessageBox::information(this,tr("Calculated field"),tr("Select a calculated/custom field to edit. Use Column heading for a normal field."));return;}
  QVariantMap field=m_custom_fields[index].toMap();
  QList<QetFormula19::Field> catalog;for(const auto &f:sourceFields19())catalog<<QetFormula19::Field{f.first,f.second};if(!QetTableQuery15::editField(this,field,"nomenclature",m_source_query18.isEmpty()?QStringList():QetTableFields18::outputNames18(m_source_query18),catalog))return;
  if(!m_source_query18.isEmpty())field["sourceColumns18"]=QetTableFields18::outputNames18(m_source_query18);
  bool conflict=false;
  for(int i=0;i<m_custom_fields.size();++i)if(i!=index){const auto other=m_custom_fields[i].toMap();if(other.value("sourceColumns18").toStringList()!=field.value("sourceColumns18").toStringList())continue;if(other.value("name").toString().compare(field.value("name").toString(),Qt::CaseInsensitive)==0||other.value("heading").toString().compare(field.value("heading").toString(),Qt::CaseInsensitive)==0)conflict=true;}
  for(int i=0;i<ui->m_choosen_list->count();++i){auto *other=ui->m_choosen_list->item(i);if(other==item)continue;const auto headings=QetTableFields18::outputNames18("SELECT "+other->data(Qt::UserRole).toString());if(!headings.isEmpty()&&headings.first().compare(field.value("heading").toString(),Qt::CaseInsensitive)==0)conflict=true;}
  if(conflict){QMessageBox::information(this,tr("Calculated field"),tr("Another field already uses this name or column heading. Choose unique values; the existing definition has been kept."));return;}
  m_custom_fields[index]=field;item->setData(Qt::UserRole,QetTableQuery15::fieldExpression(field));item->setText(field.value("name").toString()+tr(" (custom)"));item->setData(Qt::UserRole+19,"custom:nomenclature:"+field.value("name").toString());
  if(field.value("saveForReuse").toBool())QetTableQuery15::saveFields("nomenclature",m_custom_fields);updateQueryLine();
 });
 auto *filter=new QPushButton(tr("Build filter..."));filter->setObjectName("table19BuildFilter");more->addWidget(filter);connect(filter,&QPushButton::clicked,this,[this](){
  QList<QetFormula19::Field> fields;
  if(m_known_group19||m_source_query18.isEmpty()) {
   for(const auto &key:QETInformation::elementInfoKeys())if(key!="formula"&&key!="exclude_from_bom"&&key!=QETInformation::ELMT_TERMINAL_NUMBER)fields<<QetFormula19::Field{QETInformation::translatedInfoKey(key),QetTableQuery15::quotedName(key)};
   for(auto it=m_export_info.cbegin();it!=m_export_info.cend();++it)fields<<QetFormula19::Field{it.value(),QetTableQuery15::quotedName(it.key())};
  }else for(const auto &field:sourceFields19())fields<<QetFormula19::Field{field.first,field.second};
  QString condition=m_extra_filter;
  QetFormula19::Dialog dialog(this,fields,condition,true,false);if(!m_filter_tree19.isEmpty())dialog.setExpressionTree(m_filter_tree19);if(dialog.exec()==QDialog::Accepted){setFilter19(dialog.expression());m_filter_tree19=dialog.expressionTree();emit definitionChanged19();}
 });
 auto *clear=new QPushButton(tr("Clear filter"));clear->setObjectName("table19ClearFilter");more->addWidget(clear);connect(clear,&QPushButton::clicked,this,[this](){setFilter19({});});
 row->addStretch();more->addStretch();box->addLayout(row);box->addLayout(more);static_cast<QVBoxLayout*>(layout())->insertWidget(2,controls);
}

QString ElementQueryWidget::preGroupFilter19() const {
 QStringList clauses,types;
 for(int id=1;id<=8;++id)if(m_button_group.button(id)->isChecked()) {
  switch(id){
  case 1:types<<"element_type='terminal'";break;case 2:types<<"element_type='simple'";break;case 6:types<<"element_type='thumbnail'";break;case 8:types<<"element_type='slave'";break;
  case 3:types<<"(element_type='master' AND element_sub_type='"+ElementData::masterTypeToString(ElementData::Commutator)+"')";break;case 4:types<<"(element_type='master' AND element_sub_type='"+ElementData::masterTypeToString(ElementData::Coil)+"')";break;case 5:types<<"(element_type='master' AND element_sub_type='"+ElementData::masterTypeToString(ElementData::Protection)+"')";break;case 7:types<<"(element_type='master' AND element_sub_type='"+ElementData::masterTypeToString(ElementData::PLC)+"')";break;
  }
 }
 int checked=0;for(int i=1;i<=8;++i)checked+=m_button_group.button(i)->isChecked();
 if(checked!=8)clauses<< (types.isEmpty()?QStringLiteral("0"):"("+types.join(" OR ")+")");
 for(auto it=m_filter.cbegin();it!=m_filter.cend();++it) {
  QString expression=QetTableQuery15::baseExpression(it.key());const QString id=QetTableFields19::semanticId("nomenclature",it.key());if(id=="nomenclature:number")continue;
  if(id=="nomenclature:references")expression="label";else if(id=="nomenclature:manufacturer")expression="manufacturer";else if(id=="nomenclature:article")expression="manufacturer_reference";else if(id=="nomenclature:name")expression="COALESCE(NULLIF(TRIM(description),''),designation,'')";else if(id=="nomenclature:quantity")expression="CAST(REPLACE(COALESCE(NULLIF(TRIM(quantity),''),'1'),',','.') AS REAL)";else if(id=="nomenclature:unit")expression="unity";
  for(const auto &v:m_group_fields19)if(v.toMap().value("heading").toString()==QetTableFields18::unquote18(expression))expression=v.toMap().value("key").toString();
  const QString filter=QetTableFields19::condition(expression,it.value().first,it.value().second);if(!filter.isEmpty())clauses<<"("+filter+")";
 }
 if(!m_extra_filter.trimmed().isEmpty())clauses<<"("+m_extra_filter+")";return clauses.join(" AND ");
}
void ElementQueryWidget::regenerateGrouped19() {
 if(!m_known_group19)return;
 m_source_query18=QetTableFields19::groupedQuery(m_group_options19,preGroupFilter19(),m_group_fields19,m_number_start19);
}
bool ElementQueryWidget::addGroupedField19(const QString &key,const QString &heading,const QString &mode) {
 if(!m_known_group19||heading.trimmed().isEmpty()||!QStringList{"common","distinct","split"}.contains(mode))return false;
 for(const auto &v:m_group_fields19)if(v.toMap().value("key").toString()==key)return false;
 for(const auto &name:QetTableFields18::outputNames18(m_source_query18))if(name.compare(heading.trimmed(),Qt::CaseInsensitive)==0)return false;
 bool exists=false;for(auto *item:m_items_list)if(item->data(Qt::UserRole+20).toString()==key)exists=true;if(!exists)return false;
 auto selected=selectedKeys();m_group_fields19<<QVariantMap{{"key",key},{"heading",heading.trimmed()},{"mode",mode}};regenerateGrouped19();rebuildFields18();selected<<QetTableQuery15::quotedName(heading.trimmed());QetTableQuery15::restoreSelected(selected,ui->m_var_list,ui->m_choosen_list,m_items_list);updateQueryLine();return true;
}
