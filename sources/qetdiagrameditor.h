/*
	Copyright 2006-2025 The QElectroTech Team
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

#include "SearchAndReplace/ui/searchandreplacewidget.h"
#include "qetmainwindow.h"

#include <QActionGroup>
#include <QCloseEvent>
#include <QDir>
#include <QMdiArea>
#include <QSignalMapper>
#include <QUndoGroup>

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

#ifdef BUILD_WITHOUT_KF6
#else
class KAutoSaveFile;
#endif
/**
	This class represents the main window of the QElectroTech diagram editor and,
	ipso facto, the most important part of the QElectroTech user interface.
*/
class QETDiagramEditor : public QETMainWindow
{
	Q_OBJECT
	
	public:
		QETDiagramEditor(
				const QStringList & = QStringList(),
				QWidget * = nullptr);
		~QETDiagramEditor() override;
		void                 closeEvent        (QCloseEvent *) override;
		QList<ProjectView *> openedProjects    () const;
		void                 addProjectView    (ProjectView *);
		bool                 openAndAddProject (const QString &, bool = true);
		QList<QString>       editedFiles       () const;
		ProjectView         *viewForFile       (const QString &) const;
		ProjectView *currentProjectView() const;
		QETProject *currentProject() const;
		bool drawGrid() const;
#ifdef BUILD_WITHOUT_KF6
#else
		void openBackupFiles (QList<KAutoSaveFile *> backup_files);
#endif

	  protected:
		bool event(QEvent *) override;
	private:
		QETDiagramEditor(const QETDiagramEditor &);
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
		void showError(const QETResult &);
		void showError(const QString &);
		void subWindowActivated(QMdiSubWindow *subWindows);

	private slots:
		void selectionChanged();

	public:
		QAction
		*m_edit_diagram_properties, ///< Show a dialog to edit diagram properties
		*m_conductor_reset,         ///< Reset paths of selected conductors
		*m_cut,                     ///< Cut selection to clipboard
		*m_copy;                    ///< Copy selection to clipboard
		
		QActionGroup
		m_row_column_actions_group, /// Action related to add/remove rows/column in diagram
		m_selection_actions_group,  ///Action related to edit a selected item
		*m_depth_action_group = nullptr;
	
	private:
		QActionGroup
		*grp_visu_sel,            ///< Action group for visualisation vs edition mode
		*m_group_view_mode,       ///< Action group for project
		m_add_item_actions_group, ///Action related to adding (add text image shape...)
		m_zoom_actions_group,     ///Action related to zoom for diagram
		m_select_actions_group,   ///Action related to global selections
		m_file_actions_group;     ///Actions related to file (open, close, save...)
		
		QAction
		*m_tabbed_view_mode,		///< Display projects as tabs
		*m_windowed_view_mode,		///< Display projects as windows
		*m_mode_selection,		///< Set edition mode
		*m_mode_visualise,		///< Set visualisation mode
		*m_export_to_images,		///< Export diagrams of the current project as imagess
		*m_export_to_pdf = nullptr, ///< Export project to pdf.
		*m_print,			///< Print diagrams of the current project
		*m_quit_editor,			///< Quit the diagram editor
		*undo,				///< Cancel the latest action
		*redo,				///< Redo the latest cancelled operation
		*m_paste,			///< Paste clipboard content on the current diagram
		*m_auto_conductor,		///< Enable/Disable the use of auto conductor
		*conductor_default,		///< Show a dialog to edit default conductor properties
		*m_grey_background,		///< Switch the background color in white or grey
		*m_draw_grid,			///< Switch the background grid display or not
		*m_project_edit_properties,	///< Edit the properties of the current project.
		*m_project_add_diagram,		///< Add a diagram to the current project.
		*m_remove_diagram_from_project,	///< Delete a diagram from the current project
		*m_clean_project,		///< Clean the content of the current project by removing useless items
		*m_project_folio_list,		///< Sommaire des schemas
		*m_csv_export,			///< generate nomenclature
		*m_add_nomenclature,		///< Add nomenclature graphics item;
		*m_add_summary,			///<Add summary graphics item
		*m_terminal_strip_dialog = nullptr, ///<Launch terminal strip dialog
		*m_project_terminalBloc,	///< generate terminal block
		*m_project_export_conductor_num,///<Export the wire num to csv
#ifdef QET_EXPORT_PROJECT_DB
		*m_export_project_db,		///Export to file the internal database of the current project
#endif
		*m_tile_window,			///< Show MDI subwindows as tile
		*m_cascade_window,		///< Show MDI subwindows as cascade
		*m_previous_window,		///< Switch to the previous document
		*m_next_window,			///< Switch to the next document
		*m_edit_selection,		///< To edit selected item
		*m_delete_selection,		///< Delete selection
		*m_rotate_selection,		///< Rotate selected elements and text items by 90 degrees
		*m_rotate_texts,		///< Direct selected text items to a specific angle
		*m_find_element,		///< Find the selected element in the panel
		*m_group_selected_texts = nullptr,
		*m_close_file,			///< Close current project file
		*m_save_file,			///< Save current project
		*m_save_file_as,		///< Save current project as a specific file
		*m_find = nullptr;

		QList <QAction *> m_zoom_action_toolBar; ///Only zoom action must displayed in the toolbar

		QMdiArea m_workspace;
		QSignalMapper windowMapper;
		QDir open_dialog_dir; /// Directory to use for file dialogs such as File > save

		QDockWidget
		*qdw_pa, /// Dock for the elements panel
		*m_qdw_elmt_collection,
		*qdw_undo; /// Dock for the undo list

		ElementsCollectionWidget *m_element_collection_widget;
			
		DiagramPropertiesEditorDockWidget *m_selection_properties_editor;
			/// Elements panel
		ElementsPanelWidget *pa;
		QMenu *windows_menu;
		
		QToolBar
		*main_tool_bar       = nullptr,
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
