/*
	Copyright 2006-2021 The QElectroTech Team
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
#ifndef QET_APP_H
#define QET_APP_H

#include "ElementsCollection/elementslocation.h"
#include "qetarguments.h"
#include "titleblock/templatelocation.h"

#include <QByteArray>
#include <QPalette>
#include <QSystemTrayIcon>
#include <QTranslator>

class QSplashScreen;
class QMenu;
class QAction;
class QMainWindow;

#define QETAPP_COMMON_TBT_PROTOCOL "commontbt"
#define QETAPP_CUSTOM_TBT_PROTOCOL "customtbt"

class AboutQET;
class QETDiagramEditor;
class QETElementEditor;
class ElementsCollectionCache;
class TitleBlockTemplate;
class TitleBlockTemplatesCollection;
class TitleBlockTemplatesFilesCollection;
class QETProject;
class QETTitleBlockTemplateEditor;
class QTextOrientationSpinBoxWidget;
class RecentFiles;

/**
	@brief The QETApp class
	This class represents the QElectroTech application.
*/
class QETApp : public QObject
{
	Q_OBJECT
		// constructors, destructor
	public:
		QETApp();
		~QETApp() override;
	
	private:
		QETApp(const QETApp &);
	
		// methods
	public:
		static QETApp *instance();
		void setLanguage(const QString &);
		static QString langFromSetting ();
		void switchLayout(Qt::LayoutDirection);
		static void printHelp();
		static void printVersion();
		static void printLicense();
		
		static ElementsCollectionCache *collectionCache();
		
		static TitleBlockTemplatesFilesCollection *commonTitleBlockTemplatesCollection();
		static TitleBlockTemplatesFilesCollection *customTitleBlockTemplatesCollection();
		static QList<TitleBlockTemplatesCollection *> availableTitleBlockTemplatesCollections();
		static TitleBlockTemplatesCollection *titleBlockTemplatesCollection(const QString &);
		
		static QString commonElementsDir();
		static QString customElementsDir();
		static QString commonElementsDirN();
		static QString customElementsDirN();
		static void resetCollectionsPath();
		static QString commonTitleBlockTemplatesDir();
		static QString customTitleBlockTemplatesDir();
		static bool registerProject(QETProject *);
		static bool unregisterProject(QETProject *);
		static QMap<uint, QETProject *> registeredProjects();
		static QETProject *project(const uint &);
		static int projectId(const QETProject *);
		static QString configDir();
		static QString languagesPath();
		static QString realPath(const QString &);
		static QString symbolicPath(const QString &);
		static QStringList handledFileExtensions();
		static QStringList handledFiles(const QList<QUrl> &);
		static RecentFiles *projectsRecentFiles();
		static RecentFiles *elementsRecentFiles();
		
#ifdef QET_ALLOW_OVERRIDE_CED_OPTION
	public:
		static void overrideCommonElementsDir(const QString &);
	private:
		/**
			@brief common_elements_dir
			Directory containing the common elements collection
		*/
		static QString m_overrided_common_elements_dir;
#endif
		
#ifdef QET_ALLOW_OVERRIDE_CTBTD_OPTION
	public:
		static void overrideCommonTitleBlockTemplatesDir(const QString &);
	private:
		/**
			@brief common_tbt_dir_
			Directory containing
			the common title block templates collection
		*/
		static QString common_tbt_dir_;
#endif
		
#ifdef QET_ALLOW_OVERRIDE_CD_OPTION
	public:
		static void overrideConfigDir(const QString &);
	private:
		/**
			@brief config_dir
			Directory containing the user configuration
			and the custom elements collection
		*/
		static QString config_dir;
#endif
		
	public:
		static void overrideLangDir(const QString &);
		/**
			@brief lang_dir
			Directory containing localization files.
		*/
		static QString lang_dir;
		static QFont diagramTextsFont(qreal = -1.0);
		static QFont diagramTextsItemFont(qreal = -1.0);
		static QFont dynamicTextsItemFont(qreal = -1.0);
		static QFont indiTextsItemFont (qreal = -1.0);
		static QETDiagramEditor *diagramEditorForFile(const QString &);
		static QETDiagramEditor *diagramEditorAncestorOf (const QWidget *child);
		static QList<QETDiagramEditor *> diagramEditors();
		static QList<QETElementEditor *> elementEditors();
		static QList<QETElementEditor *> elementEditors(QETProject *);
		static QList<QETTitleBlockTemplateEditor *> titleBlockTemplateEditors();
		static QList<QETTitleBlockTemplateEditor *> titleBlockTemplateEditors(QETProject *);
		static QTextOrientationSpinBoxWidget *createTextOrientationSpinBoxWidget();
		static TitleBlockTemplate *defaultTitleBlockTemplate();
	
	protected:
		
#ifdef Q_OS_DARWIN
	bool eventFiltrer(QObject *object, QEvent *);
#endif
	
		// attributes
	private:
		static QETApp *m_qetapp;
		QTranslator qtTranslator;
		QTranslator qetTranslator;
		QSystemTrayIcon *m_qsti;
		QSplashScreen *m_splash_screen;
		QMenu *menu_systray;
		QAction *quitter_qet;
		QAction *reduce_appli;
		QAction *restore_appli;
		QAction *reduce_diagrams;
		QAction *restore_diagrams;
		QAction *reduce_elements;
		QAction *restore_elements;
		QAction *reduce_templates;
		QAction *restore_templates;
		QAction *new_diagram;
		QAction *new_element;
		QHash<QMainWindow *, QByteArray> window_geometries;
		QHash<QMainWindow *, QByteArray> window_states;
		bool every_editor_reduced;
		bool every_diagram_reduced;
		bool every_diagram_visible;
		bool every_element_reduced;
		bool every_element_visible;
		bool every_template_reduced;
		bool every_template_visible;
		QSignalMapper signal_map;
		QETArguments qet_arguments_; ///< Command-line arguments parser
		/**
			@brief non_interactive_execution_
			Whether the application will end
			without any user interaction
		 */
		bool non_interactive_execution_;
		QPalette initial_palette_;   ///< System color palette
		
		static TitleBlockTemplatesFilesCollection *m_common_tbt_collection;
		static TitleBlockTemplatesFilesCollection *m_custom_tbt_collection;
		static ElementsCollectionCache *collections_cache_;
		static QMap<uint, QETProject *> registered_projects_;
		static uint next_project_id;
		static RecentFiles *m_projects_recent_files;
		static RecentFiles *m_elements_recent_files;
		static TitleBlockTemplate *default_titleblock_template_;

		static QString m_common_element_dir;
		static bool m_common_element_dir_is_set;

		static QString m_custom_element_dir;
		static bool m_custom_element_dir_is_set;

		static QString m_user_custom_tbt_dir;

	
	public slots:
		void systray(QSystemTrayIcon::ActivationReason);
		void reduceEveryEditor();
		void restoreEveryEditor();
		void reduceDiagramEditors();
		void restoreDiagramEditors();
		void reduceElementEditors();
		void restoreElementEditors();
		void reduceTitleBlockTemplateEditors();
		void restoreTitleBlockTemplateEditors();
		void newDiagramEditor();
		void newElementEditor();
		bool closeEveryEditor();
		void setMainWindowVisible(QMainWindow *, bool);
		void invertMainWindowVisibility(QWidget *);
		void useSystemPalette(bool);
		void quitQET();
		void checkRemainingWindows();
		void openFiles(const QETArguments &);
		void openProjectFiles(const QStringList &);
		void openElementFiles(const QStringList &);
		void openElementLocations(const QList<ElementsLocation> &);
		void openTitleBlockTemplate(const TitleBlockTemplateLocation &, bool = false);
		void openTitleBlockTemplate(const QString &);
		void openTitleBlockTemplateFiles(const QStringList &);
		void configureQET();
		void aboutQET();
		void receiveMessage(int instanceId, QByteArray message);
	
	private:
		template <class T> QList<T *> detectWindows() const;
		template <class T> void setMainWindowsVisible(bool);
		QList<QWidget *> floatingToolbarsAndDocksForMainWindow(QMainWindow *) const;
		void parseArguments();
		void initSplashScreen();
		void setSplashScreenStep(const QString & = QString());
		void initLanguage();
		void initStyle();
		void initConfiguration();
		void initSystemTray();
		void buildSystemTrayMenu();
		void checkBackupFiles();
		void fetchWindowStats(
			const QList<QETDiagramEditor *> &,
			const QList<QETElementEditor *> &,
			const QList<QETTitleBlockTemplateEditor *> &
		);
		template <class T> void addWindowsListToMenu(QMenu *, const QList<T *> &);
		static int projectIdFromString(const QString &);
		static QETProject *projectFromString(const QString &);
};

#endif
