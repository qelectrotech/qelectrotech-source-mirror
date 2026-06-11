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
		  qelectrotech --info           <project.qet> [output.json]
		  qelectrotech --check-elements <element.elmt | directory>

		PDF: one multi-page document (one diagram per page).
		PNG/SVG: one file per diagram, named <output_dir>/<NN>_<title>.<ext>.
		cables: wiring list (one row per conductor) as CSV.
		wires: list of distinct wire numbers as CSV.
		bom: bill of materials (one row per element) as CSV.
		info: structural project summary as JSON (stdout, or a file) —
		      per-page element / conductor counts and unconnected terminals.
		check-elements: validate .elmt file(s) against the element schema.
	*/
	int run(const QStringList &args);

}

#endif // CLI_EXPORT_H
