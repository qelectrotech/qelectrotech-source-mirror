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
#include "qetdiagrameditor.h"
#include "qetapp.h"
#include "diagramcontent.h"
#include "diagramview.h"
#include "diagram.h"
#include "qetgraphicsitem/element.h"
#include "elementspanelwidget.h"
#include "conductorpropertieswidget.h"
#include "qetgraphicsitem/customelement.h"
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
#include "dveventaddimage.h"
#include "dveventaddshape.h"
#include "dveventaddtext.h"
#include "reportproperties.h"
#include "diagrampropertieseditordockwidget.h"

#include "ui/dialogautonum.h"

#include <QMessageBox>
#include <QStandardPaths>


/**
 * @brief QETDiagramEditor::QETDiagramEditor
 * Constructor
 * @param files, list of files to open
 * @param parent, parent widget
 */
QETDiagramEditor::QETDiagramEditor(const QStringList &files, QWidget *parent) :
	QETMainWindow(parent),
	m_add_item_actions_group   (this),
	m_zoom_actions_group       (this),
	m_select_actions_group     (this),
	m_selection_actions_group  (this),
	m_row_column_actions_group (this),
	m_file_actions_group       (this),
	open_dialog_dir            (QStandardPaths::writableLocation(QStandardPaths::DesktopLocation))
{
		//Setup the mdi area at center of application
	setCentralWidget(&workspace);
	
		//Set object name to be retrieved by the stylesheets
	workspace.setBackground(QBrush(Qt::NoBrush));
	workspace.setObjectName("mdiarea");
	workspace.setTabsClosable(true);
	
		//Set the signal mapper
	connect(&windowMapper, SIGNAL(mapped(QWidget *)), this, SLOT(activateWidget(QWidget *)));
	
	setWindowTitle(tr("QElectroTech", "window title"));	
	setWindowIcon(QET::Icons::QETLogo);
	statusBar() -> showMessage(tr("QElectroTech", "status bar message"));
	
	setUpElementsPanel();
	setUpUndoStack();
	setUpSelectionPropertiesEditor();
	setUpActions();
	setUpToolBar();
	setUpMenu();

	tabifyDockWidget(qdw_undo, qdw_pa);
	
		//By default the windows is maximised
	setMinimumSize(QSize(500, 350));
	setWindowState(Qt::WindowMaximized);
	
	connect (&workspace,                SIGNAL(subWindowActivated(QMdiSubWindow *)), this, SLOT(subWindowActivated(QMdiSubWindow*)));
	connect (QApplication::clipboard(), SIGNAL(dataChanged()),                       this, SLOT(slot_updatePasteAction()));
	connect (&undo_group,               SIGNAL(cleanChanged(bool)),                  this, SLOT(activeUndoStackCleanChanged(bool)));
	
	readSettings();
	show();
	
		//If valid file path is given as arguments
	uint opened_projects = 0;
	if (files.count())
	{
			//So we open this files
		foreach(QString file, files)
			if (openAndAddProject(file, false))
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
	qdw_pa = new QDockWidget(tr("Panel d'éléments", "dock title"), this);

	qdw_pa -> setObjectName   ("elements panel");
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
 * @brief QETDiagramEditor::setUpActions
 * Set up all Qaction
 */
void QETDiagramEditor::setUpActions()
{
	export_diagram    = new QAction(QET::Icons::DocumentExport,        tr("E&xporter"),                            this);
	print             = new QAction(QET::Icons::DocumentPrint,         tr("Imprimer"),                             this);
	quit_editor       = new QAction(QET::Icons::ApplicationExit,       tr("&Quitter"),                             this);

		//Undo redo
	undo = undo_group.createUndoAction(this, tr("Annuler"));
	redo = undo_group.createRedoAction(this, tr("Refaire"));

	undo -> setIcon(QET::Icons::EditUndo);
	redo -> setIcon(QET::Icons::EditRedo);

	undo -> setShortcut(QKeySequence::Undo);
	redo -> setShortcut(QKeySequence::Redo);

	undo -> setStatusTip(tr("Annule l'action précédente", "status bar tip"));
	redo -> setStatusTip(tr("Restaure l'action annulée", "status bar tip"));

		//cut copy past action
	cut   = new QAction(QET::Icons::EditCut,   tr("Co&uper"), this);
	copy  = new QAction(QET::Icons::EditCopy,  tr("Cop&ier"), this);
	paste = new QAction(QET::Icons::EditPaste, tr("C&oller"), this);

	cut   -> setShortcut(QKeySequence::Cut);
	copy  -> setShortcut(QKeySequence::Copy);
	paste -> setShortcut(QKeySequence::Paste);

	cut   -> setStatusTip(tr("Transfère les éléments sélectionnés dans le presse-papier", "status bar tip"));
	copy  -> setStatusTip(tr("Copie les éléments sélectionnés dans le presse-papier", "status bar tip"));
	paste -> setStatusTip(tr("Place les éléments du presse-papier sur le folio", "status bar tip"));

	connect(cut,   SIGNAL(triggered()), this, SLOT(slot_cut()));
	connect(copy,  SIGNAL(triggered()), this, SLOT(slot_copy()));
	connect(paste, SIGNAL(triggered()), this, SLOT(slot_paste()));

	conductor_reset   = new QAction(QET::Icons::ConductorSettings,     tr("Réinitialiser les conducteurs"),        this);
	conductor_reset  -> setShortcut( QKeySequence( tr("Ctrl+K")		) );

	m_auto_conductor = new QAction   (QET::Icons::Autoconnect, tr("Création automatique de conducteur(s)","Tool tip of auto conductor"), this);
	m_auto_conductor -> setStatusTip (tr("Utiliser la création automatique de conducteur(s) quand cela est possible", "Status tip of auto conductor"));
	m_auto_conductor -> setCheckable (true);
	connect(m_auto_conductor, SIGNAL(triggered(bool)), this, SLOT(slot_autoConductor(bool)));

	m_grey_background = new QAction   (QET::Icons::DiagramBg, tr("Couleur de fond blanc/gris","Tool tip of white/grey background button"), this);
	m_grey_background -> setStatusTip (tr("Affiche la couleur de fond du folio en blanc ou en gris", "Status tip of white/grey background button"));
	m_grey_background -> setCheckable (true);
	connect (m_grey_background, &QAction::triggered, [this](bool checked) {
		Diagram::background_color = checked ? Qt::darkGray : Qt::white;
		if (this->currentDiagram() &&  this->currentDiagram()->diagram())
			this->currentDiagram()->diagram()->update();
	});

	infos_diagram     = new QAction(QET::Icons::DialogInformation,     tr("Propriétés du folio"),                 this);
	infos_diagram    -> setShortcut( QKeySequence( tr("Ctrl+L")		) );
	prj_edit_prop     = new QAction(QET::Icons::DialogInformation,     tr("Propriétés du projet"),                 this);
	prj_add_diagram   = new QAction(QET::Icons::DiagramAdd,            tr("Ajouter un folio"),                    this);
	prj_del_diagram   = new QAction(QET::Icons::DiagramDelete,         tr("Supprimer le folio"),                  this);
	prj_clean         = new QAction(QET::Icons::EditClear,             tr("Nettoyer le projet"),                   this);
	prj_diagramList   = new QAction(QET::Icons::listDrawings,          tr("Ajouter un sommaire"),                  this);
	prj_nomenclature  = new QAction(QET::Icons::DocumentSpreadsheet,   tr("Exporter une nomenclature"),            this);
	tabbed_view_mode  = new QAction(                                   tr("en utilisant des onglets"),             this);
	windowed_view_mode= new QAction(                                   tr("en utilisant des fenêtres"),            this);
	mode_selection    = new QAction(QET::Icons::PartSelect,            tr("Mode Selection"),                       this);
	mode_visualise    = new QAction(QET::Icons::ViewMove,              tr("Mode Visualisation"),                   this);
	tile_window       = new QAction(                                   tr("&Mosaïque"),                            this);
	cascade_window    = new QAction(                                   tr("&Cascade"),                             this);
	next_window       = new QAction(                                   tr("Projet suivant"),                       this);
	prev_window       = new QAction(                                   tr("Projet précédent"),                     this);

		//Files action
	QAction *new_file  = m_file_actions_group.addAction( QET::Icons::ProjectNew,     tr("&Nouveau")						  );
	QAction *open_file = m_file_actions_group.addAction( QET::Icons::DocumentOpen,    tr("&Ouvrir")							  );
	save_file          = m_file_actions_group.addAction( QET::Icons::DocumentSave,    tr("&Enregistrer")					  );
	save_file_as       = m_file_actions_group.addAction( QET::Icons::DocumentSaveAs,  tr("Enregistrer sous")				  );
	close_file         = m_file_actions_group.addAction( QET::Icons::ProjectClose,   tr("&Fermer")							  );

	new_file   -> setShortcut( QKeySequence::New   );
	open_file  -> setShortcut( QKeySequence::Open  );
	close_file -> setShortcut( QKeySequence::Close );
	save_file  -> setShortcut( QKeySequence::Save  );

	new_file         -> setStatusTip( tr("Crée un nouveau projet", "status bar tip")								 );
	open_file        -> setStatusTip( tr("Ouvre un projet existant", "status bar tip")								 );
	close_file       -> setStatusTip( tr("Ferme le projet courant", "status bar tip")								 );
	save_file        -> setStatusTip( tr("Enregistre le projet courant et tous ses folios", "status bar tip")		 );
	save_file_as     -> setStatusTip( tr("Enregistre le projet courant avec un autre nom de fichier", "status bar tip") );

	connect(save_file_as,     SIGNAL( triggered() ), this, SLOT( saveAs()			   ) );
	connect(save_file,        SIGNAL( triggered() ), this, SLOT( save()				   ) );
	connect(new_file,         SIGNAL( triggered() ), this, SLOT( newProject()		   ) );
	connect(open_file,        SIGNAL( triggered() ), this, SLOT( openProject()		   ) );
	connect(close_file,       SIGNAL( triggered() ), this, SLOT( closeCurrentProject() ) );

		//Row and Column actions
	QAction *add_column    = m_row_column_actions_group.addAction( QET::Icons::EditTableInsertColumnRight, tr("Ajouter une colonne") );
	QAction *remove_column = m_row_column_actions_group.addAction( QET::Icons::EditTableDeleteColumn,      tr("Enlever une colonne") );
	QAction *add_row       = m_row_column_actions_group.addAction( QET::Icons::EditTableInsertRowUnder,    tr("Ajouter une ligne")   );
	QAction *remove_row    = m_row_column_actions_group.addAction( QET::Icons::EditTableDeleteRow,         tr("Enlever une ligne")   );

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
	delete_selection  = m_selection_actions_group.addAction( QET::Icons::EditDelete,        tr("Supprimer")							  );
	rotate_selection  = m_selection_actions_group.addAction( QET::Icons::ObjectRotateRight, tr("Pivoter")							  );
	rotate_texts      = m_selection_actions_group.addAction( QET::Icons::ObjectRotateRight, tr("Orienter les textes")				  );
	find_element      = m_selection_actions_group.addAction(                                tr("Retrouver dans le panel")			  );
	edit_selection    = m_selection_actions_group.addAction( QET::Icons::ElementEdit,       tr("Éditer l'item sélectionné")  );

	delete_selection -> setShortcut( QKeySequence::Delete);
	rotate_selection -> setShortcut( QKeySequence( tr("Space")		) );
	rotate_texts     -> setShortcut( QKeySequence( tr("Ctrl+Space") ) );
	edit_selection	 -> setShortcut( QKeySequence( tr("Ctrl+E")		) );

	delete_selection -> setStatusTip( tr("Enlève les éléments sélectionnés du folio", "status bar tip"));
	rotate_selection -> setStatusTip( tr("Pivote les éléments et textes sélectionnés", "status bar tip"));
	rotate_texts     -> setStatusTip( tr("Pivote les textes sélectionnés à un angle précis", "status bar tip"));
	find_element     -> setStatusTip( tr("Retrouve l'élément sélectionné dans le panel", "status bar tip"));

	delete_selection ->setData("delete_selection");
	rotate_selection ->setData("rotate_selection");
	rotate_texts     ->setData("rotate_selected_text");
	find_element     ->setData("find_selected_element");
	edit_selection   ->setData("edit_selected_element");

	connect(&m_selection_actions_group, &QActionGroup::triggered, this, &QETDiagramEditor::selectionGroupTriggered);

		//Select Action
	QAction *select_all     = m_select_actions_group.addAction( QET::Icons::EditSelectAll, tr("Tout sélectionner")	   );
	QAction *select_nothing = m_select_actions_group.addAction(                            tr("Désélectionner tout") );
	QAction *select_invert  = m_select_actions_group.addAction(                            tr("Inverser la sélection")  );

	select_all     -> setShortcut(QKeySequence::SelectAll);
	select_nothing -> setShortcut(QKeySequence::Deselect);
	select_invert  -> setShortcut(QKeySequence( tr("Ctrl+I")));

	select_all     -> setStatusTip( tr("Sélectionne tous les éléments du folio", "status bar tip")																	  );
	select_nothing -> setStatusTip( tr("Désélectionne tous les éléments du folio", "status bar tip")															  );
	select_invert  -> setStatusTip( tr("Désélectionne les éléments sélectionnés et sélectionne les éléments non sélectionnés", "status bar tip") );

	select_all     -> setData("select_all");
	select_nothing -> setData("deselect");
	select_invert  -> setData("invert_selection");

	connect(&m_select_actions_group, &QActionGroup::triggered, this, &QETDiagramEditor::selectGroupTriggered);

		//Zoom actions
	QAction *zoom_in      = m_zoom_actions_group.addAction( QET::Icons::ZoomIn,       tr("Zoom avant"));
	QAction *zoom_out     = m_zoom_actions_group.addAction( QET::Icons::ZoomOut,      tr("Zoom arrière"));
	QAction *zoom_content = m_zoom_actions_group.addAction( QET::Icons::ZoomDraw,     tr("Zoom sur le contenu"));
	QAction *zoom_fit     = m_zoom_actions_group.addAction( QET::Icons::ZoomFitBest,  tr("Zoom adapté"));
	QAction *zoom_reset   = m_zoom_actions_group.addAction( QET::Icons::ZoomOriginal, tr("Pas de zoom"));
	m_zoom_action_toolBar << zoom_content << zoom_fit << zoom_reset;

	zoom_in      -> setShortcut( QKeySequence::ZoomIn         );
	zoom_out     -> setShortcut( QKeySequence::ZoomOut        );
	zoom_content -> setShortcut( QKeySequence( tr("Ctrl+8") ) );
	zoom_fit     -> setShortcut( QKeySequence( tr("Ctrl+9") ) );
	zoom_reset   -> setShortcut( QKeySequence( tr("Ctrl+0") ) );

	zoom_in      -> setStatusTip(tr("Agrandit le folio", "status bar tip"));
	zoom_out     -> setStatusTip(tr("Rétrécit le folio", "status bar tip"));
	zoom_content -> setStatusTip(tr("Adapte le zoom de façon à afficher tout le contenu du folio indépendamment du cadre"));
	zoom_fit     -> setStatusTip(tr("Adapte le zoom exactement sur le cadre du folio", "status bar tip"));
	zoom_reset   -> setStatusTip(tr("Restaure le zoom par défaut", "status bar tip"));

	zoom_in     ->setData("zoom_in");
	zoom_out    ->setData("zoom_out");
	zoom_content->setData("zoom_content");
	zoom_fit    ->setData("zoom_fit");
	zoom_reset  ->setData("zoom_reset");

	connect(&m_zoom_actions_group, &QActionGroup::triggered, this, &QETDiagramEditor::zoomGroupTriggered);

		//Adding action (add text, image, shape...)
	QAction *add_text      = m_add_item_actions_group.addAction(QET::Icons::PartTextField, tr("Ajouter un champ de texte"));
	QAction *add_image	   = m_add_item_actions_group.addAction(QET::Icons::adding_image,  tr("Ajouter une image"));
	QAction *add_line	   = m_add_item_actions_group.addAction(QET::Icons::PartLine,      tr("Ajouter une ligne"));
	QAction *add_rectangle = m_add_item_actions_group.addAction(QET::Icons::PartRectangle, tr("Ajouter un rectangle"));
	QAction *add_ellipse   = m_add_item_actions_group.addAction(QET::Icons::PartEllipse,   tr("Ajouter une ellipse"));
	QAction *add_polyline  = m_add_item_actions_group.addAction(QET::Icons::PartPolygon,   tr("Ajouter une polyligne"));

	add_text     ->setData("text");
	add_image    ->setData("image");
	add_line     ->setData("line");
	add_rectangle->setData("rectangle");
	add_ellipse  ->setData("ellipse");
	add_polyline ->setData("polyline");

	foreach (QAction *action, m_add_item_actions_group.actions()) action->setCheckable(true);
	connect(&m_add_item_actions_group, &QActionGroup::triggered, this, &QETDiagramEditor::addItemGroupTriggered);

		//Keyboard shortcut
	export_diagram    -> setShortcut(QKeySequence(tr("Ctrl+Shift+X")));
	print             -> setShortcut(QKeySequence(QKeySequence::Print));
	quit_editor       -> setShortcut(QKeySequence(tr("Ctrl+Q")));


	prj_add_diagram   -> setShortcut(QKeySequence(tr("Ctrl+T")));

	next_window       -> setShortcut(QKeySequence::NextChild);
	prev_window       -> setShortcut(QKeySequence::PreviousChild);

	// affichage dans la barre de statut
	export_diagram    -> setStatusTip(tr("Exporte le folio courant dans un autre format", "status bar tip"));
	print             -> setStatusTip(tr("Imprime un ou plusieurs folios du projet courant", "status bar tip"));
	quit_editor       -> setStatusTip(tr("Ferme l'application QElectroTech", "status bar tip"));
	conductor_reset   -> setStatusTip(tr("Recalcule les chemins des conducteurs sans tenir compte des modifications", "status bar tip"));
	infos_diagram     -> setStatusTip(tr("Édite les propriétés du folio (dimensions, informations du cartouche, propriétés des conducteurs...)", "status bar tip"));

	windowed_view_mode -> setStatusTip(tr("Présente les différents projets ouverts dans des sous-fenêtres", "status bar tip"));
	tabbed_view_mode   -> setStatusTip(tr("Présente les différents projets ouverts des onglets", "status bar tip"));

	mode_selection    -> setStatusTip(tr("Permet de sélectionner les éléments", "status bar tip"));
	mode_visualise    -> setStatusTip(tr("Permet de visualiser le folio sans pouvoir le modifier", "status bar tip"));

	tile_window       -> setStatusTip(tr("Dispose les fenêtres en mosaïque", "status bar tip"));
	cascade_window    -> setStatusTip(tr("Dispose les fenêtres en cascade", "status bar tip"));
	next_window       -> setStatusTip(tr("Active le projet suivant", "status bar tip"));
	prev_window       -> setStatusTip(tr("Active le projet précédent", "status bar tip"));
	//mode_visualise    -> setShortcut( QKeySequence( tr("Ctrl+Shift") ) );

	// traitements speciaux
	windowed_view_mode -> setCheckable(true);
	tabbed_view_mode   -> setCheckable(true);
	mode_selection     -> setCheckable(true);
	mode_visualise     -> setCheckable(true);
	mode_selection     -> setChecked(true);

	grp_visu_sel = new QActionGroup(this);
	grp_visu_sel -> addAction(mode_selection);
	grp_visu_sel -> addAction(mode_visualise);
	grp_visu_sel -> setExclusive(true);

	grp_view_mode = new QActionGroup(this);
	grp_view_mode -> addAction(windowed_view_mode);
	grp_view_mode -> addAction(tabbed_view_mode);
	grp_view_mode -> setExclusive(true);

	// connexion a des slots
	connect(quit_editor,        SIGNAL(triggered()), this,       SLOT(close())                     );
	connect(windowed_view_mode, SIGNAL(triggered()), this,       SLOT(setWindowedMode())           );
	connect(tabbed_view_mode,   SIGNAL(triggered()), this,       SLOT(setTabbedMode())             );
	connect(mode_selection,     SIGNAL(triggered()), this,       SLOT(slot_setSelectionMode())     );
	connect(mode_visualise,     SIGNAL(triggered()), this,       SLOT(slot_setVisualisationMode()) );
	connect(prj_edit_prop,      SIGNAL(triggered()), this,       SLOT(editCurrentProjectProperties()));
	connect(prj_add_diagram,    SIGNAL(triggered()), this,       SLOT(addDiagramToProject())       );
	connect(prj_del_diagram,    SIGNAL(triggered()), this,       SLOT(removeDiagramFromProject())  );
	connect(prj_clean,          SIGNAL(triggered()), this,       SLOT(cleanCurrentProject())       );
	connect(prj_diagramList,    SIGNAL(triggered()), this,       SLOT(addDiagramFolioListToProject()));
	connect(prj_nomenclature,   SIGNAL(triggered()), this,       SLOT(nomenclatureProject())       );
	connect(print,              SIGNAL(triggered()), this,       SLOT(printDialog())               );
	connect(export_diagram,     SIGNAL(triggered()), this,       SLOT(exportDialog())              );
	connect(tile_window,        SIGNAL(triggered()), &workspace, SLOT(tileSubWindows())            );
	connect(cascade_window,     SIGNAL(triggered()), &workspace, SLOT(cascadeSubWindows())         );
	connect(next_window,        SIGNAL(triggered()), &workspace, SLOT(activateNextSubWindow())     );
	connect(prev_window,        SIGNAL(triggered()), &workspace, SLOT(activatePreviousSubWindow()) );
	connect(conductor_reset,    SIGNAL(triggered()), this,       SLOT(slot_resetConductors())      );
	connect(infos_diagram,      SIGNAL(triggered()), this,       SLOT(editCurrentDiagramProperties()));
}

/**
 * @brief QETDiagramEditor::setUpToolBar
 */
void QETDiagramEditor::setUpToolBar() {
	main_bar = new QToolBar(tr("Outils"), this);
	main_bar -> setObjectName("toolbar");

	view_bar = new QToolBar(tr("Affichage"), this);
	view_bar -> setObjectName("display");

	diagram_bar = new QToolBar(tr("Schéma"), this);
	diagram_bar -> setObjectName("diagram");

	main_bar -> addActions(m_file_actions_group.actions());
	main_bar -> addAction(print);
	main_bar -> addSeparator();
	main_bar -> addAction(undo);
	main_bar -> addAction(redo);
	main_bar -> addSeparator();
	main_bar -> addAction(cut);
	main_bar -> addAction(copy);
	main_bar -> addAction(paste);
	main_bar -> addSeparator();
	main_bar -> addAction(delete_selection);
	main_bar -> addAction(rotate_selection);

	// Modes selection / visualisation et zoom
	view_bar -> addAction(mode_selection);
	view_bar -> addAction(mode_visualise);
	view_bar -> addSeparator();
	view_bar -> addAction (m_grey_background);
	view_bar -> addSeparator();
	view_bar -> addActions(m_zoom_action_toolBar);

	diagram_bar -> addAction (infos_diagram);
	diagram_bar -> addAction (conductor_reset);
	diagram_bar -> addAction (m_auto_conductor);

	m_add_item_toolBar = new QToolBar(tr("Ajouter"), this);
	m_add_item_toolBar->setObjectName("adding");
	m_add_item_toolBar->addActions(m_add_item_actions_group.actions());

	// ajout de la barre d'outils a la fenetre principale
	addToolBar(Qt::TopToolBarArea, main_bar);
	addToolBar(Qt::TopToolBarArea, view_bar);
	addToolBar(Qt::TopToolBarArea, diagram_bar);
	addToolBar(Qt::TopToolBarArea, m_add_item_toolBar);
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
	menu_fichier -> addAction(export_diagram);
	//menu_fichier -> addSeparator();
	menu_fichier -> addAction(print);
	menu_fichier -> addSeparator();
	menu_fichier -> addAction(quit_editor);

	// menu Edition
	menu_edition -> addAction(undo);
	menu_edition -> addAction(redo);
	menu_edition -> addSeparator();
	menu_edition -> addAction(cut);
	menu_edition -> addAction(copy);
	menu_edition -> addAction(paste);
	menu_edition -> addSeparator();
	menu_edition -> addActions(m_select_actions_group.actions());
	menu_edition -> addSeparator();
	menu_edition -> addActions(m_selection_actions_group.actions());
	menu_edition -> addSeparator();
	menu_edition -> addAction(conductor_reset);
	menu_edition -> addSeparator();
	menu_edition -> addAction(infos_diagram);
	menu_edition -> addActions(m_row_column_actions_group.actions());

	// menu Projet
	menu_project -> addAction(prj_edit_prop);
	menu_project -> addAction(prj_add_diagram);
	menu_project -> addAction(prj_del_diagram);
	menu_project -> addAction(prj_clean);
	menu_project -> addSeparator();
	menu_project -> addAction(prj_diagramList);
	menu_project -> addAction(prj_nomenclature);

	main_bar    -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils principale"));
	view_bar    -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils Affichage"));
	diagram_bar -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils Schéma"));
	qdw_pa      -> toggleViewAction() -> setStatusTip(tr("Affiche ou non le panel d'appareils"));
	qdw_undo    -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la liste des modifications"));

	// menu Affichage
	QMenu *projects_view_mode = menu_affichage -> addMenu(tr("Afficher les projets"));
	projects_view_mode -> setTearOffEnabled(true);
	projects_view_mode -> addAction(windowed_view_mode);
	projects_view_mode -> addAction(tabbed_view_mode);

	menu_affichage -> addSeparator();
	menu_affichage -> addAction(mode_selection);
	menu_affichage -> addAction(mode_visualise);
	menu_affichage -> addSeparator();
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
	Gere les evenements du l'editeur de schema
	Reimplemente ici pour :
	  * eviter un conflit sur le raccourci clavier "Ctrl+W" (QKeySequence::Close)
	@param e Evenement
*/
bool QETDiagramEditor::event(QEvent *e) {
	if (e -> type() == QEvent::ShortcutOverride) {
		QKeyEvent *shortcut_event = static_cast<QKeyEvent *>(e);
		if (shortcut_event && shortcut_event -> matches(QKeySequence::Close)) {
			close_file -> trigger();
			e -> accept();
			return(true);
		}
	}
	return(QETMainWindow::event(e));
}

/**
	Imprime le schema courant
*/
void QETDiagramEditor::printDialog() {
	ProjectView *current_project = currentProject();
	if (!current_project) return;
	current_project -> printProject();
}

/**
	Gere l'export de schema sous forme d'image
*/
void QETDiagramEditor::exportDialog() {
	ProjectView *current_project = currentProject();
	if (!current_project) return;
	current_project -> exportProject();
}

/**
 * @brief QETDiagramEditor::save
 * Ask the current active project to save
 */
void QETDiagramEditor::save() {
	if (ProjectView *project_view = currentProject()) {
		QETResult saved = project_view -> save();

		if (saved.isOk()) {
			save_file -> setDisabled(true);
			QETApp::projectsRecentFiles() -> fileWasOpened(project_view -> project() -> filePath());

			QString title = (project_view -> project() -> title ());
			if (title.isEmpty()) title = "QElectroTech ";
			QString filePath = (project_view -> project() -> filePath ());
			statusBar()-> showMessage(tr("Projet %1 enregistré dans le repertoire: %2.").arg(title).arg (filePath), 2000);
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
	if (ProjectView *project_view = currentProject()) {
		QETResult save_file = project_view -> saveAs();
		if (save_file.isOk()) {
			QETApp::projectsRecentFiles() -> fileWasOpened(project_view -> project() -> filePath());

			QString title = (project_view -> project() -> title ());
			if (title.isEmpty()) title = "QElectroTech ";
			QString filePath = (project_view -> project() -> filePath ());
			statusBar()->showMessage(tr("Projet %1 enregistré dans le repertoire: %2.").arg(title).arg (filePath), 2000);
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
	Ferme le projet courant
	@return true si la fermeture du fichier a reussi, false sinon
	Note : cette methode renvoie true s'il n'y a pas de projet courant
*/
bool QETDiagramEditor::closeCurrentProject() {
	if (ProjectView *project_view = currentProject()) {
		return(closeProject(project_view));
	}
	return(true);
}

/**
	Ouvre un projet depuis un fichier et l'ajoute a cet editeur
	@param filepath Chemin du projet a ouvrir
	@param interactive true pour afficher des messages a l'utilisateur, false sinon
	@param update_panel Whether the elements panel should be warned this
	project has been added. Defaults to true.
	@return true si l'ouverture a reussi, false sinon
*/
bool QETDiagramEditor::openAndAddProject(const QString &filepath, bool interactive, bool update_panel) {
	if (filepath.isEmpty()) return(false);
	
	QFileInfo filepath_info(filepath);
	// verifie que le projet n'est pas deja ouvert dans un editeur
	QString my_filepath = filepath_info.canonicalFilePath();
	if (QETDiagramEditor *diagram_editor = QETApp::diagramEditorForFile(filepath)) {
		if (diagram_editor == this) {
			if (ProjectView *project_view = viewForFile(filepath)) {
				activateWidget(project_view);
				show();
				activateWindow();
			}
			return(false);
		} else {
			// demande a l'autre editeur d'afficher le fichier
			return(diagram_editor -> openAndAddProject(filepath));
		}
	}
	
	// check the file exists
	if (!filepath_info.exists()) {
		if (interactive) {
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
	
	// verifie que le fichier est accessible en lecture
	if (!filepath_info.isReadable()) {
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
	
	// gere le fait que le fichier puisse etre en lecture seule
	if (!filepath_info.isWritable()) {
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
	
	// cree le projet a partir du fichier
	QETProject *project = new QETProject(filepath);
	if (project -> state() != QETProject::Ok) {
		if (interactive && project -> state() != QETProject::FileOpenDiscard) {
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
		return(false);
	}

	// a ce stade, l'ouverture du fichier a reussi
	// on l'ajoute a la liste des fichiers recents
	QETApp::projectsRecentFiles() -> fileWasOpened(filepath);
	// ... et on l'ajoute dans l'application
	// Note: we require the panel not to be updated when the project is added
	// because it will update itself as soon as it becomes visible
	return(addProject(project), update_panel);
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
	
	// met a jour le panel d'elements
	if (update_panel) {
		pa -> elementsPanel().projectWasOpened(project);
	}
	
	return(true);
}

/**
	@return la liste des projets ouverts dans cette fenetre
*/
QList<ProjectView *> QETDiagramEditor::openedProjects() const {
	QList<ProjectView *> result;
	QList<QMdiSubWindow *> window_list(workspace.subWindowList());
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
ProjectView *QETDiagramEditor::currentProject() const {
	QMdiSubWindow *current_window = workspace.activeSubWindow();
	if (!current_window) return(0);
	
	QWidget *current_widget = current_window -> widget();
	if (!current_widget) return(0);
	
	if (ProjectView *project_view = qobject_cast<ProjectView *>(current_widget)) {
		return(project_view);
	}
	return(0);
}

/**
	@return Le schema actuellement edite (= l'onglet ouvert dans le projet
	courant) ou 0 s'il n'y en a pas
*/
DiagramView *QETDiagramEditor::currentDiagram() const {
	if (ProjectView *project_view = currentProject()) {
		return(project_view -> currentDiagram());
	}
	return(0);
}

/**
	@return the selected element in the current diagram view, or 0 if:
	  * no diagram is being viewed in this editor.
	  * no element is selected
	  * more than one element is selected
*/
Element *QETDiagramEditor::currentElement() const {
	DiagramView *dv = currentDiagram();
	if (!dv) return(0);
	
	QList<Element *> selected_elements = dv -> diagram() -> selectedContent().elements.toList();
	if (selected_elements.count() != 1) return(0);
	
	return(selected_elements.first());
}

/**
	@return the selected element in the current diagram view, or 0 if:
	  * no diagram is being viewed in this editor.
	  * no element is selected
	  * more than one element is selected
	  * the selected element is not a custom element
*/
CustomElement *QETDiagramEditor::currentCustomElement() const {
	return(dynamic_cast<CustomElement *>(currentElement()));
}

/**
	Cette methode permet de retrouver le projet contenant un schema donne.
	@param diagram_view Schema dont il faut retrouver
	@return la vue sur le projet contenant ce schema ou 0 s'il n'y en a pas
*/
ProjectView *QETDiagramEditor::findProject(DiagramView *diagram_view) const {
	foreach(ProjectView *project_view, openedProjects()) {
		if (project_view -> diagrams().contains(diagram_view)) {
			return(project_view);
		}
	}
	return(0);
}

/**
	Cette methode permet de retrouver le projet contenant un schema donne.
	@param diagram Schema dont il faut retrouver
	@return la vue sur le projet contenant ce schema ou 0 s'il n'y en a pas
*/
ProjectView *QETDiagramEditor::findProject(Diagram *diagram) const {
	foreach(ProjectView *project_view, openedProjects()) {
		foreach(DiagramView *diagram_view, project_view -> diagrams()) {
			if (diagram_view -> diagram() == diagram) {
				return(project_view);
			}
		}
	}
	return(0);
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
	return(0);
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
	return(0);
}

/**
	@param widget Widget a rechercher dans la zone MDI
	@return La sous-fenetre accueillant le widget passe en parametre, ou 0 si
	celui-ci n'a pas ete trouve.
*/
QMdiSubWindow *QETDiagramEditor::subWindowForWidget(QWidget *widget) const {
	foreach(QMdiSubWindow *sub_window, workspace.subWindowList()) {
		if (sub_window -> widget() == widget) {
			return(sub_window);
		}
	}
	return(0);
}

/**
	@param widget Widget a activer
*/
void QETDiagramEditor::activateWidget(QWidget *widget) {
	QMdiSubWindow *sub_window = subWindowForWidget(widget);
	if (sub_window) {
		workspace.setActiveSubWindow(sub_window);
	}
}

/**
	Effectue l'action "couper" sur le schema en cours
*/
void QETDiagramEditor::slot_cut() {
	if(currentDiagram()) currentDiagram() -> cut();
}

/**
	Effectue l'action "copier" sur le diagram en cours
*/
void QETDiagramEditor::slot_copy() {
	if(currentDiagram()) currentDiagram() -> copy();
}

/**
	Effectue l'action "coller" sur le schema en cours
*/
void QETDiagramEditor::slot_paste() {
	if(currentDiagram()) currentDiagram() -> paste();
}

void QETDiagramEditor::zoomGroupTriggered(QAction *action)
{
	QString value = action->data().toString();
	DiagramView *dv = currentDiagram();

	if (!dv || value.isEmpty()) return;

	if (value == "zoom_in")
		dv->zoomIn();
	else if (value == "zoom_out")
		dv->zoomOut();
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
	DiagramView *dv = currentDiagram();

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
	DiagramView *dv = currentDiagram();

	if (!dv || value.isEmpty()) return;

	DVEventInterface *dvevent = nullptr;

	if (value == "text")
		dvevent = new DVEventAddText(dv);
	else if (value == "image")
	{
		DVEventAddImage *event = new DVEventAddImage(dv);
		if (event -> isNull())
		{
			delete event;
			action->setChecked(false);
			return;
		}
		else
			dvevent = event;
	}
	else if (value == "line")
		dvevent = new DVEventAddShape(dv, QetShapeItem::Line);
	else if (value == "rectangle")
		dvevent = new DVEventAddShape(dv, QetShapeItem::Rectangle);
	else if (value == "ellipse")
		dvevent = new DVEventAddShape(dv, QetShapeItem::Ellipse);
	else if (value == "polyline")
		dvevent = new DVEventAddShape(dv, QetShapeItem::Polygon);

	if (dvevent)
	{
		dv->setEventInterface(dvevent);
		connect(dvevent, &DVEventInterface::finish, [action](){action->setChecked(false);});
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
	DiagramView *dv = currentDiagram();

	if (!dv || value.isEmpty()) return;

	if (value == "delete_selection")
		dv->deleteSelection();
	else if (value == "rotate_selection")
		dv->rotateSelection();
	else if (value == "rotate_selected_text")
		dv->rotateTexts();
	else if (value == "find_selected_element" && currentCustomElement())
		findElementInPanel(currentCustomElement()->location());
	else if (value == "edit_selected_element")
		dv->editSelection();
}

void QETDiagramEditor::rowColumnGroupTriggered(QAction *action)
{
	QString value = action->data().toString();
	DiagramView *dv = currentDiagram();

	if (!dv || value.isEmpty()) return;

	if (value == "add_column")
		dv->addColumn();
	else if (value == "remove_column")
		dv->removeColumn();
	else if (value == "add_row")
		dv->addRow();
	else if (value == "remove_row")
		dv->removeRow();
}

/**
 * @brief QETDiagramEditor::slot_setSelectionMode
 * Set all diagram view of the current project to selection mode
 */
void QETDiagramEditor::slot_setSelectionMode()
{
	if (ProjectView *pv = currentProject())
		foreach(DiagramView *dv, pv -> diagrams())
			dv -> setSelectionMode();
}

/**
 * @brief QETDiagramEditor::slot_setVisualisationMode
 * Set all diagram view of the current project to visualisation mode
 */
void QETDiagramEditor::slot_setVisualisationMode()
{
	if (ProjectView *pv = currentProject())
		foreach(DiagramView *dv, pv -> diagrams())
			dv -> setVisualisationMode();
}

/**
 * @brief QETDiagramEditor::slot_updateActions
 * Manage actions
 */
void QETDiagramEditor::slot_updateActions()
{
	DiagramView *dv = currentDiagram();
	ProjectView *pv = currentProject();

	bool opened_project = pv;
	bool opened_diagram = dv;
	bool editable_project = (pv && !pv -> project() -> isReadOnly());

	close_file       -> setEnabled(opened_project);
	save_file_as     -> setEnabled(opened_project);
	prj_edit_prop    -> setEnabled(opened_project);
	prj_add_diagram  -> setEnabled(editable_project);
	prj_del_diagram  -> setEnabled(editable_project);
	prj_clean        -> setEnabled(editable_project);
	prj_diagramList  -> setEnabled(opened_project);
	prj_nomenclature -> setEnabled(editable_project);
	export_diagram   -> setEnabled(opened_diagram);
	print            -> setEnabled(opened_diagram);
	infos_diagram    -> setEnabled(opened_diagram);
	prj_nomenclature -> setEnabled(editable_project);
	m_zoom_actions_group.      setEnabled(opened_diagram);
	m_select_actions_group.    setEnabled(opened_diagram);
	m_add_item_actions_group.  setEnabled(editable_project);
	m_row_column_actions_group.setEnabled(editable_project);
	m_grey_background->setEnabled(opened_diagram);


	slot_updateUndoStack();
	slot_updateModeActions();
	slot_updatePasteAction();
	slot_updateComplexActions();
}

/**
 * @brief QETDiagramEditor::slot_updateUndoStack
 * Update the undo stack view
 */
void QETDiagramEditor::slot_updateUndoStack()
{
	ProjectView *pv = currentProject();
	if (pv)
	{
		undo_group.setActiveStack(pv->project()->undoStack());
		save_file -> setEnabled (undo_group.activeStack() -> count() && !pv -> project() -> isReadOnly());
	}
	else
	{
		save_file -> setDisabled(true);
	}
}

/**
 * @brief QETDiagramEditor::slot_updateComplexActions
 * Manage the actions who need some conditions to be enable or not.
 * This method does nothing if there is no project opened
 */
void QETDiagramEditor::slot_updateComplexActions() {
	DiagramView *dv = currentDiagram();
	bool editable_diagram = (dv && !dv -> diagram() -> isReadOnly());

		//Number of selected conductors
	int selected_conductors_count = dv ? dv -> diagram() -> selectedConductors().count() : 0;
	conductor_reset  -> setEnabled(editable_diagram && selected_conductors_count);
	
		// number of selected elements
	int selected_elements_count = dv ? dv -> diagram() -> selectedContent().count(DiagramContent::Elements) : 0;
	find_element -> setEnabled(selected_elements_count == 1);
	
		//Action that need items (elements, conductors, texts...) selected, to be enabled
	bool copiable_items  = dv ? (dv -> hasCopiableItems()) : false;
	bool deletable_items = dv ? (dv -> hasDeletableItems()) : false;
	cut              -> setEnabled(editable_diagram && copiable_items);
	copy             -> setEnabled(copiable_items);
	delete_selection -> setEnabled(editable_diagram && deletable_items);
	rotate_selection -> setEnabled(editable_diagram && dv -> diagram() -> canRotateSelection());

		//Action that need selected texts
	int selected_texts = dv ? (dv -> diagram() -> selectedTexts().count()) : 0;
	int selected_conductor_texts = dv ? (dv -> diagram() -> selectedConductorTexts().count()) : 0;
	int selected_element_texts = dv ? (dv -> diagram() -> selectedElementTexts().count()) : 0;
	rotate_texts -> setEnabled(editable_diagram && selected_texts);

		// actions need only one editable item
	int selected_image = dv ? dv -> diagram() -> selectedContent().count(DiagramContent::Images) : 0;

	int selected_shape = dv ? dv -> diagram() -> selectedContent().count(DiagramContent::Shapes) : 0;
	int selected_editable = selected_elements_count +
							(selected_texts - selected_conductor_texts - selected_element_texts) +
							selected_image +
							selected_shape +
							selected_conductors_count;

	if (selected_editable == 1)
	{
		edit_selection -> setEnabled(true);
			//edit element
		if (selected_elements_count)
		{
			edit_selection -> setText(tr("Éditer l'élement", "edit element"));
			edit_selection -> setIcon(QET::Icons::ElementEdit);
		}
			//edit text field
		else if (selected_texts)
		{
			edit_selection -> setText(tr("Éditer le champ de texte", "edit text field"));
			edit_selection -> setIcon(QET::Icons::EditText);
		}
			//edit image
		else if (selected_image)
		{
			edit_selection -> setText(tr("Éditer l'image", "edit image"));
			edit_selection -> setIcon(QET::Icons::resize_image);
		}
			//edit conductor
		else if (selected_conductors_count)
		{
			edit_selection -> setText(tr("Éditer le conducteur", "edit conductor"));
			edit_selection -> setIcon(QET::Icons::ElementEdit);
		}
	}
		//not an editable item
	else
	{
		edit_selection -> setText(tr("Éditer l'objet sélectionné", "edit selected item"));
		edit_selection -> setIcon(QET::Icons::ElementEdit);
		edit_selection -> setEnabled(false);
	}
}

/**
 * @brief QETDiagramEditor::slot_updateModeActions
 * Manage action who need an opened diagram or project to be updated
 */
void QETDiagramEditor::slot_updateModeActions()
{
	DiagramView *dv = currentDiagram();
	
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
				mode_visualise -> setChecked(true);
				break;
			case QGraphicsView::RubberBandDrag:
				grp_visu_sel -> setEnabled(true);
				mode_selection -> setChecked(true);
				break;
		}
	}

	if (ProjectView *pv = currentProject())
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
	DiagramView *dv = currentDiagram();
	bool editable_diagram = (dv && !dv -> diagram() -> isReadOnly());
	
	// pour coller, il faut un schema ouvert et un schema dans le presse-papier
	paste -> setEnabled(editable_diagram && Diagram::clipboardMayContainDiagram());
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

	foreach(DiagramView *dv, project_view -> diagrams())
		diagramWasAdded(dv);
	
		//Manage the close event of project
	connect(project_view, SIGNAL(projectClosed(ProjectView*)), this, SLOT(projectWasClosed(ProjectView *)));
		//Manage the adding  of diagram
	connect(project_view, SIGNAL(diagramAdded(DiagramView *)), this, SLOT(diagramWasAdded(DiagramView *)));

	if (QETProject *project = project_view -> project())
		connect(project, SIGNAL(readOnlyChanged(QETProject *, bool)), this, SLOT(slot_updateActions()));
	
		//Manage request for edit or find element and titleblock
	connect(project_view, SIGNAL(findElementRequired(const ElementsLocation &)),                    this,               SLOT(findElementInPanel(const ElementsLocation &)));
	connect(project_view, SIGNAL(editElementRequired(const ElementsLocation &)),                    this,               SLOT(editElementInEditor(const ElementsLocation &)));
	connect(project_view, SIGNAL(editTitleBlockTemplate(const TitleBlockTemplateLocation &, bool)), QETApp::instance(), SLOT(openTitleBlockTemplate(TitleBlockTemplateLocation, bool)));
	
		// display error messages sent by the project view
	connect(project_view, SIGNAL(errorEncountered(QString)), this, SLOT(showError(const QString &)));

		//We maximise the new window if the current window is inexistent or maximized
	QWidget *current_window = workspace.activeSubWindow();
	bool     maximise       = ((!current_window) || (current_window -> windowState() & Qt::WindowMaximized));

		//Add the new window
	QMdiSubWindow *sub_window = workspace.addSubWindow(project_view);
	sub_window -> setWindowIcon(project_view -> windowIcon());
	sub_window -> systemMenu() -> clear();

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
	if (filepath.isEmpty()) return(0);
	
	QString searched_can_file_path = QFileInfo(filepath).canonicalFilePath();
	if (searched_can_file_path.isEmpty()) {
		// QFileInfo returns an empty path for non-existent files
		return(0);
	}
	foreach (ProjectView *project_view, openedProjects()) {
		QString project_can_file_path = QFileInfo(project_view -> project() -> filePath()).canonicalFilePath();
		if (project_can_file_path == searched_can_file_path) {
			return(project_view);
		}
	}
	return(0);
}

/**
	met a jour le menu "Fenetres"
*/
void QETDiagramEditor::slot_updateWindowsMenu() {
	// nettoyage du menu
	foreach(QAction *a, windows_menu -> actions()) windows_menu -> removeAction(a);
	
	// actions de fermeture
	windows_menu -> addAction(close_file);
	//windows_menu -> addAction(closeAllAct);
	
	// actions de reorganisation des fenetres
	windows_menu -> addSeparator();
	windows_menu -> addAction(tile_window);
	windows_menu -> addAction(cascade_window);
	
	// actions de deplacement entre les fenetres
	windows_menu -> addSeparator();
	windows_menu -> addAction(next_window);
	windows_menu -> addAction(prev_window);
	
	// liste des fenetres
	QList<ProjectView *> windows = openedProjects();
	
	tile_window    -> setEnabled(!windows.isEmpty() && workspace.viewMode() == QMdiArea::SubWindowView);
	cascade_window -> setEnabled(!windows.isEmpty() && workspace.viewMode() == QMdiArea::SubWindowView);
	next_window    -> setEnabled(windows.count() > 1);
	prev_window    -> setEnabled(windows.count() > 1);
	
	if (!windows.isEmpty()) windows_menu -> addSeparator();
	QActionGroup *windows_actions = new QActionGroup(this);
	foreach(ProjectView *project_view, windows) {
		QString pv_title = project_view -> windowTitle();
		QAction *action  = windows_menu -> addAction(pv_title);
		windows_actions -> addAction(action);
		action -> setStatusTip(QString(tr("Active le projet « %1 »")).arg(pv_title));
		action -> setCheckable(true);
		action -> setChecked(project_view == currentProject());
		connect(action, SIGNAL(triggered()), &windowMapper, SLOT(map()));
		windowMapper.setMapping(action, project_view);
	}
}

/**
	Edite les informations du schema en cours
*/
void QETDiagramEditor::editCurrentDiagramProperties() {
	if (ProjectView *project_view = currentProject()) {
		activateProject(project_view);
		project_view -> editCurrentDiagramProperties();
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
	Edite les proprietes des objets selectionnes
*/
void QETDiagramEditor::editSelectionProperties() {
	if (DiagramView *dv = currentDiagram()) {
		dv -> editSelectionProperties();
	}
}

/**
	Reinitialise les conducteurs selectionnes
*/
void QETDiagramEditor::slot_resetConductors() {
	if (DiagramView *dv = currentDiagram()) {
		dv -> resetConductors();
	}
}

/**
 * @brief QETDiagramEditor::slot_autoConductor
 * @param ac
 * Update the auto conductor status of current project;
 */
void QETDiagramEditor::slot_autoConductor(bool ac)
{
	if (ProjectView *pv = currentProject())
		pv -> project() -> setAutoConductor(ac);
}

/**
	Affiche les projets dans des fenetres.
*/
void QETDiagramEditor::setWindowedMode() {
	workspace.setViewMode(QMdiArea::SubWindowView);
	windowed_view_mode -> setChecked(true);
	slot_updateWindowsMenu();
}

/**
	Affiche les projets dans des onglets.
*/
void QETDiagramEditor::setTabbedMode() {
	workspace.setViewMode(QMdiArea::TabbedView);
	tabbed_view_mode -> setChecked(true);
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
		undo_group.removeStack(project -> undoStack());
		QETApp::unregisterProject(project);
	}
	project_view -> deleteLater();
	project -> deleteLater();
}

/**
	Edite les proprietes du projet courant.
*/
void QETDiagramEditor::editCurrentProjectProperties() {
	editProjectProperties(currentProject());
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
	Ajoute un nouveau schema au projet courant
*/
void QETDiagramEditor::addDiagramToProject() {
	if (ProjectView *current_project = currentProject()) {
		current_project -> addNewDiagram();
	}
}

/**
 * @brief QETDiagramEditor::addDiagramFolioListToProject
 * Add new folio list to project
 */
void QETDiagramEditor::addDiagramFolioListToProject() {
	if (ProjectView *current_project = currentProject())
		current_project -> addNewDiagramFolioList();
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


/**
	Nettoie le projet courant
*/
void QETDiagramEditor::cleanCurrentProject() {
	if (ProjectView *current_project = currentProject()) {
		int clean_count = current_project -> cleanProject();
		if (clean_count) pa -> reloadAndFilter();
	}
}

/**
 * @brief export nomemclature of schema
 */
void QETDiagramEditor::nomenclatureProject() {
	//TODO: Test nomenclature CYRIL F.
	nomenclature nomencl(currentProject()->project() ,this);
	nomencl.saveToCSVFile();
}

/**
	Supprime le schema courant du projet courant
*/
void QETDiagramEditor::removeDiagramFromProject() {
	if (ProjectView *current_project = currentProject()) {
		if (DiagramView *current_diagram = current_project -> currentDiagram()) {
			bool isFolioList = false;

			// if diagram to remove is a "folio list sheet", then set a flag.
			if (dynamic_cast<DiagramFolioList *>(current_diagram -> diagram()))
				isFolioList = true;

			current_project -> removeDiagram(current_diagram);

			// if the removed diagram was a folio sheet, then delete all the remaining folio sheets also.
			if (isFolioList) {
				foreach (DiagramView *diag, current_project -> diagrams()) {
					if (dynamic_cast<DiagramFolioList *>(diag -> diagram())) {
						current_project -> removeDiagram(diag);
					}
				}

			  // else if after diagram removal, the total diagram quantity becomes a factor of 58, then
			  // remove one (last) folio sheet.
			} else if (current_project -> diagrams().size() % 58 == 0) {
				foreach (DiagramView *diag, current_project -> diagrams()) {
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
	@param location Emplacement de l'element a retrouver dans le panel
	d'elements.
*/
void QETDiagramEditor::findElementInPanel(const ElementsLocation &location) {
	bool element_found = pa -> elementsPanel().scrollToElement(location);
	if (!element_found) {
		// l'element n'a pas ete trouve
		
		ElementsCollectionItem *element = QETApp::collectionItem(location);
		if (element) {
			// mais il semble exister tout de meme
			
			// peut-etre vient-il d'un projet ouvert dans un autre editeur ?
			if (location.project() && !findProject(location.project())) {
				statusBar() -> showMessage(
					tr("Impossible de retrouver cet élément dans le panel car il semble édité dans une autre fenêtre"),
					10000
				);
			} else {
				// il devrait etre affiche : on tente de recharger le panel
				statusBar() -> showMessage(
					tr("Impossible de retrouver cet élément dans le panel... rechargement du panel..."),
					10000
				);
				pa -> reloadAndFilter();
				statusBar() -> clearMessage();
				element_found = pa -> elementsPanel().scrollToElement(location);
			}
		}
	}
	
	if (!element_found) {
		statusBar() -> showMessage(
			tr("Impossible de retrouver cet élément dans le panel"),
			10000
		);
	}
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
	if (CustomElement *selected_element = currentCustomElement()) {
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

	DiagramView *dv = currentDiagram();
	if (dv && dv->diagram())
		m_selection_properties_editor->setDiagram(dv->diagram());
}

/**
 * @brief QETDiagramEditor::activeUndoStackCleanChanged
 * Enable the QAction save_file when @clean is set to false
 * @clean at true do nothing;
 * @param clean
 */
void QETDiagramEditor::activeUndoStackCleanChanged(bool clean) {
	if (!clean) {
		save_file -> setEnabled(true);
	}
}
