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
#ifndef DIAGRAM_EDITOR_ACTIONS_H
#define DIAGRAM_EDITOR_ACTIONS_H

#include <QAction>
#include <QActionGroup>
#include <QList>

class QETDiagramEditor;

/**
	@brief The DiagramEditorActions class
	Owns every QAction/QActionGroup used by a QETDiagramEditor window: one
	instance is constructed per open editor window (not a singleton), so
	each editor's actions keep their own independent enabled/checked state
	tied to that window's current selection and diagram.

	Centralizing them here means there is one place to find "what actions
	does this editor have", and lets a sibling class (DiagramView) reach
	the subset of actions it needs to build its context menu through
	QETDiagramEditor::actions() instead of the editor having to expose
	those members as public just for that purpose.
*/
class DiagramEditorActions
{
	public:
		explicit DiagramEditorActions(QETDiagramEditor *editor);

		QAction
		*m_edit_diagram_properties, ///< Show a dialog to edit diagram properties
		*m_conductor_reset,         ///< Reset paths of selected conductors
		*m_cut,                     ///< Cut selection to clipboard
		*m_copy;                    ///< Copy selection to clipboard

		QActionGroup
		m_row_column_actions_group, /// Action related to add/remove rows/column in diagram
		m_selection_actions_group,  ///Action related to edit a selected item
		*m_depth_action_group = nullptr;

		QActionGroup
		*grp_visu_sel,            ///< Action group for visualisation vs edition mode
		*m_group_view_mode,       ///< Action group for project
		m_add_item_actions_group, ///Action related to adding (add text image shape...)
		m_zoom_actions_group,     ///Action related to zoom for diagram
		m_select_actions_group,   ///Action related to global selections
		m_file_actions_group;     ///Actions related to file (open, close, save...)

		QAction
		*m_tabbed_view_mode,		///< Display projects as tabs
		*m_windowed_view_mode,		///< Display projects as windows
		*m_mode_selection,		///< Set edition mode
		*m_mode_visualise,		///< Set visualisation mode
		*m_export_to_images,		///< Export diagrams of the current project as imagess
		*m_export_to_pdf = nullptr, ///< Export project to pdf.
		*m_print,			///< Print diagrams of the current project
		*m_quit_editor,			///< Quit the diagram editor
		*undo,				///< Cancel the latest action
		*redo,				///< Redo the latest cancelled operation
		*m_paste,			///< Paste clipboard content on the current diagram
		*m_auto_conductor,		///< Enable/Disable the use of auto conductor
		*m_auto_break_conductor,	///< Enable/Disable the use of auto break conductor
		*conductor_default,		///< Show a dialog to edit default conductor properties
		*m_grey_background,		///< Switch the background color in white or grey
		*m_draw_grid,			///< Switch the background grid display or not
		*m_draw_guides = nullptr,	///< Switch the custom guides display or not
		*m_project_edit_properties,	///< Edit the properties of the current project.
		*m_project_add_diagram,		///< Add a diagram to the current project.
		*m_remove_diagram_from_project,	///< Delete a diagram from the current project
		*m_clean_project,		///< Clean the content of the current project by removing useless items
		*m_project_folio_list,		///< Sommaire des schemas
		*m_csv_export,			///< generate nomenclature
		*m_add_nomenclature,		///< Add nomenclature graphics item;
		*m_add_summary,			///<Add summary graphics item
		*m_terminal_strip_dialog = nullptr, ///<Launch terminal strip dialog
		*m_project_terminalBloc,	///< generate terminal block
		*m_project_export_conductor_num,///<Export the wire num to csv
		*m_project_export_wiring_list, ///< Action to export the wiring list
		*m_terminal_numbering,         ///< Action to launch terminal numbering
		*m_export_project_db,		///Export to file the internal database of the current project
		*m_tile_window,			///< Show MDI subwindows as tile
		*m_cascade_window,		///< Show MDI subwindows as cascade
		*m_previous_window,		///< Switch to the previous document
		*m_next_window,			///< Switch to the next document
		*m_edit_selection,		///< To edit selected item
		*m_delete_selection,		///< Delete selection
		*m_rotate_selection,		///< Rotate selected elements and text items by 90 degrees
		*m_rotate_group_selection = nullptr, ///< Rotate the selection as a whole around its shared center, instead of each item in place
		*m_rotate_texts,		///< Direct selected text items to a specific angle
		*m_find_element,		///< Find the selected element in the panel
		*m_group_selected_texts = nullptr,
		*m_close_file,			///< Close current project file
		*m_save_file,			///< Save current project
		*m_save_file_as,		///< Save current project as a specific file
		*m_find = nullptr,
		*m_jump_to_element = nullptr;	///< Open the "jump to element" quick-open popup

		QList <QAction *> m_zoom_action_toolBar; ///Only zoom action must displayed in the toolbar
};

#endif // DIAGRAM_EDITOR_ACTIONS_H
