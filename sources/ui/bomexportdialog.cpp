/*
    Copyright 2006-2019 The QElectroTech Team
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
#include "bomexportdialog.h"
#include "ui_bomexportdialog.h"
#include "qetapp.h"
#include "qetproject.h"
#include "elementprovider.h"
#include "element.h"
#include "diagram.h"
#include "diagramposition.h"

#include <QListWidgetItem>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>

/**
 * @brief BOMExportDialog::BOMExportDialog
 * @param project the project for create the bill of material
 * @param parent widget
 */
BOMExportDialog::BOMExportDialog(QETProject *project, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BOMExportDialog),
    m_project(project)
{
    ui->setupUi(this);
    setUpItems();
    createDataBase();
    fillSavedQuery();
}

/**
 * @brief BOMExportDialog::~BOMExportDialog
 */
BOMExportDialog::~BOMExportDialog()
{
    delete ui;
    m_data_base.close();
}

/**
 * @brief BOMExportDialog::exec
 * Reimplemented from QDialog
 * @return
 */
int BOMExportDialog::exec()
{
    int r = QDialog::exec();
    if (r == QDialog::Accepted)
    {
            //save in csv file
        QString file_name = tr("nomenclature_") + QString(m_project ->title());
        if (!file_name.endsWith(".csv")) {
            file_name += ".csv";
        }
        QString file_path = QFileDialog::getSaveFileName(this, tr("Enregister sous... "), file_name, tr("Fichiers csv (*.csv)"));
        QFile file(file_path);
        if (!file_path.isEmpty())
        {
            if (QFile::exists(file_path ))
            {
                    // if file already exist -> delete it
                if (!QFile::remove(file_path) )
                {
                    QMessageBox::critical(this, tr("Erreur"),
                                          tr("Impossible de remplacer le fichier!\n\n")+
                                          "Destination : "+file_path+"\n");
                }
            }
            if (file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream stream(&file);
                stream << getBom() << endl;
            }
        }
    }
    return r;
}

/**
 * @brief BOMExportDialog::setUpItems
 * Setup all items available for create the column of the bill of material.
 */
void BOMExportDialog::setUpItems()
{
    for(QString key : QETApp::elementInfoKeys())
    {
        auto item = new QListWidgetItem(QETApp::elementTranslatedInfoKey(key), ui->m_var_list);
        item->setData(Qt::UserRole, key.replace("-", "_")); //We must to replace "-" by "_" because "-" is a sql keyword.
        item->setData(Qt::UserRole+1, key); //But we store the real key for easily retrieve it in the element information
    }
    QStringList other_keys({"pos", "folio_title", "folio_pos", "folio_num", "designation_qty"});
    QStringList other_translated({tr("Position"), tr("Titre du folio"), tr("Position de folio"), tr("Numéro de folio"), tr("Quantité (Numéro d'article)")});
    for(int i=0 ; i<other_keys.size() ; ++i)
    {
        auto item = new QListWidgetItem(other_translated.at(i), ui->m_var_list);
        item->setData(Qt::UserRole, other_keys.at(i));
    }
}

/**
 * @brief BOMExportDialog::on_m_add_pb_clicked
 */
void BOMExportDialog::on_m_add_pb_clicked()
{
    if (auto *item = ui->m_var_list->takeItem(ui->m_var_list->currentRow())) {
        ui->m_choosen_list->addItem(item);
    }

    updateQueryLine();
}

/**
 * @brief BOMExportDialog::on_m_remove_pb_clicked
 */
void BOMExportDialog::on_m_remove_pb_clicked()
{
    if (auto *item = ui->m_choosen_list->takeItem(ui->m_choosen_list->currentRow())) {
        ui->m_var_list->addItem(item);
    }

    updateQueryLine();
}

/**
 * @brief BOMExportDialog::on_m_up_pb_clicked
 */
void BOMExportDialog::on_m_up_pb_clicked()
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
 * @brief BOMExportDialog::on_m_down_pb_clicked
 */
void BOMExportDialog::on_m_down_pb_clicked()
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

void BOMExportDialog::on_m_save_name_le_textChanged(const QString &arg1) {
    ui->m_save_current_conf_pb->setDisabled(arg1.isEmpty());
}

/**
 * @brief BOMExportDialog::getBom
 * @return the bill of material as string already formated
 * for export to csv.
 */
QString BOMExportDialog::getBom()
{
    QString data; //The string to be returned
    if (ui->m_include_header_cb->isChecked()) {
        data = headers();
    }

    QSqlQuery query (queryStr() , m_data_base);
    if (!query.exec()) {
        qDebug() << "Query error : " << query.lastError();
    }

    QStringList record;
    while (query.next())
    {
        auto i=0;
        while (query.value(i).isValid())
        {
            record << query.value(i).toString();
            ++i;
        }
        data += record.join(";") + "\n";
        record.clear();
    }

    m_data_base.close();
    return data;
}

/**
 * @brief BOMExportDialog::headers
 * @return the header to be use for the csv file
 */
QString BOMExportDialog::headers() const
{
    QString header_string;

    if (!ui->m_edit_sql_query_cb->isChecked())
    {
            //Made a string list with the colomns (keys) choosen by the user
        QStringList keys;
        int row = 0;
        while (auto *item = ui->m_choosen_list->item(row))
        {
            keys.append(item->data(Qt::UserRole).toString());
            ++row;
        }


        for (int i=0 ; i<keys.size() ; i++)
        {
            if(!header_string.isEmpty()) {
                header_string += ";";
            }
            header_string += QETApp::elementTranslatedInfoKey(keys.at(i));
        }
        header_string += "\n";
    }
    else if (!queryStr().isEmpty())     //Try to retreive the header according to the sql query
    {
        if (queryStr().startsWith("SELECT ") && queryStr().contains("FROM"))
        {
            auto header = queryStr();
            header.remove(0, 7); //Remove SELECT from the string;
            header.truncate(header.indexOf("FROM")); //Now we only have the string between SELECT and FROM
            header.replace(" ", ""); //remove white space
            QStringList list = header.split(",");
            if (!list.isEmpty())
            {
                for (int i=0 ; i<list.size() ; i++)
                {
                    if(!header_string.isEmpty()) {
                        header_string += ";";
                    }
                    header_string += QETApp::elementTranslatedInfoKey(list.at(i));
                }
                header_string += "\n";
            }
        }
    }
    return header_string;
}

/**
 * @brief BOMExportDialog::createDataBase
 * @return true if database is successfully created
 */
bool BOMExportDialog::createDataBase()
{
        //Create a sqlite data base to sort the bom
    m_data_base = QSqlDatabase::addDatabase("QSQLITE", "bill_of_material");
    if (!m_data_base.open())
    {
        m_data_base.close();
        return false;
    }

        //Create the table:
    QStringList keys;
    auto row = 0;
    while (ui->m_var_list->item(row))
    {
        keys << ui->m_var_list->item(row)->data(Qt::UserRole).toString();
        ++row;
    }
    keys << "element_type" << "element_subtype";

    QString table("CREATE TABLE bom(");
    bool first = true;
    for (auto string : keys)
    {
        if (first) {
            first = false;
        } else {
            table += ",";
        }
        if (string == "designation_qty") {
            table += string += " INT";

        } else {
            table += string += " VARCHAR(512)";

        }
    }
    table += ");";
    m_data_base.exec(table);

    QStringList bind_values;
    for (auto key : keys) {
        bind_values << key.prepend(":");
    }

        //Prepare the query used for insert new record
    QString insert("INSERT INTO bom (" +
                   keys.join(", ") +
                   ") VALUES (" +
                   bind_values.join(", ") +
                   ")");

    m_insert_query = QSqlQuery(m_data_base);
    m_insert_query.prepare(insert);

    QString update("UPDATE bom SET designation_qty = :bind_qty WHERE designation IS NOT NULL AND designation = :bind_ref");
    m_update_qty_query = QSqlQuery(m_data_base);
    m_update_qty_query.prepare(update);

    QString count ("SELECT COUNT(designation) FROM bom WHERE designation = :bind_ref");
    m_count_ref_query = QSqlQuery(m_data_base);
    m_count_ref_query.prepare(count);

    populateDataBase();

    return true;
}

/**
 * @brief BOMExportDialog::populateDataBase
 * Populate the database
 */
void BOMExportDialog::populateDataBase()
{
    for (auto *diagram : m_project->diagrams())
    {
        ElementProvider ep(diagram);
        QList<Element *> elements_list = ep.find(Element::Simple | Element::Terminale | Element::Master);

            //Insert all value into the database
        for (auto elmt : elements_list)
        {
            auto hash = elementInfoToString(elmt);
            for (auto key : hash.keys())
            {
                QString value = hash.value(key);
                QString bind = key.prepend(":");
                m_insert_query.bindValue(bind, value);
            }

            m_insert_query.bindValue(":element_type", elmt->linkTypeToString());
            m_insert_query.bindValue(":element_subtype", elmt->kindInformations()["type"].toString());
            m_insert_query.bindValue(":designation_qty", 0);

            if (!m_insert_query.exec()) {
                qDebug() << "BOMExportDialog::populateDataBase insert error : " << m_insert_query.lastError();
            }

            if (!elmt->elementInformations().value("designation").toString().isEmpty())
            {
                QString ref = elmt->elementInformations().value("designation").toString();
                m_count_ref_query.bindValue(":bind_ref", ref);
                if (m_count_ref_query.exec())
                {
                    if (m_count_ref_query.first())
                    {
                        int c = m_count_ref_query.value(0).toInt();
                        m_update_qty_query.bindValue(":bind_ref", ref);
                        m_update_qty_query.bindValue(":bind_qty", c);
                        if (!m_update_qty_query.exec()) {
                            qDebug() << "BOMExportDialog::populateDataBase update qty query error : " << m_insert_query.lastError();
                        }
                    }
                }
                else {
                    qDebug() << "BOMExportDialog::populateDataBase count ref query : " << m_count_ref_query.lastError();
                }
            }
        }
    }
}

/**
 * @brief BOMExportDialog::elementInfoToString
 * @param elmt
 * @return a Hash with as key the name of bdd columns and value the value of @elmt for each columns.
 */
QHash<QString, QString> BOMExportDialog::elementInfoToString(Element *elmt) const
{
    QHash<QString, QString> keys_hash; //Use to get the element info according to the database columns name
    int row = 0;
    while (auto *item = ui->m_var_list->item(row))
    {
        keys_hash.insert(item->data(Qt::UserRole).toString(),
                         item->data(Qt::UserRole+1).toString());
        ++row;
    }

    QHash<QString, QString> hash; //Store the value for each columns
    for (auto key : keys_hash.keys())
    {
        if (key == "pos") {
            hash.insert(key, elmt->diagram()->convertPosition(elmt->scenePos()).toString());
        }
        else if (key == "folio_title") {
            hash.insert(key, elmt->diagram()->title());
        }
        else if (key == "folio_pos") {
            hash.insert(key, QString::number(elmt->diagram()->folioIndex() + 1));
        }
        else if (key == "folio_num") {
            hash.insert(key, elmt->diagram()->border_and_titleblock.folio());
        }
        else if (key == "designation_qty") {
            hash.insert(key, key);
        }
        else {
            hash.insert(key, elmt->elementInformations()[keys_hash.value(key)].toString());
        }
    }

    return hash;
}

/**
 * @brief BOMExportDialog::queryStr
 * @return the query string
 */
QString BOMExportDialog::queryStr() const
{
        //User define is own query
    if (ui->m_edit_sql_query_cb->isChecked()) {
        return ui->m_sql_query->text();
    }
        //Made a string list with the colomns (keys) choosen by the user
    QStringList keys;
    int row = 0;
    while (auto *item = ui->m_choosen_list->item(row))
    {
        keys.append(item->data(Qt::UserRole).toString());
        ++row;
    }

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

    QString from = " FROM bom";
    QString where;
    if (ui->m_button_rb->isChecked()) {
        where = " WHERE element_subtype = 'commutator'";
    } else if (ui->m_terminal_rb->isChecked()) {
        where = " WHERE element_type = 'Terminale'";
    } else if (ui->m_coil_rb->isChecked()) {
        where = " WHERE element_subtype = 'coil'";
    } else if (ui->m_protection_rb  ->isChecked()) {
        where = " WHERE element_subtype = 'protection'";
    }
    QString where_bom;
    if(ui->m_format_as_bom_rb->isChecked())
    {
        if (where.isEmpty()) {
            where = " WHERE designation IS NOT NULL";
        } else {
            where.append(" AND designation IS NOT NULL");
        }

    }

    QString group_by = ui->m_format_as_bom_rb->isChecked() ? " GROUP BY designation" : "";
    QString q(select + column + from + where + where_bom + group_by + order_by);
    return q;
}

void BOMExportDialog::updateQueryLine() {
    ui->m_sql_query->setText(queryStr());
}

/**
 * @brief BOMExportDialog::fillSavedQuery
 * Fill the combo box with the name of the saved query
 */
void BOMExportDialog::fillSavedQuery()
{
    QFile file(QETApp::configDir() + "/bill_of_materials.json");
    if (file.open(QFile::ReadOnly))
    {
        QJsonDocument jsd(QJsonDocument::fromJson(file.readAll()));
        QJsonObject jso = jsd.object();

        for (auto it = jso.begin() ; it != jso.end() ; ++it) {
            ui->m_conf_cb->addItem(it.key());
        }
    }
}

void BOMExportDialog::on_m_all_rb_clicked() {
    updateQueryLine();
}

void BOMExportDialog::on_m_terminal_rb_clicked() {
    updateQueryLine();
}

void BOMExportDialog::on_m_button_rb_clicked() {
    updateQueryLine();
}

void BOMExportDialog::on_m_format_as_nomenclature_rb_toggled(bool checked) {
    Q_UNUSED(checked)
    updateQueryLine();
}

/**
 * @brief BOMExportDialog::on_m_edit_sql_query_cb_clicked
 * Update widgets
 */
void BOMExportDialog::on_m_edit_sql_query_cb_clicked()
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
 * @brief BOMExportDialog::on_m_save_current_conf_pb_clicked
 * Save the current query to file
 */
void BOMExportDialog::on_m_save_current_conf_pb_clicked()
{
    QFile file(QETApp::configDir() + "/bill_of_materials.json");

    if (file.open(QFile::ReadWrite))
    {
        QJsonDocument jsd(QJsonDocument::fromJson(file.readAll()));
        QJsonObject root_object;

        if (!jsd.isEmpty())
        {
            root_object = jsd.object();
            if (root_object.contains(ui->m_save_name_le->text())) {
                root_object.remove(ui->m_save_name_le->text());
            }
        }

        QVariantMap vm;
        vm.insert("query", ui->m_sql_query->text());
        vm.insert("header", ui->m_include_header_cb->isChecked());
        root_object[ui->m_save_name_le->text()] = QJsonObject::fromVariantMap(vm);

        jsd.setObject(root_object);
        file.resize(0);
        file.write(jsd.toJson());
    }
}

/**
 * @brief BOMExportDialog::on_m_load_pb_clicked
 * Load the current selected query from file
 */
void BOMExportDialog::on_m_load_pb_clicked()
{
    auto name = ui->m_conf_cb->currentText();
    if (name.isEmpty()) {
        return;
    }

    QFile file(QETApp::configDir() + "/bill_of_materials.json");
    if (file.open(QFile::ReadOnly))
    {
        QJsonDocument jsd(QJsonDocument::fromJson(file.readAll()));
        QJsonObject jso = jsd.object();

        auto value = jso.value(name);
        if (value.isObject())
        {
            auto value_object = value.toObject();
            ui->m_include_header_cb->setChecked(value_object["header"].toBool());
            ui->m_sql_query->setText(value_object["query"].toString());
            ui->m_edit_sql_query_cb->setChecked(true);
            on_m_edit_sql_query_cb_clicked(); //Force to update widgets
        }
    }
}
