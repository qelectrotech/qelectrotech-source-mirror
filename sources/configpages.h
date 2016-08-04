/*
	Copyright 2006-2016 The QElectroTech Team
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
#ifndef CONFIG_PAGES_H
#define CONFIG_PAGES_H
#include <QtWidgets>
#include "configpage.h"
#include "projectpropertiesdialog.h"
#include "titleblockpropertieswidget.h"
class BorderPropertiesWidget;
class ConductorPropertiesWidget;
class TitleBlockPropertiesWidget;
class ExportPropertiesWidget;
class ReportPropertieWidget;
class XRefPropertiesWidget;
class QETProject;
class TitleBlockProperties;

/**
	This configuration page enables users to define the properties of new
	diagrams to come.
*/
class NewDiagramPage : public ConfigPage {
	Q_OBJECT
	// constructors, destructor
	public:
	NewDiagramPage(QETProject *project = 0, QWidget * = 0, ProjectPropertiesDialog *teste = NULL);
	virtual ~NewDiagramPage();
	private:
	NewDiagramPage(const NewDiagramPage &);
public slots:
	void changeToAutoFolioTab();
	void setFolioAutonum(QString);
	void saveCurrentTbp();
	void loadSavedTbp();

	
	// methods
	public:
	void applyConf();
	QString title() const;
	QIcon icon() const;
	
	// attributes
	private:
	ProjectPropertiesDialog    *ppd_;
	QETProject                 *m_project; ///< Project to edit propertie
	BorderPropertiesWidget     *bpw;       ///< Widget to edit default diagram dimensions
	TitleBlockPropertiesWidget *ipw;       ///< Widget to edit default title block properties
	ConductorPropertiesWidget  *cpw;       ///< Widget to edit default conductor properties
	ReportPropertieWidget      *rpw;       ///< Widget to edit default report label
	XRefPropertiesWidget       *xrefpw;	   ///< Widget to edit default xref properties
	TitleBlockProperties       savedTbp;   ///< Used to save current TBP and retrieve later

};

/**
	This configuration page enables users to specify various options,most of
	them applying to the whole application.
*/
class GeneralConfigurationPage : public ConfigPage {
	Q_OBJECT
	// constructors, destructor
	public:
	GeneralConfigurationPage(QWidget * = 0);
	virtual ~GeneralConfigurationPage();
	private:
	GeneralConfigurationPage(const GeneralConfigurationPage &);
	
	// methods
	public:
	void applyConf();
	QString title() const;
	QIcon icon() const;

	private:
	void fillLang(QSettings &);
	
	// attributes
	private:
	QLabel *title_label_;
	QFrame *horiz_line_;
	QGroupBox *appearance_;
	QCheckBox *use_system_colors_;
	QCheckBox *use_trackpad_;
	QCheckBox *save_label_paste_;
	QGroupBox *projects_view_mode_;
	QRadioButton *windowed_mode_;
	QRadioButton *tabbed_mode_;
	QCheckBox *m_zoom_out_beyond_folio;
	QLabel *warning_view_mode_;
	QGroupBox *elements_management_;
	QCheckBox *highlight_integrated_elements_;
	QLabel    *default_element_infos_label_;
	QTextEdit *default_element_infos_textfield_;
	QGroupBox *lang_group_box;
	QComboBox *lang_combo_box;
	QLabel *lang_label;
};

/**
	This configuration page enables users to set default export options.
*/
class ExportConfigPage : public ConfigPage {
	Q_OBJECT
	// constructors, destructor
	public:
	ExportConfigPage(QWidget * = 0);
	virtual ~ExportConfigPage();
	private:
	ExportConfigPage(const ExportConfigPage &);
	
	// methods
	public:
	void applyConf();
	QString title() const;
	QIcon icon() const;
	
	// attributes
	public:
	ExportPropertiesWidget *epw;
};

/**
	This configuration page enables users to set default printing options.
*/
class PrintConfigPage : public ConfigPage {
	Q_OBJECT
	// constructors, destructor
	public:
	PrintConfigPage(QWidget * = 0);
	virtual ~PrintConfigPage();
	private:
	PrintConfigPage(const PrintConfigPage &);
	
	// methods
	public:
	void applyConf();
	QString title() const;
	QIcon icon() const;
	
	// attributes
	public:
	ExportPropertiesWidget *epw;
};
#endif
