/*
	Copyright 2006-2017 The QElectroTech Team
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

#include <QActionGroup>
#include <QMdiArea>
#include <QSignalMapper>
#include <QDir>
#include <QUndoGroup>

#include "qetmainwindow.h"
#include "searchandreplacewidget.h"

class QMdiSubWindow;
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
class AutoNumberingDockWidget;

/**
	This class represents the main window of the QElectroTech diagram editor and,
	ipso facto, the most important part of the QElectroTech user interface.
*/
class QETDiagramEditor : public QETMainWindow
{
	Q_OBJECT
	
	public:
		QETDiagramEditor(const QStringList & = QStringList(), QWidget * = nullptr);
		~QETDiagramEditor() override;
	
	private:
		QETDiagramEditor(const QETDiagramEditor &);
		
	public:
		void                 closeEvent        (QCloseEvent *) override;
		QList<ProjectView *> openedProjects    () const;
		void                 addProjectView    (ProjectView *);
		bool                 openAndAddProject (const QString &, bool = true);
		QList<QString>       editedFiles       () const;
		ProjectView         *viewForFile       (const QString &) const;
		ProjectView *currentProjectView() const;
		QETProject *currentProject() const;
		bool drawGrid() const;
		
	protected:
		bool event(QEvent *) override;

	private:
		void setUpElementsPanel ();
		void setUpElementsCollectionWidget();
		void setUpUndoStack     ();
		void setUpSelectionPropertiesEditor();
		void setUpAutonumberingWidget();
		void setUpActions       ();
		void setUpToolBar       ();
		void setUpMenu          ();
		
		bool addProject(QETProject *, bool = true);
		DiagramView *currentDiagramView() const;
		Element *currentElement() const;
		ProjectView *findProject(DiagramView *) const;
		ProjectView *findProject(Diagram *) const;
		ProjectView *findProject(QETProject *) const;
		ProjectView *findProject(const QString &) const;
		QMdiSubWindow *subWindowForWidget(QWidget *) const;

	public slots:
		void save();
		void saveAs();
		bool newProject();
		bool openProject();
		bool openRecentFile(const QString &);
		bool closeProject(ProjectView *);
		bool closeProject(QETProject *);
		void zoomGroupTriggered      (QAction *action);
		void selectGroupTriggered    (QAction *action);
		void addItemGroupTriggered   (QAction *action);
		void selectionGroupTriggered (QAction *action);
		void rowColumnGroupTriggered (QAction *action);
		void slot_updateActions();
		void slot_updateUndoStack();
		void slot_updateModeActions();
		void slot_updateComplexActions();
		void slot_updatePasteAction();
		void slot_updateWindowsMenu();
		void slot_updateAutoNumDock();
		void generateTerminalBlock();
		void setWindowedMode();
		void setTabbedMode();
		void readSettings();
		void writeSettings();
		void activateDiagram(Diagram *);
		void activateProject(QETProject *);
		void activateProject(ProjectView *);
		void activateWidget(QWidget *);
		void projectWasClosed(ProjectView *);
		void editProjectProperties(ProjectView *);
		void editProjectProperties(QETProject *);
		void editDiagramProperties(DiagramView *);
		void editDiagramProperties(Diagram *);
		void addDiagramToProject(QETProject *);
		void removeDiagram(Diagram *);
		void removeDiagramFromProject();
		void moveDiagramUp(Diagram *);
		void moveDiagramDown(Diagram *);
		void moveDiagramUpTop(Diagram *);
		void moveDiagramUpx10(Diagram *);
		void moveDiagramDownx10(Diagram *);
		void reloadOldElementPanel();
		void diagramWasAdded(DiagramView *);
		void findElementInPanel(const ElementsLocation &);
		void editElementInEditor(const ElementsLocation &);
		void editSelectedElementInEditor();
		void showError(const QETResult &);
		void showError(const QString &);
		void subWindowActivated(QMdiSubWindow *subWindows);

	private slots:
		void selectionChanged();

	public:
		QAction *m_edit_diagram_properties;      ///< Show a dialog to edit diagram properties
		QAction *m_conductor_reset;    ///< Reset paths of selected conductors
		QAction *m_cut;                ///< Cut selection to clipboard
		QAction *m_copy;               ///< Copy selection to clipboard
		
		QActionGroup m_row_column_actions_group; /// Action related to add/remove rows/column in diagram
		QActionGroup m_selection_actions_group; ///Action related to edit a selected item
		QActionGroup *m_depth_action_group = nullptr;
	
	private:
		QActionGroup *grp_visu_sel;  ///< Action group for visualisation vs edition mode
		QActionGroup *m_group_view_mode; ///< Action group for project
		QActionGroup m_add_item_actions_group; ///Action related to adding (add text image shape...)
		QActionGroup m_zoom_actions_group;	     ///Action related to zoom for diagram
		QActionGroup m_select_actions_group; ///Action related to global selections
		QActionGroup m_file_actions_group; ///Actions related to file (open, close, save...)
		
		QAction *m_tabbed_view_mode;   ///< Display projects as tabs
		QAction *m_windowed_view_mode; ///< Display projects as windows
		QAction *m_mode_selection;     ///< Set edition mode
		QAction *m_mode_visualise;     ///< Set visualisation mode
		QAction *m_export_diagram;     ///< Export diagrams of the current project as imagess
		QAction *m_print;              ///< Print diagrams of the current project
		QAction *m_quit_editor;        ///< Quit the diagram editor
		QAction *undo;               ///< Cancel the latest action
		QAction *redo;               ///< Redo the latest cancelled operation
		QAction *m_paste;              ///< Paste clipboard content on the current diagram
		QAction *m_auto_conductor;   ///< Enable/Disable the use of auto conductor
		QAction *conductor_default;  ///< Show a dialog to edit default conductor properties
		QAction *m_grey_background;  ///< Switch the background color in white or grey
		QAction *m_draw_grid;        ///< Switch the background grid display or not
		QAction *m_project_edit_properties;      ///< Edit the properties of the current project.
		QAction *m_project_add_diagram;    ///< Add a diagram to the current project.
		QAction *m_remove_diagram_from_project;    ///< Delete a diagram from the current project
		QAction *m_clean_project;          ///< Clean the content of the curent project by removing useless items
		QAction *m_project_folio_list;    ///< Sommaire des schemas
		QAction *m_project_nomenclature;   ///< generate nomenclature
		QAction *m_project_terminalBloc;   ///< generate terminal block
		QAction *m_tile_window;        ///< Show MDI subwindows as tile
		QAction *m_cascade_window;     ///< Show MDI subwindows as cascade
		QAction *m_previous_window;        ///< Switch to the previous document
		QAction *m_next_window;        ///< Switch to the next document
		QAction *m_edit_selection;	 ///< To edit selected item
		QList <QAction *> m_zoom_action_toolBar; ///Only zoom action must displayed in the toolbar
		QAction *m_delete_selection;				///< Delete selection
		QAction *m_rotate_selection;				///< Rotate selected elements and text items by 90 degrees
		QAction *m_rotate_texts;					///< Direct selected text items to a specific angle
		QAction *m_find_element;					///< Find the selected element in the panel
		QAction *m_group_selected_texts = nullptr;
		QAction *m_close_file;			   ///< Close current project file
		QAction *m_save_file;				   ///< Save current project
		QAction *m_save_file_as;			   ///< Save current project as a specific file
		QAction *m_find = nullptr;

		QMdiArea m_workspace;
		QSignalMapper windowMapper;
		QDir open_dialog_dir; /// Directory to use for file dialogs such as File > save
		QDockWidget *qdw_pa; /// Dock for the elements panel
		QDockWidget *m_qdw_elmt_collection;
		QDockWidget *qdw_undo; /// Dock for the undo list
		ElementsCollectionWidget *m_element_collection_widget;
			
		DiagramPropertiesEditorDockWidget *m_selection_properties_editor;
			/// Elements panel
		ElementsPanelWidget *pa;
		QMenu *windows_menu;
		
		QToolBar *main_tool_bar       = nullptr,
				 *view_tool_bar       = nullptr,
				 *diagram_tool_bar    = nullptr,
				 *m_add_item_tool_bar = nullptr,
				 *m_depth_tool_bar    = nullptr;
		
		QUndoGroup undo_group;
		AutoNumberingDockWidget *m_autonumbering_dock;
		int activeSubWindowIndex;
		bool m_first_show = true;
		SearchAndReplaceWidget m_search_and_replace_widget;
};
#endif
