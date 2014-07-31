

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
#ifndef PROJECTCONFIGPAGES_H
#define PROJECTCONFIGPAGES_H
#include "configpage.h"
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

/**
	This page enables users to configure the default properties of diagrams
	newly added to the edited project.
*/
class ProjectNewDiagramConfigPage : public ProjectConfigPage {
	Q_OBJECT
	// Constructor, destructor
	public:
	ProjectNewDiagramConfigPage(QETProject *, QWidget * = 0);
	virtual ~ProjectNewDiagramConfigPage();
	private:
	ProjectNewDiagramConfigPage(const ProjectNewDiagramConfigPage &);
	
	// methods
	public:
	QString title() const;
	QIcon icon() const;
	void applyProjectConf();
	
	protected:
	void initWidgets();
	void initLayout();
	void readValuesFromProject();
	void adjustReadOnly();
	
	// attributes
	private:
	QLabel *informative_label_;
	BorderPropertiesWidget *border_;
	TitleBlockPropertiesWidget *titleblock_;
	ConductorPropertiesWidget *conductor_;
	ReportPropertieWidget *report_;
	XRefPropertiesWidget *xref_;
};

class ProjectAutoNumConfigPage : public ProjectConfigPage {
		Q_OBJECT

	//Methods
	public:
		ProjectAutoNumConfigPage (QETProject *project, QWidget *parent = 0);

		virtual QString title() const;
		virtual QIcon   icon() const;
		virtual void    applyProjectConf();

	protected:
		virtual void initWidgets();
		virtual void initLayout();
		virtual void readValuesFromProject();
		virtual void adjustReadOnly();

	private:
		void buildConnections();

	private slots:
		void updateContext(QString);
		void saveContext();

	//Attributes
	private:
		QLabel         *m_label;
		QLineEdit      *m_name_le;
		QComboBox      *m_context_cb;
		SelectAutonumW *m_saw;

};

#endif
