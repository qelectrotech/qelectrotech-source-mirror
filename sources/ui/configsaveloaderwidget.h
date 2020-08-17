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
#ifndef CONFIGSAVELOADERWIDGET_H
#define CONFIGSAVELOADERWIDGET_H

#include <QGroupBox>

namespace Ui {
class ConfigSaveLoaderWidget;
}

/**
	@brief The ConfigSaveLoaderWidget class
	This group box provide 4 widget:
	A combo box with the available config.
	A push button to load the selected config of combo box
	A line edit to edit the text of the config to save
	A push button to save the config
*/
class ConfigSaveLoaderWidget : public QGroupBox
{
	Q_OBJECT

	public:
		explicit ConfigSaveLoaderWidget(QWidget *parent = nullptr);
		~ConfigSaveLoaderWidget();

		QString selectedText() const;
		QString text()const;
		void addItem(QString text);

	signals:
		void loadClicked();
		void saveClicked();

	private slots:
		void on_m_load_pb_clicked();
		void on_m_save_pb_clicked();

	private:
		Ui::ConfigSaveLoaderWidget *ui;
};

#endif // CONFIGSAVELOADERWIDGET_H
