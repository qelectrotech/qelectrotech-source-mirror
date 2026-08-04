/*
	Copyright 2006-2026 The QElectroTech Team
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
#include <QCoreApplication>
#include "ElementsCollection/elementscollectionwidget.h"
#include "QWidgetAnimation/qwidgetanimation.h"
#include "autoNum/ui/autonumberingdockwidget.h"
#include "conductornumexport.h"
#include "diagrameditoractions.h"
#include "diagramcommands.h"
#include "diagramevent/diagrameventaddimage.h"
#include "diagramevent/diagrameventaddshape.h"
#include "diagramevent/diagrameventaddtext.h"
#include "diagramview.h"
#include "elementspanelwidget.h"
#include "factory/qetgraphicstablefactory.h"
#include "print/projectprintwindow.h"
#include "project/projectpropertieshandler.h"
#include "projectview.h"
#include "qetproject.h"
#include "qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "qetgraphicsitem/conductortextitem.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qeticons.h"
#include "qetmessagebox.h"
#include "recentfiles.h"
#include "shortcutmanager.h"
#include "ui/bomexportdialog.h"
#include "ui/jumptoelementdialog.h"
#include "ui/diagrampropertieseditordockwidget.h"
#include "ui/backupdialog.h"
#include "ui/dialogwaiting.h"
#include "undocommand/addelementtextcommand.h"
#include "utils/qetutils.h"
#include "undocommand/rotateselectioncommand.h"
#include "undocommand/rotatetextscommand.h"
#include "diagram.h"
#include "TerminalStrip/ui/terminalstripeditorwindow.h"
#include "ui/diagrameditorhandlersizewidget.h"
#include "TerminalStrip/ui/addterminalstripitemdialog.h"
#include "wiringlistexport.h"
#include "ui/terminalnumberingdialog.h"
#include <QDateTime>
#include <QDebug>
#include <QDir>
#ifdef BUILD_WITHOUT_KF5
#	include "ui/nokde/kautosavefile.h"
#else
#	include <KAutoSaveFile>
#endif

/**
	@brief QETDiagramEditor::QETDiagramEditor
	Constructor
	@param files : list of files to open
	@param parent : parent widget
*/
QETDiagramEditor::QETDiagramEditor(const QStringList &files, QWidget *parent) :
	QETMainWindow(parent),
	open_dialog_dir            (QETApp::documentDir())
{
		//Trivial property use to set the graphics handler size
	setProperty("graphics_handler_size", 10);

	activeSubWindowIndex = 0;

	QSplitter *splitter_ = new QSplitter(this);
	splitter_->setChildrenCollapsible(false);
	splitter_->setOrientation(Qt::Vertical);
	splitter_->addWidget(&m_workspace);
	splitter_->addWidget(&m_search_and_replace_widget);
	setCentralWidget(splitter_);
	m_search_and_replace_widget.setEditor(this);

	QList<int> s;
	s << m_workspace.maximumHeight() << m_search_and_replace_widget.minimumSizeHint().height();
	splitter_->setSizes(s); //Force the size of the search and replace widget, force have a good animation the first time he is showed

	auto anim = new QWidgetAnimation(&m_search_and_replace_widget, Qt::Vertical, QWidgetAnimation::lastSize, 250);
	anim->setObjectName("search and replace animator");
	m_search_and_replace_widget.setHidden(true);
	anim->setLastShowSize(m_search_and_replace_widget.minimumSizeHint().height());

		//Set object name to be retrieved by the stylesheets
	m_workspace.setBackground(QBrush(Qt::NoBrush));
	m_workspace.setObjectName("mdiarea");
	m_workspace.setTabsClosable(true);

		//Set the signal mapper
	connect(&windowMapper, SIGNAL(mapped(QWidget *)), this, SLOT(activateWidget(QWidget *)));

	setWindowTitle(tr("QElectroTech", "window title"));
	setWindowIcon(QET::Icons::QETLogo);
	statusBar() -> showMessage(tr("QElectroTech", "status bar message"));

	setUpElementsPanel();
	setUpElementsCollectionWidget();
	setUpUndoStack();
	setUpSelectionPropertiesEditor();
	setUpAutonumberingWidget();

	setUpActions();
	setUpToolBar();
	setUpMenu();

	tabifyDockWidget(qdw_undo, qdw_pa);

		//By default the windows is maximised
	setMinimumSize(QSize(500, 350));
	setWindowState(Qt::WindowMaximized);

	connect (&m_workspace,
		 SIGNAL(subWindowActivated(QMdiSubWindow *)),
		 this,
		 SLOT(subWindowActivated(QMdiSubWindow*)));
	connect (QApplication::clipboard(),
		 SIGNAL(dataChanged()),
		 this,
		 SLOT(slot_updatePasteAction()));

	readSettings();
	show();

		//If valid file path is given as arguments
	uint opened_projects = 0;
	if (files.count())
	{
			//So we open this files
		foreach(QString file, files)
			if (openAndAddProject(file))
				++ opened_projects;
	}

	slot_updateActions();
}

/**
	Destructeur
*/
QETDiagramEditor::~QETDiagramEditor()
{
		//Must run before QObject::~QObject(): several QActionGroup members
		//of DiagramEditorActions are value members parented to `this`, and
		//rely on their own destructor (triggered by deleting m_actions) to
		//unregister themselves from this object's Qt children list before
		//~QObject() would otherwise try to delete them as if they were
		//separately heap-allocated -- which they are not.
	delete m_actions;
}

/**
	@brief QETDiagramEditor::setUpElementsPanel
	Setup the element panel and element panel widget
*/
void QETDiagramEditor::setUpElementsPanel()
{
	//Add the element panel as a QDockWidget
	qdw_pa = new QDockWidget(tr("Projets", "dock title"), this);

	qdw_pa -> setObjectName   ("projects panel");
	qdw_pa -> setAllowedAreas (Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	qdw_pa -> setFeatures     (
				QDockWidget::DockWidgetClosable
				|QDockWidget::DockWidgetMovable
				|QDockWidget::DockWidgetFloatable);
	qdw_pa -> setMinimumWidth (160);
	qdw_pa -> setWidget       (pa = new ElementsPanelWidget(qdw_pa));

	addDockWidget(Qt::LeftDockWidgetArea, qdw_pa);

	connect(pa, SIGNAL(requestForProject                  (QETProject *)), this, SLOT(activateProject(QETProject *)));
	connect(pa, SIGNAL(requestForProjectClosing           (QETProject *)), this, SLOT(closeProject(QETProject *)));
	connect(pa, SIGNAL(requestForProjectPropertiesEdition (QETProject *)), this, SLOT(editProjectProperties(QETProject *)));
	connect(pa, SIGNAL(requestForNewDiagram               (QETProject *)), this, SLOT(addDiagramToProject(QETProject *)));
	connect(pa, SIGNAL(requestForDiagramPropertiesEdition (Diagram *)), this, SLOT(editDiagramProperties(Diagram *)));
	connect(pa, SIGNAL(requestForDiagramsDeletion         (const QList<Diagram *> &)), this, SLOT(removeDiagrams(const QList<Diagram *> &)));
	connect(pa, SIGNAL(requestForDiagramMoveUp			  (const QList<Diagram *> &)), this, SLOT(moveDiagramUp(const QList<Diagram *>&)));
	connect(pa, SIGNAL(requestForDiagramMoveDown		  (const QList<Diagram *> &)), this, SLOT(moveDiagramDown(const QList<Diagram *>&)));
	connect(pa, SIGNAL(requestForDiagramMoveUpTop		  (const QList<Diagram *> &)), this, SLOT(moveDiagramUpTop(const QList<Diagram *>&)));
	connect(pa, SIGNAL(requestForDiagramMoveUpx10		  (const QList<Diagram *> &)), this, SLOT(moveDiagramUpx10(const QList<Diagram *>&)));
	connect(pa, SIGNAL(requestForDiagramMoveDownx10		  (const QList<Diagram *> &)), this, SLOT(moveDiagramDownx10(const QList<Diagram *>&)));
	connect(pa, SIGNAL(requestForDiagramMoveUpx100		  (const QList<Diagram *> &)), this, SLOT(moveDiagramUpx100(const QList<Diagram *>&)));
	connect(pa, SIGNAL(requestForDiagramMoveDownx100	  (const QList<Diagram *> &)), this, SLOT(moveDiagramDownx100(const QList<Diagram *>&)));
}

/**
	@brief QETDiagramEditor::setUpElementsCollectionWidget
	Set up the dock widget of element collection
*/
void QETDiagramEditor::setUpElementsCollectionWidget()
{
	m_qdw_elmt_collection = new QDockWidget(tr("Collections"), this);
	m_qdw_elmt_collection->setObjectName("elements_collection_widget");
	m_qdw_elmt_collection->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	m_qdw_elmt_collection->setFeatures(
				QDockWidget::DockWidgetClosable
				|QDockWidget::DockWidgetMovable
				|QDockWidget::DockWidgetFloatable);

	m_element_collection_widget = new ElementsCollectionWidget(m_qdw_elmt_collection);
	m_qdw_elmt_collection->setWidget(m_element_collection_widget);
	m_element_collection_widget->expandFirstItems();

	addDockWidget(Qt::RightDockWidgetArea, m_qdw_elmt_collection);
}

/**
	@brief QETDiagramEditor::setUpUndoStack
	Setup the undostack and undo stack widget
*/
void QETDiagramEditor::setUpUndoStack()
{

	QUndoView *undo_view = new QUndoView(&undo_group, this);

	undo_view -> setEmptyLabel (tr("Aucune modification"));
	undo_view -> setStatusTip  (tr("Cliquez sur une action pour revenir en arrière dans l'édition de votre schéma", "Status tip"));
	undo_view -> setWhatsThis  (tr("Ce panneau liste les différentes actions effectuées sur le folio courant. Cliquer sur une action permet de revenir à l'état du schéma juste après son application.", "\"What's this\" tip"));

	qdw_undo  = new QDockWidget(tr("Annulations", "dock title"), this);
	qdw_undo -> setObjectName("diagram_undo");

	qdw_undo -> setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	qdw_undo -> setFeatures(
				QDockWidget::DockWidgetClosable
				|QDockWidget::DockWidgetMovable
				|QDockWidget::DockWidgetFloatable);
	qdw_undo -> setMinimumWidth(160);
	qdw_undo -> setWidget(undo_view);

	addDockWidget(Qt::LeftDockWidgetArea, qdw_undo);
}

/**
	@brief QETDiagramEditor::setUpSelectionPropertiesEditor
	Setup the dock for edit the current selection
*/
void QETDiagramEditor::setUpSelectionPropertiesEditor()
{
	m_selection_properties_editor = new DiagramPropertiesEditorDockWidget(this);
	m_selection_properties_editor -> setObjectName("diagram_properties_editor_dock_widget");
	addDockWidget(Qt::RightDockWidgetArea, m_selection_properties_editor);
}

/**
	@brief QETDiagramEditor::setUpAutonumberingWidget
	Setup the dock for AutoNumbering Selection
*/
void QETDiagramEditor::setUpAutonumberingWidget()
{
	m_autonumbering_dock = new AutoNumberingDockWidget(this);
	m_autonumbering_dock -> setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	m_autonumbering_dock -> setFeatures(
				QDockWidget::DockWidgetClosable
				|QDockWidget::DockWidgetMovable
				|QDockWidget::DockWidgetFloatable);
	addDockWidget(Qt::RightDockWidgetArea, m_autonumbering_dock);
}

/**
	@brief QETDiagramEditor::setUpActions
	Set up all Qaction
*/
void QETDiagramEditor::setUpActions()
{
	m_actions = new DiagramEditorActions(this);
}

/**
	@brief QETDiagramEditor::setUpToolBar
*/
void QETDiagramEditor::setUpToolBar()
{
	main_tool_bar = new QToolBar(tr("Outils"), this);
	main_tool_bar -> setObjectName("toolbar");

	view_tool_bar = new QToolBar(tr("Affichage"), this);
	view_tool_bar -> setObjectName("display");

	diagram_tool_bar = new QToolBar(tr("Schéma"), this);
	diagram_tool_bar -> setObjectName("diagram");

	main_tool_bar -> addActions(m_actions->m_file_actions_group.actions());
	main_tool_bar -> addAction(m_actions->m_print);
	main_tool_bar -> addAction(m_actions->m_export_to_pdf);
	main_tool_bar -> addSeparator();
	main_tool_bar -> addAction(m_actions->undo);
	main_tool_bar -> addAction(m_actions->redo);
	main_tool_bar -> addSeparator();
	main_tool_bar -> addAction(m_actions->m_cut);
	main_tool_bar -> addAction(m_actions->m_copy);
	main_tool_bar -> addAction(m_actions->m_paste);
	main_tool_bar -> addSeparator();
	main_tool_bar -> addAction(m_actions->m_delete_selection);
	main_tool_bar -> addAction(m_actions->m_rotate_selection);

	// Modes selection / visualisation et zoom
	view_tool_bar -> addAction(m_actions->m_mode_selection);
	view_tool_bar -> addAction(m_actions->m_mode_visualise);
	view_tool_bar -> addSeparator();
	view_tool_bar -> addWidget(new DiagramEditorHandlerSizeWidget(this));
	view_tool_bar -> addSeparator();
	view_tool_bar -> addAction(m_actions->m_draw_grid);
	view_tool_bar -> addAction(m_actions->m_draw_guides);
	view_tool_bar -> addAction (m_actions->m_grey_background);
	view_tool_bar -> addSeparator();
	view_tool_bar -> addActions(m_actions->m_zoom_action_toolBar);

	diagram_tool_bar -> addAction (m_actions->m_edit_diagram_properties);
	diagram_tool_bar -> addAction (m_actions->m_conductor_reset);
	diagram_tool_bar -> addAction (m_actions->m_auto_conductor);

	m_add_item_tool_bar = new QToolBar(tr("Ajouter"), this);
	m_add_item_tool_bar->setObjectName("adding");
	m_add_item_tool_bar->addActions(m_actions->m_add_item_actions_group.actions());

	m_depth_tool_bar = new QToolBar(tr("Profondeur", "toolbar title"));
	m_depth_tool_bar->setObjectName("diagram_depth_toolbar");
	m_depth_tool_bar->addActions(m_actions->m_depth_action_group->actions());

	addToolBar(Qt::TopToolBarArea, main_tool_bar);
	addToolBar(Qt::TopToolBarArea, view_tool_bar);
	addToolBar(Qt::TopToolBarArea, diagram_tool_bar);
	addToolBar(Qt::TopToolBarArea, m_add_item_tool_bar);
	addToolBar(Qt::TopToolBarArea, m_depth_tool_bar);
}

/**
	@brief QETDiagramEditor::setUpMenu
*/
void QETDiagramEditor::setUpMenu()
{

	QMenu* menu_fichier	  = new QMenu(tr("&Fichier"), this);
	QMenu* menu_edition	  = new QMenu(tr("&Édition"), this);
	QMenu* menu_project	  = new QMenu(tr("&Projet"), this);
	QMenu* menu_affichage = new QMenu(tr("Afficha&ge"), this);
	// QMenu *menu_outils    = new QMenu(tr("O&utils"), this);
	windows_menu = new QMenu(tr("Fe&nêtres"), this);

	insertMenu(settings_menu_, menu_fichier);
	insertMenu(settings_menu_, menu_edition);
	insertMenu(settings_menu_, menu_project);
	insertMenu(settings_menu_, menu_affichage);
	insertMenu(help_menu_, windows_menu);

	// File menu
	QMenu *recentfile = menu_fichier -> addMenu(QET::Icons::DocumentOpenRecent, tr("&Récemment ouverts"));
	recentfile->addActions(QETApp::projectsRecentFiles()->menu()->actions());
	connect(QETApp::projectsRecentFiles(), SIGNAL(fileOpeningRequested(const QString &)),
		this, SLOT(openRecentFile(const QString &)));
	menu_fichier -> addActions(m_actions->m_file_actions_group.actions());
	menu_fichier -> addSeparator();
	//menu_fichier -> addAction(import_diagram);
	menu_fichier -> addAction(m_actions->m_export_to_images);
	menu_fichier -> addAction(m_actions->m_export_to_pdf);
	menu_fichier -> addAction(m_actions->m_print);
	menu_fichier -> addSeparator();
	menu_fichier -> addAction(m_actions->m_quit_editor);

	// menu Edition
	menu_edition -> addAction(m_actions->undo);
	menu_edition -> addAction(m_actions->redo);
	menu_edition -> addSeparator();
	menu_edition -> addAction(m_actions->m_cut);
	menu_edition -> addAction(m_actions->m_copy);
	menu_edition -> addAction(m_actions->m_paste);
	menu_edition -> addSeparator();
	menu_edition -> addActions(m_actions->m_select_actions_group.actions());
	menu_edition -> addSeparator();
	menu_edition -> addActions(m_actions->m_selection_actions_group.actions());
	menu_edition -> addSeparator();
	menu_edition -> addAction(m_actions->m_conductor_reset);
	menu_edition -> addSeparator();
	menu_edition -> addAction(m_actions->m_edit_diagram_properties);
	menu_edition -> addActions(m_actions->m_row_column_actions_group.actions());
	menu_edition -> addSeparator();
	menu_edition -> addActions(m_actions->m_depth_action_group->actions());
	menu_edition -> addSeparator();
	menu_edition -> addAction(m_actions->m_find);
	menu_edition -> addAction(m_actions->m_jump_to_element);

	// menu Projet
	menu_project -> addAction(m_actions->m_project_edit_properties);
	menu_project -> addAction(m_actions->m_project_add_diagram);
	menu_project -> addAction(m_actions->m_remove_diagram_from_project);
	menu_project -> addAction(m_actions->m_clean_project);
	menu_project -> addSeparator();
	menu_project -> addAction(m_actions->m_add_summary);
	menu_project -> addAction(m_actions->m_add_nomenclature);
	menu_project -> addAction(m_actions->m_csv_export);
	menu_project -> addAction(m_actions->m_project_export_conductor_num);
	menu_project -> addAction(m_actions->m_terminal_strip_dialog);
	menu_project -> addAction(m_actions->m_project_terminalBloc);
	menu_project -> addAction(m_actions->m_project_export_wiring_list);
	menu_project -> addAction(m_actions->m_terminal_numbering);
#ifdef QET_EXPORT_PROJECT_DB
	menu_project -> addSeparator();
	menu_project -> addAction(m_actions->m_export_project_db);
#endif

	main_tool_bar         -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils principale"));
	view_tool_bar         -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils Affichage"));
	diagram_tool_bar      -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la barre d'outils Schéma"));
	qdw_pa           -> toggleViewAction() -> setStatusTip(tr("Affiche ou non le panel d'appareils"));
	qdw_undo         -> toggleViewAction() -> setStatusTip(tr("Affiche ou non la liste des modifications"));


	// menu Affichage
	QMenu *projects_view_mode = menu_affichage -> addMenu(QET::Icons::ConfigureToolbars, tr("Afficher les projets"));
	projects_view_mode -> setTearOffEnabled(true);
	projects_view_mode -> addAction(m_actions->m_windowed_view_mode);
	projects_view_mode -> addAction(m_actions->m_tabbed_view_mode);

	menu_affichage -> addSeparator();
	menu_affichage -> addAction(m_actions->m_mode_selection);
	menu_affichage -> addAction(m_actions->m_mode_visualise);
	menu_affichage -> addSeparator();
	menu_affichage -> addAction(m_actions->m_draw_grid);
	menu_affichage -> addAction(m_actions->m_grey_background);
	menu_affichage -> addSeparator();
	menu_affichage -> addActions(m_actions->m_zoom_actions_group.actions());

	// menu Fenetres
	slot_updateWindowsMenu();
}

/**
	Permet de quitter l'application lors de la fermeture de la fenetre principale
	@param qce Le QCloseEvent correspondant a l'evenement de fermeture
*/
void QETDiagramEditor::closeEvent(QCloseEvent *qce)
{
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
	@brief QETDiagramEditor::event
	Reimplemented to :
	-Load elements collection when WindowActivate.
	@param e
	@return
*/
bool QETDiagramEditor::event(QEvent *e)
{
	if (m_first_show && e->type() == QEvent::WindowActivate)
	{
		m_first_show = false;
		QTimer::singleShot(250, m_element_collection_widget, SLOT(reload()));
	}
	return(QETMainWindow::event(e));
}

/**
	@brief QETDiagramEditor::save
	Ask the current active project to save
*/
void QETDiagramEditor::save()
{
	if (ProjectView *project_view = currentProjectView()) {
		QETResult saved = project_view -> save();

		if (saved.isOk()) {
			//save_file -> setDisabled(true);
			QETApp::projectsRecentFiles() -> fileWasOpened(project_view -> project() -> filePath());

			QString title = (project_view -> project() -> title ());
			if (title.isEmpty()) title = "QElectroTech ";
			QString filePath = (project_view -> project() -> filePath ());
			statusBar()-> showMessage(tr("Projet %1 enregistré dans le repertoire: %2.").arg(title).arg (filePath), 2000);
			m_element_collection_widget->highlightUnusedElement();
		}
		else {
			showError(saved);
		}
	}
}

/**
	@brief QETDiagramEditor::saveAs
	Ask the current active project to save as
*/
void QETDiagramEditor::saveAs()
{
	if (ProjectView *project_view = currentProjectView()) {
		QETResult save_file = project_view -> saveAs();
		if (save_file.isOk()) {
			QETApp::projectsRecentFiles() -> fileWasOpened(project_view -> project() -> filePath());

			QString title = (project_view -> project() -> title ());
			if (title.isEmpty()) title = "QElectroTech ";
			QString filePath = (project_view -> project() -> filePath ());
			statusBar()->showMessage(tr("Projet %1 enregistré dans le repertoire: %2.").arg(title).arg (filePath), 2000);
			m_element_collection_widget->highlightUnusedElement();
		}
		else {
			showError(save_file);
		}
	}
}

/**
	@brief QETDiagramEditor::newProject
	Create a new project with an empty diagram
	@return
*/
bool QETDiagramEditor::newProject()
{
	auto new_project = new QETProject(this);

	// add new diagram
	new_project -> addNewDiagram();

	return addProject(new_project);
}

/**
	Slot utilise pour ouvrir un fichier recent.
	Transfere filepath au slot openAndAddDiagram seulement si cet editeur est
	actif
	@param filepath Fichier a ouvrir
	@see openAndAddDiagram
*/
bool QETDiagramEditor::openRecentFile(const QString &filepath)
{
	// small hack to prevent all diagram editors from trying to topen the required
	// recent file at the same time
	if (qApp -> activeWindow() != this) return(false);
	return(openAndAddProject(filepath));
}

/**
	Cette fonction demande un nom de fichier a ouvrir a l'utilisateur
	@return true si l'ouverture a reussi, false sinon
*/
bool QETDiagramEditor::openProject()
{
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
bool QETDiagramEditor::closeProject(ProjectView *project_view)
{
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
bool QETDiagramEditor::closeProject(QETProject *project)
{
	if (ProjectView *project_view = findProject(project)) {
		return(closeProject(project_view));
	}
	return(true);
}

/**
	Ouvre un projet depuis un fichier et l'ajoute a cet editeur
	@param filepath Chemin du projet a ouvrir
	@param interactive true pour afficher des messages a l'utilisateur, false sinon
	@return true si l'ouverture a reussi, false sinon
*/
bool QETDiagramEditor::openAndAddProject(
		const QString &filepath,
		bool interactive)
{
	if (filepath.isEmpty()) return(false);

	QFileInfo filepath_info(filepath);

	//Check if project is not open in another editor
	if (QETDiagramEditor *diagram_editor = QETApp::diagramEditorForFile(filepath))
	{
		if (diagram_editor == this)
		{
			if (ProjectView *project_view = viewForFile(filepath))
			{
				activateWidget(project_view);
				show();
				activateWindow();
			}
			return(false);
		}
		else
		{
				//Ask to the other editor to display the file
			return(diagram_editor -> openAndAddProject(filepath));
		}
	}

	// check the file exists
	if (!filepath_info.exists())
	{
		if (interactive)
		{
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

	//Check if file readable
	if (!filepath_info.isReadable())
	{
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

	//Check if file is read only
	if (!filepath_info.isWritable())
	{
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

	//Create the project
	DialogWaiting::instance(this);

		//Per-project window for the font counters reported below; the folios
		//(and with them the stored font descriptions) are built between here
		//and the end of addProject(). RAII, because DialogWaiting pumps the
		//event loop during the load: a nested openAndAddProject() gets its
		//own window and this one resumes unharmed.
	QETUtils::FontRestorationScope font_scope;

	QETProject *project = new QETProject(filepath);
	if (project -> state() != QETProject::Ok)
	{
		if (interactive && project -> state() != QETProject::FileOpenDiscard)
		{
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
		DialogWaiting::dropInstance();
		return(false);
	}

	QETApp::projectsRecentFiles() -> fileWasOpened(filepath);
	addProject(project);
	DialogWaiting::dropInstance();

		//Report font descriptions which could not be read as-is (written by
		//an incompatible Qt version or corrupted), so the user learns about
		//it from somewhere else than the console. See issue #553.
	const int salvaged_fonts = font_scope.salvaged();
	const int unreadable_fonts = font_scope.unreadable();
	if (salvaged_fonts || unreadable_fonts)
	{
		qInfo().nospace() << "Project font descriptions: "
				  << salvaged_fonts << " salvaged from a foreign format, "
				  << unreadable_fonts << " unreadable (default font applies)";
	}
	if (interactive && (salvaged_fonts || unreadable_fonts))
	{
		QStringList details;
		if (salvaged_fonts) {
			details << tr("%n description(s) de police écrite(s) dans un "
					  "format étranger ou corrompu ont été restaurée(s). "
					  "Elles seront réécrites dans un format stable au "
					  "prochain enregistrement du projet.",
					  "message box content",
					  salvaged_fonts);
		}
		if (unreadable_fonts) {
			details << tr("%n description(s) de police n'ont pas pu être "
					  "lue(s) ; la police par défaut sera utilisée pour "
					  "ces textes.",
					  "message box content",
					  unreadable_fonts);
		}
		QET::QetMessageBox::information(
			this,
			tr("Polices du projet", "message box title"),
			details.join("\n\n")
		);
	}

	BackupDialog backup_dialog(this);
	if (backup_dialog.exec() == QDialog::Accepted)
	{
		QString backup_path = filepath_info.absolutePath() + QDir::separator() +
			QDateTime::currentDateTime().toString("yyyy-MM-dd-hh-mm") + "_" +
			filepath_info.fileName();
		QFile::copy(filepath, backup_path);
	}

	return true;
}

/**
	Ajoute un projetmoveDiagramUp(
	@param project projet a ajouter
	@param update_panel Whether the elements panel should be warned this
	project has been added. Defaults to true.
*/
bool QETDiagramEditor::addProject(QETProject *project, bool update_panel)
{
	// enregistre le projet
	QETApp::registerProject(project);

	// cree un ProjectView pour visualiser le projet
	ProjectView *project_view = new ProjectView(project);
	addProjectView(project_view);

	undo_group.addStack(project -> undoStack());

	m_element_collection_widget->addProject(project);

	// met a jour le panel d'elements
	if (update_panel) {
		pa -> elementsPanel().projectWasOpened(project);
		if (currentDiagramView() != nullptr)
		m_autonumbering_dock->setProject(project, project_view);
	}

	return(true);
}

/**
	@return la liste des projets ouverts dans cette fenetre
*/
QList<ProjectView *> QETDiagramEditor::openedProjects() const
{
	QList<ProjectView *> result;
	QList<QMdiSubWindow *> window_list(m_workspace.subWindowList());
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
ProjectView *QETDiagramEditor::currentProjectView() const
{
	QMdiSubWindow *current_window = m_workspace.activeSubWindow();
	if (!current_window) return(nullptr);

	QWidget *current_widget = current_window -> widget();
	if (!current_widget) return(nullptr);

	if (ProjectView *project_view = qobject_cast<ProjectView *>(current_widget)) {
		return(project_view);
	}
	return(nullptr);
}

/**
	@brief QETDiagramEditor::currentProject
	@return the current edited project.
	This function can return nullptr.
*/
QETProject *QETDiagramEditor::currentProject() const
{
	ProjectView *view = currentProjectView();
	if (view) {
		return view->project();
	}
	else {
		return nullptr;
	}
}

/**
	@return Le schema actuellement edite (= l'onglet ouvert dans le projet
	courant) ou 0 s'il n'y en a pas
*/
DiagramView *QETDiagramEditor::currentDiagramView() const
{
	if (ProjectView *project_view = currentProjectView()) {
		return(project_view -> currentDiagram());
	}
	return(nullptr);
}

/**
	@return the selected element in the current diagram view, or 0 if:
	  * no diagram is being viewed in this editor.
	  * no element is selected
	  * more than one element is selected
*/
Element *QETDiagramEditor::currentElement() const
{
	DiagramView *dv = currentDiagramView();
	if (!dv)
		return(nullptr);

	QList<Element *> selected_elements = DiagramContent(dv->diagram()).m_elements;
	if (selected_elements.count() != 1)
		return(nullptr);

	return(selected_elements.first());
}

/**
	Cette methode permet de retrouver le projet contenant un schema donne.
	@param diagram_view Schema dont il faut retrouver
	@return la vue sur le projet contenant ce schema ou 0 s'il n'y en a pas
*/
ProjectView *QETDiagramEditor::findProject(DiagramView *diagram_view) const
{
	foreach(ProjectView *project_view, openedProjects()) {
		if (project_view -> diagram_views().contains(diagram_view)) {
			return(project_view);
		}
	}
	return(nullptr);
}

/**
	Cette methode permet de retrouver le projet contenant un schema donne.
	@param diagram Schema dont il faut retrouver
	@return la vue sur le projet contenant ce schema ou 0 s'il n'y en a pas
*/
ProjectView *QETDiagramEditor::findProject(Diagram *diagram) const
{
	foreach(ProjectView *project_view, openedProjects()) {
		foreach(DiagramView *diagram_view, project_view -> diagram_views()) {
			if (diagram_view -> diagram() == diagram) {
				return(project_view);
			}
		}
	}
	return(nullptr);
}

/**
	@param project Projet dont il faut trouver la vue
	@return la vue du projet passe en parametre
*/
ProjectView *QETDiagramEditor::findProject(QETProject *project) const
{
	foreach(ProjectView *opened_project, openedProjects()) {
		if (opened_project -> project() == project) {
			return(opened_project);
		}
	}
	return(nullptr);
}

/**
	@param filepath Chemin de fichier d'un projet
	@return le ProjectView correspondant au chemin passe en parametre, ou 0 si
	celui-ci n'a pas ete trouve
*/
ProjectView *QETDiagramEditor::findProject(const QString &filepath) const
{
	foreach(ProjectView *opened_project, openedProjects()) {
		if (QETProject *project = opened_project -> project()) {
			if (project -> filePath() == filepath) {
				return(opened_project);
			}
		}
	}
	return(nullptr);
}

/**
	@param widget Widget a rechercher dans la zone MDI
	@return La sous-fenetre accueillant le widget passe en parametre, ou 0 si
	celui-ci n'a pas ete trouve.
*/
QMdiSubWindow *QETDiagramEditor::subWindowForWidget(QWidget *widget) const
{
	foreach(QMdiSubWindow *sub_window, m_workspace.subWindowList()) {
		if (sub_window -> widget() == widget) {
			return(sub_window);
		}
	}
	return(nullptr);
}

/**
	@param widget Widget a activer
*/
void QETDiagramEditor::activateWidget(QWidget *widget) {
	QMdiSubWindow *sub_window = subWindowForWidget(widget);
	if (sub_window) {
		m_workspace.setActiveSubWindow(sub_window);
	}
}

void QETDiagramEditor::zoomGroupTriggered(QAction *action)
{
	QString value = action->data().toString();
	DiagramView *dv = currentDiagramView();

	if (!dv || value.isEmpty()) return;

	if (value == "zoom_in")
		dv->zoom(1.15);
	else if (value == "zoom_out")
		dv->zoom(0.85);
	else if (value == "zoom_content")
		dv->zoomContent();
	else if (value == "zoom_fit")
		dv->zoomFit();
	else if (value == "zoom_reset")
		dv->zoomReset();
}

/**
	@brief QETDiagramEditor::selectGroupTriggered
	This slot is called when selection need to change.
	@param action : Action that describes what to do.
*/
void QETDiagramEditor::selectGroupTriggered(QAction *action)
{
	if (!currentDiagramView() || !currentDiagramView()->diagram())
		return;

	auto value = action->data().toString();
	if (value.isEmpty())
		return;

	auto diagram = currentDiagramView()->diagram();

	if (value == "select_all")
		diagram->selectAll();
	else if (value == "deselect")
		diagram->deselectAll();
	else if (value == "invert_selection")
		diagram->invertSelection();
}

/**
	@brief QETDiagramEditor::addItemGroupTriggered
	This slot is called when an item must be added to the current diagram,
	this slot use the DVEventInterface to add item
	@param action : Action that describe the item to add.
*/
void QETDiagramEditor::addItemGroupTriggered(QAction *action)
{
	QString value = action->data().toString();

	if (Q_UNLIKELY (!currentDiagramView() || !currentDiagramView()->diagram() || value.isEmpty())) return;

	Diagram *d = currentDiagramView()->diagram();
	DiagramEventInterface *diagram_event = nullptr;

	if (value == "line")
		diagram_event = new DiagramEventAddShape (d, QetShapeItem::Line);
	else if (value == "rectangle")
		diagram_event = new DiagramEventAddShape (d, QetShapeItem::Rectangle);
	else if (value == "ellipse")
		diagram_event = new DiagramEventAddShape (d, QetShapeItem::Ellipse);
	else if (value == "polyline")
	{
		diagram_event = new DiagramEventAddShape (d, QetShapeItem::Polygon);
		statusBar()-> showMessage(tr("Double-click pour terminer la forme, Click droit pour annuler le dernier point"));
		connect(diagram_event, &DiagramEventInterface::destroyed, [this]() {
		statusBar()->clearMessage();
		});
	}
	else if (value == "image")
	{
		DiagramEventAddImage *deai = new DiagramEventAddImage(d);
		if (deai->isNull())
		{
			delete deai;
			return;
		}
		else
			diagram_event = deai;
	}
	else if (value == "text")
	{
		diagram_event = new DiagramEventAddText(d);
	}
	else if (value == QLatin1String("terminal_strip"))
	{
		const auto diagram_view{currentDiagramView()};
		if (diagram_view)
		{
			AddTerminalStripItemDialog::openDialog(diagram_view->diagram(), this);
		}
	}

	if (diagram_event)
	{
		d->setEventInterface(diagram_event);
		connect(diagram_event, &DiagramEventInterface::destroyed, [action]() {action->setChecked(false);});
	}
}

/**
	@brief QETDiagramEditor::selectionGroupTriggered
	This slot is called when an action should be made on the current selection
	@param action : Action that describe the action to do.
*/
void QETDiagramEditor::selectionGroupTriggered(QAction *action)
{
	QString value = action->data().toString();
	DiagramView *dv = currentDiagramView();
	Diagram *diagram = dv->diagram();
	DiagramContent dc(diagram);

	if (!dv || value.isEmpty()) return;

        if (value == "delete_selection")
        {
            if (DeleteQGraphicsItemCommand::hasNonDeletableTerminal(dc)) {
                QET::QetMessageBox::information(this,
                                                tr("Suppression de borne impossible"),
                                                tr("La suppression ne peut être effectué car la selection "
												   "possède une ou plusieurs bornes ponté et/ou appartenant à une borne à niveau multiple.\n"
                                                   "Déponter et/ou supprimer les niveaux des bornes concerné "
                                                   "afin de pouvoir les supprimer"));
            } else {
                diagram->clearSelection();
                diagram->undoStack().push(new DeleteQGraphicsItemCommand(diagram, dc));
                dv->adjustSceneRect();
            }
        }
	else if (value == "rotate_selection")
	{
		RotateSelectionCommand *c = new RotateSelectionCommand(diagram);
		if(c->isValid())
			diagram->undoStack().push(c);
	}
	else if (value == "rotate_selected_text")
		diagram->undoStack().push(new RotateTextsCommand(diagram));
	else if (value == "find_selected_element" && currentElement())
		findElementInPanel(currentElement()->location());
	else if (value == "edit_selected_element")
		dv->editSelection();
	else if (value == "group_selected_texts")
	{
		QList<DynamicElementTextItem *> deti_list = dc.m_element_texts.values();
		if(deti_list.size() <= 1)
			return;

		diagram->undoStack().push(new AddTextsGroupCommand(deti_list.first()->parentElement(), tr("Groupe"), deti_list));
	}
}

void QETDiagramEditor::rowColumnGroupTriggered(QAction *action)
{
	QString value = action->data().toString();
	DiagramView *dv = currentDiagramView();

	if (!dv || value.isEmpty() || dv->diagram()->isReadOnly()) return;

	Diagram *d = dv->diagram();
	BorderProperties old_bp = d->border_and_titleblock.exportBorder();
	BorderProperties new_bp = d->border_and_titleblock.exportBorder();

	if (value == "add_column")
		new_bp.columns_count += 1;
	else if (value == "remove_column")
		new_bp.columns_count -= 1;
	else if (value == "add_row")
		new_bp.rows_count += 1;
	else if (value == "remove_row")
		new_bp.rows_count -= 1;

	d->undoStack().push(new ChangeBorderCommand(d, old_bp, new_bp));
}

/**
	@brief QETDiagramEditor::slot_updateActions
	Manage actions
*/
void QETDiagramEditor::slot_updateActions()
{
	DiagramView *dv = currentDiagramView();
	ProjectView *pv = currentProjectView();

	bool opened_project = pv;
	bool opened_diagram = dv;
	bool editable_project = (pv && !pv -> project() -> isReadOnly());

	m_actions->m_close_file->                  setEnabled(opened_project);
	m_actions->m_save_file->                   setEnabled(opened_project);
	m_actions->m_save_file_as->                setEnabled(opened_project);
	m_actions->m_rotate_texts->                setEnabled(editable_project);
	m_actions->m_export_to_images->            setEnabled(opened_diagram);
	m_actions->m_print->                       setEnabled(opened_diagram);
	m_actions->m_export_to_pdf->               setEnabled(opened_diagram);
	m_actions->m_edit_diagram_properties->     setEnabled(opened_diagram);
	m_actions->m_zoom_actions_group.           setEnabled(opened_diagram);
	m_actions->m_select_actions_group.         setEnabled(opened_diagram);
	m_actions->m_add_item_actions_group.       setEnabled(editable_project);
	m_actions->m_row_column_actions_group.     setEnabled(editable_project);
	m_actions->m_grey_background->             setEnabled(opened_diagram);
	m_actions->m_draw_grid->                   setEnabled(opened_diagram);
	m_actions->m_draw_guides->                 setEnabled(opened_diagram);

		//Project menu
	m_actions->m_project_edit_properties     -> setEnabled(opened_project);
	m_actions->m_project_add_diagram         -> setEnabled(editable_project);
	m_actions->m_remove_diagram_from_project -> setEnabled(editable_project);
	m_actions->m_clean_project               -> setEnabled(editable_project);
	m_actions->m_add_summary                 -> setEnabled(editable_project);
	m_actions->m_add_nomenclature            -> setEnabled(editable_project);
	m_actions->m_csv_export                  -> setEnabled(editable_project);
	m_actions->m_project_export_conductor_num-> setEnabled(opened_project);
	m_actions->m_terminal_strip_dialog       -> setEnabled(editable_project);
	m_actions->m_project_export_wiring_list  -> setEnabled(opened_project);
	m_actions->m_terminal_numbering          -> setEnabled(editable_project);
#ifdef QET_EXPORT_PROJECT_DB
	m_actions->m_export_project_db           -> setEnabled(editable_project);
#endif
	m_actions->m_project_terminalBloc        -> setEnabled(editable_project);


	slot_updateUndoStack();
	slot_updateModeActions();
	slot_updatePasteAction();
	slot_updateComplexActions();
	slot_updateAutoNumDock();
}

/**
	@brief QETDiagramEditor::slot_updateAutoNumDock
	Update Auto Num Dock Widget when changing Project
*/
void QETDiagramEditor::slot_updateAutoNumDock()
{
	if ( m_workspace.subWindowList().indexOf(m_workspace.activeSubWindow()) != activeSubWindowIndex) {
			activeSubWindowIndex = m_workspace.subWindowList().indexOf(m_workspace.activeSubWindow());
			if (currentProjectView() != nullptr && currentDiagramView() != nullptr) {
				m_autonumbering_dock->setProject(currentProjectView()->project(),currentProjectView());
			}
	}
}

/**
	@brief QETDiagramEditor::slot_updateUndoStack
	Update the undo stack view
*/
void QETDiagramEditor::slot_updateUndoStack()
{
	if(currentProjectView())
		undo_group.setActiveStack(currentProjectView()->project()->undoStack());
}

/**
	@brief QETDiagramEditor::slot_updateComplexActions
	Manage the actions that need some conditions to be enabled or not.
	This method does nothing if there is no project opened
*/
void QETDiagramEditor::slot_updateComplexActions()
{
	DiagramView *dv = currentDiagramView();
	if(!dv)
	{
		QList <QAction *> action_list;
		action_list << m_actions->m_conductor_reset
			    << m_actions->m_find_element
			    << m_actions->m_cut
			    << m_actions->m_copy
			    << m_actions->m_delete_selection
			    << m_actions->m_rotate_selection
			    << m_actions->m_edit_selection
			    << m_actions->m_group_selected_texts;
		for(QAction *action : action_list)
			action->setEnabled(false);

		return;
	}

	Diagram *diagram_ = dv->diagram();
	DiagramContent dc(diagram_);
	bool ro = diagram_->isReadOnly();


	//Number of selected conductors
	int selected_conductors_count = diagram_->selectedConductors().count();
	m_actions->m_conductor_reset->setEnabled(!ro && selected_conductors_count);

	// number of selected elements
	int selected_elements_count = dc.count(DiagramContent::Elements);
	m_actions->m_find_element->setEnabled(selected_elements_count == 1);

	//Actions that need items (elements, conductors, texts...) selected, to be enabled
	bool copiable_items  = dc.hasCopiableItems();
	bool deletable_items = dc.hasDeletableItems();
	m_actions->m_cut              -> setEnabled(!ro && copiable_items);
	m_actions->m_copy             -> setEnabled(copiable_items);
	m_actions->m_delete_selection -> setEnabled(!ro && deletable_items);
	m_actions->m_rotate_selection -> setEnabled(!ro && diagram_->canRotateSelection());

		//Action that need selected texts or texts group
	QList<DiagramTextItem *> texts = DiagramContent(diagram_).selectedTexts();
	QList<ElementTextItemGroup *> groups = DiagramContent(diagram_).selectedTextsGroup();
	int selected_texts = texts.count();
	int selected_conductor_texts   = 0;
	for(DiagramTextItem *dti : texts)
	{
		if(dti->type() == ConductorTextItem::Type)
			selected_conductor_texts++;
	}
	int selected_dynamic_elmt_text = 0;
	for(DiagramTextItem *dti : texts)
	{
		if(dti->type() == DynamicElementTextItem::Type)
			selected_dynamic_elmt_text++;
	}
	m_actions->m_rotate_texts->setEnabled(!ro && (selected_texts || groups.size()));

	//Action that need only element text selected
	QList<DynamicElementTextItem *> deti_list = dc.m_element_texts.values();
	if(deti_list.size() > 1 && dc.count() == deti_list.count())
	{
		Element *elmt = deti_list.first()->parentElement();
		bool ok = true;
		for(DynamicElementTextItem *deti : deti_list)
		{
			if(elmt != deti->parentElement())
				ok = false;
		}
		m_actions->m_group_selected_texts->setEnabled(!ro && ok);
	}
	else
		m_actions->m_group_selected_texts->setDisabled(true);

	// actions need only one editable item
	int selected_image = dc.count(DiagramContent::Images);

	int selected_shape = dc.count(DiagramContent::Shapes);
	int selected_editable = selected_elements_count
			+ (selected_texts
			   - selected_conductor_texts
			   - selected_dynamic_elmt_text)
			+ selected_image
			+ selected_shape
			+ selected_conductors_count;

	if (selected_editable == 1)
	{
		m_actions->m_edit_selection -> setEnabled(true);
		//edit element
		if (selected_elements_count)
		{
			m_actions->m_edit_selection -> setText(tr("Éditer l'élement",
						       "edit element"));
			m_actions->m_edit_selection -> setIcon(QET::Icons::ElementEdit);
		}
		//edit text field
		else if (selected_texts)
		{
			m_actions->m_edit_selection -> setText(tr("Éditer le champ de texte",
						       "edit text field"));
			m_actions->m_edit_selection -> setIcon(QET::Icons::EditText);
		}
		//edit image
		else if (selected_image)
		{
			m_actions->m_edit_selection -> setText(tr("Éditer l'image",
						       "edit image"));
			m_actions->m_edit_selection -> setIcon(QET::Icons::resize_image);
		}
		//edit conductor
		else if (selected_conductors_count)
		{
			m_actions->m_edit_selection -> setText(tr("Éditer le conducteur",
						       "edit conductor"));
			m_actions->m_edit_selection -> setIcon(QET::Icons::ConductorEdit);
		}
	}
	//not an editable item
	else
	{
		m_actions->m_edit_selection -> setText(tr("Éditer l'objet sélectionné",
					       "edit selected item"));
		m_actions->m_edit_selection -> setIcon(QET::Icons::ElementEdit);
		m_actions->m_edit_selection -> setEnabled(false);
	}

	//Actions for edit Z value
	QList<QGraphicsItem *> list = dc.items(
				DiagramContent::SelectedOnly
				| DiagramContent::Elements
				| DiagramContent::Shapes
				| DiagramContent::Images);
	m_actions->m_depth_action_group->setEnabled(list.isEmpty()? false : true);
}

/**
	@brief QETDiagramEditor::slot_updateModeActions
	Manage action who need an opened diagram or project to be updated
*/
void QETDiagramEditor::slot_updateModeActions()
{
	DiagramView *dv = currentDiagramView();

	if (!dv)
		m_actions->grp_visu_sel -> setEnabled(false);
	else
	{
		switch((int)(dv -> dragMode()))
		{
			case QGraphicsView::NoDrag:
				m_actions->grp_visu_sel -> setEnabled(false);
				break;
			case QGraphicsView::ScrollHandDrag:
				m_actions->grp_visu_sel -> setEnabled(true);
				m_actions->m_mode_visualise -> setChecked(true);
				break;
			case QGraphicsView::RubberBandDrag:
				m_actions->grp_visu_sel -> setEnabled(true);
				m_actions->m_mode_selection -> setChecked(true);
				break;
		}
	}

	if (ProjectView *pv = currentProjectView())
	{
		m_actions->m_auto_conductor -> setEnabled (true);
		m_actions->m_auto_conductor -> setChecked (pv -> project() -> autoConductor());
	}
	else
		m_actions->m_auto_conductor -> setDisabled(true);
}

/**
	@brief QETDiagramEditor::slot_updatePasteAction
	Gere les actions ayant besoin du presse-papier
*/
void QETDiagramEditor::slot_updatePasteAction()
{
	DiagramView *dv = currentDiagramView();
	bool editable_diagram = (dv && !dv -> diagram() -> isReadOnly());

	// pour coller, il faut un schema ouvert et un schema dans le presse-papier
	m_actions->m_paste -> setEnabled(editable_diagram && Diagram::clipboardMayContainDiagram());
}

/**
	@brief QETDiagramEditor::addProjectView
	Add a new project view to workspace and build the connection between
	the projectview / project and this QETDiagramEditor.
	@param project_view : project view to add
*/
void QETDiagramEditor::addProjectView(ProjectView *project_view)
{
	if (!project_view) return;

	foreach(DiagramView *dv, project_view -> diagram_views())
		diagramWasAdded(dv);

	//Manage the close event of project
	connect(project_view, SIGNAL(projectClosed(ProjectView*)),
		this, SLOT(projectWasClosed(ProjectView *)));
	//Manage the adding  of diagram
	connect(project_view, SIGNAL(diagramAdded(DiagramView *)),
		this, SLOT(diagramWasAdded(DiagramView *)));

	if (QETProject *project = project_view -> project())
		connect(project, SIGNAL(readOnlyChanged(QETProject *, bool)),
			this, SLOT(slot_updateActions()));

	//Manage request for edit or find element and titleblock
	connect (project_view, &ProjectView::findElementRequired,
		 this, &QETDiagramEditor::findElementInPanel);

	// display error messages sent by the project view
	connect(project_view, SIGNAL(errorEncountered(QString)),
		this, SLOT(showError(const QString &)));

	//Highlight the current page
	connect(project_view, &ProjectView::diagramActivated, this, [this](DiagramView *dv) {
		if (dv && dv->diagram() && pa) {
			// 1. Find the item in the tree that corresponds to this diagram
			QTreeWidgetItem *item = pa->elementsPanel().getItemForDiagram(dv->diagram());

				   // 2. If you find it, select it
			if (item) {
				pa->elementsPanel().setCurrentItem(item);
			}
		}
	});

		//Highlight the current page in projectView on project activation
	connect(this, &QETDiagramEditor::syncElementsPanel, this, [this]() {
		if (pa && currentDiagramView()) {
				// In the tree, find the element that corresponds to the diagram of the selected project.
			QTreeWidgetItem *item = pa->elementsPanel().getItemForDiagram(currentDiagramView()->diagram());
			if (item) {
					// select the diagram
				pa->elementsPanel().setCurrentItem(item);
			}
		}
	});

	//We maximise the new window if the current window is inexistent or maximized
	QWidget *current_window = m_workspace.activeSubWindow();
	bool maximise = ((!current_window)
			 || (current_window -> windowState()
			     & Qt::WindowMaximized));

		//Add the new window
	QMdiSubWindow *sub_window = m_workspace.addSubWindow(project_view);
	sub_window -> setWindowIcon(project_view -> windowIcon());
	sub_window -> systemMenu() -> clear();

	//By default QMdiSubWindow have a QAction "close" with shortcut QKeySequence::Close
	//But the QAction m_close_file of this class have the same shortcut too.
	//We remove the shortcut of the QAction of QMdiSubWindow for avoid conflic
	for(QAction *act : sub_window->actions())
	{
		if(act->shortcut() == QKeySequence::Close)
			act->setShortcut(QKeySequence());
	}

		//Display the new window
	if (maximise) project_view -> showMaximized();
	else          project_view -> show();
}

/**
	@return la liste des fichiers edites par cet editeur de schemas
*/
QList<QString> QETDiagramEditor::editedFiles() const
{
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
ProjectView *QETDiagramEditor::viewForFile(const QString &filepath) const
{
	if (filepath.isEmpty()) return(nullptr);

	QString searched_can_file_path = QFileInfo(filepath).canonicalFilePath();
	if (searched_can_file_path.isEmpty()) {
		// QFileInfo returns an empty path for non-existent files
		return(nullptr);
	}
	foreach (ProjectView *project_view, openedProjects()) {
		QString project_can_file_path = QFileInfo(project_view -> project() -> filePath()).canonicalFilePath();
		if (project_can_file_path == searched_can_file_path) {
			return(project_view);
		}
	}
	return(nullptr);
}

/**
	@brief QETDiagramEditor::drawGrid
	@return true if the grid of folio must be displayed
*/
bool QETDiagramEditor::drawGrid() const
{
	return m_actions->m_draw_grid->isChecked();
}

/**
	@brief QETDiagramEditor::openBackupFiles
	@param backup_files
*/
void QETDiagramEditor::openBackupFiles(QList<KAutoSaveFile *> backup_files)
{
	for (KAutoSaveFile *file : backup_files)
	{
			//Create the project
		DialogWaiting::instance(this);

		QETProject *project = new QETProject(file, this);
		if (project->state() != QETProject::Ok)
		{
			if (project -> state() != QETProject::FileOpenDiscard)
			{
				QET::QetMessageBox::warning(
					this,
					tr("Échec de l'ouverture du projet", "message box title"),
					QString(tr(
						"Une erreur est survenue lors de l'ouverture du fichier %1.",
						"message box content")).arg(file->managedFile().fileName()));
			}
			delete project;
			DialogWaiting::dropInstance();
		}
		addProject(project);
		DialogWaiting::dropInstance();
	}
}
/**
	met a jour le menu "Fenetres"
*/
void QETDiagramEditor::slot_updateWindowsMenu()
{
	// nettoyage du menu
	foreach(QAction *a, windows_menu -> actions()) windows_menu -> removeAction(a);

	// actions de fermeture
	windows_menu -> addAction(m_actions->m_close_file);
	//windows_menu -> addAction(closeAllAct);

	// actions de reorganisation des fenetres
	windows_menu -> addSeparator();
	windows_menu -> addAction(m_actions->m_tile_window);
	windows_menu -> addAction(m_actions->m_cascade_window);

	// actions de deplacement entre les fenetres
	windows_menu -> addSeparator();
	windows_menu -> addAction(m_actions->m_next_window);
	windows_menu -> addAction(m_actions->m_previous_window);

	// liste des fenetres
	QList<ProjectView *> windows = openedProjects();

	m_actions->m_tile_window    -> setEnabled(!windows.isEmpty() && m_workspace.viewMode() == QMdiArea::SubWindowView);
	m_actions->m_cascade_window -> setEnabled(!windows.isEmpty() && m_workspace.viewMode() == QMdiArea::SubWindowView);
	m_actions->m_next_window    -> setEnabled(windows.count() > 1);
	m_actions->m_previous_window    -> setEnabled(windows.count() > 1);

	if (!windows.isEmpty()) windows_menu -> addSeparator();
	QActionGroup *windows_actions = new QActionGroup(this);
	foreach(ProjectView *project_view, windows) {
		QString pv_title = project_view -> windowTitle();
		QAction *action  = windows_menu -> addAction(pv_title);
		windows_actions -> addAction(action);
		action -> setStatusTip(QString(tr("Active le projet « %1 »")).arg(pv_title));
		action -> setCheckable(true);
		action -> setChecked(project_view == currentProjectView());
		connect(action, SIGNAL(triggered()), &windowMapper, SLOT(map()));
		windowMapper.setMapping(action, project_view);
	}
}

/**
	Edite les proprietes du schema diagram
	@param diagram_view schema dont il faut editer les proprietes
*/
void QETDiagramEditor::editDiagramProperties(DiagramView *diagram_view)
{
	if (ProjectView *project_view = findProject(diagram_view)) {
		activateProject(project_view);
		project_view -> editDiagramProperties(diagram_view);
	}
}

/**
	Edite les proprietes du schema diagram
	@param diagram schema dont il faut editer les proprietes
*/
void QETDiagramEditor::editDiagramProperties(Diagram *diagram)
{
	if (ProjectView *project_view = findProject(diagram)) {
		activateProject(project_view);
		project_view -> editDiagramProperties(diagram);
	}
}

/**
	Affiche les projets dans des fenetres.
*/
void QETDiagramEditor::setWindowedMode()
{
	m_workspace.setViewMode(QMdiArea::SubWindowView);
	m_actions->m_windowed_view_mode -> setChecked(true);
	slot_updateWindowsMenu();
}

/**
	Affiche les projets dans des onglets.
*/
void QETDiagramEditor::setTabbedMode()
{
	m_workspace.setViewMode(QMdiArea::TabbedView);
	m_actions->m_tabbed_view_mode -> setChecked(true);
	slot_updateWindowsMenu();
}

/**
	@brief QETDiagramEditor::readSettings
	Read the settings
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
	@brief QETDiagramEditor::writeSettings
	Write the settings
*/
void QETDiagramEditor::writeSettings()
{
	QSettings settings;
	settings.setValue("diagrameditor/geometry", saveGeometry());
	settings.setValue("diagrameditor/state", saveState());
}

/**
	Active le projet passe en parametre
	@param project Projet a activer
*/
void QETDiagramEditor::activateProject(QETProject *project)
{
	activateProject(findProject(project));
}

/**
	Active le projet passe en parametre
	@param project_view Projet a activer
*/
void QETDiagramEditor::activateProject(ProjectView *project_view)
{
	if (!project_view) return;
	activateWidget(project_view);
}

/**
	@brief QETDiagramEditor::projectWasClosed
	Manage the close of a project.
	@param project_view
*/
void QETDiagramEditor::projectWasClosed(ProjectView *project_view)
{
	QETProject *project = project_view -> project();
	if (project)
	{
		pa -> elementsPanel().projectWasClosed(project);
		m_element_collection_widget->removeProject(project);
		undo_group.removeStack(project -> undoStack());
		QETApp::unregisterProject(project);
	}
	//When project is closed, a lot of signals are emitted, notably if there is an item selected in a diagram.
	//In some special case, since signal/slot connection can be direct or queued, some signals are handled after QObject is deleted, and crash qet
	//notably in the function Diagram::elements when it calls items() (I don't know exactly why).
	//set nullptr to "m_selection_properties_editor->setDiagram()" fixes this crash
	m_selection_properties_editor->setDiagram(nullptr);
	project_view -> deleteLater();
	project -> deleteLater();
}

/**
	Edite les proprietes du projet project_view.
	@param project_view Vue sur le projet dont il faut editer les proprietes
*/
void QETDiagramEditor::editProjectProperties(ProjectView *project_view)
{
	if (!project_view) return;
	activateProject(project_view);
	project_view -> editProjectProperties();
}

/**
	Edite les proprietes du projet project.
	@param project Projet dont il faut editer les proprietes
*/
void QETDiagramEditor::editProjectProperties(QETProject *project)
{
	editProjectProperties(findProject(project));
}

/**
	@brief QETDiagramEditor::addDiagramToProject
	Add a diagram to project
	@param project
*/
void QETDiagramEditor::addDiagramToProject(QETProject *project)
{
	if (!project) {
		return;
	}

	if (ProjectView *project_view = findProject(project))
	{
		activateProject(project);
		project_view->project()->addNewDiagram();
	}
}
/**
 * @brief QETDiagramEditor::removeDiagram
 * Wrapper für einzelne Diagramme, um Abwärtskompatibilität zu erhalten.
 */
void QETDiagramEditor::removeDiagram(Diagram *diagram)
{
	if (!diagram) return;
	QList<Diagram *> list;
	list << diagram;
	removeDiagrams(list);
}

/**
 * @brief QETDiagramEditor::removeDiagrams
 * Deletes a list of folios with a single query.
 */
void QETDiagramEditor::removeDiagrams(const QList<Diagram *> &diagrams)
{
	if (diagrams.isEmpty()) return;

	if (diagrams.count() == 1) {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, tr("Supprimer le folio"),
									  tr("Êtes-vous sûr de vouloir supprimer ce folio ?"),
									  QMessageBox::Yes | QMessageBox::No);
		if (reply == QMessageBox::No) return;
	} else {
		QMessageBox::StandardButton reply;
		reply = QMessageBox::question(this, tr("Supprimer les folios"),
									  tr("Êtes-vous sûr de vouloir supprimer les %1 folios sélectionnés ?").arg(diagrams.count()),
									  QMessageBox::Yes | QMessageBox::No);
		if (reply == QMessageBox::No) return;
	}

	ProjectView *project_view = nullptr;
	QETProject *project = diagrams.first()->project();
	if (project) {
		project_view = findProject(project);
	}

	if (project_view) project_view->setUpdatesEnabled(false);
	if (pa) pa->setUpdatesEnabled(false);

	if (project) {
		project->undoStack()->beginMacro(diagrams.count() == 1
			? tr("Supprimer le folio")
			: tr("Supprimer %1 folios").arg(diagrams.count()));
	}

	foreach (Diagram *diagram, diagrams) {
		removeDiagramSilent(diagram);
	}

	if (project) project->undoStack()->endMacro();

	if (pa) pa->setUpdatesEnabled(true);
	if (project_view) project_view->setUpdatesEnabled(true);

	emit syncElementsPanel();
}

/**
	Supprime un schema de son projet
	@param diagram Schema a supprimer
*/
void QETDiagramEditor::removeDiagramSilent(Diagram *diagram)
{
	if (!diagram) return;

	if (QETProject *diagram_project = diagram -> project()) {
		if (ProjectView *project_view = findProject(diagram_project)) {

			// supprime le schema
			project_view -> removeDiagram(diagram, true);
		}
	}
}
void QETDiagramEditor::moveDiagramUp(const QList<Diagram *> &diagrams) {
	if (diagrams.isEmpty()) return;
	QList<Diagram *> safeDiagrams = diagrams;
	if (QETProject *diagram_project = safeDiagrams.first()->project()) {
		if (!diagram_project->isReadOnly()) {
			if (ProjectView *project_view = findProject(diagram_project)) {
				// Forward loop for moving up
				diagram_project->undoStack()->beginMacro(tr("Déplacer les folios"));
				for (int i = 0; i < safeDiagrams.size(); ++i) {
					project_view->moveDiagramUp(safeDiagrams.at(i));
					QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
				}
				diagram_project->undoStack()->endMacro();
			}
		}
	}
}

void QETDiagramEditor::moveDiagramDown(const QList<Diagram *> &diagrams) {
	if (diagrams.isEmpty()) return;
	QList<Diagram *> safeDiagrams = diagrams;
	if (QETProject *diagram_project = safeDiagrams.first()->project()) {
		if (!diagram_project->isReadOnly()) {
			if (ProjectView *project_view = findProject(diagram_project)) {
				// Backward loop for moving down
				diagram_project->undoStack()->beginMacro(tr("Déplacer les folios"));
				for (int i = safeDiagrams.size() - 1; i >= 0; --i) {
					project_view->moveDiagramDown(safeDiagrams.at(i));
					QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
				}
				diagram_project->undoStack()->endMacro();
			}
		}
	}
}

void QETDiagramEditor::moveDiagramUpTop(const QList<Diagram *> &diagrams) {
	if (diagrams.isEmpty()) return;
	QList<Diagram *> safeDiagrams = diagrams;
	if (QETProject *diagram_project = safeDiagrams.first()->project()) {
		if (!diagram_project->isReadOnly()) {
			if (ProjectView *project_view = findProject(diagram_project)) {
				// Backward loop to preserve relative order of the selected items when moving to top
				diagram_project->undoStack()->beginMacro(tr("Déplacer les folios"));
				for (int i = safeDiagrams.size() - 1; i >= 0; --i) {
					project_view->moveDiagramUpTop(safeDiagrams.at(i));
					QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
				}
				diagram_project->undoStack()->endMacro();
			}
		}
	}
}

void QETDiagramEditor::moveDiagramUpx10(const QList<Diagram *> &diagrams) {
	if (diagrams.isEmpty()) return;
	QList<Diagram *> safeDiagrams = diagrams;
	if (QETProject *diagram_project = safeDiagrams.first()->project()) {
		if (!diagram_project->isReadOnly()) {
			if (ProjectView *project_view = findProject(diagram_project)) {
				// Forward loop for moving up
				diagram_project->undoStack()->beginMacro(tr("Déplacer les folios"));
				for (int i = 0; i < safeDiagrams.size(); ++i) {
					project_view->moveDiagramUpx10(safeDiagrams.at(i));
					QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
				}
				diagram_project->undoStack()->endMacro();
			}
		}
	}
}

void QETDiagramEditor::moveDiagramDownx10(const QList<Diagram *> &diagrams) {
	if (diagrams.isEmpty()) return;
	QList<Diagram *> safeDiagrams = diagrams;
	if (QETProject *diagram_project = safeDiagrams.first()->project()) {
		if (!diagram_project->isReadOnly()) {
			if (ProjectView *project_view = findProject(diagram_project)) {
				// Backward loop for moving down
				diagram_project->undoStack()->beginMacro(tr("Déplacer les folios"));
				for (int i = safeDiagrams.size() - 1; i >= 0; --i) {
					project_view->moveDiagramDownx10(safeDiagrams.at(i));
					QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
				}
				diagram_project->undoStack()->endMacro();
			}
		}
	}
}

void QETDiagramEditor::moveDiagramUpx100(const QList<Diagram *> &diagrams) {
	if (diagrams.isEmpty()) return;
	QList<Diagram *> safeDiagrams = diagrams;
	if (QETProject *diagram_project = safeDiagrams.first()->project()) {
		if (!diagram_project->isReadOnly()) {
			if (ProjectView *project_view = findProject(diagram_project)) {
				// Forward loop for moving up
				diagram_project->undoStack()->beginMacro(tr("Déplacer les folios"));
				for (int i = 0; i < safeDiagrams.size(); ++i) {
					project_view->moveDiagramUpx100(safeDiagrams.at(i));
					QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
				}
				diagram_project->undoStack()->endMacro();
			}
		}
	}
}

void QETDiagramEditor::moveDiagramDownx100(const QList<Diagram *> &diagrams) {
	if (diagrams.isEmpty()) return;
	QList<Diagram *> safeDiagrams = diagrams;
	if (QETProject *diagram_project = safeDiagrams.first()->project()) {
		if (!diagram_project->isReadOnly()) {
			if (ProjectView *project_view = findProject(diagram_project)) {
				// Backward loop for moving down
				diagram_project->undoStack()->beginMacro(tr("Déplacer les folios"));
				for (int i = safeDiagrams.size() - 1; i >= 0; --i) {
					project_view->moveDiagramDownx100(safeDiagrams.at(i));
					QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
				}
				diagram_project->undoStack()->endMacro();
			}
		}
	}
}

void QETDiagramEditor::reloadOldElementPanel()
{
	pa->reloadAndFilter();
}

/**
	Supprime le schema courant du projet courant
*/
void QETDiagramEditor::removeDiagramFromProject()
{
	if (ProjectView *current_project = currentProjectView()) {
		if (DiagramView *current_diagram = current_project -> currentDiagram()) {
			current_project -> removeDiagram(current_diagram);
		}
	}
}

/**
	@brief QETDiagramEditor::diagramWasAdded
	Manage the adding of diagram view in a project
	@param dv : added diagram view
*/
void QETDiagramEditor::diagramWasAdded(DiagramView *dv)
{
	connect(dv->diagram(),
		&QGraphicsScene::selectionChanged,
		this,
		&QETDiagramEditor::selectionChanged,
		Qt::DirectConnection);
	connect(dv,
		SIGNAL(modeChanged()),
		this,
		SLOT(slot_updateModeActions()));
}

/**
	@brief QETDiagramEditor::findElementInPanel
	Find the item for location in the element panel
	@param location
*/
void QETDiagramEditor::findElementInPanel(const ElementsLocation &location)
{
	m_element_collection_widget->setCurrentLocation(location);
}

/**
	Show the error message contained in \a result.
*/
void QETDiagramEditor::showError(const QETResult &result)
{
	if (result.isOk()) return;
	showError(result.errorMessage());
}

/**
	Show the \a error message.
*/
void QETDiagramEditor::showError(const QString &error)
{
	if (error.isEmpty()) return;
	QET::QetMessageBox::critical(this, tr("Erreur", "message box title"), error);
}

/**
	@brief QETDiagramEditor::subWindowActivated
	Slot used to update menu and undo stack when subwindows of MDIarea was activated
	@param subWindows
*/
void QETDiagramEditor::subWindowActivated(QMdiSubWindow *subWindows)
{
	Q_UNUSED(subWindows)

	slot_updateActions();
	slot_updateWindowsMenu();
	emit syncElementsPanel();
	updateUsageTrackersActiveState();
}

/**
	@brief QETDiagramEditor::updateUsageTrackersActiveState
	Mark the currently active project's usage tracker (time spent on this
	project) as active, and every other opened project's tracker as
	inactive. Called whenever the current MDI subwindow changes.

	Known limitation: this only accounts for tab switches within this
	QETDiagramEditor window. If the same project were ever shown as the
	active tab in two different windows at once, its tracked time could be
	double-counted -- today QETApp only ever gives a project one ProjectView,
	so this doesn't happen in practice.
*/
void QETDiagramEditor::updateUsageTrackersActiveState()
{
	QETProject *active_project = currentProject();
	const QList<ProjectView *> project_views = openedProjects();
	for (ProjectView *project_view : project_views) {
		if (QETProject *project = project_view->project()) {
			project->projectPropertiesHandler().usageTracker().setActive(project == active_project);
		}
	}
}

/**
	@brief QETDiagramEditor::selectionChanged
	This slot is called when a diagram selection was changed.
*/
void QETDiagramEditor::selectionChanged()
{
	slot_updateComplexActions();

	DiagramView *dv = currentDiagramView();
	if (dv && dv->diagram())
		m_selection_properties_editor->setDiagram(dv->diagram());
}


/**
	@brief QETDiagramEditor::generateTerminalBlock
*/
void QETDiagramEditor::generateTerminalBlock()
{
#ifdef TODO_LIST
#	pragma message("@TODO Merge 'qet_tb_generator' code in to Qet")
#	pragma message("https://github.com/qelectrotech/qet_tb_generator")
#endif

	bool success = false;
	QList<QString> exeList;
	QProcess *process = new QProcess(qApp);

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
	exeList << (QETApp::dataDir() + "/binary/qet_tb_generator.exe")
			<< (QDir::currentPath() + "/qet_tb_generator.exe")
			<< QStandardPaths::findExecutable("qet_tb_generator.exe")
			<< "qet_tb_generator.exe"
			<< "qet_tb_generator";    // from original code: missing ".exe" ???
#elif  defined(Q_OS_MACOS)
	exeList << (QETApp::dataDir() + "/binary/qet_tb_generator")
			<< (QDir::currentPath() + "/qet_tb_generator")
			<< QStandardPaths::findExecutable("qet_tb_generator")
			<< (QDir::homePath() + "/.qet/qet_tb_generator.app")
			<< "/Library/Frameworks/Python.framework/Versions/3.11/bin/qet_tb_generator";
#else
	exeList << (QETApp::dataDir() + "/binary/qet_tb_generator")
			<< (QDir::currentPath() + "/qet_tb_generator")
			<< (QDir::homePath() + "/.qet/qet_tb_generator")
			<< QStandardPaths::findExecutable("qet_tb_generator")
			<< "qet_tb_generator";
#endif

		// If launched under control:
		//connect(process, SIGNAL(errorOcurred(int error)), this, SLOT(slot_generateTerminalBlock_error()));
		//process->start("qet_tb_generator");

	qInfo() << " project to use for qet_tb_generator: "
			<< (QETDiagramEditor::currentProjectView()->project()->filePath());

	if (openedProjects().count()) {
		foreach(QString exe, exeList) {
			if ((success == false) && exe.length() && QFile::exists(exe)) {
				success = process->startDetached(exe, {(QETDiagramEditor::currentProjectView()->project()->filePath())});
			}
			if (success == true) {
				qInfo() << " qet_tb_generator found here:" << exe;
				break;
			} else {
				qInfo() << " qet_tb_generator not found :" << exe;
			}
		}
	} else {
		qInfo() << "No project loaded - no need to start \"qet_tb_generator\"";
	}
	process->close();

#if defined(Q_OS_WIN32) || defined(Q_OS_WIN64)
	QString message=QObject::tr(
		"To install the plugin qet_tb_generator"
		"<br>Visit :"
		"<br>"
		"<a href='https://pypi.python.org/pypi/qet-tb-generator'>qet-tb-generator</a>"
		"<br>Requires python 3.5 or above."
		"<br><B><U> First install on Windows</B></U>"
		"<br>1. Install, if required, python 3.5 or above"
		"<br> Visit :"
		"<br>"
		"<a href='https://www.python.org/downloads/'>python.org</a>"
		"<br>2. pip install qet_tb_generator"
		"<br><B><U> Update on Windows</B></U>"
		"<br>python -m pip install --upgrade qet_tb_generator"
		"<br>"
		">>user could launch in a terminal this script in this directory"
		"<br>"
		" C:\\users\\XXXX\\AppData\\Local\\Programs\\Python\\Python36-32\\Scripts   "
		"<br>");
#elif defined(Q_OS_MACOS)
	QString message=QObject::tr(
		"To install the plugin qet_tb_generator"
		"<br>Visit  :"
		"<br>"
		"<a href='https://pypi.python.org/pypi/qet-tb-generator'>qet-tb-generator</a>"
		"<br><B><U> First install on macOSX</B></U>"
		"<br>1. Install, if required, python 3.11 bundle only, "
		"<a href='https://www.python.org/ftp/python/3.11.2/python-3.11.2-macos11.pkg'>python-3.11.2-macos11.pkg</a>"
		"<br>2 Run Profile.command script"
		"<br>"
		"because program use hardcoded PATH for localise qet-tb-generator plugin "
		"<br> Visit :"
		"<br>"
		"<a href='https://qelectrotech.org/forum/viewtopic.php?pid=5674#p5674'>howto</a>"
		"<br>2. pip3 install qet_tb_generator"
		"<br><B><U> Update on macOSX</B></U>"
		"<br> pip3 install --upgrade qet_tb_generator"
		"<br>");
#else
	QString message=QObject::tr(
		"To install the plugin qet_tb_generator"
		"<br>Visit :"
		"<br>"
		"<a href='https://pypi.python.org/pypi/qet-tb-generator'>qet-tb-generator</a>"
		"<br>"
		"<br>Requires python 3.5 or above."
		"<br>"
		"<br><B><U> First install on Linux</B>""</U>"
		"<br>1. check you have pip3 installed: pip3 --version"
		"<br>If not install with: sudo apt-get install python3-pip"
		"<br>2. Install the program: sudo pip3 install qet_tb_generator"
		"<br>3. Run the program: qet_tb_generator"
		"<br>"
		"<br><B>""<U> Update on Linux</B>""</U>"
		"<br>sudo pip3 install --upgrade qet_tb_generator"
		"<br>");
#endif
	if ( !success ) {
		QMessageBox::warning(nullptr,
							 QObject::tr("Error launching qet_tb_generator plugin"),
							 message);
	}
}

/**
 * @brief QETDiagramEditor::slot_terminalNumbering
 * Opens the dialog for automatic terminal numbering and applies the generated undo command.
 */
void QETDiagramEditor::slot_terminalNumbering() {
	TerminalNumberingDialog dialog(this);
	if (dialog.exec() == QDialog::Accepted) {
		QETProject *project = currentProject();
		if (!project) return;

		// Fetch the generated undo command from the dialog logic
		QUndoCommand *macro = dialog.getUndoCommand(project);

		// If changes were made, push them to the global undo stack
		if (macro) {
			undo_group.activeStack()->push(macro);
		}
	}
}
