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
#ifndef PLCQUERYWIDGET_H
#define PLCQUERYWIDGET_H

#include <QWidget>

class QCheckBox;
class ConfigSaveLoaderWidget;
class QLineEdit;
class QListWidget;
class QListWidgetItem;

/**
	@brief The PlcQueryWidget class
	The content tab of the dialog which puts an SPS list on a sheet:
	which columns the list shows, and in which order. Shaped after
	SummaryQueryWidget -- two lists with arrows between them, a saved
	configuration per name, a custom SQL query to fall back on -- but the
	columns come out of qet_plc_list, one row per channel of a PLC master,
	so the query it builds always orders by master and by channel, which
	is the order the folio table keeps and breaks its pages at.
*/
class PlcQueryWidget : public QWidget
{
	Q_OBJECT

	public:
		explicit PlcQueryWidget(QWidget *parent = nullptr);
		~PlcQueryWidget();

		static QString modelIdentifier() {return QStringLiteral("plc_list");}
		QString queryStr() const;
			/// The query reading the master each row belongs to, giving
			/// one row per row of queryStr() in the same order; empty for
			/// a query written by hand, whose rows nothing of ours can
			/// follow, so that no page is broken at a master then
		QString groupQueryStr() const;
		void setQuery(const QString &query);

	private slots:
		void onAvailableItemDoubleClicked(QListWidgetItem *item);
		void onChosenItemDoubleClicked(QListWidgetItem *item);
		void addField();
		void removeField();
		void moveFieldUp();
		void moveFieldDown();
		void customQueryToggled(bool checked);
		void saveConfig();
		void loadConfig();

	private:
		void setUpItems();
		void fillSavedQuery();
		void updateQueryLine();
		void setFields(const QStringList &fields);
		QStringList selectedKeys() const;
		void reset();

		QListWidget *m_available_list = nullptr;
		QListWidget *m_choosen_list = nullptr;
		QCheckBox *m_custom_cb = nullptr;
		QLineEdit *m_query_line = nullptr;
		ConfigSaveLoaderWidget *m_config_gb = nullptr;
		QList<QListWidgetItem *> m_items_list;
		QString m_custom_query;
};

#endif // PLCQUERYWIDGET_H
