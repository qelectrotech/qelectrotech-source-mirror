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
#ifndef SUMMARYQUERYWIDGET_H
#define SUMMARYQUERYWIDGET_H

#include <QWidget>

class QListWidgetItem;

namespace Ui {
class SummaryQueryWidget;
}

class SummaryQueryWidget : public QWidget
{
	Q_OBJECT

	public:
		explicit SummaryQueryWidget(QWidget *parent = nullptr);
		~SummaryQueryWidget();

		static QString modelIdentifier() {return "summary";}
		QString queryStr() const;
		void setQuery(const QString &query);

	private:
		void setUpItems();
		void fillSavedQuery();
		void updateQueryLine();
		QStringList selectedKeys() const;

	private slots:
		void on_m_available_list_itemDoubleClicked(QListWidgetItem *item);
		void on_m_choosen_list_itemDoubleClicked(QListWidgetItem *item);
		void on_m_up_pb_clicked();
		void on_m_add_pb_clicked();
		void on_m_remove_pb_clicked();
		void on_m_down_pb_clicked();
		void on_m_edit_sql_query_cb_clicked();
		void reset();
		void saveConfig();
		void loadConfig();

	private:
		Ui::SummaryQueryWidget *ui;
		QList <QListWidgetItem *> m_items_list;
		QString m_custom_query;
};

#endif // SUMMARYQUERYWIDGET_H
