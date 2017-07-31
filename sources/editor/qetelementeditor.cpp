/*
	Copyright 2006-2017 The QElectroTech Team
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
#include "qetelementeditor.h"
#include "qet.h"
#include "qetapp.h"
#include "elementscene.h"
#include "elementview.h"
#include "customelementpart.h"
#include "newelementwizard.h"
#include "elementitemeditor.h"
#include "elementdialog.h"
#include "recentfiles.h"
#include "qeticons.h"
#include "qetmessagebox.h"

// editeurs de primitives
#include "arceditor.h"
#include "ellipseeditor.h"
#include "lineeditor.h"
#include "polygoneditor.h"
#include "rectangleeditor.h"
#include "terminaleditor.h"
#include "texteditor.h"
#include "textfieldeditor.h"
#include "partterminal.h"
#include "parttextfield.h"
#include "styleeditor.h"

#include "eseventaddline.h"
#include "eseventaddrect.h"
#include "eseventaddellipse.h"
#include "eseventaddpolygon.h"
#include "eseventaddarc.h"
#include "eseventaddtext.h"
#include "eseventaddtextfield.h"
#include "eseventaddterminal.h"

#include <QMessageBox>
#include <QTextStream>
#include <QFileDialog>
#include <QFile>
#include <QModelIndex>

/*
	Nombre maximum de primitives affichees par la "liste des parties"
	Au-dela, un petit message est affiche, indiquant que ce nombre a ete depasse
	et que la liste ne sera donc pas mise a jour.
*/
#define QET_MAX_PARTS_IN_ELEMENT_EDITOR_LIST 200

/**
	Constructeur
	@param parent QWidget parent
*/
QETElementEditor::QETElementEditor(QWidget *parent) :
	QETMainWindow(parent),
	read_only(false),
	min_title(tr("QElectroTech - Éditeur d'élément", "window title")),
	opened_from_file(false)
{
	setWindowTitle(min_title);
	setWindowIcon(QET::Icons::QETLogo);
	
	setupInterface();
	setupActions();
	setupMenus();
	
	// la fenetre est maximisee par defaut
	setMinimumSize(QSize(500, 350));
	setWindowState(Qt::WindowMaximized);
	
	// lecture des parametres
	readSettings();
	slot_updateMenus();
	
	// affichage
	show();
}

/// Destructeur
QETElementEditor::~QETElementEditor() {
	/*
		retire le widget d'edition de primitives affiche par le dock
		cela evite qu'il ne soit supprime par son widget parent
	*/
	clearToolsDock();
	
	// supprime les editeurs de primitives
	qDeleteAll(editors_.begin(), editors_.end());
	editors_.clear();
}

/**
 * @brief QETElementEditor::setLocation
 * The new location to edit
 * @param el
 */
void QETElementEditor::setLocation(const ElementsLocation &el)
{
	location_ = el;
	opened_from_file = false;
	setReadOnly(!location_.isWritable());
	slot_updateTitle();
}

/**
	@param fn Le nouveau nom de fichier de l'element edite
*/
void QETElementEditor::setFileName(const QString &fn) {
	filename_ = fn;
	opened_from_file = true;
	// modifie le mode lecture seule si besoin
	bool must_be_read_only = !QFileInfo(filename_).isWritable();
	if (isReadOnly() != must_be_read_only) {
		setReadOnly(must_be_read_only);
	}
	slot_updateTitle();
}

/**
 * @brief QETElementEditor::setupActions
 * Create action used in Element editor
 */
void QETElementEditor::setupActions() {
	new_element       = new QAction(QET::Icons::DocumentNew,          tr("&Nouveau"),                                  this);
	open              = new QAction(QET::Icons::DocumentOpen,         tr("&Ouvrir"),                                   this);
	open_file         = new QAction(QET::Icons::DocumentOpen,         tr("&Ouvrir depuis un fichier"),                 this);
	open_dxf          = new QAction(QET::Icons::DocumentOpen,         tr("&Lancer le plugin convertisseur DXF"),       this);
	save              = new QAction(QET::Icons::DocumentSave,         tr("&Enregistrer"),                              this);
	save_as           = new QAction(QET::Icons::DocumentSaveAs,       tr("Enregistrer sous"),                          this);
	save_as_file      = new QAction(QET::Icons::DocumentSaveAs,       tr("Enregistrer dans un fichier"),               this);
	reload            = new QAction(QET::Icons::ViewRefresh,          tr("Recharger"),                                 this);
	quit              = new QAction(QET::Icons::ApplicationExit,      tr("&Quitter"),                                  this);
	selectall         = new QAction(QET::Icons::EditSelectAll,        tr("Tout sélectionner"),                         this);
	deselectall       = new QAction(                                  tr("Désélectionner tout"),                       this);
	cut               = new QAction(QET::Icons::EditCut,              tr("Co&uper"),                                   this);
	copy              = new QAction(QET::Icons::EditCopy,             tr("Cop&ier"),                                   this);
	paste             = new QAction(QET::Icons::EditPaste,            tr("C&oller"),                                   this);
	paste_in_area     = new QAction(QET::Icons::EditPaste,            tr("C&oller dans la zone..."),                   this);
	paste_from_file   = new QAction(QET::Icons::XmlTextFile,          tr("un fichier"),                                this);
	paste_from_elmt   = new QAction(QET::Icons::Element,              tr("un élément"),                                this);
	inv_select        = new QAction(                                  tr("Inverser la sélection"),                     this);
	edit_delete       = new QAction(QET::Icons::EditDelete,           tr("&Supprimer"),                                this);
	edit_names        = new QAction(QET::Icons::Names,                tr("Éditer le nom et les traductions de l'élément"), this);
	edit_author       = new QAction(QET::Icons::UserInformations,     tr("Éditer les informations sur l'auteur"),      this);
	m_edit_properties = new QAction(QET::Icons::ElementEdit,          tr("Éditer les propriétés de l'élément"),        this);
	
	undo = m_elmt_scene -> undoStack().createUndoAction(this, tr("Annuler"));
	redo = m_elmt_scene -> undoStack().createRedoAction(this, tr("Refaire"));
	undo -> setIcon(QET::Icons::EditUndo);
	redo -> setIcon(QET::Icons::EditRedo);
	undo -> setShortcuts(QKeySequence::Undo);
	redo -> setShortcuts(QKeySequence::Redo);
	
	new_element       -> setShortcut(QKeySequence::New);
	open              -> setShortcut(QKeySequence::Open);
	open_file         -> setShortcut(tr("Ctrl+Shift+O"));
	save              -> setShortcut(QKeySequence::Save);
	save_as_file      -> setShortcut(tr("Ctrl+Shift+S"));
	reload            -> setShortcut(Qt::Key_F5);
	quit              -> setShortcut(QKeySequence(tr("Ctrl+Q")));
	selectall         -> setShortcut(QKeySequence::SelectAll);
	deselectall       -> setShortcut(QKeySequence(tr("Ctrl+Shift+A")));
	inv_select        -> setShortcut(QKeySequence(tr("Ctrl+I")));
	cut               -> setShortcut(QKeySequence::Cut);
	copy              -> setShortcut(QKeySequence::Copy);
	paste             -> setShortcut(QKeySequence::Paste);
	paste_in_area     -> setShortcut(tr("Ctrl+Shift+V"));
#ifndef Q_OS_MAC
	edit_delete       -> setShortcut(QKeySequence(Qt::Key_Delete));
#else
	edit_delete       -> setShortcut(QKeySequence(tr("Backspace")));
#endif
	
	edit_names        -> setShortcut(QKeySequence(tr("Ctrl+E")));
	edit_author       -> setShortcut(tr("Ctrl+Y"));
	
	connect(new_element,     SIGNAL(triggered()), this,     SLOT(slot_new()));
	connect(open,            SIGNAL(triggered()), this,     SLOT(slot_open()));
	connect(open_dxf,       SIGNAL(triggered()), this,     SLOT(slot_openDxf()));
	connect(open_file,       SIGNAL(triggered()), this,     SLOT(slot_openFile()));
	connect(save,            SIGNAL(triggered()), this,     SLOT(slot_save()));
	connect(save_as,         SIGNAL(triggered()), this,     SLOT(slot_saveAs()));
	connect(save_as_file,    SIGNAL(triggered()), this,     SLOT(slot_saveAsFile()));
	connect(reload,          SIGNAL(triggered()), this,     SLOT(slot_reload()));
	connect(quit,            SIGNAL(triggered()), this,     SLOT(close()));
	connect(selectall,       SIGNAL(triggered()), m_elmt_scene, SLOT(slot_selectAll()));
	connect(deselectall,     SIGNAL(triggered()), m_elmt_scene, SLOT(slot_deselectAll()));
	connect(inv_select,      SIGNAL(triggered()), m_elmt_scene, SLOT(slot_invertSelection()));
	connect(cut,             SIGNAL(triggered()), ce_view,  SLOT(cut()));
	connect(copy,            SIGNAL(triggered()), ce_view,  SLOT(copy()));
	connect(paste,           SIGNAL(triggered()), ce_view,  SLOT(paste()));
	connect(paste_in_area,   SIGNAL(triggered()), ce_view,  SLOT(pasteInArea()));
	connect(paste_from_file, SIGNAL(triggered()), this,     SLOT(pasteFromFile()));
	connect(paste_from_elmt, SIGNAL(triggered()), this,     SLOT(pasteFromElement()));
	connect(edit_delete,     SIGNAL(triggered()), m_elmt_scene, SLOT(slot_delete()));
	connect(edit_names,      SIGNAL(triggered()), m_elmt_scene, SLOT(slot_editNames()));
	connect(edit_author,     SIGNAL(triggered()), m_elmt_scene, SLOT(slot_editAuthorInformations()));
	connect(m_edit_properties, SIGNAL(triggered()), m_elmt_scene, SLOT(slot_editProperties()));


	/*
	 * Action related to change depth of primitive
	 */
	m_depth_ag = new QActionGroup(this);

	QAction *edit_forward  = new QAction(QET::Icons::BringForward, tr("Amener au premier plan"), m_depth_ag);
	QAction *edit_raise    = new QAction(QET::Icons::Raise,        tr("Rapprocher"),             m_depth_ag);
	QAction *edit_lower    = new QAction(QET::Icons::Lower,        tr("Éloigner"),            m_depth_ag);
	QAction *edit_backward = new QAction(QET::Icons::SendBackward, tr("Envoyer au fond"),        m_depth_ag);

	edit_raise    -> setShortcut(QKeySequence(tr("Ctrl+Shift+Up")));
	edit_lower    -> setShortcut(QKeySequence(tr("Ctrl+Shift+Down")));
	edit_backward -> setShortcut(QKeySequence(tr("Ctrl+Shift+End")));
	edit_forward  -> setShortcut(QKeySequence(tr("Ctrl+Shift+Home")));

	connect(edit_forward,  SIGNAL(triggered()), m_elmt_scene, SLOT(slot_bringForward() ));
	connect(edit_raise,    SIGNAL(triggered()), m_elmt_scene, SLOT(slot_raise()        ));
	connect(edit_lower,    SIGNAL(triggered()), m_elmt_scene, SLOT(slot_lower()        ));
	connect(edit_backward, SIGNAL(triggered()), m_elmt_scene, SLOT(slot_sendBackward() ));

	depth_toolbar = addToolBar(tr("Profondeur", "toolbar title"));
	depth_toolbar -> setObjectName("depth_toolbar");
	depth_toolbar -> addActions(m_depth_ag -> actions());
	addToolBar(Qt::TopToolBarArea, depth_toolbar);


	/*
	 * Action related to zoom
	 */
	m_zoom_ag = new QActionGroup(this);

	QAction *zoom_in    = new QAction(QET::Icons::ZoomIn,       tr("Zoom avant"),      m_zoom_ag);
	QAction *zoom_out   = new QAction(QET::Icons::ZoomOut,      tr("Zoom arrière"), m_zoom_ag);
	QAction *zoom_fit   = new QAction(QET::Icons::ZoomFitBest,  tr("Zoom adapté"),  m_zoom_ag);
	QAction *zoom_reset = new QAction(QET::Icons::ZoomOriginal, tr("Pas de zoom"),     m_zoom_ag);

	zoom_in    -> setShortcut(QKeySequence::ZoomIn);
	zoom_out   -> setShortcut(QKeySequence::ZoomOut);
	zoom_fit   -> setShortcut(QKeySequence(tr("Ctrl+9")));
	zoom_reset -> setShortcut(QKeySequence(tr("Ctrl+0")));

	connect(zoom_in,    SIGNAL(triggered()), ce_view,  SLOT(zoomIn()    ));
	connect(zoom_out,   SIGNAL(triggered()), ce_view,  SLOT(zoomOut()   ));
	connect(zoom_fit,   SIGNAL(triggered()), ce_view,  SLOT(zoomFit()   ));
	connect(zoom_reset, SIGNAL(triggered()), ce_view,  SLOT(zoomReset() ));


	/*
	 * Action related to primitive creation
	 */
	connect (m_elmt_scene, SIGNAL(partsAdded()), this, SLOT(UncheckAddPrimitive()));
	parts = new QActionGroup(this);

	QAction *add_line      = new QAction(QET::Icons::PartLine,      tr("Ajouter une ligne"),         parts);
	QAction *add_rectangle = new QAction(QET::Icons::PartRectangle, tr("Ajouter un rectangle"),      parts);
	QAction *add_ellipse   = new QAction(QET::Icons::PartEllipse,   tr("Ajouter une ellipse"),       parts);
	QAction *add_polygon   = new QAction(QET::Icons::PartPolygon,   tr("Ajouter un polygone"),       parts);
	QAction *add_text      = new QAction(QET::Icons::PartText,      tr("Ajouter du texte"),          parts);
	QAction *add_arc       = new QAction(QET::Icons::PartArc,       tr("Ajouter un arc de cercle"),  parts);
	QAction *add_terminal  = new QAction(QET::Icons::Terminal,      tr("Ajouter une borne"),         parts);
	QAction *add_textfield = new QAction(QET::Icons::PartTextField, tr("Ajouter un champ de texte"), parts);

	foreach (QAction *action, parts -> actions()) action -> setCheckable(true);

	connect(add_line,      SIGNAL(triggered()), this, SLOT(addLine()      ));
	connect(add_rectangle, SIGNAL(triggered()), this, SLOT(addRect()      ));
	connect(add_ellipse,   SIGNAL(triggered()), this, SLOT(addEllipse()   ));
	connect(add_polygon,   SIGNAL(triggered()), this, SLOT(addPolygon()   ));
	connect(add_text,      SIGNAL(triggered()), this, SLOT(addText()      ));
	connect(add_arc,       SIGNAL(triggered()), this, SLOT(addArc()       ));
	connect(add_terminal,  SIGNAL(triggered()), this, SLOT(addTerminal()  ));
	connect(add_textfield, SIGNAL(triggered()), this, SLOT(addTextField() ));

	
	parts_toolbar =  addToolBar(tr("Parties", "toolbar title"));
	parts_toolbar -> setAllowedAreas(Qt::AllToolBarAreas);
	parts_toolbar -> setObjectName("parts");
	parts_toolbar -> addActions(parts -> actions());
	addToolBar(Qt::LeftToolBarArea, parts_toolbar);


	main_toolbar = new QToolBar(tr("Outils", "toolbar title"), this);
	main_toolbar -> setObjectName("main_toolbar");
	view_toolbar = new QToolBar(tr("Affichage", "toolbar title"), this);
	view_toolbar -> setObjectName("display");
	element_toolbar = new QToolBar(tr("Élément", "toolbar title"), this);
	element_toolbar -> setObjectName("element_toolbar");
	
	main_toolbar -> addAction(new_element);
	main_toolbar -> addAction(open);
	main_toolbar -> addAction(save);
	main_toolbar -> addAction(save_as);
	main_toolbar -> addAction(reload);
	main_toolbar -> addSeparator();
	main_toolbar -> addAction(undo);
	main_toolbar -> addAction(redo);
	main_toolbar -> addSeparator();
	main_toolbar -> addAction(edit_delete);

	view_toolbar -> addAction(zoom_fit);
	view_toolbar -> addAction(zoom_reset);

	element_toolbar -> addAction(edit_names);
	element_toolbar -> addAction(m_edit_properties);
	
	addToolBar(Qt::TopToolBarArea, main_toolbar);
	addToolBar(Qt::TopToolBarArea, view_toolbar);
	addToolBar(Qt::TopToolBarArea, element_toolbar);
	
	connect(m_elmt_scene, SIGNAL(selectionChanged()), this, SLOT(slot_updateInformations()), Qt::QueuedConnection);
	connect(m_elmt_scene, SIGNAL(selectionChanged()), this, SLOT(slot_updateMenus()));
	connect(QApplication::clipboard(),  SIGNAL(dataChanged()),      this, SLOT(slot_updateMenus()));
	connect(&(m_elmt_scene -> undoStack()), SIGNAL(cleanChanged(bool)), this, SLOT(slot_updateMenus()));
	connect(&(m_elmt_scene -> undoStack()), SIGNAL(cleanChanged(bool)), this, SLOT(slot_updateTitle()));
	
	// Annuler ou refaire une action met a jour la liste des primitives ; cela sert notamment pour les
	// ajouts et suppressions de primitives ainsi que pour les actions entrainant un change
	connect(&(m_elmt_scene -> undoStack()), SIGNAL(indexChanged(int)),  this, SLOT(slot_updatePartsList()));
	
	// Annuler ou refaire une action met a jour les informations affichees sur les primitives selectionnees,
	// celles-ci etant potentiellement impactees
	connect(&(m_elmt_scene -> undoStack()), SIGNAL(indexChanged(int)),  this, SLOT(slot_updateInformations()));
}

/**
 * @brief QETElementEditor::setupMenus
 */
void QETElementEditor::setupMenus() {
	file_menu    = new QMenu(tr("&Fichier"),       this);
	edit_menu    = new QMenu(tr("&Édition"),    this);
	display_menu = new QMenu(tr("Afficha&ge"),     this);
	tools_menu   = new QMenu(tr("O&utils"),        this);
	
	file_menu    -> setTearOffEnabled(true);
	edit_menu    -> setTearOffEnabled(true);
	display_menu -> setTearOffEnabled(true);
	tools_menu   -> setTearOffEnabled(true);
	
	file_menu    -> addAction(new_element);
	file_menu    -> addAction(open);
	file_menu    -> addAction(open_file);
	file_menu    -> addAction(open_dxf);
	QMenu *recentfile = file_menu -> addMenu(QET::Icons::DocumentOpenRecent, tr("&Récemment ouverts"));
	recentfile->addActions(QETApp::elementsRecentFiles()->menu()->actions());
	connect(QETApp::elementsRecentFiles(), SIGNAL(fileOpeningRequested(const QString &)), this, SLOT(openRecentFile(const QString &)));
	file_menu    -> addAction(save);
	file_menu    -> addAction(save_as);
	file_menu    -> addAction(save_as_file);
	file_menu    -> addSeparator();
	file_menu    -> addAction(reload);
	file_menu    -> addSeparator();
	file_menu    -> addAction(quit);
	
	paste_from_menu = new QMenu(tr("Coller depuis..."));
	paste_from_menu -> setIcon(QET::Icons::EditPaste);
	paste_from_menu -> addAction(paste_from_file);
	paste_from_menu -> addAction(paste_from_elmt);

	edit_menu -> addAction(undo);
	edit_menu -> addAction(redo);
	edit_menu -> addSeparator();
	edit_menu -> addAction(selectall);
	edit_menu -> addAction(deselectall);
	edit_menu -> addAction(inv_select);
	edit_menu -> addSeparator();
	edit_menu -> addAction(cut);
	edit_menu -> addAction(copy);
	edit_menu -> addAction(paste);
	edit_menu -> addAction(paste_in_area);
	edit_menu -> addMenu(paste_from_menu);
	edit_menu -> addSeparator();
	edit_menu -> addAction(edit_delete);
	edit_menu -> addSeparator();
	edit_menu -> addAction(edit_names);
	edit_menu -> addAction(edit_author);
	edit_menu -> addAction(m_edit_properties);
	edit_menu -> addSeparator();
	edit_menu -> addActions(m_depth_ag -> actions());

	display_menu -> addActions(m_zoom_ag -> actions());
	
	insertMenu(settings_menu_, file_menu);
	insertMenu(settings_menu_, edit_menu);
	insertMenu(settings_menu_, display_menu);
}

/**
 * @brief QETElementEditor::contextMenuEvent
 * @param event
 */
void QETElementEditor::contextMenu(QPoint p) {
		QMenu menu(this);
		menu.addAction(undo);
		menu.addAction(redo);
		menu.addAction(selectall);
		menu.addAction(deselectall);
		menu.addAction(inv_select);
		menu.addSeparator();
		menu.addAction(edit_delete);
		menu.addAction(cut);
		menu.addAction(copy);
		menu.addSeparator();
		menu.addAction(paste);
		menu.addAction(paste_in_area);
		menu.addMenu(paste_from_menu);
		menu.addSeparator();
		menu.addActions(m_depth_ag -> actions());
		menu.exec(p);
 }


/**
	Met a jour les menus
*/
void QETElementEditor::slot_updateMenus() {
	bool selected_items = !read_only && !m_elmt_scene -> selectedItems().isEmpty();
	bool clipboard_elmt = !read_only && ElementScene::clipboardMayContainElement();
	
	// actions dependant seulement de l'etat "lecture seule" de l'editeur
	foreach (QAction *action, parts -> actions()) {
		action -> setEnabled(!read_only);
	}
	selectall       -> setEnabled(!read_only);
	inv_select      -> setEnabled(!read_only);
	paste_from_file -> setEnabled(!read_only);
	paste_from_elmt -> setEnabled(!read_only);
	parts_list      -> setEnabled(!read_only);
	
	// Action enabled if primitive selected
	deselectall     -> setEnabled(selected_items);
	cut             -> setEnabled(selected_items);
	copy            -> setEnabled(selected_items);
	edit_delete     -> setEnabled(selected_items);
	foreach (QAction *action, m_depth_ag -> actions())
		action->setEnabled(selected_items);
	
	// actions dependant du contenu du presse-papiers
	paste           -> setEnabled(clipboard_elmt);
	paste_in_area   -> setEnabled(clipboard_elmt);
	
	// actions dependant de l'etat de la pile d'annulation
	save            -> setEnabled(!read_only && !m_elmt_scene -> undoStack().isClean());
	undo            -> setEnabled(!read_only && m_elmt_scene -> undoStack().canUndo());
	redo            -> setEnabled(!read_only && m_elmt_scene -> undoStack().canRedo());
}

/**
	Met a jour le titre de la fenetre
*/
void QETElementEditor::slot_updateTitle() {
	QString title = min_title;
	title += " - " + m_elmt_scene -> names().name() + " ";
	if (!filename_.isEmpty() || !location_.isNull()) {
		if (!m_elmt_scene -> undoStack().isClean()) title += tr("[Modifié]", "window title tag");
	}
	if (isReadOnly()) title += tr(" [lecture seule]", "window title tag");
	setWindowTitle(title);
}

/**
	Met en place l'interface
*/
void QETElementEditor::setupInterface() {
	// editeur
	m_elmt_scene = new ElementScene(this, this);
	ce_view = new ElementView(m_elmt_scene, this);
	slot_setRubberBandToView();
	setCentralWidget(ce_view);
	
	// widget par defaut dans le QDockWidget
	default_informations = new QLabel();
	
	// ScrollArea pour accueillir un widget d'edition (change a la volee)
	tools_dock_scroll_area_ = new QScrollArea();
	tools_dock_scroll_area_ -> setFrameStyle(QFrame::NoFrame);
	tools_dock_scroll_area_ -> setAlignment(Qt::AlignHCenter|Qt::AlignTop);
	
	// Pile de widgets pour accueillir les deux widgets precedents
	tools_dock_stack_ = new QStackedWidget();
	tools_dock_stack_ -> insertWidget(0, default_informations);
	tools_dock_stack_ -> insertWidget(1, tools_dock_scroll_area_);
	
	// widgets d'editions pour les parties
	editors_["arc"]       = new ArcEditor(this);
	editors_["ellipse"]   = new EllipseEditor(this);
	editors_["line"]      = new LineEditor(this);
	editors_["polygon"]   = new PolygonEditor(this);
	editors_["rect"]      = new RectangleEditor(this);
	editors_["terminal"]  = new TerminalEditor(this);
	editors_["text"]      = new TextEditor(this);
	editors_["input"]     = new TextFieldEditor(this);
	editors_["style"]     = new StyleEditor(this);
	
	// panel sur le cote pour editer les parties
	tools_dock = new QDockWidget(tr("Informations", "dock title"), this);
	tools_dock -> setObjectName("informations");
	tools_dock -> setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	tools_dock -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	tools_dock -> setMinimumWidth(380);
	addDockWidget(Qt::RightDockWidgetArea, tools_dock);
	tools_dock -> setWidget(tools_dock_stack_);
	
	// panel sur le cote pour les annulations
	undo_dock = new QDockWidget(tr("Annulations", "dock title"), this);
	undo_dock -> setObjectName("undo");
	undo_dock -> setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	undo_dock -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	undo_dock -> setMinimumWidth(290);
	addDockWidget(Qt::RightDockWidgetArea, undo_dock);
	QUndoView* undo_view = new QUndoView(&(m_elmt_scene -> undoStack()), this);
	undo_view -> setEmptyLabel(tr("Aucune modification"));
	undo_dock -> setWidget(undo_view);
	
	// panel sur le cote pour la liste des parties
	parts_list = new QListWidget(this);
	parts_list -> setSelectionMode(QAbstractItemView::ExtendedSelection);
	connect(m_elmt_scene,   SIGNAL(partsAdded()),           this, SLOT(slot_createPartsList()));
	connect(m_elmt_scene,   SIGNAL(partsRemoved()),         this, SLOT(slot_createPartsList()));
	connect(m_elmt_scene,   SIGNAL(partsZValueChanged()),   this, SLOT(slot_createPartsList()));
	connect(m_elmt_scene,   SIGNAL(selectionChanged()),     this, SLOT(slot_updatePartsList()));
	connect(parts_list, SIGNAL(itemSelectionChanged()), this, SLOT(slot_updateSelectionFromPartsList()));
	parts_dock = new QDockWidget(tr("Parties", "dock title"), this);
	parts_dock -> setObjectName("parts_list");
	parts_dock -> setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	parts_dock -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	parts_dock -> setMinimumWidth(290);
	tabifyDockWidget(undo_dock, parts_dock);
	parts_dock -> setWidget(parts_list);
	
	slot_updateInformations();
	slot_createPartsList();
	
	// barre d'etat
	statusBar() -> showMessage(tr("Éditeur d'éléments", "status bar message"));
}

/**
	Passe l'editeur d'element en mode selection : le pointeur deplace les
	elements selectionnes et il est possible d'utiliser un rectangle de selection.
*/
void QETElementEditor::slot_setRubberBandToView() {
	ce_view -> setDragMode(QGraphicsView::RubberBandDrag);
}

/**
	Passe l'editeur d'element en mode immobile (utilise pour la lecture seule)
*/
void QETElementEditor::slot_setNoDragToView() {
	ce_view -> setDragMode(QGraphicsView::NoDrag);
}

/**
	Met a jour la zone d'information et d'edition des primitives.
	Si plusieurs primitives sont selectionnees, seule leur quantite est
	affichee. Sinon, un widget d'edition approprie est mis en place.
*/
void QETElementEditor::slot_updateInformations() {
	QList<QGraphicsItem *> selected_qgis = m_elmt_scene -> selectedItems();
	QList<CustomElementPart *> cep_list;
	bool style_editable = false;

		//Test if part are editable by style editor
	if (selected_qgis.size() >= 2)
	{
		style_editable = true;
		foreach (QGraphicsItem *qgi, selected_qgis)
		{
			if (CustomElementPart *cep = dynamic_cast<CustomElementPart *>(qgi))
				cep_list << cep;
			else
				style_editable = false;
		}
		if (style_editable)
			style_editable = StyleEditor::isStyleEditable(cep_list);

	}
	
	clearToolsDock();
	
		//There's one selected item
	if (selected_qgis.size() == 1)
	{
		QGraphicsItem *qgi = selected_qgis.first();
		if (CustomElementPart *selection = dynamic_cast<CustomElementPart *>(qgi))
		{
			// on en ajoute le widget d'edition
			QString selection_xml_name = selection -> xmlName();
			ElementItemEditor *selection_editor = editors_[selection_xml_name];
			if (selection_editor)
			{
				if (selection_editor -> setPart(selection))
				{
					tools_dock_scroll_area_ -> setWidget(selection_editor);
					tools_dock_stack_ -> setCurrentIndex(1);
				}
				else
				{
					qDebug() << "Editor refused part.";
				}
			}
		}
	}
		//There's several parts selecteds and all can be edited by style editor.
	else if (style_editable)
	{
		ElementItemEditor *selection_editor = editors_["style"];
		if (selection_editor)
		{
			if (selection_editor -> setParts(cep_list))
			{
				tools_dock_scroll_area_ -> setWidget(selection_editor);
				tools_dock_stack_ -> setCurrentIndex(1);
			}
			else
			{
				qDebug() << "Editor refused part.";
			}
		}
	}
		//Else we only display the number of selected items
	else
	{
		default_informations -> setText(tr("%n partie(s) sélectionnée(s).",
										   "",
										   selected_qgis.size()));
		default_informations -> setAlignment(Qt::AlignHCenter | Qt::AlignTop);
		tools_dock_stack_ -> setCurrentIndex(0);
	}
}

/**
	Affiche le code XML correspondant a l'element dans son etat actuel dans
	une boite de dialogue.
*/
void QETElementEditor::xmlPreview() {
	QET::QetMessageBox::information(
		this,
		"Export XML",
		m_elmt_scene -> toXml().toString(4)
	);
}

/**
 * @brief QETElementEditor::checkElement
 * Do several check about element.
 * If error is occured return false
 */
bool QETElementEditor::checkElement()
{
		//List of warning and error
	typedef QPair<QString, QString> QETWarning;
	QList<QETWarning> warnings;
	QList<QETWarning> errors;

		/// Warning #1: Element haven't got terminal
		/// (except for report, because report must have one terminal and this checking is do below)
	if (!m_elmt_scene -> containsTerminals() && !m_elmt_scene -> elementType().contains("report"))
	{
		warnings << qMakePair(
			tr("Absence de borne", "warning title"),
			tr(
				"<br>En l'absence de borne, l'élément ne pourra être"
				" relié à d'autres éléments par l'intermédiaire de conducteurs.",
				"warning description"
			)
		);
	}

		/// Check master, slave, simple and report element
	if(m_elmt_scene -> elementType() == "master" ||
	   m_elmt_scene -> elementType() == "slave"  ||
	   m_elmt_scene -> elementType() == "simple")

	{
		bool wrng = true;
		foreach (CustomElementPart *cep, m_elmt_scene->primitives())
			if (cep->property("tagg").toString() == "label") wrng = false;

			///Error #1: element is master, slave or simple but havent got input tagged 'label'
		if (wrng) {
			errors << qMakePair(
							tr("Absence de champ texte 'label'", "warning title"),
							tr("<br><b>Erreur</b  > :"
							   "<br>Les éléments de type maîtres, esclaves, simple et renvoie de folio doivent posséder un champ texte comportant le tagg 'label'."
							   "<br><b>Solution</b> :"
							   "<br>Insérer un champ texte et lui attribuer le tagg 'label'", "warning description"));
		}
	}

		/// Check folio report element
	if (m_elmt_scene -> elementType().contains("report"))
	{
		int text =0, terminal =0;

		foreach(QGraphicsItem *qgi, m_elmt_scene->items())
		{
			if		(qgraphicsitem_cast<PartTerminal *>(qgi))  terminal ++;
			else if (qgraphicsitem_cast<PartTextField *>(qgi)) text ++;
		}

			///Error #2 folio report must have only one terminal
		if (terminal != 1)
		{
			errors << qMakePair (tr("Absence de borne"),
								 tr("<br><b>Erreur</b> :"
									"<br>Les reports de folio doivent posséder une seul borne."
									"<br><b>Solution</b> :"
									"<br>Verifier que l'élément ne possède qu'une seul borne"));
		}
	}

	if (!errors.count() && !warnings.count()) return(true);

		// Display warnings
	QString dialog_message = tr("La vérification de cet élément a généré", "message box content");

	if (errors.size())
		dialog_message += QString(tr(" %n erreur(s)", "errors", errors.size()));

	if (warnings.size())
	{
		if (errors.size())
			dialog_message += QString (tr(" et"));

		dialog_message += QString (tr(" %n avertissment(s)", "warnings", warnings.size()));
	}
	dialog_message += " :";

	dialog_message += "<ol>";
	QList<QETWarning> total = warnings << errors;
	foreach(QETWarning warning, total) {
		dialog_message += "<li>";
		dialog_message += QString(
			tr("<b>%1</b> : %2", "warning title: warning description")
		).arg(warning.first).arg(warning.second);
		dialog_message += "</li>";
	}
	dialog_message += "</ol>";

	if (errors.size())
		QMessageBox::critical(this, tr("Erreurs"), dialog_message);
	else
		QMessageBox::warning(this, tr("Avertissements"), dialog_message);

		//if error == 0 that means they are only warning, we return true.
	if (errors.count() == 0) return(true);
	return false;
}

/**
	Charge un fichier
	@param filepath Chemin du fichier a charger
*/
void QETElementEditor::fromFile(const QString &filepath) {
	bool state = true;
	QString error_message;
	
	// le fichier doit exister
	QFileInfo infos_file(filepath);
	if (!infos_file.exists() || !infos_file.isFile()) {
		state = false;
		error_message = QString(tr("Le fichier %1 n'existe pas.", "message box content")).arg(filepath);
	}
	
	// le fichier doit etre lisible
	QFile file(filepath);
	if (state) {
		if (!file.open(QIODevice::ReadOnly)) {
			state = false;
			error_message = QString(tr("Impossible d'ouvrir le fichier %1.", "message box content")).arg(filepath);
		}
	}
	
	// le fichier doit etre un document XML
	QDomDocument document_xml;
	if (state) {
		if (!document_xml.setContent(&file)) {
			state = false;
			error_message = tr("Ce fichier n'est pas un document XML valide", "message box content");
		}
		file.close();
	}
	
	if (!state) {
		QET::QetMessageBox::critical(this, tr("Erreur", "toolbar title"), error_message);
		return;
	}
	
	// chargement de l'element
	m_elmt_scene -> fromXml(document_xml);
	slot_createPartsList();
	
	// gestion de la lecture seule
	if (!infos_file.isWritable()) {
		QET::QetMessageBox::warning(
			this,
			tr("Édition en lecture seule", "message box title"),
			tr("Vous n'avez pas les privilèges nécessaires pour modifier cet élement. Il sera donc ouvert en lecture seule.", "message box content")
		);
		setReadOnly(true);
	} else {
		setReadOnly(false);
	}
	
	// memorise le fichier
	setFileName(filepath);
	QETApp::elementsRecentFiles() -> fileWasOpened(filepath);
	slot_updateMenus();
}

/**
 * @brief QETElementEditor::toFile
 * Save to file the drawed element.
 * @param fn : path of the file
 * @return : true if succesfully save.
 */
bool QETElementEditor::toFile(const QString &fn)
{
	m_elmt_scene->clearEventInterface();
	m_elmt_scene->clearSelection();
	UncheckAddPrimitive();
	
	QDomDocument element_xml = m_elmt_scene->toXml();
	bool writing = QET::writeXmlFile(element_xml, fn);
	if (!writing) {
		QET::QetMessageBox::warning(
			this,
			tr("Erreur", "message box title"),
			tr("Impossible d'écrire dans ce fichier", "message box content")
		);
	}
	return(writing);
}


/**
 * @brief QETElementEditor::toLocation
 * Save the element to Location
 * @param location : location where we must save the current element
 * @return true if succesfully saved
 */
bool QETElementEditor::toLocation(const ElementsLocation &location)
{
	m_elmt_scene->clearEventInterface();
	m_elmt_scene->clearSelection();
	UncheckAddPrimitive();
	
	if (!location.setXml(m_elmt_scene->toXml()))
	{
		QET::QetMessageBox::critical(this,
									 tr("Erreur", "message box title"),
									 tr("Impossible d'enregistrer l'élément", "message box content"));
		return(false);
	}
	return(true);
}

/**
	@param provided_location Emplacement d'un element
	@return true si cet editeur est en train d'editer l'element dont
	l'emplacement est location, false sinon
*/
bool QETElementEditor::isEditing(const ElementsLocation &provided_location) {
	if (opened_from_file) {
		return(
			QET::compareCanonicalFilePaths(
				filename_,
				QETApp::realPath(provided_location.toString())
			)
		);
	} else {
		return(provided_location == location_);
	}
}

/**
	@param provided_filepath Chemin d'un element sur un filesystem
	@return true si cet editeur est en train d'editer l'element dont
	le chemin est filepath, false sinon
*/
bool QETElementEditor::isEditing(const QString &provided_filepath) {
	// determine le chemin canonique de l'element actuelle edite, si applicable
	QString current_filepath;
	if (opened_from_file) {
		current_filepath = filename_;
	} else {
		current_filepath = QETApp::realPath(location_.toString());
	}
	
	return(
		QET::compareCanonicalFilePaths(
			current_filepath,
			provided_filepath
		)
	);
}

/**
	specifie si l'editeur d'element doit etre en mode lecture seule
	@param ro true pour activer le mode lecture seule, false pour le desactiver
*/
void QETElementEditor::setReadOnly(bool ro) {
	read_only = ro;
	
	// active / desactive les interactions avec la scene
	ce_view -> setInteractive(!ro);
	
	slot_updateMenus();
}

/**
	@return true si l'editeur d'element est en mode lecture seule
*/
bool QETElementEditor::isReadOnly() const {
	return(read_only);
}

/**
 * @brief QETElementEditor::addLine
 * Set line creation interface to scene
 */
void QETElementEditor::addLine() {
	m_elmt_scene -> setEventInterface(new ESEventAddLine(m_elmt_scene));
}

/**
 * @brief QETElementEditor::addRect
 * Set rectangle creation interface to scene
 */
void QETElementEditor::addRect() {
	m_elmt_scene -> setEventInterface(new ESEventAddRect(m_elmt_scene));
}

/**
 * @brief QETElementEditor::addEllipse
 * Set ellipse creation interface to scene
 */
void QETElementEditor::addEllipse() {
	m_elmt_scene -> setEventInterface(new ESEventAddEllipse(m_elmt_scene));
}

/**
 * @brief QETElementEditor::addPolygon
 * Set polygon creation interface to scene
 */
void QETElementEditor::addPolygon() {
	m_elmt_scene -> setEventInterface(new ESEventAddPolygon(m_elmt_scene));
}

/**
 * @brief QETElementEditor::addArc
 * Set arc creation interface to scene
 */
void QETElementEditor::addArc() {
	m_elmt_scene -> setEventInterface(new ESEventAddArc(m_elmt_scene));
}

/**
 * @brief QETElementEditor::addText
 * Set text creation interface to scene
 */
void QETElementEditor::addText() {
	m_elmt_scene -> setEventInterface(new ESEventAddText(m_elmt_scene));
}

/**
 * @brief QETElementEditor::addTextField
 * Set text field creation interface to scene
 */
void QETElementEditor::addTextField() {
	m_elmt_scene -> setEventInterface(new ESEventAddTextField(m_elmt_scene));
}

/**
 * @brief QETElementEditor::addTerminal
 * Set terminal creation interface to scene
 */
void QETElementEditor::addTerminal() {
	m_elmt_scene -> setEventInterface(new ESEventAddTerminal(m_elmt_scene));
}

/**
 * @brief QETElementEditor::UncheckAddPrimitive
 * Uncheck all action related to primitive
 */
void QETElementEditor::UncheckAddPrimitive() {
	foreach(QAction *action, parts->actions()) action -> setChecked(false);
}

/**
	Lance l'assistant de creation d'un nouvel element.
*/
void QETElementEditor::slot_new() {
	NewElementWizard new_element_wizard(this);
	new_element_wizard.exec();
}

/**
	Ouvre un element
*/
void QETElementEditor::slot_open() {
	// demande le chemin virtuel de l'element a ouvrir a l'utilisateur
	ElementsLocation location = ElementDialog::getOpenElementLocation(this);
	if (location.isNull()) return;
	QETApp::instance() -> openElementLocations(QList<ElementsLocation>() << location);
}

/**
	Ouvre un fichier
	Demande un fichier a l'utilisateur et ouvre ce fichier
*/
void QETElementEditor::slot_openFile() {
	// repertoire a afficher initialement dans le dialogue
	QString open_dir = filename_.isEmpty() ? QETApp::customElementsDir() : QDir(filename_).absolutePath();
	
	// demande un nom de fichier a ouvrir a l'utilisateur
	QString user_filename = QETElementEditor::getOpenElementFileName(this, open_dir);
	
	// ouvre l'element
	openElement(user_filename);
}

/**
	Slot utilise pour ouvrir un fichier recent.
	Transfere filepath au slot openElement seulement si cet editeur est actif
	@param filepath Fichier a ouvrir
	@see openElement
*/
void QETElementEditor::openRecentFile(const QString &filepath) {
	// small hack to prevent all element editors from trying to topen the required
	// recent file at the same time
	if (qApp -> activeWindow() != this) return;
	openElement(filepath);
}

/**
 * @brief QETElementEditor::slot_openDxf
 */
void QETElementEditor::slot_openDxf (){

#if defined(Q_OS_WIN32)
QString program = (QDir::homePath() + "/Application Data/qet/DXFtoQET.exe");
#elif defined(Q_OS_MAC)
QString program = (QDir::homePath() + "/.qet/DXFtoQET.app");
#else
QString program = (QDir::homePath() + "/.qet/DXFtoQET");
#endif
QStringList arguments;
QProcess *DXF = new QProcess(qApp);
DXF->start(program,arguments);

}

/**
	Ouvre un fichier element dans un nouvel editeur
	Cette methode ne controle pas si le fichier est deja ouvert
	@param filepath Fichier a ouvrir
	@see fromFile
	@see QETApp::openElementFiles
*/
void QETElementEditor::openElement(const QString &filepath) {
	if (filepath.isEmpty()) return;
	// we have to test the file existence here because QETApp::openElementFiles()
	// will discard non-existent files through QFileInfo::canonicalFilePath()
	if (!QFile::exists(filepath)) {
		QET::QetMessageBox::critical(
			this,
			tr("Impossible d'ouvrir le fichier", "message box title"),
			QString(
				tr("Il semblerait que le fichier %1 que vous essayez d'ouvrir"
				" n'existe pas ou plus.")
			).arg(filepath)
		);
	}
	QETApp::instance() -> openElementFiles(QStringList() << filepath);
}

/**
 * @brief QETElementEditor::slot_reload
 * Reload the element from the file or location
 */
void QETElementEditor::slot_reload()
{
		//If user already edit the element, ask confirmation to reload
	if (!m_elmt_scene -> undoStack().isClean())
	{
		QMessageBox::StandardButton answer = QET::QetMessageBox::question(this,
																		  tr("Recharger l'élément", "dialog title"),
																		  tr("Vous avez efffectué des modifications sur cet élément. Si vous le rechargez, ces modifications seront perdues. Voulez-vous vraiment recharger l'élément ?", "dialog content"),
																		  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
																		  QMessageBox::Cancel);
		if (answer != QMessageBox::Yes) return;
	}

		//Reload the element
	m_elmt_scene -> reset();
	if (opened_from_file)
		fromFile(filename_);
	else
		fromLocation(location_);
}

/**
 * @brief QETElementEditor::slot_save
 * Save the current editing element.
 * If the filepath or location is unknow, use save_as instead
 * @return true if save with success
 */
bool QETElementEditor::slot_save()
{
		// Check element befor writing
	if (checkElement())
	{
			//If we don't know the name of the current file, use save as instead
		if (opened_from_file)
		{
			if (filename_.isEmpty())
				return(slot_saveAsFile());

				//Else wa save to the file at filename_ path
			bool result_save = toFile(filename_);
			if (result_save) m_elmt_scene -> undoStack().setClean();
			return(result_save);
		}
		else
		{
			if (location_.isNull())
				return(slot_saveAs());

				//Else save to the known location
			bool result_save = toLocation(location_);
			if (result_save) {
				m_elmt_scene -> undoStack().setClean();
				emit saveToLocation(location_);
			}
			return(result_save);
		}
	}

	QMessageBox::critical(this, tr("Echec de l'enregistrement"), tr("L'enregistrement à échoué,\nles conditions requises ne sont pas valides"));
	return false;
}

/**
 * @brief QETElementEditor::slot_saveAs
 * Ask a location to user and save the current edited element
 * to this location
 * @return true if save with success
 */
bool QETElementEditor::slot_saveAs()
{
		// Check element befor writing
	if (checkElement())
	{
			//Ask a location to user
		ElementsLocation location = ElementDialog::getSaveElementLocation(this);
		if (location.isNull())
			return(false);

		bool result_save = toLocation(location);
		if (result_save)
		{
			setLocation(location);
			m_elmt_scene->undoStack().setClean();
			emit saveToLocation(location);
		}

		return(result_save);
	}
	QMessageBox::critical(this, tr("Echec de l'enregistrement"), tr("L'enregistrement à échoué,\nles conditions requises ne sont pas valides"));
	return (false);
}

/**
 * @brief QETElementEditor::slot_saveAsFile
 * Ask a file to user and save the current edited element to this file
 * @return true if save with success
 */
bool QETElementEditor::slot_saveAsFile()
{
		// Check element befor writing
	if (checkElement())
	{
			//Ask a filename to user, for save the element
		QString fn = QFileDialog::getSaveFileName(
			this,
			tr("Enregistrer sous", "dialog title"),
			filename_.isEmpty() ? QETApp::customElementsDir() : QDir(filename_).absolutePath(),
			tr(
				"Éléments QElectroTech (*.elmt)",
				"filetypes allowed when saving an element file"
			)
		);

		if (fn.isEmpty())
			return(false);

			//If the name doesn't end by .elmt, we add it
		if (!fn.endsWith(".elmt", Qt::CaseInsensitive))
			fn += ".elmt";

		bool result_save = toFile(fn);
			//If the save success, the filename is keep
		if (result_save)
		{
			setFileName(fn);
			QETApp::elementsRecentFiles() -> fileWasOpened(fn);
			m_elmt_scene -> undoStack().setClean();
		}

		return(result_save);
	}
	QMessageBox::critical(this, tr("Echec de l'enregistrement"), tr("L'enregistrement à échoué,\nles conditions requises ne sont pas valides"));
	return false;
}

/**
	@return true si l'element peut etre ferme.
	Un element peut etre ferme s'il ne comporte aucune modification.
	Si l'element comporte des modifications, la question est posee a
	l'utilisateur.
*/
bool QETElementEditor::canClose() {
	if (m_elmt_scene -> undoStack().isClean()) return(true);
	// demande d'abord a l'utilisateur s'il veut enregistrer l'element en cours
	QMessageBox::StandardButton answer = QET::QetMessageBox::question(
		this,
		tr("Enregistrer l'élément en cours ?", "dialog title"),
		QString(
			tr(
				"Voulez-vous enregistrer l'élément %1 ?",
				"dialog content - %1 is an element name"
			)
		).arg(m_elmt_scene -> names().name()),
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
		QMessageBox::Cancel
	);
	bool result;
	switch(answer) {
		case QMessageBox::Cancel: result = false;         break; // l'utilisateur annule : echec de la fermeture
		case QMessageBox::Yes:    result = slot_save();   break; // l'utilisateur dit oui : la reussite depend de l'enregistrement
		default:                  result = true;                 // l'utilisateur dit non ou ferme le dialogue: c'est reussi
	}
	return(result);
}

/**
	Enleve et cache le widget affiche par le dock permettant d'editer les
	parties.
	@return le widget enleve, ou 0 s'il n'y avait pas de widget a enlever
*/
QWidget *QETElementEditor::clearToolsDock() {
	if (QWidget *previous_widget = tools_dock_scroll_area_ -> takeWidget()) {
		previous_widget -> setParent(0);
		previous_widget -> hide();
		return(previous_widget);
	}
	return(0);
}

/**
	Exporte le document XML xml_document vers le presse-papier puis declenche
	son collage dans l'editeur courant, avec selection de la zone de collage
	@param xml_document Document XML a copier/coller
	@see ElementView::pasteInArea
*/
void QETElementEditor::copyAndPasteXml(const QDomDocument &xml_document) {
	// accede au presse-papier
	QClipboard *clipboard = QApplication::clipboard();
	
	// genere la description XML de la selection
	QString clipboard_content = xml_document.toString(4);
	
	// met la description XML dans le presse-papier
	if (clipboard -> supportsSelection()) {
		clipboard -> setText(clipboard_content, QClipboard::Selection);
	}
	clipboard -> setText(clipboard_content);
	
	ce_view -> pasteInArea();
}

/**
	Permet de quitter l'editeur lors de la fermeture de la fenetre principale
	@param qce Le QCloseEvent correspondant a l'evenement de fermeture
*/
void QETElementEditor::closeEvent(QCloseEvent *qce) {
	if (canClose()) {
		writeSettings();
		setAttribute(Qt::WA_DeleteOnClose);
		m_elmt_scene -> reset();
		qce -> accept();
	} else qce -> ignore();
}

/**
	Executed the first time the window editor is displayed.
*/
void QETElementEditor::firstActivation(QEvent *event) {
	Q_UNUSED(event)
	QTimer::singleShot(250, ce_view, SLOT(zoomFit()));
}

/**
	Remplit la liste des parties
*/
void QETElementEditor::slot_createPartsList() {
	parts_list -> blockSignals(true);
	parts_list -> clear();
	QList<QGraphicsItem *> qgis = m_elmt_scene -> zItems();
	
	// on ne construit plus la liste a partir de 200 primitives
	// c'est ingerable : la maj de la liste prend trop de temps et le resultat
	// est inexploitable
	if (qgis.count() <= QET_MAX_PARTS_IN_ELEMENT_EDITOR_LIST) {
		for (int j = qgis.count() - 1 ; j >= 0 ; -- j) {
			QGraphicsItem *qgi = qgis[j];
			if (CustomElementPart *cep = dynamic_cast<CustomElementPart *>(qgi)) {
				QString part_desc = cep -> name();
				QListWidgetItem *qlwi = new QListWidgetItem(part_desc);
				QVariant v;
				v.setValue<QGraphicsItem *>(qgi);
				qlwi -> setData(42, v);
				parts_list -> addItem(qlwi);
				qlwi -> setSelected(qgi -> isSelected());
			}
		}
	} else {
		parts_list -> addItem(new QListWidgetItem(tr("Trop de primitives, liste non générée.")));
	}
	parts_list -> blockSignals(false);
}

/**
	Met a jour la selection dans la liste des parties
*/
void QETElementEditor::slot_updatePartsList() {
	int items_count = m_elmt_scene -> items().count();
	if (parts_list -> count() != items_count) {
		slot_createPartsList();
	} else if (items_count <= QET_MAX_PARTS_IN_ELEMENT_EDITOR_LIST) {
		parts_list -> blockSignals(true);
		int i = 0;
		QList<QGraphicsItem *> items = m_elmt_scene -> zItems();
		for (int j = items.count() - 1 ; j >= 0 ; -- j) {
			QGraphicsItem *qgi = items[j];
			QListWidgetItem *qlwi = parts_list -> item(i);
			if (qlwi) qlwi -> setSelected(qgi -> isSelected());
			++ i;
		}
		parts_list -> blockSignals(false);
	}
}

/**
	Met a jour la selection des parties de l'element a partir de la liste des
	parties
*/
void QETElementEditor::slot_updateSelectionFromPartsList() {
	m_elmt_scene  -> blockSignals(true);
	parts_list -> blockSignals(true);
	for (int i = 0 ; i < parts_list -> count() ; ++ i) {
		QListWidgetItem *qlwi = parts_list -> item(i);
		QGraphicsItem *qgi = qlwi -> data(42).value<QGraphicsItem *>();
		if (qgi) {
			qgi -> setSelected(qlwi -> isSelected());
		}
	}
	parts_list -> blockSignals(false);
	m_elmt_scene -> blockSignals(false);
	slot_updateInformations();
	slot_updateMenus();
}

/**
 * @brief QETElementEditor::readSettings
 * Read settings
 */
void QETElementEditor::readSettings()
{
	QSettings settings;
	
	// dimensions et position de la fenetre
	QVariant geometry = settings.value("elementeditor/geometry");
	if (geometry.isValid()) restoreGeometry(geometry.toByteArray());
	
	// etat de la fenetre (barres d'outils, docks...)
	QVariant state = settings.value("elementeditor/state");
	if (state.isValid()) restoreState(state.toByteArray());
	
	// informations complementaires de l'element : valeur par defaut
	m_elmt_scene -> setInformations(settings.value("elementeditor/default-informations", "").toString());
}

/**
 * @brief QETElementEditor::writeSettings
 * Write the settings
 */
void QETElementEditor::writeSettings()
{
	QSettings settings;
	settings.setValue("elementeditor/geometry", saveGeometry());
	settings.setValue("elementeditor/state", saveState());
}

/**
	@return les decalages horizontaux et verticaux (sous la forme d'un point) a
	utiliser lors d'un copier/coller avec decalage.
*/
QPointF QETElementEditor::pasteOffset() {
	QPointF paste_offset(5.0, 0.0);
	return(paste_offset);
}

/**
	Demande a l'utilisateur d'ouvrir un fichier sense etre un element.
	@param parent QWidget parent du dialogue d'ouverture de fichier
	@param initial_dir Repertoire a afficher initialement - si une chaine vide
	est fournie, QETApp::customElementsDir() sera utilise.
	@return Le chemin du fichier choisi ou une chaine vide si l'utilisateur a
	clique sur le bouton "Annuler".
	@see QETApp::customElementsDir()
*/
QString QETElementEditor::getOpenElementFileName(QWidget *parent, const QString &initial_dir) {
	// demande un nom de fichier a ouvrir a l'utilisateur
	QString user_filename = QFileDialog::getOpenFileName(
		parent,
		tr("Ouvrir un fichier", "dialog title"),
		initial_dir.isEmpty() ? QETApp::customElementsDir() : initial_dir,
		tr(
			"Éléments QElectroTech (*.elmt);;"
			"Fichiers XML (*.xml);;"
			"Tous les fichiers (*)",
			"filetypes allowed when opening an element file"
		)
	);
	return(user_filename);
}

/**
 * @brief QETElementEditor::fromLocation
 * Location of the element to edit
 * @param location
 */
void QETElementEditor::fromLocation(const ElementsLocation &location)
{
	if (!location.isElement())
	{
		QET::QetMessageBox::critical(this,
									 tr("Élément inexistant.", "message box title"),
									 tr("Le chemin virtuel choisi ne correspond pas à un élément.", "message box content"));
		return;
	}
	if (!location.exist())
	{
		QET::QetMessageBox::critical(this,
									 tr("Élément inexistant.", "message box title"),
									 tr("L'élément n'existe pas.", "message box content"));
		return;
	}

		//The file must be an xml document
	QDomDocument document_xml;
	QDomNode node = document_xml.importNode(location.xml(), true);
	document_xml.appendChild(node);

		//Load the element
	m_elmt_scene -> fromXml(document_xml);
	slot_createPartsList();

		//location is read only
	if (!location.isWritable())
	{
		QET::QetMessageBox::warning(this,
									tr("Édition en lecture seule", "message box title"),
									tr("Vous n'avez pas les privilèges nécessaires pour modifier cet élement. Il sera donc ouvert en lecture seule.", "message box content"));
		setReadOnly(true);
	}
	else {
		setReadOnly(false);
	}

	setLocation(location);
	slot_updateMenus();
}

/**
	Demande un fichier a l'utilisateur, l'ouvre en tant que fichier element,
	met son contenu dans le presse-papiers, et appelle ElementView::PasteInArea
*/
void QETElementEditor::pasteFromFile() {
	// demande le chemin du fichier a ouvrir a l'utilisateur
	QString element_file_path = getOpenElementFileName(this);
	if (element_file_path.isEmpty()) return;
	
	QString error_message;
	QDomDocument xml_document;
	QFile element_file(element_file_path);
	// le fichier doit etre lisible
	if (!element_file.open(QIODevice::ReadOnly)) {
		error_message = QString(tr("Impossible d'ouvrir le fichier %1.", "message box content")).arg(element_file_path);
	} else {
		// le fichier doit etre un document XML
		if (!xml_document.setContent(&element_file)) {
			error_message = tr("Ce fichier n'est pas un document XML valide", "message box content");
		}
		element_file.close();
	}
	
	if (!error_message.isEmpty()) {
		QET::QetMessageBox::critical(this, tr("Erreur", "toolbar title"), error_message);
	}
	copyAndPasteXml(xml_document);
}

/**
 * @brief QETElementEditor::pasteFromElement
 * Ask an element to user, copy the xml definition of the element
 * to the clipboard and call ElementView::PasteInArea
 */
void QETElementEditor::pasteFromElement()
{
		//Ask for a location
	ElementsLocation location = ElementDialog::getOpenElementLocation(this);
	if (location.isNull())
		return;

	if (!location.isElement())
	{
		QET::QetMessageBox::critical(this,
									 tr("Élément inexistant.", "message box title"),
									 tr("Le chemin virtuel choisi ne correspond pas à un élément.", "message box content"));
		return;
	}
	if (!location.exist())
	{
		QET::QetMessageBox::critical(this,
									 tr("Élément inexistant.", "message box title"),
									 tr("L'élément n'existe pas.", "message box content"));
		return;
	}

		//Create an xml document from the location xml
	QDomDocument document_xml;
	QDomNode node = document_xml.importNode(location.xml(), true);
	document_xml.appendChild(node);

	copyAndPasteXml(document_xml);
}

/**
	Met a jour l'editeur de primitive actuellement visible.
	Si aucun editeur de primitive n'est visible, ce slot ne fait rien.
*/
void QETElementEditor::updateCurrentPartEditor() {
	// si aucun widget d'edition n'est affiche, on ne fait rien
	if (!tools_dock_stack_ -> currentIndex()) return;
	
	// s'il y a un widget d'edition affiche, on le met a jour
	if (ElementItemEditor *current_editor = dynamic_cast<ElementItemEditor *>(tools_dock_scroll_area_ -> widget())) {
		current_editor -> updateForm();
	}
}
