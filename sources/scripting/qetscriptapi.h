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
#ifndef QET_SCRIPT_API_H
#define QET_SCRIPT_API_H

#include <QObject>
#include <QString>
#include <QStringList>

class QETProject;
class DiagramView;
class Element;

/**
	@brief The QetScriptApi class
	The object a script sees as `qet` (bugtracker #162): batch/CI work and
	human-written macros against an open project.

	@b Scope. Three groups of capability, each added at a different point in
	the discussion and each drawing its own line:

	- @b Reading the model and @b exporting: folio/element/conductor counts,
	  and the same export operations the `--export-*` CLI flags provide.
	  Every export method is a thin wrapper around CLIExport::run() -- the
	  same, already-tested code path those flags use -- built from the
	  project's own file path rather than the live instance. That keeps this
	  file free of any dependency on cli_export.cpp's internals, at the cost
	  of re-opening the project from disk per call, which means @b none of
	  them ever see edits this script made with the methods below: only
	  save() writes the live instance. A script that edits then exports
	  @b must call save() first, or the export reflects the file as it was
	  before the script ran. Verified the hard way: an early version of this
	  file had save() go through the same reopen-from-disk path as the
	  exports, so it silently wrote back the unmodified original -- an
	  addElement() call counted correctly in memory and then vanished from
	  the saved file.
	- @b Editing geometry, through the same undo commands the GUI itself
	  uses (AddGraphicsObjectCommand for placement, QPropertyUndoCommand on
	  the standard `pos` property for moves, DeleteQGraphicsItemCommand for
	  removal) -- Ctrl+Z undoes a script's edits exactly as it would the
	  equivalent manual ones, because they are, mechanically, the same
	  commands on the same stack. All four refuse on a read-only project,
		  same as their GUI equivalents check Diagram::isReadOnly() before
		  editing. addElement() also imports the element into the project's
		  own embedded collection first (QETProject::importElement()), same
		  as the drag-from-collection-panel path -- without it, the saved
		  file referenced a definition outside the project and went missing
		  on a machine without that same collection installed. One
		  consequence worth knowing, not a bug:
	  QPropertyUndoCommand merges consecutive commands on the same
	  object+property when their text() also matches
	  (QPropertyUndoCommand::mergeWith(), pre-existing), and
	  setElementPosition()/moveElement() always use the same text for a
	  given element -- so several position changes to the same element in a
	  row collapse into one undo step, the same way dragging an element
	  does, not one step per call. Verified against exactly that: two
	  consecutive calls on one element, then undo/undo/redo/redo, land
	  where a merge predicts, not where two independent steps would.
	- @b Navigating and @b messaging: select an element, zoom the active
	  view, and show the user a message. Deliberately narrow: selection and
	  messaging work with no view at all (headless `--run`); zoom is a no-op
	  returning false without one, since there is nothing to zoom.

	Explicitly @b not in scope: driving arbitrary GUI actions or dialogs. A
	script that could invoke any QAction by name could just as easily
	trigger one that opens a modal QDialog::exec() with nobody there to
	dismiss it -- exactly the hang class investigated for bugtracker #882.
	Every method here is either non-blocking by construction or, for
	messages, safe under QET::QetMessageBox's existing non-interactive mode
	(already active for headless runs). Nothing here opens a dialog the
	caller has to wait on -- including addElement(), which detects an
	import-collision case that would otherwise reach
	QETProject::importElement()'s own ImportElementDialog::exec() and
	refuses instead, rather than let a plain QDialog (not routed through
	QetMessageBox) block a script the same way.
*/
class QetScriptApi : public QObject
{
	Q_OBJECT

	public:
		/**
			@param project the project this API acts on
			@param view the active DiagramView, when run interactively via
			"Run Script..."; nullptr for the headless --run entry point.
			Only the zoom methods use it -- everything else works either way.
		*/
		explicit QetScriptApi(QETProject *project, DiagramView *view = nullptr, QObject *parent = nullptr);

		// -- read the model --
		Q_INVOKABLE QString projectTitle() const;
		Q_INVOKABLE QString filePath() const;
		Q_INVOKABLE int folioCount() const;
		Q_INVOKABLE QString folioTitle(int index) const;
		Q_INVOKABLE int elementCount(int folioIndex) const;
		Q_INVOKABLE int conductorCount(int folioIndex) const;

		// -- export / save: thin wrappers around the --export-* CLI paths --
		Q_INVOKABLE bool exportPdf(const QString &output, bool showTerminals = false);
		Q_INVOKABLE bool exportPng(const QString &outDir, bool showTerminals = false);
		Q_INVOKABLE bool exportSvg(const QString &outDir, bool showTerminals = false);
		Q_INVOKABLE bool exportCables(const QString &output);
		Q_INVOKABLE bool exportWires(const QString &output);
		Q_INVOKABLE bool exportBom(const QString &output);
		Q_INVOKABLE bool exportWiring(const QString &output);
		Q_INVOKABLE bool exportNets(const QString &output);
		Q_INVOKABLE bool exportLinks(const QString &output);
		Q_INVOKABLE bool exportInfo(const QString &output);
		Q_INVOKABLE bool setTitleBlock(const QString &output, const QStringList &assignments);
		Q_INVOKABLE bool save(const QString &output);

		// -- edit geometry, through the real undo commands --
		Q_INVOKABLE QString addElement(int folioIndex, const QString &locationPath, double x, double y);
		Q_INVOKABLE bool setElementPosition(int folioIndex, const QString &elementUuid, double x, double y);
		Q_INVOKABLE bool moveElement(int folioIndex, const QString &elementUuid, double dx, double dy);
		Q_INVOKABLE bool deleteElement(int folioIndex, const QString &elementUuid);
		Q_INVOKABLE bool undo();
		Q_INVOKABLE bool redo();
		Q_INVOKABLE bool canUndo() const;
		Q_INVOKABLE bool canRedo() const;

		// -- navigate and message --
		Q_INVOKABLE bool selectElement(const QString &elementUuid);
		Q_INVOKABLE void deselectAll(int folioIndex);
		Q_INVOKABLE bool zoomFit();
		Q_INVOKABLE bool zoomToContent();
		Q_INVOKABLE bool zoomReset();
		Q_INVOKABLE void showMessage(const QString &text);

		// -- logging: a script has no console of its own --
		Q_INVOKABLE void log(const QString &message);

	private:
		bool runFlag(const QString &flag, const QStringList &args);
		Element *findElement(int folioIndex, const QString &elementUuid) const;

		QETProject *m_project;
		DiagramView *m_view;
};

#endif // QET_SCRIPT_API_H
