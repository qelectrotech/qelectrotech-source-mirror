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
#ifndef QET_SCRIPTING_H
#define QET_SCRIPTING_H

#include <QStringList>

class QETProject;
class DiagramView;

/**
	@brief JavaScript scripting entry points (bugtracker #162).

	A script sees a single global, `qet` (see QetScriptApi): reading the
	model, exporting, editing geometry through the real undo commands, and
	a narrow set of navigation/messaging calls. See QetScriptApi's class
	comment for the exact scope and why each group of capability stops
	where it does.
*/
namespace QetScripting {

	/**
		@brief True if @p args is a `--run <script.js> <project.qet>`
		invocation.
	*/
	bool isRunRequest(const QStringList &args);

	/**
		@brief Run the script named in @p args against the project also
		named there, headless.
		Usage: qelectrotech --run <script.js> <project.qet>
		@return process exit code: 0 on success, 1 if the project failed to
		open or the script threw, 2 on a usage error.
	*/
	int run(const QStringList &args);

	/**
		@brief Run @p scriptPath against an already-open @p project (the
		"Run Script..." GUI macro path). Errors go to stderr; there is no
		modal reporting in this first version.
		@param view the active DiagramView, so the script's zoom methods
		have something to act on; nullptr from the headless entry point,
		where they become no-ops (see QetScriptApi).
		@return true if the script ran without throwing.
	*/
	bool runOnProject(const QString &scriptPath, QETProject *project, DiagramView *view = nullptr);

}

#endif // QET_SCRIPTING_H
