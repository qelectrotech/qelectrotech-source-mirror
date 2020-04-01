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
#ifndef BOMExportDialog_H
#define BOMExportDialog_H

#include <QDialog>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QButtonGroup>

class QListWidgetItem;
class QETProject;
class Element;

namespace Ui {
class BOMExportDialog;
}

class BOMExportDialog : public QDialog
{
        Q_OBJECT

    public:
        explicit BOMExportDialog(QETProject *project, QWidget *parent = nullptr);
        ~BOMExportDialog() override;

		virtual int exec() override;
		QStringList selectedKeys() const;
		QString translatedKeys(const QString &key) const;

    private slots:
        void on_m_add_pb_clicked();
        void on_m_remove_pb_clicked();
        void on_m_up_pb_clicked();
        void on_m_down_pb_clicked();
        void on_m_save_name_le_textChanged(const QString &arg1);
        void on_m_format_as_nomenclature_rb_toggled(bool checked);
        void on_m_edit_sql_query_cb_clicked();
        void on_m_save_current_conf_pb_clicked();
        void on_m_load_pb_clicked();

	private:
        void setUpItems();
        QString getBom();
        QString headers() const;
        bool createDataBase();
        void populateDataBase();
        QHash<QString, QString> elementInfoToString(Element *elmt) const;
        QString queryStr() const;
        void updateQueryLine();
        void fillSavedQuery();

    private:
        Ui::BOMExportDialog *ui;
        QETProject *m_project = nullptr;
        QSqlDatabase m_data_base;
		QSqlQuery m_insert_query;
        QString   m_custom_query;
		QHash <QString, QString> m_export_info;
		QButtonGroup m_button_group;
		QList <QListWidgetItem *> m_items_list;
};

#endif // BOMExportDialog_H
