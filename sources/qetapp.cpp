/*
	Copyright 2006-2020 The QElectroTech Team
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
#include "configdialog.h"
#include "configpages.h"
#include "qetdiagrameditor.h"
#include "qetelementeditor.h"
#include "elementscollectioncache.h"
#include "titleblocktemplate.h"
#include "qettemplateeditor.h"
#include "qetproject.h"
#include "qtextorientationspinboxwidget.h"
#include "recentfiles.h"
#include "qeticons.h"
#include "templatescollection.h"
#include "generalconfigurationpage.h"
#include "qetmessagebox.h"
#include "projectview.h"
#include "elementpicturefactory.h"
#include "aboutqetdialog.h"
#include "factory/elementfactory.h"

#include <cstdlib>
#include <iostream>
#define QUOTE(x) STRINGIFY(x)
#define STRINGIFY(x) #x
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <KAutoSaveFile>

#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
QString QETApp::common_elements_dir = QString();
#endif
#ifdef QET_ALLOW_OVERRIDE_CTBTD_OPTION
QString QETApp::common_tbt_dir_ = QString();
#endif
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
QString QETApp::config_dir = QString();
#endif
QString QETApp::lang_dir = QString();
TitleBlockTemplatesFilesCollection *QETApp::m_common_tbt_collection;
TitleBlockTemplatesFilesCollection *QETApp::m_custom_tbt_collection;
ElementsCollectionCache *QETApp::collections_cache_ = nullptr;
QMap<uint, QETProject *> QETApp::registered_projects_ = QMap<uint, QETProject *>();
uint QETApp::next_project_id = 0;
RecentFiles *QETApp::m_projects_recent_files = nullptr;
RecentFiles *QETApp::m_elements_recent_files = nullptr;
TitleBlockTemplate *QETApp::default_titleblock_template_ = nullptr;
QString QETApp::m_user_common_elements_dir = QString();
QString QETApp::m_user_custom_elements_dir = QString();
QString QETApp::m_user_custom_tbt_dir = QString();
QETApp *QETApp::m_qetapp = nullptr;


/**
	@brief QETApp::QETApp
*/
QETApp::QETApp() :
	m_splash_screen(nullptr),
	non_interactive_execution_(false)
{
	m_qetapp = this;
	parseArguments();
	if (non_interactive_execution_) {
		std::exit(EXIT_SUCCESS);
	}
	initConfiguration();
	initLanguage();
	QET::Icons::initIcons();
	initStyle();
	initSplashScreen();
	initSystemTray();

	connect(&signal_map, SIGNAL(mapped(QWidget *)),
		this, SLOT(invertMainWindowVisibility(QWidget *)));
	qApp->setQuitOnLastWindowClosed(false);
	connect(qApp, &QApplication::lastWindowClosed,
		this, &QETApp::checkRemainingWindows);

	setSplashScreenStep(
		tr("Chargement... Initialisation du cache des collections d'éléments",
		   "splash screen caption"));
	if (!collections_cache_) {
	QString cache_path = QETApp::configDir() + "/elements_cache.sqlite";

		collections_cache_ = new ElementsCollectionCache(cache_path, this);
		collections_cache_->setLocale(langFromSetting());
	}

	if (qet_arguments_.files().isEmpty())
	{
		setSplashScreenStep(tr("Chargement... Éditeur de schéma",
					   "splash screen caption"));
		new QETDiagramEditor();
	} else
	{
		setSplashScreenStep(tr("Chargement... Ouverture des fichiers",
					   "splash screen caption"));
		openFiles(qet_arguments_);
	}

	buildSystemTrayMenu();
	if (m_splash_screen) {
		m_splash_screen -> hide();
	}

	checkBackupFiles();
}

/**
	@brief QETApp::~QETApp
*/
QETApp::~QETApp()
{
	m_elements_recent_files->save();
	m_projects_recent_files->save();

	delete m_splash_screen;
	delete m_elements_recent_files;
	delete m_projects_recent_files;
	delete m_qsti;

	if (m_custom_tbt_collection)
		delete m_custom_tbt_collection;
	if (m_common_tbt_collection)
		delete m_common_tbt_collection;

	ElementFactory::dropInstance();
	ElementPictureFactory::dropInstance();
}


/**
	@brief QETApp::instance
	\~ @return the instance of the QETApp
	\~French l'instance de la QETApp
*/
QETApp *QETApp::instance()
{
	return m_qetapp;
}

/**
	@brief QETApp::setLanguage
	Change the language used by the application.
	\~French Change le langage utilise par l'application.
	\~ @param desired_language :
	wanted language
	\~French langage voulu
*/
void QETApp::setLanguage(const QString &desired_language) {
	QString languages_path = languagesPath();

	// load Qt library translations
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)	// ### Qt 6: remove
	QString qt_l10n_path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#else
#if TODO_LIST
#pragma message("@TODO remove code for QT 6 or later")
#endif
	QString qt_l10n_path = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#endif
	if (!qtTranslator.load("qt_" + desired_language, qt_l10n_path))
	{
		if(!qtTranslator.load("qt_" + desired_language, languages_path))
			qWarning()
					<<"failed to load"
					<<"qt_" + desired_language
					<<languages_path;
	}
	qApp->installTranslator(&qtTranslator);

	// load translations for the QET application
	// charge les traductions pour l'application QET
	if (!qetTranslator.load("qet_" + desired_language, languages_path)) {
		/* in case of failure,
		 *  we fall back on the native channels for French
		 * en cas d'echec,
		 *  on retombe sur les chaines natives pour le francais
		 */
		if (desired_language != "fr") {
			// use of the English version by default
			// utilisation de la version anglaise par defaut
			if(!qetTranslator.load("qet_en", languages_path))
				qWarning()
						<<"failed to load"
						<<"qet_en"
						<<languages_path;
		}
	}
	qApp->installTranslator(&qetTranslator);

	QString ltr_special_string = tr(
		"LTR",
		"Translate this string to RTL if you are translating \
		 to a Right-to-Left language, else translate to LTR"
	);
	if (ltr_special_string == "RTL") switchLayout(Qt::RightToLeft);
}

/**
	@brief QETApp::langFromSetting
	@return the langage found in setting file
	if nothing was found return the system local.
*/
QString QETApp::langFromSetting()
{
	QSettings settings;
	QString system_language = settings.value("lang", "system").toString();
	if(system_language == "system") {
		system_language = QLocale::system().name().left(2);
	}
	return system_language;
}
/**
	Switches the application to the provided layout.
*/
void QETApp::switchLayout(Qt::LayoutDirection direction) {
	qApp->setLayoutDirection(direction);
}

/**
	@brief QETApp::systray
	Manages QSystemTrayIcon related events
	\~French Gere les evenements relatifs au QSystemTrayIcon
	\~ @param reason :
	reason an integer representing the event on the systray
	\~French reason un entier representant l'evenement survenu sur le systray
*/
void QETApp::systray(QSystemTrayIcon::ActivationReason reason) {
	if (!QSystemTrayIcon::isSystemTrayAvailable()) return;
	switch(reason) {
		case QSystemTrayIcon::Context:
			// menu display
			// affichage du menu
			buildSystemTrayMenu();
			m_qsti -> contextMenu() -> show();
			break;
		case QSystemTrayIcon::DoubleClick:
		case QSystemTrayIcon::Trigger:
			// reduce or restore the application
			// reduction ou restauration de l'application
			fetchWindowStats(
						diagramEditors(),
						elementEditors(),
						titleBlockTemplateEditors());
			if (every_editor_reduced)
				restoreEveryEditor();
			else
				reduceEveryEditor();
			break;
		case QSystemTrayIcon::Unknown:
		default:
			// do nothing
			// ne rien faire
		break;
	}
}

/**
	@brief QETApp::reduceEveryEditor
	Minimizes all application windows in the systray
	\~French Reduit toutes les fenetres de l'application dans le systray
*/
void QETApp::reduceEveryEditor()
{
	reduceDiagramEditors();
	reduceElementEditors();
	reduceTitleBlockTemplateEditors();
	every_editor_reduced = true;
}

/**
	@brief QETApp::restoreEveryEditor
	Restores all application windows in the systray
	\~French Restaure toutes les fenetres de l'application dans le systray
*/
void QETApp::restoreEveryEditor()
{
	restoreDiagramEditors();
	restoreElementEditors();
	restoreTitleBlockTemplateEditors();
	every_editor_reduced = false;
}

/**
	@brief QETApp::reduceDiagramEditors
	Minimize all schema editors in the systray
	\~French Reduit tous les editeurs de schemas dans le systray
*/
void QETApp::reduceDiagramEditors()
{
	setMainWindowsVisible<QETDiagramEditor>(false);
}

/**
	@brief QETApp::restoreDiagramEditors
	Restore all schema editors in the systray
	\~French Restaure tous les editeurs de schemas dans le systray
*/
void QETApp::restoreDiagramEditors()
{
	setMainWindowsVisible<QETDiagramEditor>(true);
}


/**
	@brief QETApp::reduceElementEditors
	Minimize all element editors in systray
	\~French Reduit tous les editeurs d'element dans le systray
*/
void QETApp::reduceElementEditors()
{
	setMainWindowsVisible<QETElementEditor>(false);
}

/**
	@brief QETApp::restoreElementEditors
	Restore all element editors in the systray
	\~French Restaure tous les editeurs d'element dans le systray
*/
void QETApp::restoreElementEditors()
{
	setMainWindowsVisible<QETElementEditor>(true);
}

/**
	@brief QETApp::reduceTitleBlockTemplateEditors
	Reduce all known template editors
*/
void QETApp::reduceTitleBlockTemplateEditors()
{
	setMainWindowsVisible<QETTitleBlockTemplateEditor>(false);
}

/**
	@brief QETApp::restoreTitleBlockTemplateEditors
	Restore all known template editors
*/
void QETApp::restoreTitleBlockTemplateEditors()
{
	setMainWindowsVisible<QETTitleBlockTemplateEditor>(true);
}

/**
	@brief QETApp::newDiagramEditor
	launches a new schema editor
	\~French lance un nouvel editeur de schemas
*/
void QETApp::newDiagramEditor()
{
	new QETDiagramEditor();
}

/**
	@brief QETApp::newElementEditor
	launches a new element editor
	\~French lance un nouvel editeur d'element
*/
void QETApp::newElementEditor()
{
	new QETElementEditor();
}

/**
	@brief QETApp::collectionCache
	@return the collection cache provided by the application itself.
*/
ElementsCollectionCache *QETApp::collectionCache()
{
	return(collections_cache_);
}

/**
	@brief QETApp::commonTitleBlockTemplatesCollection
	@return the common title block templates collection,
	i.e. the one provided by QElecrotTech
*/
TitleBlockTemplatesFilesCollection *QETApp::commonTitleBlockTemplatesCollection()
{
	if (!m_common_tbt_collection) {
		m_common_tbt_collection =
				new TitleBlockTemplatesFilesCollection(
					QETApp::commonTitleBlockTemplatesDir());
		m_common_tbt_collection -> setTitle(
					tr("Cartouches QET",
					   "title of the title block templates \
					collection provided by QElectroTech"));
		m_common_tbt_collection -> setProtocol(QETAPP_COMMON_TBT_PROTOCOL);
		m_common_tbt_collection -> setCollection(QET::QetCollection::Common);
	}
	return(m_common_tbt_collection);
}

/**
	@brief QETApp::customTitleBlockTemplatesCollection
	@return the custom title block templates collection,
	i.e. the one managed by the end user
*/
TitleBlockTemplatesFilesCollection *QETApp::customTitleBlockTemplatesCollection()
{
	if (!m_custom_tbt_collection) {
		m_custom_tbt_collection =
				new TitleBlockTemplatesFilesCollection(
					QETApp::customTitleBlockTemplatesDir());
		m_custom_tbt_collection -> setTitle(tr("Cartouches utilisateur",
							   "title of the user's \
					title block templates collection"));
		m_custom_tbt_collection -> setProtocol(QETAPP_CUSTOM_TBT_PROTOCOL);
		m_custom_tbt_collection -> setCollection(QET::QetCollection::Custom);
	}
	return(m_custom_tbt_collection);
}

/**
	@brief QETApp::availableTitleBlockTemplatesCollections
	@return the list of all available title block tempaltes collections,
	beginning with the common and custom ones, plus the projects-embedded ones.
*/
QList<TitleBlockTemplatesCollection *> QETApp::availableTitleBlockTemplatesCollections()
{
	QList<TitleBlockTemplatesCollection *> collections_list;

	collections_list << commonTitleBlockTemplatesCollection();
	collections_list << customTitleBlockTemplatesCollection();

	foreach(QETProject *opened_project, registered_projects_) {
		collections_list << opened_project -> embeddedTitleBlockTemplatesCollection();
	}

	return(collections_list);
}

/**
	@brief QETApp::titleBlockTemplatesCollection
	@param protocol Protocol string
	@return the templates collection matching the provided protocol,
	or 0 if none could be found
*/
TitleBlockTemplatesCollection *QETApp::titleBlockTemplatesCollection(
		const QString &protocol) {
	if (protocol == QETAPP_COMMON_TBT_PROTOCOL) {
		return(m_common_tbt_collection);
	} else if (protocol == QETAPP_CUSTOM_TBT_PROTOCOL) {
		return(m_custom_tbt_collection);
	} else {
		QETProject *project = QETApp::projectFromString(protocol);
		if (project) {
			return(project -> embeddedTitleBlockTemplatesCollection());
		}
	}
	return(nullptr);
}

/**
	@brief QETApp::commonElementsDir
	@return the dir path of the common elements collection.
*/
QString QETApp::commonElementsDir()
{
	if (m_user_common_elements_dir.isEmpty())
	{
		QSettings settings;
		QString path = settings.value(
					"elements-collections/common-collection-path",
					"default").toString();
		if (path != "default" && !path.isEmpty())
		{
			QDir dir(path);
			if (dir.exists())
			{
				m_user_common_elements_dir = path;
				return m_user_common_elements_dir;
			}
		}
		else {
			m_user_common_elements_dir = "default";
		}
	}
	else if (m_user_common_elements_dir != "default") {
		return m_user_common_elements_dir;
	}

#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
	if (common_elements_dir != QString()) return(common_elements_dir);
#endif
#ifndef QET_COMMON_COLLECTION_PATH
	/* in the absence of a compilation option,
	 *  we use the elements folder, located next to the executable binary
	 * en l'absence d'option de compilation,
	 *  on utilise le dossier elements, situe a cote du binaire executable
	 */
	return(QCoreApplication::applicationDirPath() + "/elements/");
#else
	#ifndef QET_COMMON_COLLECTION_PATH_RELATIVE_TO_BINARY_PATH
		/* the compilation option represents a classic absolute
		 *  or relative path
		 * l'option de compilation represente un chemin absolu
		 *  ou relatif classique
		 */
		return(QUOTE(QET_COMMON_COLLECTION_PATH));
	#else
		/* the compilation option represents a path
		 *  relative to the folder containing the executable binary
		 * l'option de compilation represente un chemin
		 *  relatif au dossier contenant le binaire executable
		 */
		return(QCoreApplication::applicationDirPath()
			   + "/" + QUOTE(QET_COMMON_COLLECTION_PATH));
	#endif
#endif
}

/**
	@brief QETApp::customElementsDir
	@return the dir path of user elements collection ened by a "/" separator
*/
QString QETApp::customElementsDir()
{
	if (m_user_custom_elements_dir.isEmpty())
	{
		QSettings settings;
		QString path = settings.value(
					"elements-collections/custom-collection-path",
					"default").toString();
		if (path != "default" && !path.isEmpty())
		{
			QDir dir(path);
			if (dir.exists())
				{
					m_user_custom_elements_dir = path;
					if(!m_user_custom_elements_dir.endsWith("/")) {
						m_user_custom_elements_dir.append("/");
					}
					return m_user_custom_elements_dir;
			}
		}
		else {
			m_user_custom_elements_dir = "default";
		}
	}
	else if (m_user_custom_elements_dir != "default") {
		return m_user_custom_elements_dir;
	}

	return(configDir() + "elements/");
}

/**
	@brief QETApp::commonElementsDirN
	like QString QETApp::commonElementsDir but without "/" at the end
	@return QString path
*/
QString QETApp::commonElementsDirN()
{
	QString path = commonElementsDir();
	if (path.endsWith("/")) path.remove(path.length()-1, 1);
	return path;
}

/**
	@brief QETApp::customElementsDirN
	like QString QETApp::customElementsDir but without "/" at the end
	@return QString path
*/
QString QETApp::customElementsDirN()
{
	QString path = customElementsDir();
	if (path.endsWith("/")) path.remove(path.length()-1, 1);
	return path;
}

/**
	@brief QETApp::resetUserElementsDir
	Reset the path of the user common and custom elements dir.
	Use this function when the user path (common and/or custom) change.
*/
void QETApp::resetUserElementsDir()
{
	m_user_common_elements_dir.clear();
	m_user_custom_elements_dir.clear();
	m_user_custom_tbt_dir.clear();
}

/**
	@brief QETApp::commonTitleBlockTemplatesDir
	@return the path of the directory containing the common title block
	templates collection.
*/
QString QETApp::commonTitleBlockTemplatesDir()
{
#ifdef QET_ALLOW_OVERRIDE_CTBTD_OPTION
	if (common_tbt_dir_ != QString()) return(common_tbt_dir_);
#endif
#ifndef QET_COMMON_TBT_PATH
	/* without any compile-time option,
	 *  use the "titleblocks" directory next to the executable binary
	 */
	return(QCoreApplication::applicationDirPath() + "/titleblocks/");
#else
	#ifndef QET_COMMON_COLLECTION_PATH_RELATIVE_TO_BINARY_PATH
		// the compile-time option represents a usual path
		// (be it absolute or relative)
		return(QUOTE(QET_COMMON_TBT_PATH));
	#else
		/* the compile-time option represents a path relative
		 * to the directory that contains the executable binary
		 */
		return(QCoreApplication::applicationDirPath()
			   + "/" + QUOTE(QET_COMMON_TBT_PATH));
	#endif
#endif
}

/**
	@brief QETApp::customTitleBlockTemplatesDir
	@return the path of the directory containing the custom title block
	templates collection.
*/
QString QETApp::customTitleBlockTemplatesDir()
{
		if (m_user_custom_tbt_dir.isEmpty())
	{
			QSettings settings;
			QString path = settings.value(
						"elements-collections/custom-tbt-path",
						"default").toString();
			if (path != "default" && !path.isEmpty())
			{
				QDir dir(path);
				if (dir.exists())
				{
					m_user_custom_tbt_dir = path;
					return m_user_custom_tbt_dir;
				}
			}
		else {
			m_user_custom_tbt_dir = "default";
		}
	}
	else if (m_user_custom_tbt_dir != "default") {
		return m_user_custom_tbt_dir;
	}

	return(configDir() + "titleblocks/");
}

/**
	@brief QETApp::configDir
	Return the QET configuration folder, i.e. the path to the folder in
	which QET will read configuration and customization information
	specific to the current user. This file is generally
	C:\\Documents And Settings\\user\\Application Data\ qet
	on Windows and
	~/.qet
	under UNIX-like systems.
	\~French Renvoie le dossier de configuration de QET,
	c-a-d le chemin du dossier dans lequel QET lira les informations
	de configuration et de personnalisation propres a l'utilisateur courant.
	Ce dossier est generalement
	C:\\Documents And Settings\\utilisateur\\Application Data\\qet
	sous Windows et
	~/.qet
	sous les systemes type UNIX.
	\~ @return The path of the QElectroTech configuration folder
	\~French Le chemin du dossier de configuration de QElectroTech
*/
QString QETApp::configDir()
{
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
	if (config_dir != QString()) return(config_dir);
#endif
#ifdef Q_OS_WIN32
	// recupere l'emplacement du dossier Application Data
	// char *app_data_env = getenv("APPDATA");
	// QString app_data_str(app_data_env);
	QProcess * process = new QProcess();
	QString app_data_str = (process->processEnvironment()).value("APPDATA");
	// delete app_data_env;
	delete process;
	if (app_data_str.isEmpty()) {
		app_data_str = QDir::homePath() + "/Application Data";
	}
	return(app_data_str + "/qet/");
#else
	return(QDir::homePath() + "/.qet/");
#endif
}

/**
	@brief QETApp::realPath
	Allows you to know the absolute path of the * .elmt file
	corresponding to a symbolic path
	(of the type custom: // tools_pervers / sado_maso / contact_bizarre)
	\~French Permet de connaitre le chemin absolu du fichier *.elmt
	correspondant a un chemin symbolique
	(du type custom://outils_pervers/sado_maso/contact_bizarre)
	\~ @param sym_path
	Character string representing the absolute path of the file
	\~French Chaine de caracteres representant le chemin absolu du fichier
	\~ @return An empty string in case of error or the absolute path
	of the * .elmt file.
	\~French Une chaine de caracteres vide en cas d'erreur ou
	le chemin absolu du fichier *.elmt.
*/
QString QETApp::realPath(const QString &sym_path) {
	QString directory;
	if (sym_path.startsWith("common://")) {
		directory = commonElementsDir();
	} else if (sym_path.startsWith("custom://")) {
		directory = customElementsDir();
	} else if (sym_path.startsWith(QETAPP_COMMON_TBT_PROTOCOL "://")) {
		directory = commonTitleBlockTemplatesDir();
	} else if (sym_path.startsWith(QETAPP_CUSTOM_TBT_PROTOCOL "://")) {
		directory = customTitleBlockTemplatesDir();
	} else return(QString());
	return(directory
		   + QDir::toNativeSeparators(sym_path.right(sym_path.length() - 9)));
}


/**
	@brief QETApp::symbolicPath
	Build the symbolic path
	(of the type custom: // tools_pervers / sado_maso / contact_bizarre)
	corresponding to a file.
	\~French Construit le chemin symbolique
	(du type custom://outils_pervers/sado_maso/ contact_bizarre)
	correspondant a un fichier.
	\~ @param real_path :
	Character string representing the symbolic path of the file
	\~French Chaine de caracteres representant le chemin symbolique du fichier
	\~ @return An empty string in case of error
	or the path symbolic designating the element.
	\~French Une chaine de caracteres vide en cas d'erreur ou le chemin
	symbolique designant l'element.
*/
QString QETApp::symbolicPath(const QString &real_path) {
	// get the common and custom folders
	// recupere les dossier common et custom
	QString commond = commonElementsDir();
	QString customd = customElementsDir();
	QString chemin;
	// analyzes the file path passed in parameter
	// analyse le chemin de fichier passe en parametre
	if (real_path.startsWith(commond)) {
		chemin = "common://"
				+ real_path.right(
					real_path.length() - commond.length());
	} else if (real_path.startsWith(customd)) {
		chemin = "custom://"
				+ real_path.right(
					real_path.length() - customd.length());
	} else chemin = QString();
	return(chemin);
}

/**
	@brief QETApp::handledFileExtensions
	@return the list of file extensions QElectroTech is able to open and
	supposed to handle. Note they are provided with no leading point.
*/
QStringList QETApp::handledFileExtensions()
{
	static QStringList ext;
	if (!ext.count()) {
		ext << "qet";
		ext << "elmt";
		ext << QString(TITLEBLOCKS_FILE_EXTENSION).remove(QRegularExpression("^\\."));
	}
	return(ext);
}

/**
	@brief QETApp::handledFiles
	@param urls : URLs list
	@return the list of filepaths QElectroTech is able to open.
*/
QStringList QETApp::handledFiles(const QList<QUrl> &urls) {
	QList<QString> filepaths;
	foreach (QUrl url, urls) {
		if (url.scheme() != "file") continue;
		QString local_path = url.toLocalFile();
		QFileInfo local_path_info(local_path);
		QString local_path_ext = local_path_info.completeSuffix();
		if (handledFileExtensions().contains(local_path_ext)) {
			filepaths << local_path;
		}
	}
	return(filepaths);
}

/**
	@brief QETApp::diagramEditorForFile
	\~French
	\~ @param filepath : A file path
	\~French Un chemin de fichier
	\~ @note if filepath is an empty string, this method returns 0.
	\~French si filepath est une chaine vide, cette methode retourne 0.
	\~ @return the QETDiagramEditor editing the filepath file,
	or 0 if this file is not edited by the application.
	\~French le QETDiagramEditor editant le fichier filepath,
	ou 0 si ce fichier n'est pas edite par l'application.
*/
QETDiagramEditor *QETApp::diagramEditorForFile(const QString &filepath) {
	if (filepath.isEmpty()) return(nullptr);

	QETApp *qet_app(QETApp::instance());
	foreach (QETDiagramEditor *diagram_editor, qet_app -> diagramEditors())
	{
		if (diagram_editor -> viewForFile(filepath)) {
			return(diagram_editor);
		}
	}

	return(nullptr);
}

/**
	@brief QETApp::diagramEditorAncestorOf
	@param child
	@return the parent QETDiagramEditor
	(or grandparent and so on to any level) of the given child.
	If not return nullptr;
*/
QETDiagramEditor *QETApp::diagramEditorAncestorOf (const QWidget *child)
{
	foreach (QETDiagramEditor *qde, QETApp::diagramEditors()) {
		if (qde->isAncestorOf(child)) {
			return qde;
		}
	}

	return nullptr;
}

#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
/**
	@brief QETApp::overrideCommonElementsDir
	Redefines the path of the common elements folder
	\~French Redefinit le chemin du dossier des elements communs
	\~ @param new_ced :
	New path of the common elements folder
	\~French Nouveau chemin du dossier des elements communs
*/
void QETApp::overrideCommonElementsDir(const QString &new_ced) {
	QFileInfo new_ced_info(new_ced);
	if (new_ced_info.isDir())
	{
		common_elements_dir = new_ced_info.absoluteFilePath();
		if (!common_elements_dir.endsWith("/"))
			common_elements_dir += "/";
	}
}
#endif

#ifdef QET_ALLOW_OVERRIDE_CTBTD_OPTION
/**
	@brief QETApp::overrideCommonTitleBlockTemplatesDir
	Define the path of the directory containing the common title block
	tempaltes collection.
	@param new_ctbtd
*/
void QETApp::overrideCommonTitleBlockTemplatesDir(const QString &new_ctbtd) {
	QFileInfo new_ctbtd_info(new_ctbtd);
	if (new_ctbtd_info.isDir()) {
		common_tbt_dir_ = new_ctbtd_info.absoluteFilePath();
		if (!common_tbt_dir_.endsWith("/")) common_tbt_dir_ += "/";
	}
}
#endif

#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
/**
	@brief QETApp::overrideConfigDir
	Redefines the path of the configuration folder
	\~French Redefinit le chemin du dossier de configuration
	\~ @param new_cd :
	New path to configuration folder
	\~French Nouveau chemin du dossier de configuration
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
	@brief QETApp::overrideLangDir
	Redefines the path of the folder containing the language files
	\~French Redefinit le chemin du dossier contenant les fichiers de langue
	\~ @param new_ld :
	New path of the folder containing the language files
	\~French Nouveau chemin du dossier contenant les fichiers de langue
*/
void QETApp::overrideLangDir(const QString &new_ld) {
	QFileInfo new_ld_info(new_ld);
	if (new_ld_info.isDir()) {
		lang_dir = new_ld_info.absoluteFilePath();
		if (!lang_dir.endsWith("/")) lang_dir += "/";
	}
}

/**
	@brief QETApp::languagesPath
	@return The path of the folder containing the language files
	\~French Le chemin du dossier contenant les fichiers de langue
*/
QString QETApp::languagesPath()
{
	if (!lang_dir.isEmpty()) {
		return(lang_dir);
	} else {
#ifndef QET_LANG_PATH
	/* in the absence of a compilation option, we use the lang folder,
	 *  located next to the executable binary
	 * en l'absence d'option de compilation, on utilise le dossier lang,
	 *  situe a cote du binaire executable
	 */
	return(QCoreApplication::applicationDirPath() + "/lang/");
#else
	#ifndef QET_LANG_PATH_RELATIVE_TO_BINARY_PATH
		/* the compilation option represents
		 *  a classic absolute or relative path
		 * l'option de compilation represente
		 *  un chemin absolu ou relatif classique
		 */
		return(QUOTE(QET_LANG_PATH));
	#else
		/* the compilation option represents a path relative
		 *  to the folder containing the executable binary
		 * l'option de compilation represente un chemin relatif
		 *  au dossier contenant le binaire executable
		 */
		return(QCoreApplication::applicationDirPath()
			   + "/" + QUOTE(QET_LANG_PATH));
	#endif
#endif
	}
}

/**
	@brief QETApp::closeEveryEditor
	Close all editors
	\~French Ferme tous les editeurs
	\~ @return true if the user has accepted all closures, false otherwise
	\~French true si l'utilisateur a accepte toutes les fermetures,
	false sinon
*/
bool QETApp::closeEveryEditor()
{
	// make sure all windows are visible before leaving
	// s'assure que toutes les fenetres soient visibles avant de quitter
	restoreEveryEditor();
	foreach(QETProject *project, registered_projects_) {
		project -> close();
	}
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
	@brief QETApp::diagramTextsFont
	The font to use
	By default the font is "sans Serif" and size 9.
	@param size : the size of font
	@return the font to use
*/
QFont QETApp::diagramTextsFont(qreal size)
{
	QSettings settings;

	//Font to use
	QString diagram_texts_family = settings.value("diagramfont",
							  "Sans Serif").toString();
	qreal diagram_texts_size     = settings.value("diagramsize",
							  9.0).toDouble();

	if (size != -1.0) {
		diagram_texts_size = size;
	}
	QFont diagram_texts_font = QFont(diagram_texts_family);
	diagram_texts_font.setPointSizeF(diagram_texts_size);
	if (diagram_texts_size <= 4.0) {
		diagram_texts_font.setWeight(QFont::Light);
	}
	return(diagram_texts_font);
}
/**
	@brief QETApp::diagramTextsItemFont
	the font for to use in independent text item
	@param size of font
	@return
*/
QFont QETApp::diagramTextsItemFont(qreal size)
{
	QSettings settings;

	//Font to use
	QString diagram_texts_item_family = settings.value("diagramitemfont",
							   "Sans Serif").toString();
	qreal diagram_texts_item_size     = settings.value("diagramitemsize",
							   9.0).toDouble();
	auto diagram_texts_item_weight =
			static_cast<QFont::Weight>(
				settings.value("diagramitemweight").toInt());
	QString diagram_texts_item_style  = settings.value("diagramitemstyle",
							   "normal").toString();

	if (size != -1.0) {
		diagram_texts_item_size = size;
	}
	QFont diagram_texts_item_font = QFont(diagram_texts_item_family);
	diagram_texts_item_font.setPointSizeF(diagram_texts_item_size);
	diagram_texts_item_font.setWeight(diagram_texts_item_weight);
	diagram_texts_item_font.setStyleName(diagram_texts_item_style);
	if (diagram_texts_item_size <= 4.0) {
		diagram_texts_item_font.setWeight(QFont::Light);
	}
	return(diagram_texts_item_font);
}
/**
	@brief QETApp::dynamicTextsFont
	the default font of dynamic element text item
	@param size
	@return dynamic text font with PointSizeF(size)
*/
 QFont QETApp::dynamicTextsItemFont(qreal size)
{
	QSettings settings;
	//Font to use
	QFont font_ = diagramTextsItemFont();
	if (settings.contains("diagrameditor/dynamic_text_font")) {
		font_.fromString(settings.value(
					 "diagrameditor/dynamic_text_font"
						).toString());
	}
	if (size > 0) {
		font_.setPointSizeF(size);
	}
	return(font_);
}



/**
	@brief QETApp::indiTextsItemFont
	The default font to use for independent text item
	@param size
	@return independent text font with PointSizeF(size)
*/
QFont QETApp::indiTextsItemFont(qreal size)
{
	QSettings settings;
	//Font to use
	QFont font_ = diagramTextsItemFont();
	if (settings.contains("diagrameditor/independent_text_font")) {
		font_.fromString(settings.value(
					 "diagrameditor/independent_text_font"
					 ).toString());
	}
	if (size > 0) {
		font_.setPointSizeF(size);
	}
	return(font_);
}

/**
	@brief QETApp::diagramEditors
	@return schema editors
	\~French les editeurs de schemas
*/
QList<QETDiagramEditor *> QETApp::diagramEditors()
{
	return(QETApp::instance() -> detectWindows<QETDiagramEditor>());
}

/**
	@brief QETApp::elementEditors
	@return element editors
	\~French les editeurs d'elements
*/
QList<QETElementEditor *> QETApp::elementEditors()
{
	return(QETApp::instance() -> detectWindows<QETElementEditor>());
}

/**
	@brief QETApp::titleBlockTemplateEditors
	@return the title block template editors
*/
QList<QETTitleBlockTemplateEditor *> QETApp::titleBlockTemplateEditors()
{
	return(QETApp::instance() -> detectWindows<QETTitleBlockTemplateEditor>());
}

/**
	@brief QETApp::titleBlockTemplateEditors
	@param project Opened project object.
	@return the list of title block template editors which are currently
	editing a template embedded within \a project.
*/
QList<QETTitleBlockTemplateEditor *> QETApp::titleBlockTemplateEditors(
		QETProject *project) {
	QList<QETTitleBlockTemplateEditor *> editors;
	if (!project) return(editors);

	// foreach known template editor
	foreach (QETTitleBlockTemplateEditor *tbt_editor,
		 titleBlockTemplateEditors()) {
		if (tbt_editor -> location().parentProject() == project) {
			editors << tbt_editor;
		}
	}

	return(editors);
}

/**
	@brief QETApp::createTextOrientationSpinBoxWidget
	Instantiates a QTextOrientationSpinBoxWidget and configures:
	  - his font
	  - his strings
	\~French Instancie un QTextOrientationSpinBoxWidget et configure :
	  - sa police de caracteres
	  - ses chaines de caracteres
	\~ @note that the deletion of the widget thus allocated is the
	responsibility of the caller.
	\~French que la suppression du widget ainsi alloue est a la charge
	de l'appelant.
	\~ @return
	\~ @see QTextOrientationSpinBoxWidget
*/
QTextOrientationSpinBoxWidget *QETApp::createTextOrientationSpinBoxWidget()
{
	QTextOrientationSpinBoxWidget *widget = new QTextOrientationSpinBoxWidget();
	widget -> orientationWidget() -> setFont(QETApp::diagramTextsFont());
	widget -> orientationWidget() -> setUsableTexts(QList<QString>()
		<< QETApp::tr("Q",
				  "Single-letter example text - translate length, not meaning")
		<< QETApp::tr("QET",
				  "Small example text - translate length, not meaning")
		<< QETApp::tr("Schema",
				  "Normal example text - translate length, not meaning")
		<< QETApp::tr("Electrique",
				  "Normal example text - translate length, not meaning")
		<< QETApp::tr("QElectroTech",
				  "Long example text - translate length, not meaning")
	);
	return(widget);
}

/**
	@brief QETApp::defaultTitleBlockTemplate
	@return the default titleblock template for diagrams
*/
TitleBlockTemplate *QETApp::defaultTitleBlockTemplate()
{
	if (!QETApp::default_titleblock_template_) {
		TitleBlockTemplate *titleblock_template = new TitleBlockTemplate(QETApp::instance());
		if (titleblock_template -> loadFromXmlFile(":/titleblocks/default.titleblock")) {
			QETApp::default_titleblock_template_ = titleblock_template;
		}
	}
	return(default_titleblock_template_);
}

/**
	@brief QETApp::elementEditors
	@param project : a project
	\~French un projet
	\~ @return element editors editing
	an element belonging to the QETProject project
	\~French les editeurs d'elements editant un element appartenant
	au projet project
*/
QList<QETElementEditor *> QETApp::elementEditors(QETProject *project) {
	QList<QETElementEditor *> editors;
	if (!project) return(editors);

	// for each element editor ...
	// pour chaque editeur d'element...
	foreach(QETElementEditor *elmt_editor, elementEditors()) {
		// we retrieve the location of the element it edits
		// on recupere l'emplacement de l'element qu'il edite
		ElementsLocation elmt_editor_loc(elmt_editor -> location());

		// the editor may edit an unsaved item or a file
		// il se peut que l'editeur edite un element non enregistre ou un fichier
		if (elmt_editor_loc.isNull()) continue;

		if (elmt_editor_loc.project() == project) {
			editors << elmt_editor;
		}
	}

	return(editors);
}

/**
	@brief QETApp::receiveMessage
	openFiles from message
	@param instanceId Q_UNUSED
	@param message
*/
void QETApp::receiveMessage(int instanceId, QByteArray message)
{
	Q_UNUSED(instanceId);

	QString str(message);

	if (str.startsWith("launched-with-args: "))
	{
		QString my_message(str.mid(20));
		QStringList args_list = QET::splitWithSpaces(my_message);
		openFiles(QETArguments(args_list));
	}
}

/**
	@param T a class inheriting QMainWindow
	@return the list of windows of type T
*/
template <class T> QList<T *> QETApp::detectWindows() const
{
	QList<T *> windows;
	foreach(QWidget *widget, qApp->topLevelWidgets()) {
		if (!widget -> isWindow()) continue;
		if (T *window = qobject_cast<T *>(widget)) {
			windows << window;
		}
	}
	return(windows);
}

/**
	T a class inheriting QMainWindow
	@param visible whether detected main windows should be visible
*/
template <class T> void QETApp::setMainWindowsVisible(bool visible) {
	foreach(T *e, detectWindows<T>()) {
		setMainWindowVisible(e, visible);
	}
}

/**
	@brief QETApp::projectsRecentFiles
	@return The list of recent files for projects
	\~French La liste des fichiers recents pour les projets
*/
RecentFiles *QETApp::projectsRecentFiles()
{
	return(m_projects_recent_files);
}

/**
	@brief QETApp::elementsRecentFiles
	@return The list of recent files for the elements
	\~French La liste des fichiers recents pour les elements
*/
RecentFiles *QETApp::elementsRecentFiles()
{
	return(m_elements_recent_files);
}

/**
	@brief QETApp::setMainWindowVisible
	Show or hide a window (schema editors / element editors)
	\~French Affiche ou cache une fenetre
	(editeurs de schemas / editeurs d'elements)
	\~ @param window : window to show / hide
	\~French fenetre a afficher / cacher
	\~ @param visible : true to display the window, false otherwise
	\~French true pour affiche la fenetre, false sinon
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
	@brief QETApp::invertMainWindowVisibility
	Display a window (schema editors / element editors)
	if this is hidden or hides it if it is displayed.
	\~French Affiche une fenetre (editeurs de schemas / editeurs d'elements)
	si celle-ci est cachee ou la cache si elle est affichee.
	\~ @param window : window to show / hide
	\~French fenetre a afficher / cacher
*/
void QETApp::invertMainWindowVisibility(QWidget *window) {
	if (QMainWindow *w = qobject_cast<QMainWindow *>(window))
		setMainWindowVisible(w, !w -> isVisible());
}

/**
	@brief QETApp::useSystemPalette
	Change the application palette
	\~French Change la palette de l'application
	\~ @param use :
	true to use the system colors, false to use those of the current theme
	\~French true pour utiliser les couleurs du systeme,
	false pour utiliser celles du theme en cours
*/
void QETApp::useSystemPalette(bool use) {
	if (use) {
		qApp->setPalette(initial_palette_);
		qApp->setStyleSheet(
				"QAbstractScrollArea#mdiarea {"
				"background-color -> setPalette(initial_palette_);"
				"}"
				);
	} else {
		QFile file(configDir() + "style.css");
		file.open(QFile::ReadOnly);
		QString styleSheet = QLatin1String(file.readAll());
		qApp->setStyleSheet(styleSheet);
		file.close();
	}
}

/**
	@brief QETApp::quitQET
	Request the closing of all windows;
	if the user accepts them, the application quits
	\~French Demande la fermeture de toutes les fenetres ;
	si l'utilisateur les accepte, l'application quitte
*/
void QETApp::quitQET()
{
#if TODO_LIST
#pragma message("@TODO Segmentation fault when closing program before loading elements is finished")
#endif
	if (closeEveryEditor()) {
		qApp->quit();
	}
}

/**
	@brief QETApp::checkRemainingWindows
	Check if there are any windows (hidden or not)
	and exit if there are no more.
	\~French Verifie s'il reste des fenetres (cachees ou non)
	et quitte s'il n'en reste plus.
*/
void QETApp::checkRemainingWindows()
{
	/* little hack:
	 *  the slot remembers after 500 ms of waiting in order to compensate
	 *  for the fact that some windows can still appear alive when they
	 *  have just been closed
	 * petite bidouille :
	 *  le slot se rappelle apres 500 ms d'attente afin de compenser
	 *  le fait que certaines fenetres peuvent encore paraitre vivantes
	 *  alors qu'elles viennent d'etre fermees
	*/
	static bool sleep = true;
	if (sleep) {
		QTimer::singleShot(500, this, SLOT(checkRemainingWindows()));
	} else {
		if (!diagramEditors().count() && !elementEditors().count()) {
			qApp->quit();
		}
	}
	sleep = !sleep;
}

/**
	@brief QETApp::openFiles
	Open files passed as arguments
	\~French Ouvre les fichiers passes en arguments
	\~ @param args :
	Object containing arguments; the files
	\~French Objet contenant des arguments ; les fichiers
	\~ @see openProjectFiles openElementFiles
*/
void QETApp::openFiles(const QETArguments &args) {
	openProjectFiles(args.projectFiles());
	openElementFiles(args.elementFiles());
	openTitleBlockTemplateFiles(args.titleBlockTemplateFiles());
}

/**
	@brief QETApp::openProjectFiles
	Opens a list of files.
	Files are opened in the first visible schematic editor.
	Otherwise, the first existing schema editor becomes visible and is uses.
	If there is no schematic editor open, a new one is created and uses.
	\~French Ouvre une liste de fichiers.
	Les fichiers sont ouverts dans le premier editeur de schemas visible
	venu. Sinon, le premier editeur de schemas existant venu devient
	visible et est utilise. S'il n'y a aucun editeur de schemas ouvert,
	un nouveau est cree et utilise.
	\~ @param files_list :
	Files to open
	\~French Fichiers a ouvrir
*/
void QETApp::openProjectFiles(const QStringList &files_list) {
	if (files_list.isEmpty()) return;

	// list of open schema editors
	// liste des editeurs de schema ouverts
	QList<QETDiagramEditor *> diagrams_editors = diagramEditors();

	// if there are open diagram editors, we look for those that are visible
	// s'il y a des editeur de schemas ouvert, on cherche ceux qui sont visibles
	if (diagrams_editors.count()) {
		QList<QETDiagramEditor *> visible_diagrams_editors;
		foreach(QETDiagramEditor *de, diagrams_editors) {
			if (de -> isVisible()) visible_diagrams_editors << de;
		}

		// we choose either the first visible or the very first
		// on choisit soit le premier visible soit le premier tout court
		QETDiagramEditor *de_open;
		if (visible_diagrams_editors.count()) {
			de_open = visible_diagrams_editors.first();
		} else {
			de_open = diagrams_editors.first();
			de_open -> setVisible(true);
		}

		// opens the files in the editor thus chosen
		// ouvre les fichiers dans l'editeur ainsi choisi
		foreach(QString file, files_list) {
			de_open -> openAndAddProject(file);
		}
	} else {
		// create a new editor that will open the files
		// cree un nouvel editeur qui ouvrira les fichiers
		new QETDiagramEditor(files_list);
	}
}

/**
	@brief QETApp::openElementFiles
	Open the elements files passed in parameter.
	If an element is already open, the window which edits it is activated.
	\~French Ouvre les fichiers elements passes en parametre.
	Si un element est deja ouvert, la fenetre qui l'edite est activee.
	\~ @param files_list : Files to open
	\~French Fichiers a ouvrir
*/
void QETApp::openElementFiles(const QStringList &files_list) {
	if (files_list.isEmpty()) return;

	// avoid as much as possible duplicates in the list provided
	// evite autant que possible les doublons dans la liste fournie
	QSet<QString> files_set;
	foreach(QString file, files_list) {
		QString canonical_filepath = QFileInfo(file).canonicalFilePath();
		if (!canonical_filepath.isEmpty()) files_set << canonical_filepath;
	}
	// at this stage, all the files in the Set exist and are a priori different
	// a ce stade, tous les fichiers dans le Set existent et sont a priori differents
	if (files_set.isEmpty()) return;

	// list of open item editors
	// liste des editeurs d'element ouverts
	QList<QETElementEditor *> element_editors = elementEditors();

	// we process the files one after the other ...
	// on traite les fichiers a la queue leu leu...
	foreach(QString element_file, files_set) {
		bool already_opened_in_existing_element_editor = false;
		foreach(QETElementEditor *element_editor, element_editors) {
			if (element_editor -> isEditing(element_file)) {
				// this file is already open in an editor
				// ce fichier est deja ouvert dans un editeur
				already_opened_in_existing_element_editor = true;
				element_editor -> setVisible(true);
				element_editor -> raise();
				element_editor -> activateWindow();
				break;
			}
		}
		if (!already_opened_in_existing_element_editor) {
			// this file is not open in any editor
			// ce fichier n'est ouvert dans aucun editeur
			QETElementEditor *element_editor = new QETElementEditor();
			element_editor -> fromFile(element_file);
		}
	}
}

/**
	@brief QETApp::openElementLocations
	Open the elements whose location is passed in parameter. If an element
	is already open, the window which is edited is active.
	\~French Ouvre les elements dont l'emplacement est passe en parametre.
	Si un element est deja ouvert, la fentre qui l'edite est activee.
	\~ @param locations_list : Locations to open
	\~French Emplacements a ouvrir
*/
void QETApp::openElementLocations(const QList<ElementsLocation> &locations_list) {
	if (locations_list.isEmpty()) return;

	// list of open item editors
	// liste des editeurs d'element ouverts
	QList<QETElementEditor *> element_editors = elementEditors();

	// we treat the sites in a single file ...
	// on traite les emplacements  a la queue leu leu...
	foreach(ElementsLocation element_location, locations_list) {
		bool already_opened_in_existing_element_editor = false;
		foreach(QETElementEditor *element_editor, element_editors) {
			if (element_editor -> isEditing(element_location)) {
				// this location is already open in an editor
				// cet emplacement est deja ouvert dans un editeur
				already_opened_in_existing_element_editor = true;
				element_editor -> setVisible(true);
				element_editor -> raise();
				element_editor -> activateWindow();
				break;
			}
		}
		if (!already_opened_in_existing_element_editor) {
			// this location is not open in any editor
			// cet emplacement n'est ouvert dans aucun editeur
			QETElementEditor *element_editor = new QETElementEditor();
			element_editor -> fromLocation(element_location);
		}
	}
}

/**
	@brief QETApp::openTitleBlockTemplate
	Launch a new title block template editor to edit the given template
	@param location location of the title block template to be edited

	@param duplicate if true, the template is opened for duplication,
	which means the user will be prompter for a new template name.
	@see QETTitleBlockTemplateEditor::setOpenForDuplication()
*/
void QETApp::openTitleBlockTemplate(const TitleBlockTemplateLocation &location,
					bool duplicate) {
	QETTitleBlockTemplateEditor *qet_template_editor = new QETTitleBlockTemplateEditor();
	qet_template_editor -> setOpenForDuplication(duplicate);
	qet_template_editor -> edit(location);
	qet_template_editor -> show();
}

/**
	@brief QETApp::openTitleBlockTemplate
	Launch a new title block template editor to edit the given template
	@param filepath Path of the .titleblock file to be opened
*/
void QETApp::openTitleBlockTemplate(const QString &filepath) {
	QETTitleBlockTemplateEditor *qet_template_editor = new QETTitleBlockTemplateEditor();
	qet_template_editor -> edit(filepath);
	qet_template_editor -> show();
}

/**
	@brief QETApp::openTitleBlockTemplateFiles
	Open provided title block template files.
	If a title block template is already opened,
	the adequate window is activated.
	@param files_list Files to be opened
*/
void QETApp::openTitleBlockTemplateFiles(const QStringList &files_list) {
	if (files_list.isEmpty()) return;

	// avoid duplicates in the provided files list
	QSet<QString> files_set;
	foreach (QString file, files_list) {
		QString canonical_filepath = QFileInfo(file).canonicalFilePath();
		if (!canonical_filepath.isEmpty()) files_set << canonical_filepath;
	}
	// here, we can assume all files in the set exist and are different
	if (files_set.isEmpty()) return;

	// opened title block template editors
	QList<QETTitleBlockTemplateEditor *> tbt_editors = titleBlockTemplateEditors();

	foreach(QString tbt_file, files_set) {
		bool already_opened_in_existing_tbt_editor = false;
		foreach(QETTitleBlockTemplateEditor *tbt_editor, tbt_editors) {
			if (tbt_editor -> isEditing(tbt_file)) {
				// this file is already opened
				already_opened_in_existing_tbt_editor = true;
				tbt_editor -> setVisible(true);
				tbt_editor -> raise();
				tbt_editor -> activateWindow();
				break;
			}
		}
		if (!already_opened_in_existing_tbt_editor) {
			// this file is not opened yet
			openTitleBlockTemplate(tbt_file);
		}
	}
}

/**
	@brief QETApp::configureQET
	Allows the user to configure QET by launching an appropriate dialog.
	\~French Permet a l'utilisateur de configurer QET
	en lancant un dialogue approprie.
	\~ @see ConfigDialog
*/
void QETApp::configureQET()
{
	// determine the parent widget to use for the dialog
	// determine le widget parent a utiliser pour le dialogue
	QWidget *parent_widget = qApp->activeWindow();

	// create dialogue
	// cree le dialogue
	ConfigDialog cd;
	cd.setWindowTitle(tr("Configurer QElectroTech", "window title"));
	cd.setWindowModality(Qt::WindowModal);
	cd.addPage(new GeneralConfigurationPage());
	cd.addPage(new NewDiagramPage());
	cd.addPage(new ExportConfigPage());
	cd.addPage(new PrintConfigPage());

	// associates the dialog with a possible parent widget
	// associe le dialogue a un eventuel widget parent
	if (parent_widget) {
		cd.setParent(parent_widget, cd.windowFlags());
		cd.setMaximumSize(parent_widget->size());
	}

	// display the dialog then avoid linking it to any parent widget
	// affiche le dialogue puis evite de le lier a un quelconque widget parent
	cd.exec();
	cd.setParent(nullptr, cd.windowFlags());
}

/**
	@brief QETApp::aboutQET
	Open the dialog about qet.
*/
void QETApp::aboutQET()
{
	AboutQETDialog aqd(qApp->activeWindow());
#ifdef Q_OS_MACOS
	aqd.setWindowFlags(Qt::Sheet);
#endif
	aqd.exec();
}

/**
	@brief QETApp::floatingToolbarsAndDocksForMainWindow
	\~French
	\~ @param window :
	window where you have to find the toolbars and floating dock
	\~French fenetre dont il faut trouver les barres d'outils et dock flottants
	\~ @return the window's toolbars and floating dock
	\~French les barres d'outils et dock flottants de la fenetre
*/
QList<QWidget *> QETApp::floatingToolbarsAndDocksForMainWindow(
		QMainWindow *window) const
{
	QList<QWidget *> widgets;
	foreach(QWidget *qw, qApp->topLevelWidgets()) {
		if (!qw -> isWindow()) continue;
		if (qobject_cast<QToolBar *>(qw)
			|| qobject_cast<QDockWidget *>(qw)) {
			if (qw -> parent() == window) widgets << qw;
		}
	}
	return(widgets);
}


/**
	@brief QETApp::parseArguments
	Parse the following arguments:
	  - --common-elements-dir=
	  - --config-dir
	  - --help
	  - --version
	  - -v
	  - --license

	The other arguments are normally file paths.
	If they exist, they are just stored in the arguments_files_ attribute.
	Otherwise, they are stored in the arguments_options_ attribute.
	\~French
	Parse les arguments suivants :
	  - --common-elements-dir=
	  - --config-dir
	  - --help
	  - --version
	  - -v
	  - --license

	Les autres arguments sont normalement des chemins de fichiers.
	S'ils existent, ils sont juste memorises dans l'attribut arguments_files_.
	Sinon, ils sont memorises dans l'attribut arguments_options_.
*/
void QETApp::parseArguments()
{
	// get the arguments
	// recupere les arguments
	QList<QString> arguments_list(qApp->arguments());

	// remove the first argument: this is the binary file
	// enleve le premier argument : il s'agit du fichier binaire
	arguments_list.takeFirst();

	// analyze the arguments
	// analyse les arguments
	qet_arguments_ = QETArguments(arguments_list);

#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
	if (qet_arguments_.commonElementsDirSpecified()) {
		overrideCommonElementsDir(qet_arguments_.commonElementsDir());
	}
#endif
#ifdef QET_ALLOW_OVERRIDE_CTBTD_OPTION
	if (qet_arguments_.commonTitleBlockTemplatesDirSpecified()) {
		overrideCommonTitleBlockTemplatesDir(
				qet_arguments_.commonTitleBlockTemplatesDir());
	}
#endif
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
	if (qet_arguments_.configDirSpecified()) {
		overrideConfigDir(qet_arguments_.configDir());
	}
#endif

	if (qet_arguments_.langDirSpecified()) {
		overrideLangDir(qet_arguments_.langDir());
	}

	if (qet_arguments_.printLicenseRequested()) {
		printLicense();
		non_interactive_execution_ = true;
	}
	if (qet_arguments_.printHelpRequested()) {
		printHelp();
		non_interactive_execution_ = true;
	}
	if (qet_arguments_.printVersionRequested()) {
		printVersion();
		non_interactive_execution_ = true;
	}
}

/**
	@brief QETApp::initSplashScreen
	Initializes the splash screen if and only if the execution is
	interactive. Otherwise, the splash_screen_ attribute is 0.
	\~French Initialise le splash screen si et seulement si l'execution est
	interactive. Autrement, l'attribut splash_screen_ vaut 0.
*/
void QETApp::initSplashScreen()
{
	if (non_interactive_execution_) return;
	m_splash_screen = new QSplashScreen(QPixmap(":/ico/splash.png"));
	m_splash_screen -> show();
	setSplashScreenStep(tr("Chargement...", "splash screen caption"));
}

/**
	@brief QETApp::setSplashScreenStep
	Changes the text of the splash screen and takes events into account.
	If the application is running non-interactively,
	this method does nothing.
	\~French Change le texte du splash screen et prend en compte les
	evenements. Si l'application s'execute de facon non interactive,
	cette methode ne fait rien.
	\~ @param message : the text of the splash screen
	\~French
*/
void QETApp::setSplashScreenStep(const QString &message) {
	if (!m_splash_screen) return;
	if (!message.isEmpty()) {
		m_splash_screen -> showMessage(message,
						   Qt::AlignBottom | Qt::AlignLeft);
	}
	qApp->processEvents();
}

/**
	@brief QETApp::initLanguage
	Determine and apply the language to use for the application
	\~French Determine et applique le langage a utiliser pour l'application
*/
void QETApp::initLanguage()
{
	setLanguage(langFromSetting());
}

/**
	@brief QETApp::initStyle
	Setup the gui style
*/
void QETApp::initStyle()
{
	initial_palette_ = qApp->palette();

	//Apply or not the system style
	QSettings settings;
	useSystemPalette(settings.value("usesystemcolors", true).toBool());
}

/**
	@brief QETApp::initConfiguration
	Reads and takes into account the configuration of the application.
	This method will create, if necessary:
	  - the configuration folder
	  - the folder of the personal collection
	  - the directory for custom title blocks
	\~French
	Lit et prend en compte la configuration de l'application.
	Cette methode creera, si necessaire :
	  - le dossier de configuration
	  - le dossier de la collection perso
	  - the directory for custom title blocks
*/
void QETApp::initConfiguration()
{
	// create configuration files if necessary
	// cree les dossiers de configuration si necessaire
	QDir config_dir(QETApp::configDir());
	if (!config_dir.exists()) config_dir.mkpath(QETApp::configDir());

	QDir custom_elements_dir(QETApp::customElementsDir());
	if (!custom_elements_dir.exists())
		custom_elements_dir.mkpath(QETApp::customElementsDir());

	QDir custom_tbt_dir(QETApp::customTitleBlockTemplatesDir());
	if (!custom_tbt_dir.exists())
		custom_tbt_dir.mkpath(QETApp::customTitleBlockTemplatesDir());

	/* recent files
	 * note:
	 *  icons must be initialized before these instructions
	 *  (which create menus internally)
	 *
	 * fichiers recents
	 * note :
	 *  les icones doivent etre initialisees avant ces instructions
	 *  (qui creent des menus en interne)
	*/
	m_projects_recent_files = new RecentFiles("projects");
	m_projects_recent_files -> setIconForFiles(QET::Icons::ProjectFile);
	m_elements_recent_files = new RecentFiles("elements");
	m_elements_recent_files -> setIconForFiles(QET::Icons::Element);
}

/**
	@brief QETApp::initSystemTray
	Build the icon in the systray and its menu
	\~French Construit l'icone dans le systray et son menu
*/
void QETApp::initSystemTray()
{
	setSplashScreenStep(tr("Chargement... icône du systray",
				   "splash screen caption"));
	// initialization of the icon menus in the systray
	// initialisation des menus de l'icone dans le systray
	menu_systray = new QMenu(tr("QElectroTech", "systray menu title"));

	quitter_qet       = new QAction(QET::Icons::ApplicationExit,
					tr("&Quitter"),this);
	reduce_appli      = new QAction(QET::Icons::Hide,
					tr("&Masquer"),this);
	restore_appli     = new QAction(QET::Icons::Restore,
					tr("&Restaurer"),this);
	reduce_diagrams   = new QAction(QET::Icons::Hide,
					tr("&Masquer tous les éditeurs de schéma"),
					this);
	restore_diagrams  = new QAction(QET::Icons::Restore,
					tr("&Restaurer tous les éditeurs de schéma"),
					this);
	reduce_elements   = new QAction(QET::Icons::Hide,
					tr("&Masquer tous les éditeurs d'élément"),
					this);
	restore_elements  = new QAction(QET::Icons::Restore,
					tr("&Restaurer tous les éditeurs d'élément"),
					this);
	reduce_templates  = new QAction(QET::Icons::Hide,
					tr("&Masquer tous les éditeurs de cartouche",
					   "systray submenu entry"), this);
	restore_templates = new QAction(QET::Icons::Restore,
					tr("&Restaurer tous les éditeurs de cartouche",
					   "systray submenu entry"), this);
	new_diagram       = new QAction(QET::Icons::WindowNew,
					tr("&Nouvel éditeur de schéma"),this);
	new_element       = new QAction(QET::Icons::WindowNew,
					tr("&Nouvel éditeur d'élément"),this);

	quitter_qet   -> setStatusTip(tr("Ferme l'application QElectroTech"));
	reduce_appli  -> setToolTip(tr("Réduire QElectroTech dans le systray"));
	restore_appli -> setToolTip(tr("Restaurer QElectroTech"));

	connect(quitter_qet,      SIGNAL(triggered()), this, SLOT(quitQET()));
	connect(reduce_appli,     SIGNAL(triggered()), this, SLOT(reduceEveryEditor()));
	connect(restore_appli,    SIGNAL(triggered()), this, SLOT(restoreEveryEditor()));
	connect(reduce_diagrams,  SIGNAL(triggered()), this, SLOT(reduceDiagramEditors()));
	connect(restore_diagrams, SIGNAL(triggered()), this, SLOT(restoreDiagramEditors()));
	connect(reduce_elements,  SIGNAL(triggered()), this, SLOT(reduceElementEditors()));
	connect(restore_elements, SIGNAL(triggered()), this, SLOT(restoreElementEditors()));
	connect(reduce_templates, SIGNAL(triggered()), this, SLOT(reduceTitleBlockTemplateEditors()));
	connect(restore_templates,SIGNAL(triggered()), this, SLOT(restoreTitleBlockTemplateEditors()));
	connect(new_diagram,      SIGNAL(triggered()), this, SLOT(newDiagramEditor()));
	connect(new_element,      SIGNAL(triggered()), this, SLOT(newElementEditor()));

	// initialization of the systray icon
	// initialisation de l'icone du systray
	m_qsti = new QSystemTrayIcon(QET::Icons::QETLogo, this);
	m_qsti -> setToolTip(tr("QElectroTech", "systray icon tooltip"));
	connect(m_qsti, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		this, SLOT(systray(QSystemTrayIcon::ActivationReason)));
	m_qsti -> setContextMenu(menu_systray);
	m_qsti -> show();
}

/**
	Add a list of \a windows to \a menu.
	This template function assumes it will be given a QList of pointers to
	objects inheriting the QMainWindow class.
	T the class inheriting QMainWindow
	@param menu the menu windows will be added to
	@param windows A list of top-level windows.
*/
template <class T> void QETApp::addWindowsListToMenu(
		QMenu *menu, const QList<T *> &windows) {
	menu -> addSeparator();
	foreach (QMainWindow *window, windows) {
		QAction *current_menu = menu -> addAction(window -> windowTitle());
		current_menu -> setCheckable(true);
		current_menu -> setChecked(window -> isVisible());
		connect(current_menu, SIGNAL(triggered()), &signal_map, SLOT(map()));
		signal_map.setMapping(current_menu, window);
	}
}

/**
	@brief QETApp::projectIdFromString
	@param url The location of a collection item (title block template,
	element, category, ...).
	@return the id of the project mentionned in the URL,
	or -1 if none could be found.
*/
int QETApp::projectIdFromString(const QString &url) {
	QRegularExpression embedded(
				"^project(?<project_id>[0-9]+)\\+embed.*$",
				QRegularExpression::CaseInsensitiveOption);
	if (!embedded.isValid())
	{
		qWarning() <<QObject::tr("this is an error in the code")
			  << embedded.errorString()
			  << embedded.patternErrorOffset();
		return(-1);
	}
	QRegularExpressionMatch match = embedded.match(url);
	if (!match.hasMatch())
	{
		qDebug()<<"no Match => return"
			   <<url;
		return(-1);
	}
	bool conv_ok = false;
	int project_id = match.captured("project_id").toUInt(&conv_ok);
	if (conv_ok) {
		return(project_id);
	}
	return(-1);
}

/**
	@brief QETApp::projectFromString
	@param url The location of a collection item (title block template,
	element, category, ...).
	@return the project mentionned in the URL, or 0 if none could be
	found.
*/
QETProject *QETApp::projectFromString(const QString &url) {
	int project_id = projectIdFromString(url);
	if (project_id == -1) return(nullptr);
	return(project(project_id));
}

/**
	@brief QETApp::buildSystemTrayMenu
	builds the icon menu in the systray
	\~French construit le menu de l'icone dans le systray
*/
void QETApp::buildSystemTrayMenu()
{
	menu_systray -> clear();

	// get editors
	// recupere les editeurs
	QList<QETDiagramEditor *> diagrams = diagramEditors();
	QList<QETElementEditor *> elements = elementEditors();
	QList<QETTitleBlockTemplateEditor *> tbtemplates = titleBlockTemplateEditors();
	fetchWindowStats(diagrams, elements, tbtemplates);

	// add reduce / restore button to menu
	// ajoute le bouton reduire / restaurer au menu
	menu_systray -> addAction(every_editor_reduced ? restore_appli : reduce_appli);

	// add schema editors in a submenu
	// ajoute les editeurs de schemas dans un sous-menu
	QMenu *diagrams_submenu = menu_systray -> addMenu(tr("Éditeurs de schémas"));
	diagrams_submenu -> addAction(reduce_diagrams);
	diagrams_submenu -> addAction(restore_diagrams);
	diagrams_submenu -> addAction(new_diagram);
	reduce_diagrams -> setEnabled(!diagrams.isEmpty() && !every_diagram_reduced);
	restore_diagrams -> setEnabled(!diagrams.isEmpty() && !every_diagram_visible);
	addWindowsListToMenu<QETDiagramEditor>(diagrams_submenu, diagrams);

	// add item editors to the menu
	// ajoute les editeurs d'elements au menu
	QMenu *elements_submenu = menu_systray -> addMenu(tr("Éditeurs d'élément"));
	elements_submenu -> addAction(reduce_elements);
	elements_submenu -> addAction(restore_elements);
	elements_submenu -> addAction(new_element);
	reduce_elements -> setEnabled(!elements.isEmpty() && !every_element_reduced);
	restore_elements -> setEnabled(!elements.isEmpty() && !every_element_visible);
	elements_submenu -> addSeparator();
	addWindowsListToMenu<QETElementEditor>(elements_submenu, elements);

	// add title block template editors in a submenu
	// add title block template editors in a submenu
	QMenu *tbtemplates_submenu = menu_systray -> addMenu(tr("Éditeurs de cartouche",
								"systray menu entry"));
	tbtemplates_submenu -> addAction(reduce_templates);
	tbtemplates_submenu -> addAction(restore_templates);
	reduce_templates  -> setEnabled(!tbtemplates.isEmpty() && !every_template_reduced);
	restore_templates -> setEnabled(!tbtemplates.isEmpty() && !every_template_visible);
	addWindowsListToMenu<QETTitleBlockTemplateEditor>(tbtemplates_submenu, tbtemplates);

	// add the exit button to the menu
	// ajoute le bouton quitter au menu
	menu_systray -> addSeparator();
	menu_systray -> addAction(quitter_qet);
}

/**
	@brief QETApp::checkBackupFiles
	Check for backup files.
	If backup was found, open a dialog and ask user what to do.
*/
void QETApp::checkBackupFiles()
{
	QList<KAutoSaveFile *> stale_files = KAutoSaveFile::allStaleFiles();

	//Remove from the list @stale_files, the stales file of opened project
	const QList<KAutoSaveFile *> sf = stale_files;
	for (KAutoSaveFile *kasf : sf)
	{
		for (QETProject *project : registeredProjects().values())
		{
			/* We must to adjust with the flag
			 * QUrl::StripTrailingSlash to compar a path formated
			 * like the path returned by KAutoSaveFile
			 */
			const QString path = QUrl::fromLocalFile(
						project->filePath()).adjusted(
						QUrl::RemoveScheme
						| QUrl::StripTrailingSlash).path();
			if (kasf->managedFile() == path) {
				stale_files.removeOne(kasf);
			}
		}
	}

	if (stale_files.isEmpty()) {
		return;
	}

	QString text;
	if(stale_files.size() == 1) {
		text.append(tr("<b>Le fichier de restauration suivant a été trouvé,<br>"
					   "Voulez-vous l'ouvrir ?</b><br>"));
	} else {
		text.append(tr("<b>Les fichiers de restauration suivant on été trouvé,<br>"
					   "Voulez-vous les ouvrir ?</b><br>"));
	}
	for(const KAutoSaveFile *kasf : stale_files)
	{
#ifdef Q_OS_WIN
	//Remove the first character '/' before the name of the drive
	text.append("<br>" + kasf->managedFile().path().remove(0,1));
#else
	text.append("<br>" + kasf->managedFile().path());
#endif
	}

	//Open backup file
	if (QET::QetMessageBox::question(nullptr,
					 tr("Fichier de restauration"),
					 text,
					 QMessageBox::Ok
					 |QMessageBox::Cancel
					 )
			== QMessageBox::Ok)
	{
		//If there is opened editors, we find those who are visible
		if (diagramEditors().count())
		{
			diagramEditors().first()->setVisible(true);
			diagramEditors().first()->openBackupFiles(stale_files);
		}
		else
		{
			QETDiagramEditor *editor = new QETDiagramEditor();
			editor->openBackupFiles(stale_files);
		}
	}
	else //Clear backup file
	{
		//Remove the stale files
		for (KAutoSaveFile *stale : stale_files)
		{
			stale->open(QIODevice::ReadWrite);
			delete stale;
		}
	}
}

/**
	@brief QETApp::fetchWindowStats
	Updates the booleans concerning the state of the windows
	\~French Met a jour les booleens concernant l'etat des fenetres
	\~ @param diagrams : QList<QETDiagramEditor *>
	\~ @param elements : QList<QETElementEditor *>
	\~ @param tbtemplates : QList<QETTitleBlockTemplateEditor *>
*/
void QETApp::fetchWindowStats(
	const QList<QETDiagramEditor *> &diagrams,
	const QList<QETElementEditor *> &elements,
	const QList<QETTitleBlockTemplateEditor *> &tbtemplates
) {
	// count the number of visible diagrams
	// compte le nombre de schemas visibles
	int visible_diagrams = 0;
	foreach(QMainWindow *w, diagrams)
		if (w -> isVisible())
			++ visible_diagrams;
	every_diagram_reduced = !visible_diagrams;
	every_diagram_visible = visible_diagrams == diagrams.count();

	// count the number of visible elements
	// compte le nombre de schemas visibles
	int visible_elements = 0;
	foreach(QMainWindow *w, elements)
		if (w -> isVisible())
			++ visible_elements;
	every_element_reduced = !visible_elements;
	every_element_visible = visible_elements == elements.count();

	// count visible template editors
	int visible_templates = 0;
	foreach(QMainWindow *window, tbtemplates) {
		if (window -> isVisible())
			++ visible_templates;
	}
	every_template_reduced = !visible_templates;
	every_template_visible = visible_templates == tbtemplates.count();

	// determines if all elements are reduced
	// determine si tous les elements sont reduits
	every_editor_reduced = every_element_reduced && every_diagram_reduced;
}

#ifdef Q_OS_DARWIN
/**
	Gere les evenements, en particulier l'evenement FileOpen sous MacOs.
	@param e Evenement a gerer
*/
bool QETApp::eventFiltrer(QObject *object, QEvent *e) {
	// gere l'ouverture de fichiers (sous MacOs)
	if (e -> type() == QEvent::FileOpen) {
	// nom du fichier a ouvrir
	QString filename = static_cast<QFileOpenEvent *>(e) -> file();
	openFiles(QStringList() << filename);
	return(true);
	} else {
	return QObject::eventFilter(object, e);
	}
}
#endif

/**
	@brief QETApp::printHelp
	Display help and usage on standard output
	\~French Affiche l'aide et l'usage sur la sortie standard
*/
void QETApp::printHelp()
{
	QString help(
		tr("Usage : ")
		+ QFileInfo(qApp->applicationFilePath()).fileName()
		+ tr(" [options] [fichier]...\n\n")
		+ tr("QElectroTech, une application de réalisation de schémas électriques.\n\n"
		"Options disponibles : \n"
		"  --help                        Afficher l'aide sur les options\n"
		"  -v, --version                 Afficher la version\n"
		"  --license                     Afficher la licence\n")
#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
		+ tr("  --common-elements-dir=DIR     Definir le dossier de la collection d'elements\n")
#endif
#ifdef QET_ALLOW_OVERRIDE_CTBTD_OPTION
		+ tr("  --common-tbt-dir=DIR          Definir le dossier de la collection de modeles de cartouches\n")
#endif
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
		+ tr("  --config-dir=DIR              Definir le dossier de configuration\n")
#endif
		+ tr("  --lang-dir=DIR                Definir le dossier contenant les fichiers de langue\n")
	);
	std::cout << qPrintable(help) << std::endl;
}

/**
	@brief QETApp::printVersion
	Print version to standard output
	\~French Affiche la version sur la sortie standard
*/
void QETApp::printVersion()
{
	std::cout << qPrintable(QET::displayedVersion) << std::endl;
}

/**
	@brief QETApp::printLicense
	Display license on standard output
	\~French Affiche la licence sur la sortie standard
*/
void QETApp::printLicense()
{
	std::cout << qPrintable(QET::license()) << std::endl;
}

/**
	@brief QETApp::registeredProjects
	@return the list of projects with their associated ids
	\~French la liste des projets avec leurs ids associes
*/
QMap<uint, QETProject *> QETApp::registeredProjects()
{
	return(registered_projects_);
}

/**
	@brief QETApp::registerProject
	\~ @param project :
	Project to register with the application
	\~French Projet a enregistrer aupres de l'application
	\~ @return true if the project could be saved, false otherwise
	Failure to save a project usually means that it is already saved.
	\~French true si le projet a pu etre enregistre, false sinon
	L'echec de l'enregistrement d'un projet signifie
	generalement qu'il est deja enregistre.
*/
bool QETApp::registerProject(QETProject *project) {
	// the project must seem valid
	// le projet doit sembler valide
	if (!project) return(false);

	// if the project is already saved, returns false
	// si le projet est deja enregistre, renvoie false
	if (projectId(project) != -1) return(false);

	// save the project
	// enregistre le projet
	registered_projects_.insert(next_project_id ++, project);
	return(true);
}

/**
	@brief QETApp::unregisterProject
	Cancel the recording of the project project
	\~French Annule l'enregistrement du projet project
	\~ @param project :
	Project to be unregistered
	\~French Projet dont il faut annuler l'enregistrement
	\~ @return true if the cancellation was successful, false otherwise
	The failure of this method usually means that the project was not saved.
	\~French true si l'annulation a reussi, false sinon L'echec de cette
	methode signifie generalement que le projet n'etait pas enregistre.
*/
bool QETApp::unregisterProject(QETProject *project) {
	int project_id = projectId(project);

	// if the project is not saved, returns false
	// si le projet n'est pas enregistre, renvoie false
	if (project_id == -1) return(false);

	// cancels the registration of the project
	// annule l'enregistrement du projet
	return(registered_projects_.remove(project_id) == 1);
}

/**
	@brief QETApp::project
	\~French
	\~ @param id :
	Id of the desired project
	\~French Id du projet voulu
	\~ @return the project corresponds to the id passed in parameter
	\~French le projet correspond a l'id passe en parametre
*/
QETProject *QETApp::project(const uint &id) {
	if (registered_projects_.contains(id)) {
		return(registered_projects_[id]);
	} else {
		return(nullptr);
	}
}

/**
	@brief QETApp::projectId
	\~French
	\~ @param project :
	Project whose id we want to retrieve
	\~French Projet dont on souhaite recuperer l'id
	\~ @return the project id as a parameter if it is saved, -1 otherwise
	\~French l'id du projet en parametre si celui-ci est enregistre, -1 sinon
*/
int QETApp::projectId(const QETProject *project) {
	foreach(int id, registered_projects_.keys()) {
		if (registered_projects_[id] == project) {
			return(id);
		}
	}
	return(-1);
}
