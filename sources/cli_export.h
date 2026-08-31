/*
	Copyright 2006-2025 The QElectroTech Team
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
#ifndef CLI_EXPORT_H
#define CLI_EXPORT_H

#include <QStringList>

/**
	@brief Headless command-line export.

	Implements the long-requested batch/headless export
	(qelectrotech.org bugtracker #171, GitHub #309): render a project's
	diagrams to files without opening the GUI.

	Detected and handled in main() before the GUI is created.
*/
namespace CLIExport {

	/**
		@brief True if @p args request a CLI export
		(i.e. contain one of the export options).
	*/
	bool isExportRequest(const QStringList &args);

	/**
		@brief Run the CLI export described by @p args.
		@return process exit code (0 on success).

		Usage:
		  qelectrotech --export-pdf     <project.qet> <output.pdf>
		  qelectrotech --export-png     <project.qet> <output_dir>
		  qelectrotech --export-svg     <project.qet> <output_dir>
		  qelectrotech --export-cables  <project.qet> <output.csv>
		  qelectrotech --export-wires   <project.qet> <output.csv>
		  qelectrotech --export-bom     <project.qet> <output.csv>
		  qelectrotech --export-nets    <project.qet> <output.json>
		  qelectrotech --export-links   <project.qet> <output.csv>
		  qelectrotech --info           <project.qet> [output.json]
		  qelectrotech --check-elements <element.elmt | directory>
		  qelectrotech --resave         <project.qet> <output.qet>
		  qelectrotech --set-titleblock <project.qet> <output.qet> key=value...
		  qelectrotech --test-ops       <project.qet> <ops.json> <output.qet>

		PDF: one multi-page document (one diagram per page).
		PNG/SVG: one file per diagram, named <output_dir>/<NN>_<title>.<ext>.
		cables: wiring list (one row per conductor) as CSV.
		wires: list of distinct wire numbers as CSV.
		bom: bill of materials (one row per element) as CSV.
		nets: electrical nets (connected-terminal groups) as JSON.
		links: element cross-references (coil/contact) as CSV, with
		       unresolved links flagged.
		info: structural project summary as JSON (stdout, or a file) —
		      per-page element / conductor counts and unconnected terminals.
		check-elements: validate .elmt file(s) against the element schema.
		resave: load and rewrite the project XML (round-trip integrity).
		set-titleblock: stamp title-block fields onto every folio, then save.
		      Keys: title, author, date (or date=today), plant, location,
		      revision, version, filename; any other key becomes a custom
		      field.  E.g. --set-titleblock in.qet out.qet revision=B date=today
		test-ops: headless, scripted editing for automated regression
		      testing (not an end-user feature). Applies a JSON array of
		      operations to the FIRST diagram's selection state, in the
		      same code path the GUI uses (DeleteQGraphicsItemCommand,
		      RotateSelectionCommand, QUndoStack::undo/redo), then saves.
		      Ops (each a JSON object with an "op" key):
		        {"op": "select", "uuids": ["{...}", ...]}
		            Clears the diagram's selection, then selects every
		            element whose uuid is listed. Unknown uuids are
		            reported on stderr and otherwise ignored.
		        {"op": "delete"}
		            Deletes the current selection (same command the GUI's
		            "Delete" action pushes).
		        {"op": "rotate", "angle": 90}
		            Rotates the current selection in place ("angle"
		            defaults to 90). Rotating as a single rigid group
		            (the GUI's "Pivoter le groupe") is not available
		            here yet -- it needs PR #660, not merged as of this
		            writing; an "as_group" key is rejected rather than
		            silently ignored.
		        {"op": "undo"} / {"op": "redo"}
		            One step on the diagram's QUndoStack.
		      On completion, prints a one-line JSON summary to stdout:
		      {"ops_applied": N, "element_count": N, "element_info_count": N}
		      -- the last two are row counts from the in-memory project
		      database (SELECT COUNT(*) FROM element / element_info), the
		      same table PR #664 found leaking orphan rows on delete+undo.
		      A mismatch between them is exactly that bug class,
		      independent of which specific operation caused it.
	*/
	int run(const QStringList &args);

}

#endif // CLI_EXPORT_H
