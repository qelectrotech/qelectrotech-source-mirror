/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef CONDUCTOR_PROPERTIES_WIDGET_H
#define CONDUCTOR_PROPERTIES_WIDGET_H
#include "conductorproperties.h"
#include "qtextorientationspinboxwidget.h"
#include <QWidget>
/**
	This widget enables users to change the properties of a particular
	conductor; these properties include singleline symbols, style, color, and
	conductor type but exclude the path it draws.
*/
class ConductorPropertiesWidget : public QWidget {
	Q_OBJECT
	// constructors, destructor
	public:
	ConductorPropertiesWidget(QWidget * = 0);
	ConductorPropertiesWidget(const ConductorProperties &, QWidget * = 0);
	virtual ~ConductorPropertiesWidget();
	
	private:
	ConductorPropertiesWidget(const ConductorPropertiesWidget  &);
	
	// methods
	public:
	void setConductorProperties(const ConductorProperties &);
	ConductorProperties conductorProperties() const;
	bool isReadOnly() const;
	void setReadOnly(bool);
	
	private:
	void setConductorType(ConductorProperties::ConductorType);
	void setColorButton(const QColor &);
	QColor colorButton() const;
	
	public slots:
	void updatePreview();
	void updateConfig();
	void updateDisplay();
	void chooseColor();
	
	protected:
	void focusInEvent(QFocusEvent *);
	
	// private attributes
	private:
	QButtonGroup *radio_buttons;
	QRadioButton *simple;
	QRadioButton *multiline;
	QLineEdit *text_field;
	QTextOrientationSpinBoxWidget *verti_select;
	QTextOrientationSpinBoxWidget *horiz_select;
	QRadioButton *singleline;
	QCheckBox *phase_checkbox;
	QSlider *phase_slider;
	QSpinBox *phase_spinbox;
	QCheckBox *ground_checkbox;
	QCheckBox *neutral_checkbox;
	QLabel *preview;
	QPushButton *color_button;
	QComboBox *line_style;
	QCheckBox *merge_checkbox;
	QSpinBox *text_size_sb;
	
	ConductorProperties properties_;
	
	// private methods
	void buildInterface();
	void buildConnections();
	void destroyConnections();
};
#endif
