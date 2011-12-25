/*
	Copyright 2006-2011 Xavier Guerrin
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
#include "qeticons.h"
#include "qetapp.h"
#include "qetproject.h"
#include "templatecellwidget.h"
#include "templatecommands.h"
#include "templateview.h"
#include "templatelogomanager.h"
#include "templatecellwidget.h"

/**
	@param parent parent QWidget of this window
*/
QETTitleBlockTemplateEditor::QETTitleBlockTemplateEditor(QWidget *parent) :
	QMainWindow(parent),
	read_only(false),
	parent_project_(0),
	tb_template_(0),
	logo_manager_(0)
{
	initWidgets();
	initActions();
	initMenus();
}

/**
	Destructor
*/
QETTitleBlockTemplateEditor::~QETTitleBlockTemplateEditor() {
}

/**
	Edit the given template.
	@param project Parent project of the template to edit.
	@param template_name Name of the template to edit within its parent project.
	@return true if this editor was able to edit the given template, false otherwise
*/
bool QETTitleBlockTemplateEditor::edit(QETProject *project, const QString &template_name) {
	// we require a project we will rattach templates to
	if (!project) return(false);
	
	// the template name may be empty to create a new one
	const TitleBlockTemplate *tb_template_orig;
	if (template_name.isEmpty()) {
		// loads the default title block template provided by the application
		// it will be used as a start point to design the title block
		tb_template_orig = QETApp::defaultTitleBlockTemplate();
	} else {
		tb_template_orig = project -> getTemplateByName(template_name);
	}
	
	if (!tb_template_orig) {
		/// TODO The TBT does not exist, manage error
		return(false);
	}
	
	tb_template_ = tb_template_orig -> clone();
	parent_project_ = project;
	template_name_ = template_name;
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
			logo_manager_ = new TitleBlockTemplateLogoManager(tb_template_);
		}
		logo_manager_ -> show();
		template_cell_editor_widget_ -> updateLogosComboBox(tb_template_);
	}
}

/**
	Initialize the various actions.
*/
void QETTitleBlockTemplateEditor::initActions() {
	QETApp *qet_app = QETApp::instance();
	
	save_           = new QAction(QET::Icons::DocumentSave,         tr("&Enregistrer",                 "menu entry"), this);
	quit_           = new QAction(QET::Icons::ApplicationExit,      tr("&Quitter",                     "menu entry"), this);
	configure_      = new QAction(QET::Icons::Configure,            tr("&Configurer QElectroTech",     "menu entry"), this);
	about_qet_      = new QAction(QET::Icons::QETLogo,              tr("\300 &propos de QElectroTech", "menu entry"), this);
	about_qt_       = new QAction(QET::Icons::QtLogo,               tr("\300 propos de &Qt",           "menu entry"), this);
	merge_cells_    = new QAction(                                  tr("&Fusionner les cellules",      "menu entry"), this);
	split_cell_     = new QAction(                                  tr("&S\351parer les cellules",     "menu entry"), this);
	
	save_             -> setShortcut(QKeySequence::Save);
	quit_             -> setShortcut(QKeySequence(tr("Ctrl+Q", "shortcut to quit")));
	merge_cells_      -> setShortcut(QKeySequence(tr("Ctrl+K", "shortcut to merge cells")));
	split_cell_       -> setShortcut(QKeySequence(tr("Ctrl+J", "shortcut to split merged cell")));
	
	configure_    -> setStatusTip(tr("Permet de r\351gler diff\351rents param\350tres de QElectroTech", "status bar tip"));
	about_qet_    -> setStatusTip(tr("Affiche des informations sur QElectroTech",                       "status bar tip"));
	about_qt_     -> setStatusTip(tr("Affiche des informations sur la biblioth\350que Qt",              "status bar tip"));
	
	connect(save_,            SIGNAL(triggered()), this,     SLOT(save()));
	connect(quit_,            SIGNAL(triggered()), this,     SLOT(quit()));
	connect(configure_,       SIGNAL(triggered()), qet_app,  SLOT(configureQET()));
	connect(about_qet_,       SIGNAL(triggered()), qet_app,  SLOT(aboutQET()));
	connect(about_qt_,        SIGNAL(triggered()), qet_app,  SLOT(aboutQt()));
	connect(merge_cells_,     SIGNAL(triggered()), template_edition_area_view_, SLOT(mergeSelectedCells()));
	connect(split_cell_,      SIGNAL(triggered()), template_edition_area_view_, SLOT(splitSelectedCell()));
}

/**
	Initialize the various menus.
*/
void QETTitleBlockTemplateEditor::initMenus() {
	file_menu_    = new QMenu(tr("&Fichier",        "menu title"), this);
	edit_menu_    = new QMenu(tr("&\311dition",     "menu title"), this);
	config_menu_  = new QMenu(tr("&Configuration",  "menu title"), this);
	help_menu_    = new QMenu(tr("&Aide",           "menu title"), this);
	
	file_menu_    -> setTearOffEnabled(true);
	edit_menu_    -> setTearOffEnabled(true);
	config_menu_  -> setTearOffEnabled(true);
	help_menu_    -> setTearOffEnabled(true);
	
	file_menu_    -> addAction(save_);
	file_menu_    -> addSeparator();
	file_menu_    -> addAction(quit_);
	
	edit_menu_   -> addAction(merge_cells_);
	edit_menu_   -> addAction(split_cell_);
	
	config_menu_ -> addAction(configure_);
	
	help_menu_ -> addAction(about_qet_);
	help_menu_ -> addAction(about_qt_);
	
	menuBar() -> addMenu(file_menu_);
	menuBar() -> addMenu(edit_menu_);
	menuBar() -> addMenu(config_menu_);
	menuBar() -> addMenu(help_menu_);
}

/**
	Initialize layouts and widgets
*/
void QETTitleBlockTemplateEditor::initWidgets() {
	// undo list on the right
	undo_stack_ = new QUndoStack(this);
	undo_view_ = new QUndoView(undo_stack_);
	undo_view_ -> setEmptyLabel(tr("Aucune modification", "label displayed in the undo list when empty"));
	
	undo_dock_widget_ = new QDockWidget(tr("Annulations", "dock title"));
	undo_dock_widget_ -> setFeatures(QDockWidget::AllDockWidgetFeatures);
	undo_dock_widget_ -> setWidget(undo_view_);
	undo_dock_widget_ -> setMinimumWidth(290);
	addDockWidget(Qt::RightDockWidgetArea, undo_dock_widget_);
	
	// WYSIWYG editor as central widget
	template_edition_area_scene_ = new QGraphicsScene(this);
	template_edition_area_view_  = new TitleBlockTemplateView(template_edition_area_scene_); 
	setCentralWidget(template_edition_area_view_);
	
	// cell edition widget at the bottom
	template_cell_editor_widget_ = new TitleBlockTemplateCellWidget(tb_template_);
	template_cell_editor_dock_widget_ = new QDockWidget(tr("Propri\351t\351s de la cellule", "dock title"), this);
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
	QString min_title(
		tr(
			"QElectroTech - \311diteur de mod\350le de cartouche",
			"titleblock template editor: base window title"
		)
	);
	
	QString title;
	if (template_name_.isEmpty()) {
		title = min_title;
	} else {
		title = QString(
			tr(
				"%1 - %2",
				"window title: %1 is the base window title, %2 is a template name"
			)
		).arg(min_title).arg(template_name_);
	}
	setWindowTitle(title);
}

/**
	Save the currently edited title block template back to its parent project.
*/
void QETTitleBlockTemplateEditor::save() {
	QDomDocument doc;
	QDomElement elmt = doc.createElement("root");
	tb_template_ -> saveToXmlElement(elmt);
	doc.appendChild(elmt);
	
	if (parent_project_ && !template_name_.isEmpty()) {
		parent_project_ -> setTemplateXmlDescription(template_name_, elmt);
	}
}

/**
	Close the current editor.
*/
void QETTitleBlockTemplateEditor::quit() {
	/// TODO save if needed
	close();
}
