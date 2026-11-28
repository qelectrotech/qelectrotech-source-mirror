/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef EPLANPROJECTIMPORT_H
#define EPLANPROJECTIMPORT_H

#include <QString>
#include <QStringList>

/**
	@brief The parts of "File > Import EPLAN project" that need no
	window: where the import program is, how it is called and what its
	output means. The program, tools/eplan_import/eplan_zw1/epdz_nach_qet.py,
	turns an EPLAN export archive (.epdz) into an editable project; it
	prints one line "PROGRESS <percent> <step>" per step and
	"done: <project>" at the end.
*/
namespace EplanProjectImport
{
	/// file name of the import program
	QString scriptName();

	/// the import program: @p chosen when it is a file, otherwise the
	/// first of the environment variable QET_EPLAN_IMPORT, the install
	/// beside @p application_dir (eplan_import/eplan_zw1/, also under
	/// ../, ../share/qelectrotech/ and ../Resources/) and a source tree the
	/// build folder lies in (tools/eplan_import/eplan_zw1/); empty when it
	/// is nowhere to be found
	QString findScript(const QString &application_dir, const QString &chosen = QString());

	/// <folder of the archive>/<name of the archive>.qet
	QString defaultTarget(const QString &archive);

	/// the command line for the program; @p unpacked is the folder the
	/// archive was already unpacked into (empty: the program unpacks it)
	QStringList arguments(const QString &script, const QString &archive,
						  const QString &target, const QString &unpacked,
						  bool pdf, bool keep_parts);

	/// the path after the last "done: " of the program's output, or empty
	QString resultFromOutput(const QString &output);

	/// the text for a progress line of the program, in the language of
	/// QElectroTech; @p percent gets the percentage. Empty when @p line is
	/// no progress line.
	QString progressText(const QString &line, int *percent);
}

#endif // EPLANPROJECTIMPORT_H
