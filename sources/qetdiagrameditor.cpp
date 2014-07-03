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

#include "ui/dialogautonum.h"

#include <QMessageBox>

/**
	constructeur
	@param files Liste de fichiers a ouvrir
	@param parent le widget parent de la fenetre principale
 */
QETDiagramEditor::QETDiagramEditor(const QStringList &files, QWidget *parent) :
	QETMainWindow(parent),
	open_dialog_dir(QDesktopServices::storageLocation(QDesktopServices::DesktopLocation)),
	can_update_actions(true),
	m_add_item_actions_group(this),
	m_zoom_actions_group(this),
	m_select_actions_group(this),
	m_selection_actions_group(this),
	m_row_column_actions_group(this),
	m_file_actions_group(this)
{
	// mise en place de l'interface MDI au centre de l'application
	setCentralWidget(&workspace);
	
	// nomme l'objet afin qu'il soit reperable par les feuilles de style
	workspace.setBackground(QBrush(Qt::NoBrush));
	workspace.setObjectName("mdiarea");
	
#if QT_VERSION >= 0x040800
	workspace.setTabsClosable(true);
#endif
	
	// mise en place du signalmapper
	connect(&windowMapper, SIGNAL(mapped(QWidget *)), this, SLOT(activateWidget(QWidget *)));
	
	// titre de la fenetre
	setWindowTitle(tr("QElectroTech", "window title"));
	
	// icone de la fenetre
	setWindowIcon(QET::Icons::QETLogo);
	
	// barre de statut de la fenetre
	statusBar() -> showMessage(tr("QElectroTech", "status bar message"));
	
	// ajout du panel d'Elements en tant que QDockWidget
	qdw_pa = new QDockWidget(tr("Panel d'\351l\351ments", "dock title"), this);
	qdw_pa -> setObjectName("elements panel");
	qdw_pa -> setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	qdw_pa -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	qdw_pa -> setMinimumWidth(160);
	qdw_pa -> setWidget(pa = new ElementsPanelWidget(qdw_pa));
	
	connect(pa, SIGNAL(requestForDiagram(Diagram *)),                     this, SLOT(activateDiagram(Diagram *)));
	connect(pa, SIGNAL(requestForProject(QETProject *)),                  this, SLOT(activateProject(QETProject *)));
	connect(pa, SIGNAL(requestForProjectClosing(QETProject *)),           this, SLOT(closeProject(QETProject *)));
	connect(pa, SIGNAL(requestForProjectPropertiesEdition(QETProject *)), this, SLOT(editProjectProperties(QETProject *)));
	connect(pa, SIGNAL(requestForDiagramPropertiesEdition(Diagram *)),    this, SLOT(editDiagramProperties(Diagram *)));
	connect(pa, SIGNAL(requestForNewDiagram(QETProject *)),               this, SLOT(addDiagramToProject(QETProject *)));
	connect(pa, SIGNAL(requestForDiagramDeletion(Diagram *)),             this, SLOT(removeDiagram(Diagram *)));
	connect(pa, SIGNAL(requestForDiagramMoveUp(Diagram *)),               this, SLOT(moveDiagramUp(Diagram *)));
	connect(pa, SIGNAL(requestForDiagramMoveDown(Diagram *)),             this, SLOT(moveDiagramDown(Diagram *)));
	
	qdw_undo = new QDockWidget(tr("Annulations", "dock title"));
	qdw_undo -> setObjectName("diagram_undo");
	qdw_pa -> setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	qdw_undo -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	qdw_undo -> setMinimumWidth(160);
	tabifyDockWidget(qdw_pa, qdw_undo);
	QUndoView *undo_view = new QUndoView(&undo_group, this);
	undo_view -> setEmptyLabel(tr("Aucune modification"));
	undo_view -> setStatusTip(tr("Cliquez sur une action pour revenir en arri\350re dans l'\351dition de votre sch\351ma", "Status tip"));
	undo_view -> setWhatsThis(tr("Ce panneau liste les diff\351rentes actions effectu\351es sur le sch\351ma courant. Cliquer sur une action permet de revenir \340 l'\351tat du sch\351ma juste apr\350s son application.", "\"What's this\" tip"));
	qdw_undo -> setWidget(undo_view);
	
	addDockWidget(Qt::LeftDockWidgetArea, qdw_undo);
	tabifyDockWidget(qdw_undo, qdw_pa);
	
	// mise en place des actions
	actions();
	
	// mise en place de la barre d'outils
	toolbar();
	
	// mise en place des menus
	menus();
	
	// la fenetre est maximisee par defaut
	setMinimumSize(QSize(500, 350));
	setWindowState(Qt::WindowMaximized);
	
	// connexions signaux / slots pour une interface sensee
	connect(&workspace,                SIGNAL(subWindowActivated(QMdiSubWindow *)), this, SLOT(slot_updateWindowsMenu()));
	connect(&workspace,                SIGNAL(subWindowActivated(QMdiSubWindow *)), this, SLOT(slot_updateUndoStack()));
	connect(QApplication::clipboard(), SIGNAL(dataChanged()),              this, SLOT(slot_updatePasteAction()));
	
	// lecture des parametres
	readSettings();
	
	// affichage
	show();
	
	// si des chemins de fichiers valides sont passes en arguments
	uint opened_projects = 0;
	if (files.count()) {
		// alors on ouvre ces fichiers
		foreach(QString file, files) {
			bool project_opening = openAndAddProject(file, false);
			if (project_opening) {
				++ opened_projects;
			}
		}
	}
	
	// si aucun schema n'a ete ouvert jusqu'a maintenant, on ouvre un nouveau schema
	if (!opened_projects) newProject();
}

/**
	Destructeur
*/
QETDiagramEditor::~QETDiagramEditor() {
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
	Mise en place des actions
*/
void QETDiagramEditor::actions() {
	// icones et labels
	export_diagram    = new QAction(QET::Icons::DocumentExport,        tr("E&xporter"),                            this);
	print             = new QAction(QET::Icons::DocumentPrint,         tr("Imprimer"),                             this);
	quit_editor       = new QAction(QET::Icons::ApplicationExit,       tr("&Quitter"),                             this);
	
	undo = undo_group.createUndoAction(this, tr("Annuler"));
	undo -> setIcon(QET::Icons::EditUndo);
	redo = undo_group.createRedoAction(this, tr("Refaire"));
	redo -> setIcon(QET::Icons::EditRedo);
	cut               = new QAction(QET::Icons::EditCut,               tr("Co&uper"),                              this);
	copy              = new QAction(QET::Icons::EditCopy,              tr("Cop&ier"),                              this);
	paste             = new QAction(QET::Icons::EditPaste,             tr("C&oller"),                              this);
	conductor_reset   = new QAction(QET::Icons::ConductorSettings,     tr("R\351initialiser les conducteurs"),     this);
	infos_diagram     = new QAction(QET::Icons::DialogInformation,     tr("Propri\351t\351s du sch\351ma"),        this);
	prj_edit_prop     = new QAction(QET::Icons::DialogInformation,     tr("Propri\351t\351s du projet"),           this);
	prj_add_diagram   = new QAction(QET::Icons::DiagramAdd,            tr("Ajouter un sch\351ma"),                 this);
	prj_del_diagram   = new QAction(QET::Icons::DiagramDelete,         tr("Supprimer le sch\351ma"),               this);
	prj_clean         = new QAction(QET::Icons::EditClear,             tr("Nettoyer le projet"),                   this);
	prj_diagramNum    = new QAction(QET::Icons::ConductorSettings,     tr("Annoter les sch\351mas"),               this);
	prj_diagramList   = new QAction(QET::Icons::listDrawings,          tr("Ajouter un sommaire"),                  this);
	prj_nomenclature  = new QAction(QET::Icons::DocumentExport,        tr("Exporter une nomenclature (beta)"),     this);
	tabbed_view_mode  = new QAction(                                   tr("en utilisant des onglets"),             this);
	windowed_view_mode= new QAction(                                   tr("en utilisant des fen\352tres"),         this);
	mode_selection    = new QAction(QET::Icons::PartSelect,            tr("Mode Selection"),                       this);
	mode_visualise    = new QAction(QET::Icons::ViewMove,              tr("Mode Visualisation"),                   this);
	tile_window        = new QAction(                                  tr("&Mosa\357que"),                         this);
	cascade_window     = new QAction(                                  tr("&Cascade"),                             this);
	next_window        = new QAction(                                  tr("Projet suivant"),                       this);
	prev_window        = new QAction(                                  tr("Projet pr\351c\351dent"),               this);
	
	///Files action///
	QAction *new_file  = m_file_actions_group.addAction( QET::Icons::DocumentNew,     tr("&Nouveau")						  );
	QAction *open_file = m_file_actions_group.addAction( QET::Icons::DocumentOpen,    tr("&Ouvrir")							  );
	save_file          = m_file_actions_group.addAction( QET::Icons::DocumentSave,    tr("&Enregistrer")					  );
	save_file_as       = m_file_actions_group.addAction( QET::Icons::DocumentSaveAs,  tr("Enregistrer sous")				  );
	save_cur_diagram   = m_file_actions_group.addAction( QET::Icons::DocumentSaveAll, tr("&Enregistrer le sch\351ma courant") );
	close_file         = m_file_actions_group.addAction( QET::Icons::DocumentClose,   tr("&Fermer")							  );

	new_file   -> setShortcut( QKeySequence::New   );
	open_file  -> setShortcut( QKeySequence::Open  );
	close_file -> setShortcut( QKeySequence::Close );
	save_file  -> setShortcut( QKeySequence::Save  );

	new_file         -> setStatusTip( tr("Cr\351e un nouveau sch\351ma", "status bar tip")								 );
	open_file        -> setStatusTip( tr("Ouvre un sch\351ma existant", "status bar tip")								 );
	close_file       -> setStatusTip( tr("Ferme le sch\351ma courant", "status bar tip")								 );
	save_file        -> setStatusTip( tr("Enregistre le projet courant et tous ses sch\351mas", "status bar tip")		 );
	save_file_as     -> setStatusTip( tr("Enregistre le project courant avec un autre nom de fichier", "status bar tip") );
	save_cur_diagram -> setStatusTip( tr("Enregistre le sch\351ma courant du projet courant", "status bar tip")			 );

	connect(save_file_as,     SIGNAL( triggered() ), this, SLOT( saveAs()			   ) );
	connect(save_file,        SIGNAL( triggered() ), this, SLOT( save()				   ) );
	connect(save_cur_diagram, SIGNAL( triggered() ), this, SLOT( saveCurrentDiagram()  ) );
	connect(new_file,         SIGNAL( triggered() ), this, SLOT( newProject()		   ) );
	connect(open_file,        SIGNAL( triggered() ), this, SLOT( openProject()		   ) );
	connect(close_file,       SIGNAL( triggered() ), this, SLOT( closeCurrentProject() ) );

	///Row Column action///
	QAction *add_column    = m_row_column_actions_group.addAction( QET::Icons::EditTableInsertColumnRight, tr("Ajouter une colonne") );
	QAction *remove_column = m_row_column_actions_group.addAction( QET::Icons::EditTableDeleteColumn,      tr("Enlever une colonne") );
	QAction *add_row       = m_row_column_actions_group.addAction( QET::Icons::EditTableInsertRowUnder,    tr("Ajouter une ligne")   );
	QAction *remove_row    = m_row_column_actions_group.addAction( QET::Icons::EditTableDeleteRow,         tr("Enlever une ligne")   );

	add_column    -> setStatusTip( tr("Ajoute une colonne au sch\351ma", "status bar tip")		  );
	remove_column -> setStatusTip( tr("Enl\350ve une colonne au sch\351ma", "status bar tip")	  );
	add_row       -> setStatusTip( tr("Agrandit le sch\351ma en hauteur", "status bar tip")		  );
	remove_row    -> setStatusTip( tr("R\351tr\351cit le sch\351ma en hauteur", "status bar tip") );

	connect(add_column,    SIGNAL( triggered() ), this, SLOT( slot_addColumn()	  ) );
	connect(remove_column, SIGNAL( triggered() ), this, SLOT( slot_removeColumn() ) );
	connect(add_row,       SIGNAL( triggered() ), this, SLOT( slot_addRow()		  ) );
	connect(remove_row,    SIGNAL( triggered() ), this, SLOT( slot_removeRow()	  ) );

	///Selections Actions (related to a selected item)///
	delete_selection  = m_selection_actions_group.addAction( QET::Icons::EditDelete,        tr("Supprimer")							  );
	rotate_selection  = m_selection_actions_group.addAction( QET::Icons::ObjectRotateRight, tr("Pivoter")							  );
	rotate_texts      = m_selection_actions_group.addAction( QET::Icons::ObjectRotateRight, tr("Orienter les textes")				  );
	find_element      = m_selection_actions_group.addAction(                                tr("Retrouver dans le panel")			  );
	edit_selection    = m_selection_actions_group.addAction( QET::Icons::ElementEdit,       tr("\311diter l'item s\351lectionn\351")  );
	selection_prop    = m_selection_actions_group.addAction( QET::Icons::DialogInformation, tr("Propri\351t\351s de la s\351lection") );

#ifndef Q_WS_MAC
	delete_selection -> setShortcut( QKeySequence( Qt::Key_Delete)    );
#else
	delete_selection -> setShortcut( QKeySequence( tr("Backspace")  ) );
#endif

	rotate_selection -> setShortcut( QKeySequence( tr("Space")		) );
	rotate_texts     -> setShortcut( QKeySequence( tr("Ctrl+Space") ) );
	selection_prop   -> setShortcut( QKeySequence( tr("Ctrl+J")		) );
	conductor_reset  -> setShortcut( QKeySequence( tr("Ctrl+K")		) );
	infos_diagram    -> setShortcut( QKeySequence( tr("Ctrl+L")		) );
	edit_selection	 -> setShortcut( QKeySequence( tr("Ctrl+E")		) );

	delete_selection -> setStatusTip( tr("Enl\350ve les \351l\351ments s\351lectionn\351s du sch\351ma", "status bar tip") );
	rotate_selection -> setStatusTip( tr("Pivote les \351l\351ments et textes s\351lectionn\351s", "status bar tip")       );
	rotate_texts     -> setStatusTip( tr("Pivote les textes s\351lectionn\351s \340 un angle pr\351cis", "status bar tip") );
	find_element     -> setStatusTip( tr("Retrouve l'\351l\351ment s\351lectionn\351 dans le panel", "status bar tip")	   );
	selection_prop   -> setStatusTip( tr("\311dite les propri\351t\351s des objets s\351lectionn\351", "status bar tip")   );

	connect(delete_selection,   SIGNAL( triggered() ), this,       SLOT( slot_delete()				  ) );
	connect(rotate_selection,   SIGNAL( triggered() ), this,       SLOT( slot_rotate()				  ) );
	connect(rotate_texts,       SIGNAL( triggered() ), this,       SLOT( slot_rotateTexts()			  ) );
	connect(find_element,       SIGNAL( triggered() ), this,       SLOT( findSelectedElementInPanel() ) );
	connect(edit_selection,     SIGNAL( triggered() ), this,       SLOT( slot_editSelection()		  ) );
	connect(selection_prop,     SIGNAL( triggered() ), this,       SLOT( editSelectionProperties()	  ) );

	///Select Action///
	QAction *select_all     = m_select_actions_group.addAction( QET::Icons::EditSelectAll, tr("Tout s\351lectionner")	   );
	QAction *select_nothing = m_select_actions_group.addAction(                            tr("D\351s\351lectionner tout") );
	QAction *select_invert  = m_select_actions_group.addAction(                            tr("Inverser la s\351lection")  );

	select_all     -> setShortcut( QKeySequence::SelectAll			  );
	select_nothing -> setShortcut( QKeySequence( tr("Ctrl+Shift+A") ) );
	select_invert  -> setShortcut( QKeySequence( tr("Ctrl+I")		) );

	select_all     -> setStatusTip( tr("S\351lectionne tous les \351l\351ments du sch\351ma", "status bar tip")																	  );
	select_nothing -> setStatusTip( tr("D\351s\351lectionne tous les \351l\351ments du sch\351ma", "status bar tip")															  );
	select_invert  -> setStatusTip( tr("D\351s\351lectionne les \351l\351ments s\351lectionn\351s et s\351lectionne les \351l\351ments non s\351lectionn\351s", "status bar tip") );

	connect(select_all,     SIGNAL( triggered() ), this, SLOT( slot_selectAll()		) );
	connect(select_nothing, SIGNAL( triggered() ), this, SLOT( slot_selectNothing() ) );
	connect(select_invert,  SIGNAL( triggered() ), this, SLOT( slot_selectInvert()	) );

	///Zoom actions///
	QAction *zoom_in      = m_zoom_actions_group.addAction( QET::Icons::ZoomIn,       tr("Zoom avant")			);
	QAction *zoom_out     = m_zoom_actions_group.addAction( QET::Icons::ZoomOut,      tr("Zoom arri\350re")		);
	QAction *zoom_content = m_zoom_actions_group.addAction( QET::Icons::ZoomDraw,     tr("Zoom sur le contenu") );
	QAction *zoom_fit     = m_zoom_actions_group.addAction( QET::Icons::ZoomFitBest,  tr("Zoom adapt\351")		);
	QAction *zoom_reset   = m_zoom_actions_group.addAction( QET::Icons::ZoomOriginal, tr("Pas de zoom")			);
	m_zoom_action_toolBar << zoom_content << zoom_fit << zoom_reset;

	zoom_in      -> setShortcut( QKeySequence::ZoomIn         );
	zoom_out     -> setShortcut( QKeySequence::ZoomOut        );
	zoom_content -> setShortcut( QKeySequence( tr("Ctrl+8") ) );
	zoom_fit     -> setShortcut( QKeySequence( tr("Ctrl+9") ) );
	zoom_reset   -> setShortcut( QKeySequence( tr("Ctrl+0") ) );

	zoom_in      -> setStatusTip(tr("Agrandit le sch\351ma", "status bar tip"));
	zoom_out     -> setStatusTip(tr("R\351tr\351cit le sch\351ma", "status bar tip"));
	zoom_content -> setStatusTip(tr("Adapte le zoom de fa\347on \340 afficher tout le contenu ind\351pendamment du cadre"));
	zoom_fit     -> setStatusTip(tr("Adapte la taille du sch\351ma afin qu'il soit enti\350rement visible", "status bar tip"));
	zoom_reset   -> setStatusTip(tr("Restaure le zoom par d\351faut", "status bar tip"));

	connect( zoom_in,      SIGNAL( triggered() ), this,       SLOT( slot_zoomIn()      ) );
	connect( zoom_out,     SIGNAL( triggered() ), this,       SLOT( slot_zoomOut()     ) );
	connect( zoom_content, SIGNAL( triggered() ), this,       SLOT( slot_zoomContent() ) );
	connect( zoom_fit,     SIGNAL( triggered() ), this,       SLOT( slot_zoomFit()     ) );
	connect( zoom_reset,   SIGNAL( triggered() ), this,       SLOT( slot_zoomReset()   ) );

	///Adding action (add text, image, shape...)///
	m_add_item_actions_group.setExclusive(true);

	QAction *add_text      = m_add_item_actions_group.addAction( QET::Icons::PartTextField, tr("Ajouter un champ de texte")		);
	QAction *add_image	   = m_add_item_actions_group.addAction( QET::Icons::adding_image,  tr("Ajouter une image")				);
	QAction *add_line	   = m_add_item_actions_group.addAction( QET::Icons::PartLine,      tr("Ajouter une liaison mecanique") );
	QAction *add_rectangle = m_add_item_actions_group.addAction( QET::Icons::PartRectangle, tr("Ajouter une zone rectangle")	);
	QAction *add_ellipse   = m_add_item_actions_group.addAction( QET::Icons::PartEllipse,   tr("Ajouter une zone ellipse")		);
	QAction *add_polyline  = m_add_item_actions_group.addAction( QET::Icons::PartPolygon,	tr("Ajouter une zone polyligne")    );

	connect( add_text,      SIGNAL( triggered() ), this, SLOT( slot_addText()      ) );
	connect( add_image,     SIGNAL( triggered() ), this, SLOT( slot_addImage()     ) );
	connect( add_line,      SIGNAL( triggered() ), this, SLOT( slot_addLine()      ) );
	connect( add_rectangle, SIGNAL( triggered() ), this, SLOT( slot_addRectangle() ) );
	connect( add_ellipse,   SIGNAL( triggered() ), this, SLOT( slot_addEllipse()   ) );
	connect( add_polyline,  SIGNAL( triggered() ), this, SLOT( slot_addPolyline()  ) );

	foreach(QAction *action, m_add_item_actions_group.actions()) action->setCheckable(true);

	///Keyboard shortcut
	export_diagram    -> setShortcut(QKeySequence(tr("Ctrl+Shift+X")));
	print             -> setShortcut(QKeySequence(QKeySequence::Print));
	quit_editor       -> setShortcut(QKeySequence(tr("Ctrl+Q")));
	undo              -> setShortcut(QKeySequence::Undo);
	redo              -> setShortcut(QKeySequence::Redo);
	cut               -> setShortcut(QKeySequence::Cut);
	copy              -> setShortcut(QKeySequence::Copy);
	paste             -> setShortcut(QKeySequence::Paste);
	
	prj_add_diagram   -> setShortcut(QKeySequence(tr("Ctrl+T")));
	
	next_window       -> setShortcut(QKeySequence::NextChild);
	prev_window       -> setShortcut(QKeySequence::PreviousChild);
	
	// affichage dans la barre de statut
	export_diagram    -> setStatusTip(tr("Exporte le sch\351ma courant dans un autre format", "status bar tip"));
	print             -> setStatusTip(tr("Imprime le sch\351ma courant", "status bar tip"));
	quit_editor       -> setStatusTip(tr("Ferme l'application QElectroTech", "status bar tip"));
	undo              -> setStatusTip(tr("Annule l'action pr\351c\351dente", "status bar tip"));
	redo              -> setStatusTip(tr("Restaure l'action annul\351e", "status bar tip"));
	cut               -> setStatusTip(tr("Transf\350re les \351l\351ments s\351lectionn\351s dans le presse-papier", "status bar tip"));
	copy              -> setStatusTip(tr("Copie les \351l\351ments s\351lectionn\351s dans le presse-papier", "status bar tip"));
	paste             -> setStatusTip(tr("Place les \351l\351ments du presse-papier sur le sch\351ma", "status bar tip"));
	conductor_reset   -> setStatusTip(tr("Recalcule les chemins des conducteurs sans tenir compte des modifications", "status bar tip"));
	infos_diagram     -> setStatusTip(tr("\311dite les informations affich\351es par le cartouche", "status bar tip"));
	
	windowed_view_mode -> setStatusTip(tr("Pr\351sente les diff\351rents projets ouverts dans des sous-fen\352tres", "status bar tip"));
	tabbed_view_mode   -> setStatusTip(tr("Pr\351sente les diff\351rents projets ouverts des onglets", "status bar tip"));
	
	mode_selection    -> setStatusTip(tr("Permet de s\351lectionner les \351l\351ments", "status bar tip"));
	mode_visualise    -> setStatusTip(tr("Permet de visualiser le sch\351ma sans pouvoir le modifier", "status bar tip"));
	
	tile_window       -> setStatusTip(tr("Dispose les fen\352tres en mosa\357que", "status bar tip"));
	cascade_window    -> setStatusTip(tr("Dispose les fen\352tres en cascade", "status bar tip"));
	next_window       -> setStatusTip(tr("Active le projet suivant", "status bar tip"));
	prev_window       -> setStatusTip(tr("Active le projet pr\351c\351dent", "status bar tip"));
	
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
	connect(prj_diagramNum,     SIGNAL(triggered()), this,       SLOT(diagramNumProject())         );
	connect(prj_diagramList,    SIGNAL(triggered()), this,       SLOT(addDiagramFolioListToProject()));
	connect(prj_nomenclature,   SIGNAL(triggered()), this,       SLOT(nomenclatureProject())       );
	connect(print,              SIGNAL(triggered()), this,       SLOT(printDialog())               );
	connect(export_diagram,     SIGNAL(triggered()), this,       SLOT(exportDialog())              );
	connect(cut,                SIGNAL(triggered()), this,       SLOT(slot_cut())                  );
	connect(copy,               SIGNAL(triggered()), this,       SLOT(slot_copy())                 );
	connect(paste,              SIGNAL(triggered()), this,       SLOT(slot_paste())                );
	connect(tile_window,        SIGNAL(triggered()), &workspace, SLOT(tileSubWindows())            );
	connect(cascade_window,     SIGNAL(triggered()), &workspace, SLOT(cascadeSubWindows())         );
	connect(next_window,        SIGNAL(triggered()), &workspace, SLOT(activateNextSubWindow())     );
	connect(prev_window,        SIGNAL(triggered()), &workspace, SLOT(activatePreviousSubWindow()) );
	connect(conductor_reset,    SIGNAL(triggered()), this,       SLOT(slot_resetConductors())      );
	connect(infos_diagram,      SIGNAL(triggered()), this,       SLOT(editCurrentDiagramProperties()));
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
	Mise en place des menus
*/
void QETDiagramEditor::menus() {
	
	QMenu *menu_fichier   = new QMenu(tr("&Fichier"));
	QMenu *menu_edition   = new QMenu(tr("&\311dition"));
	QMenu *menu_project   = new QMenu(tr("&Projet"));
	QMenu *menu_affichage = new QMenu(tr("Afficha&ge"));
	//QMenu *menu_outils    = new QMenu(tr("O&utils"));
	windows_menu          = new QMenu(tr("Fe&n\352tres"));
	
	insertMenu(settings_menu_, menu_fichier);
	insertMenu(settings_menu_, menu_edition);
	insertMenu(settings_menu_, menu_project);
	insertMenu(settings_menu_, menu_affichage);
	insertMenu(help_menu_, windows_menu);
	
	// File menu
	QMenu *recentfile = menu_fichier -> addMenu(QET::Icons::DocumentOpenRecent, tr("&R\351cemment ouverts"));
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
	menu_project -> addAction(prj_diagramNum);
	menu_project -> addAction(prj_diagramList);
	menu_project -> addAction(prj_nomenclature);
	
	main_bar    -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils principale"));
	view_bar    -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils Affichage"));
	diagram_bar -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils Sch\351ma"));
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
	menu_affichage -> addActions(m_zoom_actions_group.actions());

	// menu Fenetres
	slot_updateWindowsMenu();
}

/**
	Mise en place de la barre d'outils
*/
void QETDiagramEditor::toolbar() {
	main_bar = new QToolBar(tr("Outils"), this);
	main_bar -> setObjectName("toolbar");
	
	view_bar = new QToolBar(tr("Affichage"), this);
	view_bar -> setObjectName("display");
	
	diagram_bar = new QToolBar(tr("Sch\351ma"), this);
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
	main_bar -> addAction(selection_prop);
	
	// Modes selection / visualisation et zoom
	view_bar -> addAction(mode_selection);
	view_bar -> addAction(mode_visualise);
	view_bar -> addSeparator();
	view_bar -> addActions(m_zoom_action_toolBar);
	
	diagram_bar -> addAction(infos_diagram);
	diagram_bar -> addAction(conductor_reset);

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
	Methode enregistrant le schema dans le dernier nom de fichier connu.
	@return true si l'enregistrement a reussi, false sinon
*/
void QETDiagramEditor::save() {
	if (ProjectView *project_view = currentProject()) {
		QETResult save_file = project_view -> save();
		if (save_file.isOk()) {
			QETApp::projectsRecentFiles() -> fileWasOpened(project_view -> project() -> filePath());
		} else {
			showError(save_file);
		}
	}
}

/**
	Cette methode demande un nom de fichier a l'utilisateur pour enregistrer le schema
	@return true si l'enregistrement a reussi, false sinon
*/
void QETDiagramEditor::saveAs() {
	if (ProjectView *project_view = currentProject()) {
		QETResult save_file = project_view -> saveAs();
		if (save_file.isOk()) {
			QETApp::projectsRecentFiles() -> fileWasOpened(project_view -> project() -> filePath());
		} else {
			showError(save_file);
		}
	}
}

/**
	Methode enregistrant tous les schemas.
	@return true si l'enregistrement a reussi, false sinon
*/
void QETDiagramEditor::saveCurrentDiagram() {
	if (ProjectView *project_view = currentProject()) {
		QETResult save_file = project_view -> saveCurrentDiagram();
		if (save_file.isOk()) {
			QETApp::projectsRecentFiles() -> fileWasOpened(project_view -> project() -> filePath());
		} else {
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
	new_project -> setDefaultBorderProperties	  (defaultBorderProperties());
	new_project -> setDefaultConductorProperties  (defaultConductorProperties());
	new_project -> setDefaultTitleBlockProperties (defaultTitleBlockProperties());
	new_project -> setDefaultReportProperties	  (defaultReportProperties());
	new_project -> setDefaultXRefProperties		  (defaultXRefProperties());
	
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
		tr("Sch\351mas QElectroTech (*.qet);;Fichiers XML (*.xml);;Tous les fichiers (*)")
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
		if (QMdiSubWindow *sub_window = subWindowForWidget(project_view)) {
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
			QET::MessageBox::critical(
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
			QET::MessageBox::critical(
				this,
				tr("Impossible d'ouvrir le fichier", "message box title"),
				tr("Il semblerait que le fichier que vous essayez d'ouvrir ne "
				"soit pas accessible en lecture. Il est donc impossible de "
				"l'ouvrir. Veuillez v\351rifier les permissions du fichier.")
			);
		}
		return(false);
	}
	
	// gere le fait que le fichier puisse etre en lecture seule
	if (!filepath_info.isWritable()) {
		if (interactive) {
			QET::MessageBox::warning(
				this,
				tr("Ouverture du projet en lecture seule", "message box title"),
				tr("Il semblerait que le projet que vous essayez d'ouvrir ne "
				"soit pas accessible en \351criture. Il sera donc ouvert en "
				"lecture seule.")
			);
		}
	}
	
	// cree le projet a partir du fichier
	QETProject *project = new QETProject(filepath);
	if (project -> state() != QETProject::Ok) {
		if (interactive && project -> state() != QETProject::FileOpenDiscard) {
			QET::MessageBox::warning(
				this,
				tr("\311chec de l'ouverture du projet", "message box title"),
				QString(
					tr(
						"Il semblerait que le fichier %1 ne soit pas un fichier"
						" projet QElectroTech. Il ne peut donc \352tre ouvert.",
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

/**
	Effectue l'action "zoom avant" sur le diagram en cours
*/
void QETDiagramEditor::slot_zoomIn() {
	if(currentDiagram()) currentDiagram() -> zoomIn();
}

/**
	Effectue l'action "zoom arriere" sur le schema en cours
*/
void QETDiagramEditor::slot_zoomOut() {
	if(currentDiagram()) currentDiagram() -> zoomOut();
}

/**
	Effectue l'action "zoom arriere" sur le diagram en cours
*/
void QETDiagramEditor::slot_zoomFit() {
	if(currentDiagram()) currentDiagram() -> zoomFit();
}

/**
	Call the "zoom content" action for the current diagram.
*/
void QETDiagramEditor::slot_zoomContent() {
	if(currentDiagram()) currentDiagram() -> zoomContent();
}

/**
	Effectue l'action "zoom par defaut" sur le schema en cours
*/
void QETDiagramEditor::slot_zoomReset() {
	if(currentDiagram()) currentDiagram() -> zoomReset();
}

/**
	Effectue l'action "selectionner tout" sur le schema en cours
*/
void QETDiagramEditor::slot_selectAll() {
	if(currentDiagram()) currentDiagram() -> selectAll();
}

/**
	Effectue l'action "deselectionenr tout" sur le schema en cours
*/
void QETDiagramEditor::slot_selectNothing() {
	if(currentDiagram()) currentDiagram() -> selectNothing();
}

/**
	Effectue l'action "inverser la selection" sur le schema en cours
*/
void QETDiagramEditor::slot_selectInvert() {
	if(currentDiagram()) currentDiagram() -> selectInvert();
}

/**
	Effectue l'action "supprimer" sur le schema en cours
*/
void QETDiagramEditor::slot_delete() {
	if(currentDiagram()) currentDiagram() -> deleteSelection();
}

/**
	Effectue l'action "pivoter" sur le schema en cours
*/
void QETDiagramEditor::slot_rotate() {
	if(currentDiagram()) currentDiagram() -> rotateSelection();
}

/**
	Effectue l'action "Orienter les textes selectionnes" sur le schema en cours
*/
void QETDiagramEditor::slot_rotateTexts() {
	if (currentDiagram()) currentDiagram() -> rotateTexts();
}

/**
	Effectue l'action "mode selection" sur le schema en cours
*/
void QETDiagramEditor::slot_setSelectionMode() {
	if(currentDiagram()) currentDiagram() -> setSelectionMode();
}

/**
	Effectue l'action "mode visualisation" sur le schema en cours
*/
void QETDiagramEditor::slot_setVisualisationMode() {
	if(currentDiagram()) currentDiagram() -> setVisualisationMode();
}

/**
	gere les actions
*/
void QETDiagramEditor::slot_updateActions() {
	DiagramView *dv = currentDiagram();
	ProjectView *pv = currentProject();
	bool opened_project = pv;
	bool opened_diagram = dv;
	bool editable_project = (pv && !pv -> project() -> isReadOnly());
	bool editable_diagram = (dv && !dv -> diagram() -> isReadOnly());
	
	// actions ayant juste besoin d'un document ouvert
	close_file        -> setEnabled(opened_project);
	save_file         -> setEnabled(editable_project);
	save_file_as      -> setEnabled(opened_project);
	save_cur_diagram  -> setEnabled(editable_diagram);
	prj_edit_prop     -> setEnabled(opened_project);
	prj_add_diagram   -> setEnabled(editable_project);
	//prj_add_diagram_foliolist   -> setEnabled(editable_project);
	prj_del_diagram   -> setEnabled(editable_project);
	prj_clean         -> setEnabled(editable_project);
	prj_diagramNum    -> setEnabled(editable_project);
	prj_diagramList   -> setEnabled(opened_project);
	prj_nomenclature  -> setEnabled(editable_project);
	export_diagram    -> setEnabled(opened_diagram);
	print             -> setEnabled(opened_diagram);
	infos_diagram     -> setEnabled(opened_diagram);
	prj_nomenclature  -> setEnabled(editable_project);
	m_zoom_actions_group.setEnabled(opened_diagram);
	m_select_actions_group.setEnabled(opened_diagram);
	m_add_item_actions_group.setEnabled(editable_diagram);
	m_row_column_actions_group.setEnabled(editable_diagram);

	
	slot_updateModeActions();
	slot_updatePasteAction();
	slot_updateComplexActions();
}

/**
 * @brief QETDiagramEditor::slot_updateUndoStack
 * Update the undo stack view
 */
void QETDiagramEditor::slot_updateUndoStack() {
	ProjectView *pv = currentProject();
	if (pv && can_update_actions) {
		undo_group.setActiveStack(pv->project()->undoStack());
		undo -> setEnabled(undo_group.canUndo());
		redo -> setEnabled(undo_group.canRedo());
	}
}

/**
	gere les actions ayant des besoins precis pour etre active ou non
	Cette methode ne fait rien si aucun document n'est ouvert
*/
void QETDiagramEditor::slot_updateComplexActions() {
	DiagramView *dv = currentDiagram();
	bool editable_diagram = (dv && !dv -> diagram() -> isReadOnly());
	
	// nombre de conducteurs selectionnes
	int selected_conductors_count = dv ? dv -> diagram() -> selectedConductors().count() : 0;
	conductor_reset  -> setEnabled(editable_diagram && selected_conductors_count);
	
	// number of selected elements
	int selected_elements_count = dv ? dv -> diagram() -> selectedContent().count(DiagramContent::Elements) : 0;
	find_element -> setEnabled(selected_elements_count == 1);
	
	// actions ayant aussi besoin d'items (elements, conducteurs, textes, ...) selectionnes
	bool copiable_items  = dv ? (dv -> hasCopiableItems()) : false;
	bool deletable_items = dv ? (dv -> hasDeletableItems()) : false;
	cut              -> setEnabled(editable_diagram && copiable_items);
	copy             -> setEnabled(copiable_items);
	delete_selection -> setEnabled(editable_diagram && deletable_items);
	rotate_selection -> setEnabled(editable_diagram && dv -> diagram() -> canRotateSelection());
	selection_prop   -> setEnabled(deletable_items);
	prj_diagramNum   -> setEnabled(editable_diagram);

	// actions ayant besoin de textes selectionnes
	int selected_texts = dv ? (dv -> diagram() -> selectedTexts().count()) : 0;
	int selected_conductor_texts = dv ? (dv -> diagram() -> selectedConductorTexts().count()) : 0;
	rotate_texts -> setEnabled(editable_diagram && selected_texts);

	// actions need only one editable item
	int selected_image = dv ? dv -> diagram() -> selectedContent().count(DiagramContent::Images) : 0;

	int selected_shape = dv ? dv -> diagram() -> selectedContent().count(DiagramContent::Shapes) : 0;
	int selected_editable = selected_elements_count + (selected_texts - selected_conductor_texts) + selected_image + selected_shape;

	if (selected_editable == 1) {
		edit_selection -> setEnabled(true);
		//edit element
		if (selected_elements_count) {
			edit_selection -> setText(tr("\311diter l'\351lement", "edit element"));
			edit_selection -> setIcon(QET::Icons::ElementEdit);
		}
		//edit text field
		else if (selected_texts) {
			edit_selection -> setText(tr("\311diter le champ de texte", "edit text field"));
			edit_selection -> setIcon(QET::Icons::EditText);
		}
		//edit image
		else if (selected_image) {
			edit_selection -> setText(tr("\311diter l'image", "edit image"));
			edit_selection -> setIcon(QET::Icons::resize_image);
		}
	}
	//not an editable item
	else {
		edit_selection -> setText(tr("\311diter l'objet s\351lectionn\351", "edit selected item"));
		edit_selection -> setIcon(QET::Icons::ElementEdit);
		edit_selection -> setEnabled(false);
	}
}


/**
	Gere les actions relatives au mode du schema
*/
void QETDiagramEditor::slot_updateModeActions() {
	DiagramView *dv = currentDiagram();
	
	// actions ayant aussi besoin d'un document ouvert et de la connaissance de son mode
	if (!dv) {
		grp_visu_sel -> setEnabled(false);
	} else {
		switch((int)(dv -> dragMode())) {
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
	Ajoute un projet dans l'espace de travail
	@param project_view Le projet a ajouter dans l'espace de travail
*/
void QETDiagramEditor::addProjectView(ProjectView *project_view) {
	if (!project_view) return;
	
	// on maximise la nouvelle fenetre si la fenetre en cours est inexistante ou bien maximisee
	QWidget *current_window = workspace.activeSubWindow();
	bool maximise = ((!current_window) || (current_window -> windowState() & Qt::WindowMaximized));
	
	// ajoute la fenetre
	QMdiSubWindow *sub_window = workspace.addSubWindow(project_view);
	sub_window -> setWindowIcon(project_view -> windowIcon());
	sub_window -> systemMenu() -> clear();
	
	// lie les schemas du projet a l'editeur :
	// quand on change de schemas a l'interieur d'un projet, on met a jour les menus
	connect(project_view, SIGNAL(diagramActivated(DiagramView *)), this, SLOT(slot_updateWindowsMenu()));
	connect(project_view, SIGNAL(diagramActivated(DiagramView *)), this, SLOT(slot_updateActions()));
	foreach(DiagramView *dv, project_view -> diagrams()) {
		diagramWasAdded(dv);
	}
	
	// gere la fermeture du projet
	connect(project_view, SIGNAL(projectClosed(ProjectView*)), this, SLOT(projectWasClosed(ProjectView *)));
	
	// gere l'ajout et le retrait de schema du projet
	connect(project_view, SIGNAL(diagramAdded(DiagramView *)),   this, SLOT(diagramWasAdded(DiagramView *)));
	connect(project_view, SIGNAL(diagramAdded(DiagramView *)),   this, SLOT(slot_updateActions()));
	connect(project_view, SIGNAL(diagramAboutToBeRemoved(DiagramView *)), this, SLOT(diagramIsAboutToBeRemoved(DiagramView *)));
	connect(project_view, SIGNAL(diagramRemoved(DiagramView *)), this, SLOT(diagramWasRemoved(DiagramView *)));
	connect(project_view, SIGNAL(diagramRemoved(DiagramView *)), this, SLOT(slot_updateActions()));
	if (QETProject *project = project_view -> project()) {
		// on met aussi les menus a jour quand un projet passe en lecture seule ou non
		connect(project, SIGNAL(readOnlyChanged(QETProject *, bool)), this, SLOT(slot_updateActions()));
	}
	
	// gere les demandes consistant a retrouver un element dans le panel
	connect(project_view, SIGNAL(findElementRequired(const ElementsLocation &)), this, SLOT(findElementInPanel(const ElementsLocation &)));
	
	// gere les demandes pour l'edition d'un element
	connect(project_view, SIGNAL(editElementRequired(const ElementsLocation &)), this, SLOT(editElementInEditor(const ElementsLocation &)));
	
	// handles requests to edit and/or duplicate an existing title block template
	connect(
		project_view, SIGNAL(editTitleBlockTemplate(const TitleBlockTemplateLocation &, bool)),
		QETApp::instance(), SLOT(openTitleBlockTemplate(TitleBlockTemplateLocation, bool))
	);
	
	// display error messages sent by the project view
	connect(project_view, SIGNAL(errorEncountered(QString)), this, SLOT(showError(const QString &)));
	
	// affiche la fenetre
	if (maximise) project_view -> showMaximized();
	else project_view -> show();

	slot_updateActions();
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
		action -> setStatusTip(QString(tr("Active le projet \253\240%1\240\273")).arg(pv_title));
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
	Ajoute une colonne au schema en cours
*/
void QETDiagramEditor::slot_addColumn() {
	if (DiagramView *dv = currentDiagram()) {
		dv -> addColumn();
	}
}

/**
	Enleve une colonne au schema en cours
*/
void QETDiagramEditor::slot_removeColumn() {
	if (DiagramView *dv = currentDiagram()) {
		dv -> removeColumn();
	}
}

/**
	Allonge le schema en cours en hauteur
*/
void QETDiagramEditor::slot_addRow() {
	if (DiagramView *dv = currentDiagram()) {
		dv -> addRow();
	}
}

/**
	Retrecit le schema en cours en hauteur
*/
void QETDiagramEditor::slot_removeRow() {
	if (DiagramView *dv = currentDiagram()) {
		dv -> removeRow();
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
	Edite les proprietes du conducteur selectionne
*/
void QETDiagramEditor::slot_editConductor() {
	if (DiagramView *dv = currentDiagram()) {
		dv -> editConductor();
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
 * @brief QETDiagramEditor::slot_addText
 * add text to curent diagram
 */
void QETDiagramEditor::slot_addText() {
	if (DiagramView *dv = currentDiagram()) dv -> addText();
}

/**
 * @brief QETDiagramEditor::slot_addImage
 * add image to curent diagram
 */
void QETDiagramEditor::slot_addImage() {
	if (DiagramView *dv = currentDiagram()) dv -> addImage();
}

/**
 * @brief QETDiagramEditor::slot_addLine
 * add line to curent diagram
 */
void QETDiagramEditor::slot_addLine() {
	if (DiagramView *dv = currentDiagram()) dv -> addLine();
}

/**
 * @brief QETDiagramEditor::slot_addRectangle
 * add recatngle to curent diagram
 */
void QETDiagramEditor::slot_addRectangle() {
	if (DiagramView *dv = currentDiagram()) dv -> addRectangle();
}

/**
 * @brief QETDiagramEditor::slot_addEllipse
 * add ellipse to curent diagram
 */
void QETDiagramEditor::slot_addEllipse() {
	if (DiagramView *dv = currentDiagram()) dv -> addEllipse();
}

/**
 * @brief QETDiagramEditor::slot_addPolyline
 * add polyline to current diagram
 */
void QETDiagramEditor::slot_addPolyline() {
	if (DiagramView *dv = currentDiagram()) dv -> addPolyline();
}

/**
 * @brief QETDiagramEditor::slot_editSelection
 * edit the selected item if he can be edited and if only  one item is selected
 */
void QETDiagramEditor::slot_editSelection() {
	if (DiagramView *dv = currentDiagram()) {
		DiagramContent dc = dv -> diagram() -> selectedContent();
		if (dc.count(DiagramContent::SelectedOnly | DiagramContent::All) != 1) return;

		if (dc.count(DiagramContent::Elements)) {
			findSelectedElementInPanel();
			editSelectedElementInEditor();
		}
		else if (dc.count(DiagramContent::TextFields)) dv -> editText();
		else if (dc.count(DiagramContent::Images)) dv -> editImage();
		else if (dc.count(DiagramContent::Shapes)) dv -> editShape();
	}
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

/// Lit les parametres de l'editeur de schemas
void QETDiagramEditor::readSettings() {
	QSettings &settings = QETApp::settings();
	
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

/// Enregistre les parametres de l'editeur de schemas
void QETDiagramEditor::writeSettings() {
	QSettings &settings = QETApp::settings();
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

void QETDiagramEditor::addDiagramFolioListToProject() {
	ProjectView *current_project = currentProject();
	if (current_project && current_project -> project() -> getFolioSheetsQuantity() == 0) {

		// The number of folio sheets depend on the number of diagrams in the project.
		int diagram_qty = current_project -> diagrams().size();
		for (int i = 0; i <= diagram_qty/58; i++)
			current_project -> addNewDiagramFolioList();
	}
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
	Nettoie le projet courant
*/
void QETDiagramEditor::cleanCurrentProject() {
	if (ProjectView *current_project = currentProject()) {
		int clean_count = current_project -> cleanProject();
		if (clean_count) pa -> reloadAndFilter();
	}
}

/**
 * @brief launch dialog for numerate diagram
 */
void QETDiagramEditor::diagramNumProject() {
	DialogAutoNum *dg = new DialogAutoNum(currentDiagram()->diagram(), this);
	dg->setModal(true);
	dg->exec();
	
	delete dg;
}

/**
 * @brief export nomemclature of schema
 */
void QETDiagramEditor::nomenclatureProject() {
	//TODO: Test nomenclature CYRIL F.
	nomenclature *nomencl= new nomenclature(currentProject()->project() ,this);
	nomencl->saveToCSVFile();
	
	delete nomencl;
}

/**
	Supprime le schema courant du projet courant
*/
void QETDiagramEditor::removeDiagramFromProject() {
	if (ProjectView *current_project = currentProject()) {
		if (DiagramView *current_diagram = current_project -> currentDiagram()) {
			can_update_actions = false;
			bool isFolioList = false;

			// if diagram to remove is a "folio list sheet", then set a flag.
			if (DiagramFolioList *ptr = dynamic_cast<DiagramFolioList *>(current_diagram -> diagram()))
				isFolioList = true;

			current_project -> removeDiagram(current_diagram);

			// if the removed diagram was a folio sheet, then delete all the remaining folio sheets also.
			if (isFolioList) {
				foreach (DiagramView *diag, current_project -> diagrams()) {
					if (DiagramFolioList *ptr = dynamic_cast<DiagramFolioList *>(diag -> diagram())) {
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
	Gere l'ajout d'un schema dans un projet
	@param dv DiagramView concerne
*/
void QETDiagramEditor::diagramWasAdded(DiagramView *dv) {
	// quand on change qqc a l'interieur d'un schema, on met a jour les menus
	undo_group.addStack(&(dv -> diagram() -> undoStack()));
	connect(dv,              SIGNAL(selectionChanged()),         this,     SLOT(slot_updateComplexActions()));
	connect(dv,              SIGNAL(modeChanged()),              this,     SLOT(slot_updateModeActions()));
	connect(dv,				 SIGNAL(ImageAddedCanceled(bool)),   this,	   SLOT(addItemFinish()));
	connect(dv,				 SIGNAL(itemAdded()),				 this,	   SLOT(addItemFinish()));
}

/**
	Gere le retrait d'un schema dans un projet avant que le retrait ne soit effectif
	@param dv DiagramView concerne
*/
void QETDiagramEditor::diagramIsAboutToBeRemoved(DiagramView *dv) {
	undo_group.removeStack(&(dv -> diagram() -> undoStack()));
	can_update_actions = false;
}

/**
	Gere le retrait d'un schema dans un projet apres que le retrait soit effectif
	@param dv DiagramView concerne
*/
void QETDiagramEditor::diagramWasRemoved(DiagramView *dv) {
	Q_UNUSED(dv);
	can_update_actions = true;
	slot_updateUndoStack();
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
					tr("Impossible de retrouver cet \351l\351ment dans le panel car il semble \351dit\351 dans une autre fen\352tre"),
					10000
				);
			} else {
				// il devrait etre affiche : on tente de recharger le panel
				statusBar() -> showMessage(
					tr("Impossible de retrouver cet \351l\351ment dans le panel... rechargement du panel..."),
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
			tr("Impossible de retrouver cet \351l\351ment dans le panel"),
			10000
		);
	}
}

/**
	Search the panel for the definition for the selected element in the current
	diagram view.
*/
void QETDiagramEditor::findSelectedElementInPanel() {
	if (CustomElement *selected_element = currentCustomElement()) {
		findElementInPanel(selected_element -> location());
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
	QET::MessageBox::critical(this, tr("Erreur", "message box title"), error);
}

/**
 * @brief QETDiagramEditor::addItemFinish
 * Uncheck all action in m_add_item_actions_group
 */
void QETDiagramEditor::addItemFinish() {
	foreach(QAction *action, m_add_item_actions_group.actions()) action->setChecked(false);
}

/**
	@return Les proprietes par defaut pour le cartouche d'un schema
*/
TitleBlockProperties QETDiagramEditor::defaultTitleBlockProperties() {
	// accede a la configuration de l'application
	QSettings &settings = QETApp::settings();
	
	TitleBlockProperties def;
	// lit le cartouche par defaut dans la configuration
	def.fromSettings(settings, "diagrameditor/default");
	
	return(def);
}

/**
	@return Les dimensions par defaut d'un schema
*/
BorderProperties QETDiagramEditor::defaultBorderProperties() {
	// accede a la configuration de l'application
	QSettings &settings = QETApp::settings();
	
	BorderProperties def;
	// lit les dimensions par defaut dans la configuration
	def.fromSettings(settings, "diagrameditor/default");
	
	return(def);
}

/**
	@return Les proprietes par defaut d'un conducteur
*/
ConductorProperties QETDiagramEditor::defaultConductorProperties() {
	// accede a la configuration de l'application
	QSettings &settings = QETApp::settings();
	
	ConductorProperties def;
	// lit les caracteristiques des conducteurs par defaut dans la configuration
	def.fromSettings(settings, "diagrameditor/defaultconductor");
	
	return(def);
}

/**
	@return Les parametres d'export par defaut pour un schema
*/
ExportProperties QETDiagramEditor::defaultExportProperties() {
	// accede a la configuration de l'application
	QSettings &settings = QETApp::settings();
	
	ExportProperties def;
	// lit les caracteristiques des conducteurs par defaut dans la configuration
	def.fromSettings(settings, "export/default");
	
	return(def);
}

/**
	@return Les parametres d'impression par defaut pour un schema
*/
ExportProperties QETDiagramEditor::defaultPrintProperties() {
	// accede a la configuration de l'application
	QSettings &settings = QETApp::settings();
	
	ExportProperties def;
	// lit les caracteristiques des conducteurs par defaut dans la configuration
	def.fromSettings(settings, "print/default");
	
	return(def);
}

/**
 * @brief QETDiagramEditor::defaultReportProperties
 * @return default folio report properties
 */
QString QETDiagramEditor::defaultReportProperties() {
	QSettings &settings= QETApp::settings();
	return(settings.value("diagrameditor/defaultreportlabel", "%f-%l%c").toString());
}

/**
 * @brief QETDiagramEditor::defaultXRefProperties
 * @return the default setting for Xref
 */
QHash <QString, XRefProperties> QETDiagramEditor::defaultXRefProperties() {
	QHash <QString, XRefProperties> hash;
	QStringList keys;
	keys << "coil" << "protection";
	foreach (QString key, keys) {
		XRefProperties properties;
		QString str("diagrameditor/defaultxref");
		properties.fromSettings(QETApp::settings(), str += key);
		hash.insert(key, properties);
	}
	return hash;
}
