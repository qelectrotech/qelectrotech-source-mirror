/*
	Copyright 2006-2017 The QElectroTech Team
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
#ifndef DYNAMICTEXTFIELDEDITOR_H
#define DYNAMICTEXTFIELDEDITOR_H

#include "elementitemeditor.h"
#include "partdynamictextfield.h"

namespace Ui {
	class DynamicTextFieldEditor;
}

/**
 * @brief The DynamicTextFieldEditor class
 * This class provide a widget used to edit the property of a dynamic text field
 */
class DynamicTextFieldEditor : public ElementItemEditor
{
	Q_OBJECT
	
	public:
		explicit DynamicTextFieldEditor(QETElementEditor *editor, PartDynamicTextField *text_field = nullptr, QWidget *parent = nullptr);
		~DynamicTextFieldEditor();
	
		bool setPart(CustomElementPart *part) override;
		CustomElementPart *currentPart() const override;
		void updateForm() override;
		
	private:
		void setColorPushButton(QColor color);
	
	private slots:
		void on_m_x_sb_editingFinished();
		void on_m_y_sb_editingFinished();
		void on_m_rotation_sb_editingFinished();
		void on_m_user_text_le_editingFinished();
		void on_m_size_sb_editingFinished();
		void on_m_color_pb_clicked();	
		void on_m_frame_cb_clicked();
		
		private:
		Ui::DynamicTextFieldEditor *ui;
		QPointer<PartDynamicTextField> m_text_field;
		QList<QMetaObject::Connection> m_connection_list;
};

#endif // DYNAMICTEXTFIELDEDITOR_H
