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
#include "qetdiagrameditor.h"
#include "qetapp.h"
#include "diagramcontent.h"
#include "diagramview.h"
#include "diagram.h"
#include "qetgraphicsitem/element.h"
#include "elementspanelwidget.h"
#include "conductorpropertieswidget.h"
#include "qetproject.h"
#include "projectview.h"
#include "recentfiles.h"
#include "qeticons.h"
#include "qetelementeditor.h"
#include "qetmessagebox.h"
#include "qetresult.h"
#include "genericpanel.h"
#include "nomenclature.h"
#include "diagramfoliolist.h"
#include "qetshapeitem.h"
#include "reportproperties.h"
#include "diagrampropertieseditordockwidget.h"
#include "diagrameventaddshape.h"
#include "diagrameventaddimage.h"
#include "diagrameventaddtext.h"
#include "elementscollectionwidget.h"
#include "autonumberingdockwidget.h"
#include "dynamicelementtextitem.h"
#include "conductortextitem.h"
#include "undocommand/rotateselectioncommand.h"
#include "rotatetextscommand.h"
#include "diagramcommands.h"
#include "dialogwaiting.h"
#include "addelementtextcommand.h"

#include <QMessageBox>
#include <QStandardPaths>

#include "elementscollectionmodel.h"


/**
 * @brief QETDiagramEditor::QETDiagramEditor
 * Constructor
 * @param files, list of files to open
 * @param parent, parent widget
 */
QETDiagramEditor::QETDiagramEditor(const QStringList &files, QWidget *parent) :
	QETMainWindow(parent),
	m_row_column_actions_group (this),
	m_selection_actions_group  (this),
	m_add_item_actions_group   (this),
	m_zoom_actions_group       (this),
	m_select_actions_group     (this),
	m_file_actions_group       (this),
	open_dialog_dir            (QStandardPaths::writableLocation(QStandardPaths::DesktopLocation))
{
	activeSubWindowIndex = 0;
	
	QSplitter *splitter_ = new QSplitter(this);
	splitter_->setChildrenCollapsible(false);
	splitter_->setOrientation(Qt::Vertical);
	splitter_->addWidget(&m_workspace);
	splitter_->addWidget(&m_search_and_replace_widget);
	m_search_and_replace_widget.setHidden(true);
	m_search_and_replace_widget.setEditor(this);
	setCentralWidget(splitter_);
	
		//Set object name to be retrieved by the stylesheets
	m_workspace.setBackground(QBrush(Qt::NoBrush));
	m_workspace.setObjectName("mdiarea");
	m_workspace.setTabsClosable(true);
	
		//Set the signal mapper
	connect(&windowMapper, SIGNAL(mapped(QWidget *)), this, SLOT(activateWidget(QWidget *)));
	
	setWindowTitle(tr("QElectroTech", "window title"));	
	setWindowIcon(QET::Icons::QETLogo);
	statusBar() -> showMessage(tr("QElectroTech", "status bar message"));
	
	setUpElementsPanel();
	setUpElementsCollectionWidget();
	setUpUndoStack();
	setUpSelectionPropertiesEditor();
	setUpAutonumberingWidget();

	setUpActions();
	setUpToolBar();
	setUpMenu();

	tabifyDockWidget(qdw_undo, qdw_pa);
	
		//By default the windows is maximised
	setMinimumSize(QSize(500, 350));
	setWindowState(Qt::WindowMaximized);
	
	connect (&m_workspace,                SIGNAL(subWindowActivated(QMdiSubWindow *)), this, SLOT(subWindowActivated(QMdiSubWindow*)));
	connect (QApplication::clipboard(), SIGNAL(dataChanged()),                       this, SLOT(slot_updatePasteAction()));

	readSettings();
	show();
	
		//If valid file path is given as arguments
	uint opened_projects = 0;
	if (files.count())
	{
			//So we open this files
		foreach(QString file, files)
			if (openAndAddProject(file))
				++ opened_projects;
	}

	slot_updateActions();
}

/**
	Destructeur
*/
QETDiagramEditor::~QETDiagramEditor() {
}

/**
 * @brief QETDiagramEditor::setUpElementsPanel
 * Setup the element panel and element panel widget
 */
void QETDiagramEditor::setUpElementsPanel() {
	//Add the element panel as a QDockWidget
	qdw_pa = new QDockWidget(tr("Projets", "dock title"), this);

	qdw_pa -> setObjectName   ("projects panel");
	qdw_pa -> setAllowedAreas (Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	qdw_pa -> setFeatures     (QDockWidget::AllDockWidgetFeatures);
	qdw_pa -> setMinimumWidth (160);
	qdw_pa -> setWidget       (pa = new ElementsPanelWidget(qdw_pa));

	addDockWidget(Qt::LeftDockWidgetArea, qdw_pa);

	connect(pa, SIGNAL(requestForProject                  (QETProject *)), this, SLOT(activateProject(QETProject *)));
	connect(pa, SIGNAL(requestForProjectClosing           (QETProject *)), this, SLOT(closeProject(QETProject *)));
	connect(pa, SIGNAL(requestForProjectPropertiesEdition (QETProject *)), this, SLOT(editProjectProperties(QETProject *)));
	connect(pa, SIGNAL(requestForNewDiagram               (QETProject *)), this, SLOT(addDiagramToProject(QETProject *)));

	connect(pa, SIGNAL(requestForDiagram                  (Diagram *)), this, SLOT(activateDiagram(Diagram *)));
	connect(pa, SIGNAL(requestForDiagramPropertiesEdition (Diagram *)), this, SLOT(editDiagramProperties(Diagram *)));
	connect(pa, SIGNAL(requestForDiagramDeletion          (Diagram *)), this, SLOT(removeDiagram(Diagram *)));
	connect(pa, SIGNAL(requestForDiagramMoveUp            (Diagram *)), this, SLOT(moveDiagramUp(Diagram *)));
	connect(pa, SIGNAL(requestForDiagramMoveDown          (Diagram *)), this, SLOT(moveDiagramDown(Diagram *)));
	connect(pa, SIGNAL(requestForDiagramMoveUpTop         (Diagram *)), this, SLOT(moveDiagramUpTop(Diagram *)));
	connect(pa, SIGNAL(requestForDiagramMoveUpx10         (Diagram *)), this, SLOT(moveDiagramUpx10(Diagram *)));
	connect(pa, SIGNAL(requestForDiagramMoveDownx10       (Diagram *)), this, SLOT(moveDiagramDownx10(Diagram *)));
}

/**
 * @brief QETDiagramEditor::setUpElementsCollectionWidget
 * Set up the dock widget of element collection
 */
void QETDiagramEditor::setUpElementsCollectionWidget()
{
	m_qdw_elmt_collection = new QDockWidget(tr("Collections"), this);
	m_qdw_elmt_collection->setObjectName("elements_collection_widget");
	m_qdw_elmt_collection->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	m_qdw_elmt_collection->setFeatures(QDockWidget::AllDockWidgetFeatures);

	m_element_collection_widget = new ElementsCollectionWidget(m_qdw_elmt_collection);
	m_qdw_elmt_collection->setWidget(m_element_collection_widget);
	m_element_collection_widget->expandFirstItems();

	addDockWidget(Qt::RightDockWidgetArea, m_qdw_elmt_collection);
}

/**
 * @brief QETDiagramEditor::setUpUndoStack
 * Setup the undostack and undo stack widget
 */
void QETDiagramEditor::setUpUndoStack() {

	QUndoView *undo_view = new QUndoView(&undo_group, this);

	undo_view -> setEmptyLabel (tr("Aucune modification"));
	undo_view -> setStatusTip  (tr("Cliquez sur une action pour revenir en arrière dans l'édition de votre schéma", "Status tip"));
	undo_view -> setWhatsThis  (tr("Ce panneau liste les différentes actions effectuées sur le folio courant. Cliquer sur une action permet de revenir à l'état du schéma juste après son application.", "\"What's this\" tip"));

	qdw_undo  = new QDockWidget(tr("Annulations", "dock title"), this);
	qdw_undo -> setObjectName("diagram_undo");

	qdw_undo -> setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	qdw_undo -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	qdw_undo -> setMinimumWidth(160);
	qdw_undo -> setWidget(undo_view);

	addDockWidget(Qt::LeftDockWidgetArea, qdw_undo);
}

/**
 * @brief QETDiagramEditor::setUpSelectionPropertiesEditor
 * Setup the dock for edit the current selection
 */
void QETDiagramEditor::setUpSelectionPropertiesEditor()
{
	m_selection_properties_editor = new DiagramPropertiesEditorDockWidget(this);
	m_selection_properties_editor -> setObjectName("diagram_properties_editor_dock_widget");
	addDockWidget(Qt::RightDockWidgetArea, m_selection_properties_editor);
}

/**
 * @brief QETDiagramEditor::setUpAutonumberingWidget
 * Setup the dock for AutoNumbering Selection
 */
void QETDiagramEditor::setUpAutonumberingWidget()
{
	m_autonumbering_dock = new AutoNumberingDockWidget(this);
	m_autonumbering_dock -> setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	m_autonumbering_dock -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	addDockWidget(Qt::RightDockWidgetArea, m_autonumbering_dock);
}

/**
 * @brief QETDiagramEditor::setUpActions
 * Set up all Qaction
 */
void QETDiagramEditor::setUpActions()
{
		//Export to another file type (jpeg, dxf etc...)
	m_export_diagram = new QAction(QET::Icons::DocumentExport,  tr("E&xporter"), this);
	m_export_diagram->setShortcut(QKeySequence(tr("Ctrl+Shift+X")));
	m_export_diagram->setStatusTip(tr("Exporte le folio courant dans un autre format", "status bar tip"));
	connect(m_export_diagram, &QAction::triggered, [this]() {
		ProjectView *current_project = currentProjectView();
		if (current_project) {
			current_project -> exportProject();
		}
	});
	
		//Print
	m_print = new QAction(QET::Icons::DocumentPrint,   tr("Imprimer"),  this);
	m_print->setShortcut(QKeySequence(QKeySequence::Print));
	m_print->setStatusTip(tr("Imprime un ou plusieurs folios du projet courant", "status bar tip"));
	connect(m_print, &QAction::triggered, [this]() {
		ProjectView *current_project = currentProjectView();
		if (current_project) {
			current_project -> printProject();
		}
	});
	
		//Quit editor
	m_quit_editor = new QAction(QET::Icons::ApplicationExit, tr("&Quitter"),  this);
	m_quit_editor->setShortcut(QKeySequence(tr("Ctrl+Q")));
	m_quit_editor->setStatusTip(tr("Ferme l'application QElectroTech", "status bar tip"));
	connect(m_quit_editor, &QAction::triggered, this, &QETDiagramEditor::close);

		//Undo
	undo = undo_group.createUndoAction(this, tr("Annuler"));
	undo->setIcon(QET::Icons::EditUndo);
	undo->setShortcut(QKeySequence::Undo);
	undo->setStatusTip(tr("Annule l'action précédente", "status bar tip"));
		//Redo
	redo = undo_group.createRedoAction(this, tr("Refaire"));
	redo->setIcon(QET::Icons::EditRedo);
	redo->setShortcut(QKeySequence::Redo);
	redo->setStatusTip(tr("Restaure l'action annulée", "status bar tip"));

		//cut copy past
	m_cut   = new QAction(QET::Icons::EditCut,   tr("Co&uper"), this);
	m_copy  = new QAction(QET::Icons::EditCopy,  tr("Cop&ier"), this);
	m_paste = new QAction(QET::Icons::EditPaste, tr("C&oller"), this);

	m_cut   -> setShortcut(QKeySequence::Cut);
	m_copy  -> setShortcut(QKeySequence::Copy);
	m_paste -> setShortcut(QKeySequence::Paste);

	m_cut   -> setStatusTip(tr("Transfère les éléments sélectionnés dans le presse-papier", "status bar tip"));
	m_copy  -> setStatusTip(tr("Copie les éléments sélectionnés dans le presse-papier", "status bar tip"));
	m_paste -> setStatusTip(tr("Place les éléments du presse-papier sur le folio", "status bar tip"));

	connect(m_cut, &QAction::triggered, [this]() {
		if (currentDiagramView())
			currentDiagramView()->cut();
	});
	connect(m_copy, &QAction::triggered, [this]() {
		if (currentDiagramView())
			currentDiagramView()->copy();
	});
	connect(m_paste, &QAction::triggered, [this]() {
		if(currentDiagramView())
			currentDiagramView()->paste();
	});

		//Reset conductor path
	m_conductor_reset = new QAction(QET::Icons::ConductorSettings,     tr("Réinitialiser les conducteurs"),        this);
	m_conductor_reset->setShortcut( QKeySequence( tr("Ctrl+K")		) );
	m_conductor_reset->setStatusTip(tr("Recalcule les chemins des conducteurs sans tenir compte des modifications", "status bar tip"));
	connect(m_conductor_reset, &QAction::triggered, [this]() {
		if (DiagramView *dv = currentDiagramView())
			dv->resetConductors();
	});
	
		//AutoConductor
	m_auto_conductor = new QAction   (QET::Icons::Autoconnect, tr("Création automatique de conducteur(s)","Tool tip of auto conductor"), this);
	m_auto_conductor->setStatusTip (tr("Utiliser la création automatique de conducteur(s) quand cela est possible", "Status tip of auto conductor"));
	m_auto_conductor->setCheckable (true);
	connect(m_auto_conductor, &QAction::triggered, [this](bool ac) {
		if (ProjectView *pv = currentProjectView())
			pv->project()->setAutoConductor(ac);
	});

		//Switch background color
	m_grey_background = new QAction   (QET::Icons::DiagramBg, tr("Couleur de fond blanc/gris","Tool tip of white/grey background button"), this);
	m_grey_background -> setStatusTip (tr("Affiche la couleur de fond du folio en blanc ou en gris", "Status tip of white/grey background button"));
	m_grey_background -> setCheckable (true);
	connect (m_grey_background, &QAction::triggered, [this](bool checked) {
		Diagram::background_color = checked ? Qt::darkGray : Qt::white;
		if (this->currentDiagramView() &&  this->currentDiagramView()->diagram())
			this->currentDiagramView()->diagram()->update();
	});

		//Draw or not the background grid
	m_draw_grid = new QAction ( QET::Icons::Grid, tr("Afficher la grille"), this);
	m_draw_grid->setStatusTip(tr("Affiche ou masque la grille des folios"));
	m_draw_grid->setCheckable(true);
	m_draw_grid->setChecked(true);
	connect(m_draw_grid, &QAction::triggered, [this](bool checked) {
		foreach (ProjectView *prjv, this->openedProjects())
			foreach (Diagram *d, prjv->project()->diagrams()) {
				d->setDisplayGrid(checked);
				d->update();
			}
	});

		//Edit current diagram properties
	m_edit_diagram_properties = new QAction(QET::Icons::DialogInformation, tr("Propriétés du folio"), this);
	m_edit_diagram_properties->setShortcut( QKeySequence( tr("Ctrl+L")));
	m_edit_diagram_properties     -> setStatusTip(tr("Édite les propriétés du folio (dimensions, informations du cartouche, propriétés des conducteurs...)", "status bar tip"));
	connect(m_edit_diagram_properties, &QAction::triggered, [this]() {
		if (ProjectView *project_view = currentProjectView())
		{
			activateProject(project_view);
			project_view->editCurrentDiagramProperties();
		}
	});
	
		//Edit current project properties
	m_project_edit_properties = new QAction(QET::Icons::ProjectProperties, tr("Propriétés du projet"), this);
	connect(m_project_edit_properties, &QAction::triggered, [this]() {
		editProjectProperties(currentProjectView());
	});
	
		//Add new folio to current project
	m_project_add_diagram = new QAction(QET::Icons::DiagramAdd, tr("Ajouter un folio"), this);
	m_project_add_diagram->setShortcut(QKeySequence(tr("Ctrl+T")));
	connect(m_project_add_diagram, &QAction::triggered, [this]() {
		if (ProjectView *current_project = currentProjectView()) {
			current_project->addNewDiagram();
		}
	});
	
		//Remove current folio from current project
	m_remove_diagram_from_project = new QAction(QET::Icons::DiagramDelete, tr("Supprimer le folio"), this);
	connect(m_remove_diagram_from_project, &QAction::triggered, this, &QETDiagramEditor::removeDiagramFromProject);
	
		//Clean the current project
	m_clean_project         = new QAction(QET::Icons::EditClear,             tr("Nettoyer le projet"),                   this);
	connect(m_clean_project, &QAction::triggered, [this]() {
		if (ProjectView *current_project = currentProjectView()) {
			if (current_project->cleanProject()) {
				pa -> reloadAndFilter();
			}
		}
	});
	
		//Add folio list to current project
	m_project_folio_list = new QAction(QET::Icons::TableOfContent, tr("Ajouter un sommaire"), this);
	connect(m_project_folio_list, &QAction::triggered, [this]() {
		if (ProjectView *current_project = currentProjectView()) {
			current_project->addNewDiagramFolioList();
		}
	});
	
		//Export nomenclature to CSV
	m_project_nomenclature = new QAction(QET::Icons::DocumentSpreadsheet, tr("Exporter une nomenclature"), this);
	connect(m_project_nomenclature, &QAction::triggered, [this]() {
		nomenclature nomencl(currentProjectView()->project(), this);
		nomencl.saveToCSVFile();
	});
	
		//Lauch the plugin of terminal generator
	m_project_terminalBloc = new QAction(QET::Icons::TerminalStrip, tr("Lancer le plugin de création de borniers"), this);
	connect(m_project_terminalBloc, &QAction::triggered, this, &QETDiagramEditor::generateTerminalBlock);
	
		//MDI view style
	m_tabbed_view_mode = new QAction(tr("en utilisant des onglets"), this);
	m_tabbed_view_mode->setStatusTip(tr("Présente les différents projets ouverts des onglets", "status bar tip"));
	m_tabbed_view_mode->setCheckable(true);
	connect(m_tabbed_view_mode, &QAction::triggered, this, &QETDiagramEditor::setTabbedMode);
	
	m_windowed_view_mode = new QAction(tr("en utilisant des fenêtres"), this);
	m_windowed_view_mode->setStatusTip(tr("Présente les différents projets ouverts dans des sous-fenêtres", "status bar tip"));
	m_windowed_view_mode->setCheckable(true);
	connect(m_windowed_view_mode, &QAction::triggered, this, &QETDiagramEditor::setWindowedMode);
	
	m_group_view_mode = new QActionGroup(this);
	m_group_view_mode -> addAction(m_windowed_view_mode);
	m_group_view_mode -> addAction(m_tabbed_view_mode);
	m_group_view_mode -> setExclusive(true);
	
	m_tile_window = new QAction(tr("&Mosaïque"), this);
	m_tile_window->setStatusTip(tr("Dispose les fenêtres en mosaïque", "status bar tip"));
	connect(m_tile_window, &QAction::triggered, &m_workspace, &QMdiArea::tileSubWindows);
	
	m_cascade_window = new QAction(tr("&Cascade"), this);
	m_cascade_window->setStatusTip(tr("Dispose les fenêtres en cascade", "status bar tip"));
	connect(m_cascade_window, &QAction::triggered, &m_workspace, &QMdiArea::cascadeSubWindows);

		//Switch selection/view mode
	m_mode_selection = new QAction(QET::Icons::PartSelect, tr("Mode Selection"), this);
	m_mode_selection->setStatusTip(tr("Permet de sélectionner les éléments", "status bar tip"));
	m_mode_selection->setCheckable(true);
	m_mode_selection->setChecked(true);
	connect(m_mode_selection, &QAction::triggered, [this]() {
		if (ProjectView *pv = currentProjectView()) {
			for (DiagramView *dv : pv->diagram_views()) {
				dv->setSelectionMode();
			}
		}
	});
	
	m_mode_visualise = new QAction(QET::Icons::ViewMove, tr("Mode Visualisation"), this);
	m_mode_visualise->setStatusTip(tr("Permet de visualiser le folio sans pouvoir le modifier", "status bar tip"));
	m_mode_visualise->setCheckable(true);
	connect(m_mode_visualise, &QAction::triggered, [this]() {
		if (ProjectView *pv = currentProjectView()) {
			for(DiagramView *dv : pv->diagram_views()) {
				dv->setVisualisationMode();
			}
		}
	});

	grp_visu_sel = new QActionGroup(this);
	grp_visu_sel->addAction(m_mode_selection);
	grp_visu_sel->addAction(m_mode_visualise);
	grp_visu_sel->setExclusive(true);

		//Navigate next/previous project
	m_next_window = new QAction(tr("Projet suivant"), this);
	m_next_window->setShortcut(QKeySequence::NextChild);
	m_next_window->setStatusTip(tr("Active le projet suivant", "status bar tip"));
	connect(m_next_window, &QAction::triggered, &m_workspace, &QMdiArea::activateNextSubWindow);
	
	m_previous_window = new QAction(tr("Projet précédent"), this);
	m_previous_window->setShortcut(QKeySequence::PreviousChild);
	m_previous_window->setStatusTip(tr("Active le projet précédent", "status bar tip"));
	connect(m_previous_window, &QAction::triggered, &m_workspace, &QMdiArea::activatePreviousSubWindow);

		//Files action
	QAction *new_file  = m_file_actions_group.addAction(QET::Icons::ProjectNew,     tr("&Nouveau"));
	QAction *open_file = m_file_actions_group.addAction(QET::Icons::DocumentOpen,   tr("&Ouvrir"));
	m_save_file          = m_file_actions_group.addAction(QET::Icons::DocumentSave,   tr("&Enregistrer"));
	m_save_file_as       = m_file_actions_group.addAction(QET::Icons::DocumentSaveAs, tr("Enregistrer sous"));
	m_close_file       = m_file_actions_group.addAction(QET::Icons::ProjectClose,   tr("&Fermer"));

	new_file     ->setShortcut(QKeySequence::New);
	open_file    ->setShortcut(QKeySequence::Open);
	m_close_file ->setShortcut(QKeySequence::Close);
	m_save_file    ->setShortcut(QKeySequence::Save);

	new_file     ->setStatusTip( tr("Crée un nouveau projet", "status bar tip")								 );
	open_file    ->setStatusTip( tr("Ouvre un projet existant", "status bar tip")								 );
	m_close_file ->setStatusTip( tr("Ferme le projet courant", "status bar tip")								 );
	m_save_file    ->setStatusTip( tr("Enregistre le projet courant et tous ses folios", "status bar tip")		 );
	m_save_file_as ->setStatusTip( tr("Enregistre le projet courant avec un autre nom de fichier", "status bar tip") );

	connect(m_save_file_as, &QAction::triggered, this, &QETDiagramEditor::saveAs);
	connect(m_save_file,    &QAction::triggered, this, &QETDiagramEditor::save);
	connect(new_file,       &QAction::triggered, this, &QETDiagramEditor::newProject);
	connect(open_file,      &QAction::triggered, this, &QETDiagramEditor::openProject);
	connect(m_close_file,   &QAction::triggered, [this]() {
		if (ProjectView *project_view = currentProjectView()) {
			closeProject(project_view);
		}
	});

		//Rows and Columns
	QAction *add_column    = m_row_column_actions_group.addAction( QET::Icons::EditTableInsertColumnRight, tr("Ajouter une colonne") );
	QAction *remove_column = m_row_column_actions_group.addAction( QET::Icons::EditTableDeleteColumn,      tr("Enlever une colonne") );
	QAction *add_row       = m_row_column_actions_group.addAction( QET::Icons::EditTableInsertRowUnder,    tr("Ajouter une ligne", "Add row") );
	QAction *remove_row    = m_row_column_actions_group.addAction( QET::Icons::EditTableDeleteRow,         tr("Enlever une ligne","Remove row") );

	add_column    -> setStatusTip( tr("Ajoute une colonne au folio", "status bar tip"));
	remove_column -> setStatusTip( tr("Enlève une colonne au folio", "status bar tip"));
	add_row       -> setStatusTip( tr("Agrandit le folio en hauteur", "status bar tip"));
	remove_row    -> setStatusTip( tr("Rétrécit le folio en hauteur", "status bar tip"));

	add_column   ->setData("add_column");
	remove_column->setData("remove_column");
	add_row      ->setData("add_row");
	remove_row   ->setData("remove_row");

	connect(&m_row_column_actions_group, &QActionGroup::triggered, this, &QETDiagramEditor::rowColumnGroupTriggered);

		//Selections Actions (related to a selected item)
	m_delete_selection    = m_selection_actions_group.addAction( QET::Icons::EditDelete,        tr("Supprimer")                 );
	m_rotate_selection     = m_selection_actions_group.addAction( QET::Icons::TransformRotate,   tr("Pivoter")                   );
	m_rotate_texts         = m_selection_actions_group.addAction( QET::Icons::ObjectRotateRight, tr("Orienter les textes")       );
	m_find_element         = m_selection_actions_group.addAction( QET::Icons::ZoomDraw,          tr("Retrouver dans le panel")   );
	m_edit_selection       = m_selection_actions_group.addAction( QET::Icons::ElementEdit,       tr("Éditer l'item sélectionné") );
	m_group_selected_texts = m_selection_actions_group.addAction(QET::Icons::textGroup,       tr("Grouper les textes sélectionnés"));

	m_delete_selection->setShortcut(QKeySequence::Delete);
	m_rotate_selection->setShortcut(QKeySequence( tr("Space")));
	m_rotate_texts    ->setShortcut(QKeySequence( tr("Ctrl+Space")));
	m_edit_selection  ->setShortcut(QKeySequence( tr("Ctrl+E")));

	m_delete_selection->setStatusTip( tr("Enlève les éléments sélectionnés du folio", "status bar tip"));
	m_rotate_selection->setStatusTip( tr("Pivote les éléments et textes sélectionnés", "status bar tip"));
	m_rotate_texts    ->setStatusTip( tr("Pivote les textes sélectionnés à un angle précis", "status bar tip"));
	m_find_element    ->setStatusTip( tr("Retrouve l'élément sélectionné dans le panel", "status bar tip"));

	m_delete_selection    ->setData("delete_selection");
	m_rotate_selection    ->setData("rotate_selection");
	m_rotate_texts        ->setData("rotate_selected_text");
	m_find_element        ->setData("find_selected_element");
	m_edit_selection      ->setData("edit_selected_element");
	m_group_selected_texts->setData("group_selected_texts");

	connect(&m_selection_actions_group, &QActionGroup::triggered, this, &QETDiagramEditor::selectionGroupTriggered);

		//Select Action
	QAction *select_all     = m_select_actions_group.addAction( QET::Icons::EditSelectAll,      tr("Tout sélectionner") );
	QAction *select_nothing = m_select_actions_group.addAction( QET::Icons::EditSelectNone,     tr("Désélectionner tout") );
	QAction *select_invert  = m_select_actions_group.addAction( QET::Icons::EditSelectInvert,   tr("Inverser la sélection") );

	select_all    ->setShortcut(QKeySequence::SelectAll);
	select_nothing->setShortcut(QKeySequence::Deselect);
	select_invert ->setShortcut(QKeySequence( tr("Ctrl+I")));

	select_all    ->setStatusTip( tr("Sélectionne tous les éléments du folio", "status bar tip")																	  );
	select_nothing->setStatusTip( tr("Désélectionne tous les éléments du folio", "status bar tip")															  );
	select_invert ->setStatusTip( tr("Désélectionne les éléments sélectionnés et sélectionne les éléments non sélectionnés", "status bar tip") );

	select_all    ->setData("select_all");
	select_nothing->setData("deselect");
	select_invert ->setData("invert_selection");

	connect(&m_select_actions_group, &QActionGroup::triggered, this, &QETDiagramEditor::selectGroupTriggered);

		//Zoom actions
	QAction *zoom_in      = m_zoom_actions_group.addAction( QET::Icons::ZoomIn,       tr("Zoom avant"));
	QAction *zoom_out     = m_zoom_actions_group.addAction( QET::Icons::ZoomOut,      tr("Zoom arrière"));
	QAction *zoom_content = m_zoom_actions_group.addAction( QET::Icons::ZoomDraw,     tr("Zoom sur le contenu"));
	QAction *zoom_fit     = m_zoom_actions_group.addAction( QET::Icons::ZoomFitBest,  tr("Zoom adapté"));
	QAction *zoom_reset   = m_zoom_actions_group.addAction( QET::Icons::ZoomOriginal, tr("Pas de zoom"));
	m_zoom_action_toolBar << zoom_content << zoom_fit << zoom_reset;

	zoom_in     ->setShortcut(QKeySequence::ZoomIn);
	zoom_out    ->setShortcut(QKeySequence::ZoomOut);
	zoom_content->setShortcut(QKeySequence( tr("Ctrl+8")));
	zoom_fit    ->setShortcut(QKeySequence( tr("Ctrl+9")));
	zoom_reset  ->setShortcut(QKeySequence( tr("Ctrl+0")));

	zoom_in     ->setStatusTip(tr("Agrandit le folio", "status bar tip"));
	zoom_out    ->setStatusTip(tr("Rétrécit le folio", "status bar tip"));
	zoom_content->setStatusTip(tr("Adapte le zoom de façon à afficher tout le contenu du folio indépendamment du cadre"));
	zoom_fit    ->setStatusTip(tr("Adapte le zoom exactement sur le cadre du folio", "status bar tip"));
	zoom_reset  ->setStatusTip(tr("Restaure le zoom par défaut", "status bar tip"));

	zoom_in     ->setData("zoom_in");
	zoom_out    ->setData("zoom_out");
	zoom_content->setData("zoom_content");
	zoom_fit    ->setData("zoom_fit");
	zoom_reset  ->setData("zoom_reset");

	connect(&m_zoom_actions_group, &QActionGroup::triggered, this, &QETDiagramEditor::zoomGroupTriggered);

		//Adding action (add text, image, shape...)
	QAction *add_text      = m_add_item_actions_group.addAction(QET::Icons::PartTextField, tr("Ajouter un champ de texte"));
	QAction *add_image	   = m_add_item_actions_group.addAction(QET::Icons::adding_image,  tr("Ajouter une image"));
	QAction *add_line	   = m_add_item_actions_group.addAction(QET::Icons::PartLine,      tr("Ajouter une ligne", "Draw line"));
	QAction *add_rectangle = m_add_item_actions_group.addAction(QET::Icons::PartRectangle, tr("Ajouter un rectangle"));
	QAction *add_ellipse   = m_add_item_actions_group.addAction(QET::Icons::PartEllipse,   tr("Ajouter une ellipse"));
	QAction *add_polyline  = m_add_item_actions_group.addAction(QET::Icons::PartPolygon,   tr("Ajouter une polyligne"));

	add_text     ->setData("text");
	add_image    ->setData("image");
	add_line     ->setData("line");
	add_rectangle->setData("rectangle");
	add_ellipse  ->setData("ellipse");
	add_polyline ->setData("polyline");

	for(QAction *action : m_add_item_actions_group.actions()) {
		action->setCheckable(true);
	}
	connect(&m_add_item_actions_group, &QActionGroup::triggered, this, &QETDiagramEditor::addItemGroupTriggered);

		//Depth action
	m_depth_action_group = QET::depthActionGroup(this);
	m_depth_action_group->setDisabled(true);
	
	connect(m_depth_action_group, &QActionGroup::triggered, [this](QAction *action) {
		this->currentDiagramView()->diagram()->changeZValue(action->data().value<QET::DepthOption>());
	});
	
	m_find = new QAction(tr("Chercher/remplacer"), this);
	m_find->setShortcut(QKeySequence::Find);
	connect(m_find, &QAction::triggered, [this]() {
		this->m_search_and_replace_widget.setHidden(!m_search_and_replace_widget.isHidden());
	});
}

/**
 * @brief QETDiagramEditor::setUpToolBar
 */
void QETDiagramEditor::setUpToolBar()
{
	main_tool_bar = new QToolBar(tr("Outils"), this);
	main_tool_bar -> setObjectName("toolbar");

	view_tool_bar = new QToolBar(tr("Affichage"), this);
	view_tool_bar -> setObjectName("display");

	diagram_tool_bar = new QToolBar(tr("Schéma"), this);
	diagram_tool_bar -> setObjectName("diagram");

	main_tool_bar -> addActions(m_file_actions_group.actions());
	main_tool_bar -> addAction(m_print);
	main_tool_bar -> addSeparator();
	main_tool_bar -> addAction(undo);
	main_tool_bar -> addAction(redo);
	main_tool_bar -> addSeparator();
	main_tool_bar -> addAction(m_cut);
	main_tool_bar -> addAction(m_copy);
	main_tool_bar -> addAction(m_paste);
	main_tool_bar -> addSeparator();
	main_tool_bar -> addAction(m_delete_selection);
	main_tool_bar -> addAction(m_rotate_selection);

	// Modes selection / visualisation et zoom
	view_tool_bar -> addAction(m_mode_selection);
	view_tool_bar -> addAction(m_mode_visualise);
	view_tool_bar -> addSeparator();
	view_tool_bar -> addAction(m_draw_grid);
	view_tool_bar -> addAction (m_grey_background);
	view_tool_bar -> addSeparator();
	view_tool_bar -> addActions(m_zoom_action_toolBar);

	diagram_tool_bar -> addAction (m_edit_diagram_properties);
	diagram_tool_bar -> addAction (m_conductor_reset);
	diagram_tool_bar -> addAction (m_auto_conductor);

	m_add_item_tool_bar = new QToolBar(tr("Ajouter"), this);
	m_add_item_tool_bar->setObjectName("adding");
	m_add_item_tool_bar->addActions(m_add_item_actions_group.actions());
	
	m_depth_tool_bar = new QToolBar(tr("Profondeur", "toolbar title"));
	m_depth_tool_bar->setObjectName("diagram_depth_toolbar");
	m_depth_tool_bar->addActions(m_depth_action_group->actions());

	addToolBar(Qt::TopToolBarArea, main_tool_bar);
	addToolBar(Qt::TopToolBarArea, view_tool_bar);
	addToolBar(Qt::TopToolBarArea, diagram_tool_bar);
	addToolBar(Qt::TopToolBarArea, m_add_item_tool_bar);
	addToolBar(Qt::TopToolBarArea, m_depth_tool_bar);
}

/**
 * @brief QETDiagramEditor::setUpMenu
 */
void QETDiagramEditor::setUpMenu() {

	QMenu *menu_fichier   = new QMenu(tr("&Fichier"));
	QMenu *menu_edition   = new QMenu(tr("&Édition"));
	QMenu *menu_project   = new QMenu(tr("&Projet"));
	QMenu *menu_affichage = new QMenu(tr("Afficha&ge"));
	//QMenu *menu_outils    = new QMenu(tr("O&utils"));
	windows_menu          = new QMenu(tr("Fe&nêtres"));

	insertMenu(settings_menu_, menu_fichier);
	insertMenu(settings_menu_, menu_edition);
	insertMenu(settings_menu_, menu_project);
	insertMenu(settings_menu_, menu_affichage);
	insertMenu(help_menu_, windows_menu);

	// File menu
	QMenu *recentfile = menu_fichier -> addMenu(QET::Icons::DocumentOpenRecent, tr("&Récemment ouverts"));
	recentfile->addActions(QETApp::projectsRecentFiles()->menu()->actions());
	connect(QETApp::projectsRecentFiles(), SIGNAL(fileOpeningRequested(const QString &)), this, SLOT(openRecentFile(const QString &)));
	menu_fichier -> addActions(m_file_actions_group.actions());
	menu_fichier -> addSeparator();
	//menu_fichier -> addAction(import_diagram);
	menu_fichier -> addAction(m_export_diagram);
	//menu_fichier -> addSeparator();
	menu_fichier -> addAction(m_print);
	menu_fichier -> addSeparator();
	menu_fichier -> addAction(m_quit_editor);

	// menu Edition
	menu_edition -> addAction(undo);
	menu_edition -> addAction(redo);
	menu_edition -> addSeparator();
	menu_edition -> addAction(m_cut);
	menu_edition -> addAction(m_copy);
	menu_edition -> addAction(m_paste);
	menu_edition -> addSeparator();
	menu_edition -> addActions(m_select_actions_group.actions());
	menu_edition -> addSeparator();
	menu_edition -> addActions(m_selection_actions_group.actions());
	menu_edition -> addSeparator();
	menu_edition -> addAction(m_conductor_reset);
	menu_edition -> addSeparator();
	menu_edition -> addAction(m_edit_diagram_properties);
	menu_edition -> addActions(m_row_column_actions_group.actions());
	menu_edition -> addSeparator();
	menu_edition -> addActions(m_depth_action_group->actions());
	menu_edition -> addSeparator();
	menu_edition -> addAction(m_find);

	// menu Projet
	menu_project -> addAction(m_project_edit_properties);
	menu_project -> addAction(m_project_add_diagram);
	menu_project -> addAction(m_remove_diagram_from_project);
	menu_project -> addAction(m_clean_project);
	menu_project -> addSeparator();
	menu_project -> addAction(m_project_folio_list);
	menu_project -> addAction(m_project_nomenclature);
	menu_project -> addAction(m_project_terminalBloc);

	main_tool_bar         -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils principale"));
	view_tool_bar         -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils Affichage"));
	diagram_tool_bar      -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils Schéma"));
	qdw_pa           -> toggleViewAction() -> setStatusTip(tr("Affiche ou non le panel d'appareils"));
	qdw_undo         -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la liste des modifications"));
	

	// menu Affichage
	QMenu *projects_view_mode = menu_affichage -> addMenu(QET::Icons::ConfigureToolbars, tr("Afficher les projets"));
	projects_view_mode -> setTearOffEnabled(true);
	projects_view_mode -> addAction(m_windowed_view_mode);
	projects_view_mode -> addAction(m_tabbed_view_mode);

	menu_affichage -> addSeparator();
	menu_affichage -> addAction(m_mode_selection);
	menu_affichage -> addAction(m_mode_visualise);
	menu_affichage -> addSeparator();
	menu_affichage -> addAction(m_draw_grid);
	menu_affichage -> addAction(m_grey_background);
	menu_affichage -> addSeparator();
	menu_affichage -> addActions(m_zoom_actions_group.actions());

	// menu Fenetres
	slot_updateWindowsMenu();
}

/**
	Permet de quitter l'application lors de la fermeture de la fenetre principale
	@param qce Le QCloseEvent correspondant a l'evenement de fermeture
*/
void QETDiagramEditor::closeEvent(QCloseEvent *qce) {
	// quitte directement s'il n'y a aucun projet ouvert
	bool can_quit = true;
	if (openedProjects().count()) {
		// s'assure que la fenetre soit visible s'il y a des projets a fermer
		if (!isVisible() || isMinimized()) {
			if (isMaximized()) showMaximized();
			else showNormal();
		}
		// sinon demande la permission de fermer chaque projet
		foreach(ProjectView *project, openedProjects()) {
			if (!closeProject(project)) {
				can_quit = false;
				qce -> ignore();
				break;
			}
		}
	}
	if (can_quit) {
		writeSettings();
		setAttribute(Qt::WA_DeleteOnClose);
		qce -> accept();
	}
}

/**
 * @brief QETDiagramEditor::event
 * Reimplemented to :
 * -Load elements collection when WindowActivate.
 * @param e
 * @return 
 */
bool QETDiagramEditor::event(QEvent *e)
{
	if (m_first_show && e->type() == QEvent::WindowActivate)
	{
		m_first_show = false;
		QTimer::singleShot(250, m_element_collection_widget, SLOT(reload()));
	}
	return(QETMainWindow::event(e));
}

/**
 * @brief QETDiagramEditor::save
 * Ask the current active project to save
 */
void QETDiagramEditor::save() {
	if (ProjectView *project_view = currentProjectView()) {
		QETResult saved = project_view -> save();

		if (saved.isOk()) {
			//save_file -> setDisabled(true);
			QETApp::projectsRecentFiles() -> fileWasOpened(project_view -> project() -> filePath());

			QString title = (project_view -> project() -> title ());
			if (title.isEmpty()) title = "QElectroTech ";
			QString filePath = (project_view -> project() -> filePath ());
			statusBar()-> showMessage(tr("Projet %1 enregistré dans le repertoire: %2.").arg(title).arg (filePath), 2000);
			m_element_collection_widget->highlightUnusedElement();
		}
		else {
			showError(saved);
		}
	}
}

/**
 * @brief QETDiagramEditor::saveAs
 * Ask the current active project to save as
 */
void QETDiagramEditor::saveAs() {
	if (ProjectView *project_view = currentProjectView()) {
		QETResult save_file = project_view -> saveAs();
		if (save_file.isOk()) {
			QETApp::projectsRecentFiles() -> fileWasOpened(project_view -> project() -> filePath());

			QString title = (project_view -> project() -> title ());
			if (title.isEmpty()) title = "QElectroTech ";
			QString filePath = (project_view -> project() -> filePath ());
			statusBar()->showMessage(tr("Projet %1 enregistré dans le repertoire: %2.").arg(title).arg (filePath), 2000);
			m_element_collection_widget->highlightUnusedElement();
		}
		else {
			showError(save_file);
		}
	}
}

/**
 * @brief QETDiagramEditor::newProject
 * Create an empty project
 * @return
 */
bool QETDiagramEditor::newProject() {
	// create new project without diagram
	QETProject *new_project = new QETProject(0);
	
	// Set default properties for new diagram
	new_project -> setDefaultBorderProperties	  (BorderProperties::    defaultProperties());
	new_project -> setDefaultConductorProperties  (ConductorProperties:: defaultProperties());
	new_project -> setDefaultTitleBlockProperties (TitleBlockProperties::defaultProperties());
	new_project -> setDefaultReportProperties	  (ReportProperties::    defaultProperties());
	new_project -> setDefaultXRefProperties		  (XRefProperties::      defaultProperties());
	
	// add new diagram
	new_project -> addNewDiagram();
	
	return(addProject(new_project));
}

/**
	Slot utilise pour ouvrir un fichier recent.
	Transfere filepath au slot openAndAddDiagram seulement si cet editeur est
	actif
	@param filepath Fichier a ouvrir
	@see openAndAddDiagram
*/
bool QETDiagramEditor::openRecentFile(const QString &filepath) {
	// small hack to prevent all diagram editors from trying to topen the required
	// recent file at the same time
	if (qApp -> activeWindow() != this) return(false);
	return(openAndAddProject(filepath));
}

/**
	Cette fonction demande un nom de fichier a ouvrir a l'utilisateur
	@return true si l'ouverture a reussi, false sinon
*/
bool QETDiagramEditor::openProject() {
	// demande un chemin de fichier a ouvrir a l'utilisateur
	QString filepath = QFileDialog::getOpenFileName(
		this,
		tr("Ouvrir un fichier"),
		open_dialog_dir.absolutePath(),
		tr("Projets QElectroTech (*.qet);;Fichiers XML (*.xml);;Tous les fichiers (*)")
	);
	if (filepath.isEmpty()) return(false);
	
	// retient le dossier contenant le dernier projet ouvert
	open_dialog_dir = QDir(filepath);
	
	// ouvre le fichier
	return(openAndAddProject(filepath));
}

/**
	Ferme un projet
	@param project_view Projet a fermer
	@return true si la fermeture du projet a reussi, false sinon
	Note : cette methode renvoie true si project est nul
*/
bool QETDiagramEditor::closeProject(ProjectView *project_view) {
	if (project_view) {
		activateProject(project_view);
		if (QMdiSubWindow *sub_window = subWindowForWidget(project_view)){
			return(sub_window -> close());
		}
	}
	return(true);
}

/**
	Ferme un projet
	@param project projet a fermer
	@return true si la fermeture du fichier a reussi, false sinon
	Note : cette methode renvoie true si project est nul
*/
bool QETDiagramEditor::closeProject(QETProject *project) {
	if (ProjectView *project_view = findProject(project)) {
		return(closeProject(project_view));
	}
	return(true);
}

/**
	Ouvre un projet depuis un fichier et l'ajoute a cet editeur
	@param filepath Chemin du projet a ouvrir
	@param interactive true pour afficher des messages a l'utilisateur, false sinon
	@return true si l'ouverture a reussi, false sinon
*/
bool QETDiagramEditor::openAndAddProject(const QString &filepath, bool interactive)
{
	if (filepath.isEmpty()) return(false);
	
	QFileInfo filepath_info(filepath);

		//Check if project is not open in another editor
	if (QETDiagramEditor *diagram_editor = QETApp::diagramEditorForFile(filepath))
	{
		if (diagram_editor == this)
		{
			if (ProjectView *project_view = viewForFile(filepath))
			{
				activateWidget(project_view);
				show();
				activateWindow();
			}
			return(false);
		}
		else
		{
				//Ask to the other editor to display the file
			return(diagram_editor -> openAndAddProject(filepath));
		}
	}
	
		// check the file exists
	if (!filepath_info.exists())
	{
		if (interactive)
		{
			QET::QetMessageBox::critical(
				this,
				tr("Impossible d'ouvrir le fichier", "message box title"),
				QString(
					tr("Il semblerait que le fichier %1 que vous essayez d'ouvrir"
					" n'existe pas ou plus.")
				).arg(filepath)
			);
		}
		return(false);
	}
	
		//Check if file readable
	if (!filepath_info.isReadable())
	{
		if (interactive) {
			QET::QetMessageBox::critical(
				this,
				tr("Impossible d'ouvrir le fichier", "message box title"),
				tr("Il semblerait que le fichier que vous essayez d'ouvrir ne "
				"soit pas accessible en lecture. Il est donc impossible de "
				"l'ouvrir. Veuillez vérifier les permissions du fichier.")
			);
		}
		return(false);
	}
	
		//Check if file is read only
	if (!filepath_info.isWritable())
	{
		if (interactive) {
			QET::QetMessageBox::warning(
				this,
				tr("Ouverture du projet en lecture seule", "message box title"),
				tr("Il semblerait que le projet que vous essayez d'ouvrir ne "
				"soit pas accessible en écriture. Il sera donc ouvert en "
				"lecture seule.")
			);
		}
	}
	
		//Create the project
	DialogWaiting::instance(this);
	
	QETProject *project = new QETProject(filepath);
	if (project -> state() != QETProject::Ok)
	{
		if (interactive && project -> state() != QETProject::FileOpenDiscard)
		{
			QET::QetMessageBox::warning(
				this,
				tr("Échec de l'ouverture du projet", "message box title"),
				QString(
					tr(
						"Il semblerait que le fichier %1 ne soit pas un fichier"
						" projet QElectroTech. Il ne peut donc être ouvert.",
						"message box content"
					)
				).arg(filepath)
			);
		}
		delete project;
		DialogWaiting::dropInstance();
		return(false);
	}

	QETApp::projectsRecentFiles() -> fileWasOpened(filepath);
	addProject(project);
	DialogWaiting::dropInstance();
	return true;
}

/**
	Ajoute un projetmoveDiagramUp(
	@param project projet a ajouter
	@param update_panel Whether the elements panel should be warned this
	project has been added. Defaults to true.
*/
bool QETDiagramEditor::addProject(QETProject *project, bool update_panel) {
	// enregistre le projet
	QETApp::registerProject(project);
	
	// cree un ProjectView pour visualiser le projet
	ProjectView *project_view = new ProjectView(project);
	addProjectView(project_view);

	undo_group.addStack(project -> undoStack());

	m_element_collection_widget->addProject(project);
	
	// met a jour le panel d'elements
	if (update_panel) {
		pa -> elementsPanel().projectWasOpened(project);
		if (currentDiagramView() != nullptr)
		m_autonumbering_dock->setProject(project, project_view);
	}
	
	return(true);
}

/**
	@return la liste des projets ouverts dans cette fenetre
*/
QList<ProjectView *> QETDiagramEditor::openedProjects() const {
	QList<ProjectView *> result;
	QList<QMdiSubWindow *> window_list(m_workspace.subWindowList());
	foreach(QMdiSubWindow *window, window_list) {
		if (ProjectView *project_view = qobject_cast<ProjectView *>(window -> widget())) {
			result << project_view;
		}
	}
	return(result);
}

/**
	@return Le projet actuellement edite (= qui a le focus dans l'interface
	MDI) ou 0 s'il n'y en a pas
*/
ProjectView *QETDiagramEditor::currentProjectView() const {
	QMdiSubWindow *current_window = m_workspace.activeSubWindow();
	if (!current_window) return(nullptr);
	
	QWidget *current_widget = current_window -> widget();
	if (!current_widget) return(nullptr);
	
	if (ProjectView *project_view = qobject_cast<ProjectView *>(current_widget)) {
		return(project_view);
	}
	return(nullptr);
}

/**
 * @brief QETDiagramEditor::currentProject
 * @return the current edited project.
 * This function can return nullptr.
 */
QETProject *QETDiagramEditor::currentProject() const
{
	ProjectView *view = currentProjectView();
	if (view) {
		return view->project();
	}
	else {
		return nullptr;
	}
}

/**
	@return Le schema actuellement edite (= l'onglet ouvert dans le projet
	courant) ou 0 s'il n'y en a pas
*/
DiagramView *QETDiagramEditor::currentDiagramView() const {
	if (ProjectView *project_view = currentProjectView()) {
		return(project_view -> currentDiagram());
	}
	return(nullptr);
}

/**
	@return the selected element in the current diagram view, or 0 if:
	  * no diagram is being viewed in this editor.
	  * no element is selected
	  * more than one element is selected
*/
Element *QETDiagramEditor::currentElement() const
{
	DiagramView *dv = currentDiagramView();
	if (!dv)
		return(nullptr);
	
	QList<Element *> selected_elements = DiagramContent(dv->diagram()).m_elements;
	if (selected_elements.count() != 1)
		return(nullptr);
	
	return(selected_elements.first());
}

/**
	Cette methode permet de retrouver le projet contenant un schema donne.
	@param diagram_view Schema dont il faut retrouver
	@return la vue sur le projet contenant ce schema ou 0 s'il n'y en a pas
*/
ProjectView *QETDiagramEditor::findProject(DiagramView *diagram_view) const {
	foreach(ProjectView *project_view, openedProjects()) {
		if (project_view -> diagram_views().contains(diagram_view)) {
			return(project_view);
		}
	}
	return(nullptr);
}

/**
	Cette methode permet de retrouver le projet contenant un schema donne.
	@param diagram Schema dont il faut retrouver
	@return la vue sur le projet contenant ce schema ou 0 s'il n'y en a pas
*/
ProjectView *QETDiagramEditor::findProject(Diagram *diagram) const {
	foreach(ProjectView *project_view, openedProjects()) {
		foreach(DiagramView *diagram_view, project_view -> diagram_views()) {
			if (diagram_view -> diagram() == diagram) {
				return(project_view);
			}
		}
	}
	return(nullptr);
}

/**
	@param project Projet dont il faut trouver la vue
	@return la vue du projet passe en parametre
*/
ProjectView *QETDiagramEditor::findProject(QETProject *project) const {
	foreach(ProjectView *opened_project, openedProjects()) {
		if (opened_project -> project() == project) {
			return(opened_project);
		}
	}
	return(nullptr);
}

/**
	@param filepath Chemin de fichier d'un projet
	@return le ProjectView correspondant au chemin passe en parametre, ou 0 si
	celui-ci n'a pas ete trouve
*/
ProjectView *QETDiagramEditor::findProject(const QString &filepath) const {
	foreach(ProjectView *opened_project, openedProjects()) {
		if (QETProject *project = opened_project -> project()) {
			if (project -> filePath() == filepath) {
				return(opened_project);
			}
		}
	}
	return(nullptr);
}

/**
	@param widget Widget a rechercher dans la zone MDI
	@return La sous-fenetre accueillant le widget passe en parametre, ou 0 si
	celui-ci n'a pas ete trouve.
*/
QMdiSubWindow *QETDiagramEditor::subWindowForWidget(QWidget *widget) const {
	foreach(QMdiSubWindow *sub_window, m_workspace.subWindowList()) {
		if (sub_window -> widget() == widget) {
			return(sub_window);
		}
	}
	return(nullptr);
}

/**
	@param widget Widget a activer
*/
void QETDiagramEditor::activateWidget(QWidget *widget) {
	QMdiSubWindow *sub_window = subWindowForWidget(widget);
	if (sub_window) {
		m_workspace.setActiveSubWindow(sub_window);
	}
}

void QETDiagramEditor::zoomGroupTriggered(QAction *action)
{
	QString value = action->data().toString();
	DiagramView *dv = currentDiagramView();

	if (!dv || value.isEmpty()) return;

	if (value == "zoom_in")
		dv->zoom(1.15);
	else if (value == "zoom_out")
		dv->zoom(0.85);
	else if (value == "zoom_content")
		dv->zoomContent();
	else if (value == "zoom_fit")
		dv->zoomFit();
	else if (value == "zoom_reset")
		dv->zoomReset();
}

/**
 * @brief QETDiagramEditor::selectGroupTriggered
 * This slot is called when selection need to change.
 * @param action : Action that describes what to do.
 */
void QETDiagramEditor::selectGroupTriggered(QAction *action)
{
	QString value = action->data().toString();
	DiagramView *dv = currentDiagramView();

	if (!dv || value.isEmpty()) return;

	if (value == "select_all")
		dv->selectAll();
	else if (value == "deselect")
		dv->selectNothing();
	else if (value == "invert_selection")
		dv->selectInvert();
}

/**
 * @brief QETDiagramEditor::addItemGroupTriggered
 * This slot is called when an item must be added to the curent diagram,
 * this slot use the DVEventInterface to add item
 * @param action : Action that describe the item to add.
 */
void QETDiagramEditor::addItemGroupTriggered(QAction *action)
{
	QString value = action->data().toString();

	if (Q_UNLIKELY (!currentDiagramView() || !currentDiagramView()->diagram() || value.isEmpty())) return;

	Diagram *d = currentDiagramView()->diagram();
	DiagramEventInterface *diagram_event = nullptr;

	if (value == "line")
		diagram_event = new DiagramEventAddShape (d, QetShapeItem::Line);
	else if (value == "rectangle")
		diagram_event = new DiagramEventAddShape (d, QetShapeItem::Rectangle);
	else if (value == "ellipse")
		diagram_event = new DiagramEventAddShape (d, QetShapeItem::Ellipse);
	else if (value == "polyline")
		diagram_event = new DiagramEventAddShape (d, QetShapeItem::Polygon);
	else if (value == "image")
	{
		DiagramEventAddImage *deai = new DiagramEventAddImage(d);
		if (deai->isNull())
		{
			delete deai;
			action->setChecked(false);
			return;
		}
		else
			diagram_event = deai;
	}
	else if (value == "text")
		diagram_event = new DiagramEventAddText(d);

	if (diagram_event)
	{
		d->setEventInterface(diagram_event);
		connect(diagram_event, &DiagramEventInterface::finish, [action](){action->setChecked(false);});
	}
}

/**
 * @brief QETDiagramEditor::selectionGroupTriggered
 * This slot is called when an action should be made on the current selection
 * @param action : Action that describe the action to do.
 */
void QETDiagramEditor::selectionGroupTriggered(QAction *action)
{
	QString value = action->data().toString();
	DiagramView *dv = currentDiagramView();
	Diagram *diagram = dv->diagram();
	DiagramContent dc(diagram);

	if (!dv || value.isEmpty()) return;

	if (value == "delete_selection")
	{
		diagram->clearSelection();
		diagram->undoStack().push(new DeleteQGraphicsItemCommand(diagram, dc));
		dv->adjustSceneRect();
	}
	else if (value == "rotate_selection")
	{
		RotateSelectionCommand *c = new RotateSelectionCommand(diagram);
		if(c->isValid())
			diagram->undoStack().push(c);
	}
	else if (value == "rotate_selected_text")
		diagram->undoStack().push(new RotateTextsCommand(diagram));
	else if (value == "find_selected_element" && currentElement())
		findElementInPanel(currentElement()->location());
	else if (value == "edit_selected_element")
		dv->editSelection();
	else if (value == "group_selected_texts")
	{
		QList<DynamicElementTextItem *> deti_list = dc.m_element_texts.toList();
		if(deti_list.size() <= 1)
			return;
		
		diagram->undoStack().push(new AddTextsGroupCommand(deti_list.first()->parentElement(), tr("Groupe"), deti_list));
	}
}

void QETDiagramEditor::rowColumnGroupTriggered(QAction *action)
{
	QString value = action->data().toString();
	DiagramView *dv = currentDiagramView();

	if (!dv || value.isEmpty() || dv->diagram()->isReadOnly()) return;

	Diagram *d = dv->diagram();
	BorderProperties old_bp = d->border_and_titleblock.exportBorder();
	BorderProperties new_bp = d->border_and_titleblock.exportBorder();
	
	if (value == "add_column")
		new_bp.columns_count += 1;
	else if (value == "remove_column")
		new_bp.columns_count -= 1;
	else if (value == "add_row")
		new_bp.rows_count += 1;
	else if (value == "remove_row")
		new_bp.rows_count -= 1;
	
	d->undoStack().push(new ChangeBorderCommand(d, old_bp, new_bp));
}

/**
 * @brief QETDiagramEditor::slot_updateActions
 * Manage actions
 */
void QETDiagramEditor::slot_updateActions()
{
	DiagramView *dv = currentDiagramView();
	ProjectView *pv = currentProjectView();

	bool opened_project = pv;
	bool opened_diagram = dv;
	bool editable_project = (pv && !pv -> project() -> isReadOnly());

	m_close_file       -> setEnabled(opened_project);
	m_save_file        -> setEnabled(opened_project);
	m_save_file_as     -> setEnabled(opened_project);
	m_project_edit_properties    -> setEnabled(opened_project);
	//prj_terminalBloc -> setEnabled(opened_project);
	m_rotate_texts -> setEnabled(editable_project);
	m_project_add_diagram  -> setEnabled(editable_project);
	m_remove_diagram_from_project  -> setEnabled(editable_project);
	m_clean_project        -> setEnabled(editable_project);
	m_project_folio_list  -> setEnabled(opened_project);
	m_project_nomenclature -> setEnabled(editable_project);
	m_export_diagram   -> setEnabled(opened_diagram);
	m_print            -> setEnabled(opened_diagram);
	m_edit_diagram_properties    -> setEnabled(opened_diagram);
	m_project_nomenclature -> setEnabled(editable_project);
	m_zoom_actions_group.      setEnabled(opened_diagram);
	m_select_actions_group.    setEnabled(opened_diagram);
	m_add_item_actions_group.  setEnabled(editable_project);
	m_row_column_actions_group.setEnabled(editable_project);
	m_grey_background->setEnabled(opened_diagram);


	slot_updateUndoStack();
	slot_updateModeActions();
	slot_updatePasteAction();
	slot_updateComplexActions();
	slot_updateAutoNumDock();
}

/**
 * @brief QETDiagramEditor::slot_updateAutoNumDock
 * Update Auto Num Dock Widget when changing Project
 */
void QETDiagramEditor::slot_updateAutoNumDock() {
	if ( m_workspace.subWindowList().indexOf(m_workspace.activeSubWindow()) != activeSubWindowIndex) {
			activeSubWindowIndex = m_workspace.subWindowList().indexOf(m_workspace.activeSubWindow());
			if (currentProjectView() != nullptr && currentDiagramView() != nullptr) {
				m_autonumbering_dock->setProject(currentProjectView()->project(),currentProjectView());
			}
	}
}

/**
 * @brief QETDiagramEditor::slot_updateUndoStack
 * Update the undo stack view
 */
void QETDiagramEditor::slot_updateUndoStack()
{
	if(currentProjectView())
		undo_group.setActiveStack(currentProjectView()->project()->undoStack());
}

/**
 * @brief QETDiagramEditor::slot_updateComplexActions
 * Manage the actions who need some conditions to be enable or not.
 * This method does nothing if there is no project opened
 */
void QETDiagramEditor::slot_updateComplexActions()
{
	DiagramView *dv = currentDiagramView();
	if(!dv)
	{
		QList <QAction *> action_list;
		action_list << m_conductor_reset << m_find_element << m_cut << m_copy << m_delete_selection << m_rotate_selection << m_edit_selection << m_group_selected_texts;
		for(QAction *action : action_list)
			action->setEnabled(false);
		
		return;
	}
	
	Diagram *diagram_ = dv->diagram();
	DiagramContent dc(diagram_);
	bool ro = diagram_->isReadOnly();
	

		//Number of selected conductors
	int selected_conductors_count = diagram_->selectedConductors().count();
	m_conductor_reset->setEnabled(!ro && selected_conductors_count);
	
		// number of selected elements
	int selected_elements_count = dc.count(DiagramContent::Elements);
	m_find_element->setEnabled(selected_elements_count == 1);
	
		//Action that need items (elements, conductors, texts...) selected, to be enabled
	bool copiable_items  = dc.hasCopiableItems();
	bool deletable_items = dc.hasDeletableItems();
	m_cut              -> setEnabled(!ro && copiable_items);
	m_copy             -> setEnabled(copiable_items);
	m_delete_selection -> setEnabled(!ro && deletable_items);
	m_rotate_selection -> setEnabled(!ro && diagram_->canRotateSelection());

		//Action that need selected texts or texts group
	QList<DiagramTextItem *> texts = DiagramContent(diagram_).selectedTexts();
	QList<ElementTextItemGroup *> groups = DiagramContent(diagram_).selectedTextsGroup();
	int selected_texts = texts.count();
	int selected_conductor_texts   = 0; for(DiagramTextItem *dti : texts) {if(dti->type() == ConductorTextItem::Type) selected_conductor_texts++;}
	int selected_dynamic_elmt_text = 0; for(DiagramTextItem *dti : texts) {if(dti->type() == DynamicElementTextItem::Type) selected_dynamic_elmt_text++;}
	m_rotate_texts->setEnabled(!ro && (selected_texts || groups.size()));
	
		//Action that need only element text selected
	QList<DynamicElementTextItem *> deti_list = dc.m_element_texts.toList();
	if(deti_list.size() > 1 && dc.count() == deti_list.count())
	{
		Element *elmt = deti_list.first()->parentElement();
		bool ok = true;
		for(DynamicElementTextItem *deti : deti_list)
		{
			if(elmt != deti->parentElement())
				ok = false;
		}
		m_group_selected_texts->setEnabled(!ro && ok);
	}
	else
		m_group_selected_texts->setDisabled(true);

		// actions need only one editable item
	int selected_image = dc.count(DiagramContent::Images);

	int selected_shape = dc.count(DiagramContent::Shapes);
	int selected_editable = selected_elements_count +
							(selected_texts - selected_conductor_texts - selected_dynamic_elmt_text) +
							selected_image +
							selected_shape +
							selected_conductors_count;

	if (selected_editable == 1)
	{
		m_edit_selection -> setEnabled(true);
			//edit element
		if (selected_elements_count)
		{
			m_edit_selection -> setText(tr("Éditer l'élement", "edit element"));
			m_edit_selection -> setIcon(QET::Icons::ElementEdit);
		}
			//edit text field
		else if (selected_texts)
		{
			m_edit_selection -> setText(tr("Éditer le champ de texte", "edit text field"));
			m_edit_selection -> setIcon(QET::Icons::EditText);
		}
			//edit image
		else if (selected_image)
		{
			m_edit_selection -> setText(tr("Éditer l'image", "edit image"));
			m_edit_selection -> setIcon(QET::Icons::resize_image);
		}
			//edit conductor
		else if (selected_conductors_count)
		{
			m_edit_selection -> setText(tr("Éditer le conducteur", "edit conductor"));
            m_edit_selection -> setIcon(QET::Icons::ConductorEdit);
		}
	}
		//not an editable item
	else
	{
		m_edit_selection -> setText(tr("Éditer l'objet sélectionné", "edit selected item"));
		m_edit_selection -> setIcon(QET::Icons::ElementEdit);
		m_edit_selection -> setEnabled(false);
	}
	
		//Actions for edit Z value
	QList<QGraphicsItem *> list = dc.items(DiagramContent::SelectedOnly | \
											 DiagramContent::Elements | \
											 DiagramContent::Shapes | \
											 DiagramContent::Images);
	m_depth_action_group->setEnabled(list.isEmpty()? false : true);
}

/**
 * @brief QETDiagramEditor::slot_updateModeActions
 * Manage action who need an opened diagram or project to be updated
 */
void QETDiagramEditor::slot_updateModeActions()
{
	DiagramView *dv = currentDiagramView();
	
	if (!dv)
		grp_visu_sel -> setEnabled(false);
	else
	{
		switch((int)(dv -> dragMode()))
		{
			case QGraphicsView::NoDrag:
				grp_visu_sel -> setEnabled(false);
				break;
			case QGraphicsView::ScrollHandDrag:
				grp_visu_sel -> setEnabled(true);
				m_mode_visualise -> setChecked(true);
				break;
			case QGraphicsView::RubberBandDrag:
				grp_visu_sel -> setEnabled(true);
				m_mode_selection -> setChecked(true);
				break;
		}
	}

	if (ProjectView *pv = currentProjectView())
	{
		m_auto_conductor -> setEnabled (true);
		m_auto_conductor -> setChecked (pv -> project() -> autoConductor());
	}
	else
		m_auto_conductor -> setDisabled(true);
}

/**
	Gere les actions ayant besoin du presse-papier
*/
void QETDiagramEditor::slot_updatePasteAction() {
	DiagramView *dv = currentDiagramView();
	bool editable_diagram = (dv && !dv -> diagram() -> isReadOnly());
	
	// pour coller, il faut un schema ouvert et un schema dans le presse-papier
	m_paste -> setEnabled(editable_diagram && Diagram::clipboardMayContainDiagram());
}

/**
 * @brief QETDiagramEditor::addProjectView
 * Add a new project view to workspace and
 * build the connection between the projectview / project and this QETDiagramEditor.
 * @param project_view, project view to add
 */
void QETDiagramEditor::addProjectView(ProjectView *project_view)
{
	if (!project_view) return;

	foreach(DiagramView *dv, project_view -> diagram_views())
		diagramWasAdded(dv);
	
		//Manage the close event of project
	connect(project_view, SIGNAL(projectClosed(ProjectView*)), this, SLOT(projectWasClosed(ProjectView *)));
		//Manage the adding  of diagram
	connect(project_view, SIGNAL(diagramAdded(DiagramView *)), this, SLOT(diagramWasAdded(DiagramView *)));

	if (QETProject *project = project_view -> project())
		connect(project, SIGNAL(readOnlyChanged(QETProject *, bool)), this, SLOT(slot_updateActions()));
	
		//Manage request for edit or find element and titleblock
	connect (project_view, &ProjectView::findElementRequired, this, &QETDiagramEditor::findElementInPanel);
	connect (project_view, &ProjectView::editElementRequired, this, &QETDiagramEditor::editElementInEditor);
	
		// display error messages sent by the project view
	connect(project_view, SIGNAL(errorEncountered(QString)), this, SLOT(showError(const QString &)));

		//We maximise the new window if the current window is inexistent or maximized
	QWidget *current_window = m_workspace.activeSubWindow();
	bool     maximise       = ((!current_window) || (current_window -> windowState() & Qt::WindowMaximized));

		//Add the new window
	QMdiSubWindow *sub_window = m_workspace.addSubWindow(project_view);
	sub_window -> setWindowIcon(project_view -> windowIcon());
	sub_window -> systemMenu() -> clear();
	
		//By defaut QMdiSubWindow have a QAction "close" with shortcut QKeySequence::Close
		//But the QAction m_close_file of this class have the same shortcut too.
		//We remove the shortcut of the QAction of QMdiSubWindow for avoid conflic
	for(QAction *act : sub_window->actions())
	{
		if(act->shortcut() == QKeySequence::Close)
			act->setShortcut(QKeySequence());
	}

		//Display the new window
	if (maximise) project_view -> showMaximized();
	else          project_view -> show();
}

/**
	@return la liste des fichiers edites par cet editeur de schemas
*/
QList<QString> QETDiagramEditor::editedFiles() const {
	QList<QString> edited_files_list;
	foreach (ProjectView *project_view, openedProjects()) {
		QString diagram_file(project_view -> project() -> filePath());
		if (!diagram_file.isEmpty()) {
			edited_files_list << QFileInfo(diagram_file).canonicalFilePath();
		}
	}
	return(edited_files_list);
}

/**
	@param filepath Un chemin de fichier
	Note : si filepath est une chaine vide, cette methode retourne 0.
	@return le ProjectView editant le fichier filepath, ou 0 si ce fichier n'est
	pas edite par cet editeur de schemas.
*/
ProjectView *QETDiagramEditor::viewForFile(const QString &filepath) const {
	if (filepath.isEmpty()) return(nullptr);
	
	QString searched_can_file_path = QFileInfo(filepath).canonicalFilePath();
	if (searched_can_file_path.isEmpty()) {
		// QFileInfo returns an empty path for non-existent files
		return(nullptr);
	}
	foreach (ProjectView *project_view, openedProjects()) {
		QString project_can_file_path = QFileInfo(project_view -> project() -> filePath()).canonicalFilePath();
		if (project_can_file_path == searched_can_file_path) {
			return(project_view);
		}
	}
	return(nullptr);
}

/**
 * @brief QETDiagramEditor::drawGrid
 * @return true if the grid of folio must be displayed
 */
bool QETDiagramEditor::drawGrid() const {
	return m_draw_grid->isChecked();
}

/**
	met a jour le menu "Fenetres"
*/
void QETDiagramEditor::slot_updateWindowsMenu() {
	// nettoyage du menu
	foreach(QAction *a, windows_menu -> actions()) windows_menu -> removeAction(a);
	
	// actions de fermeture
	windows_menu -> addAction(m_close_file);
	//windows_menu -> addAction(closeAllAct);
	
	// actions de reorganisation des fenetres
	windows_menu -> addSeparator();
	windows_menu -> addAction(m_tile_window);
	windows_menu -> addAction(m_cascade_window);
	
	// actions de deplacement entre les fenetres
	windows_menu -> addSeparator();
	windows_menu -> addAction(m_next_window);
	windows_menu -> addAction(m_previous_window);
	
	// liste des fenetres
	QList<ProjectView *> windows = openedProjects();
	
	m_tile_window    -> setEnabled(!windows.isEmpty() && m_workspace.viewMode() == QMdiArea::SubWindowView);
	m_cascade_window -> setEnabled(!windows.isEmpty() && m_workspace.viewMode() == QMdiArea::SubWindowView);
	m_next_window    -> setEnabled(windows.count() > 1);
	m_previous_window    -> setEnabled(windows.count() > 1);
	
	if (!windows.isEmpty()) windows_menu -> addSeparator();
	QActionGroup *windows_actions = new QActionGroup(this);
	foreach(ProjectView *project_view, windows) {
		QString pv_title = project_view -> windowTitle();
		QAction *action  = windows_menu -> addAction(pv_title);
		windows_actions -> addAction(action);
		action -> setStatusTip(QString(tr("Active le projet « %1 »")).arg(pv_title));
		action -> setCheckable(true);
		action -> setChecked(project_view == currentProjectView());
		connect(action, SIGNAL(triggered()), &windowMapper, SLOT(map()));
		windowMapper.setMapping(action, project_view);
	}
}

/**
	Edite les proprietes du schema diagram
	@param diagram_view schema dont il faut editer les proprietes
*/
void QETDiagramEditor::editDiagramProperties(DiagramView *diagram_view) {
	if (ProjectView *project_view = findProject(diagram_view)) {
		activateProject(project_view);
		project_view -> editDiagramProperties(diagram_view);
	}
}

/**
	Edite les proprietes du schema diagram
	@param diagram schema dont il faut editer les proprietes
*/
void QETDiagramEditor::editDiagramProperties(Diagram *diagram) {
	if (ProjectView *project_view = findProject(diagram)) {
		activateProject(project_view);
		project_view -> editDiagramProperties(diagram);
	}
}

/**
	Affiche les projets dans des fenetres.
*/
void QETDiagramEditor::setWindowedMode() {
	m_workspace.setViewMode(QMdiArea::SubWindowView);
	m_windowed_view_mode -> setChecked(true);
	slot_updateWindowsMenu();
}

/**
	Affiche les projets dans des onglets.
*/
void QETDiagramEditor::setTabbedMode() {
	m_workspace.setViewMode(QMdiArea::TabbedView);
	m_tabbed_view_mode -> setChecked(true);
	slot_updateWindowsMenu();
}

/**
 * @brief QETDiagramEditor::readSettings
 * Read the settings
 */
void QETDiagramEditor::readSettings()
{
	QSettings settings;
	
	// dimensions et position de la fenetre
	QVariant geometry = settings.value("diagrameditor/geometry");
	if (geometry.isValid()) restoreGeometry(geometry.toByteArray());
	
	// etat de la fenetre (barres d'outils, docks...)
	QVariant state = settings.value("diagrameditor/state");
	if (state.isValid()) restoreState(state.toByteArray());
	
	// gestion des projets (onglets ou fenetres)
	bool tabbed = settings.value("diagrameditor/viewmode", "tabbed") == "tabbed";
	if (tabbed) {
		setTabbedMode();
	} else {
		setWindowedMode();
	}
}

/**
 * @brief QETDiagramEditor::writeSettings
 * Write the settings
 */
void QETDiagramEditor::writeSettings()
{
	QSettings settings;
	settings.setValue("diagrameditor/geometry", saveGeometry());
	settings.setValue("diagrameditor/state", saveState());
}

/**
	Active le schema passe en parametre
	@param diagram Schema a activer
*/
void QETDiagramEditor::activateDiagram(Diagram *diagram) {
	if (QETProject *project = diagram -> project()) {
		if (ProjectView *project_view = findProject(project)) {
			activateWidget(project_view);
			project_view -> showDiagram(diagram);
		}
	} else {
		/// @todo gerer ce cas
	}
}

/**
	Active le projet passe en parametre
	@param project Projet a activer
*/
void QETDiagramEditor::activateProject(QETProject *project) {
	activateProject(findProject(project));
}

/**
	Active le projet passe en parametre
	@param project_view Projet a activer
*/
void QETDiagramEditor::activateProject(ProjectView *project_view) {
	if (!project_view) return;
	activateWidget(project_view);
}

/**
	Gere la fermeture d'une ProjectView
	@param project_view ProjectView fermee
*/
void QETDiagramEditor::projectWasClosed(ProjectView *project_view) {
	QETProject *project = project_view -> project();
	if (project) {
		pa -> elementsPanel().projectWasClosed(project);
		m_element_collection_widget->removeProject(project);
		undo_group.removeStack(project -> undoStack());
		QETApp::unregisterProject(project);
	}
	project_view -> deleteLater();
	project -> deleteLater();
}

/**
	Edite les proprietes du projet project_view.
	@param project_view Vue sur le projet dont il faut editer les proprietes
*/
void QETDiagramEditor::editProjectProperties(ProjectView *project_view) {
	if (!project_view) return;
	activateProject(project_view);
	project_view -> editProjectProperties();
}

/**
	Edite les proprietes du projet project.
	@param project Projet dont il faut editer les proprietes
*/
void QETDiagramEditor::editProjectProperties(QETProject *project) {
	editProjectProperties(findProject(project));
}

/**
	Ajoute un nouveau schema a un projet
	@param project Projet auquel il faut ajouter un schema
*/
void QETDiagramEditor::addDiagramToProject(QETProject *project) {
	if (!project) return;
	
	// recupere le ProjectView visualisant ce projet
	if (ProjectView *project_view = findProject(project)) {
		
		// affiche le projet en question
		activateProject(project);
		
		// ajoute un schema au projet
		project_view -> addNewDiagram();
	}
}

/**
	Supprime un schema de son projet
	@param diagram Schema a supprimer
*/
void QETDiagramEditor::removeDiagram(Diagram *diagram) {
	if (!diagram) return;
	
	// recupere le projet contenant le schema
	if (QETProject *diagram_project = diagram -> project()) {
		// recupere la vue sur ce projet
		if (ProjectView *project_view = findProject(diagram_project)) {
			
			// affiche le schema en question
			project_view -> showDiagram(diagram);
			
			// supprime le schema
			project_view -> removeDiagram(diagram);
		}
	}
}

/**
	Change l'ordre des schemas d'un projet, en decalant le schema vers le haut /
	la gauche
	@param diagram Schema a decaler vers le haut / la gauche
*/
void QETDiagramEditor::moveDiagramUp(Diagram *diagram) {
	if (!diagram) return;
	
	// recupere le projet contenant le schema
	if (QETProject *diagram_project = diagram -> project()) {
		if (diagram_project -> isReadOnly()) return;
		
		// recupere la vue sur ce projet
		if (ProjectView *project_view = findProject(diagram_project)) {
			project_view -> moveDiagramUp(diagram);
		}
	}
}

/**
	Change l'ordre des schemas d'un projet, en decalant le schema vers le bas /
	la droite
	@param diagram Schema a decaler vers le bas / la droite
*/
void QETDiagramEditor::moveDiagramDown(Diagram *diagram) {
	if (!diagram) return;
	
	// recupere le projet contenant le schema
	if (QETProject *diagram_project = diagram -> project()) {
		if (diagram_project -> isReadOnly()) return;
		
		// recupere la vue sur ce projet
		if (ProjectView *project_view = findProject(diagram_project)) {
			project_view -> moveDiagramDown(diagram);
		}
	}
}

/**
	Change l'ordre des schemas d'un projet, en decalant le schema vers le haut /
	la gauche en position 0
	@param diagram Schema a decaler vers le haut / la gauche en position 0
 */
void QETDiagramEditor::moveDiagramUpTop(Diagram *diagram) {
	if (!diagram) return;

	// recupere le projet contenant le schema
	if (QETProject *diagram_project = diagram -> project()) {
		if (diagram_project -> isReadOnly()) return;

		// recupere la vue sur ce projet
		if (ProjectView *project_view = findProject(diagram_project)) {
			project_view -> moveDiagramUpTop(diagram);
		}
	}
}


/**
	Change l'ordre des schemas d'un projet, en decalant le schema vers le haut /
	la gauche x10
	@param diagram Schema a decaler vers le haut / la gauche x10
*/
void QETDiagramEditor::moveDiagramUpx10(Diagram *diagram) {
	if (!diagram) return;

	// recupere le projet contenant le schema
	if (QETProject *diagram_project = diagram -> project()) {
		if (diagram_project -> isReadOnly()) return;

		// recupere la vue sur ce projet
		if (ProjectView *project_view = findProject(diagram_project)) {
			project_view -> moveDiagramUpx10(diagram);
		}
	}
}

/**
	Change l'ordre des schemas d'un projet, en decalant le schema vers le bas /
	la droite x10
	@param diagram Schema a decaler vers le bas / la droite x10
*/
void QETDiagramEditor::moveDiagramDownx10(Diagram *diagram) {
	if (!diagram) return;

	// recupere le projet contenant le schema
	if (QETProject *diagram_project = diagram -> project()) {
		if (diagram_project -> isReadOnly()) return;

		// recupere la vue sur ce projet
		if (ProjectView *project_view = findProject(diagram_project)) {
			project_view -> moveDiagramDownx10(diagram);
		}
	}
}

void QETDiagramEditor::reloadOldElementPanel() {
	pa->reloadAndFilter();
}

/**
	Supprime le schema courant du projet courant
*/
void QETDiagramEditor::removeDiagramFromProject() {
	if (ProjectView *current_project = currentProjectView()) {
		if (DiagramView *current_diagram = current_project -> currentDiagram()) {
			bool isFolioList = false;

			// if diagram to remove is a "folio list sheet", then set a flag.
			if (dynamic_cast<DiagramFolioList *>(current_diagram -> diagram()))
				isFolioList = true;

			current_project -> removeDiagram(current_diagram);

			// if the removed diagram was a folio sheet, then delete all the remaining folio sheets also.
			if (isFolioList) {
				foreach (DiagramView *diag, current_project -> diagram_views()) {
					if (dynamic_cast<DiagramFolioList *>(diag -> diagram())) {
						current_project -> removeDiagram(diag);
					}
				}

			  // else if after diagram removal, the total diagram quantity becomes a factor of 58, then
			  // remove one (last) folio sheet.
			} else if (current_project -> diagram_views().size() % 58 == 0) {
				foreach (DiagramView *diag, current_project -> diagram_views()) {
					DiagramFolioList *ptr = dynamic_cast<DiagramFolioList *>(diag -> diagram());
					if (ptr && ptr -> getId() == current_project -> project() -> getFolioSheetsQuantity() - 1) {
						current_project -> removeDiagram(diag);
					}
				}
			}
		}
	}
}

/**
 * @brief QETDiagramEditor::diagramWasAdded
 * Manage the adding of diagram view in a project
 * @param dv, added diagram view
 */
void QETDiagramEditor::diagramWasAdded(DiagramView *dv)
{
	connect(dv, SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
	connect(dv, SIGNAL(modeChanged()),      this, SLOT(slot_updateModeActions()));
}

/**
 * @brief QETDiagramEditor::findElementInPanel
 * Find the item for @location in the element panel
 * @param location
 */
void QETDiagramEditor::findElementInPanel(const ElementsLocation &location)
{
	m_element_collection_widget->setCurrentLocation(location);
}

/**
	Lance l'editeur d'element pour l'element filename
	@param location Emplacement de l'element a editer
*/
void QETDiagramEditor::editElementInEditor(const ElementsLocation &location) {
	QETApp::instance() -> openElementLocations(QList<ElementsLocation>() << location);
}

/**
	Launch an element editor to edit the selected element in the current
	diagram view.
*/
void QETDiagramEditor::editSelectedElementInEditor() {
	if (Element *selected_element = currentElement()) {
		editElementInEditor(selected_element -> location());
	}
}

/**
	Show the error message contained in \a result.
*/
void QETDiagramEditor::showError(const QETResult &result) {
	if (result.isOk()) return;
	showError(result.errorMessage());
}

/**
	Show the \a error message.
*/
void QETDiagramEditor::showError(const QString &error) {
	if (error.isEmpty()) return;
	QET::QetMessageBox::critical(this, tr("Erreur", "message box title"), error);
}

/**
 * @brief QETDiagramEditor::subWindowActivated
 * Slot used to update menu and undo stack when subwindows of MDIarea was activated
 * @param subWindows
 */
void QETDiagramEditor::subWindowActivated(QMdiSubWindow *subWindows)
{
	Q_UNUSED(subWindows);

	slot_updateActions();
	slot_updateWindowsMenu();
}

/**
 * @brief QETDiagramEditor::selectionChanged
 * This slot is called when a diagram selection was changed.
 */
void QETDiagramEditor::selectionChanged()
{
	slot_updateComplexActions();

	DiagramView *dv = currentDiagramView();
	if (dv && dv->diagram())
		m_selection_properties_editor->setDiagram(dv->diagram());
}


/**
 * @brief QETDiagramEditor::generateTerminalBlock
 */
void QETDiagramEditor::generateTerminalBlock()
{
	bool success;
	QProcess *process = new QProcess(qApp);
	
		// If launched under control:
		//connect(process, SIGNAL(errorOcurred(int error)), this, SLOT(slot_generateTerminalBlock_error()));
		//process->start("qet_tb_generator");
	
#ifdef Q_OS_MAC
	if (openedProjects().count()){
		success = process->startDetached("/Library/Frameworks/Python.framework/Versions/3.5/bin/qet_tb_generator", {(QETDiagramEditor::currentProjectView()->project()->filePath())});
	}
	else  {
		success = process->startDetached("/Library/Frameworks/Python.framework/Versions/3.5/bin/qet_tb_generator");
	}
#else
	if (openedProjects().count()){
		success = process->startDetached("qet_tb_generator", {(QETDiagramEditor::currentProjectView()->project()->filePath())});
	}
	else  {
		success = process->startDetached("qet_tb_generator");
	}
	
#endif
	if ( !success ) {
		QMessageBox::warning(nullptr,
							 tr("Error launching qet_tb_generator plugin"), 
							 tr("To install the plugin qet_tb_generator\nVisit https://pypi.python.org/pypi/qet-tb-generator/\n"
								"\n"
								"Requires python 3.5 or above.\n"
								">> First install on Linux\n"
								"1. check you have pip3 installed: pip3 --version\n"
								"If not install with: sudo apt-get install python3-pip\n"
								"2. Install the program: sudo pip3 install qet_tb_generator\n"
								"3. Run the program: qet_tb_generator\n"
								">> Update on Linux\n"
								"sudo pip3 install --upgrade qet_tb_generator \n"
								"\n"
								">> First install on Windows\n"
								"1. Install, if required, python 3.5 or above \n"
								" Visit https://www.python.org/downloads/ \n"
								"2. pip install qet_tb_generator\n"
								">> Update on Windows\n"
								"python -m pip install --upgrade qet_tb_generator\n"
								">>user could launch in a terminal this script in this directory \n"
								" C:\\users\\XXXX\\AppData\\Local\\Programs\\Python\\Python36-32\\Scripts \n"
								"\n"
								">> First install on macOSX \n"
								"1. Install, if required, python 3.5 or above \n"
								" Visit https://qelectrotech.org/forum/viewtopic.php?pid=5674#p5674 \n"
								"2. pip3 install qet_tb_generator \n"
								">> Update on macOSX \n"
								" pip3 install --upgrade qet_tb_generator \n"
								));
	}
}

