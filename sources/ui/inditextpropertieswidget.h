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
#ifndef INDITEXTPROPERTIESWIDGET_H
#define INDITEXTPROPERTIESWIDGET_H

#include "PropertiesEditor/propertieseditorwidget.h"
#include <QPointer>
class IndependentTextItem;

namespace Ui {
	class IndiTextPropertiesWidget;
}

/**
 * @brief The IndiTextPropertiesWidget class
 * This widget is used to edit the properties of one or several independent text item
 */
class IndiTextPropertiesWidget : public PropertiesEditorWidget
{
	Q_OBJECT
	
	public:
		IndiTextPropertiesWidget(IndependentTextItem *text = nullptr, QWidget *parent = nullptr);
		IndiTextPropertiesWidget(QList <IndependentTextItem *> text_list, QWidget *parent = nullptr);
		~IndiTextPropertiesWidget() override;
		void setText (IndependentTextItem *text);
		void setText (QList<IndependentTextItem *> text_list);
		
		void apply() override;
		bool setLiveEdit(bool live_edit) override;
		QUndoCommand* associatedUndo() const override;
		
	private slots:
		void on_m_advanced_editor_pb_clicked();	
		void on_m_break_html_pb_clicked();
		void on_m_font_pb_clicked();

		private:
		void setUpEditConnection();
		void updateUi() override;
		
	private:
		Ui::IndiTextPropertiesWidget *ui;
		QPointer <IndependentTextItem> m_text;
		QList <QPointer<IndependentTextItem>> m_text_list;
		QList <QMetaObject::Connection> m_connect_list,
										m_edit_connection;
		QFont m_selected_font;
		bool m_font_is_selected = false;
};

#endif // INDITEXTPROPERTIESWIDGET_H
