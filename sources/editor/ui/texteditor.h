/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef TEXTEDITOR_H
#define TEXTEDITOR_H

#include "../elementitemeditor.h"

#include <QPointer>
#include <QWidget>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include "../../colorbutton.h"
class PartText;

class TextEditor : public ElementItemEditor {
	Q_OBJECT

	public:
		explicit TextEditor(QETElementEditor *editor,  PartText *text = nullptr, QWidget *parent = nullptr);
		~TextEditor() override;

		void updateForm() override;
		bool setPart(CustomElementPart *part) override;
		bool setParts(QList <CustomElementPart *>) override;
		CustomElementPart *currentPart() const override;
		QList<CustomElementPart*> currentParts() const override;

	private slots:
		void on_m_font_pb_clicked();
		void on_m_color_pb_changed(const QColor &newColor);
	private:
		void setUpWidget(QWidget* parent = nullptr);
		void setUpEditConnection();
		void setUpChangeConnection(QPointer<PartText> part);
		void disconnectChangeConnection();
		void disconnectEditConnection();

	private:
		QPointer <PartText> m_text;
		QList<PartText*> m_parts;
		QList <QMetaObject::Connection> m_edit_connection;
		QList <QMetaObject::Connection> m_change_connection;
		QSpinBox *m_y_sb;
		QSpinBox *m_rotation_sb;
		QSpinBox *m_x_sb;
		QSpinBox *m_size_sb;
		QLineEdit *m_line_edit;
		QPushButton *m_font_pb;
		ColorButton *m_color_pb;
};

#endif // TEXTEDITOR_H
