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
#ifndef GRAPHICSTABLEPROPERTIESEDITOR_H
#define GRAPHICSTABLEPROPERTIESEDITOR_H

#include "PropertiesEditor/propertieseditorwidget.h"
#include <QPointer>
#include <QButtonGroup>

namespace Ui {
class GraphicsTablePropertiesEditor;
}

class QetGraphicsTableItem;
class QAbstractItemModel;
class QUndoStack;
class QButtonGroup;

/**
	@brief The GraphicsTablePropertiesEditor class
	This widget is used to edit the property of both QetGraphicsTableItem and QetGraphicsHeaderItem
*/
class GraphicsTablePropertiesEditor : public PropertiesEditorWidget
{
	Q_OBJECT

	public:
		explicit GraphicsTablePropertiesEditor(QetGraphicsTableItem *table = nullptr, QWidget *parent = nullptr);
		~GraphicsTablePropertiesEditor() override;

		void setTable(QetGraphicsTableItem *table);
		virtual void apply() override;
		QUndoCommand * associatedUndo() const override;
		virtual bool setLiveEdit(bool live_edit) override;

	private slots:
		void on_m_header_font_pb_clicked();
		void on_m_table_font_pb_clicked();
		virtual void updateUi() override;
		void updateInfoLabel();
		void on_m_table_name_le_textEdited(const QString &arg1);
		void on_m_previous_table_cb_activated(int index);
		void on_m_previous_pb_clicked();
		void on_m_next_pb_clicked();
		void on_m_auto_geometry_pb_clicked();
		void on_m_apply_geometry_to_linked_table_pb_clicked();

	private:
		void setUpEditConnection();

	private:
		Ui::GraphicsTablePropertiesEditor *ui;
		QPointer<QetGraphicsTableItem> m_table_item;
		QList <QMetaObject::Connection> m_connect_list,
										m_edit_connection;
		QButtonGroup *m_header_button_group = nullptr,
					 *m_table_button_group = nullptr;
		QWidget *m_current_model_editor = nullptr;
		QVector<QetGraphicsTableItem *> m_other_table_vector;
};

Q_DECLARE_METATYPE(QMargins)

#endif // GRAPHICSTABLEPROPERTIESEDITOR_H
