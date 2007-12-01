/*
	Copyright 2006-2007 Xavier Guerrin
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
#include "diagramview.h"
#include "diagram.h"
#include "elementspanelwidget.h"
#include "aboutqet.h"
#include "conductorpropertieswidget.h"
#include "configdialog.h"

/**
	constructeur
	@param files Liste de fichiers a ouvrir
	@param parent le widget parent de la fenetre principale
 */
QETDiagramEditor::QETDiagramEditor(const QStringList &files, QWidget *parent) : QMainWindow(parent), open_dialog_dir(QDir::homePath()) {
	
	// cree les dossiers de configuration si necessaire
	QDir config_dir(QETApp::configDir());
	if (!config_dir.exists()) config_dir.mkpath(QETApp::configDir());
	
	QDir custom_elements_dir(QETApp::customElementsDir());
	if (!custom_elements_dir.exists()) custom_elements_dir.mkpath(QETApp::customElementsDir());
	
	// mise en place de l'interface MDI au centre de l'application
	setCentralWidget(&workspace);
	
	// mise en place du signalmapper
	connect(&windowMapper, SIGNAL(mapped(QWidget *)), &workspace, SLOT(setActiveWindow(QWidget *)));
	
	// titre de la fenetre
	setWindowTitle(tr("QElectroTech"));
	
	// icone de la fenetre
	setWindowIcon(QIcon(":/ico/qet.png"));
	
	// barre de statut de la fenetre
	statusBar() -> showMessage(tr("QElectroTech"));
	
	// ajout du panel d'Appareils en tant que QDockWidget
	qdw_pa = new QDockWidget(tr("Panel d'appareils"), this);
	qdw_pa -> setObjectName("elements panel");
	qdw_pa -> setAllowedAreas(Qt::AllDockWidgetAreas);
	qdw_pa -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	qdw_pa -> setMinimumWidth(160);
	qdw_pa -> setWidget(pa = new ElementsPanelWidget(qdw_pa));
	
	qdw_undo = new QDockWidget(tr("Annulations"));
	qdw_undo -> setObjectName("diagram_undo");
	qdw_undo -> setAllowedAreas(Qt::AllDockWidgetAreas);
	qdw_undo -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	qdw_undo -> setMinimumWidth(160);
	tabifyDockWidget(qdw_pa, qdw_undo);
	QUndoView *undo_view = new QUndoView(&undo_group, this);
	undo_view -> setEmptyLabel(tr("Aucune modification"));
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
	
	// si des chemins de fichiers valides sont passes en arguments
	QList<DiagramView *> diagram_views;
	if (files.size()) {
		// alors on ouvre ces fichiers
		foreach(QString file, files) {
			DiagramView *sv = new DiagramView(this);
			if (sv -> open(file)) diagram_views << sv;
			else delete sv;
		}
	}
	
	// si aucun schema n'a ete ouvert jusqu'a maintenant, on ouvre un nouveau schema
	if (!diagram_views.size()) diagram_views << new DiagramView(this);
	
	// connexions signaux / slots pour une interface sensee
	connect(&workspace,                SIGNAL(windowActivated(QWidget *)), this, SLOT(slot_updateWindowsMenu()));
	connect(&workspace,                SIGNAL(windowActivated(QWidget *)), this, SLOT(slot_updateActions()));
	connect(QApplication::clipboard(), SIGNAL(dataChanged()),              this, SLOT(slot_updatePasteAction()));
	
	// ajout de tous les DiagramView necessaires
	foreach (DiagramView *sv, diagram_views) addDiagramView(sv);
	
	// lecture des parametres
	readSettings();
	
	// affichage
	show();
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
	// quitte directement s'il n'y a aucun schema ouvert
	bool can_quit = true;
	if (currentDiagram()) {
		// sinon demande la permission de fermer chaque schema
		foreach(QWidget *diagram_window, workspace.windowList()) {
			if (qobject_cast<DiagramView *>(diagram_window)) {
				workspace.setActiveWindow(diagram_window);
				if (!closeDiagram()) {
					can_quit = false;
					qce -> ignore();
					break;
				}
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
	Fait passer la fenetre du mode plein ecran au mode normal et vice-versa
*/
void QETDiagramEditor::toggleFullScreen() {
	setWindowState(windowState() ^ Qt::WindowFullScreen);
}

/**
	Dialogue « A propos de QElectroTech »
	Le dialogue en question est cree lors du premier appel de cette fonction.
	En consequence, sa premiere apparition n'est pas immediate. Par la suite,
	le dialogue n'a pas a etre recree et il apparait instantanement. Il est
	detruit en meme temps que son parent (ici, le QETDiagramEditor).
*/
void QETDiagramEditor::aboutQET() {
	static AboutQET *apqet = new AboutQET(this);
	apqet -> exec();
}

/**
	Mise en place des actions
*/
void QETDiagramEditor::actions() {
	// icones et labels
	new_file          = new QAction(QIcon(":/ico/new.png"),        tr("&Nouveau"),                             this);
	open_file         = new QAction(QIcon(":/ico/open.png"),       tr("&Ouvrir"),                              this);
	close_file        = new QAction(QIcon(":/ico/fileclose.png"),  tr("&Fermer"),                              this);
	save_file         = new QAction(QIcon(":/ico/save.png"),       tr("&Enregistrer"),                         this);
	save_file_sous    = new QAction(QIcon(":/ico/saveas.png"),     tr("Enregistrer sous"),                     this);
	import_diagram    = new QAction(QIcon(":/ico/import.png"),     tr("&Importer"),                            this);
	export_diagram    = new QAction(QIcon(":/ico/export.png"),     tr("E&xporter"),                            this);
	print             = new QAction(QIcon(":/ico/print.png"),      tr("Imprimer"),                             this);
	quit_editor       = new QAction(QIcon(":/ico/exit.png"),       tr("&Quitter"),                             this);
	
	undo = undo_group.createUndoAction(this, tr("Annuler"));
	undo -> setIcon(QIcon(":/ico/undo.png"));
	redo = undo_group.createRedoAction(this, tr("Refaire"));
	redo -> setIcon(QIcon(":/ico/redo.png"));
	cut               = new QAction(QIcon(":/ico/cut.png"),        tr("Co&uper"),                              this);
	copy              = new QAction(QIcon(":/ico/copy.png"),       tr("Cop&ier"),                              this);
	paste             = new QAction(QIcon(":/ico/paste.png"),      tr("C&oller"),                              this);
	select_all        = new QAction(                               tr("Tout s\351lectionner"),                 this);
	select_nothing    = new QAction(                               tr("D\351s\351lectionner tout"),            this);
	select_invert     = new QAction(                               tr("Inverser la s\351lection"),             this);
	delete_selection  = new QAction(QIcon(":/ico/delete.png"),     tr("Supprimer"),                            this);
	rotate_selection  = new QAction(QIcon(":/ico/pivoter.png"),    tr("Pivoter"),                              this);
	conductor_prop    = new QAction(QIcon(":/ico/conductor.png"),  tr("Propri\351t\351s du conducteur"),       this);
	conductor_reset   = new QAction(QIcon(":/ico/conductor2.png"), tr("R\351initialiser les conducteurs"),     this);
	conductor_default = new QAction(QIcon(":/ico/conductor3.png"), tr("Conducteurs par d\351faut"),            this);
	infos_diagram     = new QAction(QIcon(":/ico/info.png"),       tr("Propri\351t\351s du sch\351ma"),        this);
	add_text          = new QAction(QIcon(":/ico/textfield.png"),  tr("Ajouter un champ de texte"),            this);
	add_column        = new QAction(QIcon(":/ico/add_col.png"),    tr("Ajouter une colonne"),                  this);
	remove_column     = new QAction(QIcon(":/ico/remove_col.png"), tr("Enlever une colonne"),                  this);
	expand_diagram    = new QAction(                               tr("Agrandir le sch\351ma"),                this);
	shrink_diagram    = new QAction(                               tr("R\351tr\351cir le sch\351ma"),          this);
	
	zoom_in           = new QAction(QIcon(":/ico/viewmag+.png"),   tr("Zoom avant"),                           this);
	zoom_out          = new QAction(QIcon(":/ico/viewmag-.png"),   tr("Zoom arri\350re"),                      this);
	zoom_fit          = new QAction(QIcon(":/ico/viewmagfit.png"), tr("Zoom adapt\351"),                       this);
	zoom_reset        = new QAction(QIcon(":/ico/viewmag.png"),    tr("Pas de zoom"),                          this);
	
	mode_selection    = new QAction(QIcon(":/ico/select.png"),     tr("Mode Selection"),                       this);
	mode_visualise    = new QAction(QIcon(":/ico/move.png"),       tr("Mode Visualisation"),                   this);
	
	fullscreen_on     = new QAction(QIcon(":/ico/entrer_fs.png"),  tr("Passer en &mode plein \351cran"),       this);
	fullscreen_off    = new QAction(QIcon(":/ico/sortir_fs.png"),  tr("Sortir du &mode plein \351cran"),       this);
	configure         = new QAction(QIcon(":/ico/configure.png"),  tr("&Configurer QElectroTech"),             this);
	
	tile_window        = new QAction(                               tr("&Mosa\357que"),                         this);
	cascade_window     = new QAction(                               tr("&Cascade"),                             this);
	arrange_window     = new QAction(                               tr("Arranger les fen\352tres r\351duites"), this);
	next_window        = new QAction(                               tr("Fen\352tre suivante"),                  this);
	prev_window        = new QAction(                               tr("Fen\352tre pr\351c\351dente"),          this);
	
	about_qet         = new QAction(QIcon(":/ico/qet.png"),        tr("\300 &propos de QElectroTech"),         this);
	about_qt          = new QAction(QIcon(":/ico/qt.png"),         tr("\300 propos de &Qt"),                   this);
	
	// raccourcis clavier
	new_file          -> setShortcut(QKeySequence::New);
	open_file         -> setShortcut(QKeySequence::Open);
	close_file        -> setShortcut(QKeySequence::Close);
	save_file         -> setShortcut(QKeySequence::Save);
	import_diagram    -> setShortcut(QKeySequence(tr("Ctrl+Shift+I")));
	export_diagram    -> setShortcut(QKeySequence(tr("Ctrl+Shift+X")));
	print             -> setShortcut(QKeySequence(QKeySequence::Print));
	quit_editor       -> setShortcut(QKeySequence(tr("Ctrl+Q")));
	
	undo              -> setShortcut(QKeySequence::Undo);
	redo              -> setShortcut(QKeySequence::Redo);
	cut               -> setShortcut(QKeySequence::Cut);
	copy              -> setShortcut(QKeySequence::Copy);
	paste             -> setShortcut(QKeySequence::Paste);
	select_all        -> setShortcut(QKeySequence::SelectAll);
	select_nothing    -> setShortcut(QKeySequence(tr("Ctrl+Shift+A")));
	select_invert     -> setShortcut(QKeySequence(tr("Ctrl+I")));
	delete_selection  -> setShortcut(QKeySequence(tr("Suppr")));
	rotate_selection  -> setShortcut(QKeySequence(tr("Ctrl+R")));
	conductor_prop    -> setShortcut(QKeySequence(tr("Ctrl+J")));
	conductor_reset   -> setShortcut(QKeySequence(tr("Ctrl+K")));
	infos_diagram     -> setShortcut(QKeySequence(tr("Ctrl+L")));
	conductor_default -> setShortcut(QKeySequence(tr("Ctrl+D")));
	
	zoom_in           -> setShortcut(QKeySequence::ZoomIn);
	zoom_out          -> setShortcut(QKeySequence::ZoomOut);
	zoom_fit          -> setShortcut(QKeySequence(tr("Ctrl+9")));
	zoom_reset        -> setShortcut(QKeySequence(tr("Ctrl+0")));
	
	fullscreen_on     -> setShortcut(QKeySequence(tr("Ctrl+Shift+F")));
	fullscreen_off    -> setShortcut(QKeySequence(tr("Ctrl+Shift+F")));
	
	next_window       -> setShortcut(QKeySequence::NextChild);
	prev_window       -> setShortcut(QKeySequence::PreviousChild);
	
	// affichage dans la barre de statut
	new_file          -> setStatusTip(tr("Cr\351e un nouveau sch\351ma"));
	open_file         -> setStatusTip(tr("Ouvre un sch\351ma existant"));
	close_file        -> setStatusTip(tr("Ferme le sch\351ma courant"));
	save_file         -> setStatusTip(tr("Enregistre le sch\351ma courant"));
	save_file_sous    -> setStatusTip(tr("Enregistre le sch\351ma courant avec un autre nom de fichier"));
	import_diagram    -> setStatusTip(tr("Importe un sch\351ma dans le sch\351ma courant"));
	export_diagram    -> setStatusTip(tr("Exporte le sch\351ma courant dans un autre format"));
	print             -> setStatusTip(tr("Imprime le sch\351ma courant"));
	quit_editor       -> setStatusTip(tr("Ferme l'application QElectroTech"));
	
	undo              -> setStatusTip(tr("Annule l'action pr\351c\351dente"));
	redo              -> setStatusTip(tr("Restaure l'action annul\351e"));
	cut               -> setStatusTip(tr("Transf\350re les \351l\351ments s\351lectionn\351s dans le presse-papier"));
	copy              -> setStatusTip(tr("Copie les \351l\351ments s\351lectionn\351s dans le presse-papier"));
	paste             -> setStatusTip(tr("Place les \351l\351ments du presse-papier sur le sch\351ma"));
	select_all        -> setStatusTip(tr("S\351lectionne tous les \351l\351ments du sch\351ma"));
	select_nothing    -> setStatusTip(tr("D\351s\351lectionne tous les \351l\351ments du sch\351ma"));
	select_invert     -> setStatusTip(tr("D\351s\351lectionne les \351l\351ments s\351lectionn\351s et s\351lectionne les \351l\351ments non s\351lectionn\351s"));
	delete_selection  -> setStatusTip(tr("Enl\350ve les \351l\351ments s\351lectionn\351s du sch\351ma"));
	rotate_selection  -> setStatusTip(tr("Pivote les \351l\351ments s\351lectionn\351s"));
	conductor_prop    -> setStatusTip(tr("\311dite les propri\351t\351s du conducteur s\351lectionn\351"));
	conductor_reset   -> setStatusTip(tr("Recalcule les chemins des conducteurs sans tenir compte des modifications"));
	conductor_default -> setStatusTip(tr("Sp\351cifie les propri\351t\351s par d\351faut des conducteurs"));
	infos_diagram     -> setStatusTip(tr("\311dite les informations affich\351es par le cartouche"));
	add_column        -> setStatusTip(tr("Ajoute une colonne au sch\351ma"));
	remove_column     -> setStatusTip(tr("Enl\350ve une colonne au sch\351ma"));
	expand_diagram    -> setStatusTip(tr("Agrandit le sch\351ma en hauteur"));
	shrink_diagram    -> setStatusTip(tr("R\351tr\351cit le sch\351ma en hauteur"));
	
	zoom_in           -> setStatusTip(tr("Agrandit le sch\351ma"));
	zoom_out          -> setStatusTip(tr("R\351tr\351cit le sch\351ma"));
	zoom_fit          -> setStatusTip(tr("Adapte la taille du sch\351ma afin qu'il soit enti\350rement visible"));
	zoom_reset        -> setStatusTip(tr("Restaure le zoom par d\351faut"));
	
	mode_selection    -> setStatusTip(tr("Permet de s\351lectionner les \351l\351ments"));
	mode_visualise    -> setStatusTip(tr("Permet de visualiser le sch\351ma sans pouvoir le modifier"));
	
	fullscreen_on     -> setStatusTip(tr("Affiche QElectroTech en mode plein \351cran"));
	fullscreen_off    -> setStatusTip(tr("Affiche QElectroTech en mode fen\352tr\351"));
	configure         -> setStatusTip(tr("Permet de r\351gler diff\351rents param\350tres de QElectroTech"));
	
	tile_window       -> setStatusTip(tr("Dispose les fen\352tres en mosa\357que"));
	cascade_window    -> setStatusTip(tr("Dispose les fen\352tres en cascade"));
	arrange_window    -> setStatusTip(tr("Aligne les fen\352tres r\351duites"));
	next_window       -> setStatusTip(tr("Active la fen\352tre suivante"));
	prev_window       -> setStatusTip(tr("Active la fen\352tre pr\351c\351dente"));
	
	about_qet         -> setStatusTip(tr("Affiche des informations sur QElectroTech"));
	about_qt          -> setStatusTip(tr("Affiche des informations sur la biblioth\350que Qt"));
	
	// traitements speciaux
	add_text          -> setCheckable(true);
	mode_selection    -> setCheckable(true);
	mode_visualise    -> setCheckable(true);
	mode_selection    -> setChecked(true);
	
	grp_visu_sel = new QActionGroup(this);
	grp_visu_sel -> addAction(mode_selection);
	grp_visu_sel -> addAction(mode_visualise);
	grp_visu_sel -> setExclusive(true);
	
	// connexion a des slots
	connect(quit_editor,      SIGNAL(triggered()), this,       SLOT(close())                    );
	connect(select_all,       SIGNAL(triggered()), this,       SLOT(slot_selectAll())           );
	connect(select_nothing,   SIGNAL(triggered()), this,       SLOT(slot_selectNothing())       );
	connect(select_invert,    SIGNAL(triggered()), this,       SLOT(slot_selectInvert())        );
	connect(delete_selection, SIGNAL(triggered()), this,       SLOT(slot_delete())              );
	connect(rotate_selection, SIGNAL(triggered()), this,       SLOT(slot_rotate())              );
	connect(fullscreen_on,    SIGNAL(triggered()), this,       SLOT(toggleFullScreen())         );
	connect(fullscreen_off,   SIGNAL(triggered()), this,       SLOT(toggleFullScreen())         );
	connect(configure,        SIGNAL(triggered()), this,       SLOT(configureQET())             );
	connect(mode_selection,   SIGNAL(triggered()), this,       SLOT(slot_setSelectionMode())    );
	connect(mode_visualise,   SIGNAL(triggered()), this,       SLOT(slot_setVisualisationMode()));
	connect(about_qet,        SIGNAL(triggered()), this,       SLOT(aboutQET())                 );
	connect(about_qt,         SIGNAL(triggered()), qApp,       SLOT(aboutQt())                  );
	connect(zoom_in,          SIGNAL(triggered()), this,       SLOT(slot_zoomIn())              );
	connect(zoom_out,         SIGNAL(triggered()), this,       SLOT(slot_zoomOut())             );
	connect(zoom_fit,         SIGNAL(triggered()), this,       SLOT(slot_zoomFit())             );
	connect(zoom_reset,       SIGNAL(triggered()), this,       SLOT(slot_zoomReset())           );
	connect(print,            SIGNAL(triggered()), this,       SLOT(printDialog())              );
	connect(export_diagram,   SIGNAL(triggered()), this,       SLOT(exportDialog())             );
	connect(save_file_sous,   SIGNAL(triggered()), this,       SLOT(saveAsDialog())             );
	connect(save_file,        SIGNAL(triggered()), this,       SLOT(save())                     );
	connect(new_file,         SIGNAL(triggered()), this,       SLOT(newDiagram())               );
	connect(open_file,        SIGNAL(triggered()), this,       SLOT(openDiagram())              );
	connect(close_file,       SIGNAL(triggered()), this,       SLOT(closeDiagram())             );
	connect(cut,              SIGNAL(triggered()), this,       SLOT(slot_cut())                 );
	connect(copy,             SIGNAL(triggered()), this,       SLOT(slot_copy())                );
	connect(paste,            SIGNAL(triggered()), this,       SLOT(slot_paste())               );
	connect(tile_window,      SIGNAL(triggered()), &workspace, SLOT(tile())                     );
	connect(cascade_window,   SIGNAL(triggered()), &workspace, SLOT(cascade())                  );
	connect(arrange_window,   SIGNAL(triggered()), &workspace, SLOT(arrangeIcons())             );
	connect(next_window,      SIGNAL(triggered()), &workspace, SLOT(activateNextWindow())       );
	connect(prev_window,      SIGNAL(triggered()), &workspace, SLOT(activatePreviousWindow())   );
	connect(conductor_prop,   SIGNAL(triggered()), this,       SLOT(slot_editConductor())       );
	connect(conductor_reset,  SIGNAL(triggered()), this,       SLOT(slot_resetConductors())     );
	connect(conductor_default,SIGNAL(triggered()), this,       SLOT(slot_editDefaultConductors()));
	connect(infos_diagram,    SIGNAL(triggered()), this,       SLOT(slot_editInfos())           );
	connect(add_text,         SIGNAL(triggered()), this,       SLOT(slot_addText())             );
	connect(add_column,       SIGNAL(triggered()), this,       SLOT(slot_addColumn())           );
	connect(remove_column,    SIGNAL(triggered()), this,       SLOT(slot_removeColumn())        );
	connect(expand_diagram,   SIGNAL(triggered()), this,       SLOT(slot_expand())              );
	connect(shrink_diagram,   SIGNAL(triggered()), this,       SLOT(slot_shrink())              );
}

/**
	Mise en place des menus
*/
void QETDiagramEditor::menus() {
	QMenu *menu_fichier   = menuBar() -> addMenu(tr("&Fichier"));
	QMenu *menu_edition   = menuBar() -> addMenu(tr("&\311dition"));
	QMenu *menu_affichage = menuBar() -> addMenu(tr("Afficha&ge"));
	//QMenu *menu_outils    = menuBar() -> addMenu(tr("O&utils"));
	QMenu *menu_config    = menuBar() -> addMenu(tr("&Configuration"));
	windows_menu          = menuBar() -> addMenu(tr("Fe&n\352tres"));
	QMenu *menu_aide      = menuBar() -> addMenu(tr("&Aide"));
	
	// tear off feature rulezz... pas ^^ mais bon...
	menu_fichier   -> setTearOffEnabled(true);
	menu_edition   -> setTearOffEnabled(true);
	menu_affichage -> setTearOffEnabled(true);
	//menu_outils    -> setTearOffEnabled(true);
	menu_config    -> setTearOffEnabled(true);
	windows_menu   -> setTearOffEnabled(true);
	menu_aide      -> setTearOffEnabled(true);
	
	// menu Fichier
	menu_fichier -> addAction(new_file);
	menu_fichier -> addAction(open_file);
	menu_fichier -> addAction(save_file);
	menu_fichier -> addAction(save_file_sous);
	menu_fichier -> addAction(close_file);
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
	menu_edition -> addAction(select_all);
	menu_edition -> addAction(select_nothing);
	menu_edition -> addAction(select_invert);
	menu_edition -> addSeparator();
	menu_edition -> addAction(delete_selection);
	menu_edition -> addAction(rotate_selection);
	menu_edition -> addSeparator();
	menu_edition -> addAction(conductor_prop);
	menu_edition -> addAction(conductor_reset);
	menu_edition -> addAction(conductor_default);
	menu_edition -> addSeparator();
	menu_edition -> addAction(infos_diagram);
	menu_edition -> addAction(add_column);
	menu_edition -> addAction(remove_column);
	menu_edition -> addAction(expand_diagram);
	menu_edition -> addAction(shrink_diagram);
	
	// menu Configurer > Afficher
	QMenu *display_toolbars = createPopupMenu();
	display_toolbars -> setTearOffEnabled(true);
	display_toolbars -> setTitle(tr("Afficher"));
	display_toolbars -> setIcon(QIcon(":/ico/toolbars.png"));
	main_bar    -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils principale"));
	view_bar    -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils Affichage"));
	diagram_bar -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils Sch\351ma"));
	qdw_pa      -> toggleViewAction() -> setStatusTip(tr("Affiche ou non le panel d'appareils"));
	qdw_undo    -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la liste des modifications"));
	
	// menu Affichage
	menu_affichage -> addAction(mode_selection);
	menu_affichage -> addAction(mode_visualise);
	menu_affichage -> addSeparator();
	menu_affichage -> addAction(zoom_in);
	menu_affichage -> addAction(zoom_out);
	menu_affichage -> addAction(zoom_fit);
	menu_affichage -> addAction(zoom_reset);
	
	// menu Configuration
	menu_config -> addMenu(display_toolbars);
	menu_config -> addAction(fullscreen_on);
	menu_config -> addAction(configure);
	
	// menu Fenetres
	slot_updateWindowsMenu();
	
	// menu Aide
	menu_aide -> addAction(about_qet);
	menu_aide -> addAction(about_qt);
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
	
	main_bar -> addAction(new_file);
	main_bar -> addAction(open_file);
	main_bar -> addAction(save_file);
	main_bar -> addAction(save_file_sous);
	main_bar -> addAction(close_file);
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
	view_bar -> addAction(zoom_in);
	view_bar -> addAction(zoom_out);
	view_bar -> addAction(zoom_fit);
	view_bar -> addAction(zoom_reset);
	
	diagram_bar -> addAction(infos_diagram);
	diagram_bar -> addAction(add_text);
	diagram_bar -> addAction(conductor_default);
	diagram_bar -> addAction(conductor_prop);
	diagram_bar -> addAction(conductor_reset);
	
	// ajout de la barre d'outils a la fenetre principale
	addToolBar(Qt::TopToolBarArea, main_bar);
	addToolBar(Qt::TopToolBarArea, view_bar);
	addToolBar(Qt::TopToolBarArea, diagram_bar);
}

/**
	Imprime le schema courant
*/
void QETDiagramEditor::printDialog() {
	DiagramView *sv = currentDiagram();
	if (!sv) return;
	sv -> dialogPrint();
}

/**
	Gere l'export de schema sous forme d'image
*/
void QETDiagramEditor::exportDialog() {
	DiagramView *sv = currentDiagram();
	if (!sv) return;
	sv -> dialogExport();
}

/**
	Methode enregistrant le schema dans le dernier nom de fichier connu.
	@return true si l'enregistrement a reussi, false sinon
*/
bool QETDiagramEditor::save() {
	if (!currentDiagram()) return(false);
	return(currentDiagram() -> save());
}

/**
	Cette methode demande un nom de fichier a l'utilisateur pour enregistrer le schema
	@return true si l'enregistrement a reussi, false sinon
*/
bool QETDiagramEditor::saveAsDialog() {
	if (!currentDiagram()) return(false);
	return(currentDiagram() -> saveAs());
}

/**
	Cette methode cree un nouveau schema.
	@return true si tout s'est bien passe ; false si vous executez cette fonction dans un univers non cartesien (en fait y'a pas de return(false) :p)
*/
bool QETDiagramEditor::newDiagram() {
	addDiagramView(new DiagramView(this));
	return(true);
}

/**
	Cette fonction demande un nom de fichier a ouvrir a l'utilisateur
	@return true si l'ouverture a reussi, false sinon
*/
bool QETDiagramEditor::openDiagram() {
	// demande un nom de fichier a ouvrir a l'utilisateur
	QString nom_fichier = QFileDialog::getOpenFileName(
		this,
		tr("Ouvrir un fichier"),
		open_dialog_dir.absolutePath(),
		tr("Sch\351mas QElectroTech (*.qet);;Fichiers XML (*.xml);;Tous les fichiers (*)")
	);
	if (nom_fichier.isEmpty()) return(false);
	
	open_dialog_dir = QDir(nom_fichier);
	// verifie que le fichier n'est pas deja ouvert
	QString chemin_fichier = QFileInfo(nom_fichier).canonicalFilePath();
	foreach (QWidget *fenetre, workspace.windowList()) {
		DiagramView *fenetre_en_cours = qobject_cast<DiagramView *>(fenetre);
		if (QFileInfo(fenetre_en_cours -> file_name).canonicalFilePath() == chemin_fichier) {
			workspace.setActiveWindow(fenetre);
			return(false);
		}
	}
	
	// ouvre le fichier
	DiagramView *sv = new DiagramView(this);
	int code_erreur;
	if (sv -> open(nom_fichier, &code_erreur)) {
		addDiagramView(sv);
		return(true);
	} else {
		QString message_erreur;
		switch(code_erreur) {
			case 1: message_erreur = tr("Ce fichier n'existe pas."); break;
			case 2: message_erreur = tr("Impossible de lire ce fichier."); break;
			case 3: message_erreur = tr("Ce fichier n'est pas un document XML valide."); break;
			case 4: message_erreur = tr("Une erreur s'est produite lors de l'ouverture du fichier."); break;
		}
		QMessageBox::warning(this, tr("Erreur"), message_erreur);
		delete sv;
		return(false);
	}
}

/**
	Ferme le document courant
	@return true si la fermeture du fichier a reussi, false sinon
*/
bool QETDiagramEditor::closeDiagram() {
	DiagramView *sv = currentDiagram();
	if (!sv) return(false);
	return(sv -> close());
}

/**
	@return Le DiagramView qui a le focus dans l'interface MDI
*/
DiagramView *QETDiagramEditor::currentDiagram() const {
	return(qobject_cast<DiagramView *>(workspace.activeWindow()));
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
	DiagramView *sv = currentDiagram();
	bool opened_document = (sv != 0);
	
	// actions ayant juste besoin d'un document ouvert
	close_file       -> setEnabled(opened_document);
	save_file        -> setEnabled(opened_document);
	save_file_sous   -> setEnabled(opened_document);
	import_diagram   -> setEnabled(opened_document);
	export_diagram   -> setEnabled(opened_document);
	print            -> setEnabled(opened_document);
	select_all       -> setEnabled(opened_document);
	select_nothing   -> setEnabled(opened_document);
	select_invert    -> setEnabled(opened_document);
	zoom_in          -> setEnabled(opened_document);
	zoom_out         -> setEnabled(opened_document);
	zoom_fit         -> setEnabled(opened_document);
	zoom_reset       -> setEnabled(opened_document);
	conductor_default-> setEnabled(opened_document);
	infos_diagram    -> setEnabled(opened_document);
	add_text         -> setEnabled(opened_document);
	add_column       -> setEnabled(opened_document);
	remove_column    -> setEnabled(opened_document);
	expand_diagram   -> setEnabled(opened_document);
	shrink_diagram   -> setEnabled(opened_document);
	
	// affiche les actions correspondant au diagram view en cours
	if (sv) undo_group.setActiveStack(&(sv -> diagram() -> undoStack()));
	else {
		undo -> setEnabled(false);
		redo -> setEnabled(false);
	}
	
	slot_updateModeActions();
	slot_updatePasteAction();
	slot_updateComplexActions();
}

/**
	gere les actions ayant des besoins precis pour etre active ou non
	Cette methode ne fait rien si aucun document n'est ouvert
*/
void QETDiagramEditor::slot_updateComplexActions() {
	DiagramView *dv = currentDiagram();
	bool opened_document = (dv != 0);
	
	// nombre de conducteurs selectionnes
	int selected_conductors_count = opened_document ? dv -> diagram() -> selectedConductors().count() : 0;
	conductor_prop   -> setEnabled(opened_document && selected_conductors_count == 1);
	conductor_reset  -> setEnabled(opened_document && selected_conductors_count);
	
	// actions ayant aussi besoin d'elements selectionnes
	bool selected_elements = opened_document ? (dv -> hasSelectedItems()) : false;
	cut              -> setEnabled(selected_elements);
	copy             -> setEnabled(selected_elements);
	delete_selection -> setEnabled(selected_elements);
	rotate_selection -> setEnabled(selected_elements);
}

/**
	Gere les actions realtives au mode du schema
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
	// pour coller, il faut un schema ouvert et un schema dans le presse-papier
	paste -> setEnabled(currentDiagram() && Diagram::clipboardMayContainDiagram());
}

/**
	Ajoute un schema dans l'espace de travail
	@param dv L'objet DiagramView a ajouter a l'espace de travail
*/
void QETDiagramEditor::addDiagramView(DiagramView *dv) {
	if (!dv) return;
	undo_group.addStack(&(dv -> diagram() -> undoStack()));
	
	// on maximise la nouvelle fenetre si la fenetre en cours est inexistante ou bien maximisee
	DiagramView *d_v = currentDiagram();
	bool maximise = ((!d_v) || (d_v -> windowState() & Qt::WindowMaximized));
	
	// ajoute la fenetre
	QWidget *p = workspace.addWindow(dv);
	connect(dv -> diagram(), SIGNAL(selectionChanged()), this, SLOT(slot_updateComplexActions()));
	connect(dv, SIGNAL(modeChanged()),      this, SLOT(slot_updateModeActions()));
	connect(dv, SIGNAL(textAdded(bool)), add_text, SLOT(setChecked(bool)));
	
	// affiche la fenetre
	if (maximise) p -> showMaximized();
	else p -> show();
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
	windows_menu -> addAction(arrange_window);
	
	// actions de deplacement entre les fenetres
	windows_menu -> addSeparator();
	windows_menu -> addAction(next_window);
	windows_menu -> addAction(prev_window);
	
	// liste des fenetres
	QList<QWidget *> windows = workspace.windowList();
	
	tile_window    -> setEnabled(!windows.isEmpty());
	cascade_window -> setEnabled(!windows.isEmpty());
	arrange_window -> setEnabled(!windows.isEmpty());
	next_window    -> setEnabled(windows.count() > 1);
	prev_window    -> setEnabled(windows.count() > 1);
	
	if (!windows.isEmpty()) windows_menu -> addSeparator();
	QActionGroup *windows_actions = new QActionGroup(this);
	for (int i = 0 ; i < windows.size() ; ++ i) {
		DiagramView *dv = qobject_cast<DiagramView *>(windows.at(i));
		if (!dv) continue;
		QString dv_title = dv -> windowTitle().left(dv -> windowTitle().length() - 3);
		QAction *action  = windows_menu -> addAction(dv_title);
		windows_actions -> addAction(action);
		action -> setStatusTip(tr("Active la fen\352tre ") + dv_title);
		action -> setCheckable(true);
		action -> setChecked(dv == currentDiagram());
		connect(action, SIGNAL(triggered()), &windowMapper, SLOT(map()));
		windowMapper.setMapping(action, dv);
	}
}

/**
	Edite les informations du schema en cours
*/
void QETDiagramEditor::slot_editInfos() {
	DiagramView *sv = currentDiagram();
	if (!sv) return;
	sv -> dialogEditInfos();
}

/**
	Ajoute une colonne au schema en cours
*/
void QETDiagramEditor::slot_addColumn() {
	DiagramView *sv = currentDiagram();
	if (!sv) return;
	sv -> addColumn();
}

/**
	Enleve une colonne au schema en cours
*/
void QETDiagramEditor::slot_removeColumn() {
	DiagramView *sv = currentDiagram();
	if (!sv) return;
	sv -> removeColumn();
}

/**
	Allonge le schema en cours en hauteur
*/
void QETDiagramEditor::slot_expand() {
	DiagramView *sv = currentDiagram();
	if (!sv) return;
	sv -> expand();
}

/**
	Retrecit le schema en cours en hauteur
*/
void QETDiagramEditor::slot_shrink() {
	DiagramView *sv = currentDiagram();
	if (!sv) return;
	sv -> shrink();
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
	Edite les proprietes par defaut des conducteurs
*/
void QETDiagramEditor::slot_editDefaultConductors() {
	if (DiagramView *dv = currentDiagram()) {
		dv -> editDefaultConductorProperties();
	}
}

/**
	Ajoute un texte au schema courant
*/
void QETDiagramEditor::slot_addText() {
	if (DiagramView *dv = currentDiagram()) {
		dv -> addText();
	}
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
}

/// Enregistre les parametres de l'editeur de schemas
void QETDiagramEditor::writeSettings() {
	QSettings &settings = QETApp::settings();
	settings.setValue("diagrameditor/geometry", saveGeometry());
	settings.setValue("diagrameditor/state", saveState());
}

/**
	Permet a l'utilisateur de configurer QET en lancant un dialogue approprie.
	@see ConfigDialog
*/
void QETDiagramEditor::configureQET() {
	ConfigDialog cd;
	cd.exec();
}

/**
	@return Les proprietes par defaut pour le cartouche d'un schema
*/
InsetProperties QETDiagramEditor::defaultInsetProperties() {
	// accede a la configuration de l'application
	QSettings &settings = QETApp::settings();
	
	InsetProperties def;
	def.title    = settings.value("diagrameditor/defaulttitle").toString();
	def.author   = settings.value("diagrameditor/defaultauthor").toString();
	def.filename = settings.value("diagrameditor/defaultfilename").toString();
	def.folio    = settings.value("diagrameditor/defaultfolio").toString();
	
	QString settings_date = settings.value("diagrameditor/defaultdate").toString();
	if (settings_date == "now") {
		def.date = QDate::currentDate();
		def.useDate = InsetProperties::CurrentDate;
	} else if (settings_date.isEmpty() || settings_date == "null") {
		def.date = QDate();
		def.useDate = InsetProperties::UseDateValue;
	} else {
		def.date = QDate::fromString(settings_date, "yyyyMMdd");
		def.useDate = InsetProperties::UseDateValue;
	}
	
	return(def);
}
