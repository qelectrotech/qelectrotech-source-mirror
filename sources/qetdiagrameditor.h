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
#ifndef QET_DIAGRAM_EDITOR_H
#define QET_DIAGRAM_EDITOR_H
#include <QtWidgets>
#include "qetmainwindow.h"

class QETProject;
class QETResult;
class ProjectView;
class CustomElement;
class Diagram;
class DiagramView;
class Element;
class ElementsPanelWidget;
class ElementsLocation;
class RecentFiles;
class DiagramPropertiesEditorDockWidget;
class ElementsCollectionWidget;

/**
	This class represents the main window of the QElectroTech diagram editor and,
	ipso facto, the most important part of the QElectroTech user interface.
*/
class QETDiagramEditor : public QETMainWindow {
	Q_OBJECT
	
	// constructors, destructor
	public:
		QETDiagramEditor(const QStringList & = QStringList(), QWidget * = 0);
		virtual ~QETDiagramEditor();
	
	private:
		QETDiagramEditor(const QETDiagramEditor &);

	private:
		void setUpElementsPanel ();
		void setUpElementsCollectionWidget();
		void setUpUndoStack     ();
		void setUpSelectionPropertiesEditor();
		void setUpActions       ();
		void setUpToolBar       ();
		void setUpMenu          ();
	
		// methods
	public:
		void                 closeEvent        (QCloseEvent *);
		QList<ProjectView *> openedProjects    () const;
		void                 addProjectView    (ProjectView *);
		bool                 openAndAddProject (const QString &, bool = true, bool = true);
		QList<QString>       editedFiles       () const;
		ProjectView         *viewForFile       (const QString &) const;
		ProjectView         *acessCurrentProject ();
		DiagramView         *acessCurrentDiagramView ();

	protected:
	virtual bool event(QEvent *);
	
	private:
	bool addProject(QETProject *, bool = true);
	ProjectView *currentProject() const;
	DiagramView *currentDiagram() const;
	Element *currentElement() const;
	CustomElement * currentCustomElement() const;
	ProjectView *findProject(DiagramView *) const;
	ProjectView *findProject(Diagram *) const;
	ProjectView *findProject(QETProject *) const;
	ProjectView *findProject(const QString &) const;
	QMdiSubWindow *subWindowForWidget(QWidget *) const;

	public slots:
		void printDialog();
		void exportDialog();
		void save();
		void saveAs();
		bool newProject();
		bool openProject();
		bool openRecentFile(const QString &);
		bool closeProject(ProjectView *);
		bool closeProject(QETProject *);
		bool closeCurrentProject();
		void slot_cut();
		void slot_copy();
		void slot_paste();
		void zoomGroupTriggered      (QAction *action);
		void selectGroupTriggered    (QAction *action);
		void addItemGroupTriggered   (QAction *action);
		void selectionGroupTriggered (QAction *action);
		void rowColumnGroupTriggered (QAction *action);
		void slot_setSelectionMode();
		void slot_setVisualisationMode();
		void slot_updateActions();
		void slot_updateUndoStack();
		void slot_updateModeActions();
		void slot_updateComplexActions();
		void slot_updatePasteAction();
		void slot_updateWindowsMenu();
		void editSelectionProperties();
		void slot_resetConductors();
		void slot_autoConductor(bool ac);
		void setWindowedMode();
		void setTabbedMode();
		void readSettings();
		void writeSettings();
		void activateDiagram(Diagram *);
		void activateProject(QETProject *);
		void activateProject(ProjectView *);
		void activateWidget(QWidget *);
		void projectWasClosed(ProjectView *);
		void editCurrentProjectProperties();
		void editProjectProperties(ProjectView *);
		void editProjectProperties(QETProject *);
		void editCurrentDiagramProperties();
		void editDiagramProperties(DiagramView *);
		void editDiagramProperties(Diagram *);
		void addDiagramToProject();
		void addDiagramFolioListToProject();
		void addDiagramToProject(QETProject *);
		void removeDiagram(Diagram *);
		void removeDiagramFromProject();
		void moveDiagramUp(Diagram *);
		void moveDiagramDown(Diagram *);
		void moveDiagramUpTop(Diagram *);
		void moveDiagramUpx10(Diagram *);
		void moveDiagramDownx10(Diagram *);
		void cleanCurrentProject();
		void nomenclatureProject();
		void diagramWasAdded(DiagramView *);
		void findElementInPanel(const ElementsLocation &);
		void editElementInEditor(const ElementsLocation &);
		void editSelectedElementInEditor();
		void showError(const QETResult &);
		void showError(const QString &);
		void subWindowActivated(QMdiSubWindow *subWindows);

	private slots:
		void selectionChanged();
		void activeUndoStackCleanChanged (bool clean);
	
		// attributes
	public:
		// Actions reachable through menus within QElectroTech
		QActionGroup *grp_visu_sel;  ///< Action group for visualisation vs edition mode
		QActionGroup *grp_view_mode; ///< Action group for project
		QAction *tabbed_view_mode;   ///< Display projects as tabs
		QAction *windowed_view_mode; ///< Display projects as windows
		QAction *mode_selection;     ///< Set edition mode
		QAction *mode_visualise;     ///< Set visualisation mode
		QAction *export_diagram;     ///< Export diagrams of the current project as imagess
		QAction *print;              ///< Print diagrams of the current project
		QAction *quit_editor;        ///< Quit the diagram editor
		QAction *undo;               ///< Cancel the latest action
		QAction *redo;               ///< Redo the latest cancelled operation
		QAction *cut;                ///< Cut selection to clipboard
		QAction *copy;               ///< Copy selection to clipboard
		QAction *paste;              ///< Paste clipboard content on the current diagram
		QAction *conductor_reset;    ///< Reset paths of selected conductors
		QAction *m_auto_conductor;   ///< Enable/Disable the use of auto conductor
		QAction *conductor_default;  ///< Show a dialog to edit default conductor properties
		QAction *m_grey_background;  ///< Switch the background color in white or grey
		QAction *infos_diagram;      ///< Show a dialog to edit diagram properties
		QAction *prj_edit_prop;      ///< Edit the properties of the current project.
		QAction *prj_add_diagram;    ///< Add a diagram to the current project.
		QAction *prj_del_diagram;    ///< Delete a diagram from the current project
		QAction *prj_clean;          ///< Clean the content of the curent project by removing useless items
		QAction *prj_diagramList;    ///< Sommaire des schemas
		QAction *prj_nomenclature;   ///< generate nomenclature
		QAction *tile_window;        ///< Show MDI subwindows as tile
		QAction *cascade_window;     ///< Show MDI subwindows as cascade
		QAction *prev_window;        ///< Switch to the previous document
		QAction *next_window;        ///< Switch to the next document
		QAction *edit_selection;	 ///< To edit selected item

		QActionGroup m_add_item_actions_group; ///Action related to adding (add text image shape...)

		QActionGroup m_zoom_actions_group;	     ///Action related to zoom for diagram
		QList <QAction *> m_zoom_action_toolBar; ///Only zoom action must displayed in the toolbar

		QActionGroup m_select_actions_group; ///Action related to global selections

		QActionGroup m_selection_actions_group; ///Action related to edit a selected item
		QAction *delete_selection;				///< Delete selection
		QAction *rotate_selection;				///< Rotate selected elements and text items by 90 degrees
		QAction *rotate_texts;					///< Direct selected text items to a specific angle
		QAction *find_element;					///< Find the selected element in the panel

		QActionGroup m_row_column_actions_group; /// Action related to add/remove rows/column in diagram

		QActionGroup m_file_actions_group; ///Actions related to file (open, close, save...)
		QAction *close_file;			   ///< Close current project file
		QAction *save_file;				   ///< Save current project
		QAction *save_file_as;			   ///< Save current project as a specific file

	private:
		QMdiArea workspace;
		QSignalMapper windowMapper;
		/// Directory to use for file dialogs such as File > save
		QDir open_dialog_dir;
		/// Dock for the elements panel
		QDockWidget *qdw_pa;
		QDockWidget *m_qdw_elmt_collection;
		ElementsCollectionWidget *m_element_collection_widget;
		/// Dock for the undo list
		QDockWidget *qdw_undo;
		DiagramPropertiesEditorDockWidget *m_selection_properties_editor;
		/// Elements panel
		ElementsPanelWidget *pa;
		QMenu *windows_menu;
		QToolBar *main_bar;
		QToolBar *view_bar;
		QToolBar *diagram_bar;
		QToolBar *m_add_item_toolBar;
		QUndoGroup undo_group;
};
#endif
