/*
	Copyright 2006-2026 The QElectroTech Team
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
class TerminalNumberingDialog;
class DiagramEditorActions;

class KAutoSaveFile;
/**
	This class represents the main window of the QElectroTech diagram editor and,
	ipso facto, the most important part of the QElectroTech user interface.
*/
class QETDiagramEditor : public QETMainWindow
{
	Q_OBJECT

        friend class TerminalStripEditorWindow;
        friend class DiagramEditorActions;

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
		void openBackupFiles (QList<KAutoSaveFile *> backup_files);
		DiagramEditorActions *actions() const { return m_actions; }

	  protected:
		bool event(QEvent *) override;
	private:
		// Declared first so it is initialised before any member whose
		// construction/setup may dispatch a Qt event calling event()
		// (e.g. a QObject member's setParent, or a widget shown early).
		bool m_first_show = true;

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
		void updateUsageTrackersActiveState();

	signals:
		void syncElementsPanel();

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
		void slot_terminalNumbering();
		void editDiagramProperties(DiagramView *);
		void editDiagramProperties(Diagram *);
		void addDiagramToProject(QETProject *);
		void removeDiagram(Diagram *);
		void removeDiagrams(const QList<Diagram *> &diagrams);
		void removeDiagramFromProject();
		void moveDiagramUp(const QList<Diagram *> &diagrams);
		void moveDiagramDown(const QList<Diagram *> &diagrams);
		void moveDiagramUpTop(const QList<Diagram *> &diagrams);
		void moveDiagramUpx10(const QList<Diagram *> &diagrams);
		void moveDiagramDownx10(const QList<Diagram *> &diagrams);
		void moveDiagramUpx100(const QList<Diagram *> &diagrams);
		void moveDiagramDownx100(const QList<Diagram *> &diagrams);
		void reloadOldElementPanel();
		void diagramWasAdded(DiagramView *);
		void findElementInPanel(const ElementsLocation &);
		void showError(const QETResult &);
		void showError(const QString &);
		void subWindowActivated(QMdiSubWindow *subWindows);

	private slots:
		void selectionChanged();

	private:
		DiagramEditorActions *m_actions = nullptr;

		void removeDiagramSilent(Diagram *diagram);

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
		SearchAndReplaceWidget m_search_and_replace_widget;
};
#endif
