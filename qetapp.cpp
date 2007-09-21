#include "qetapp.h"
#include "qetdiagrameditor.h"
#include "qetelementeditor.h"
/**
	Constructeur
	@param argc Nombre d'arguments passes a l'application
	@param argv Arguments passes a l'application
*/
QETApp::QETApp(int &argc, char **argv) : QApplication(argc, argv) {
	// QET se loge dans le systray et ne doit donc pas quitter des que toutes
	// les fenetres sont cachees
	//setQuitOnLastWindowClosed(false);
	
	// selectionne le langage du systeme
	QString system_language = QLocale::system().name().left(2);
	setLanguage(system_language);
	
	// systray de l'application
	quitter_qet       = new QAction(QIcon(":/ico/exit.png"),       tr("&Quitter"),                             this);
	reduce_appli      = new QAction(QIcon(":/ico/masquer.png"),    tr("&Masquer"),                             this);
	restore_appli     = new QAction(QIcon(":/ico/restaurer.png"),  tr("&Restaurer"),                           this);
	quitter_qet       -> setStatusTip(tr("Ferme l'application QElectroTech"));
	reduce_appli      -> setToolTip(tr("Reduire QElectroTech dans le systray"));
	restore_appli     -> setToolTip(tr("Restaurer QElectroTech"));
	connect(quitter_qet,      SIGNAL(triggered()), this,       SLOT(closeEveryEditor()));
	connect(reduce_appli,     SIGNAL(triggered()), this,       SLOT(systrayReduce()));
	connect(restore_appli,    SIGNAL(triggered()), this,       SLOT(systrayRestore()));
	if (QSystemTrayIcon::isSystemTrayAvailable()) {
		qsti = new QSystemTrayIcon(QIcon(":/ico/qet.png"), this);
		qsti -> setToolTip(tr("QElectroTech"));
		connect(qsti, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(systray(QSystemTrayIcon::ActivationReason)));
		connect(this, SIGNAL(aboutToQuit()), this, SLOT(cleanup()));
		menu_systray = new QMenu(tr("QElectroTech"));
		menu_systray -> addAction(reduce_appli);
		menu_systray -> addAction(quitter_qet);
		qsti -> setContextMenu(menu_systray);
		qsti -> show();
		every_editor_reduced = false;
	}
	
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
			(qsti -> contextMenu()) -> show();
			break;
		case QSystemTrayIcon::DoubleClick:
		case QSystemTrayIcon::Trigger:
			// reduction ou restauration de l'application
			if (every_editor_reduced) systrayRestore(); else systrayReduce();
			break;
		case QSystemTrayIcon::Unknown:
		default: // ne rien faire
		break;
	}
}

/**
	Reduit toutes les fenetres de l'application dans le systray
*/
void QETApp::systrayReduce() {
	foreach(QETDiagramEditor *e, diagramEditors()) setMainWindowVisible(e, false);
	foreach(QETElementEditor *e, elementEditors()) setMainWindowVisible(e, false);
	// on ajoute le menu "Restaurer" et on enleve le menu "Masquer"
	menu_systray -> insertAction(reduce_appli, restore_appli);
	menu_systray -> removeAction(reduce_appli);
	every_editor_reduced = true;
}

/**
	Restaure toutes les fenetres de l'application dans le systray
*/
void QETApp::systrayRestore() {
	foreach(QETDiagramEditor *e, diagramEditors()) setMainWindowVisible(e, true);
	foreach(QETElementEditor *e, elementEditors()) setMainWindowVisible(e, true);
	// on ajoute le menu "Masquer" et on enleve le menu "Restaurer"
	menu_systray -> insertAction(restore_appli, reduce_appli);
	menu_systray -> removeAction(restore_appli);
	every_editor_reduced = false;
}

/**
	Renvoie le dossier des elements communs, c-a-d le chemin du dossier dans
	lequel QET doit chercher les definitions XML des elements de la collection QET.
	@return Le chemin du dossier des elements communs
*/
QString QETApp::commonElementsDir() {
	return(QDir::current().path() + "/elements/");
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
QString QETApp::realPath(QString &sym_path) {
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
QString QETApp::symbolicPath(QString &real_path) {
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
	@return Le chemin du dossier contenant les fichiers de langue
*/
QString QETApp::languagesPath() {
	return(QDir::current().path() + "/lang/");
}

/**
	Ferme tous les editeurs
*/
void QETApp::closeEveryEditor() {
	// s'assure que toutes les fenetres soient visibles avant de quitter
	systrayRestore();
	foreach(QETDiagramEditor *e, diagramEditors()) e -> close();
	foreach(QETElementEditor *e, elementEditors()) e -> close();
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
		/// @todo cacher aussi les toolbars et les docks ?
	} else {
		window -> show();
		window -> restoreGeometry(window_geometries[window]);
		window -> restoreState(window_states[window]);
	}
}
