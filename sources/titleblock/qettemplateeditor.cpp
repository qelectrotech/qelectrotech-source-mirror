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
#include "qettemplateeditor.h"
#include "qetmessagebox.h"
#include "qeticons.h"
#include "qetapp.h"
#include "qetproject.h"
#include "templatecellwidget.h"
#include "templatecommands.h"
#include "templateview.h"
#include "templatelocationsaver.h"
#include "templatelogomanager.h"
#include "templatecellwidget.h"

/**
	@param parent parent QWidget of this window
*/
QETTitleBlockTemplateEditor::QETTitleBlockTemplateEditor(QWidget *parent) :
	QETMainWindow(parent),
	opened_from_file_(false),
	read_only_(false),
	duplicate_(false),
	tb_template_(0),
	logo_manager_(0)
{
	setWindowIcon(QET::Icons::QETLogo);
	setAttribute(Qt::WA_DeleteOnClose);
	
	initWidgets();
	initActions();
	initMenus();
	initToolbars();
	readSettings();
}

/**
	Destructor
*/
QETTitleBlockTemplateEditor::~QETTitleBlockTemplateEditor() {
}

/**
	@return the location of the currently edited template
*/
TitleBlockTemplateLocation QETTitleBlockTemplateEditor::location() const {
	return(location_);
}

/**
	@return true if the provided filepath matches the currently edited template.
	@param filepath path of a title block template on the filesystem
*/
bool QETTitleBlockTemplateEditor::isEditing(const QString &filepath) {
	QString current_filepath;
	if (opened_from_file_) {
		current_filepath = filepath_;
	} else {
		current_filepath = QETApp::realPath(location_.toString());
	}
	
	return(
		QET::compareCanonicalFilePaths(
			current_filepath,
			filepath
		)
	);
}

/**
	@param true for this editor to prompt the user for a new template name as
	soon as the window appears in order to duplicate the edited one.
*/
void QETTitleBlockTemplateEditor::setOpenForDuplication(bool duplicate) {
	duplicate_ = duplicate;
}

/**
	@return true if this editor will prompt the user for a new template name as
	soon as the window appears in order to duplicate the edited one.
*/
bool QETTitleBlockTemplateEditor::openForDuplication() const {
	return(duplicate_);
}

/**
	@return true if the currently edited template can be closed. A template can be
	closed if it has not been modified. If the template has been modified, this
	method asks the user what he wants to do.
*/
bool QETTitleBlockTemplateEditor::canClose() {
	if (undo_stack_ -> isClean()) return(true);
	// ask the user whether he wants to save the current template
	QMessageBox::StandardButton answer = QET::QetMessageBox::question(
		this,
		tr("Enregistrer le modèle en cours ?", "dialog title"),
		QString(
			tr(
				"Voulez-vous enregistrer le modèle %1 ?",
				"dialog content - %1 is a title block template name"
			)
		).arg(location_.name()),
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
		QMessageBox::Cancel
	);
	bool result;
	switch(answer) {
		case QMessageBox::Cancel: result = false;  break; // the user hits Cancel or closes the dialog: abort the closing
		case QMessageBox::Yes:    result = save(); break; // the user hits Yes: la reussite depend de l'enregistrement
		default:                  result = true;          // the user hits no: the editor can be closed
	}
	return(result);
}

/**
	@param event Object describing the received event.
*/
void QETTitleBlockTemplateEditor::firstActivation(QEvent *event) {
	Q_UNUSED(event)
	if (duplicate_ && !opened_from_file_ && location_.parentCollection()) {
		// this editor is supposed to duplicate its current location
		QTimer::singleShot(250, this, SLOT(duplicateCurrentLocation()));
	}
}

/**
	Handle the closing of the main window
	@param qce The QCloseEvent event
*/
void QETTitleBlockTemplateEditor::closeEvent(QCloseEvent *qce) {
	if (canClose()) {
		writeSettings();
		setAttribute(Qt::WA_DeleteOnClose);
		qce -> accept();
	} else qce -> ignore();
}

/**
	Ask the user for a new template name in order to duplicate the currently
	edited template.
*/
void QETTitleBlockTemplateEditor::duplicateCurrentLocation() {
	// this method does not work for templates edited from the filesystem
	if (opened_from_file_) return;
	
	QString proposed_name;
	if (location_.name().isEmpty()) {
		proposed_name = tr("nouveau_modele", "template name suggestion when duplicating the default one");
	} else {
		proposed_name = QString("%1_copy").arg(location_.name());
	}
	
	bool accepted = false;
	QString new_template_name = QInputDialog::getText(
		this,
		tr("Dupliquer un modèle de cartouche", "input dialog title"),
		tr("Pour dupliquer ce modèle, entrez le nom voulu pour sa copie", "input dialog text"),
		QLineEdit::Normal,
		proposed_name,
		&accepted
	);
	if (accepted) {
		TitleBlockTemplateLocation new_template_location(new_template_name, location_.parentCollection());
		saveAs(new_template_location);
	}
}

/**
	@param location Location of the tile block template to be edited.
*/
bool QETTitleBlockTemplateEditor::edit(const TitleBlockTemplateLocation &location) {
	// the template name may be empty to create a new one
	const TitleBlockTemplate *tb_template_orig;
	if (location.name().isEmpty()) {
		// loads the default title block template provided by the application
		// it will be used as a start point to design the title block
		tb_template_orig = QETApp::defaultTitleBlockTemplate();
	} else {
		tb_template_orig = location.getTemplate();
	}
	if (!tb_template_orig) {
		/// TODO The TBT does not exist, manage error
		return(false);
	}
	
	opened_from_file_ = false;
	location_ = location;
	setReadOnly(location.isReadOnly());
	editCopyOf(tb_template_orig);
	return(true);
}

/**
	Edit the given template.
	@param project Parent project of the template to edit.
	@param template_name Name of the template to edit within its parent project.
	@return true if this editor was able to edit the given template, false otherwise
*/
bool QETTitleBlockTemplateEditor::edit(QETProject *project, const QString &template_name)
{
		// we require a project we will rattach templates to
	if (!project) return(false);
	
		// the template name may be empty to create a new one
	const TitleBlockTemplate *tb_template_orig;
	if (template_name.isEmpty())
	{
			// loads the default title block template provided by the application
			// it will be used as a start point to design the title block
		tb_template_orig = QETApp::defaultTitleBlockTemplate();
	}
	else
	{
		tb_template_orig = project->embeddedTitleBlockTemplatesCollection()->getTemplate(template_name);
	}
	
	if (!tb_template_orig) {
		/// TODO The TBT does not exist, manage error
		return(false);
	}
	
	opened_from_file_ = false;
	location_.setParentCollection(project -> embeddedTitleBlockTemplatesCollection());
	location_.setName(template_name);
	setReadOnly(project -> isReadOnly());
	return(editCopyOf(tb_template_orig));
}

/**
	@param file_path Path of the template file to edit.
	@return false if a problem occured while opening the template, true otherwise.
*/
bool QETTitleBlockTemplateEditor::edit(const QString &file_path) {
	// get title block template object from the file, edit it
	TitleBlockTemplate *tbt = new TitleBlockTemplate();
	bool loading = tbt -> loadFromXmlFile(file_path);
	if (!loading) {
		/// TODO the file opening failed, warn the user?
		return(false);
	}
	
	bool editing = edit(tbt);
	if (!editing) {
		/// TODO the file editing failed, warn the user?
		return(false);
	}
	
	QFileInfo file_path_info(file_path);
	filepath_ = file_path;
	opened_from_file_ = true;
	setReadOnly(!file_path_info.isWritable());
	return(true);
}

/**
	@param tbt Title block template to be edited
	@return false if a problem occured while opening the template, true otherwise.
*/
bool QETTitleBlockTemplateEditor::editCopyOf(const TitleBlockTemplate *tbt) {
	if (!tbt) return(false);
	return(edit(tbt -> clone()));
}

/**
	@param tbt Title block template to be directly edited
	@return false if a problem occured while opening the template, true otherwise.
*/
bool QETTitleBlockTemplateEditor::edit(TitleBlockTemplate *tbt) {
	if (!tbt) return(false);
	tb_template_ = tbt;
	template_edition_area_view_ -> setTitleBlockTemplate(tb_template_);
	template_cell_editor_widget_ -> updateLogosComboBox(tb_template_);
	updateEditorTitle();
	return(true);
}

/**
	Launches the logo manager widget, which allows the user to manage the
	logos embedded within the edited template.
*/
void QETTitleBlockTemplateEditor::editLogos() {
	if (tb_template_) {
		if (!logo_manager_) {
			initLogoManager();
		}
		
		logo_manager_ -> layout() -> setContentsMargins(0, 0, 0, 0);
		QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
		
		QVBoxLayout *vlayout0 = new QVBoxLayout();
		vlayout0 -> addWidget(logo_manager_);
		vlayout0 -> addWidget(buttons);
		
		QDialog d(this);
		d.setWindowTitle(logo_manager_ -> windowTitle());
		d.setLayout(vlayout0);
		connect(buttons, SIGNAL(rejected()), &d, SLOT(reject()));
		d.exec();
		
		// prevent the logo manager from being deleted along with the dialog
		logo_manager_ -> setParent(this);
	}
}

/**
	Launch a new title block template editor.
*/
void QETTitleBlockTemplateEditor::newTemplate() {
	QETTitleBlockTemplateEditor *qet_template_editor = new QETTitleBlockTemplateEditor();
	qet_template_editor -> edit(TitleBlockTemplateLocation());
	qet_template_editor -> show();
}

/**
	Initialize the various actions.
*/
void QETTitleBlockTemplateEditor::initActions() {
	new_            = new QAction(QET::Icons::DocumentNew,          tr("&Nouveau",                     "menu entry"), this);
	open_           = new QAction(QET::Icons::DocumentOpen,         tr("&Ouvrir",                      "menu entry"), this);
	open_from_file_ = new QAction(QET::Icons::DocumentOpen,         tr("Ouvrir depuis un fichier",     "menu entry"), this);
	save_           = new QAction(QET::Icons::DocumentSave,         tr("&Enregistrer",                 "menu entry"), this);
	save_as_        = new QAction(QET::Icons::DocumentSaveAs,       tr("Enregistrer sous",             "menu entry"), this);
	save_as_file_   = new QAction(QET::Icons::DocumentSaveAs,       tr("Enregistrer vers un fichier",  "menu entry"), this);
	quit_           = new QAction(QET::Icons::ApplicationExit,      tr("&Quitter",                     "menu entry"), this);
	undo_           = undo_stack_ -> createUndoAction(this);
	redo_           = undo_stack_ -> createRedoAction(this);
	cut_            = new QAction(QET::Icons::EditCut,              tr("Co&uper", "menu entry"),                      this);
	copy_           = new QAction(QET::Icons::EditCopy,             tr("Cop&ier", "menu entry"),                      this);
	paste_          = new QAction(QET::Icons::EditPaste,            tr("C&oller", "menu entry"),                      this);
	edit_logos_     = new QAction(QET::Icons::InsertImage,          tr("Gérer les logos", "menu entry"),           this);
	edit_info_      = new QAction(QET::Icons::UserInformations,     tr("Éditer les informations complémentaires", "menu entry"), this);
	zoom_in_        = new QAction(QET::Icons::ZoomIn,               tr("Zoom avant",                   "menu entry"), this);
	zoom_out_       = new QAction(QET::Icons::ZoomOut,              tr("Zoom arrière",              "menu entry"), this);
	zoom_fit_       = new QAction(QET::Icons::ZoomFitBest,          tr("Zoom adapté",               "menu entry"), this);
	zoom_reset_     = new QAction(QET::Icons::ZoomOriginal,         tr("Pas de zoom",                  "menu entry"), this);
	add_row_        = new QAction(QET::Icons::EditTableInsertRowAbove,    tr("Ajouter une &ligne",      "menu entry"), this);
	add_col_        = new QAction(QET::Icons::EditTableInsertColumnRight, tr("Ajouter une &colonne",    "menu entry"), this);
	merge_cells_    = new QAction(QET::Icons::EditTableCellMerge,   tr("&Fusionner les cellules",      "menu entry"), this);
	split_cell_     = new QAction(QET::Icons::EditTableCellSplit,   tr("&Séparer les cellules",     "menu entry"), this);
	
	undo_ -> setIcon(QET::Icons::EditUndo);
	redo_ -> setIcon(QET::Icons::EditRedo);
	
	new_              -> setShortcut(QKeySequence::New);
	open_             -> setShortcut(QKeySequence::Open);
	open_from_file_   -> setShortcut(tr("Ctrl+Shift+O", "shortcut to open a template from a file"));
	save_             -> setShortcut(QKeySequence::Save);
	save_as_file_     -> setShortcut(tr("Ctrl+Shift+S", "shortcut to save a template to a file"));
	quit_             -> setShortcut(QKeySequence(tr("Ctrl+Q", "shortcut to quit")));
	undo_             -> setShortcut(QKeySequence::Undo);
	redo_             -> setShortcut(QKeySequence::Redo);
	cut_              -> setShortcut(QKeySequence::Cut);
	copy_             -> setShortcut(QKeySequence::Copy);
	paste_            -> setShortcut(QKeySequence::Paste);
	edit_logos_       -> setShortcut(QKeySequence(tr("Ctrl+T", "shortcut to manage embedded logos")));
	edit_info_        -> setShortcut(QKeySequence(tr("Ctrl+Y", "shortcut to edit extra information")));
	merge_cells_      -> setShortcut(QKeySequence(tr("Ctrl+J", "shortcut to merge cells")));
	split_cell_       -> setShortcut(QKeySequence(tr("Ctrl+K", "shortcut to split merged cell")));
	zoom_in_          -> setShortcut(QKeySequence::ZoomIn);
	zoom_out_         -> setShortcut(QKeySequence::ZoomOut);
	zoom_fit_         -> setShortcut(QKeySequence(tr("Ctrl+9", "shortcut to enable fit zoom")));
	zoom_reset_       -> setShortcut(QKeySequence(tr("Ctrl+0", "shortcut to reset zoom")));
	
	connect(new_,             SIGNAL(triggered()), this,     SLOT(newTemplate()));
	connect(open_,            SIGNAL(triggered()), this,     SLOT(open()));
	connect(open_from_file_,  SIGNAL(triggered()), this,     SLOT(openFromFile()));
	connect(save_,            SIGNAL(triggered()), this,     SLOT(save()));
	connect(save_as_,         SIGNAL(triggered()), this,     SLOT(saveAs()));
	connect(save_as_file_,    SIGNAL(triggered()), this,     SLOT(saveAsFile()));
	connect(quit_,            SIGNAL(triggered()), this,     SLOT(quit()));
	connect(cut_,             SIGNAL(triggered()), template_edition_area_view_, SLOT(cut()));
	connect(copy_,            SIGNAL(triggered()), template_edition_area_view_, SLOT(copy()));
	connect(paste_,           SIGNAL(triggered()), template_edition_area_view_, SLOT(paste()));
	connect(zoom_in_,         SIGNAL(triggered()), template_edition_area_view_, SLOT(zoomIn()));
	connect(zoom_out_,        SIGNAL(triggered()), template_edition_area_view_, SLOT(zoomOut()));
	connect(zoom_fit_,        SIGNAL(triggered()), template_edition_area_view_, SLOT(zoomFit()));
	connect(zoom_reset_,      SIGNAL(triggered()), template_edition_area_view_, SLOT(zoomReset()));
	connect(edit_logos_,      SIGNAL(triggered()), this, SLOT(editLogos()));
	connect(edit_info_,       SIGNAL(triggered()), this, SLOT(editTemplateInformation()));
	connect(add_row_,         SIGNAL(triggered()), template_edition_area_view_, SLOT(addRowAtEnd()));
	connect(add_col_,         SIGNAL(triggered()), template_edition_area_view_, SLOT(addColumnAtEnd()));
	connect(merge_cells_,     SIGNAL(triggered()), template_edition_area_view_, SLOT(mergeSelectedCells()));
	connect(split_cell_,      SIGNAL(triggered()), template_edition_area_view_, SLOT(splitSelectedCell()));
}

/**
	Initialize the various menus.
*/
void QETTitleBlockTemplateEditor::initMenus() {
	file_menu_    = new QMenu(tr("&Fichier",        "menu title"), this);
	edit_menu_    = new QMenu(tr("&Édition",     "menu title"), this);
	display_menu_ = new QMenu(tr("Afficha&ge",      "menu title"), this);
	
	file_menu_    -> addAction(new_);
	file_menu_    -> addAction(open_);
	file_menu_    -> addAction(open_from_file_);
	file_menu_    -> addAction(save_);
	file_menu_    -> addAction(save_as_);
	file_menu_    -> addAction(save_as_file_);
	file_menu_    -> addSeparator();
	file_menu_    -> addAction(quit_);
	
	edit_menu_   -> addAction(undo_);
	edit_menu_   -> addAction(redo_);
	edit_menu_   -> addSeparator();
	edit_menu_   -> addAction(cut_);
	edit_menu_   -> addAction(copy_);
	edit_menu_   -> addAction(paste_);
	edit_menu_   -> addSeparator();
	edit_menu_   -> addAction(add_row_);
	edit_menu_   -> addAction(add_col_);
	edit_menu_   -> addAction(merge_cells_);
	edit_menu_   -> addAction(split_cell_);
	edit_menu_   -> addSeparator();
	edit_menu_   -> addAction(edit_logos_);
	edit_menu_   -> addAction(edit_info_);
	display_menu_ -> addAction(zoom_in_);
	display_menu_ -> addAction(zoom_out_);
	display_menu_ -> addAction(zoom_fit_);
	display_menu_ -> addAction(zoom_reset_);
	
	insertMenu(settings_menu_, file_menu_);
	insertMenu(settings_menu_, edit_menu_);
	insertMenu(settings_menu_, display_menu_);
}

/**
	Initalize toolbars.
*/
void QETTitleBlockTemplateEditor::initToolbars() {
	QToolBar *main_toolbar = new QToolBar(tr("Outils", "toolbar title"), this);
	main_toolbar -> setObjectName("tbt_main_toolbar");
	main_toolbar -> addAction(new_);
	main_toolbar -> addAction(open_);
	main_toolbar -> addAction(save_);
	main_toolbar -> addAction(save_as_);
	addToolBar(Qt::TopToolBarArea, main_toolbar);
	
	QToolBar *edit_toolbar = new QToolBar(tr("Édition", "toolbar title"), this);
	edit_toolbar -> setObjectName("tbt_edit_toolbar");
	edit_toolbar -> addAction(undo_);
	edit_toolbar -> addAction(redo_);
	edit_toolbar -> addSeparator();
	edit_toolbar -> addAction(merge_cells_);
	edit_toolbar -> addAction(split_cell_);
	addToolBar(Qt::TopToolBarArea, edit_toolbar);
	
	QToolBar *display_toolbar = new QToolBar(tr("Affichage", "toolbar title"), this);
	display_toolbar -> setObjectName("tbt_display_toolbar");
	display_toolbar -> addAction(zoom_in_);
	display_toolbar -> addAction(zoom_out_);
	display_toolbar -> addAction(zoom_fit_);
	display_toolbar -> addAction(zoom_reset_);
	addToolBar(Qt::TopToolBarArea, display_toolbar);
}

/**
	Initialize layouts and widgets
*/
void QETTitleBlockTemplateEditor::initWidgets()
{
	QSettings settings;
	
	// undo list on the right
	undo_stack_ = new QUndoStack(this);
	undo_view_ = new QUndoView(undo_stack_);
	undo_view_ -> setEmptyLabel(tr("Aucune modification", "label displayed in the undo list when empty"));
	
	undo_dock_widget_ = new QDockWidget(tr("Annulations", "dock title"));
	undo_dock_widget_ -> setObjectName("tbt_undo_dock");
	undo_dock_widget_ -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	undo_dock_widget_ -> setWidget(undo_view_);
	undo_dock_widget_ -> setMinimumWidth(290);
	addDockWidget(Qt::RightDockWidgetArea, undo_dock_widget_);
	
	// WYSIWYG editor as central widget
	template_edition_area_scene_ = new QGraphicsScene(this);
	template_edition_area_view_  = new TitleBlockTemplateView(template_edition_area_scene_);
	bool conv_ok;
	int conf_preview_width = settings.value("titleblocktemplateeditor/preview_width", -1).toInt(&conv_ok);
	if (conv_ok && conf_preview_width != -1) {
		template_edition_area_view_ -> setPreviewWidth(conf_preview_width);
	}
	setCentralWidget(template_edition_area_view_);
	
	// cell edition widget at the bottom
	template_cell_editor_widget_ = new TitleBlockTemplateCellWidget(tb_template_);
	template_cell_editor_dock_widget_ = new QDockWidget(tr("Propriétés de la cellule", "dock title"), this);
	template_cell_editor_dock_widget_ -> setObjectName("tbt_celleditor_dock");
	template_cell_editor_dock_widget_ -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	template_cell_editor_dock_widget_ -> setWidget(template_cell_editor_widget_);
	template_cell_editor_dock_widget_ -> setMinimumWidth(180);
	template_cell_editor_dock_widget_ -> setMinimumHeight(250);
	addDockWidget(Qt::BottomDockWidgetArea, template_cell_editor_dock_widget_);
	template_cell_editor_widget_ -> setVisible(false);
	
	connect(
		template_edition_area_view_,
		SIGNAL(selectedCellsChanged(QList<TitleBlockCell *>)),
		this,
		SLOT(selectedCellsChanged(QList<TitleBlockCell *>))
	);
	connect(template_cell_editor_widget_, SIGNAL(logoEditionRequested()), this, SLOT(editLogos()));
	connect(
		template_cell_editor_widget_,
		SIGNAL(cellModified(ModifyTitleBlockCellCommand *)),
		this,
		SLOT(pushCellUndoCommand(ModifyTitleBlockCellCommand *))
	);
	connect(
		template_edition_area_view_,
		SIGNAL(gridModificationRequested(TitleBlockTemplateCommand *)),
		this,
		SLOT(pushGridUndoCommand(TitleBlockTemplateCommand *))
	);
	connect(
		template_edition_area_view_,
		SIGNAL(previewWidthChanged(int,int)),
		this,
		SLOT(savePreviewWidthToApplicationSettings(int, int))
	);
	connect(undo_stack_, SIGNAL(cleanChanged(bool)), this, SLOT(updateEditorTitle()));
	connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(updateActions()));
}

/**
	Initialize the logo manager
*/
void QETTitleBlockTemplateEditor::initLogoManager() {
	logo_manager_ = new TitleBlockTemplateLogoManager(tb_template_, this);
	logo_manager_ -> setReadOnly(read_only_);
	connect(
		logo_manager_,
		SIGNAL(logosChanged(const TitleBlockTemplate *)),
		template_cell_editor_widget_,
		SLOT(updateLogosComboBox(const TitleBlockTemplate *))
	);
}

/**
	@return a string describing what is being edited, along with [Changed] or
	[Read only] tags. Useful to compose the window title.
*/
QString QETTitleBlockTemplateEditor::currentlyEditedTitle() const {
	QString titleblock_title;
	if (opened_from_file_) {
		titleblock_title = filepath_;
	} else {
		titleblock_title = location_.name();
	}
	
	// if a (file)name has been added, also add a "[Changed]" tag if needed
	if (!titleblock_title.isEmpty()) {
		QString tag;
		if (!undo_stack_ -> isClean()) {
			tag = tr("[Modifié]", "window title tag");
		}
		if (read_only_) {
			tag = tr("[Lecture seule]", "window title tag");
		}
		titleblock_title = QString(
			tr(
				"%1 %2",
				"part of the window title - %1 is the filepath or template name, %2 is the [Changed] or [Read only] tag"
			)
		).arg(titleblock_title).arg(tag);
	}
	
	return(titleblock_title);
}

/**
 * @brief QETTitleBlockTemplateEditor::readSettings
 * Read settings
 */
void QETTitleBlockTemplateEditor::readSettings()
{
	QSettings settings;
	
	// window size and position
	QVariant geometry = settings.value("titleblocktemplateeditor/geometry");
	if (geometry.isValid()) restoreGeometry(geometry.toByteArray());
	
	// window state (toolbars, docks...)
	QVariant state = settings.value("titleblocktemplateeditor/state");
	if (state.isValid()) restoreState(state.toByteArray());
}

/**
 * @brief QETTitleBlockTemplateEditor::writeSettings
 * Write the settings
 */
void QETTitleBlockTemplateEditor::writeSettings()
{
	QSettings settings;
	settings.setValue("titleblocktemplateeditor/geometry", saveGeometry());
	settings.setValue("titleblocktemplateeditor/state", saveState());
}

/**
	Update various things when user changes the selected cells.
	@param selected_cells List of selected cells.
*/
void QETTitleBlockTemplateEditor::selectedCellsChanged(QList<TitleBlockCell *> selected_cells) {
	if (selected_cells.count() == 1) {
		template_cell_editor_widget_ -> edit(selected_cells.at(0));
		template_cell_editor_widget_ -> setVisible(true);
	} else {
		template_cell_editor_widget_ -> setVisible(false);
	}
	updateActions();
}

/**
	Configure an undo Command before adding it to the undo stack.
	@param command to be added to the undo stack
*/
void QETTitleBlockTemplateEditor::pushCellUndoCommand(ModifyTitleBlockCellCommand *command) {
	command -> setView(template_edition_area_view_);
	pushUndoCommand(command);
}

/**
	Add an undo Command to the undo stack.
	@param command QUndoCommand to be added to the undo stack
*/
void QETTitleBlockTemplateEditor::pushGridUndoCommand(TitleBlockTemplateCommand *command) {
	pushUndoCommand(command);
}

/**
	Add an undo Command to the undo stack.
	@param command QUndoCommand to be added to the undo stack
*/
void QETTitleBlockTemplateEditor::pushUndoCommand(QUndoCommand *command) {
	undo_stack_ -> push(command);
}

/**
	Set the title of this editor.
*/
void QETTitleBlockTemplateEditor::updateEditorTitle() {
	// base title
	QString min_title(
		tr(
			"QElectroTech - Éditeur de modèle de cartouche",
			"titleblock template editor: base window title"
		)
	);
	
	// get the currently edited template (file)name
	QString titleblock_title = currentlyEditedTitle();
	
	// generate the final window title
	QString title;
	if (titleblock_title.isEmpty()) {
		title = min_title;
	} else {
		title = QString(
			tr(
				"%1 - %2",
				"window title: %1 is the base window title, %2 is a template name"
			)
		).arg(min_title).arg(titleblock_title);
	}
	setWindowTitle(title);
}

/**
	Ensure the user interface remains consistent by enabling or disabling
	adequate actions.
*/
void QETTitleBlockTemplateEditor::updateActions() {
	save_ -> setEnabled(!read_only_);
	
	bool can_merge;
	bool can_split;
	int count;
	if (!read_only_) {
		template_edition_area_view_ -> analyzeSelectedCells(&can_merge, &can_split, &count);
	}
	cut_ -> setEnabled(!read_only_ && count);
	copy_ -> setEnabled(count);
	paste_ -> setEnabled(!read_only_ && count && template_edition_area_view_ -> mayPaste());
	add_row_ -> setEnabled(!read_only_);
	add_col_ -> setEnabled(!read_only_);
	merge_cells_ -> setEnabled(!read_only_ && can_merge);
	split_cell_ -> setEnabled(!read_only_ && can_split);
}

/**
	Save the template under the provided location.
	@see QETProject::setTemplateXmlDescription()
	@param location Location where the title block template should be saved.
*/
bool QETTitleBlockTemplateEditor::saveAs(const TitleBlockTemplateLocation &location) {
	TitleBlockTemplatesCollection *collection = location.parentCollection();
	if (!collection) return(false);
	
	QDomDocument doc;
	QDomElement elmt = doc.createElement("root");
	tb_template_ -> saveToXmlElement(elmt);
	elmt.setAttribute("name", location.name());
	doc.appendChild(elmt);
	
	collection -> setTemplateXmlDescription(location.name(), elmt);
	
	opened_from_file_ = false;
	location_ = location;
	undo_stack_ -> setClean();
	setReadOnly(false);
	return(true);
}

/**
	Save the template in the provided filepath.
	@see TitleBlockTemplate::saveToXmlFile()
	@param filepath location Location where the title block template should be saved.
*/
bool QETTitleBlockTemplateEditor::saveAs(const QString &filepath) {
	bool saving = tb_template_ -> saveToXmlFile(filepath);
	if (!saving) return(false);
	
	opened_from_file_ = true;
	filepath_ = filepath;
	undo_stack_ -> setClean();
	setReadOnly(false);
	return(true);
}

/**
	Ask the user to choose a title block template from the known collections
	then open it for edition.
*/
void QETTitleBlockTemplateEditor::open() {
	TitleBlockTemplateLocation location = getTitleBlockTemplateLocationFromUser(
		tr("Ouvrir un modèle", "File > open dialog window title"),
		true
	);
	if (location.isValid()) {
		QETApp::instance() -> openTitleBlockTemplate(location);
	}
}

/**
	Ask the user to choose a file supposed to contain a title block template,
	then open it for edition.
*/
void QETTitleBlockTemplateEditor::openFromFile() {
	// directory to show
	QString initial_dir = filepath_.isEmpty() ? QETApp::customTitleBlockTemplatesDir() : QDir(filepath_).absolutePath();
	
	// ask the user to choose a filepath
	QString user_filepath = QFileDialog::getOpenFileName(
		this,
		tr("Ouvrir un fichier", "dialog title"),
		initial_dir,
		tr(
			"Modèles de cartouches QElectroTech (*%1);;"
			"Fichiers XML (*.xml);;"
			"Tous les fichiers (*)",
			"filetypes allowed when opening a title block template file - %1 is the .titleblock extension"
		).arg(QString(TITLEBLOCKS_FILE_EXTENSION))
	);
	
	
	if (!user_filepath.isEmpty()) QETApp::instance() -> openTitleBlockTemplate(user_filepath);
}

/**
	Save the currently edited title block template back to its parent project.
*/
bool QETTitleBlockTemplateEditor::save() {
	if (opened_from_file_) {
		if (!filepath_.isEmpty()) {
			QFileInfo file_path_info(filepath_);
			if (file_path_info.isWritable()) {
				return(saveAs(filepath_));
			}
		}
		return(saveAsFile());
	} else {
		if (location_.isValid()) {
			if (!location_.isReadOnly()) {
				return(saveAs(location_));
			}
		}
		return(saveAs());
	}
}

/**
	Ask the user where he wishes to save the currently edited template.
*/
bool QETTitleBlockTemplateEditor::saveAs() {
	TitleBlockTemplateLocation location = getTitleBlockTemplateLocationFromUser(
		tr("Enregistrer le modèle sous", "dialog window title"),
		false
	);
	if (location.isValid()) {
		return(saveAs(location));
	}
	return(false);
}

/**
	Ask the user where on the filesystem he wishes to save the currently edited template.
*/
bool QETTitleBlockTemplateEditor::saveAsFile() {
	// directory to show
	QString initial_dir = filepath_.isEmpty() ? QETApp::customTitleBlockTemplatesDir() : QDir(filepath_).absolutePath();
	
	// ask the user to choose a target file
	QString filepath = QFileDialog::getSaveFileName(
		this,
		tr("Enregistrer sous", "dialog title"),
		initial_dir,
		tr(
			"Modèles de cartouches QElectroTech (*%1)",
			"filetypes allowed when saving a title block template file - %1 is the .titleblock extension"
		).arg(QString(TITLEBLOCKS_FILE_EXTENSION))
	);
	
	// if no name was entered, return false
	if (filepath.isEmpty()) return(false);
	
	// if the name does not end with ".titleblock", add it
	if (!filepath.endsWith(".titleblock", Qt::CaseInsensitive)) filepath += ".titleblock";
	
	// attempts to save the file
	bool saving = saveAs(filepath);
	
	// retourne un booleen representatif de la reussite de l'enregistrement
	return(saving);
}

/**
	@param read_only True to restrict this editor to visualization of the
	currently edited template, false to allow full edition.
*/
void QETTitleBlockTemplateEditor::setReadOnly(bool read_only) {
	if (read_only != read_only_) {
		read_only_ = read_only;
		if (logo_manager_) {
			logo_manager_ -> setReadOnly(read_only_);
		}
		template_cell_editor_widget_ -> setReadOnly(read_only_);
		template_edition_area_view_ -> setReadOnly(read_only_);
	}
	updateActions();
	updateEditorTitle();
}

/**
	Ask the user for a title block template location
	@param title Title displayed by the dialog window
	@param existing_only True for the user to be forced to choose an existing
	template, false if he may specify the template name
	@return The location chosen by the user, or an empty
	TitleBlockTemplateLocation if the user cancelled the dialog
*/
TitleBlockTemplateLocation QETTitleBlockTemplateEditor::getTitleBlockTemplateLocationFromUser(const QString &title, bool existing_only) {
	TitleBlockTemplateLocationChooser *widget;
	if (existing_only) {
		widget = new TitleBlockTemplateLocationChooser(location());
	} else {
		widget = new TitleBlockTemplateLocationSaver(location());
	}
	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	
	QVBoxLayout *dialog_layout = new QVBoxLayout();
	dialog_layout -> addWidget(widget);
	dialog_layout -> addWidget(buttons);
	
	QDialog dialog;
	dialog.setWindowTitle(title);
	dialog.setLayout(dialog_layout);
	
	connect(buttons, SIGNAL(accepted()), &dialog, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), &dialog, SLOT(reject()));
	
	if (dialog.exec() == QDialog::Accepted) {
		return(widget -> location());
	}
	return TitleBlockTemplateLocation();
}

/**
	Close the current editor.
*/
void QETTitleBlockTemplateEditor::quit() {
	close();
}

/**
 * @brief QETTitleBlockTemplateEditor::savePreviewWidthToApplicationSettings
 * Save the new preview width to application settings
 * @param former_preview_width : former_preview_width Unused, former preview width
 * @param new_preview_width : new_preview_width New preview width
 */
void QETTitleBlockTemplateEditor::savePreviewWidthToApplicationSettings(int former_preview_width, int new_preview_width)
{
	Q_UNUSED(former_preview_width)
	QSettings settings;
	settings.setValue("titleblocktemplateeditor/preview_width", new_preview_width);
}

/**
	Edit extra information attached to the template.
*/
void QETTitleBlockTemplateEditor::editTemplateInformation() {
	if (!tb_template_) return;
	
	QDialog dialog_author(this);
	dialog_author.setModal(true);
#ifdef Q_OS_MAC
	dialog_author.setWindowFlags(Qt::Sheet);
#endif
	dialog_author.setMinimumSize(400, 260);
	dialog_author.setWindowTitle(tr("Éditer les informations complémentaires", "window title"));
	QVBoxLayout *dialog_layout = new QVBoxLayout(&dialog_author);
	
	// explanation label
	QLabel *information_label = new QLabel(tr("Vous pouvez utiliser ce champ libre pour mentionner les auteurs du cartouche, sa licence, ou tout autre renseignement que vous jugerez utile."));
	information_label -> setAlignment(Qt::AlignJustify | Qt::AlignVCenter);
	information_label -> setWordWrap(true);
	dialog_layout -> addWidget(information_label);
	
	// add a QTextEdit to the dialog
	QTextEdit *text_field = new QTextEdit();
	text_field -> setAcceptRichText(false);
	text_field -> setPlainText(tb_template_ -> information());
	text_field -> setReadOnly(read_only_);
	dialog_layout -> addWidget(text_field);
	
	// add two buttons to the dialog
	QDialogButtonBox *dialog_buttons = new QDialogButtonBox(read_only_ ? QDialogButtonBox::Ok : QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	dialog_layout -> addWidget(dialog_buttons);
	connect(dialog_buttons, SIGNAL(accepted()),    &dialog_author, SLOT(accept()));
	connect(dialog_buttons, SIGNAL(rejected()),    &dialog_author, SLOT(reject()));
	
	// run the dialog
	if (dialog_author.exec() == QDialog::Accepted && !read_only_) {
		QString new_info = text_field -> toPlainText().remove(QChar(13)); // CR-less text
		if (new_info != tb_template_ -> information()) {
			pushUndoCommand(new ChangeTemplateInformationsCommand(tb_template_, tb_template_ -> information(), new_info));
		}
	}
}
