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
#ifndef DYNAMICTEXTFIELDEDITOR_H
#define DYNAMICTEXTFIELDEDITOR_H

#include "../elementitemeditor.h"
#include "../graphicspart/partdynamictextfield.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#ifdef BUILD_WITHOUT_KF5
#else
#	include <KColorButton>
#endif

/**
	@brief The DynamicTextFieldEditor class
	This class provide a widget used to edit the property of a dynamic text field
*/
class DynamicTextFieldEditor : public ElementItemEditor {
	Q_OBJECT

  public:
	explicit DynamicTextFieldEditor(
		QETElementEditor*	  editor,
		PartDynamicTextField* text_field = nullptr,
		QWidget*			  parent	 = nullptr);
	~DynamicTextFieldEditor() override;

	bool					  setPart(CustomElementPart* part) override;
	bool					  setParts(QList<CustomElementPart*>) override;
	CustomElementPart*		  currentPart() const override;
	QList<CustomElementPart*> currentParts() const override;
	void					  updateForm() override;

  private:
	void setUpWidget(QWidget* parent = nullptr);
	void fillInfoComboBox();
	void setUpConnections();
	void disconnectConnections();

  private slots:
	void on_m_x_sb_editingFinished();
	void on_m_y_sb_editingFinished();
	void on_m_rotation_sb_editingFinished();
	void on_m_user_text_le_editingFinished();
	void on_m_size_sb_editingFinished();
	void on_m_frame_cb_clicked();
	void on_m_width_sb_editingFinished();
	void on_m_elmt_info_cb_activated(const QString& arg1);
	void on_m_text_from_cb_activated(int index);
	void on_m_composite_text_pb_clicked();
	void on_m_alignment_pb_clicked();
	void on_m_font_pb_clicked();
#ifdef BUILD_WITHOUT_KF5
#else
	void		  on_m_color_kpb_changed(const QColor& newColor);
#endif
  private:
	QPointer<PartDynamicTextField> m_text_field;
	QList<PartDynamicTextField*>   m_parts;
	QList<QMetaObject::Connection> m_connection_list;
	QComboBox*					   m_text_from_cb;
	QComboBox*					   m_elmt_info_cb;
	QDoubleSpinBox*				   m_x_sb;
	QDoubleSpinBox*				   m_y_sb;
	QSpinBox*					   m_rotation_sb;
	QLineEdit*					   m_user_text_le;
	QSpinBox*					   m_size_sb;
	QCheckBox*					   m_frame_cb;
	QSpinBox*					   m_width_sb;
	QPushButton*				   m_composite_text_pb;
	QPushButton*				   m_font_pb;
#ifdef BUILD_WITHOUT_KF5
#else
	KColorButton* m_color_kpb;
#endif
};

#endif // DYNAMICTEXTFIELDEDITOR_H
