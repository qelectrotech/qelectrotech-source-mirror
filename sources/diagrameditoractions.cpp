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
#include "diagrameditoractions.h"

#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "diagramview.h"
#include "elementspanelwidget.h"
#include "factory/qetgraphicstablefactory.h"
#include "print/projectprintwindow.h"
#include "projectview.h"
#include "qet.h"
#include "qetdiagrameditor.h"
#include "qeticons.h"
#include "qetproject.h"
#include "QWidgetAnimation/qwidgetanimation.h"
#include "TerminalStrip/ui/terminalstripeditorwindow.h"
#include "conductornumexport.h"
#include "shortcutmanager.h"
#include "ui/bomexportdialog.h"
#include "ui/jumptoelementdialog.h"
#include "wiringlistexport.h"

#include <QMdiArea>
#include <QSettings>

/**
	@brief DiagramEditorActions::DiagramEditorActions
	Construct and set up every QAction/QActionGroup owned by @a editor.
	@param editor : the editor window this action pool belongs to. Actions
	are parented to @a editor (their QObject lifetime is unchanged from
	before this class existed), and lambdas/connections that need to reach
	the editor's own methods or members go through @a editor rather than a
	captured `this`, since this class is not itself a QObject/QWidget.
*/
DiagramEditorActions::DiagramEditorActions(QETDiagramEditor *editor) :
	m_row_column_actions_group (editor),
	m_selection_actions_group  (editor),
	m_add_item_actions_group   (editor),
	m_zoom_actions_group       (editor),
	m_select_actions_group     (editor),
	m_file_actions_group       (editor)
{
		//Export to another file type (jpeg, dxf etc...)
	m_export_to_images = new QAction(QET::Icons::DocumentExport,  editor->tr("E&xporter"), editor);
	ShortcutManager::instance().registerAction(m_export_to_images, "diagrameditor.export_to_images", editor->tr("Éditeur de schémas"), Qt::CTRL | Qt::SHIFT | Qt::Key_X);
	m_export_to_images->setStatusTip(editor->tr("Exporte le folio courant dans un autre format", "status bar tip"));
	QObject::connect(m_export_to_images, &QAction::triggered, [editor]() {
		ProjectView *current_project = editor->currentProjectView();
		if (current_project) {
			current_project -> exportProject();
		}
	});

		//Print
	m_print = new QAction(QET::Icons::DocumentPrint,   editor->tr("Imprimer"),  editor);
	ShortcutManager::instance().registerAction(m_print, "diagrameditor.print", editor->tr("Éditeur de schémas"), QKeySequence::Print);
	m_print->setStatusTip(editor->tr("Imprime un ou plusieurs folios du projet courant", "status bar tip"));
	QObject::connect(m_print, &QAction::triggered, [editor]() {
		auto project = editor->currentProject();
		if (project) {
			ProjectPrintWindow::launchDialog(project, QPrinter::NativeFormat ,editor);
		}
	});

		//export to pdf
	m_export_to_pdf = new QAction(QET::Icons::PDF, editor->tr("Exporter en pdf"), editor);
	m_export_to_pdf->setStatusTip(editor->tr("Exporte un ou plusieurs folios du projet courant", "status bar tip"));
	QObject::connect(m_export_to_pdf, &QAction::triggered, [editor] () {
		auto project = editor->currentProject();
		if (project) {
			ProjectPrintWindow::launchDialog(project, QPrinter::PdfFormat, editor);
		}
	});

		//Quit editor
	m_quit_editor = new QAction(QET::Icons::ApplicationExit, editor->tr("&Quitter"),  editor);
	ShortcutManager::instance().registerAction(m_quit_editor, "diagrameditor.quit", editor->tr("Éditeur de schémas"), Qt::CTRL | Qt::Key_Q);
	m_quit_editor->setStatusTip(editor->tr("Ferme l'application QElectroTech", "status bar tip"));
	QObject::connect(m_quit_editor, &QAction::triggered, editor, &QETDiagramEditor::close);

		//Undo
	undo = editor->undo_group.createUndoAction(editor, editor->tr("Annuler"));
	undo->setIcon(QET::Icons::EditUndo);
	ShortcutManager::instance().registerAction(undo, "diagrameditor.undo", editor->tr("Éditeur de schémas"), QKeySequence::Undo);
	undo->setStatusTip(editor->tr("Annule l'action précédente", "status bar tip"));
		//Redo
	redo = editor->undo_group.createRedoAction(editor, editor->tr("Refaire"));
	redo->setIcon(QET::Icons::EditRedo);
	ShortcutManager::instance().registerAction(redo, "diagrameditor.redo", editor->tr("Éditeur de schémas"), QKeySequence::Redo);
	redo->setStatusTip(editor->tr("Restaure l'action annulée", "status bar tip"));

		//cut copy past
	m_cut   = new QAction(QET::Icons::EditCut,   editor->tr("Co&uper"), editor);
	m_copy  = new QAction(QET::Icons::EditCopy,  editor->tr("Cop&ier"), editor);
	m_paste = new QAction(QET::Icons::EditPaste, editor->tr("C&oller"), editor);

	ShortcutManager::instance().registerAction(m_cut, "diagrameditor.cut", editor->tr("Éditeur de schémas"), QKeySequence::Cut);
	ShortcutManager::instance().registerAction(m_copy, "diagrameditor.copy", editor->tr("Éditeur de schémas"), QKeySequence::Copy);
	ShortcutManager::instance().registerAction(m_paste, "diagrameditor.paste", editor->tr("Éditeur de schémas"), QKeySequence::Paste);

	m_cut   -> setStatusTip(editor->tr("Transfère les éléments sélectionnés dans le presse-papier", "status bar tip"));
	m_copy  -> setStatusTip(editor->tr("Copie les éléments sélectionnés dans le presse-papier", "status bar tip"));
	m_paste -> setStatusTip(editor->tr("Place les éléments du presse-papier sur le folio", "status bar tip"));

	QObject::connect(m_cut, &QAction::triggered, [editor]() {
		if (editor->currentDiagramView())
			editor->currentDiagramView()->cut();
	});
	QObject::connect(m_copy, &QAction::triggered, [editor]() {
		if (editor->currentDiagramView())
			editor->currentDiagramView()->copy();
	});
	QObject::connect(m_paste, &QAction::triggered, [editor]() {
		if(editor->currentDiagramView())
			editor->currentDiagramView()->paste();
	});

		//Reset conductor path
	m_conductor_reset = new QAction(QET::Icons::ConductorSettings,     editor->tr("Réinitialiser les conducteurs"),        editor);
	ShortcutManager::instance().registerAction(m_conductor_reset, "diagrameditor.conductor_reset", editor->tr("Éditeur de schémas"), Qt::CTRL | Qt::Key_K);
	m_conductor_reset->setStatusTip(editor->tr("Recalcule les chemins des conducteurs sans tenir compte des modifications", "status bar tip"));
	QObject::connect(m_conductor_reset, &QAction::triggered, [editor]() {
		if (DiagramView *dv = editor->currentDiagramView())
			dv->resetConductors();
	});

		//AutoConductor
	m_auto_conductor = new QAction   (QET::Icons::Autoconnect, editor->tr("Création automatique de conducteur(s)","Tool tip of auto conductor"), editor);
	m_auto_conductor->setStatusTip (editor->tr("Utiliser la création automatique de conducteur(s) quand cela est possible", "Status tip of auto conductor"));
	m_auto_conductor->setCheckable (true);
	QObject::connect(m_auto_conductor, &QAction::triggered, [editor](bool ac) {
		if (ProjectView *pv = editor->currentProjectView())
			pv->project()->setAutoConductor(ac);
	});

		//AutoBreakConductor
	m_auto_break_conductor = new QAction   (QET::Icons::Conductor, editor->tr("Coupure automatique de conducteur(s)","Tool tip of auto break conductor"), editor);
	m_auto_break_conductor->setStatusTip (editor->tr("Couper automatiquement les conducteurs existants lors du placement d'un élément", "Status tip of auto break conductor"));
	m_auto_break_conductor->setCheckable (true);
	{
		QSettings settings;
		m_auto_break_conductor->setChecked(settings.value("diagrameditor/auto_break_conductor", false).toBool());
	}
	QObject::connect(m_auto_break_conductor, &QAction::triggered, [editor](bool abc) {
		QSettings settings;
		settings.setValue("diagrameditor/auto_break_conductor", abc);
		if (ProjectView *pv = editor->currentProjectView())
			pv->project()->setAutoBreakConductor(abc);
	});

		//Switch background color
	m_grey_background = new QAction   (QET::Icons::DiagramBg, editor->tr("Couleur de fond blanc/gris","Tool tip of white/grey background button"), editor);
	m_grey_background -> setStatusTip (editor->tr("Affiche la couleur de fond du folio en blanc ou en gris", "Status tip of white/grey background button"));
	m_grey_background -> setCheckable (true);
	QObject::connect (m_grey_background, &QAction::triggered, [editor](bool checked) {
		Diagram::background_color = checked ? Qt::darkGray : Qt::white;
		if (editor->currentDiagramView() &&  editor->currentDiagramView()->diagram())
			editor->currentDiagramView()->diagram()->update();
	});

		//Draw or not the background grid
	m_draw_grid = new QAction ( QET::Icons::Grid, editor->tr("Afficher la grille"), editor);
	m_draw_grid->setStatusTip(editor->tr("Affiche ou masque la grille des folios"));
	QSettings settings;
	m_draw_grid->setCheckable(true);
	m_draw_grid->setChecked(settings.value("diagrameditor/grid_display_startup", true).toBool());
	QObject::connect(m_draw_grid, &QAction::triggered, [editor](bool checked) {
		foreach (ProjectView *prjv, editor->openedProjects())
			foreach (Diagram *d, prjv->project()->diagrams()) {
				d->setDisplayGrid(checked);
				d->update();
			}
	});

	// Draw or not the custom guides
	m_draw_guides = new QAction ( QIcon(":/ico/22x22/guides.png"), editor->tr("Afficher les guides"), editor);
	m_draw_guides->setStatusTip(editor->tr("Affiche ou masque les guides"));
	m_draw_guides->setCheckable(true);
	m_draw_guides->setChecked(settings.value("diagrameditor/guides_display_startup", false).toBool());
	QObject::connect(m_draw_guides, &QAction::triggered, [editor](bool checked) {
		foreach (ProjectView *prjv, editor->openedProjects())
			foreach (Diagram *d, prjv->project()->diagrams()) {
				d->setDisplayGuides(checked);
			}
	});

		//Edit current diagram properties
	m_edit_diagram_properties = new QAction(QET::Icons::DialogInformation, editor->tr("Propriétés du folio"), editor);
	ShortcutManager::instance().registerAction(m_edit_diagram_properties, "diagrameditor.edit_diagram_properties", editor->tr("Éditeur de schémas"), Qt::CTRL | Qt::Key_L);
	m_edit_diagram_properties     -> setStatusTip(editor->tr("Édite les propriétés du folio (dimensions, informations du cartouche, propriétés des conducteurs...)", "status bar tip"));
	QObject::connect(m_edit_diagram_properties, &QAction::triggered, [editor]() {
		if (ProjectView *project_view = editor->currentProjectView())
		{
			editor->activateProject(project_view);
			project_view->editCurrentDiagramProperties();
		}
	});

		//Edit current project properties
	m_project_edit_properties = new QAction(QET::Icons::ProjectProperties, editor->tr("Propriétés du projet"), editor);
	QObject::connect(m_project_edit_properties, &QAction::triggered, [editor]() {
		editor->editProjectProperties(editor->currentProjectView());
	});

		//Add new folio to current project
	m_project_add_diagram = new QAction(QET::Icons::DiagramAdd, editor->tr("Ajouter un folio"), editor);
	ShortcutManager::instance().registerAction(m_project_add_diagram, "diagrameditor.project_add_diagram", editor->tr("Éditeur de schémas"), Qt::CTRL | Qt::Key_T);
	QObject::connect(m_project_add_diagram, &QAction::triggered, [editor]() {
		if (ProjectView *current_project = editor->currentProjectView()) {
			current_project->project()->addNewDiagram();
		}
	});

		//Remove current folio from current project
	m_remove_diagram_from_project = new QAction(QET::Icons::DiagramDelete, editor->tr("Supprimer le folio"), editor);
	QObject::connect(m_remove_diagram_from_project, &QAction::triggered, editor, &QETDiagramEditor::removeDiagramFromProject);

		//Clean the current project
	m_clean_project         = new QAction(QET::Icons::EditClear,             editor->tr("Nettoyer le projet"),                   editor);
	QObject::connect(m_clean_project, &QAction::triggered, [editor]() {
		if (ProjectView *current_project = editor->currentProjectView()) {
			if (current_project->cleanProject()) {
				editor->pa -> reloadAndFilter();
			}
		}
	});

		//Export nomenclature to CSV
	m_csv_export = new QAction(QET::Icons::DocumentSpreadsheet, editor->tr("Exporter au format CSV"), editor);
	QObject::connect(m_csv_export, &QAction::triggered, [editor]() {
		BOMExportDialog bom(editor->currentProjectView()->project(), editor);
		bom.exec();
	});

		//Add a nomenclature item
	m_add_nomenclature = new QAction(QET::Icons::TableOfContent, editor->tr("Ajouter une nomenclature"), editor);
	QObject::connect(m_add_nomenclature, &QAction::triggered, editor, [editor]() {
		if(editor->currentDiagramView()) {
			QetGraphicsTableFactory::createAndAddNomenclature(editor->currentDiagramView()->diagram());
		}
	});

		//Add a summary item
	m_add_summary = new QAction(QET::Icons::TableOfContent, editor->tr("Ajouter un sommaire"), editor);
	QObject::connect(m_add_summary, &QAction::triggered, editor, [editor]() {
		if(editor->currentDiagramView()) {
			QetGraphicsTableFactory::createAndAddSummary(editor->currentDiagramView()->diagram());
		}
	});

	m_terminal_strip_dialog = new QAction(QET::Icons::TerminalStrip, editor->tr("Gestionnaire de borniers (DEV)"), editor);
	QObject::connect(m_terminal_strip_dialog, &QAction::triggered, editor, [editor]()
	{
		if (auto project = editor->currentProject())
		{
			TerminalStripEditorWindow::instance(project, editor)->show();
		}
	});

		//Launch the plugin of terminal generator
	m_project_terminalBloc = new QAction(QET::Icons::TerminalStrip, editor->tr("Lancer le plugin de création de borniers"), editor);
	QObject::connect(m_project_terminalBloc, &QAction::triggered, editor, &QETDiagramEditor::generateTerminalBlock);

	//Export conductor num to csv
	m_project_export_conductor_num = new QAction(QET::Icons::DocumentSpreadsheet, editor->tr("Exporter la liste des noms de conducteurs"), editor);
	QObject::connect(m_project_export_conductor_num, &QAction::triggered, [editor]() {
		QETProject *project = editor->currentProject();
		if (project)
		{
			ConductorNumExport wne(project, editor);
			wne.toCsv();
		}
	});
	// Export wiring list to CSV
	m_project_export_wiring_list = new QAction(QET::Icons::DocumentSpreadsheet, editor->tr("Exporter le plan de câblage"), editor);
	QObject::connect(m_project_export_wiring_list, &QAction::triggered, [editor]() {
		QETProject *project = editor->currentProject();
		if (project)
		{
			WiringListExport wle(project, editor);
			wle.toCsv();
		}
	});

	// Terminal Numbering
	m_terminal_numbering = new QAction(QET::Icons::TerminalStrip, editor->tr("Numérotation automatique des bornes"), editor);
	QObject::connect(m_terminal_numbering, &QAction::triggered, editor, &QETDiagramEditor::slot_terminalNumbering);

	#ifdef QET_EXPORT_PROJECT_DB
		m_export_project_db = new QAction(QET::Icons::DocumentSpreadsheet, editor->tr("Exporter la base de donnée interne du projet"), editor);
		QObject::connect(m_export_project_db, &QAction::triggered, [editor]() {
			projectDataBase::exportDb(editor->currentProject()->dataBase(), editor);
		});
	#endif

		//MDI view style
	m_tabbed_view_mode = new QAction(editor->tr("en utilisant des onglets"), editor);
	m_tabbed_view_mode->setStatusTip(editor->tr("Présente les différents projets ouverts des onglets", "status bar tip"));
	m_tabbed_view_mode->setCheckable(true);
	QObject::connect(m_tabbed_view_mode, &QAction::triggered, editor, &QETDiagramEditor::setTabbedMode);

	m_windowed_view_mode = new QAction(editor->tr("en utilisant des fenêtres"), editor);
	m_windowed_view_mode->setStatusTip(editor->tr("Présente les différents projets ouverts dans des sous-fenêtres", "status bar tip"));
	m_windowed_view_mode->setCheckable(true);
	QObject::connect(m_windowed_view_mode, &QAction::triggered, editor, &QETDiagramEditor::setWindowedMode);

	m_group_view_mode = new QActionGroup(editor);
	m_group_view_mode -> addAction(m_windowed_view_mode);
	m_group_view_mode -> addAction(m_tabbed_view_mode);
	m_group_view_mode -> setExclusive(true);

	m_tile_window = new QAction(editor->tr("&Mosaïque"), editor);
	m_tile_window->setStatusTip(editor->tr("Dispose les fenêtres en mosaïque", "status bar tip"));
	QObject::connect(m_tile_window, &QAction::triggered, &editor->m_workspace, &QMdiArea::tileSubWindows);

	m_cascade_window = new QAction(editor->tr("&Cascade"), editor);
	m_cascade_window->setStatusTip(editor->tr("Dispose les fenêtres en cascade", "status bar tip"));
	QObject::connect(m_cascade_window, &QAction::triggered, &editor->m_workspace, &QMdiArea::cascadeSubWindows);

		//Switch selection/view mode
	m_mode_selection = new QAction(QET::Icons::PartSelect, editor->tr("Mode Selection"), editor);
	m_mode_selection->setStatusTip(editor->tr("Permet de sélectionner les éléments", "status bar tip"));
	m_mode_selection->setCheckable(true);
	m_mode_selection->setChecked(true);
	QObject::connect(m_mode_selection, &QAction::triggered, [editor]() {
		if (ProjectView *pv = editor->currentProjectView()) {
			for (DiagramView *dv : pv->diagram_views()) {
				dv->setSelectionMode();
			}
		}
	});

	m_mode_visualise = new QAction(QET::Icons::ViewMove, editor->tr("Mode Visualisation"), editor);
	m_mode_visualise->setStatusTip(editor->tr("Permet de visualiser le folio sans pouvoir le modifier", "status bar tip"));
	m_mode_visualise->setCheckable(true);
	QObject::connect(m_mode_visualise, &QAction::triggered, [editor]() {
		if (ProjectView *pv = editor->currentProjectView()) {
			for(DiagramView *dv : pv->diagram_views()) {
				dv->setVisualisationMode();
			}
		}
	});

	grp_visu_sel = new QActionGroup(editor);
	grp_visu_sel->addAction(m_mode_selection);
	grp_visu_sel->addAction(m_mode_visualise);
	grp_visu_sel->setExclusive(true);

		//Navigate next/previous project
	m_next_window = new QAction(editor->tr("Projet suivant"), editor);
	ShortcutManager::instance().registerAction(m_next_window, "diagrameditor.next_window", editor->tr("Éditeur de schémas"), QKeySequence::NextChild);
	m_next_window->setStatusTip(editor->tr("Active le projet suivant", "status bar tip"));
	QObject::connect(m_next_window, &QAction::triggered, &editor->m_workspace, &QMdiArea::activateNextSubWindow);

	m_previous_window = new QAction(editor->tr("Projet précédent"), editor);
	ShortcutManager::instance().registerAction(m_previous_window, "diagrameditor.previous_window", editor->tr("Éditeur de schémas"), QKeySequence::PreviousChild);
	m_previous_window->setStatusTip(editor->tr("Active le projet précédent", "status bar tip"));
	QObject::connect(m_previous_window, &QAction::triggered, &editor->m_workspace, &QMdiArea::activatePreviousSubWindow);

		//Files action
	QAction *new_file  = m_file_actions_group.addAction(QET::Icons::ProjectNew,     editor->tr("&Nouveau"));
	QAction *open_file = m_file_actions_group.addAction(QET::Icons::DocumentOpen,   editor->tr("&Ouvrir"));
	m_save_file        = m_file_actions_group.addAction(QET::Icons::DocumentSave,   editor->tr("&Enregistrer"));
	m_save_file_as     = m_file_actions_group.addAction(QET::Icons::DocumentSaveAs, editor->tr("Enregistrer sous"));
	m_close_file       = m_file_actions_group.addAction(QET::Icons::ProjectClose,   editor->tr("&Fermer"));

	ShortcutManager::instance().registerAction(new_file, "diagrameditor.new_file", editor->tr("Éditeur de schémas"), QKeySequence::New);
	ShortcutManager::instance().registerAction(open_file, "diagrameditor.open_file", editor->tr("Éditeur de schémas"), QKeySequence::Open);
	ShortcutManager::instance().registerAction(m_close_file, "diagrameditor.close_file", editor->tr("Éditeur de schémas"), QKeySequence::Close);
	ShortcutManager::instance().registerAction(m_save_file, "diagrameditor.save_file", editor->tr("Éditeur de schémas"), QKeySequence::Save);
	ShortcutManager::instance().registerAction(m_save_file_as, "diagrameditor.save_file_as", editor->tr("Éditeur de schémas"), Qt::CTRL | Qt::SHIFT | Qt::Key_S);

	new_file     ->setStatusTip( editor->tr("Crée un nouveau projet", "status bar tip") );
	open_file    ->setStatusTip( editor->tr("Ouvre un projet existant", "status bar tip") );
	m_close_file ->setStatusTip( editor->tr("Ferme le projet courant", "status bar tip") );
	m_save_file    ->setStatusTip( editor->tr("Enregistre le projet courant et tous ses folios", "status bar tip") );
	m_save_file_as ->setStatusTip( editor->tr("Enregistre le projet courant avec un autre nom de fichier", "status bar tip") );

	QObject::connect(m_save_file_as, &QAction::triggered, editor, &QETDiagramEditor::saveAs);
	QObject::connect(m_save_file,    &QAction::triggered, editor, &QETDiagramEditor::save);
	QObject::connect(new_file,       &QAction::triggered, editor, &QETDiagramEditor::newProject);
	QObject::connect(open_file,      &QAction::triggered, editor, &QETDiagramEditor::openProject);
	QObject::connect(m_close_file,   &QAction::triggered, [editor]() {
		if (ProjectView *project_view = editor->currentProjectView()) {
			editor->closeProject(project_view);
		}
	});

		//Rows and Columns
	QAction *add_column    = m_row_column_actions_group.addAction( QET::Icons::EditTableInsertColumnRight, editor->tr("Ajouter une colonne") );
	QAction *remove_column = m_row_column_actions_group.addAction( QET::Icons::EditTableDeleteColumn,      editor->tr("Enlever une colonne") );
	QAction *add_row       = m_row_column_actions_group.addAction( QET::Icons::EditTableInsertRowUnder,    editor->tr("Ajouter une ligne", "Add row") );
	QAction *remove_row    = m_row_column_actions_group.addAction( QET::Icons::EditTableDeleteRow,         editor->tr("Enlever une ligne","Remove row") );

	add_column    -> setStatusTip( editor->tr("Ajoute une colonne au folio", "status bar tip"));
	remove_column -> setStatusTip( editor->tr("Enlève une colonne au folio", "status bar tip"));
	add_row       -> setStatusTip( editor->tr("Agrandit le folio en hauteur", "status bar tip"));
	remove_row    -> setStatusTip( editor->tr("Rétrécit le folio en hauteur", "status bar tip"));

	add_column   ->setData("add_column");
	remove_column->setData("remove_column");
	add_row      ->setData("add_row");
	remove_row   ->setData("remove_row");

	QObject::connect(&m_row_column_actions_group, &QActionGroup::triggered, editor, &QETDiagramEditor::rowColumnGroupTriggered);

		//Selections Actions (related to a selected item)
	m_delete_selection     = m_selection_actions_group.addAction( QET::Icons::EditDelete,        editor->tr("Supprimer")                 );
	m_rotate_selection     = m_selection_actions_group.addAction( QET::Icons::TransformRotate,   editor->tr("Pivoter")                   );
	m_rotate_group_selection = m_selection_actions_group.addAction( QET::Icons::TransformRotate, editor->tr("Pivoter le groupe")         );
	m_rotate_texts         = m_selection_actions_group.addAction( QET::Icons::ObjectRotateRight, editor->tr("Orienter les textes")       );
	m_find_element         = m_selection_actions_group.addAction( QET::Icons::ZoomDraw,          editor->tr("Retrouver dans le panel")   );
	m_edit_selection       = m_selection_actions_group.addAction( QET::Icons::ElementEdit,       editor->tr("Éditer l'item sélectionné") );
	m_group_selected_texts = m_selection_actions_group.addAction( QET::Icons::textGroup,         editor->tr("Grouper les textes sélectionnés"));

	ShortcutManager::instance().registerAction(m_delete_selection, "diagrameditor.delete_selection", editor->tr("Éditeur de schémas"), Qt::Key_Delete);
	ShortcutManager::instance().registerAction(m_rotate_selection, "diagrameditor.rotate_selection", editor->tr("Éditeur de schémas"), Qt::Key_Space);
	ShortcutManager::instance().registerAction(m_rotate_group_selection, "diagrameditor.rotate_group_selection", editor->tr("Éditeur de schémas"), Qt::SHIFT | Qt::Key_Space);
	ShortcutManager::instance().registerAction(m_rotate_texts, "diagrameditor.rotate_texts", editor->tr("Éditeur de schémas"), Qt::CTRL | Qt::Key_Space);
	ShortcutManager::instance().registerAction(m_edit_selection, "diagrameditor.edit_selection", editor->tr("Éditeur de schémas"), Qt::CTRL | Qt::Key_E);

	m_delete_selection->setStatusTip( editor->tr("Enlève les éléments sélectionnés du folio", "status bar tip"));
	m_rotate_selection->setStatusTip( editor->tr("Pivote les éléments et textes sélectionnés", "status bar tip"));
	m_rotate_group_selection->setStatusTip( editor->tr("Pivote la sélection comme un groupe autour de son centre, au lieu de chaque élément sur place", "status bar tip"));
	m_rotate_texts    ->setStatusTip( editor->tr("Pivote les textes sélectionnés à un angle précis", "status bar tip"));
	m_find_element    ->setStatusTip( editor->tr("Retrouve l'élément sélectionné dans le panel", "status bar tip"));

	m_delete_selection    ->setData("delete_selection");
	m_rotate_selection    ->setData("rotate_selection");
	m_rotate_group_selection->setData("rotate_group_selection");
	m_rotate_texts        ->setData("rotate_selected_text");
	m_find_element        ->setData("find_selected_element");
	m_edit_selection      ->setData("edit_selected_element");
	m_group_selected_texts->setData("group_selected_texts");

	QObject::connect(&m_selection_actions_group, &QActionGroup::triggered, editor, &QETDiagramEditor::selectionGroupTriggered);

		//Select Action
	QAction *select_all     = m_select_actions_group.addAction( QET::Icons::EditSelectAll,      editor->tr("Tout sélectionner") );
	QAction *select_nothing = m_select_actions_group.addAction( QET::Icons::EditSelectNone,     editor->tr("Désélectionner tout") );
	QAction *select_invert  = m_select_actions_group.addAction( QET::Icons::EditSelectInvert,   editor->tr("Inverser la sélection") );

	ShortcutManager::instance().registerAction(select_all, "diagrameditor.select_all", editor->tr("Éditeur de schémas"), QKeySequence::SelectAll);
	ShortcutManager::instance().registerAction(select_nothing, "diagrameditor.select_nothing", editor->tr("Éditeur de schémas"), QKeySequence::Deselect);
	ShortcutManager::instance().registerAction(select_invert, "diagrameditor.select_invert", editor->tr("Éditeur de schémas"), Qt::CTRL | Qt::Key_I);

	select_all    ->setStatusTip( editor->tr("Sélectionne tous les éléments du folio", "status bar tip") );
	select_nothing->setStatusTip( editor->tr("Désélectionne tous les éléments du folio", "status bar tip") );
	select_invert ->setStatusTip( editor->tr("Désélectionne les éléments sélectionnés et sélectionne les éléments non sélectionnés", "status bar tip") );

	select_all    ->setData("select_all");
	select_nothing->setData("deselect");
	select_invert ->setData("invert_selection");

	QObject::connect(&m_select_actions_group, &QActionGroup::triggered, editor, &QETDiagramEditor::selectGroupTriggered);

		//Zoom actions
	QAction *zoom_in      = m_zoom_actions_group.addAction( QET::Icons::ZoomIn,       editor->tr("Zoom avant"));
	QAction *zoom_out     = m_zoom_actions_group.addAction( QET::Icons::ZoomOut,      editor->tr("Zoom arrière"));
	QAction *zoom_content = m_zoom_actions_group.addAction( QET::Icons::ZoomDraw,     editor->tr("Zoom sur le contenu"));
	QAction *zoom_fit     = m_zoom_actions_group.addAction( QET::Icons::ZoomFitBest,  editor->tr("Zoom adapté"));
	QAction *zoom_reset   = m_zoom_actions_group.addAction( QET::Icons::ZoomOriginal, editor->tr("Pas de zoom"));
	m_zoom_action_toolBar << zoom_content << zoom_fit << zoom_reset;

	ShortcutManager::instance().registerAction(zoom_in, "diagrameditor.zoom_in", editor->tr("Éditeur de schémas"), QKeySequence::ZoomIn);
	ShortcutManager::instance().registerAction(zoom_out, "diagrameditor.zoom_out", editor->tr("Éditeur de schémas"), QKeySequence::ZoomOut);
	ShortcutManager::instance().registerAction(zoom_content, "diagrameditor.zoom_content", editor->tr("Éditeur de schémas"), Qt::CTRL | Qt::Key_8);
	ShortcutManager::instance().registerAction(zoom_fit, "diagrameditor.zoom_fit", editor->tr("Éditeur de schémas"), Qt::CTRL | Qt::Key_9);
	ShortcutManager::instance().registerAction(zoom_reset, "diagrameditor.zoom_reset", editor->tr("Éditeur de schémas"), Qt::CTRL | Qt::Key_0);

	zoom_in     ->setStatusTip(editor->tr("Agrandit le folio", "status bar tip"));
	zoom_out    ->setStatusTip(editor->tr("Rétrécit le folio", "status bar tip"));
	zoom_content->setStatusTip(editor->tr("Adapte le zoom de façon à afficher tout le contenu du folio indépendamment du cadre"));
	zoom_fit    ->setStatusTip(editor->tr("Adapte le zoom exactement sur le cadre du folio", "status bar tip"));
	zoom_reset  ->setStatusTip(editor->tr("Restaure le zoom par défaut", "status bar tip"));

	zoom_in     ->setData("zoom_in");
	zoom_out    ->setData("zoom_out");
	zoom_content->setData("zoom_content");
	zoom_fit    ->setData("zoom_fit");
	zoom_reset  ->setData("zoom_reset");

	QObject::connect(&m_zoom_actions_group, &QActionGroup::triggered, editor, &QETDiagramEditor::zoomGroupTriggered);

		//Adding action (add text, image, shape...)
	QAction *add_text      = m_add_item_actions_group.addAction(QET::Icons::PartTextField, editor->tr("Ajouter un champ de texte"));
	QAction *add_image	   = m_add_item_actions_group.addAction(QET::Icons::adding_image,  editor->tr("Ajouter une image"));
	QAction *add_line	   = m_add_item_actions_group.addAction(QET::Icons::PartLine,      editor->tr("Ajouter une ligne", "Draw line"));
	QAction *add_rectangle = m_add_item_actions_group.addAction(QET::Icons::PartRectangle, editor->tr("Ajouter un rectangle"));
	QAction *add_ellipse   = m_add_item_actions_group.addAction(QET::Icons::PartEllipse,   editor->tr("Ajouter une ellipse"));
	QAction *add_polyline  = m_add_item_actions_group.addAction(QET::Icons::PartPolygon,   editor->tr("Ajouter une polyligne"));
	QAction *add_terminal_strip = m_add_item_actions_group.addAction(QET::Icons::TerminalStrip, editor->tr("Ajouter un plan de bornes"));

	add_text     ->setStatusTip(editor->tr("Ajoute un champ de texte sur le folio actuel"));
	add_image    ->setStatusTip(editor->tr("Ajoute une image sur le folio actuel"));
	add_line     ->setStatusTip(editor->tr("Ajoute une ligne sur le folio actuel"));
	add_rectangle->setStatusTip(editor->tr("Ajoute un rectangle sur le folio actuel"));
	add_ellipse  ->setStatusTip(editor->tr("Ajoute une ellipse sur le folio actuel"));
	add_polyline ->setStatusTip(editor->tr("Ajoute une polyligne sur le folio actuel"));
	add_terminal_strip->setStatusTip(editor->tr("Ajoute un plan de bornier sur le folio actuel"));

	add_text     ->setData(QStringLiteral("text"));
	add_image    ->setData(QStringLiteral("image"));
	add_line     ->setData(QStringLiteral("line"));
	add_rectangle->setData(QStringLiteral("rectangle"));
	add_ellipse  ->setData(QStringLiteral("ellipse"));
	add_polyline ->setData(QStringLiteral("polyline"));
	add_terminal_strip->setData(QStringLiteral("terminal_strip"));

	add_text->setCheckable(true);
	add_line->setCheckable(true);
	add_rectangle->setCheckable(true);
	add_ellipse->setCheckable(true);
	add_polyline->setCheckable(true);

	QObject::connect(&m_add_item_actions_group, &QActionGroup::triggered, editor, &QETDiagramEditor::addItemGroupTriggered);

		//Depth action
	m_depth_action_group = QET::depthActionGroup(editor);
	m_depth_action_group->setDisabled(true);

	QObject::connect(m_depth_action_group, &QActionGroup::triggered, [editor](QAction *action) {
		editor->currentDiagramView()->diagram()->changeZValue(action->data().value<QET::DepthOption>());
	});

	m_find = new QAction(editor->tr("Chercher/remplacer"), editor);
	ShortcutManager::instance().registerAction(m_find, "diagrameditor.find", editor->tr("Éditeur de schémas"), QKeySequence::Find);
	QObject::connect(m_find, &QAction::triggered, [editor]()
	{
		if (auto animator = editor->m_search_and_replace_widget.findChild<QWidgetAnimation *>("search and replace animator")) {
			animator->setHidden(!editor->m_search_and_replace_widget.isHidden());
		} else {
			editor->m_search_and_replace_widget.setHidden(!editor->m_search_and_replace_widget.isHidden());
		}
	});

	m_jump_to_element = new QAction(editor->tr("Atteindre un élément"), editor);
	m_jump_to_element->setShortcut(Qt::CTRL | Qt::Key_G);
	m_jump_to_element->setStatusTip(editor->tr("Recherche et sélectionne rapidement un élément du folio", "status bar tip"));
	QObject::connect(m_jump_to_element, &QAction::triggered, [editor]()
	{
		DiagramView *diagram_view = editor->currentDiagramView();
		if (!diagram_view || !diagram_view->diagram()) {
			return;
		}
		JumpToElementDialog dialog(diagram_view->diagram(), editor);
		dialog.exec();
	});
}
