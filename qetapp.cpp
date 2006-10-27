#include "qetapp.h"
#include "schemavue.h"
#include "schema.h"
#include "panelappareils.h"
#include "aboutqet.h"

/**
	constructeur
	@param parent le widget parent de la fenetre principale
 */
QETApp::QETApp(QWidget *parent) : QMainWindow(parent) {
	// mise en place de l'interface MDI au centre de l'application
	setCentralWidget(&workspace);
	
	// mise en place du signalmapper
	connect(&windowMapper, SIGNAL(mapped(QWidget *)), &workspace, SLOT(setActiveWindow(QWidget *)));
	
	// recupere les arguments passes au programme
	QStringList args = QCoreApplication::arguments();
	
	// recupere les chemins de fichiers parmi les arguments
	QStringList fichiers;
	for (int i = 1 ; i < args.size() ; ++ i) {
		if (QFileInfo(args.at(i)).exists()) fichiers << args.at(i);
	}
	
	// si des chemins de fichiers valides sont passes en arguments
	QList<SchemaVue *> schema_vues;
	if (fichiers.size()) {
		// alors on ouvre ces fichiers
		foreach(QString fichier, fichiers) {
			SchemaVue *sv = new SchemaVue(this);
			if (sv -> ouvrir(fichier)) schema_vues << sv;
			else delete sv;
		}
	}
	
	// si aucun schema n'a ete ouvert jusqu'a maintenant, on ouvre un nouveau schema
	if (!schema_vues.size()) schema_vues << new SchemaVue(this);
	
	// ajout de tous les SchemaVue necessaires
	foreach (SchemaVue *sv, schema_vues) addSchemaVue(sv);
	
	// titre de la fenetre
	setWindowTitle(tr("QElectroTech"));
	
	// icone de la fenetre
	setWindowIcon(QIcon(":/ico/qet.png"));
	
	// barre de statut de la fenetre
	statusBar() -> showMessage(tr("QElectrotech"));
	
	// ajout du panel d'Appareils en tant que QDockWidget
	qdw_pa = new QDockWidget(tr("Panel d'appareils"), this);
	qdw_pa -> setAllowedAreas(Qt::AllDockWidgetAreas);
	qdw_pa -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	qdw_pa -> setMinimumWidth(160);
	qdw_pa -> setWidget(pa = new PanelAppareils(qdw_pa));
	addDockWidget(Qt::LeftDockWidgetArea, qdw_pa);
	
	// mise en place des actions
	actions();
	
	// mise en place de la barre d'outils
	toolbar();
	
	// mise en place des menus
	menus();
	
	// systray de l'application
	if (QSystemTrayIcon::isSystemTrayAvailable()) {
		qsti = new QSystemTrayIcon(QIcon(":/ico/qet.png"), this);
		qsti -> setToolTip(tr("QElectroTech"));
		connect(qsti, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(systray(QSystemTrayIcon::ActivationReason)));
		menu_systray = new QMenu(tr("QElectroTech"));
		menu_systray -> addAction(masquer_appli);
		menu_systray -> addAction(quitter_qet);
		qsti -> setContextMenu(menu_systray);
		qsti -> show();
	}
	
	// la fenetre est maximisee par defaut
	setMinimumWidth(500);
	setMinimumHeight(350);
	setWindowState(Qt::WindowMaximized);
	
	// connexions signaux / slots pour une interface sensee
	connect(&workspace, SIGNAL(windowActivated(QWidget *)), this, SLOT(slot_updateActions()));
	connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slot_updateActions()));
}

/**
	Gere les evenements relatifs au QSystemTrayIcon
	@param raison un entier representant l'evenement survenu sur le systray
*/
void QETApp::systray(QSystemTrayIcon::ActivationReason raison) {
	if (!QSystemTrayIcon::isSystemTrayAvailable()) return;
	switch(raison) {
		case QSystemTrayIcon::Context:
			// affichage du menu
			(qsti -> contextMenu()) -> show();
			break;
		case QSystemTrayIcon::DoubleClick:
		case QSystemTrayIcon::Trigger:
			// reduction ou restauration de l'application
			if (isVisible()) systrayReduire(); else systrayRestaurer();
			break;
		case QSystemTrayIcon::Unknown:
		default: // ne rien faire
		break;
	}
}

/**
	Reduit l'application dans le systray
*/
void QETApp::systrayReduire() {
	// on sauvegarde la position et les dimensions de l'application
	wg = saveGeometry();
	// on cache l'application
	hide();
	// on ajoute le menu "Restaurer" et on enlève le menu "Masquer"
	menu_systray -> insertAction(masquer_appli, restaurer_appli);
	menu_systray -> removeAction(masquer_appli);
}

/**
	Restaure l'application reduite dans le systray
*/
void QETApp::systrayRestaurer() {
	// on restaure la position et les dimensions de l'application
	restoreGeometry(wg);
	// on affiche l'application
	show();
	// on ajoute le menu "Masquer" et on enlève le menu "Restaurer"
	menu_systray -> insertAction(restaurer_appli, masquer_appli);
	menu_systray -> removeAction(restaurer_appli);
}

/**
	Permet de quitter l'application lors de la fermeture de la fenetre principale
*/
void QETApp::closeEvent(QCloseEvent *) {
	quitter();
}

/**
	Gere la sortie de l'application
	@todo gerer les eventuelles fermetures de fichiers
*/
void QETApp::quitter() {
	if (!schemaEnCours()) qApp -> quit();
	else {
		bool peut_quitter = true;
		foreach(QWidget *fenetre, workspace.windowList()) {
			if (qobject_cast<SchemaVue *>(fenetre)) {
				workspace.setActiveWindow(fenetre);
				if (!fermer()) {
					peut_quitter = false;
					break;
				}
			}
		}
		if (peut_quitter) qApp -> quit();
	}
	
}

/**
	Fait passer la fenetre en mode plein ecran au mode normal et vice-versa
*/
void QETApp::toggleFullScreen() {
	setWindowState(windowState() ^ Qt::WindowFullScreen);
}

/**
	Active ou desactive l'antialiasing sur le rendu graphique du Schema
*/
void QETApp::toggleAntialiasing() {
	SchemaVue *sv = schemaEnCours();
	if (!sv) return;
	sv -> setAntialiasing(!sv -> antialiased());
	toggle_aa -> setText(sv -> antialiased() ? tr("D\351sactiver l'&antialiasing") : tr("Activer l'&antialiasing"));
}

/**
	Dialogue « A propos de QElectroTech »
	Le dialogue en question est cree lors du premier appel de cette fonction.
	En consequence, sa premiere apparition n'est pas immediate. Par la suite,
	le dialogue n'a pas a etre recree et il apparait instantanement. Il est
	detruit en meme temps que son parent (ici, la QETApp).
*/
void QETApp::aPropos() {
	static AboutQET *apqet = new AboutQET(this);
	apqet -> exec();
}

/**
	Mise en place des actions
*/
void QETApp::actions() {
	// icones et labels
	nouveau_fichier   = new QAction(QIcon(":/ico/new.png"),        tr("&Nouveau"),                       this);
	ouvrir_fichier    = new QAction(QIcon(":/ico/open.png"),       tr("&Ouvrir"),                        this);
	fermer_fichier    = new QAction(QIcon(":/ico/fileclose.png"),  tr("&Fermer"),                        this);
	enr_fichier       = new QAction(QIcon(":/ico/save.png"),       tr("&Enregistrer"),                   this);
	enr_fichier_sous  = new QAction(QIcon(":/ico/saveas.png"),     tr("Enregistrer sous"),               this);
	importer          = new QAction(QIcon(":/ico/import.png"),     tr("&Importer"),                      this);
	exporter          = new QAction(QIcon(":/ico/export.png"),     tr("E&xporter"),                      this);
	imprimer          = new QAction(QIcon(":/ico/print.png"),      tr("Imprimer"),                       this);
	quitter_qet       = new QAction(QIcon(":/ico/exit.png"),       tr("&Quitter"),                       this);
	
	annuler           = new QAction(QIcon(":/ico/undo.png"),       tr("Annu&ler"),                       this);
	refaire           = new QAction(QIcon(":/ico/redo.png"),       tr("Re&faire"),                       this);
	couper            = new QAction(QIcon(":/ico/cut.png"),        tr("Co&uper"),                        this);
	copier            = new QAction(QIcon(":/ico/copy.png"),       tr("Cop&ier"),                        this);
	coller            = new QAction(QIcon(":/ico/paste.png"),      tr("C&oller"),                        this);
	sel_tout          = new QAction(                               tr("Tout s\351lectionner"),           this);
	sel_rien          = new QAction(                               tr("D\351s\351lectionner tout"),      this);
	sel_inverse       = new QAction(                               tr("Inverser la s\351lection"),       this);
	supprimer         = new QAction(QIcon(":/ico/delete.png"),     tr("Supprimer"),                      this);
	pivoter           = new QAction(QIcon(":/ico/pivoter.png"),    tr("Pivoter"),                        this);
	
	toggle_aa         = new QAction(                               tr("D\351sactiver l'&antialiasing"),  this);
	zoom_avant        = new QAction(QIcon(":/ico/viewmag+.png"),   tr("Zoom avant"),                     this);
	zoom_arriere      = new QAction(QIcon(":/ico/viewmag-.png"),   tr("Zoom arri\350re"),                this);
	zoom_adapte       = new QAction(QIcon(":/ico/viewmagfit.png"), tr("Zoom adapt\351"),                 this);
	zoom_reset        = new QAction(QIcon(":/ico/viewmag.png"),    tr("Pas de zoom"),                    this);
	
	mode_selection    = new QAction(QIcon(":/ico/select.png"),     tr("Mode Selection"),                 this);
	mode_visualise    = new QAction(QIcon(":/ico/move.png"),       tr("Mode Visualisation"),             this);
	
	entrer_pe         = new QAction(QIcon(":/ico/entrer_fs.png"),  tr("Passer en &mode plein \351cran"), this);
	sortir_pe         = new QAction(QIcon(":/ico/sortir_fs.png"),  tr("Sortir du &mode plein \351cran"), this);
	configurer        = new QAction(QIcon(":/ico/configure.png"),  tr("&Configurer QElectroTech"),       this);
	
	f_mosaique        = new QAction(                               tr("&Mosa\357que"),                   this);
	f_cascade         = new QAction(                               tr("&Cascade"),                       this);
	f_reorganise      = new QAction(                               tr("Arranger les fen\352tres réduites"),this);
	f_suiv            = new QAction(                               tr("Fen\352tre suivante"),            this);
	f_prec            = new QAction(                               tr("Fen\352tre pr\351c\351dente"),    this);
	
	a_propos_de_qet   = new QAction(QIcon(":/ico/qet.png"),        tr("\300 &propos de QElectroTech"),   this);
	a_propos_de_qt    = new QAction(QIcon(":/ico/qt.png"),         tr("\300 propos de &Qt"),             this);
	
	masquer_appli     = new QAction(QIcon(":/ico/masquer.png"),    tr("&Masquer"),                       this);
	restaurer_appli   = new QAction(QIcon(":/ico/restaurer.png"),  tr("&Restaurer"),                     this);
	
	// info-bulles / indications dans la barre de statut
	masquer_appli     -> setToolTip(tr("Reduire QElectroTech dans le systray"));
	restaurer_appli   -> setToolTip(tr("Restaurer QElectroTech"));
	
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
	supprimer         -> setShortcut(QKeySequence::Delete);
	pivoter           -> setShortcut(QKeySequence(tr("P")));
	
	zoom_avant        -> setShortcut(QKeySequence::ZoomIn);
	zoom_arriere      -> setShortcut(QKeySequence::ZoomOut);
	zoom_adapte       -> setShortcut(QKeySequence(tr("Ctrl+9")));
	zoom_reset        -> setShortcut(QKeySequence(tr("Ctrl+0")));
	
	entrer_pe         -> setShortcut(QKeySequence(tr("Ctrl+Shift+F")));
	sortir_pe         -> setShortcut(QKeySequence(tr("Ctrl+Shift+F")));
	
	// 
	f_mosaique        -> setStatusTip(tr("Dispose les fen\352tres en mosa\357que"));
	f_cascade         -> setStatusTip(tr("Dispose les fen\352tres en cascade"));
	f_reorganise      -> setStatusTip(tr("Aligne les fen\352tres réduites"));
	f_suiv            -> setStatusTip(tr("Active la fen\352tre suivante"));
	f_prec            -> setStatusTip(tr("Active la fen\352tre pr\351c\351dente"));
	
	// traitements speciaux
	mode_selection    -> setCheckable(true);
	mode_visualise    -> setCheckable(true);
	mode_selection    -> setChecked(true);
	
	QActionGroup *grp_visu_sel = new QActionGroup(this);
	grp_visu_sel -> addAction(mode_selection);
	grp_visu_sel -> addAction(mode_visualise);
	grp_visu_sel -> setExclusive(true);
	
	// connexion a des slots
	connect(quitter_qet,      SIGNAL(triggered()), this,       SLOT(quitter())                  );
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
	connect(masquer_appli,    SIGNAL(triggered()), this,       SLOT(systrayReduire  ())         );
	connect(restaurer_appli,  SIGNAL(triggered()), this,       SLOT(systrayRestaurer())         );
	connect(zoom_avant,       SIGNAL(triggered()), this,       SLOT(slot_zoomPlus())            );
	connect(zoom_arriere,     SIGNAL(triggered()), this,       SLOT(slot_zoomMoins())           );
	connect(zoom_adapte,      SIGNAL(triggered()), this,       SLOT(slot_zoomFit())             );
	connect(zoom_reset,       SIGNAL(triggered()), this,       SLOT(slot_zoomReset())           );
	connect(imprimer,         SIGNAL(triggered()), this,       SLOT(dialogue_imprimer())        );
	connect(exporter,         SIGNAL(triggered()), this,       SLOT(dialogue_exporter())        );
	connect(enr_fichier_sous, SIGNAL(triggered()), this,       SLOT(dialogue_enregistrer_sous()));
	connect(enr_fichier,      SIGNAL(triggered()), this,       SLOT(enregistrer())              );
	connect(nouveau_fichier,  SIGNAL(triggered()), this,       SLOT(nouveau())                  );
	connect(ouvrir_fichier,   SIGNAL(triggered()), this,       SLOT(ouvrir())                   );
	connect(fermer_fichier,   SIGNAL(triggered()), this,       SLOT(fermer())                   );
	connect(couper,           SIGNAL(triggered()), this,       SLOT(slot_couper())              );
	connect(copier,           SIGNAL(triggered()), this,       SLOT(slot_copier())              );
	connect(coller,           SIGNAL(triggered()), this,       SLOT(slot_coller())              );
	connect(toggle_aa,        SIGNAL(triggered()), this,       SLOT(toggleAntialiasing())       );
	connect(f_mosaique,       SIGNAL(triggered()), &workspace, SLOT(tile()));
	connect(f_cascade,        SIGNAL(triggered()), &workspace, SLOT(cascade()));
	connect(f_reorganise,     SIGNAL(triggered()), &workspace, SLOT(arrangeIcons()));
	connect(f_suiv,           SIGNAL(triggered()), &workspace, SLOT(activateNextWindow()));
	connect(f_prec,           SIGNAL(triggered()), &workspace, SLOT(activatePreviousWindow()));
}

/**
	Mise en place des menus
*/
void QETApp::menus() {
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
	
	// menu Affichage > Afficher
	QMenu *menu_aff_aff = new QMenu(tr("Afficher"));
	menu_aff_aff -> addAction(barre_outils -> toggleViewAction());
	menu_aff_aff -> addAction(qdw_pa -> toggleViewAction());
	
	// menu Affichage
	menu_affichage -> addMenu(menu_aff_aff);
	menu_affichage -> addSeparator();
	menu_affichage -> addAction(toggle_aa);
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
	
	// menu Fenêtres
	slot_updateMenuFenetres();
	
	// menu Aide
	menu_aide -> addAction(a_propos_de_qet);
	menu_aide -> addAction(a_propos_de_qt);
}

/**
	Mise en place de la barre d'outils
*/
void QETApp::toolbar() {
	barre_outils = new QToolBar(tr("Outils"), this);
	
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
	gere l'impression
*/
void QETApp::dialogue_imprimer() {
	QPrinter *qprin = new QPrinter();
	QPrintDialog *qpd = new QPrintDialog(qprin, this);
	qpd -> exec();
}

void QETApp::dialogue_exporter() {
	QString nom_fichier = QFileDialog::getSaveFileName(
		this,
		tr("Exporter vers le fichier"),
		QDir::homePath(),
		tr("Image PNG (*.png)")
	);
	if (nom_fichier != "") {
		if (!nom_fichier.endsWith(".png", Qt::CaseInsensitive)) nom_fichier += ".png";
		QFile fichier(nom_fichier);
		QImage image = schemaEnCours() -> scene -> toImage();
		image.save(&fichier, "PNG");
		fichier.close();
	}
}

/**
	Methode enregistrant le schema dans le dernier nom de fichier connu.
	Si aucun nom de fichier n'est connu, cette methode appelle la methode enregistrer_sous
	@return true si l'enregistrement a reussi, false sinon
*/
bool QETApp::enregistrer() {
	if (!schemaEnCours()) return(false);
	return(schemaEnCours() -> enregistrer());
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
bool QETApp::dialogue_enregistrer_sous() {
	if (!schemaEnCours()) return(false);
	return(schemaEnCours() -> enregistrer_sous());
}

/**
	Cette methode cree un nouveau schema.
	@return true si tout s'est bien passe ; false si vous executez cette fonction dans un univers non cartesien (en fait y'a pas de return(false) :p)
*/
bool QETApp::nouveau() {
	addSchemaVue(new SchemaVue(this));
	return(true);
}

/**
	Cette fonction demande un nom de fichier a ouvrir a l'utilisateur
	@return true si l'ouverture a reussi, false sinon
*/
bool QETApp::ouvrir() {
	// demande un nom de fichier a ouvrir a l'utilisateur
	QString nom_fichier = QFileDialog::getOpenFileName(
		this,
		tr("Ouvrir un fichier"),
		QDir::homePath(),
		tr("Schema QelectroTech (*.qet);;Fichiers XML (*.xml);;Tous les fichiers (*)")
	);
	if (nom_fichier == "") return(false);
	
	// verifie que le fichier n'est pas deja ouvert
	QString chemin_fichier = QFileInfo(nom_fichier).canonicalFilePath();
	foreach (QWidget *fenetre, workspace.windowList()) {
		SchemaVue *fenetre_en_cours = qobject_cast<SchemaVue *>(fenetre);
		if (QFileInfo(fenetre_en_cours -> nom_fichier).canonicalFilePath() == chemin_fichier) {
			workspace.setActiveWindow(fenetre);
			return(false);
		}
	}
	
	// ouvre le fichier
	SchemaVue *sv = new SchemaVue(this);
	int code_erreur;
	if (sv -> ouvrir(nom_fichier, &code_erreur)) {
		addSchemaVue(sv);
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
bool QETApp::fermer() {
	SchemaVue *sv = schemaEnCours();
	if (!sv) return(false);
	bool fermeture_schema = sv -> close();
	if (fermeture_schema) delete sv;
	return(fermeture_schema);
}

/**
	@return Le SchemaVue qui a le focus dans l'interface MDI
*/
SchemaVue *QETApp::schemaEnCours() {
	return(qobject_cast<SchemaVue *>(workspace.activeWindow()));
}

void QETApp::slot_couper() {
	if(schemaEnCours()) schemaEnCours() -> couper();
}

void QETApp::slot_copier() {
	if(schemaEnCours()) schemaEnCours() -> copier();
}

void QETApp::slot_coller() {
	if(schemaEnCours()) schemaEnCours() -> coller();
}

void QETApp::slot_zoomPlus() {
	if(schemaEnCours()) schemaEnCours() -> zoomPlus();
}

void QETApp::slot_zoomMoins() {
	if(schemaEnCours()) schemaEnCours() -> zoomMoins();
}

void QETApp::slot_zoomFit() {
	if(schemaEnCours()) schemaEnCours() -> zoomFit();
}

void QETApp::slot_zoomReset() {
	if(schemaEnCours()) schemaEnCours() -> zoomReset();
}

void QETApp::slot_selectAll() {
	if(schemaEnCours()) schemaEnCours() -> selectAll();
}

void QETApp::slot_selectNothing() {
	if(schemaEnCours()) schemaEnCours() -> selectNothing();
}

void QETApp::slot_selectInvert() {
	if(schemaEnCours()) schemaEnCours() -> selectInvert();
}

void QETApp::slot_supprimer() {
	if(schemaEnCours()) schemaEnCours() -> supprimer();
}

void QETApp::slot_pivoter() {
	if(schemaEnCours()) schemaEnCours() -> pivoter();
}

void QETApp::slot_setSelectionMode() {
	if(schemaEnCours()) schemaEnCours() -> setSelectionMode();
}

void QETApp::slot_setVisualisationMode() {
	if(schemaEnCours()) schemaEnCours() -> setVisualisationMode();
}

/**
	gere les actions ayant besoin d'un document ouvert
*/
void QETApp::slot_updateActions() {
	SchemaVue *sv = schemaEnCours();
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
	toggle_aa        -> setEnabled(document_ouvert);
	
	// actions ayant aussi besoin d'un historique des actions
	annuler          -> setEnabled(document_ouvert);
	refaire          -> setEnabled(document_ouvert);
	
	// actions ayant aussi besoin d'elements selectionnes
	bool elements_selectionnes = document_ouvert ? (sv -> scene -> selectedItems().size() > 0) : false;
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
	
	// actions ayant besoin de la connaissance de son mode
	if (document_ouvert) toggle_aa -> setText(sv -> antialiased() ? tr("D\351sactiver l'&antialiasing") : tr("Activer l'&antialiasing"));
	
	slot_updateMenuFenetres();
}

void QETApp::addSchemaVue(SchemaVue *sv) {
	if (!sv) return;
	SchemaVue *s_v = schemaEnCours();
	bool maximise = ((!s_v) || (s_v -> windowState() & Qt::WindowMaximized));
	QWidget *p = workspace.addWindow(sv);
	connect(sv, SIGNAL(selectionChanged()), this, SLOT(slot_updateActions()));
	connect(sv, SIGNAL(modeChanged()),      this, SLOT(slot_updateActions()));
	if (maximise) p -> showMaximized();
	else p -> show();
}

void QETApp::slot_updateMenuFenetres() {
	// nettoyage du menu
	menu_fenetres -> clear();
	
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
	if (!fenetres.isEmpty()) menu_fenetres -> addSeparator();
	for (int i = 0 ; i < fenetres.size() ; ++ i) {
		SchemaVue *sv = qobject_cast<SchemaVue *>(fenetres.at(i));
		QAction *action  = menu_fenetres -> addAction(sv -> windowTitle().left(sv -> windowTitle().length()-3));
		action -> setCheckable(true);
		action -> setChecked(sv == schemaEnCours());
		connect(action, SIGNAL(triggered()), &windowMapper, SLOT(map()));
		windowMapper.setMapping(action, sv);
	}
}
