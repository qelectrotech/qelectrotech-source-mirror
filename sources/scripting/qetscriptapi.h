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
#include <QVariantList>

class QETProject;
class DiagramView;
class Element;
class Terminal;
class Conductor;
class IndependentTextItem;
class QetShapeItem;

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
	  setElementPosition()/moveElement()/rotateElement() always use the
	  same text for a given element -- so several position changes, or
	  several rotations, of the same element in a row collapse into one
	  undo step, the same way dragging or repeatedly rotating an element
	  does, not one step per call. setElementInfo()/setElementLabel()
	  behave the same way for the same reason, through
	  ChangeElementInformationCommand::mergeWith(). Verified against exactly that: two
	  consecutive calls on one element, then undo/undo/redo/redo, land
	  where a merge predicts, not where two independent steps would.
	- @b Wiring, @b labelling and @b folios: create a conductor between two
	  terminals (ConductorCreator, the same class the GUI's
	  drag-a-rectangle-over-terminals path uses, so the result inherits an
	  existing potential's properties and joins conductor auto-numbering),
	  change an element's label or any other information key
	  (ChangeElementInformationCommand, which also tells the project
	  database what changed), add a folio (QETProject::addNewDiagram(),
	  already undoable) and set its title (ChangeTitleBlockCommand). With
	  addElement() these are what make a script able to draw rather than
	  only rearrange: before them a script could place two symbols and had
	  no way to connect them.

	  Terminals are addressed by their @b index in Element::terminals(),
	  not by uuid, and elementTerminals() prints that indexing so a script
	  can see what it is about to wire. Terminal uuids look like the
	  obvious key and are not one: Terminal::uuid() is a property of the
	  catalog .elmt definition, empty for most of the installed base and,
	  where present, identical across every instance of that element -- so
	  it does not distinguish one placed coil's A1 from another's.
	- @b Conductor properties and @b cross-references: set a conductor's
	  number, formula, colour or section, and link a master to a slave or
	  one report to another. Both follow the application's own rules rather
	  than writing the field: a conductor property is applied to every
	  conductor of the same electrical potential, which is what the GUI and
	  search-and-replace both do -- a wire number belongs to a potential,
	  not to one drawn segment -- and a link is refused unless
	  LinkElementCommand::isLinkable() allows it, which is where the
	  master/slave, PLC-pairing and report-direction rules already live.
	  linkElements() takes a folio index for each end because a master and
	  its slave are usually on different ones.

	  A conductor is addressed as "the conductor on terminal i of element
	  U", not by an identity of its own: conductors have no persisted uuid,
	  and the folio-scoped integer ids the file uses for their ends are
	  renumbered on every save, so there is nothing stable to name one by.
	  Since the change is potential-wide anyway, any terminal of the
	  potential names it equally well. A terminal carrying more than one
	  conductor is ambiguous and is refused rather than guessed at -- which
	  in practice means a potential is addressed from one of its leaf
	  terminals, not from the hub several conductors meet at.
	- @b Text and @b shapes: the drawing furniture a folio carries beside
	  its circuit -- a free-standing note, a line, a rectangle, an ellipse
	  -- added with the same AddGraphicsObjectCommand the corresponding GUI
	  tools use, and changed through the plainText/color/rotation
	  properties those items already publish.

	  These are addressed by @b index into a listing sorted by position
	  (top to bottom, then left to right), because unlike an element they
	  carry no uuid and unlike a conductor they have no terminal to be
	  named by. Position is the only identity they have, and it persists,
	  so the ordering is the same after a save and reload -- verified
	  against exactly that. What it is @b not stable against is adding or
	  deleting one: indexes after the affected position shift, the way a
	  list's do. Call texts() or shapes() again rather than holding an
	  index across an edit that adds or removes one.
	- @b Querying the project database: run a read-only SELECT against the
	  SQLite database QElectroTech builds from the project, and get rows
	  back as objects. This is not a new door. QET already ships a
	  "Requête SQL personnalisée" box in the element-query dialog where a
	  user types arbitrary SQL, and it is guarded by the same
	  projectDataBase::isReadOnlySelect() this calls through
	  projectDataBase::newQuery(). A script gets what a user already has,
	  under the same rule, and neither can write.

	  What is worth knowing is what the database @b is: a cache, rebuilt
	  from the XML on every load and never written to disk. The three
	  views -- element_nomenclature_view, project_summary_view and
	  wiring_list_view -- exist to be queried and are the surface to
	  depend on. The underlying tables are how the cache happens to be
	  arranged today, and a column may move. tables() lists both so a
	  script can see what it is querying rather than guess.
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
	QetMessageBox) block a script the same way. addConductor() declines the
	same way, for the same reason, when the two terminals belong to two
	different existing potentials and ConductorCreator would therefore ask
	which one's properties to inherit -- measured: with that check removed,
	exactly that call never returns.
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
		Q_INVOKABLE bool rotateElement(int folioIndex, const QString &elementUuid, double angle);
		Q_INVOKABLE bool deleteElement(int folioIndex, const QString &elementUuid);

		// -- address what is already there --
		Q_INVOKABLE QStringList elementUuids(int folioIndex) const;
		Q_INVOKABLE QString elementName(int folioIndex, const QString &elementUuid) const;
		Q_INVOKABLE QStringList elementTerminals(int folioIndex, const QString &elementUuid) const;

		// -- element information, through ChangeElementInformationCommand --
		Q_INVOKABLE QString elementInfo(int folioIndex, const QString &elementUuid, const QString &key) const;
		Q_INVOKABLE bool setElementInfo(int folioIndex, const QString &elementUuid, const QString &key, const QString &value);
		Q_INVOKABLE QString elementLabel(int folioIndex, const QString &elementUuid) const;
		Q_INVOKABLE bool setElementLabel(int folioIndex, const QString &elementUuid, const QString &label);

		// -- wire two terminals together --
		Q_INVOKABLE bool addConductor(int folioIndex,
									  const QString &elementUuidA, int terminalIndexA,
									  const QString &elementUuidB, int terminalIndexB);

		// -- conductor properties, applied to the whole potential --
		Q_INVOKABLE QStringList conductors(int folioIndex) const;
		Q_INVOKABLE QString conductorProperty(int folioIndex, const QString &elementUuid,
											  int terminalIndex, const QString &property) const;
		Q_INVOKABLE bool setConductorProperty(int folioIndex, const QString &elementUuid,
											  int terminalIndex, const QString &property,
											  const QString &value);

		// -- cross-references: master/slave and report links --
		Q_INVOKABLE QString elementLinkType(int folioIndex, const QString &elementUuid) const;
		Q_INVOKABLE QStringList linkedElements(int folioIndex, const QString &elementUuid) const;
		Q_INVOKABLE bool linkElements(int folioIndexA, const QString &elementUuidA,
									  int folioIndexB, const QString &elementUuidB);
		Q_INVOKABLE bool unlinkElement(int folioIndex, const QString &elementUuid);

		// -- independent text and drawing shapes --
		Q_INVOKABLE QStringList texts(int folioIndex) const;
		Q_INVOKABLE int addText(int folioIndex, const QString &text, double x, double y);
		Q_INVOKABLE bool setTextContent(int folioIndex, int textIndex, const QString &text);
		Q_INVOKABLE bool setTextColor(int folioIndex, int textIndex, const QString &color);
		Q_INVOKABLE bool setTextRotation(int folioIndex, int textIndex, double angle);
		Q_INVOKABLE bool deleteText(int folioIndex, int textIndex);

		Q_INVOKABLE QStringList shapes(int folioIndex) const;
		Q_INVOKABLE int addShape(int folioIndex, const QString &type,
								 double x1, double y1, double x2, double y2);
		Q_INVOKABLE bool deleteShape(int folioIndex, int shapeIndex);

		// -- query the project database --
		Q_INVOKABLE QStringList tables() const;
		Q_INVOKABLE QVariantList query(const QString &sql);
		Q_INVOKABLE QString queryError() const;

		// -- folios --
		Q_INVOKABLE int addFolio();
		Q_INVOKABLE bool setFolioTitle(int folioIndex, const QString &title);

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
		Terminal *findTerminal(int folioIndex, const QString &elementUuid, int terminalIndex,
							   const QString &caller);
		Conductor *findConductor(int folioIndex, const QString &elementUuid, int terminalIndex,
								 const QString &caller);
		QList<IndependentTextItem *> sortedTexts(int folioIndex) const;
		QList<QetShapeItem *> sortedShapes(int folioIndex) const;
		IndependentTextItem *findText(int folioIndex, int textIndex, const QString &caller);
		bool setInfoKey(int folioIndex, const QString &elementUuid,
						const QString &key, const QString &value, const QString &caller);

		QETProject *m_project;
		DiagramView *m_view;
		QString m_query_error;
};

#endif // QET_SCRIPT_API_H
