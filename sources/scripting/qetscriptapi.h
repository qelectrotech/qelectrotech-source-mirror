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
#include <QVariantMap>

class QETProject;
class DiagramView;
class Element;
class Terminal;
class Conductor;
class IndependentTextItem;
class QetShapeItem;
class DiagramImageItem;
class DynamicElementTextItem;
class QetGraphicsTableItem;

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
	  can see what it is about to wire. The index is the terminal's place
	  in the element's own top-to-bottom, left-to-right ordering, not the
	  order its definition file lists them. Terminal uuids look like the
	  obvious key and are not one: Terminal::uuid() is a property of the
	  catalog .elmt definition, empty for most of the installed base and,
	  where present, identical across every instance of that element -- so
	  it does not distinguish one placed coil's A1 from another's.
	- @b Conductor properties and @b cross-references: set a conductor's
	  number, formula, colour or section (and its look: style normal/
	  dashed/dashdotted, two-colour mode and second colour, dash size,
	  line width, text size, whether its number is shown -- all under the
	  names the .qet file uses for them), and link a master to a slave or
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
	  properties those items already publish. A shape's look is set with
	  setShapeProperty(): color and fill (a colour name, or "none" for no
	  fill), width, line-style (solid, dashed, dotted, dashdot) and
	  rotation, through the pen/brush/rotation properties the shape's own
	  style editor changes.

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
	- @b Terminal @b strips: create a strip, put terminal-type elements on
	  it, remove it. Strips are addressed by index into terminalStrips(),
	  which is the project's own order (unlike texts and shapes it is not
	  re-sorted) and does shift when one is removed. Only elements whose
	  link type is "terminal" can be added, the same restriction the
	  editor enforces by construction.

	  stripRealTerminals() lists the strip's real terminals -- the actual
	  wire-ends added by addTerminalToStrip(), one per index -- which
	  physical position (clamp) each currently sits on and how many
	  neighbours share it, since that is what groupTerminals() and
	  bridgeTerminals() address by index into.

	  groupTerminals() merges several real terminals onto one physical
	  position, choosing the receiving position the same way the terminal
	  strip editor's own "group" button does: the position among the ones
	  named that already carries the most real terminals, not necessarily
	  the first one given -- a script asking to group indices [0, 1] is not
	  guaranteed index 0's position is where they end up. bridgeTerminals()
	  wires several real terminals together electrically without merging
	  their positions, refused (TerminalStrip::isBridgeable()) when they
	  are not all at the same level -- the same check the editor's bridge
	  button applies, not a rule reimplemented here. sortTerminalStrip()
	  reorders the strip's physical positions into the canonical order the
	  editor's own sort button computes.
	- @b Tables: a BOM/nomenclature or a summary (table of contents) placed
	  on a folio, through QetGraphicsTableFactory::create() -- the same
	  factory call the "add table" menu action makes, minus the modal
	  AddTableDialog it collects its settings from first. That dialog is
	  still built here, off-screen and never shown or exec'd: addTable()
	  calls setTableName() and the query widget's setQuery() on it, the
	  same as a user filling in the form, and forces its two checkboxes
	  ("adjust table to folio" and "add a new folio if the table overflows"
	  it) off regardless of their .ui-file default of checked -- a script
	  calling addTable() once should create exactly the one table it asked
	  for, not possibly several spread across folios it never asked to add.
	  A script that wants either behaviour can resize the result itself or
	  add its own folio.

	  Neither creating nor deleting a table is undoable:
	  QetGraphicsTableFactory::newTable(), which create() calls, calls
	  Diagram::addItem() directly, with no undo command of its own, in the
	  stock "add table" action as much as here -- a pre-existing gap in the
	  application, not something introduced by this API. Tables are
	  addressed by index in a position-sorted listing, like texts, shapes
	  and images.
	- @b Auto-numbering: define a named numbering context of kind
	  "conductor", "element" or "folio", built from parts written
	  "type[:value[:increase]]" -- types are the ones the auto-numbering
	  dialog offers (string, unit, ten, hundred, alpha, idfolio, folio,
	  plant, locmach, elementline, elementcolumn, elementprefix, wrap,
	  unitfolio, tenfolio, hundredfolio) -- and select which one a folio's
	  new conductors use. Defining or removing a context is not undoable,
	  because the application itself does it through direct project calls
	  and only the counter advance is on the undo stack; the numbering
	  actually applied to a conductor is.

	  For elements, useElementAutoNum() selects the current context and
	  numberElement() applies it to one element, as the "add element" tool
	  does right after placing one. addElement() deliberately does not
	  number what it places: doing it silently would change what an existing
	  script produces the moment its project happens to have a context
	  selected, so it is a separate, explicit call. Folio auto-numbering is
	  not offered: in the application it spawns whole new folios from a
	  context, which is a different operation from labelling.
	- @b Duplicating: copy elements, together with the conductors that run
	  between them, to a position on the same or another folio, through
	  Diagram::toXml() and fromXml() and PasteDiagramCommand -- what Ctrl+C
	  and Ctrl+V do, so a paste behaves as a paste does there: the copies
	  come without their labels and without their conductors' wire numbers,
	  which the application clears on paste (measured: '' on both).
	  The position is the top left of the pasted group's bounding rectangle,
	  so an element's own origin ends up offset from it by its hotspot
	  (measured: +20, +30 for a coil); (0, 0) is not a position but means
	  "keep the source coordinates", as Diagram::fromXml() treats it. The
	  result lists the copies in the order the elements were named --
	  the application's own list is in scene order, and a caller pairing by
	  index would otherwise be wired to the wrong copies -- paired by
	  position, which a paste preserves, so two elements at the same point
	  cannot be told apart. A conductor is copied
	  only if both its ends are among the copied elements. The previous
	  selection is put back afterwards, since copying works by selecting.
	- @b Project title and folio frame: setProjectTitle(), and the grid that
	  frames each folio -- columns and rows, their size, and whether the
	  headers show (columns, column-width, display-columns, rows, row-height,
	  display-rows) -- through ChangeBorderCommand. These are the six fields
	  the folio properties panel offers; the title block's header sizes,
	  which it does not, are left alone. Changing the project title is not
	  undoable: the application sets it directly too.

	  A folio's title block @b template is a seventh, separate case:
	  Diagram::setTitleBlockTemplate() resolves a name only against
	  QETProject::embeddedTitleBlockTemplatesCollection() -- the same
	  copy-into-the-project step addElement() already does for elements,
	  and for the same reason (a project opened on another machine must not
	  depend on files only this one has). titleBlockTemplates() lists what
	  is embedded and what is available to embed from the common/company
	  /custom collections, each name suffixed with its source;
	  embedTitleBlockTemplate() does the copy (QDomElement in, unmodified,
	  via *TemplatesCollection::get/setTemplateXmlDescription() -- neither
	  side is scripting-specific code, both already exist for the template
	  editor to call). setFolioProperty(folio, "template", name) then
	  embeds it first if it is not already, refusing only if no collection
	  has that name at all. Embedding is not undoable, the same as defining
	  an auto-numbering context is not: the application does both through
	  direct collection/project calls with no undo command of their own.
	  A template literally named "default" reads back as folioProperty()
	  "" afterwards, not "default": BorderTitleBlock::titleBlockTemplateName()
	  treats the two as the same thing, since "no override" already renders
	  with the template named "default".
	- @b Geometry and folio order: elementGeometry() reads where an element
	  is -- x, y (its origin), rotation, and the box it occupies on the folio
	  (left, top, right, bottom) -- so a script can lay one thing out relative
	  to another instead of only setting absolute coordinates, and can check
	  that a move landed. insertFolio() puts a new folio at a position
	  instead of at the end, which is what reordering is mostly for while
	  moving an existing folio still needs the application's project view.
	- @b Images: place a picture from a file. The pixels are copied into
	  the project, which stores them inline in the .qet -- the saved file
	  does not refer to the original path, so it opens on another machine,
	  and it grows by roughly the size of the image, which is why files
	  over 10 MB are refused. Images are addressed by index in a
	  position-sorted listing, like texts and shapes -- by the on-screen
	  bounding box, so scaling or rotating an image, which turns about its
	  centre, can change where it sorts. Re-list after either.
	- @b Element @b texts: the text fields drawn on a symbol -- its label,
	  the names beside its terminals, any value the definition placed there.
	  A symbol arrives with the fields its definition gives it; setElementLabel()
	  fills the value one of them shows, and these methods control the fields
	  themselves: where each sits, its size, whether it draws a frame, what it
	  shows, and adding or deleting one. Addressed by index in the element's
	  own list, which follows the definition's order and shifts when one is
	  deleted -- and undoing a deletion puts the field back at the end, so
	  list again after either.

	  Two things called text, which differ for a field bound to an
	  information key: the @b "text" property is the field's stored string,
	  which for an information-bound field is an unused placeholder (empty,
	  or "Texte" once one has been added), and @b "shows" is what is drawn,
	  which follows the element's information straight away -- compared
	  against elementInfo() at seven points across relabel, rebinding,
	  setting and undo, with no difference. Read "shows".

	  Consecutive setElementInfo()/setElementLabel() calls on one element
	  merge into a single undo step, as ChangeElementInformationCommand
	  does, so one undo can revert several.

	  A field's @b source is "text" (a fixed string), "info" (the value of one
	  of the element's information keys, so it follows setElementInfo() and
	  setElementLabel()) or "composite" (a formula over several). Position is in
	  the element's own coordinates, not the folio's.
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
									  int folioIndexB, const QString &elementUuidB,
									  int groupIndex = -1);
		Q_INVOKABLE bool unlinkElement(int folioIndex, const QString &elementUuid);
		Q_INVOKABLE int elementLinkGroupIndex(int folioIndex, const QString &elementUuid,
											  int otherFolioIndex, const QString &otherElementUuid) const;

		// -- a PLC master's IO table: address/function/comment rows a PLC
		// slave links onto via linkElements()'s groupIndex --
		Q_INVOKABLE QStringList plcIOs(int folioIndex, const QString &elementUuid) const;
		Q_INVOKABLE int addPlcIO(int folioIndex, const QString &elementUuid, const QString &type,
								 const QString &address, const QString &functionText,
								 const QString &comment);
		Q_INVOKABLE bool setPlcIO(int folioIndex, const QString &elementUuid, int ioIndex,
								  const QString &property, const QString &value);
		Q_INVOKABLE bool removePlcIO(int folioIndex, const QString &elementUuid, int ioIndex);

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
		Q_INVOKABLE QString shapeProperty(int folioIndex, int shapeIndex, const QString &property) const;
		Q_INVOKABLE bool setShapeProperty(int folioIndex, int shapeIndex,
										  const QString &property, const QString &value);

		// -- query the project database --
		Q_INVOKABLE QStringList tables() const;
		Q_INVOKABLE QVariantList query(const QString &sql);
		Q_INVOKABLE QString queryError() const;

		// -- removing a conductor or a folio; folio properties beyond the title --
		Q_INVOKABLE bool deleteConductor(int folioIndex, const QString &elementUuid, int terminalIndex);
		Q_INVOKABLE bool removeFolio(int folioIndex);
		Q_INVOKABLE bool setFolioProperty(int folioIndex, const QString &property, const QString &value);
		Q_INVOKABLE QString folioProperty(int folioIndex, const QString &property) const;

		// -- terminal strips (borniers) --
		Q_INVOKABLE QStringList terminalStrips() const;
		Q_INVOKABLE int addTerminalStrip(const QString &installation, const QString &location,
										 const QString &name);
		Q_INVOKABLE bool removeTerminalStrip(int stripIndex);
		Q_INVOKABLE bool addTerminalToStrip(int stripIndex, int folioIndex,
											const QString &elementUuid);
		Q_INVOKABLE QStringList stripRealTerminals(int stripIndex) const;
		Q_INVOKABLE bool groupTerminals(int stripIndex, const QVariantList &realTerminalIndices);
		Q_INVOKABLE bool bridgeTerminals(int stripIndex, const QVariantList &realTerminalIndices);
		Q_INVOKABLE bool sortTerminalStrip(int stripIndex);

		// -- a BOM/nomenclature or summary table placed on a folio --
		Q_INVOKABLE QStringList tables(int folioIndex) const;
		Q_INVOKABLE int addTable(int folioIndex, const QString &kind, const QString &name,
								 const QString &query);
		Q_INVOKABLE bool deleteTable(int folioIndex, int tableIndex);
		Q_INVOKABLE bool setTablePosition(int folioIndex, int tableIndex, double x, double y);

		// -- auto-numbering contexts (conductor, element, folio) --
		Q_INVOKABLE QStringList autoNums(const QString &kind) const;
		Q_INVOKABLE bool addAutoNum(const QString &kind, const QString &name, const QStringList &parts);
		Q_INVOKABLE bool removeAutoNum(const QString &kind, const QString &name);
		Q_INVOKABLE bool useConductorAutoNum(int folioIndex, const QString &name);
		Q_INVOKABLE bool useElementAutoNum(const QString &name);
		Q_INVOKABLE bool numberElement(int folioIndex, const QString &elementUuid);

		// -- images, embedded in the project --
		Q_INVOKABLE QStringList images(int folioIndex) const;
		Q_INVOKABLE int addImage(int folioIndex, const QString &filePath, double x, double y);
		Q_INVOKABLE bool setImageScale(int folioIndex, int imageIndex, double factor);
		Q_INVOKABLE bool setImageRotation(int folioIndex, int imageIndex, double angle);
		Q_INVOKABLE bool deleteImage(int folioIndex, int imageIndex);

		// -- the text fields shown on a symbol (label, terminal names, ...) --
		Q_INVOKABLE QStringList elementTexts(int folioIndex, const QString &elementUuid) const;
		Q_INVOKABLE int addElementText(int folioIndex, const QString &elementUuid,
									   const QString &source, const QString &value,
									   double x, double y);
		Q_INVOKABLE bool setElementTextProperty(int folioIndex, const QString &elementUuid,
												int textIndex, const QString &property,
												const QString &value);
		Q_INVOKABLE QString elementTextProperty(int folioIndex, const QString &elementUuid,
												int textIndex, const QString &property) const;
		Q_INVOKABLE bool deleteElementText(int folioIndex, const QString &elementUuid, int textIndex);

		// -- copy elements (with the conductors between them) to a position --
		Q_INVOKABLE QStringList duplicateElements(int fromFolioIndex, const QStringList &elementUuids,
												  int toFolioIndex, double x, double y);

		// -- the project title, and each folio's frame (grid of columns and rows) --
		Q_INVOKABLE bool setProjectTitle(const QString &title);
		Q_INVOKABLE QString folioBorder(int folioIndex, const QString &property) const;
		Q_INVOKABLE bool setFolioBorder(int folioIndex, const QString &property, const QString &value);

		// -- title block templates: which exist, embedding one into the project --
		Q_INVOKABLE QStringList titleBlockTemplates() const;
		Q_INVOKABLE bool embedTitleBlockTemplate(const QString &name);

		// -- read an element's geometry --
		Q_INVOKABLE QVariantMap elementGeometry(int folioIndex, const QString &elementUuid) const;

		// -- folios --
		Q_INVOKABLE int addFolio();
		Q_INVOKABLE int insertFolio(int position);
		Q_INVOKABLE bool setFolioTitle(int folioIndex, const QString &title);

		Q_INVOKABLE bool undo();
		Q_INVOKABLE bool redo();
		Q_INVOKABLE bool canUndo() const;
		Q_INVOKABLE bool canRedo() const;

		// -- navigate and message --
		Q_INVOKABLE bool selectElement(const QString &elementUuid);
		Q_INVOKABLE void deselectAll(int folioIndex);
		Q_INVOKABLE QStringList selectedElements(int folioIndex) const;
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
		QList<QetGraphicsTableItem *> sortedTables(int folioIndex) const;
		QList<DiagramImageItem *> sortedImages(int folioIndex) const;
		DynamicElementTextItem *findElementText(int folioIndex, const QString &elementUuid,
												int textIndex, const QString &caller) const;
		IndependentTextItem *findText(int folioIndex, int textIndex, const QString &caller);
		bool setInfoKey(int folioIndex, const QString &elementUuid,
						const QString &key, const QString &value, const QString &caller);

		QETProject *m_project;
		DiagramView *m_view;
		QString m_query_error;
};

#endif // QET_SCRIPT_API_H
