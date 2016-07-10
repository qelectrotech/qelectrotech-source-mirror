

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
#ifndef PROJECTCONFIGPAGES_H
#define PROJECTCONFIGPAGES_H
#include "configpage.h"
#include <QtWidgets>

class QLabel;
class QLineEdit;
class QETProject;
class BorderPropertiesWidget;
class TitleBlockPropertiesWidget;
class ConductorPropertiesWidget;
class DiagramContextWidget;
class ReportPropertieWidget;
class XRefPropertiesWidget;
class SelectAutonumW;
class QComboBox;
class QPushButton;
class FolioAutonumberingW;
class ElementAutonumberingW;

/**
	This class, derived from ConfigPage, aims at providing the basic skeleton
	for a project configuration page.
*/
class ProjectConfigPage : public ConfigPage {
	Q_OBJECT
	// Constructor, destructor
	public:
	ProjectConfigPage(QETProject *, QWidget * = 0);
	virtual ~ProjectConfigPage();
	private:
	ProjectConfigPage(const ProjectConfigPage &);
	
	// methods
	public:
	virtual QETProject *project() const;
	virtual QETProject *setProject(QETProject *project, bool = true);
	virtual void applyConf();
	/**
		Apply configuration to the project after user input. This method is
		automatically called when the ConfigDialog is validated, and only if the
		project is both non-zero and not read-only.
	*/
	virtual void applyProjectConf() = 0;
	
	protected:
	virtual void init();
	/**
		Use this pure virtual method to initialize your page widgets.
	*/
	virtual void initWidgets() = 0;
	/**
		Use this pure virtual method to initialize your page layout. This method is
		always called after initWidgets().
	*/
	virtual void initLayout() = 0;
	/**
		Use this pure virtual method to fill widgets with project values.
	*/
	virtual void readValuesFromProject()  = 0;
	/**
		Use this pure virtual method to adjust the "read only" state of your page
		widgets according to the currently edited project.
	*/
	virtual void adjustReadOnly() = 0;
	
	// attributes
	protected:
	QETProject *project_; ///< Currently edited project
};

/**
	This page enables users to configure the main properties of a project.
*/
class ProjectMainConfigPage : public ProjectConfigPage {
	Q_OBJECT
	// Constructor, destructor
	public:
	ProjectMainConfigPage(QETProject *, QWidget * = 0);
	virtual ~ProjectMainConfigPage();
	private:
	ProjectMainConfigPage(const ProjectMainConfigPage &);
	
	// methods
	public:
	QString title() const;
	QIcon icon() const;
	void applyProjectConf();
	QString projectTitle() const;
	
	protected:
	void initWidgets();
	void initLayout();
	void readValuesFromProject();
	void adjustReadOnly();
	
	// attributes
	protected:
	QLabel *title_label_;
	QLineEdit *title_value_;
	QLabel *title_information_;
	QLabel *project_variables_label_;
	DiagramContextWidget *project_variables_;
};

class ProjectAutoNumConfigPage : public ProjectConfigPage {
		Q_OBJECT

	//Methods
	public:
		ProjectAutoNumConfigPage (QETProject *project, QWidget *parent = 0);

		virtual QString title() const;
		virtual QIcon   icon() const;
		virtual void    applyProjectConf();
		virtual void    changeToTab(int);
	protected:
		virtual void initWidgets();
		virtual void initLayout();
		virtual void readValuesFromProject();
		virtual void adjustReadOnly();
	private:
		void buildConnections();
	private slots:
		void updateContext_conductor(QString);//conductor
		void saveContext_conductor();
		void removeContext_conductor();
		void updateContext_folio(QString);//folio
		void saveContext_folio();
		void removeContext_folio();
		void updateContext_element(QString);//element
		void saveContext_element();
		void removeContext_element();

		void applyAutoNum();

		void tabChanged(int);

	signals:
		void setAutoNum(QString);
		void setAutoNum(int,int);
		void saveCurrentTbp();
		void loadSavedTbp();

	//Attributes
	private:
		QTabWidget            *tab_widget;
		QWidget               *conductor_tab_widget;
		QWidget               *element_tab_widget;
		QWidget               *folio_tab_widget;
		QWidget               *autoNumbering_tab_widget;
		QScrollArea           *scrollArea;
		QLabel                *m_label_conductor;
		QLabel                *m_label_folio;
		QLabel                *m_label_element;
		QComboBox             *m_context_cb_conductor;
		QComboBox             *m_context_cb_folio;
		QComboBox             *m_context_cb_element;
		QPushButton           *m_remove_pb_conductor;
		QPushButton           *m_remove_pb_folio;
		QPushButton           *m_remove_pb_element;
		SelectAutonumW        *m_saw_conductor;
		SelectAutonumW        *m_saw_folio;
		SelectAutonumW        *m_saw_element;
		FolioAutonumberingW   *m_faw;
		ElementAutonumberingW *m_eaw;

};

#endif
