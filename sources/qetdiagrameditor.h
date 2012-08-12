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
class Diagram;
class DiagramView;
class ElementsPanelWidget;
class ElementsLocation;
class RecentFiles;
/**
	Cette classe represente la fenetre principale de QElectroTech et,
	ipso facto, la plus grande partie de l'interface graphique de QElectroTech.
	Il s'agit d'un objet QMainWindow avec un QWorkSpace contenant des objets
	"DiagramView" en guise de widget central et un "Panel d'Elements" en
	guise de widget "Dock".
*/
class QETDiagramEditor : public QETMainWindow {
	Q_OBJECT
	
	// constructeurs, destructeur
	public:
	QETDiagramEditor(const QStringList & = QStringList(), QWidget * = 0);
	virtual ~QETDiagramEditor();
	
	private:
	QETDiagramEditor(const QETDiagramEditor &);
	
	// methodes
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
	
	protected:
	void actions();
	virtual bool event(QEvent *);
	
	private:
	bool addProject(QETProject *, bool = true);
	ProjectView *currentProject() const;
	DiagramView *currentDiagram() const;
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
	void addDiagramToProject(QETProject *);
	void removeDiagram(Diagram *);
	void removeDiagramFromProject();
	void moveDiagramUp(Diagram *);
	void moveDiagramDown(Diagram *);
	void cleanCurrentProject();
	void diagramWasAdded(DiagramView *);
	void diagramIsAboutToBeRemoved(DiagramView *);
	void diagramWasRemoved(DiagramView *);
	void findElementInPanel(const ElementsLocation &);
	void editElementInEditor(const ElementsLocation &);
	void showError(const QETResult &);
	void showError(const QString &);
	
	// attributs
	public:
	// Actions faisables au travers de menus dans l'application QElectroTech
	QActionGroup *grp_visu_sel;  ///< Groupe d'actions pour les modes (edition et visualisation)
	QActionGroup *grp_view_mode; ///< Groupe d'actions pour l'affichage des projets (onglets ou fenetres)
	QAction *tabbed_view_mode;   ///< Passe les projets en mode onglets
	QAction *windowed_view_mode; ///< Passe les projets en mode fenetre
	QAction *mode_selection;     ///< Passe en mode edition
	QAction *mode_visualise;     ///< Passe en mode visualisation
	QAction *new_file;           ///< Cree un nouveau schema
	QAction *open_file;          ///< OUvre un fichier
	QAction *close_file;         ///< Ferme le fichier
	QAction *save_file;          ///< Save current project
	QAction *save_file_as;       ///< Save current project as a specific file
	QAction *save_cur_diagram;   ///< Save current diagram of the current project only
	QAction *import_diagram;     ///< Importe un schema existant (non implemente)
	QAction *export_diagram;     ///< Exporte le schema sous forme d'image
	QAction *print;              ///< Imprime le schema
	QAction *quit_editor;        ///< Quitte l'editeur de schema
	QAction *undo;               ///< Annule la derniere action
	QAction *redo;               ///< Refait une action annulee
	QAction *cut;                ///< Coupe la selection dans le presse-papier
	QAction *copy;               ///< Copie la selection dans le presse-papier
	QAction *paste;              ///< Colle le contenu du presse-papier sur le schema
	QAction *select_all;         ///< Selectionne tout
	QAction *select_nothing;     ///< Deselectionne tout
	QAction *select_invert;      ///< Inverse la selection
	QAction *delete_selection;   ///< Supprime la selection
	QAction *rotate_selection;   ///< Pivote les elements et textes selectionnes de 90 degres
	QAction *rotate_texts;       ///< Pivote les textes selectionnes selon un angle parametrable
	QAction *selection_prop;     ///< Lance le dialogue de description ou d'edition de la selection
	QAction *conductor_reset;    ///< Reinitialise les conducteurs selectionnes
	QAction *conductor_default;  ///< Lance le dialogue d'edition des conducteurs par defaut
	QAction *infos_diagram;      ///< Lance le dialogue d'edition des proprietes du schema
	QAction *add_text;           ///< Passe en mode "Ajout de texte"
	QAction *add_column;         ///< Ajoute une colonne au schema
	QAction *remove_column;      ///< Enleve une colonne du schema
	QAction *add_row;            ///< Augmente la hauteur du schema
	QAction *remove_row;         ///< Reduit la hauteur du schema
	QAction *prj_edit_prop;      ///< Edite les proprietes du projet
	QAction *prj_add_diagram;    ///< Ajoute un schema au projet
	QAction *prj_del_diagram;    ///< Supprime un schema du projet
	QAction *prj_clean;          ///< Nettoie un projet
	QAction *zoom_in;            ///< Zoome avant
	QAction *zoom_out;           ///< Zoome arriere
	QAction *zoom_fit;           ///< Adjust zoom to fit the whole diagram, including potential elements outside its borders, in the view
	QAction *zoom_content;       ///< Adjust zoom to fit all elements in the view, regardless of diagram borders
	QAction *zoom_reset;         ///< Remet le zoom 1:1
	QAction *tile_window;        ///< Affiche les fenetre MDI en mosaique
	QAction *cascade_window;     ///< Affiche les fenetres MDI en cascade
	QAction *prev_window;        ///< Affiche la fenetre MDI precedente
	QAction *next_window;        ///< Affiche la fenetre MDI suivante
	
	private:
	QMdiArea workspace;
	QSignalMapper windowMapper;
	/// Dossier a utiliser pour Fichier > ouvrir
	QDir open_dialog_dir;
	/// Dock pour le Panel d'Elements
	QDockWidget *qdw_pa;
	/// Dock pour la liste des annulations
	QDockWidget *qdw_undo;
	/// Panel d'Elements
	ElementsPanelWidget *pa;
	QMenu *windows_menu;
	QToolBar *main_bar;
	QToolBar *view_bar;
	QToolBar *diagram_bar;
	QUndoGroup undo_group;
	bool can_update_actions;
};
#endif
