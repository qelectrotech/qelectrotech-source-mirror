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
#include "qetscriptapi.h"

#include "../ElementsCollection/elementslocation.h"
#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../cli_export.h"
#include "../diagram.h"
#include "../diagramcontent.h"
#include "../diagramview.h"
#include "../factory/elementfactory.h"
#include "../qet.h"
#include "../qetgraphicsitem/element.h"
#include "../qetmessagebox.h"
#include "../dataBase/projectdatabase.h"
#include "../qetproject.h"
#include "../qetresult.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetgraphicsitem/qetshapeitem.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetinformation.h"
#include "../titleblockproperties.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../undocommand/changeelementinformationcommand.h"
#include "../undocommand/changetitleblockcommand.h"
#include "../undocommand/deleteqgraphicsitemcommand.h"
#include "../undocommand/linkelementcommand.h"
#include "../utils/conductorcreator.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTextStream>
#include <QUndoCommand>

QetScriptApi::QetScriptApi(QETProject *project, DiagramView *view, QObject *parent) :
	QObject(parent),
	m_project(project),
	m_view(view)
{
}

QString QetScriptApi::projectTitle() const
{
	return m_project ? m_project->title() : QString();
}

QString QetScriptApi::filePath() const
{
	return m_project ? m_project->filePath() : QString();
}

int QetScriptApi::folioCount() const
{
	return m_project ? m_project->diagrams().count() : 0;
}

QString QetScriptApi::folioTitle(int index) const
{
	if (!m_project) return QString();
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (index < 0 || index >= diagrams.count()) return QString();
	return diagrams.at(index)->title();
}

int QetScriptApi::elementCount(int folioIndex) const
{
	if (!m_project) return 0;
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return 0;
	DiagramContent content(diagrams.at(folioIndex), false);
	return content.m_elements.count();
}

int QetScriptApi::conductorCount(int folioIndex) const
{
	if (!m_project) return 0;
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return 0;
	DiagramContent content(diagrams.at(folioIndex), false);
	return content.conductors(DiagramContent::AnyConductor).count();
}

/**
	@brief QetScriptApi::runFlag
	Build a CLIExport::run() argument list from this project's own file
	path plus @p args, and run it. Reopens the project from disk -- see the
	class comment for why that trade-off was made.
	@param flag one of the --export-* / --resave / --set-titleblock flags
	@param args the flag's own positional arguments (output path, etc.)
	@return true if CLIExport::run() returned 0 (success)
*/
bool QetScriptApi::runFlag(const QString &flag, const QStringList &args)
{
	if (!m_project) {
		log(QStringLiteral("qet.%1: no project").arg(flag));
		return false;
	}
	const QString path = m_project->filePath();
	if (path.isEmpty()) {
		log(QStringLiteral("qet.%1: project has no file path -- save it first").arg(flag));
		return false;
	}
	QStringList full_args;
	full_args << flag << path << args;

	if (m_view)
	{
		// Interactive "Run Script...": CLIExport::run() opens a second,
		// temporary QETProject on the same file the GUI already has open.
		// Backups are only disabled on the headless --run path (main.cpp);
		// here that second, short-lived project would otherwise manage its
		// own KAutoSaveFile for the same path as the user's real, already
		// open project, racing it and risking a stale-restore prompt next
		// launch. Disable backups for just this one export and restore
		// them right after -- the headless path must never see this
		// change, since it depends on backups staying off for the whole
		// run (see the crash note next to the other setBackupEnabled(false)
		// call in main.cpp).
		QETProject::setBackupEnabled(false);
		const int result = CLIExport::run(full_args);
		QETProject::setBackupEnabled(true);
		return result == 0;
	}

	return CLIExport::run(full_args) == 0;
}

bool QetScriptApi::exportPdf(const QString &output, bool showTerminals)
{
	QStringList args{output};
	if (showTerminals) args << QStringLiteral("--show-terminals");
	return runFlag(QStringLiteral("--export-pdf"), args);
}

bool QetScriptApi::exportPng(const QString &outDir, bool showTerminals)
{
	QStringList args{outDir};
	if (showTerminals) args << QStringLiteral("--show-terminals");
	return runFlag(QStringLiteral("--export-png"), args);
}

bool QetScriptApi::exportSvg(const QString &outDir, bool showTerminals)
{
	QStringList args{outDir};
	if (showTerminals) args << QStringLiteral("--show-terminals");
	return runFlag(QStringLiteral("--export-svg"), args);
}

bool QetScriptApi::exportCables(const QString &output)
{
	return runFlag(QStringLiteral("--export-cables"), {output});
}

bool QetScriptApi::exportWires(const QString &output)
{
	return runFlag(QStringLiteral("--export-wires"), {output});
}

bool QetScriptApi::exportBom(const QString &output)
{
	return runFlag(QStringLiteral("--export-bom"), {output});
}

bool QetScriptApi::exportWiring(const QString &output)
{
	return runFlag(QStringLiteral("--export-wiring"), {output});
}

bool QetScriptApi::exportNets(const QString &output)
{
	return runFlag(QStringLiteral("--export-nets"), {output});
}

bool QetScriptApi::exportLinks(const QString &output)
{
	return runFlag(QStringLiteral("--export-links"), {output});
}

bool QetScriptApi::exportInfo(const QString &output)
{
	return runFlag(QStringLiteral("--info"), output.isEmpty() ? QStringList{} : QStringList{output});
}

bool QetScriptApi::setTitleBlock(const QString &output, const QStringList &assignments)
{
	QStringList args{output};
	args << assignments;
	return runFlag(QStringLiteral("--set-titleblock"), args);
}

/**
	@brief QetScriptApi::save
	Write this project's own current in-memory state -- unlike every export
	method above, this does NOT go through runFlag()/CLIExport::run(): that
	reopens the project fresh from its file on disk, which would never see
	any addElement()/setElementPosition()/moveElement()/deleteElement() this
	script made, only silently rewrite the file exactly as it already was.
	Caught by testing this against a script that added an element and
	called save(): the element counted correctly in memory but the saved
	file didn't have it, since the old implementation opened an unrelated,
	unmodified second copy of the project to write.

	With no output path, this goes through QETProject::write() -- the same
	path "Enregistrer" uses -- so it honours read-only mode the same way,
	updates saveddate/savedtime, and clears the modified flag. A plain
	QFile write used to skip all of that and could tear the file on an
	interruption, where write() goes through QET::writeXmlFile()'s
	QSaveFile. With an explicit output path this is a save to a different
	file, so it goes through QET::writeXmlFile() directly without touching
	the project's own filePath() or read-only state, the same way "Save As"
	targeting a writable location is allowed even for a project opened
	read-only.
	@param output path to write to; the project's own file path if empty
	@return false if there is no output path to use, or the write failed
*/
bool QetScriptApi::save(const QString &output)
{
	if (!m_project) return false;

	if (output.isEmpty())
	{
		if (m_project->filePath().isEmpty()) {
			log(QStringLiteral("qet.save: no output path given and the project has none of its own -- pass one"));
			return false;
		}
		const QETResult result = m_project->write();
		if (!result.isOk()) {
			log(QStringLiteral("qet.save: %1").arg(result.errorMessage()));
			return false;
		}
		return true;
	}

	QDomDocument xml_doc(m_project->toXml());
	QString error_message;
	if (!QET::writeXmlFile(xml_doc, output, &error_message)) {
		log(QStringLiteral("qet.save: %1").arg(error_message));
		return false;
	}
	return true;
}

void QetScriptApi::log(const QString &message)
{
	QTextStream(stderr) << message << "\n";
}

/**
	@brief QetScriptApi::findElement
	@param folioIndex
	@param elementUuid as returned by addElement(), or read from the
	element's own uuid attribute
	@return the matching element on that folio, or nullptr
*/
Element *QetScriptApi::findElement(int folioIndex, const QString &elementUuid) const
{
	if (!m_project) return nullptr;
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return nullptr;
	DiagramContent content(diagrams.at(folioIndex), false);
	for (Element *elmt : std::as_const(content.m_elements)) {
		if (elmt->uuid().toString() == elementUuid)
			return elmt;
	}
	return nullptr;
}

Terminal *QetScriptApi::findTerminal(int folioIndex, const QString &elementUuid,
									 int terminalIndex, const QString &caller)
{
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) {
		log(QStringLiteral("qet.%1: no element %2 on folio %3").arg(caller, elementUuid).arg(folioIndex));
		return nullptr;
	}
	const QList<Terminal *> terminals = element->terminals();
	if (terminalIndex < 0 || terminalIndex >= terminals.count()) {
		log(QStringLiteral("qet.%1: %2 has %3 terminal(s), no index %4")
			.arg(caller, element->name()).arg(terminals.count()).arg(terminalIndex));
		return nullptr;
	}
	return terminals.at(terminalIndex);
}

/**
	@brief QetScriptApi::findConductor
	The single conductor attached to a terminal, or nullptr.

	Conductors carry no persisted uuid, and the terminal1/terminal2 ids the
	file uses for their ends are folio-scoped integers QElectroTech
	renumbers on every save, so a conductor has no name that survives a
	save/load cycle. Naming one by a terminal it is attached to does, and
	it reads the way the question is usually asked ("the wire on A1 of
	KM1"). A terminal with several conductors on it does not name one, so
	refuse rather than silently take the first.
*/
Conductor *QetScriptApi::findConductor(int folioIndex, const QString &elementUuid,
									   int terminalIndex, const QString &caller)
{
	Terminal *terminal = findTerminal(folioIndex, elementUuid, terminalIndex, caller);
	if (!terminal) return nullptr;
	const QList<Conductor *> conductors = terminal->conductors();
	if (conductors.isEmpty()) {
		log(QStringLiteral("qet.%1: terminal %2 of %3 has no conductor on it")
			.arg(caller).arg(terminalIndex).arg(elementUuid));
		return nullptr;
	}
	if (conductors.count() > 1) {
		log(QStringLiteral("qet.%1: terminal %2 of %3 carries %4 conductors, so it does "
						   "not name one -- use a terminal with a single conductor")
			.arg(caller).arg(terminalIndex).arg(elementUuid).arg(conductors.count()));
		return nullptr;
	}
	return conductors.first();
}

namespace {

/**
	Read or write one named conductor property. The names are the ones the
	project file uses for the same fields (ConductorProperties::toXml), so
	that what a script sets is what a reader of the .qet sees, rather than
	a third spelling invented here.
*/
QString conductorPropertyValue(const ConductorProperties &p, const QString &name)
{
	if (name == QLatin1String("num"))               return p.text;
	if (name == QLatin1String("formula"))           return p.m_formula;
	if (name == QLatin1String("function"))          return p.m_function;
	if (name == QLatin1String("bus"))               return p.m_bus;
	if (name == QLatin1String("cable"))             return p.m_cable;
	if (name == QLatin1String("tension_protocol"))  return p.m_tension_protocol;
	if (name == QLatin1String("conductor_color"))   return p.m_wire_color;
	if (name == QLatin1String("conductor_section")) return p.m_wire_section;
	if (name == QLatin1String("color"))             return p.color.name();
	if (name == QLatin1String("text_color"))        return p.text_color.name();
	return QString();
}

bool setConductorPropertyValue(ConductorProperties &p, const QString &name, const QString &value)
{
	if (name == QLatin1String("num"))               { p.text = value; return true; }
	if (name == QLatin1String("formula"))           { p.m_formula = value; return true; }
	if (name == QLatin1String("function"))          { p.m_function = value; return true; }
	if (name == QLatin1String("bus"))               { p.m_bus = value; return true; }
	if (name == QLatin1String("cable"))             { p.m_cable = value; return true; }
	if (name == QLatin1String("tension_protocol"))  { p.m_tension_protocol = value; return true; }
	if (name == QLatin1String("conductor_color"))   { p.m_wire_color = value; return true; }
	if (name == QLatin1String("conductor_section")) { p.m_wire_section = value; return true; }
	// The two real colours are QColor, not free text: an unparseable name
	// would otherwise be stored as an invalid colour and drawn as black.
	if (name == QLatin1String("color") || name == QLatin1String("text_color"))
	{
		const QColor c(value);
		if (!c.isValid()) return false;
		if (name == QLatin1String("color")) p.color = c; else p.text_color = c;
		return true;
	}
	return false;
}

const QStringList &conductorPropertyNames()
{
	static const QStringList names {
		QStringLiteral("num"), QStringLiteral("formula"), QStringLiteral("function"),
		QStringLiteral("bus"), QStringLiteral("cable"), QStringLiteral("tension_protocol"),
		QStringLiteral("conductor_color"), QStringLiteral("conductor_section"),
		QStringLiteral("color"), QStringLiteral("text_color")};
	return names;
}

} // namespace

bool QetScriptApi::setInfoKey(int folioIndex, const QString &elementUuid,
							  const QString &key, const QString &value, const QString &caller)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return false;
	}
	if (key.isEmpty()) {
		log(QStringLiteral("qet.%1: empty information key").arg(caller));
		return false;
	}
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return false;

	const DiagramContext old_info = element->elementInformations();
	if (old_info.value(key).toString() == value) return true; // nothing to push
	DiagramContext new_info = old_info;
	new_info.addValue(key, value);

	auto *cmd = new ChangeElementInformationCommand(element, old_info, new_info);
	m_project->undoStack()->push(cmd);
	return true;
}

/**
	@brief QetScriptApi::addElement
	Place a new element on a folio, through the same AddGraphicsObjectCommand
	the interactive drag-from-collection-panel path uses (see
	DiagramEventAddElement::addElement()) -- so Ctrl+Z undoes it exactly as
	it would a manually dropped element.

	Like that path, the element is first imported into the project's own
	embedded collection (QETProject::importElement()): building straight
	from a common://custom:// location without embedding it left the saved
	.qet referencing a definition outside the project, missing on any
	machine that doesn't have that same collection installed.

	importElement() can, on a name collision with a different, already
	embedded element, pop a modal dialog asking the user to choose -- with
	nobody there to answer it in a script, headless or interactive, that is
	exactly the hang class this whole API is built to avoid (see the class
	comment). Detected and refused before it can happen, rather than risked.

	@param folioIndex
	@param locationPath an element collection path, e.g.
	"embed://some/path.elmt" or "common://10_electric/...elmt"
	@param x @param y target position, in the diagram's own coordinates
	@return the new element's uuid (empty string on failure -- read-only
	project, bad folio index, an import collision, or the location could
	not be resolved/built)
*/
QString QetScriptApi::addElement(int folioIndex, const QString &locationPath, double x, double y)
{
	if (!m_project) return QString();
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.addElement: project is read-only"));
		return QString();
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return QString();
	Diagram *diagram = diagrams.at(folioIndex);

	// ElementsLocation::setPath() forces ANY path to embed:// as soon as a
	// non-null project is given, common://custom:// included -- passing
	// m_project unconditionally here (as the pre-fix code did) silently
	// turned every non-embed:: locationPath into a lookup for an embedded
	// element that was never there. Caught by actually running this
	// against a common:// path: "does not resolve to an element" even
	// though the file plainly exists.
	ElementsLocation location = locationPath.startsWith(QStringLiteral("embed://"))
			? ElementsLocation(locationPath, m_project)
			: ElementsLocation(locationPath);
	if (!location.isElement() || !location.exist()) {
		log(QStringLiteral("qet.addElement: '%1' does not resolve to an element").arg(locationPath));
		return QString();
	}

	ElementsLocation import_location = location;
	if (!(location.isProject() && location.project() == m_project))
	{
		const QString import_path = location.isFileSystem()
				? QStringLiteral("import/") + location.collectionPath(false)
				: location.collectionPath(false);
		const ElementsLocation existing(import_path, m_project);
		if (existing.exist() && existing.uuid() != location.uuid()) {
			log(QStringLiteral("qet.addElement: '%1' would collide with a different element "
								"already embedded under the same name -- refusing rather than "
								"risk the interactive import-conflict dialog").arg(locationPath));
			return QString();
		}

		import_location = m_project->importElement(location);
		if (!import_location.exist()) {
			log(QStringLiteral("qet.addElement: could not import '%1' into the project").arg(locationPath));
			return QString();
		}
	}

	int state = 0;
	Element *element = ElementFactory::Instance()->createElement(import_location, nullptr, &state);
	if (state) {
		delete element;
		log(QStringLiteral("qet.addElement: could not build element from '%1'").arg(locationPath));
		return QString();
	}

	const QPointF pos(x, y);
	element->setPos(pos);
	diagram->addItem(element);

	auto *undo_group = new QUndoCommand(QObject::tr("Ajouter %1").arg(element->name()));
	new AddGraphicsObjectCommand(element, diagram, pos, undo_group);
	diagram->undoStack().push(undo_group);

	return element->uuid().toString();
}

bool QetScriptApi::setElementPosition(int folioIndex, const QString &elementUuid, double x, double y)
{
	if (m_project && m_project->isReadOnly()) {
		log(QStringLiteral("qet.setElementPosition: project is read-only"));
		return false;
	}
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return false;

	const QVariant old_value = element->pos();
	const QVariant new_value = QPointF(x, y);
	if (old_value == new_value) return true; // already there; nothing to push

	auto *cmd = new QPropertyUndoCommand(element, "pos", old_value, new_value);
	cmd->setText(QObject::tr("Déplacer %1").arg(element->name()));
	m_project->undoStack()->push(cmd);
	return true;
}

bool QetScriptApi::moveElement(int folioIndex, const QString &elementUuid, double dx, double dy)
{
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return false;
	const QPointF p = element->pos();
	return setElementPosition(folioIndex, elementUuid, p.x() + dx, p.y() + dy);
}

/**
	@brief QetScriptApi::deleteElement
	Through DeleteQGraphicsItemCommand -- the same command the Delete key
	uses -- so any conductors attached to the element's terminals are
	cleaned up the same way, not left dangling.
*/
bool QetScriptApi::deleteElement(int folioIndex, const QString &elementUuid)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.deleteElement: project is read-only"));
		return false;
	}
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return false;
	Diagram *diagram = m_project->diagrams().at(folioIndex);

	DiagramContent content;
	content.m_elements << element;
	if (DeleteQGraphicsItemCommand::hasNonDeletableTerminal(content)) {
		log(QStringLiteral("qet.deleteElement: %1 has a non-deletable terminal (linked master/slave?), refusing").arg(elementUuid));
		return false;
	}

	auto *cmd = new DeleteQGraphicsItemCommand(diagram, content);
	diagram->undoStack().push(cmd);
	return true;
}

bool QetScriptApi::rotateElement(int folioIndex, const QString &elementUuid, double angle)
{
	if (m_project && m_project->isReadOnly()) {
		log(QStringLiteral("qet.rotateElement: project is read-only"));
		return false;
	}
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return false;

	// The same property command RotateSelectionCommand pushes for an
	// Element -- deliberately not RotateSelectionCommand itself, which
	// works on diagram->selectedItems() and would mean quietly rewriting
	// the user's selection to rotate one element by uuid. For a single
	// element the two are mechanically identical: that class special-cases
	// Element::Type to exactly this one command, and only adds a second,
	// positional one when rotating a multi-item selection as a group.
	auto *cmd = new QPropertyUndoCommand(element, "rotation",
										 QVariant(element->rotation()),
										 QVariant(element->rotation() + angle));
	cmd->setText(QObject::tr("Pivoter %1").arg(element->name()));
	m_project->undoStack()->push(cmd);
	return true;
}

QStringList QetScriptApi::elementUuids(int folioIndex) const
{
	QStringList uuids;
	if (!m_project) return uuids;
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return uuids;
	DiagramContent content(diagrams.at(folioIndex), false);
	for (Element *elmt : std::as_const(content.m_elements)) {
		uuids << elmt->uuid().toString();
	}
	return uuids;
}

QString QetScriptApi::elementName(int folioIndex, const QString &elementUuid) const
{
	Element *element = findElement(folioIndex, elementUuid);
	return element ? element->name() : QString();
}

/**
	@brief QetScriptApi::elementTerminals
	The element's terminals, in the order addConductor() indexes them: one
	entry per terminal, "<index>: <name> (<n> conductor(s))". Descriptive
	rather than structured because its only job is to let a script -- or a
	human reading a script's output -- see which index is which before
	wiring anything to it.

	Indexes, not uuids, because a terminal uuid does not address a terminal
	on a folio. Terminal::uuid() comes from the catalog .elmt definition
	(see Terminal::stableUuid()), so it is empty for most of the installed
	base, and where it is not, every instance of that same element carries
	the same one -- two coils of one type placed side by side have
	byte-identical terminal uuids, which is plainly visible in the saved
	file of any project written through this API. The order of
	Element::terminals() also comes from the definition, but it is at least
	unambiguous within the element the caller has already named by uuid.
*/
QStringList QetScriptApi::elementTerminals(int folioIndex, const QString &elementUuid) const
{
	QStringList list;
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return list;
	const QList<Terminal *> terminals = element->terminals();
	for (int i = 0 ; i < terminals.count() ; ++i)
	{
		Terminal *t = terminals.at(i);
		list << QStringLiteral("%1: %2 (%3 conductor(s))")
				.arg(i)
				.arg(t->name().isEmpty() ? QStringLiteral("-") : t->name())
				.arg(t->conductorsCount());
	}
	return list;
}

QString QetScriptApi::elementInfo(int folioIndex, const QString &elementUuid, const QString &key) const
{
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return QString();
	return element->elementInformations().value(key).toString();
}

bool QetScriptApi::setElementInfo(int folioIndex, const QString &elementUuid,
								  const QString &key, const QString &value)
{
	return setInfoKey(folioIndex, elementUuid, key, value, QStringLiteral("setElementInfo"));
}

QString QetScriptApi::elementLabel(int folioIndex, const QString &elementUuid) const
{
	return elementInfo(folioIndex, elementUuid, QETInformation::ELMT_LABEL);
}

bool QetScriptApi::setElementLabel(int folioIndex, const QString &elementUuid, const QString &label)
{
	return setInfoKey(folioIndex, elementUuid, QETInformation::ELMT_LABEL, label,
					  QStringLiteral("setElementLabel"));
}

/**
	@brief QetScriptApi::addConductor
	Wire terminal terminalIndexA of one element to terminalIndexB of
	another, on the same folio, through ConductorCreator -- the same class
	the "draw a selection rectangle over terminals" GUI path uses. Going
	through it rather than constructing a Conductor directly is what makes
	the new conductor inherit an existing potential's properties and take
	part in conductor auto-numbering; a hand-built one would be silently
	outside both.

	Refuses, rather than creating anything, when the two terminals sit on
	two different existing potentials: ConductorCreator then has to ask
	which one's properties the new conductor should inherit, and it asks
	with a plain modal QDialog that QET::QetMessageBox's non-interactive
	mode does not cover -- so under headless --run there would be nobody to
	answer it and the script would hang forever. Same reasoning, and the
	same choice, as addElement() makes about the import-conflict dialog.
	@return true if a conductor was created
*/
bool QetScriptApi::addConductor(int folioIndex,
								const QString &elementUuidA, int terminalIndexA,
								const QString &elementUuidB, int terminalIndexB)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.addConductor: project is read-only"));
		return false;
	}
	const QString caller = QStringLiteral("addConductor");
	Terminal *t1 = findTerminal(folioIndex, elementUuidA, terminalIndexA, caller);
	Terminal *t2 = findTerminal(folioIndex, elementUuidB, terminalIndexB, caller);
	if (!t1 || !t2) return false;

	if (t1 == t2) {
		log(QStringLiteral("qet.addConductor: both ends are the same terminal"));
		return false;
	}
	if (t1->isLinkedTo(t2)) {
		log(QStringLiteral("qet.addConductor: those two terminals are already wired together"));
		return false;
	}
	if (!t1->canBeLinkedTo(t2)) {
		log(QStringLiteral("qet.addConductor: those two terminals cannot be linked"));
		return false;
	}

	const QList<Terminal *> terminals {t1, t2};
	if (ConductorCreator::needsPotentialChoice(terminals)) {
		log(QStringLiteral("qet.addConductor: those terminals are on two different existing "
						   "potentials, so creating a conductor would ask which one to inherit "
						   "-- refusing rather than open a dialog no script can answer"));
		return false;
	}

	Diagram *diagram = m_project->diagrams().at(folioIndex);
	ConductorCreator creator(diagram, terminals);
	Q_UNUSED(creator)

	// ConductorCreator has no return value and several ways to decline
	// quietly, so report what actually happened rather than that it ran.
	return t1->isLinkedTo(t2);
}

/**
	@brief QetScriptApi::conductors
	One line per conductor on the folio: which terminals it joins and its
	number, in the form setConductorProperty() addresses them. Descriptive
	rather than structured for the same reason elementTerminals() is -- it
	exists so a script, or a person reading its output, can see what is
	there before changing it.
*/
QStringList QetScriptApi::conductors(int folioIndex) const
{
	QStringList list;
	if (!m_project) return list;
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return list;

	auto describe = [](Terminal *t) -> QString {
		if (!t || !t->parentElement()) return QStringLiteral("?");
		return QStringLiteral("%1 terminal %2")
				.arg(t->parentElement()->uuid().toString())
				.arg(t->parentElement()->terminals().indexOf(t));
	};

	DiagramContent content(diagrams.at(folioIndex), false);
	const QList<Conductor *> all = content.conductors(DiagramContent::AnyConductor);
	for (Conductor *c : all)
	{
		list << QStringLiteral("%1 -- %2 : num='%3'")
				.arg(describe(c->terminal1), describe(c->terminal2), c->properties().text);
	}
	return list;
}

QString QetScriptApi::conductorProperty(int folioIndex, const QString &elementUuid,
										int terminalIndex, const QString &property) const
{
	// const_cast: findConductor logs, and log() writes to stderr, which is
	// not a const operation on this object. The lookup itself changes
	// nothing.
	auto *self = const_cast<QetScriptApi *>(this);
	Conductor *conductor = self->findConductor(folioIndex, elementUuid, terminalIndex,
											   QStringLiteral("conductorProperty"));
	if (!conductor) return QString();
	return conductorPropertyValue(conductor->properties(), property);
}

/**
	@brief QetScriptApi::setConductorProperty
	Set one property on the conductor attached to a terminal -- and on
	every other conductor of the same electrical potential.

	That is not a convenience, it is the rule the application already
	follows: SearchAndReplaceWorker does exactly this, pushing one
	QPropertyUndoCommand per conductor of relatedPotentialConductors()
	inside a single macro, because a wire number, colour or section
	describes a potential and not one drawn segment. Setting it on one
	conductor and leaving the rest of the potential disagreeing would
	produce a file no GUI action could have produced.
	@return true if anything was changed, or if it already held that value
*/
bool QetScriptApi::setConductorProperty(int folioIndex, const QString &elementUuid,
										int terminalIndex, const QString &property,
										const QString &value)
{
	if (!m_project) return false;
	const QString caller = QStringLiteral("setConductorProperty");
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return false;
	}
	if (!conductorPropertyNames().contains(property)) {
		log(QStringLiteral("qet.%1: unknown property '%2'; expected one of %3")
			.arg(caller, property, conductorPropertyNames().join(QStringLiteral(", "))));
		return false;
	}
	Conductor *conductor = findConductor(folioIndex, elementUuid, terminalIndex, caller);
	if (!conductor) return false;

	ConductorProperties properties = conductor->properties();
	if (!setConductorPropertyValue(properties, property, value)) {
		log(QStringLiteral("qet.%1: '%2' is not a valid value for %3")
			.arg(caller, value, property));
		return false;
	}
	if (properties == conductor->properties()) return true; // already so

	QSet<Conductor *> potential = conductor->relatedPotentialConductors(true);
	potential << conductor;

	m_project->undoStack()->beginMacro(QObject::tr("Modifier les propriétés du conducteur"));
	for (Conductor *c : std::as_const(potential))
	{
		QVariant old_value, new_value;
		old_value.setValue(c->properties());
		new_value.setValue(properties);
		m_project->undoStack()->push(new QPropertyUndoCommand(c, "properties", old_value, new_value));
	}
	m_project->undoStack()->endMacro();
	return true;
}

QString QetScriptApi::elementLinkType(int folioIndex, const QString &elementUuid) const
{
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return QString();
	switch (element->linkType())
	{
		case Element::Simple:         return QStringLiteral("simple");
		case Element::NextReport:     return QStringLiteral("next_report");
		case Element::PreviousReport: return QStringLiteral("previous_report");
		case Element::Master:         return QStringLiteral("master");
		case Element::Slave:          return QStringLiteral("slave");
		case Element::Terminale:      return QStringLiteral("terminal");
		default:                      return QStringLiteral("unknown");
	}
}

QStringList QetScriptApi::linkedElements(int folioIndex, const QString &elementUuid) const
{
	QStringList list;
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return list;
	const QList<Element *> linked = element->linkedElements();
	for (Element *e : linked) {
		list << e->uuid().toString();
	}
	return list;
}

/**
	@brief QetScriptApi::linkElements
	Link two elements -- a master to a slave, or one report to its
	counterpart. Two folio indices because a master and its slave normally
	sit on different folios; that is the usual case, not the exception.

	Whether a given pair may be linked is not decided here.
	LinkElementCommand::isLinkable() already holds those rules -- that a
	master takes a slave and not another master, that a PLC master pairs
	only with a PLC slave, that a next-report pairs only with a
	previous-report, and that the target is free -- and asking it rather
	than re-deriving them is what keeps a script from producing a link the
	GUI would refuse to make.
*/
bool QetScriptApi::linkElements(int folioIndexA, const QString &elementUuidA,
								int folioIndexB, const QString &elementUuidB)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.linkElements: project is read-only"));
		return false;
	}
	Element *a = findElement(folioIndexA, elementUuidA);
	Element *b = findElement(folioIndexB, elementUuidB);
	if (!a || !b) {
		log(QStringLiteral("qet.linkElements: %1 does not resolve to an element")
			.arg(a ? elementUuidB : elementUuidA));
		return false;
	}
	if (a == b) {
		log(QStringLiteral("qet.linkElements: an element cannot be linked to itself"));
		return false;
	}
	if (!LinkElementCommand::isLinkable(a, b)) {
		log(QStringLiteral("qet.linkElements: %1 (%2) cannot be linked to %3 (%4) -- "
						   "check the two link types, and that the target is still free")
			.arg(elementUuidA, elementLinkType(folioIndexA, elementUuidA),
				 elementUuidB, elementLinkType(folioIndexB, elementUuidB)));
		return false;
	}

	auto *cmd = new LinkElementCommand(a);
	cmd->setLink(b);
	m_project->undoStack()->push(cmd);
	return a->linkedElements().contains(b);
}

bool QetScriptApi::unlinkElement(int folioIndex, const QString &elementUuid)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.unlinkElement: project is read-only"));
		return false;
	}
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return false;
	if (element->linkedElements().isEmpty()) return true; // nothing to undo

	auto *cmd = new LinkElementCommand(element);
	cmd->unlinkAll();
	m_project->undoStack()->push(cmd);
	return element->linkedElements().isEmpty();
}

namespace {

/**
	Reading order for items that have no identity but their position:
	top to bottom, then left to right.

	On sceneBoundingRect(), not pos(): a QetShapeItem keeps its geometry in
	its line/rect/polygon, and its pos() stays at the origin, so three
	shapes drawn in different places all sort as (0, 0) and the ordering
	collapses -- which is exactly what the first version of this did, and
	it made every shape index refer to whichever one the set happened to
	yield first. The scene bounding rect reflects where the item actually
	is for both kinds.
*/
template <typename T>
QList<T *> sortedByPosition(const QSet<T *> &items)
{
	QList<T *> list(items.cbegin(), items.cend());
	std::sort(list.begin(), list.end(), [](T *a, T *b) {
		const QPointF pa = a->sceneBoundingRect().topLeft();
		const QPointF pb = b->sceneBoundingRect().topLeft();
		if (pa.y() != pb.y()) return pa.y() < pb.y();
		if (pa.x() != pb.x()) return pa.x() < pb.x();
		// Two items genuinely at the same point still need a total order,
		// or std::sort's result depends on the set's iteration order.
		return a < b;
	});
	return list;
}

} // namespace

QList<IndependentTextItem *> QetScriptApi::sortedTexts(int folioIndex) const
{
	if (!m_project) return {};
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return {};
	DiagramContent content(diagrams.at(folioIndex), false);
	return sortedByPosition(content.m_text_fields);
}

QList<QetShapeItem *> QetScriptApi::sortedShapes(int folioIndex) const
{
	if (!m_project) return {};
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return {};
	DiagramContent content(diagrams.at(folioIndex), false);
	return sortedByPosition(content.m_shapes);
}

IndependentTextItem *QetScriptApi::findText(int folioIndex, int textIndex, const QString &caller)
{
	const QList<IndependentTextItem *> list = sortedTexts(folioIndex);
	if (textIndex < 0 || textIndex >= list.count()) {
		log(QStringLiteral("qet.%1: folio %2 has %3 independent text(s), no index %4")
			.arg(caller).arg(folioIndex).arg(list.count()).arg(textIndex));
		return nullptr;
	}
	return list.at(textIndex);
}

QStringList QetScriptApi::texts(int folioIndex) const
{
	QStringList out;
	const QList<IndependentTextItem *> list = sortedTexts(folioIndex);
	for (int i = 0 ; i < list.count() ; ++i)
	{
		IndependentTextItem *t = list.at(i);
		const QPointF at = t->sceneBoundingRect().topLeft();
		out << QStringLiteral("%1: '%2' at (%3, %4)")
				.arg(i)
				.arg(t->toPlainText())
				.arg(at.x())
				.arg(at.y());
	}
	return out;
}

/**
	@brief QetScriptApi::addText
	Place a free-standing text, as the "add text" tool does.
	@return its index in texts(), or -1
*/
int QetScriptApi::addText(int folioIndex, const QString &text, double x, double y)
{
	if (!m_project) return -1;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.addText: project is read-only"));
		return -1;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return -1;
	Diagram *diagram = diagrams.at(folioIndex);

	auto *item = new IndependentTextItem();
	item->setPlainText(text);
	diagram->undoStack().push(new AddGraphicsObjectCommand(item, diagram, QPointF(x, y)));
	return sortedTexts(folioIndex).indexOf(item);
}

bool QetScriptApi::setTextContent(int folioIndex, int textIndex, const QString &text)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.setTextContent: project is read-only"));
		return false;
	}
	IndependentTextItem *item = findText(folioIndex, textIndex, QStringLiteral("setTextContent"));
	if (!item) return false;
	if (item->toPlainText() == text) return true;

	auto *cmd = new QPropertyUndoCommand(item, "plainText",
										 QVariant(item->toPlainText()), QVariant(text));
	cmd->setText(QObject::tr("Modifier un texte"));
	m_project->undoStack()->push(cmd);
	return true;
}

bool QetScriptApi::setTextColor(int folioIndex, int textIndex, const QString &color)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.setTextColor: project is read-only"));
		return false;
	}
	const QColor new_color(color);
	if (!new_color.isValid()) {
		log(QStringLiteral("qet.setTextColor: '%1' is not a valid colour").arg(color));
		return false;
	}
	IndependentTextItem *item = findText(folioIndex, textIndex, QStringLiteral("setTextColor"));
	if (!item) return false;
	if (item->color() == new_color) return true;

	auto *cmd = new QPropertyUndoCommand(item, "color",
										 QVariant(item->color()), QVariant(new_color));
	cmd->setText(QObject::tr("Modifier la couleur d'un texte"));
	m_project->undoStack()->push(cmd);
	return true;
}

bool QetScriptApi::setTextRotation(int folioIndex, int textIndex, double angle)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.setTextRotation: project is read-only"));
		return false;
	}
	IndependentTextItem *item = findText(folioIndex, textIndex, QStringLiteral("setTextRotation"));
	if (!item) return false;

	auto *cmd = new QPropertyUndoCommand(item, "rotation",
										 QVariant(item->rotation()),
										 QVariant(item->rotation() + angle));
	cmd->setText(QObject::tr("Pivoter un texte"));
	m_project->undoStack()->push(cmd);
	return true;
}

bool QetScriptApi::deleteText(int folioIndex, int textIndex)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.deleteText: project is read-only"));
		return false;
	}
	IndependentTextItem *item = findText(folioIndex, textIndex, QStringLiteral("deleteText"));
	if (!item) return false;
	Diagram *diagram = m_project->diagrams().at(folioIndex);

	DiagramContent content;
	content.m_text_fields << item;
	diagram->undoStack().push(new DeleteQGraphicsItemCommand(diagram, content));
	return true;
}

QStringList QetScriptApi::shapes(int folioIndex) const
{
	QStringList out;
	const QList<QetShapeItem *> list = sortedShapes(folioIndex);
	for (int i = 0 ; i < list.count() ; ++i)
	{
		QetShapeItem *shape = list.at(i);
		const QRectF r = shape->sceneBoundingRect();
		out << QStringLiteral("%1: %2 (%3, %4) to (%5, %6)")
				.arg(i)
				.arg(shape->name())
				.arg(r.left()).arg(r.top()).arg(r.right()).arg(r.bottom());
	}
	return out;
}

/**
	@brief QetScriptApi::addShape
	Draw a line, rectangle, ellipse or polygon, as the shape tools do.
	Path is deliberately absent: it is built by successive clicks and has
	no two-point form to give here.
	@return the shape's index in shapes(), or -1
*/
int QetScriptApi::addShape(int folioIndex, const QString &type,
						   double x1, double y1, double x2, double y2)
{
	if (!m_project) return -1;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.addShape: project is read-only"));
		return -1;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return -1;

	QetShapeItem::ShapeType shape_type;
	const QString t = type.toLower();
	if (t == QLatin1String("line"))           shape_type = QetShapeItem::Line;
	else if (t == QLatin1String("rectangle")) shape_type = QetShapeItem::Rectangle;
	else if (t == QLatin1String("ellipse"))   shape_type = QetShapeItem::Ellipse;
	else if (t == QLatin1String("polygon"))   shape_type = QetShapeItem::Polygon;
	else {
		log(QStringLiteral("qet.addShape: unknown shape '%1'; expected line, "
						   "rectangle, ellipse or polygon").arg(type));
		return -1;
	}

	Diagram *diagram = diagrams.at(folioIndex);
	auto *shape = new QetShapeItem(QPointF(x1, y1), QPointF(x2, y2), shape_type);
	diagram->undoStack().push(new AddGraphicsObjectCommand(shape, diagram, QPointF(0, 0)));
	return sortedShapes(folioIndex).indexOf(shape);
}

bool QetScriptApi::deleteShape(int folioIndex, int shapeIndex)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.deleteShape: project is read-only"));
		return false;
	}
	const QList<QetShapeItem *> list = sortedShapes(folioIndex);
	if (shapeIndex < 0 || shapeIndex >= list.count()) {
		log(QStringLiteral("qet.deleteShape: folio %1 has %2 shape(s), no index %3")
			.arg(folioIndex).arg(list.count()).arg(shapeIndex));
		return false;
	}
	Diagram *diagram = m_project->diagrams().at(folioIndex);

	DiagramContent content;
	content.m_shapes << list.at(shapeIndex);
	diagram->undoStack().push(new DeleteQGraphicsItemCommand(diagram, content));
	return true;
}

/**
	@brief QetScriptApi::tables
	The tables and views the project database holds, as "name (type)".

	Worth reading before writing a query against them: the three *_view
	entries are the queryable surface and are named for it; the tables are
	how the cache is arranged today.
*/
QStringList QetScriptApi::tables() const
{
	QStringList list;
	if (!m_project || !m_project->dataBase()) return list;

	QSqlQuery q = m_project->dataBase()->newQuery(QStringLiteral(
		"SELECT name, type FROM sqlite_master WHERE type IN ('table','view') "
		"ORDER BY type, name"));
	while (q.next()) {
		list << QStringLiteral("%1 (%2)").arg(q.value(0).toString(), q.value(1).toString());
	}
	return list;
}

/**
	@brief QetScriptApi::query
	Run a read-only SELECT against the project database and return its rows
	as objects, one property per column.

	Goes through projectDataBase::newQuery(), which applies
	isReadOnlySelect() itself -- the same rule, and the same rejection
	message, that the "Requête SQL personnalisée" box in the element-query
	dialog shows a user. Nothing here can write: a statement that is not a
	single SELECT or WITH...SELECT is refused before it reaches SQLite.

	No updateDB() first, deliberately. A script that has just edited
	something is the expected caller, so querying a stale cache was the
	obvious hazard -- but projectDataBase maintains itself incrementally
	through addElement()/elementInfoChanged()/addConductor() and the rest,
	which the undo commands behind every edit here already call. Tested
	both ways on the cases most likely to be stale: an element added and
	labelled, and a conductor property changed, each queried immediately
	afterwards through both the table and the view. The counts are the
	same with the rebuild and without it. Since updateDB() is a full
	repopulation of every table, calling it per query would have been a
	real cost for no observable benefit -- so it is not called, and this
	note exists so it is not added back on the assumption that it must be
	needed.

	@return the rows; empty on refusal or SQL error, with queryError()
	saying which. An empty result and a failure are not the same thing.
*/
QVariantList QetScriptApi::query(const QString &sql)
{
	m_query_error.clear();
	QVariantList rows;
	if (!m_project || !m_project->dataBase()) {
		m_query_error = QStringLiteral("no project database");
		return rows;
	}

	QString rejection;
	QSqlQuery q = m_project->dataBase()->newQuery(sql, &rejection);
	if (!rejection.isEmpty()) {
		m_query_error = rejection;
		log(QStringLiteral("qet.query: %1").arg(rejection));
		return rows;
	}
	if (q.lastError().isValid()) {
		m_query_error = q.lastError().text();
		log(QStringLiteral("qet.query: %1").arg(m_query_error));
		return rows;
	}

	const QSqlRecord record = q.record();
	while (q.next())
	{
		QVariantMap row;
		for (int i = 0 ; i < record.count() ; ++i) {
			row.insert(record.fieldName(i), q.value(i));
		}
		rows << row;
	}
	return rows;
}

QString QetScriptApi::queryError() const
{
	return m_query_error;
}

int QetScriptApi::addFolio()
{
	if (!m_project) return -1;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.addFolio: project is read-only"));
		return -1;
	}
	Diagram *diagram = m_project->addNewDiagram();
	if (!diagram) return -1;
	return m_project->diagrams().indexOf(diagram);
}

bool QetScriptApi::setFolioTitle(int folioIndex, const QString &title)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.setFolioTitle: project is read-only"));
		return false;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return false;
	Diagram *diagram = diagrams.at(folioIndex);

	// The folio title is one field of the title block properties, so it
	// changes the way the title block dialog changes it: read the whole
	// struct, set one member, push the command with both versions.
	const TitleBlockProperties old_properties = diagram->border_and_titleblock.exportTitleBlock();
	if (old_properties.title == title) return true;
	TitleBlockProperties new_properties = old_properties;
	new_properties.title = title;

	auto *cmd = new ChangeTitleBlockCommand(diagram, old_properties, new_properties);
	m_project->undoStack()->push(cmd);
	return true;
}

bool QetScriptApi::undo()
{
	if (!m_project || !m_project->undoStack()->canUndo()) return false;
	m_project->undoStack()->undo();
	return true;
}

bool QetScriptApi::redo()
{
	if (!m_project || !m_project->undoStack()->canRedo()) return false;
	m_project->undoStack()->redo();
	return true;
}

bool QetScriptApi::canUndo() const
{
	return m_project && m_project->undoStack()->canUndo();
}

bool QetScriptApi::canRedo() const
{
	return m_project && m_project->undoStack()->canRedo();
}

bool QetScriptApi::selectElement(const QString &elementUuid)
{
	if (!m_project) return false;
	for (Diagram *diagram : m_project->diagrams()) {
		DiagramContent content(diagram, false);
		for (Element *elmt : std::as_const(content.m_elements)) {
			if (elmt->uuid().toString() == elementUuid) {
				elmt->setSelected(true);
				return true;
			}
		}
	}
	return false;
}

void QetScriptApi::deselectAll(int folioIndex)
{
	if (!m_project) return;
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return;
	diagrams.at(folioIndex)->clearSelection();
}

bool QetScriptApi::zoomFit()
{
	if (!m_view) return false;
	m_view->zoomFit();
	return true;
}

bool QetScriptApi::zoomToContent()
{
	if (!m_view) return false;
	m_view->zoomContent();
	return true;
}

bool QetScriptApi::zoomReset()
{
	if (!m_view) return false;
	m_view->zoomReset();
	return true;
}

/**
	@brief QetScriptApi::showMessage
	A modal QET::QetMessageBox::information() -- safe to call headless
	because non-interactive mode is already set for the whole process
	before any script runs (main.cpp), which is exactly the mechanism
	that keeps --export-* and --resave from hanging on a warning dialog.
	Interactively, a real person is there to see and dismiss it.
*/
void QetScriptApi::showMessage(const QString &text)
{
	QET::QetMessageBox::information(nullptr, QObject::tr("Script"), text);
}
