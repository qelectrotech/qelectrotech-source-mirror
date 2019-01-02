/*
	Copyright 2006-2018 The QElectroTech Team
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
#ifndef NAMELISTWIDGET_H
#define NAMELISTWIDGET_H

#include "nameslist.h"
#include <QWidget>

namespace Ui {
	class NameListWidget;
}

/**
 * @brief The NameListWidget class
 * Provide a widget for let user define localized string;
 */
class NameListWidget : public QWidget
{
	Q_OBJECT
	
	public:
		explicit NameListWidget(QWidget *parent = nullptr);
		~NameListWidget();
		
		void addLine();
		void setNames (const NamesList &name_list);
		NamesList names() const;
		void setReadOnly(bool ro);
		bool isEmpty() const;
		void setClipboardValue (QHash <QString, QString> value);
	
	private slots:
		void on_m_clipboard_cb_activated(int index);
		
	private:
		void clean();
	
	private:
		Ui::NameListWidget *ui;
		bool m_read_only = false;	
};

#endif // NAMELISTWIDGET_H
