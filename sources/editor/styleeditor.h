﻿/*
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
#ifndef STYLE_EDITOR_H
#define STYLE_EDITOR_H

#include "elementitemeditor.h"

class CustomElementGraphicPart;
class QVBoxLayout;
class QCheckBox;
class QComboBox;

/**
	This class provides a widget to edit styles (color, pen style and thickness,
	filling, antialiasing) common to most primitives within the element editor.
	Its appendWidget() method makes the insertion of another widget below it
	easier.
*/
class StyleEditor : public ElementItemEditor
{
		Q_OBJECT
		// constructors, destructor
	public:
		StyleEditor(QETElementEditor *, CustomElementGraphicPart * = nullptr, QWidget * = nullptr);
		~StyleEditor() override;
	
	private:
		StyleEditor(const StyleEditor &);
	
		// attributes
	private:
		CustomElementGraphicPart *part;
		QList <CustomElementGraphicPart *> m_part_list;
		QList <CustomElementPart *> m_cep_list;
		QCheckBox *antialiasing;
		QComboBox *filling_color, *outline_color, *size_weight, *line_style;
	
		// methods
	public:
		bool setPart(CustomElementPart *) override;
        bool setParts(QList<CustomElementPart *>);
		CustomElementPart *currentPart() const override;
        QList<CustomElementPart*> currentParts() const override;

		static bool isStyleEditable (QList <CustomElementPart *> cep_list);
	
	public slots:
		void updateForm() override;
		void updatePartAntialiasing();
		void updatePartColor();
		void updatePartLineStyle();
		void updatePartLineWeight();
		void updatePartFilling();
	
	private:
		void activeConnections(bool);
		void makeUndo(const QString &undo_text, const char *property_name, const QVariant &new_value);
};
#endif
