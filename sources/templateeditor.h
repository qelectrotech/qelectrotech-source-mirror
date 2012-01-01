/*
	Copyright 2006-2012 Xavier Guerrin
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
#ifndef TEMPLATE_EDITOR_H
#define TEMPLATE_EDITOR_H
#include <QtGui>
#include <QtXml>
class QETProject;
/**
	This class allows the user to edit a title block template.
	For the moment, it simply provides a text editor.
*/
class TemplateEditor : public QWidget {
	Q_OBJECT
	
	// constructors, destructor
	public:
	TemplateEditor(QWidget * = 0, Qt::WindowFlags = 0);
	virtual ~TemplateEditor();
	private:
	TemplateEditor(const TemplateEditor &);
	
	// method\s
	public:
	bool edit(QETProject *, const QString &);
	
	private slots:
	void validate();
	void save();
	void quit();
	void integrateLogo();
	
	private:
	void build();
	void updateProjectLabel();
	QString getXmlString() const;
	void setXmlString(const QDomDocument &);
	
	// attributes
	private:
	QLabel *parent_project_label_;
	QLabel *static_xml_1_;
	QLabel *static_xml_2_;
	QLabel *static_xml_3_;
	QLineEdit *template_name_edit_;
	QTextEdit *template_xml_edit_;
	QPushButton *integrate_logo_;
	QPushButton *validate_button_;
	QPushButton *save_button_;
	QPushButton *quit_button_;
	QETProject *parent_project_;
	QString template_name_;
};
#endif
