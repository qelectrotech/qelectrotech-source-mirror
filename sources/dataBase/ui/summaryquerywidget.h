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
#ifndef SUMMARYQUERYWIDGET_H
#define SUMMARYQUERYWIDGET_H

#include <QWidget>
#include <QVariantMap>
#include <QVariantList>

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
		void usePredefined18(const QString &recipe);
		bool addCustomField(const QString &name, const QString &heading, const QString &expression, bool saveForReuse = true, const QVariantMap &formulaTree19 = QVariantMap());
		QStringList selectedFieldExpressions() const;
        QStringList selectedFieldIds19() const;
        QString validationMessage19() const;
        QList<QPair<QString,QString>> sourceFields19() const;
        void setNumberStart19(int start);
        void enableNumber19(bool enabled=true);
        void setFilter19(const QString &condition);
        void setDateFormat19(const QString &format);


    signals:
        void definitionChanged19();

	private:
		void setLegacyQuery(const QString &query);
		QString rawQueryStr() const;
		QVariantMap editorState() const;
		void restoreEditorState(const QVariantMap &state);
		void setUpItems();
		void rebuildFields18();
		QVariantList applicableFields18() const;
		void setDerivedSource18(const QString &query);
		void refreshMode18();
        void cleanFields19();
        void setupControls19();

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
		QVariantList m_custom_fields;
		QString m_custom_query, m_source_query18;
        QVariantMap m_filter_tree19;
        bool m_loading19=false;
        int m_number_start19=1;
        QString m_filter19,m_date_format19;

};

#endif // SUMMARYQUERYWIDGET_H
