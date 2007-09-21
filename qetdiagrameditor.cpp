#include "qetdiagrameditor.h"
#include "qetapp.h"
#include "diagramview.h"
#include "elementspanelwidget.h"
#include "aboutqet.h"

/**
	constructeur
	@param parent le widget parent de la fenetre principale
 */
QETDiagramEditor::QETDiagramEditor(QWidget *parent) : QMainWindow(parent) {
	
	// cree les dossiers de configuration si necessaire
	QDir config_dir(QETApp::configDir());
	if (!config_dir.exists()) config_dir.mkpath(QETApp::configDir());
	
	QDir custom_elements_dir(QETApp::customElementsDir());
	if (!custom_elements_dir.exists()) custom_elements_dir.mkpath(QETApp::customElementsDir());
	
	// mise en place de l'interface MDI au centre de l'application
	setCentralWidget(&workspace);
	
	// mise en place du signalmapper
	connect(&windowMapper, SIGNAL(mapped(QWidget *)), &workspace, SLOT(setActiveWindow(QWidget *)));
	
	// recupere les arguments passes au programme
	QStringList args = QCoreApplication::arguments();
	
	// recupere les chemins de fichiers parmi les arguments
	QStringList files;
	for (int i = 1 ; i < args.size() ; ++ i) {
		if (QFileInfo(args.at(i)).exists()) files << args.at(i);
	}
	
	// si des chemins de fichiers valides sont passes en arguments
	QList<DiagramView *> diagram_views;
	if (files.size()) {
		// alors on ouvre ces fichiers
		foreach(QString file, files) {
			DiagramView *sv = new DiagramView(this);
			if (sv -> ouvrir(file)) diagram_views << sv;
			else delete sv;
		}
	}
	
	// si aucun schema n'a ete ouvert jusqu'a maintenant, on ouvre un nouveau schema
	if (!diagram_views.size()) diagram_views << new DiagramView(this);
	
	// ajout de tous les DiagramView necessaires
	foreach (DiagramView *sv, diagram_views) addDiagramView(sv);
	
	// titre de la fenetre
	setWindowTitle(tr("QElectroTech"));
	
	// icone de la fenetre
	setWindowIcon(QIcon(":/ico/qet.png"));
	
	// barre de statut de la fenetre
	statusBar() -> showMessage(tr("QElectrotech"));
	
	// ajout du panel d'Appareils en tant que QDockWidget
	qdw_pa = new QDockWidget(tr("Panel d'appareils"), this);
	qdw_pa -> setObjectName("elements panel");
	qdw_pa -> setAllowedAreas(Qt::AllDockWidgetAreas);
	qdw_pa -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	qdw_pa -> setMinimumWidth(160);
	qdw_pa -> setWidget(pa = new ElementsPanelWidget(qdw_pa));
	addDockWidget(Qt::LeftDockWidgetArea, qdw_pa);
	
	// mise en place des actions
	actions();
	
	// mise en place de la barre d'outils
	toolbar();
	
	// mise en place des menus
	menus();
	
	// la fenetre est maximisee par defaut
	setMinimumWidth(500);
	setMinimumHeight(350);
	setWindowState(Qt::WindowMaximized);
	
	// connexions signaux / slots pour une interface sensee
	connect(&workspace,                SIGNAL(windowActivated(QWidget *)), this, SLOT(slot_updateActions()));
	connect(QApplication::clipboard(), SIGNAL(dataChanged()),              this, SLOT(slot_updateActions()));
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
	bool peut_quitter = true;
	if (diagramEnCours()) {
		// sinon demande la permission de fermer chaque schema
		foreach(QWidget *fenetre, workspace.windowList()) {
			if (qobject_cast<DiagramView *>(fenetre)) {
				workspace.setActiveWindow(fenetre);
				if (!fermer()) {
					peut_quitter = false;
					qce -> ignore();
					break;
				}
			}
		}
	}
	if (peut_quitter) {
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
	detruit en meme temps que son parent (ici, la QETDiagramEditor).
*/
void QETDiagramEditor::aPropos() {
	static AboutQET *apqet = new AboutQET(this);
	apqet -> exec();
}

/**
	Mise en place des actions
*/
void QETDiagramEditor::actions() {
	// icones et labels
	nouveau_fichier   = new QAction(QIcon(":/ico/new.png"),        tr("&Nouveau"),                             this);
	ouvrir_fichier    = new QAction(QIcon(":/ico/open.png"),       tr("&Ouvrir"),                              this);
	fermer_fichier    = new QAction(QIcon(":/ico/fileclose.png"),  tr("&Fermer"),                              this);
	enr_fichier       = new QAction(QIcon(":/ico/save.png"),       tr("&Enregistrer"),                         this);
	enr_fichier_sous  = new QAction(QIcon(":/ico/saveas.png"),     tr("Enregistrer sous"),                     this);
	importer          = new QAction(QIcon(":/ico/import.png"),     tr("&Importer"),                            this);
	exporter          = new QAction(QIcon(":/ico/export.png"),     tr("E&xporter"),                            this);
	imprimer          = new QAction(QIcon(":/ico/print.png"),      tr("Imprimer"),                             this);
	quitter_qet       = new QAction(QIcon(":/ico/exit.png"),       tr("&Quitter"),                             this);
	
	annuler           = new QAction(QIcon(":/ico/undo.png"),       tr("Annu&ler"),                             this);
	refaire           = new QAction(QIcon(":/ico/redo.png"),       tr("Re&faire"),                             this);
	couper            = new QAction(QIcon(":/ico/cut.png"),        tr("Co&uper"),                              this);
	copier            = new QAction(QIcon(":/ico/copy.png"),       tr("Cop&ier"),                              this);
	coller            = new QAction(QIcon(":/ico/paste.png"),      tr("C&oller"),                              this);
	sel_tout          = new QAction(                               tr("Tout s\351lectionner"),                 this);
	sel_rien          = new QAction(                               tr("D\351s\351lectionner tout"),            this);
	sel_inverse       = new QAction(                               tr("Inverser la s\351lection"),             this);
	supprimer         = new QAction(QIcon(":/ico/delete.png"),     tr("Supprimer"),                            this);
	pivoter           = new QAction(QIcon(":/ico/pivoter.png"),    tr("Pivoter"),                              this);
	infos_diagram     = new QAction(QIcon(":/ico/info.png"),       tr("Informations sur le sch\351ma"),        this);
	add_column        = new QAction(QIcon(":/ico/add_col.png"),    tr("Ajouter une colonne"),                  this);
	remove_column     = new QAction(QIcon(":/ico/remove_col.png"), tr("Enlever une colonne"),                  this);
	expand_diagram    = new QAction(                               tr("Agrandir le sch\351ma"),                this);
	shrink_diagram    = new QAction(                               tr("R\351tr\351cir le sch\351ma"),          this);
	
	zoom_avant        = new QAction(QIcon(":/ico/viewmag+.png"),   tr("Zoom avant"),                           this);
	zoom_arriere      = new QAction(QIcon(":/ico/viewmag-.png"),   tr("Zoom arri\350re"),                      this);
	zoom_adapte       = new QAction(QIcon(":/ico/viewmagfit.png"), tr("Zoom adapt\351"),                       this);
	zoom_reset        = new QAction(QIcon(":/ico/viewmag.png"),    tr("Pas de zoom"),                          this);
	
	mode_selection    = new QAction(QIcon(":/ico/select.png"),     tr("Mode Selection"),                       this);
	mode_visualise    = new QAction(QIcon(":/ico/move.png"),       tr("Mode Visualisation"),                   this);
	
	entrer_pe         = new QAction(QIcon(":/ico/entrer_fs.png"),  tr("Passer en &mode plein \351cran"),       this);
	sortir_pe         = new QAction(QIcon(":/ico/sortir_fs.png"),  tr("Sortir du &mode plein \351cran"),       this);
	configurer        = new QAction(QIcon(":/ico/configure.png"),  tr("&Configurer QElectroTech"),             this);
	
	f_mosaique        = new QAction(                               tr("&Mosa\357que"),                         this);
	f_cascade         = new QAction(                               tr("&Cascade"),                             this);
	f_reorganise      = new QAction(                               tr("Arranger les fen\352tres r\351duites"), this);
	f_suiv            = new QAction(                               tr("Fen\352tre suivante"),                  this);
	f_prec            = new QAction(                               tr("Fen\352tre pr\351c\351dente"),          this);
	
	a_propos_de_qet   = new QAction(QIcon(":/ico/qet.png"),        tr("\300 &propos de QElectroTech"),         this);
	a_propos_de_qt    = new QAction(QIcon(":/ico/qt.png"),         tr("\300 propos de &Qt"),                   this);
	
	// raccourcis clavier
	nouveau_fichier   -> setShortcut(QKeySequence::New);
	ouvrir_fichier    -> setShortcut(QKeySequence::Open);
	fermer_fichier    -> setShortcut(QKeySequence::Close);
	enr_fichier       -> setShortcut(QKeySequence::Save);
	importer          -> setShortcut(QKeySequence(tr("Ctrl+Shift+I")));
	exporter          -> setShortcut(QKeySequence(tr("Ctrl+Shift+X")));
	imprimer          -> setShortcut(QKeySequence(QKeySequence::Print));
	quitter_qet       -> setShortcut(QKeySequence(tr("Ctrl+Q")));
	
	annuler           -> setShortcut(QKeySequence::Undo);
	refaire           -> setShortcut(QKeySequence::Redo);
	couper            -> setShortcut(QKeySequence::Cut);
	copier            -> setShortcut(QKeySequence::Copy);
	coller            -> setShortcut(QKeySequence::Paste);
	sel_tout          -> setShortcut(QKeySequence::SelectAll);
	sel_rien          -> setShortcut(QKeySequence(tr("Ctrl+Shift+A")));
	sel_inverse       -> setShortcut(QKeySequence(tr("Ctrl+I")));
	supprimer         -> setShortcut(QKeySequence(tr("Ctrl+Suppr")));
	pivoter           -> setShortcut(QKeySequence(tr("Ctrl+R")));
	
	zoom_avant        -> setShortcut(QKeySequence::ZoomIn);
	zoom_arriere      -> setShortcut(QKeySequence::ZoomOut);
	zoom_adapte       -> setShortcut(QKeySequence(tr("Ctrl+9")));
	zoom_reset        -> setShortcut(QKeySequence(tr("Ctrl+0")));
	
	entrer_pe         -> setShortcut(QKeySequence(tr("Ctrl+Shift+F")));
	sortir_pe         -> setShortcut(QKeySequence(tr("Ctrl+Shift+F")));
	
	f_suiv            -> setShortcut(QKeySequence::NextChild);
	f_prec            -> setShortcut(QKeySequence::PreviousChild);
	
	// affichage dans la barre de statut
	nouveau_fichier   -> setStatusTip(tr("Cr\351e un nouveau sch\351ma"));
	ouvrir_fichier    -> setStatusTip(tr("Ouvre un sch\351ma existant"));
	fermer_fichier    -> setStatusTip(tr("Ferme le sch\351ma courant"));
	enr_fichier       -> setStatusTip(tr("Enregistre le sch\351ma courant"));
	enr_fichier_sous  -> setStatusTip(tr("Enregistre le sch\351ma courant avec un autre nom de fichier"));
	importer          -> setStatusTip(tr("Importe un sch\351ma dans le sch\351ma courant"));
	exporter          -> setStatusTip(tr("Exporte le sch\351ma courant dans un autre format"));
	imprimer          -> setStatusTip(tr("Imprime le sch\351ma courant"));
	quitter_qet       -> setStatusTip(tr("Ferme l'application QElectroTech"));
	
	annuler           -> setStatusTip(tr("Annule l'action pr\351c\351dente"));
	refaire           -> setStatusTip(tr("Restaure l'action annul\351e"));
	couper            -> setStatusTip(tr("Transf\350re les \351l\351ments s\351lectionn\351s dans le presse-papier"));
	copier            -> setStatusTip(tr("Copie les \351l\351ments s\351lectionn\351s dans le presse-papier"));
	coller            -> setStatusTip(tr("Place les \351l\351ments du presse-papier sur le sch\351ma"));
	sel_tout          -> setStatusTip(tr("S\351lectionne tous les \351l\351ments du sch\351ma"));
	sel_rien          -> setStatusTip(tr("D\351s\351lectionne tous les \351l\351ments du sch\351ma"));
	sel_inverse       -> setStatusTip(tr("D\351s\351lectionne les \351l\351ments s\351lectionn\351s et s\351lectionne les \351l\351ments non s\351lectionn\351s"));
	supprimer         -> setStatusTip(tr("Enl\350ve les \351l\351ments s\351lectionn\351s du sch\351ma"));
	pivoter           -> setStatusTip(tr("Pivote les \351l\351ments s\351lectionn\351s"));
	infos_diagram     -> setStatusTip(tr("\311dite les informations affich\351es par le cartouche"));
	add_column        -> setStatusTip(tr("Ajoute une colonne au sch\351ma"));
	remove_column     -> setStatusTip(tr("Enl\350ve une colonne au sch\351ma"));
	expand_diagram    -> setStatusTip(tr("Agrandit le sch\351ma en hauteur"));
	shrink_diagram    -> setStatusTip(tr("R\351tr\351cit le sch\351ma en hauteur"));
	
	zoom_avant        -> setStatusTip(tr("Agrandit le sch\351ma"));
	zoom_arriere      -> setStatusTip(tr("R\351tr\351cit le sch\351ma"));
	zoom_adapte       -> setStatusTip(tr("Adapte la taille du sch\351ma afin qu'il soit enti\350rement visible"));
	zoom_reset        -> setStatusTip(tr("Restaure le zoom par d\351faut"));
	
	mode_selection    -> setStatusTip(tr("Permet de s\351lectionner les \351l\351ments"));
	mode_visualise    -> setStatusTip(tr("Permet de visualiser le sch\351ma sans pouvoir le modifier"));
	
	entrer_pe         -> setStatusTip(tr("Affiche QElectroTech en mode plein \351cran"));
	sortir_pe         -> setStatusTip(tr("Affiche QElectroTech en mode fen\352tr\351"));
	configurer        -> setStatusTip(tr("Permet de r\351gler diff\351rents param\350tres de QElectroTech"));
	
	f_mosaique        -> setStatusTip(tr("Dispose les fen\352tres en mosa\357que"));
	f_cascade         -> setStatusTip(tr("Dispose les fen\352tres en cascade"));
	f_reorganise      -> setStatusTip(tr("Aligne les fen\352tres r\351duites"));
	f_suiv            -> setStatusTip(tr("Active la fen\352tre suivante"));
	f_prec            -> setStatusTip(tr("Active la fen\352tre pr\351c\351dente"));
	
	a_propos_de_qet   -> setStatusTip(tr("Affiche des informations sur QElectroTech"));
	a_propos_de_qt    -> setStatusTip(tr("Affiche des informations sur la biblioth\350que Qt"));
	
	// traitements speciaux
	mode_selection    -> setCheckable(true);
	mode_visualise    -> setCheckable(true);
	mode_selection    -> setChecked(true);
	
	QActionGroup *grp_visu_sel = new QActionGroup(this);
	grp_visu_sel -> addAction(mode_selection);
	grp_visu_sel -> addAction(mode_visualise);
	grp_visu_sel -> setExclusive(true);
	
	// connexion a des slots
	connect(quitter_qet,      SIGNAL(triggered()), this,       SLOT(close())                     );
	connect(sel_tout,         SIGNAL(triggered()), this,       SLOT(slot_selectAll())           );
	connect(sel_rien,         SIGNAL(triggered()), this,       SLOT(slot_selectNothing())       );
	connect(sel_inverse,      SIGNAL(triggered()), this,       SLOT(slot_selectInvert())        );
	connect(supprimer,        SIGNAL(triggered()), this,       SLOT(slot_supprimer())           );
	connect(pivoter,          SIGNAL(triggered()), this,       SLOT(slot_pivoter())             );
	connect(entrer_pe,        SIGNAL(triggered()), this,       SLOT(toggleFullScreen())         );
	connect(sortir_pe,        SIGNAL(triggered()), this,       SLOT(toggleFullScreen())         );
	connect(mode_selection,   SIGNAL(triggered()), this,       SLOT(slot_setSelectionMode())    );
	connect(mode_visualise,   SIGNAL(triggered()), this,       SLOT(slot_setVisualisationMode()));
	connect(a_propos_de_qet,  SIGNAL(triggered()), this,       SLOT(aPropos())                  );
	connect(a_propos_de_qt,   SIGNAL(triggered()), qApp,       SLOT(aboutQt())                  );
	connect(zoom_avant,       SIGNAL(triggered()), this,       SLOT(slot_zoomPlus())            );
	connect(zoom_arriere,     SIGNAL(triggered()), this,       SLOT(slot_zoomMoins())           );
	connect(zoom_adapte,      SIGNAL(triggered()), this,       SLOT(slot_zoomFit())             );
	connect(zoom_reset,       SIGNAL(triggered()), this,       SLOT(slot_zoomReset())           );
	connect(imprimer,         SIGNAL(triggered()), this,       SLOT(dialog_print())             );
	connect(exporter,         SIGNAL(triggered()), this,       SLOT(dialog_export())            );
	connect(enr_fichier_sous, SIGNAL(triggered()), this,       SLOT(dialogue_enregistrer_sous()));
	connect(enr_fichier,      SIGNAL(triggered()), this,       SLOT(enregistrer())              );
	connect(nouveau_fichier,  SIGNAL(triggered()), this,       SLOT(nouveau())                  );
	connect(ouvrir_fichier,   SIGNAL(triggered()), this,       SLOT(ouvrir())                   );
	connect(fermer_fichier,   SIGNAL(triggered()), this,       SLOT(fermer())                   );
	connect(couper,           SIGNAL(triggered()), this,       SLOT(slot_couper())              );
	connect(copier,           SIGNAL(triggered()), this,       SLOT(slot_copier())              );
	connect(coller,           SIGNAL(triggered()), this,       SLOT(slot_coller())              );
	connect(f_mosaique,       SIGNAL(triggered()), &workspace, SLOT(tile())                     );
	connect(f_cascade,        SIGNAL(triggered()), &workspace, SLOT(cascade())                  );
	connect(f_reorganise,     SIGNAL(triggered()), &workspace, SLOT(arrangeIcons())             );
	connect(f_suiv,           SIGNAL(triggered()), &workspace, SLOT(activateNextWindow())       );
	connect(f_prec,           SIGNAL(triggered()), &workspace, SLOT(activatePreviousWindow())   );
	connect(infos_diagram,    SIGNAL(triggered()), this,       SLOT(slot_editInfos())           );
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
	QMenu *menu_outils    = menuBar() -> addMenu(tr("O&utils"));
	QMenu *menu_config    = menuBar() -> addMenu(tr("&Configuration"));
	menu_fenetres         = menuBar() -> addMenu(tr("Fe&n\352tres"));
	QMenu *menu_aide      = menuBar() -> addMenu(tr("&Aide"));
	
	// tear off feature rulezz... pas ^^ mais bon...
	menu_fichier   -> setTearOffEnabled(true);
	menu_edition   -> setTearOffEnabled(true);
	menu_affichage -> setTearOffEnabled(true);
	menu_outils    -> setTearOffEnabled(true);
	menu_config    -> setTearOffEnabled(true);
	menu_fenetres  -> setTearOffEnabled(true);
	menu_aide      -> setTearOffEnabled(true);
	
	// menu Fichier
	menu_fichier -> addAction(nouveau_fichier);
	menu_fichier -> addAction(ouvrir_fichier);
	menu_fichier -> addAction(enr_fichier);
	menu_fichier -> addAction(enr_fichier_sous);
	menu_fichier -> addAction(fermer_fichier);
	menu_fichier -> addSeparator();
	menu_fichier -> addAction(importer);
	menu_fichier -> addAction(exporter);
	menu_fichier -> addSeparator();
	menu_fichier -> addAction(imprimer);
	menu_fichier -> addSeparator();
	menu_fichier -> addAction(quitter_qet);
	
	// menu Edition
	menu_edition -> addAction(annuler);
	menu_edition -> addAction(refaire);
	menu_edition -> addSeparator();
	menu_edition -> addAction(couper);
	menu_edition -> addAction(copier);
	menu_edition -> addAction(coller);
	menu_edition -> addSeparator();
	menu_edition -> addAction(sel_tout);
	menu_edition -> addAction(sel_rien);
	menu_edition -> addAction(sel_inverse);
	menu_edition -> addSeparator();
	menu_edition -> addAction(supprimer);
	menu_edition -> addAction(pivoter);
	menu_edition -> addSeparator();
	menu_edition -> addAction(infos_diagram);
	menu_edition -> addAction(add_column);
	menu_edition -> addAction(remove_column);
	menu_edition -> addAction(expand_diagram);
	menu_edition -> addAction(shrink_diagram);
	
	// menu Affichage > Afficher
	QMenu *menu_aff_aff = new QMenu(tr("Afficher"));
	menu_aff_aff -> setTearOffEnabled(true);
	menu_aff_aff -> addAction(barre_outils -> toggleViewAction());
	barre_outils -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils"));
	menu_aff_aff -> addAction(qdw_pa -> toggleViewAction());
	qdw_pa -> toggleViewAction() -> setStatusTip(tr("Affiche ou non le panel d'appareils"));
	
	// menu Affichage
	menu_affichage -> addMenu(menu_aff_aff);
	menu_affichage -> addSeparator();
	menu_affichage -> addAction(zoom_avant);
	menu_affichage -> addAction(zoom_arriere);
	menu_affichage -> addAction(zoom_adapte);
	menu_affichage -> addAction(zoom_reset);
	
	// menu Outils
	menu_outils -> addAction(mode_selection);
	menu_outils -> addAction(mode_visualise);
	
	// menu Configuration
	menu_config -> addAction(entrer_pe);
	menu_config -> addAction(configurer);
	
	// menu Fenetres
	slot_updateMenuFenetres();
	
	// menu Aide
	menu_aide -> addAction(a_propos_de_qet);
	menu_aide -> addAction(a_propos_de_qt);
}

/**
	Mise en place de la barre d'outils
*/
void QETDiagramEditor::toolbar() {
	barre_outils = new QToolBar(tr("Outils"), this);
	barre_outils -> setObjectName("toolbar");
	
	// Modes selection / visualisation
	barre_outils -> addAction(mode_selection);
	barre_outils -> addAction(mode_visualise);
	barre_outils -> addSeparator();
	barre_outils -> addAction(annuler);
	barre_outils -> addAction(refaire);
	barre_outils -> addSeparator();
	barre_outils -> addAction(couper);
	barre_outils -> addAction(copier);
	barre_outils -> addAction(coller);
	barre_outils -> addSeparator();
	barre_outils -> addAction(supprimer);
	barre_outils -> addAction(pivoter);
	barre_outils -> addSeparator();
	barre_outils -> addAction(zoom_avant);
	barre_outils -> addAction(zoom_arriere);
	barre_outils -> addAction(zoom_adapte);
	barre_outils -> addAction(zoom_reset);
	
	// ajout de la barre d'outils a la fenetre principale
	addToolBar(Qt::TopToolBarArea, barre_outils);
}

/**
	Imprime le schema courant
*/
void QETDiagramEditor::dialog_print() {
	DiagramView *sv = diagramEnCours();
	if (!sv) return;
	sv -> dialogPrint();
}

/**
	Gere l'export de schema sous forme d'image
*/
void QETDiagramEditor::dialog_export() {
	DiagramView *sv = diagramEnCours();
	if (!sv) return;
	sv -> dialogExport();
}

/**
	Methode enregistrant le schema dans le dernier nom de fichier connu.
	Si aucun nom de fichier n'est connu, cette methode appelle la methode enregistrer_sous
	@return true si l'enregistrement a reussi, false sinon
*/
bool QETDiagramEditor::enregistrer() {
	if (!diagramEnCours()) return(false);
	return(diagramEnCours() -> enregistrer());
}

/**
	Cette methode demande un nom de fichier a l'utilisateur pour enregistrer le schema
	Si aucun nom n'est entre, elle renvoie faux.
	Si le nom ne se termine pas par l'extension .qet, celle-ci est ajoutee.
	Si l'enregistrement reussit, le nom du fichier est conserve et la fonction renvoie true.
	Sinon, faux est renvoye.
	@return true si l'enregistrement a reussi, false sinon
	@todo detecter le chemin du bureau automatiquement
*/
bool QETDiagramEditor::dialogue_enregistrer_sous() {
	if (!diagramEnCours()) return(false);
	return(diagramEnCours() -> enregistrer_sous());
}

/**
	Cette methode cree un nouveau schema.
	@return true si tout s'est bien passe ; false si vous executez cette fonction dans un univers non cartesien (en fait y'a pas de return(false) :p)
*/
bool QETDiagramEditor::nouveau() {
	addDiagramView(new DiagramView(this));
	return(true);
}

/**
	Cette fonction demande un nom de fichier a ouvrir a l'utilisateur
	@return true si l'ouverture a reussi, false sinon
*/
bool QETDiagramEditor::ouvrir() {
	// demande un nom de fichier a ouvrir a l'utilisateur
	QString nom_fichier = QFileDialog::getOpenFileName(
		this,
		tr("Ouvrir un fichier"),
		QDir::homePath(),
		tr("Sch\351mas QElectroTech (*.qet);;Fichiers XML (*.xml);;Tous les fichiers (*)")
	);
	if (nom_fichier == "") return(false);
	
	// verifie que le fichier n'est pas deja ouvert
	QString chemin_fichier = QFileInfo(nom_fichier).canonicalFilePath();
	foreach (QWidget *fenetre, workspace.windowList()) {
		DiagramView *fenetre_en_cours = qobject_cast<DiagramView *>(fenetre);
		if (QFileInfo(fenetre_en_cours -> nom_fichier).canonicalFilePath() == chemin_fichier) {
			workspace.setActiveWindow(fenetre);
			return(false);
		}
	}
	
	// ouvre le fichier
	DiagramView *sv = new DiagramView(this);
	int code_erreur;
	if (sv -> ouvrir(nom_fichier, &code_erreur)) {
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
	@todo detecter les modifications et ne demander que si besoin est
*/
bool QETDiagramEditor::fermer() {
	DiagramView *sv = diagramEnCours();
	if (!sv) return(false);
	return(sv -> close());
}

/**
	@return Le DiagramView qui a le focus dans l'interface MDI
*/
DiagramView *QETDiagramEditor::diagramEnCours() const {
	return(qobject_cast<DiagramView *>(workspace.activeWindow()));
}

/**
	Effectue l'action "couper" sur le schema en cours
*/
void QETDiagramEditor::slot_couper() {
	if(diagramEnCours()) diagramEnCours() -> couper();
}

/**
	Effectue l'action "copier" sur le diagram en cours
*/
void QETDiagramEditor::slot_copier() {
	if(diagramEnCours()) diagramEnCours() -> copier();
}

/**
	Effectue l'action "coller" sur le schema en cours
*/
void QETDiagramEditor::slot_coller() {
	if(diagramEnCours()) diagramEnCours() -> coller();
}

/**
	Effectue l'action "zoom avant" sur le diagram en cours
*/
void QETDiagramEditor::slot_zoomPlus() {
	if(diagramEnCours()) diagramEnCours() -> zoomPlus();
}

/**
	Effectue l'action "zoom arriere" sur le schema en cours
*/
void QETDiagramEditor::slot_zoomMoins() {
	if(diagramEnCours()) diagramEnCours() -> zoomMoins();
}

/**
	Effectue l'action "zoom arriere" sur le diagram en cours
*/
void QETDiagramEditor::slot_zoomFit() {
	if(diagramEnCours()) diagramEnCours() -> zoomFit();
}

/**
	Effectue l'action "zoom par defaut" sur le schema en cours
*/
void QETDiagramEditor::slot_zoomReset() {
	if(diagramEnCours()) diagramEnCours() -> zoomReset();
}

/**
	Effectue l'action "selectionner tout" sur le schema en cours
*/
void QETDiagramEditor::slot_selectAll() {
	if(diagramEnCours()) diagramEnCours() -> selectAll();
}

/**
	Effectue l'action "deselectionenr tout" sur le schema en cours
*/
void QETDiagramEditor::slot_selectNothing() {
	if(diagramEnCours()) diagramEnCours() -> selectNothing();
}

/**
	Effectue l'action "inverser la selection" sur le schema en cours
*/
void QETDiagramEditor::slot_selectInvert() {
	if(diagramEnCours()) diagramEnCours() -> selectInvert();
}

/**
	Effectue l'action "supprimer" sur le schema en cours
*/
void QETDiagramEditor::slot_supprimer() {
	if(diagramEnCours()) diagramEnCours() -> supprimer();
}

/**
	Effectue l'action "pivoter" sur le schema en cours
*/
void QETDiagramEditor::slot_pivoter() {
	if(diagramEnCours()) diagramEnCours() -> pivoter();
}

/**
	Effectue l'action "mode selection" sur le schema en cours
*/
void QETDiagramEditor::slot_setSelectionMode() {
	if(diagramEnCours()) diagramEnCours() -> setSelectionMode();
}

/**
	Effectue l'action "mode visualisation" sur le schema en cours
*/
void QETDiagramEditor::slot_setVisualisationMode() {
	if(diagramEnCours()) diagramEnCours() -> setVisualisationMode();
}

/**
	gere les actions ayant besoin d'un document ouvert
*/
void QETDiagramEditor::slot_updateActions() {
	DiagramView *sv = diagramEnCours();
	bool document_ouvert = (sv != 0);
	
	// actions ayant juste besoin d'un document ouvert
	fermer_fichier   -> setEnabled(document_ouvert);
	enr_fichier      -> setEnabled(document_ouvert);
	enr_fichier_sous -> setEnabled(document_ouvert);
	importer         -> setEnabled(document_ouvert);
	exporter         -> setEnabled(document_ouvert);
	imprimer         -> setEnabled(document_ouvert);
	sel_tout         -> setEnabled(document_ouvert);
	sel_rien         -> setEnabled(document_ouvert);
	sel_inverse      -> setEnabled(document_ouvert);
	zoom_avant       -> setEnabled(document_ouvert);
	zoom_arriere     -> setEnabled(document_ouvert);
	zoom_adapte      -> setEnabled(document_ouvert);
	zoom_reset       -> setEnabled(document_ouvert);
	infos_diagram    -> setEnabled(document_ouvert);
	add_column       -> setEnabled(document_ouvert);
	remove_column    -> setEnabled(document_ouvert);
	expand_diagram   -> setEnabled(document_ouvert);
	shrink_diagram   -> setEnabled(document_ouvert);
	
	// actions ayant aussi besoin d'un historique des actions
	annuler          -> setEnabled(document_ouvert);
	refaire          -> setEnabled(document_ouvert);
	
	// actions ayant aussi besoin d'elements selectionnes
	bool elements_selectionnes = document_ouvert ? (sv -> hasSelectedItems()) : false;
	couper           -> setEnabled(elements_selectionnes);
	copier           -> setEnabled(elements_selectionnes);
	supprimer        -> setEnabled(elements_selectionnes);
	pivoter          -> setEnabled(elements_selectionnes);
	
	// action ayant aussi besoin d'un presse-papier plein
	bool peut_coller = QApplication::clipboard() -> text() != QString();
	coller           -> setEnabled(document_ouvert && peut_coller);
	
	// actions ayant aussi besoin d'un document ouvert et de la connaissance de son mode
	if (!document_ouvert) {
		mode_selection   -> setEnabled(false);
		mode_visualise   -> setEnabled(false);
	} else {
		switch((int)(sv -> dragMode())) {
			case QGraphicsView::NoDrag:
				mode_selection -> setEnabled(false);
				mode_visualise -> setEnabled(false);
				break;
			case QGraphicsView::ScrollHandDrag:
				mode_selection -> setEnabled(true);
				mode_visualise -> setEnabled(true);
				mode_selection -> setChecked(false);
				mode_visualise -> setChecked(true);
				break;
			case QGraphicsView::RubberBandDrag:
				mode_selection -> setEnabled(true);
				mode_visualise -> setEnabled(true);
				mode_selection -> setChecked(true);
				mode_visualise -> setChecked(false);
				break;
		}
	}
	
	slot_updateMenuFenetres();
}

/**
	Ajoute un schema dans l'espace de travail
	@param sv L'objet DiagramView a ajouter a l'espace de travail
*/
void QETDiagramEditor::addDiagramView(DiagramView *sv) {
	if (!sv) return;
	
	// on maximise la nouvelle fenetre si la fenetre en cours est inexistante ou bien maximisee
	DiagramView *s_v = diagramEnCours();
	bool maximise = ((!s_v) || (s_v -> windowState() & Qt::WindowMaximized));
	
	// ajoute la fenetre
	QWidget *p = workspace.addWindow(sv);
	connect(sv, SIGNAL(selectionChanged()), this, SLOT(slot_updateActions()));
	connect(sv, SIGNAL(modeChanged()),      this, SLOT(slot_updateActions()));
	
	// affiche la fenetre
	if (maximise) p -> showMaximized();
	else p -> show();
}

/**
	met a jour le menu "Fenetres"
*/
void QETDiagramEditor::slot_updateMenuFenetres() {
	// nettoyage du menu
	foreach(QAction *a, menu_fenetres -> actions()) menu_fenetres -> removeAction(a);
	
	// actions de fermeture
	menu_fenetres -> addAction(fermer_fichier);
	//menu_fenetres -> addAction(closeAllAct);
	
	// actions de reorganisation des fenetres
	menu_fenetres -> addSeparator();
	menu_fenetres -> addAction(f_mosaique);
	menu_fenetres -> addAction(f_cascade);
	menu_fenetres -> addAction(f_reorganise);
	
	// actiosn de deplacement entre les fenetres
	menu_fenetres -> addSeparator();
	menu_fenetres -> addAction(f_suiv);
	menu_fenetres -> addAction(f_prec);
	
	// liste des fenetres
	QList<QWidget *> fenetres = workspace.windowList();
	
	f_mosaique    -> setEnabled(!fenetres.isEmpty());
	f_cascade     -> setEnabled(!fenetres.isEmpty());
	f_reorganise  -> setEnabled(!fenetres.isEmpty());
	f_suiv        -> setEnabled(!fenetres.isEmpty());
	f_prec        -> setEnabled(!fenetres.isEmpty());
	
	if (!fenetres.isEmpty()) menu_fenetres -> addSeparator();
	for (int i = 0 ; i < fenetres.size() ; ++ i) {
		DiagramView *sv = qobject_cast<DiagramView *>(fenetres.at(i));
		QString sv_titre = sv -> windowTitle().left(sv -> windowTitle().length() - 3);
		QAction *action  = menu_fenetres -> addAction(sv_titre);
		action -> setStatusTip(tr("Active la fen\352tre ") + sv_titre);
		action -> setCheckable(true);
		action -> setChecked(sv == diagramEnCours());
		connect(action, SIGNAL(triggered()), &windowMapper, SLOT(map()));
		windowMapper.setMapping(action, sv);
	}
}

/**
	Edite les informations du schema en cours
*/
void QETDiagramEditor::slot_editInfos() {
	DiagramView *sv = diagramEnCours();
	if (!sv) return;
	sv -> dialogEditInfos();
}

/**
	Ajoute une colonne au schema en cours
*/
void QETDiagramEditor::slot_addColumn() {
	DiagramView *sv = diagramEnCours();
	if (!sv) return;
	sv -> addColumn();
}

/**
	Enleve une colonne au schema en cours
*/
void QETDiagramEditor::slot_removeColumn() {
	DiagramView *sv = diagramEnCours();
	if (!sv) return;
	sv -> removeColumn();
}

/**
	Allonge le schema en cours en hauteur
*/
void QETDiagramEditor::slot_expand() {
	DiagramView *sv = diagramEnCours();
	if (!sv) return;
	sv -> expand();
}

/**
	Retrecit le schema en cours en hauteur
*/
void QETDiagramEditor::slot_shrink() {
	DiagramView *sv = diagramEnCours();
	if (!sv) return;
	sv -> shrink();
}
