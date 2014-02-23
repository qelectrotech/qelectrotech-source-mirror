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
#ifndef QET_DIAGRAM_EDITOR_H
#define QET_DIAGRAM_EDITOR_H
#include <QtGui>
#include "qetmainwindow.h"
#include "borderproperties.h"
#include "conductorproperties.h"
#include "titleblockproperties.h"
#include "exportproperties.h"
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
	
	// methods
	public:
	void closeEvent(QCloseEvent *);
	QList<ProjectView *> openedProjects() const;
	void addProjectView(ProjectView *);
	bool openAndAddProject(const QString &, bool = true, bool = true);
	QList<DiagramView *> projectViews() const;
	QList<QString> editedFiles() const;
	ProjectView *viewForFile(const QString &) const;
	static TitleBlockProperties     defaultTitleBlockProperties();
	static BorderProperties    defaultBorderProperties();
	static ConductorProperties defaultConductorProperties();
	static ExportProperties    defaultExportProperties();
	static ExportProperties    defaultPrintProperties();
	static QString defaultReportProperties();
	
	protected:
	void actions();
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
	
	void menus();
	void toolbar();
	
	public slots:
	void printDialog();
	void exportDialog();
	void save();
	void saveAs();
	void saveCurrentDiagram();
	bool newProject();
	bool openProject();
	bool openRecentFile(const QString &);
	bool closeProject(ProjectView *);
	bool closeProject(QETProject *);
	bool closeCurrentProject();
	void slot_cut();
	void slot_copy();
	void slot_paste();
	void slot_zoomIn();
	void slot_zoomOut();
	void slot_zoomFit();
	void slot_zoomContent();
	void slot_zoomReset();
	void slot_selectAll();
	void slot_selectNothing();
	void slot_selectInvert();
	void slot_delete();
	void slot_rotate();
	void slot_rotateTexts();
	void slot_setSelectionMode();
	void slot_setVisualisationMode();
	void slot_updateActions();
	void slot_updateUndoStack();
	void slot_updateModeActions();
	void slot_updateComplexActions();
	void slot_updatePasteAction();
	void slot_updateWindowsMenu();
	void slot_addColumn();
	void slot_removeColumn();
	void slot_addRow();
	void slot_removeRow();
	void editSelectionProperties();
	void slot_editConductor();
	void slot_resetConductors();
	void slot_addText();
	void slot_addImage();
	void slot_addLine();
	void slot_addRectangle();
	void slot_addEllipse();
	void slot_editSelection();
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
	void cleanCurrentProject();
	void diagramNumProject();
	void nomenclatureProject();
	void diagramWasAdded(DiagramView *);
	void diagramIsAboutToBeRemoved(DiagramView *);
	void diagramWasRemoved(DiagramView *);
	void findElementInPanel(const ElementsLocation &);
	void findSelectedElementInPanel();
	void editElementInEditor(const ElementsLocation &);
	void editSelectedElementInEditor();
	void showError(const QETResult &);
	void showError(const QString &);
	
	// attributes
	public:
	// Actions reachable through menus within QElectroTech
	QActionGroup *grp_visu_sel;  ///< Action group for visualisation vs edition mode
	QActionGroup *grp_view_mode; ///< Action group for project
	QAction *tabbed_view_mode;   ///< Display projects as tabs
	QAction *windowed_view_mode; ///< Display projects as windows
	QAction *mode_selection;     ///< Set edition mode
	QAction *mode_visualise;     ///< Set visualisation mode
	QAction *new_file;           ///< Create new project file
	QAction *open_file;          ///< Open project file
	QAction *close_file;         ///< Close current project file
	QAction *save_file;          ///< Save current project
	QAction *save_file_as;       ///< Save current project as a specific file
	QAction *save_cur_diagram;   ///< Save current diagram of the current project only
	QAction *import_diagram;     ///< Importe an existing diagram (not implemented)
	QAction *export_diagram;     ///< Export diagrams of the current project as imagess
	QAction *print;              ///< Print diagrams of the current project
	QAction *quit_editor;        ///< Quit the diagram editor
	QAction *undo;               ///< Cancel the latest action
	QAction *redo;               ///< Redo the latest cancelled operation
	QAction *cut;                ///< Cut selection to clipboard
	QAction *copy;               ///< Copy selection to clipboard
	QAction *paste;              ///< Paste clipboard content on the current diagram
	QAction *select_all;         ///< Select all
	QAction *select_nothing;     ///< Cancel selection
	QAction *select_invert;      ///< Invest selection
	QAction *delete_selection;   ///< Delete selection
	QAction *rotate_selection;   ///< Rotate selected elements and text items by 90 degrees
	QAction *rotate_texts;       ///< Direct selected text items to a specific angle
	QAction *find_element;       ///< Find the selected element in the panel
	QAction *selection_prop;     ///< Show a dialog describing the selection
	QAction *conductor_reset;    ///< Reset paths of selected conductors
	QAction *conductor_default;  ///< Show a dialog to edit default conductor properties
	QAction *infos_diagram;      ///< Show a dialog to edit diagram properties
	QAction *add_text;           ///< Tool to add an independent text item on diagrams
	QAction *add_column;         ///< Increase diagram width by adding an extra column
	QAction *remove_column;      ///< Decrease diagram width by removing the last column
	QAction *add_row;            ///< Increase diagram height by adding an extra row
	QAction *remove_row;         ///< Decrease diagram height by removing the last row
	QAction *prj_edit_prop;      ///< Edit the properties of the current project.
	QAction *prj_add_diagram;    ///< Add a diagram to the current project.
	QAction *prj_del_diagram;    ///< Delete a diagram from the current project
	QAction *prj_clean;          ///< Clean the content of the curent project by removing useless items
	QAction *prj_diagramNum;     ///< Numerotation des schemas
	QAction *prj_diagramList;    ///< Sommaire des schemas
	QAction *prj_nomenclature;   ///< generate nomenclature
	QAction *zoom_in;            ///< Zoom in
	QAction *zoom_out;           ///< Zoom out
	QAction *zoom_fit;           ///< Adjust zoom to fit the whole diagram, including potential elements outside its borders, in the view
	QAction *zoom_content;       ///< Adjust zoom to fit all elements in the view, regardless of diagram borders
	QAction *zoom_reset;         ///< Reset zoom to 1:1
	QAction *tile_window;        ///< Show MDI subwindows as tile
	QAction *cascade_window;     ///< Show MDI subwindows as cascade
	QAction *prev_window;        ///< Switch to the previous document
	QAction *next_window;        ///< Switch to the next document
	QAction *add_image;          ///< Tool to add an independent image item on diagrams
	QAction *add_line;		     ///< Tool to add an independent line shape item on diagrams
	QAction *add_rectangle;		 ///< Tool to add an independent rectangle shape item on diagrams
	QAction *add_ellipse;		 ///< Tool to add an independent ellipse shape item on diagrams
	QAction *edit_selection;	 ///< To edit selected item

	private:
	QMdiArea workspace;
	QSignalMapper windowMapper;
	/// Directory to use for file dialogs such as File > save
	QDir open_dialog_dir;
	/// Dock for the elements panel
	QDockWidget *qdw_pa;
	/// Dock for the undo list
	QDockWidget *qdw_undo;
	/// Elements panel
	ElementsPanelWidget *pa;
	QMenu *windows_menu;
	QToolBar *main_bar;
	QToolBar *view_bar;
	QToolBar *diagram_bar;
	QUndoGroup undo_group;
	bool can_update_actions;
};
#endif
