/*
	Copyright 2006-2008 Xavier Guerrin
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
#include "qetapp.h"
#include "qetdiagrameditor.h"
#include "qetelementeditor.h"
#include <cstdlib>
#include <iostream>
#define QUOTE(x) STRINGIFY(x)
#define STRINGIFY(x) #x

QString QETApp::common_elements_dir = QString();
QString QETApp::config_dir = QString();
QString QETApp::diagram_texts_font = QString();

/**
	Constructeur
	@param argc Nombre d'arguments passes a l'application
	@param argv Arguments passes a l'application
*/
QETApp::QETApp(int &argc, char **argv) :
	QETSingleApplication(argc, argv, QString("qelectrotech-" + QETApp::userName())),
	non_interactive_execution_(false)
{
	parseArguments();
	initLanguage();
	initConfiguration();
	
	if (!non_interactive_execution_ && isRunning()) {
		QStringList abs_arg_list(arguments_options_);
		abs_arg_list << arguments_files_;
		
		// envoie les arguments a l'instance deja existante
		non_interactive_execution_ = sendMessage("launched-with-args: " + abs_arg_list.join(" "));
	}
	
	if (non_interactive_execution_) {
		std::exit(EXIT_SUCCESS);
	}
	
	initStyle();
	initSystemTray();
	
	// prise en compte des messages des autres instances
	connect(this, SIGNAL(messageAvailable(QString)), this, SLOT(messageReceived(const QString&)));
	
	// nettoyage avant de quitter l'application
	connect(this, SIGNAL(aboutToQuit()), this, SLOT(cleanup()));
	
	// connexion pour le signalmapper
	connect(&signal_map, SIGNAL(mapped(QWidget *)), this, SLOT(invertMainWindowVisibility(QWidget *)));
	
	setQuitOnLastWindowClosed(false);
	connect(this, SIGNAL(lastWindowClosed()), this, SLOT(checkRemainingWindows()));
	
	// Creation et affichage d'un editeur de schema
	new QETDiagramEditor(arguments_files_);
	buildSystemTrayMenu();
}

/// Destructeur
QETApp::~QETApp() {
	delete qsti;
}

/**
	@return l'instance de la QETApp
*/
QETApp *QETApp::instance() {
	return(static_cast<QETApp *>(qApp));
}

/**
	Change le langage utilise par l'application.
	@param desired_language langage voulu
*/
void QETApp::setLanguage(const QString &desired_language) {
	QString languages_path = languagesPath();
	
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
	@param reason un entier representant l'evenement survenu sur le systray
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
	@return le nom de l'utilisateur courant
*/
QString QETApp::userName() {
#ifndef Q_OS_WIN32
	return(QString(getenv("USER")));
#else
	return(QString(getenv("USERNAME")));
#endif
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
	return(configDir() + "elements/");
}

/**
	Renvoie le dossier de configuration de QET, c-a-d le chemin du dossier dans
	lequel QET lira les informations de configuration et de personnalisation
	propres a l'utilisateur courant. Ce dossier est generalement
	C:\\Documents And Settings\\utilisateur\\Application Data\\qet sous Windows et
	~/.qet sous les systemes type UNIX.
	@return Le chemin du dossier de configuration de QElectroTech
*/
QString QETApp::configDir() {
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
	if (config_dir != QString()) return(config_dir);
#endif
#ifdef Q_OS_WIN32
	// recupere l'emplacement du dossier Application Data
	char *app_data_env = getenv("APPDATA");
	QString app_data_str(app_data_env);
	delete app_data_env;
	if (app_data_str.isEmpty()) {
		app_data_str = QDir::homePath() + "/Application Data";
	}
	return(app_data_str + "/qet/");
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
	@param real_path Chaine de caracteres representant le chemin symbolique du fichier
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

/**
	@param filepath Un chemin de fichier
	Note : si filepath est une chaine vide, cette methode retourne 0.
	@return le QETDiagramEditor editant le fichier filepath, ou 0 si ce fichier
	n'est pas edite par l'application.
*/
QETDiagramEditor *QETApp::diagramEditorForFile(const QString &filepath) {
	if (filepath.isEmpty()) return(0);
	
	QETApp *qet_app(QETApp::instance());
	foreach (QETDiagramEditor *diagram_editor, qet_app -> diagramEditors()) {
		if (diagram_editor -> viewForFile(filepath)) {
			return(diagram_editor);
		}
	}
	
	return(0);
}

#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
/**
	Redefinit le chemin du dossier des elements communs
	@param new_ced Nouveau chemin du dossier des elements communs
*/
void QETApp::overrideCommonElementsDir(const QString &new_ced) {
	QFileInfo new_ced_info(new_ced);
	if (new_ced_info.isDir()) {
		common_elements_dir = new_ced_info.absoluteFilePath();
		if (!common_elements_dir.endsWith("/")) common_elements_dir += "/";
	}
}
#endif

#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
/**
	Redefinit le chemin du dossier de configuration
	@param new_cd Nouveau chemin du dossier de configuration
*/
void QETApp::overrideConfigDir(const QString &new_cd) {
	QFileInfo new_cd_info(new_cd);
	if (new_cd_info.isDir()) {
		config_dir = new_cd_info.absoluteFilePath();
		if (!config_dir.endsWith("/")) config_dir += "/";
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

QString QETApp::diagramTextsFont() {
	return(diagram_texts_font);
}

/**
	Nettoie certaines choses avant que l'application ne quitte
*/
void QETApp::cleanup() {
	qsti -> hide();
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
	Affiche ou cache une fenetre (editeurs de schemas / editeurs d'elements)
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
	Affiche une fenetre (editeurs de schemas / editeurs d'elements) si
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
	Gere les messages recus
	@param message Message recu
*/
void QETApp::messageReceived(const QString &message) {
	if (message.startsWith("launched-with-args: ")) {
		QString my_message(message.mid(20));
		QStringList files_list = my_message.split(' ');
		openFiles(files_list);
	}
}

/**
	Ouvre une liste de fichiers.
	Les fichiers sont ouverts dans le premier editeur de schemas visible venu.
	Sinon, le premier editeur de schemas existant venu devient visible et est
	utilise. S'il n'y a aucun editeur de schemas ouvert, un nouveau est cree et
	utilise.
	@param files_list Liste des fichiers a ouvrir
*/
void QETApp::openFiles(const QStringList &files_list) {
	if (files_list.isEmpty()) return;
	
	// liste des editeurs de schema ouverts
	QList<QETDiagramEditor *> diagrams_editors = diagramEditors();
	
	// s'il y a des editeur de schemas ouvert, on cherche ceux qui sont visibles
	if (diagrams_editors.count()) {
		QList<QETDiagramEditor *> visible_diagrams_editors;
		foreach(QETDiagramEditor *de, diagrams_editors) {
			if (de -> isVisible()) visible_diagrams_editors << de;
		}
		
		// on choisit soit le premier visible soit le premier tout court
		QETDiagramEditor *de_open;
		if (visible_diagrams_editors.count()) {
			de_open = visible_diagrams_editors.first();
		} else {
			de_open = diagrams_editors.first();
			de_open -> setVisible(true);
		}
		
		// ouvre les fichiers dans l'editeur ainsi choisi
		foreach(QString file, files_list) {
			de_open -> openAndAddDiagram(file);
		}
	} else {
		// cree un nouvel editeur qui ouvrira les fichiers
		new QETDiagramEditor(files_list);
	}
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

/**
	Parse les arguments suivants :
	  * --common-elements-dir=
	  * --config-dir
	  * --help
	  * --version
	  * -v
	  * --license
	Les autres arguments sont normalement des chemins de fichiers.
	S'ils existent, ils sont juste memorises dans l'attribut arguments_files_.
	Sinon, ils sont memorises dans l'attribut arguments_options_.
*/
void QETApp::parseArguments() {
	// recupere les arguments
	QList<QString> arguments_list(arguments());
	
	// enleve le premier argument : il s'agit du fichier binaire
	arguments_list.takeFirst();
	
	// separe les fichiers des options
	foreach(QString argument, arguments_list) {
		QFileInfo argument_info(argument);
		if (argument_info.exists()) {
			// on exprime les chemins des fichiers en absolu
			arguments_files_ << argument_info.canonicalFilePath();
		} else {
			arguments_options_ << argument;
		}
	}
	
	// parcourt les options
	foreach(QString argument, arguments_options_) {
#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
		QString ced_arg("--common-elements-dir=");
		if (argument.startsWith(ced_arg)) {
			QString ced_value = argument.right(argument.length() - ced_arg.length());
			overrideCommonElementsDir(ced_value);
			continue;
		}
#endif
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
		QString cd_arg("--config-dir=");
		if (argument.startsWith(cd_arg)) {
			QString cd_value = argument.right(argument.length() - cd_arg.length());
			overrideConfigDir(cd_value);
			continue;
		}
#endif
		
		if (argument == QString("--help")) {
			printHelp();
			non_interactive_execution_ = true;
		} else if (argument == QString("--version") || argument == QString("-v")) {
			printVersion();
			non_interactive_execution_ = true;
		} else if (argument == QString("--license")) {
			printLicense();
			non_interactive_execution_ = true;
		}
	}
}

/**
	Determine et applique le langage a utiliser pour l'application
*/
void QETApp::initLanguage() {
	// selectionne le langage du systeme
	QString system_language = QLocale::system().name().left(2);
	setLanguage(system_language);
}

/**
	Met en place tout ce qui concerne le style graphique de l'application
*/
void QETApp::initStyle() {
	// lorsque le style Plastique est active, on le remplace par une version amelioree
	if (qobject_cast<QPlastiqueStyle *>(style())) {
		setStyle(new QETStyle());
	}
}

/**
	Lit et prend en compte la configuration de l'application.
	Cette methode creera, si necessaire :
	  * le dossier de configuration
	  * le dossier de la collection perso
*/
void QETApp::initConfiguration() {
	// cree les dossiers de configuration si necessaire
	QDir config_dir(QETApp::configDir());
	if (!config_dir.exists()) config_dir.mkpath(QETApp::configDir());
	
	QDir custom_elements_dir(QETApp::customElementsDir());
	if (!custom_elements_dir.exists()) custom_elements_dir.mkpath(QETApp::customElementsDir());
	
	// lit le fichier de configuration
	qet_settings = new QSettings(configDir() + "qelectrotech.conf", QSettings::IniFormat, this);
	
	// police a utiliser pour le rendu de texte
	diagram_texts_font = qet_settings -> value("diagramfont", "Sans Serif").toString();
}

/**
	Construit l'icone dans le systray et son menu
*/
void QETApp::initSystemTray() {
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
	
	// initialisation de l'icone du systray
	qsti = new QSystemTrayIcon(QIcon(":/ico/qet.png"), this);
	qsti -> setToolTip(tr("QElectroTech"));
	connect(qsti, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(systray(QSystemTrayIcon::ActivationReason)));
	qsti -> setContextMenu(menu_systray);
	qsti -> show();
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

#ifdef Q_OS_DARWIN
/**
	Gere les evenements, en particulier l'evenement FileOpen sous MacOs.
	@param e Evenement a gerer
*/
bool QETApp::event(QEvent *e) {
	// gere l'ouverture de fichiers (sous MacOs)
	if (e -> type() == QEvent::FileOpen) {
		// nom du fichier a ouvrir
		QString filename = static_cast<QFileOpenEvent *>(e) -> file();
		openFiles(QStringList() << filename);
		return(true);
	} else {
		return(QApplication::event(e));
	}
}
#endif

/**
	Affiche l'aide et l'usage sur la sortie standard
*/
void QETApp::printHelp() {
	QString help(
		tr("Usage : ") + QFileInfo(applicationFilePath()).fileName() + tr(" [options] [fichier]...\n\n") +
		tr("QElectroTech, une application de r\351alisation de sch\351mas \351lectriques.\n\n"
		"Options disponibles : \n"
		"  --help                        Afficher l'aide sur les options\n"
		"  -v, --version                 Afficher la version\n"
		"  --license                     Afficher la licence\n")
#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
		+ tr("  --common-elements-dir=DIR     Definir le dossier de la collection d'elements\n")
#endif
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
		+ tr("  --config-dir=DIR              Definir le dossier de configuration\n")
#endif
	);
	std::cout << qPrintable(help) << std::endl;
}

/**
	Affiche la version sur la sortie standard
*/
void QETApp::printVersion() {
	std::cout << qPrintable(QET::version) << std::endl;
}

/**
	Affiche la licence sur la sortie standard
*/
void QETApp::printLicense() {
	std::cout << qPrintable(QET::license()) << std::endl;
}

/// Constructeur
QETStyle::QETStyle() : QPlastiqueStyle() {
}

/// Destructeur
QETStyle::~QETStyle() {
}

/// Gere les parametres de style
int QETStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returndata) const {
	if (hint == QStyle::SH_DialogButtonBox_ButtonsHaveIcons) {
		return(int(true));
	} else {
		return(QPlastiqueStyle::styleHint(hint, option, widget, returndata));
	}
}

/// Gere les icones standard
QIcon QETStyle::standardIconImplementation(StandardPixmap standardIcon, const QStyleOption *option, const QWidget* widget) const {
	switch(standardIcon) {
		case QStyle::SP_DialogSaveButton:
			return(QIcon(":/ico/save.png"));
		case QStyle::SP_DialogOpenButton:
			return(QIcon(":/ico/open.png"));
		case QStyle::SP_DialogCancelButton:
			return(QIcon(":/ico/button_cancel.png"));
		case QStyle::SP_DialogOkButton:
		case QStyle::SP_DialogApplyButton:
			return(QIcon(":/ico/button_ok.png"));
		case QStyle::SP_DialogCloseButton:
			return(QIcon(":/ico/fileclose.png"));
		case QStyle::SP_DialogYesButton:
			return(QIcon(":/ico/allowed.png"));
		case QStyle::SP_DialogNoButton:
			return(QIcon(":/ico/forbidden.png"));
		case QStyle::SP_DialogResetButton:
			return(QIcon(":/ico/undo.png"));
		case QStyle::SP_DialogHelpButton:
		case QStyle::SP_DialogDiscardButton:
			return(QIcon());
		default:
			return(QPlastiqueStyle::standardIconImplementation(standardIcon, option, widget));
	}
}

/// @return une reference vers les parametres de QElectroTEch
QSettings &QETApp::settings() {
	return(*(instance() -> qet_settings));
}
