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
};

#endif
