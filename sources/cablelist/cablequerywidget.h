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
#ifndef CABLEQUERYWIDGET_H
#define CABLEQUERYWIDGET_H

#include <QWidget>

class ConfigSaveLoaderWidget;
class QListWidget;
class QListWidgetItem;
class QPushButton;

/**
	@brief The CableQueryWidget class
	The content tab of the dialog which puts a cable list on a sheet:
	which columns the list shows, and in which order. Shaped after
	SummaryQueryWidget -- two lists with arrows between them, a saved
	configuration per name -- but the columns are chosen out of the
	fixed set of CableList instead of out of a SQL query, so there is no
	query line to edit.
*/
class CableQueryWidget : public QWidget
{
	Q_OBJECT

	public:
		explicit CableQueryWidget(QWidget *parent = nullptr);
		~CableQueryWidget();

		static QString modelIdentifier() {return "cable_list";}
			/// The chosen columns, in order
		QStringList selectedKeys() const;
			/// Put exactly these columns in the right list, in that
			/// order; keys this list does not know are ignored
		void setFields(const QStringList &fields);

	private slots:
		void on_m_available_list_itemDoubleClicked(QListWidgetItem *item);
		void on_m_choosen_list_itemDoubleClicked(QListWidgetItem *item);
		void addField();
		void removeField();
		void moveFieldUp();
		void moveFieldDown();
		void saveConfig();
		void loadConfig();

	private:
		void setUpItems();
		void fillSavedQuery();

		QListWidget *m_available_list = nullptr;
		QListWidget *m_choosen_list = nullptr;
		ConfigSaveLoaderWidget *m_config_gb = nullptr;
};

#endif // CABLEQUERYWIDGET_H
