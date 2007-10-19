#include "qetapp.h"
#include "qetdiagrameditor.h"
#include "qetelementeditor.h"
#define QUOTE(x) STRINGIFY(x)
#define STRINGIFY(x) #x

QString QETApp::common_elements_dir = QString();

/**
	Constructeur
	@param argc Nombre d'arguments passes a l'application
	@param argv Arguments passes a l'application
*/
QETApp::QETApp(int &argc, char **argv) : QApplication(argc, argv) {
	// selectionne le langage du systeme
	QString system_language = QLocale::system().name().left(2);
	setLanguage(system_language);
	
	// nettoyage avant de quitter l'application
	connect(this, SIGNAL(aboutToQuit()), this, SLOT(cleanup()));
	
	// systray de l'application
	if (QSystemTrayIcon::isSystemTrayAvailable()) {
		// initialisation des menus de l'icone dans le systray
		menu_systray = new QMenu(tr("QElectroTech"));
		
		quitter_qet       = new QAction(QIcon(":/ico/exit.png"),       tr("&Quitter"),                                        this);
		reduce_appli      = new QAction(QIcon(":/ico/masquer.png"),    tr("&Masquer"),                                        this);
		restore_appli     = new QAction(QIcon(":/ico/restaurer.png"),  tr("&Restaurer"),                                      this);
		reduce_diagrams   = new QAction(QIcon(":/ico/masquer.png"),    tr("&Masquer tous les \351diteurs de sch\351ma"),      this);
		restore_diagrams  = new QAction(QIcon(":/ico/restaurer.png"),  tr("&Restaurer tous les \351diteurs de sch\351ma"),    this);
		reduce_elements   = new QAction(QIcon(":/ico/masquer.png"),    tr("&Masquer tous les \351diteurs d'\351l\351ment"),   this);
		restore_elements  = new QAction(QIcon(":/ico/restaurer.png"),  tr("&Restaurer tous les \351diteurs d'\351l\351ment"), this);
		new_diagram       = new QAction(QIcon(":/ico/window_new.png"), tr("&Nouvel \351diteur de sch\351ma"),                 this);
		new_element       = new QAction(QIcon(":/ico/window_new.png"), tr("&Nouvel \351diteur d'\351l\351ment"),              this);
		
		quitter_qet   -> setStatusTip(tr("Ferme l'application QElectroTech"));
		reduce_appli  -> setToolTip(tr("R\351duire QElectroTech dans le systray"));
		restore_appli -> setToolTip(tr("Restaurer QElectroTech"));
		
		connect(quitter_qet,      SIGNAL(triggered()), this, SLOT(quitQET()));
		connect(reduce_appli,     SIGNAL(triggered()), this, SLOT(reduceEveryEditor()));
		connect(restore_appli,    SIGNAL(triggered()), this, SLOT(restoreEveryEditor()));
		connect(reduce_diagrams,  SIGNAL(triggered()), this, SLOT(reduceDiagramEditors()));
		connect(restore_diagrams, SIGNAL(triggered()), this, SLOT(restoreDiagramEditors()));
		connect(reduce_elements,  SIGNAL(triggered()), this, SLOT(reduceElementEditors()));
		connect(restore_elements, SIGNAL(triggered()), this, SLOT(restoreElementEditors()));
		connect(new_diagram,      SIGNAL(triggered()), this, SLOT(newDiagramEditor()));
		connect(new_element,      SIGNAL(triggered()), this, SLOT(newElementEditor()));
		
		// connexion pour le signalmapper
		connect(&signal_map, SIGNAL(mapped(QWidget *)), this, SLOT(invertMainWindowVisibility(QWidget *)));
		
		// initialisation de l'icone du systray
		qsti = new QSystemTrayIcon(QIcon(":/ico/qet.png"), this);
		qsti -> setToolTip(tr("QElectroTech"));
		connect(qsti, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(systray(QSystemTrayIcon::ActivationReason)));
		qsti -> setContextMenu(menu_systray);
		qsti -> show();
		
		setQuitOnLastWindowClosed(false);
		connect(this, SIGNAL(lastWindowClosed()), this, SLOT(checkRemainingWindows()));
	}
	
	// parse les arguments
	foreach(QString argument, arguments()) {
#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
		QString ced_arg("--common-elements-dir=");
		if (argument.startsWith(ced_arg)) {
			QString ced_value = argument.right(argument.length() - ced_arg.length());
			overrideCommonElementsDir(ced_value);
		}
#endif
	}
	
	// Creation et affichage d'un editeur de schema
	QStringList files;
	foreach(QString argument, arguments()) {
		if (QFileInfo(argument).exists()) files << argument;
	}
	new QETDiagramEditor(files);
	buildSystemTrayMenu();
}

/// Destructeur
QETApp::~QETApp() {
}

/**
	Change le langage utilise par l'application.
	@param desired_language langage voulu
*/
void QETApp::setLanguage(const QString &desired_language) {
	QString languages_path = QETApp::languagesPath();
	
	// charge les eventuelles traductions pour la lib Qt
	qtTranslator.load("qt_" + desired_language, languages_path);
	installTranslator(&qtTranslator);
	
	// determine la langue a utiliser pour l'application
	if (desired_language != "fr") {
		// utilisation de la version anglaise par defaut
		if (!qetTranslator.load("qet_" + desired_language, languages_path)) {
			qetTranslator.load("qet_en", languages_path);
		}
		installTranslator(&qetTranslator);
	}
}

/**
	Gere les evenements relatifs au QSystemTrayIcon
	@param raison un entier representant l'evenement survenu sur le systray
*/
void QETApp::systray(QSystemTrayIcon::ActivationReason reason) {
	if (!QSystemTrayIcon::isSystemTrayAvailable()) return;
	switch(reason) {
		case QSystemTrayIcon::Context:
			// affichage du menu
			buildSystemTrayMenu();
			qsti -> contextMenu() -> show();
			break;
		case QSystemTrayIcon::DoubleClick:
		case QSystemTrayIcon::Trigger:
			// reduction ou restauration de l'application
			fetchWindowStats(diagramEditors(), elementEditors());
			if (every_editor_reduced) restoreEveryEditor(); else reduceEveryEditor();
			break;
		case QSystemTrayIcon::Unknown:
		default: // ne rien faire
		break;
	}
}

/// Reduit toutes les fenetres de l'application dans le systray
void QETApp::reduceEveryEditor() {
	reduceDiagramEditors();
	reduceElementEditors();
	every_editor_reduced = true;
}

/// Restaure toutes les fenetres de l'application dans le systray
void QETApp::restoreEveryEditor() {
	restoreDiagramEditors();
	restoreElementEditors();
	every_editor_reduced = false;
}

/// Reduit tous les editeurs de schemas dans le systray
void QETApp::reduceDiagramEditors() {
	foreach(QETDiagramEditor *e, diagramEditors()) setMainWindowVisible(e, false);
}

/// Restaure tous les editeurs de schemas dans le systray
void QETApp::restoreDiagramEditors() {
	foreach(QETDiagramEditor *e, diagramEditors()) setMainWindowVisible(e, true);
}

/// Reduit tous les editeurs d'element dans le systray
void QETApp::reduceElementEditors() {
	foreach(QETElementEditor *e, elementEditors()) setMainWindowVisible(e, false);
}

/// Restaure tous les editeurs d'element dans le systray
void QETApp::restoreElementEditors() {
	foreach(QETElementEditor *e, elementEditors()) setMainWindowVisible(e, true);
}

/// lance un nouvel editeur de schemas
void QETApp::newDiagramEditor() {
	new QETDiagramEditor();
}

/// lance un nouvel editeur d'element
void QETApp::newElementEditor() {
	new QETElementEditor();
}

/**
	Renvoie le dossier des elements communs, c-a-d le chemin du dossier dans
	lequel QET doit chercher les definitions XML des elements de la collection QET.
	@return Le chemin du dossier des elements communs
*/
QString QETApp::commonElementsDir() {
#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
	if (common_elements_dir != QString()) return(common_elements_dir);
#endif
#ifdef QET_COMMON_COLLECTION_PATH
	return(QUOTE(QET_COMMON_COLLECTION_PATH));
#else
	return(QDir::current().path() + "/elements/");
#endif
}

/**
	Renvoie le dossier des elements de l'utilisateur, c-a-d le chemin du dossier
	dans lequel QET chercher les definitions XML des elements propres a
	l'utilisateur.
	@return Le chemin du dossier des elements persos
*/
QString QETApp::customElementsDir() {
	return(QETApp::configDir() + "elements/");
}

/**
	Renvoie le dossier de configuration de QET, c-a-d le chemin du dossier dans
	lequel QET lira les informations de configuration et de personnalisation
	propres a l'utilisateur courant. Ce dossier est generalement
	C:\Documents And Settings\utilisateur\Application Data\qet sous Windows et
	~/.qet sous les systemes type UNIX.
	@return Le chemin du dossier de configuration de QElectroTech
*/
QString QETApp::configDir() {
#ifdef Q_OS_WIN32
	return(QDir::homePath() + "/Application Data/qet/");
#else
	return(QDir::homePath() + "/.qet/");
#endif
}

/**
	Permet de connaitre le chemin absolu du fichier *.elmt correspondant a un
	chemin symbolique (du type custom://outils_pervers/sado_maso/contact_bizarre)
	@param sym_path Chaine de caracteres representant le chemin absolu du fichier
	@return Une chaine de caracteres vide en cas d'erreur ou le chemin absolu du
	fichier *.elmt.
*/
QString QETApp::realPath(const QString &sym_path) {
	QString directory;
	if (sym_path.startsWith("common://")) {
		directory = commonElementsDir();
	} else if (sym_path.startsWith("custom://")) {
		directory = customElementsDir();
	} else return(QString());
	return(directory + QDir::toNativeSeparators(sym_path.right(sym_path.length() - 9)));
}

/**
	Construit le chemin symbolique (du type custom://outils_pervers/sado_maso/
	contact_bizarre) correspondant a un fichier.
	@param real_pathChaine de caracteres representant le chemin symbolique du fichier
	@return Une chaine de caracteres vide en cas d'erreur ou le chemin
	symbolique designant l'element.
*/
QString QETApp::symbolicPath(const QString &real_path) {
	// recupere les dossier common et custom
	QString commond = commonElementsDir();
	QString customd = customElementsDir();
	QString chemin;
	// analyse le chemin de fichier passe en parametre
	if (real_path.startsWith(commond)) {
		chemin = "common://" + real_path.right(real_path.length() - commond.length());
	} else if (real_path.startsWith(customd)) {
		chemin = "custom://" + real_path.right(real_path.length() - customd.length());
	} else chemin = QString();
	return(chemin);
}

#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
/**
	Redefinit le chemin du dossier des elements communs.
*/
void QETApp::overrideCommonElementsDir(const QString &new_ced) {
	QFileInfo new_ced_info(new_ced);
	if (new_ced_info.isDir()) {
		common_elements_dir = new_ced_info.absoluteFilePath();
		if (!common_elements_dir.endsWith("/")) common_elements_dir += "/";
	}
}
#endif

/**
	@return Le chemin du dossier contenant les fichiers de langue
*/
QString QETApp::languagesPath() {
#ifndef QET_LANG_PATH
	return(QDir::current().path() + "/lang/");
#else
	return(QUOTE(QET_LANG_PATH));
#endif
}

/**
	Ferme tous les editeurs
	@return true si l'utilisateur a accepte toutes les fermetures, false sinon
*/
bool QETApp::closeEveryEditor() {
	// s'assure que toutes les fenetres soient visibles avant de quitter
	restoreEveryEditor();
	bool every_window_closed = true;
	foreach(QETDiagramEditor *e, diagramEditors()) {
		every_window_closed = every_window_closed && e -> close();
	}
	foreach(QETElementEditor *e, elementEditors()) {
		every_window_closed = every_window_closed && e -> close();
	}
	return(every_window_closed);
}

/**
	Nettoie certaines choses avant que l'application ne quitte
*/
void QETApp::cleanup() {
	if (QSystemTrayIcon::isSystemTrayAvailable()) qsti -> hide();
}

/// @return les editeurs de schemas ouverts
QList<QETDiagramEditor *> QETApp::diagramEditors() const {
	QList<QETDiagramEditor *> diagram_editors;
	foreach(QWidget *qw, topLevelWidgets()) {
		if (!qw -> isWindow()) continue;
		if (QETDiagramEditor *de = qobject_cast<QETDiagramEditor *>(qw)) {
			diagram_editors << de;
		}
	}
	return(diagram_editors);
}

/// @return les editeurs d'elements ouverts
QList<QETElementEditor *> QETApp::elementEditors() const {
	QList<QETElementEditor *> element_editors;
	foreach(QWidget *qw, topLevelWidgets()) {
		if (!qw -> isWindow()) continue;
		if (QETElementEditor *ee = qobject_cast<QETElementEditor *>(qw)) {
			element_editors << ee;
		}
	}
	return(element_editors);
}

/**
	Affiche ou cache une fenetre (editeurs de schemas / editeurs d élements)
	@param window fenetre a afficher / cacher
	@param visible true pour affiche la fenetre, false sinon
*/
void QETApp::setMainWindowVisible(QMainWindow *window, bool visible) {
	if (window -> isVisible() == visible) return;
	if (!visible) {
		window_geometries.insert(window, window -> saveGeometry());
		window_states.insert(window, window -> saveState());
		window -> hide();
		// cache aussi les toolbars et les docks
		foreach (QWidget *qw, floatingToolbarsAndDocksForMainWindow(window)) {
			qw -> hide();
		}
	} else {
		window -> show();
#ifndef Q_OS_WIN32
		window -> restoreGeometry(window_geometries[window]);
#endif
		window -> restoreState(window_states[window]);
	}
}

/**
	Affiche une fenetre (editeurs de schemas / editeurs d élements) si
	celle-ci est cachee ou la cache si elle est affichee.
	@param window fenetre a afficher / cacher
*/
void QETApp::invertMainWindowVisibility(QWidget *window) {
	if (QMainWindow *w = qobject_cast<QMainWindow *>(window)) setMainWindowVisible(w, !w -> isVisible());
}

/**
	Demande la fermeture de toutes les fenetres ; si l'utilisateur les accepte,
	l'application quitte
*/
void QETApp::quitQET() {
	if (closeEveryEditor()) {
		quit();
	}
}

/**
	Verifie s'il reste des fenetres (cachees ou non) et quitte s'il n'en reste
	plus.
*/
void QETApp::checkRemainingWindows() {
	/* petite bidouille : le slot se rappelle apres 500 ms d'attente
	afin de compenser le fait que certaines fenetres peuvent encore
	paraitre vivantes alors qu'elles viennent d'etre fermees
	*/
	static bool sleep = true;
	if (sleep) {
		QTimer::singleShot(500, this, SLOT(checkRemainingWindows()));
	} else {
		if (!diagramEditors().count() && !elementEditors().count()) {
			quit();
		}
	}
	sleep = !sleep;
}

/**
	@param window fenetre dont il faut trouver les barres d'outils et dock flottants
	@return les barres d'outils et dock flottants de la fenetre
*/
QList<QWidget *> QETApp::floatingToolbarsAndDocksForMainWindow(QMainWindow *window) const {
	QList<QWidget *> widgets;
	foreach(QWidget *qw, topLevelWidgets()) {
		if (!qw -> isWindow()) continue;
		if (qobject_cast<QToolBar *>(qw) || qobject_cast<QDockWidget *>(qw)) {
			if (qw -> parent() == window) widgets << qw;
		}
	}
	return(widgets);
}

/// construit le menu de l'icone dans le systray
void QETApp::buildSystemTrayMenu() {
	menu_systray -> clear();
	
	// recupere les editeurs
	QList<QETDiagramEditor *> diagrams = diagramEditors();
	QList<QETElementEditor *> elements = elementEditors();
	fetchWindowStats(diagrams, elements);
	
	// ajoute le bouton reduire / restaurer au menu
	menu_systray -> addAction(every_editor_reduced ? restore_appli : reduce_appli);
	
	// ajoute les editeurs de schemas dans un sous-menu
	QMenu *diagrams_submenu = menu_systray -> addMenu(tr("\311diteurs de sch\351mas"));
	diagrams_submenu -> addAction(reduce_diagrams);
	diagrams_submenu -> addAction(restore_diagrams);
	diagrams_submenu -> addAction(new_diagram);
	reduce_diagrams -> setEnabled(!diagrams.isEmpty() && !every_diagram_reduced);
	restore_diagrams -> setEnabled(!diagrams.isEmpty() && !every_diagram_visible);
	diagrams_submenu -> addSeparator();
	foreach(QETDiagramEditor *diagram, diagrams) {
		QAction *current_diagram_menu = diagrams_submenu -> addAction(diagram -> windowTitle());
		current_diagram_menu -> setCheckable(true);
		current_diagram_menu -> setChecked(diagram -> isVisible());
		connect(current_diagram_menu, SIGNAL(triggered()), &signal_map, SLOT(map()));
		signal_map.setMapping(current_diagram_menu, diagram);
	}
	
	// ajoute les editeurs d'elements au menu
	QMenu *elements_submenu = menu_systray -> addMenu(tr("\311diteurs d'\351l\351ment"));
	elements_submenu -> addAction(reduce_elements);
	elements_submenu -> addAction(restore_elements);
	elements_submenu -> addAction(new_element);
	reduce_elements -> setEnabled(!elements.isEmpty() && !every_element_reduced);
	restore_elements -> setEnabled(!elements.isEmpty() && !every_element_visible);
	elements_submenu -> addSeparator();
	foreach(QETElementEditor *element, elements) {
		QAction *current_element_menu = elements_submenu -> addAction(element -> windowTitle());
		current_element_menu -> setCheckable(true);
		current_element_menu -> setChecked(element -> isVisible());
		connect(current_element_menu, SIGNAL(triggered()), &signal_map, SLOT(map()));
		signal_map.setMapping(current_element_menu, element);
	}
	
	// ajoute le bouton quitter au menu
	menu_systray -> addSeparator();
	menu_systray -> addAction(quitter_qet);
}

/// Met a jour les booleens concernant l'etat des fenetres
void QETApp::fetchWindowStats(const QList<QETDiagramEditor *> &diagrams, const QList<QETElementEditor *> &elements) {
	// compte le nombre de schemas visibles
	int visible_diagrams = 0;
	foreach(QMainWindow *w, diagrams) if (w -> isVisible()) ++ visible_diagrams;
	every_diagram_reduced = !visible_diagrams;
	every_diagram_visible = visible_diagrams == diagrams.count();
	
	// compte le nombre de schemas visibles
	int visible_elements = 0;
	foreach(QMainWindow *w, elements) if (w -> isVisible()) ++ visible_elements;
	every_element_reduced = !visible_elements;
	every_element_visible = visible_elements == elements.count();
	
	// determine si tous les elements sont reduits
	every_editor_reduced = every_element_reduced && every_diagram_reduced;
}

/**
	Gere les evenement
	@param e Evenement a gerer
*/
bool QETApp::event(QEvent *e) {
	// gere l'ouverture de fichiers (sous MacOs)
	if (e -> type() == QEvent::FileOpen) {
		// nom du fichier a ouvrir
		QString filename = static_cast<QFileOpenEvent *>(e) -> file();
		// liste des editeurs de schema ouverts
		QList<QETDiagramEditor *> diagrams_editors = diagramEditors();
		if (diagrams_editors.count()) {
			// s'il y a des editeur de schemas ouvert, on cherche ceux qui sont visibles
			QList<QETDiagramEditor *> visible_diagrams_editors;
			foreach(QETDiagramEditor *de, diagrams_editors) {
				if (de -> isVisible()) visible_diagrams_editors << de;
			}
			// ob choisit soit le premier visible soit le premier tout court
			QETDiagramEditor *de_open;
			if (visible_diagrams_editors.count()) {
				de_open = visible_diagrams_editors.first();
			} else {
				de_open = diagrams_editors.first();
				de_open -> setVisible(true);
			}
		} else {
			new QETDiagramEditor(QStringList() << filename);
		}
		return(true);
	} else {
		return(QApplication::event(e));
	}
}
