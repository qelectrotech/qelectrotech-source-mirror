/*
	Copyright 2006-2015 The QElectroTech Team
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
#ifndef EXPORT_PROPERTIES_WIDGET_H
#define EXPORT_PROPERTIES_WIDGET_H
#include <QtWidgets>
#include "exportproperties.h"

/**
	This widget enables users to edit the various options available when
	exporting a project.
*/
class ExportPropertiesWidget : public QWidget {
	Q_OBJECT
	// constructors, destructor
	public:
	ExportPropertiesWidget(QWidget * = 0);
	ExportPropertiesWidget(const ExportProperties &, QWidget * = 0);
	virtual ~ExportPropertiesWidget();
	private:
	ExportPropertiesWidget(const ExportPropertiesWidget &);
	
	// methods
	public:
	void setExportProperties(const ExportProperties &);
	ExportProperties exportProperties() const;
	void setPrintingMode(bool);
	
	public slots:
	void slot_chooseADirectory();
	
	signals:
	void formatChanged();
	void exportedAreaChanged();
	void optionChanged();
	
	private:
	void build();
	
	// attributes
	private:
	QLabel *dirpath_label;
	QLineEdit *dirpath;
	QPushButton *button_browse;
	QLabel *format_label;
	QComboBox *format;
	QCheckBox *draw_grid;
	QCheckBox *draw_border;
	QCheckBox *draw_titleblock;
	QCheckBox *draw_terminals;
	QCheckBox *draw_colored_conductors;
	QRadioButton *export_border;
	QRadioButton *export_elements;
	QButtonGroup *exported_content_choices;
};
#endif
