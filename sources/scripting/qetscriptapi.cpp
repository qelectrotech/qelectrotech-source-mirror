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

#include <QRegularExpression>
#include <optional>

#include "../ElementsCollection/elementslocation.h"
#include "../QPropertyUndoCommand/qpropertyundocommand.h"
#include "../cli_export.h"
#include "../diagram.h"
#include "../dataBase/ui/elementquerywidget.h"
#include "../dataBase/ui/summaryquerywidget.h"
#include "../diagramcontent.h"
#include "../diagramview.h"
#include "../factory/elementfactory.h"
#include "../factory/qetgraphicstablefactory.h"
#include "../factory/ui/addtabledialog.h"
#include "../qet.h"
#include "../qetgraphicsitem/ViewItem/qetgraphicstableitem.h"
#include "../qetgraphicsitem/element.h"
#include "../qetmessagebox.h"
#include "../dataBase/projectdatabase.h"
#include "../qetapp.h"
#include "../qetproject.h"
#include "../qetresult.h"
#include "../qetgraphicsitem/conductor.h"
#include "../conductorsegment.h"
#include "../qetgraphicsitem/diagramimageitem.h"

// See diagrameventaddpdf.h: a missing QtPdf module (or Qt < 6.4) is not
// fatal at build time, so addPdfPage() is always declared -- a script
// asking qet.addPdfPage exists must never get "not a function" for a
// reason it has no way to discover -- and logs a clear refusal instead of
// failing to compile.
#ifdef QET_HAS_QTPDF
#include <QPdfDocument>
#include <QPainter>
#endif
#include "../qetgraphicsitem/dynamicelementtextitem.h"
#include "../qetgraphicsitem/independenttextitem.h"
#include "../qetgraphicsitem/qetshapeitem.h"
#include "../undocommand/promoteshapecommand.h"
#include "../TerminalStrip/UndoCommand/addterminalstripcommand.h"
#include "../TerminalStrip/UndoCommand/addterminaltostripcommand.h"
#include "../TerminalStrip/UndoCommand/bridgeterminalscommand.h"
#include "../TerminalStrip/UndoCommand/groupterminalscommand.h"
#include "../TerminalStrip/UndoCommand/sortterminalstripcommand.h"
#include "../TerminalStrip/physicalterminal.h"
#include "../TerminalStrip/realterminal.h"
#include "../TerminalStrip/terminalstrip.h"
#include "../autoNum/assignvariables.h"
#include "../autoNum/numerotationcontext.h"
#include "../borderproperties.h"
#include "../titleblock/templatescollection.h"
#include "../diagramcommands.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetgraphicsitem/terminalelement.h"
#include "../qetinformation.h"
#include "../titleblockproperties.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../undocommand/addelementtextcommand.h"
#include "../undocommand/changeelementinformationcommand.h"
#include "../undocommand/changetitleblockcommand.h"
#include "../undocommand/deleteqgraphicsitemcommand.h"
#include "../undocommand/linkelementcommand.h"
#include "../undocommand/removediagramcommand.h"
#include "../utils/conductorcreator.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDomDocument>
#include <QFileInfo>
#include <QFont>
#include <QImage>
#include <QPixmap>
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
	if (name == QLatin1String("color2"))            return p.m_color_2.name();
	if (name == QLatin1String("bicolor"))           return p.m_bicolor ? QStringLiteral("true") : QStringLiteral("false");
	if (name == QLatin1String("dash-size"))         return QString::number(p.m_dash_size);
	if (name == QLatin1String("condsize"))          return QString::number(p.cond_size);
	if (name == QLatin1String("numsize"))           return QString::number(p.text_size);
	if (name == QLatin1String("displaytext"))       return p.m_show_text ? QStringLiteral("true") : QStringLiteral("false");
	if (name == QLatin1String("style")) {
		if (p.style == Qt::DashLine)    return QStringLiteral("dashed");
		if (p.style == Qt::DashDotLine) return QStringLiteral("dashdotted");
		return QStringLiteral("normal");
	}
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
	if (name == QLatin1String("color") || name == QLatin1String("text_color")
		|| name == QLatin1String("color2"))
	{
		const QColor c(value);
		if (!c.isValid()) return false;
		if (name == QLatin1String("color")) p.color = c;
		else if (name == QLatin1String("color2")) p.m_color_2 = c;
		else p.text_color = c;
		return true;
	}
	if (name == QLatin1String("bicolor") || name == QLatin1String("displaytext"))
	{
		const QString v = value.toLower();
		if (v != QLatin1String("true") && v != QLatin1String("false")) return false;
		(name == QLatin1String("bicolor") ? p.m_bicolor : p.m_show_text) = (v == QLatin1String("true"));
		return true;
	}
	if (name == QLatin1String("dash-size") || name == QLatin1String("numsize"))
	{
		bool ok = false;
		const int n = value.toInt(&ok);
		if (!ok || n < 1) return false;
		(name == QLatin1String("dash-size") ? p.m_dash_size : p.text_size) = n;
		return true;
	}
	if (name == QLatin1String("condsize"))
	{
		bool ok = false;
		const double d = value.toDouble(&ok);
		if (!ok || d <= 0) return false;
		p.cond_size = d;
		return true;
	}
	if (name == QLatin1String("style"))
	{
		// The three the file format can express (ConductorProperties::readStyle);
		// any other Qt pen style would be written back as a solid line.
		if (value == QLatin1String("normal"))          p.style = Qt::SolidLine;
		else if (value == QLatin1String("dashed"))     p.style = Qt::DashLine;
		else if (value == QLatin1String("dashdotted")) p.style = Qt::DashDotLine;
		else return false;
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
		QStringLiteral("color"), QStringLiteral("text_color"), QStringLiteral("color2"),
		QStringLiteral("bicolor"), QStringLiteral("style"), QStringLiteral("dash-size"),
		QStringLiteral("condsize"), QStringLiteral("numsize"), QStringLiteral("displaytext")};
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
	file of any project written through this API.

	The index is the terminal's place in Element::terminals(), and that is
	@b not the order the .elmt lists them in. Element::parseTerminal()
	re-sorts the list on every insertion, top to bottom and then left to
	right on each terminal's local position, so index 0 is the topmost
	terminal. bobine_ka_a_remanence.elmt writes A2 (y=20) before A1 (y=-20)
	and index 0 is A1. Of the 837 shipped elements whose terminals all have
	distinct names, 619 list them in a different order than this. Two
	terminals at the same point tie, and the sort is not stable, so which
	of those is which is undefined -- read this listing rather than
	assuming.
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

/**
	@brief QetScriptApi::conductorSegments
	List the drawn path of the conductor on a terminal, one line per
	segment: "index: (x1,y1)-(x2,y2) horizontal|vertical static|movable".
	static marks a segment anchored to a terminal (moveConductorSegment()
	on it is a no-op, the same as dragging its handle would be -- there is
	no handle on it in the GUI). Points are in scene coordinates, matching
	element_geometry().
*/
QStringList QetScriptApi::conductorSegments(int folioIndex, const QString &elementUuid,
											int terminalIndex) const
{
	// const_cast: findConductor logs, and log() writes to stderr, which is
	// not a const operation on this object. The lookup itself changes
	// nothing.
	auto *self = const_cast<QetScriptApi *>(this);
	Conductor *conductor = self->findConductor(folioIndex, elementUuid, terminalIndex,
											   QStringLiteral("conductorSegments"));
	if (!conductor) return {};

	QStringList result;
	const QList<ConductorSegment *> segs = conductor->segmentsList();
	for (int i = 0; i < segs.count(); ++i) {
		ConductorSegment *seg = segs.at(i);
		const QPointF p1 = conductor->mapToScene(seg->firstPoint());
		const QPointF p2 = conductor->mapToScene(seg->secondPoint());
		result << QStringLiteral("%1: (%2,%3)-(%4,%5) %6 %7")
			.arg(i)
			.arg(p1.x()).arg(p1.y()).arg(p2.x()).arg(p2.y())
			.arg(seg->isHorizontal() ? QStringLiteral("horizontal") : QStringLiteral("vertical"),
				 seg->isStatic() ? QStringLiteral("static") : QStringLiteral("movable"));
	}
	return result;
}

/**
	@brief QetScriptApi::moveConductorSegment
	Move one segment of the conductor on a terminal by (dx, dy) and push
	one undo step for the whole move -- Conductor::moveSegment(), the same
	primitive a manual handle drag applies. A segment only moves
	perpendicular to its own direction, the same as dragging its handle:
	dx moves a vertical segment, dy moves a horizontal one, and the other
	of the pair is ignored (ConductorSegment::moveX()/moveY() each silently
	no-op on the wrong axis) -- check conductorSegments() for which one
	applies before calling this. dx/dy are in scene coordinates; a
	translation-only item (every conductor) makes a scene-space delta equal
	to a local one, so no conversion is needed. A static segment or an
	out-of-range index is refused.
*/
bool QetScriptApi::moveConductorSegment(int folioIndex, const QString &elementUuid,
										int terminalIndex, int segmentIndex,
										double dx, double dy)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.moveConductorSegment: project is read-only"));
		return false;
	}
	Conductor *conductor = findConductor(folioIndex, elementUuid, terminalIndex,
										 QStringLiteral("moveConductorSegment"));
	if (!conductor) return false;

	const QList<ConductorSegment *> segs = conductor->segmentsList();
	if (segmentIndex < 0 || segmentIndex >= segs.count()) {
		log(QStringLiteral("qet.moveConductorSegment: terminal %1 of %2 has %3 "
						   "segment(s), no index %4")
			.arg(terminalIndex).arg(elementUuid).arg(segs.count()).arg(segmentIndex));
		return false;
	}
	if (segs.at(segmentIndex)->isStatic()) {
		log(QStringLiteral("qet.moveConductorSegment: segment %1 is anchored to a "
						   "terminal and cannot be moved").arg(segmentIndex));
		return false;
	}

	return conductor->moveSegment(segmentIndex, dx, dy);
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
								int folioIndexB, const QString &elementUuidB,
								int groupIndex)
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
	// Linking two report elements (next_report/previous_report) that each
	// already carry a conductor with differing properties (colour, line
	// style, text, function or tension protocol) makes
	// LinkElementCommand::redo() open PotentialSelectorDialog -- a plain
	// QDialog::exec(), not routed through QET::QetMessageBox, so headless
	// --run has nobody to answer it and hangs until killed. Measured:
	// confirmed hanging with the fix below NOT in place, confirmed clean
	// (no hang, false returned) with it in place. Same reasoning and the
	// same choice addConductor() already makes about ConductorCreator's
	// own ambiguous-potential dialog.
	if (LinkElementCommand::reportLinkNeedsPotentialChoice(a, b)) {
		log(QStringLiteral("qet.linkElements: %1 and %2 already carry conductors whose "
						   "colour, style, text, function or tension protocol disagree -- "
						   "linking them would open a dialog asking which to keep, and "
						   "nobody is there to answer it under --run. Use "
						   "qet.checkContinuity() to see the mismatch, resolve it with "
						   "setConductorProperty() on one side, then link")
			.arg(elementUuidA, elementUuidB));
		return false;
	}
	auto isPlcMaster = [](Element *e) {
		return e->elementData().m_type == ElementData::Master
			&& e->elementData().m_master_type == ElementData::PLC;
	};
	if (groupIndex >= 0 && !isPlcMaster(a) && !isPlcMaster(b)) {
		log(QStringLiteral("qet.linkElements: groupIndex only applies to a PLC "
						   "master/slave pair -- neither %1 nor %2 is a PLC master")
			.arg(elementUuidA, elementUuidB));
		return false;
	}

	// LinkElementCommand only reads m_group_index when the command's OWN
	// element is the Slave -- when it is the Master it looks in a
	// per-slave m_group_indices map this call never populates, and
	// setGroupIndex() is silently a no-op. Build the command from the
	// slave's side instead, exactly as PlcLinkWidget does (m_element is
	// always the slave being edited there).
	Element *slave = (groupIndex >= 0 && b->elementData().m_type == ElementData::Slave) ? b : a;
	Element *master = (slave == a) ? b : a;

	auto *cmd = new LinkElementCommand(slave);
	cmd->setLink(master);
	if (groupIndex >= 0)
		cmd->setGroupIndex(groupIndex);
	m_project->undoStack()->push(cmd);
	return a->linkedElements().contains(b);
}

/**
	@brief QetScriptApi::elementLinkGroupIndex
	The PLC IO row a linked slave is attached to -- the index passed as
	linkElements()'s groupIndex when the link was made, or -1 if the pair
	is not linked or the link carries no group index (an ordinary
	master/slave or report pair, not a PLC one).
*/
int QetScriptApi::elementLinkGroupIndex(int folioIndex, const QString &elementUuid,
										int otherFolioIndex, const QString &otherElementUuid) const
{
	Element *element = findElement(folioIndex, elementUuid);
	Element *other = findElement(otherFolioIndex, otherElementUuid);
	if (!element || !other) return -1;
	return element->groupIndexForElement(other);
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
bool isPlcMaster(Element *element)
{
	return element
		&& element->elementData().m_type == ElementData::Master
		&& element->elementData().m_master_type == ElementData::PLC;
}
}

/**
	@brief QetScriptApi::plcIOs
	List a PLC master's IO table, one line per row: "index: type address
	'functionText' 'comment' -> crossRef". crossRef is empty until a slave
	is linked onto that row (linkElements()'s groupIndex).
*/
QStringList QetScriptApi::plcIOs(int folioIndex, const QString &elementUuid) const
{
	Element *element = findElement(folioIndex, elementUuid);
	if (!isPlcMaster(element)) return {};

	QStringList result;
	const auto ios = element->elementData().plcMasterData().ios;
	for (int i = 0; i < ios.count(); ++i) {
		const auto &io = ios.at(i);
		result << QStringLiteral("%1: %2 %3 '%4' '%5' -> %6")
			.arg(i)
			.arg(ElementData::plcIOTypeToString(io.type), io.address,
				 io.functionText, io.comment, io.crossRef);
	}
	return result;
}

/**
	@brief QetScriptApi::addPlcIO
	Append a row to a PLC master's IO table. type is one of
	entree_digitale, sortie_digitale, entree_analogique, sortie_analogique,
	entree_universelle, sortie_universelle. Returns the new row's index, or
	-1 if elementUuid is not a PLC master.

	This edits ElementData directly through setElementData(), the same as
	MasterPropertiesWidget's own PLC IO table -- which, like it, is NOT
	undoable: MasterPropertiesWidget::associatedUndo() deliberately returns
	nullptr for PLC masters (their linking is managed through the IO table,
	not the link-tree widget it would otherwise build an unlink-all command
	from), so qet.undo() cannot revert an addPlcIO/setPlcIO/removePlcIO call
	any more than the GUI's own PLC IO editor can.
*/
int QetScriptApi::addPlcIO(int folioIndex, const QString &elementUuid, const QString &type,
						   const QString &address, const QString &functionText,
						   const QString &comment)
{
	if (!m_project) return -1;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.addPlcIO: project is read-only"));
		return -1;
	}
	Element *element = findElement(folioIndex, elementUuid);
	if (!isPlcMaster(element)) {
		log(QStringLiteral("qet.addPlcIO: %1 is not a PLC master").arg(elementUuid));
		return -1;
	}

	ElementData ed = element->elementData();
	ElementData::PlcMasterData plc_data = ed.plcMasterData();
	ElementData::PlcIO io;
	io.type = ElementData::plcIOTypeFromString(type);
	io.address = address;
	io.functionText = functionText;
	io.comment = comment;
	plc_data.ios.append(io);
	ed.setPlcMasterData(plc_data);
	element->setElementData(ed);
	if (element->scene()) element->update();

	return plc_data.ios.count() - 1;
}

/**
	@brief QetScriptApi::setPlcIO
	Change one field of a PLC master IO row: type, address, function
	(functionText) or comment. See addPlcIO() for the undo caveat this
	shares.
*/
bool QetScriptApi::setPlcIO(int folioIndex, const QString &elementUuid, int ioIndex,
							const QString &property, const QString &value)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.setPlcIO: project is read-only"));
		return false;
	}
	Element *element = findElement(folioIndex, elementUuid);
	if (!isPlcMaster(element)) {
		log(QStringLiteral("qet.setPlcIO: %1 is not a PLC master").arg(elementUuid));
		return false;
	}

	ElementData ed = element->elementData();
	ElementData::PlcMasterData plc_data = ed.plcMasterData();
	if (ioIndex < 0 || ioIndex >= plc_data.ios.count()) {
		log(QStringLiteral("qet.setPlcIO: %1 has %2 IO row(s), no index %3")
			.arg(elementUuid).arg(plc_data.ios.count()).arg(ioIndex));
		return false;
	}

	ElementData::PlcIO &io = plc_data.ios[ioIndex];
	if (property == QLatin1String("type")) {
		io.type = ElementData::plcIOTypeFromString(value);
	} else if (property == QLatin1String("address")) {
		io.address = value;
	} else if (property == QLatin1String("function")) {
		io.functionText = value;
	} else if (property == QLatin1String("comment")) {
		io.comment = value;
	} else {
		log(QStringLiteral("qet.setPlcIO: unknown property '%1'; expected type, "
						   "address, function or comment").arg(property));
		return false;
	}

	ed.setPlcMasterData(plc_data);
	element->setElementData(ed);
	if (element->scene()) element->update();
	return true;
}

/**
	@brief QetScriptApi::removePlcIO
	Remove one row from a PLC master's IO table. See addPlcIO() for the
	undo caveat this shares. Indexes shift afterwards.
*/
bool QetScriptApi::removePlcIO(int folioIndex, const QString &elementUuid, int ioIndex)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.removePlcIO: project is read-only"));
		return false;
	}
	Element *element = findElement(folioIndex, elementUuid);
	if (!isPlcMaster(element)) {
		log(QStringLiteral("qet.removePlcIO: %1 is not a PLC master").arg(elementUuid));
		return false;
	}

	ElementData ed = element->elementData();
	ElementData::PlcMasterData plc_data = ed.plcMasterData();
	if (ioIndex < 0 || ioIndex >= plc_data.ios.count()) {
		log(QStringLiteral("qet.removePlcIO: %1 has %2 IO row(s), no index %3")
			.arg(elementUuid).arg(plc_data.ios.count()).arg(ioIndex));
		return false;
	}

	plc_data.ios.removeAt(ioIndex);
	ed.setPlcMasterData(plc_data);
	element->setElementData(ed);
	if (element->scene()) element->update();
	return true;
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
QList<T *> sortedByPosition(const QList<T *> &items_in)
{
	QList<T *> list = items_in;
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

template <typename T>
QList<T *> sortedByPosition(const QSet<T *> &items)
{
	// Qt 6 makes QVector an alias of QList, so this one overload also
	// serves m_tables (a QVector<QetGraphicsTableItem *>) without a
	// separate one -- adding one was a redefinition error, not a second
	// overload, on this Qt version.
	return sortedByPosition(QList<T *>(items.cbegin(), items.cend()));
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

QString QetScriptApi::textContent(int folioIndex, int textIndex) const
{
	auto *self = const_cast<QetScriptApi *>(this);
	IndependentTextItem *item = self->findText(folioIndex, textIndex, QStringLiteral("textContent"));
	if (!item) return QString();
	return item->toPlainText();
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

namespace {
const QStringList &shapePropertyNames()
{
	static const QStringList n{QStringLiteral("color"), QStringLiteral("fill"),
		QStringLiteral("width"), QStringLiteral("line-style"), QStringLiteral("rotation")};
	return n;
}
} // namespace

QString QetScriptApi::shapeProperty(int folioIndex, int shapeIndex, const QString &property) const
{
	const QList<QetShapeItem *> list = sortedShapes(folioIndex);
	if (shapeIndex < 0 || shapeIndex >= list.count()) return QString();
	QetShapeItem *shape = list.at(shapeIndex);
	if (property == QLatin1String("color"))  return shape->pen().color().name();
	if (property == QLatin1String("width"))  return QString::number(shape->pen().widthF());
	if (property == QLatin1String("fill"))
		return shape->brush().style() == Qt::NoBrush ? QStringLiteral("none") : shape->brush().color().name();
	if (property == QLatin1String("rotation")) return QString::number(shape->rotation());
	if (property == QLatin1String("line-style")) {
		switch (shape->pen().style()) {
			case Qt::DashLine:    return QStringLiteral("dashed");
			case Qt::DotLine:     return QStringLiteral("dotted");
			case Qt::DashDotLine: return QStringLiteral("dashdot");
			default:              return QStringLiteral("solid");
		}
	}
	return QString();
}

/**
	@brief QetScriptApi::setShapeProperty
	Change one aspect of a shape's look through QPropertyUndoCommand on the
	"pen", "brush" or "rotation" property, the properties the shape's own
	style editor publishes. Values are validated and refused rather than
	stored: a colour that does not parse, a non-positive width, a line
	style outside solid/dashed/dotted/dashdot.
*/
bool QetScriptApi::setShapeProperty(int folioIndex, int shapeIndex,
									const QString &property, const QString &value)
{
	if (!m_project) return false;
	const QString caller = QStringLiteral("setShapeProperty");
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return false;
	}
	if (!shapePropertyNames().contains(property)) {
		log(QStringLiteral("qet.%1: unknown property '%2'; expected one of %3")
			.arg(caller, property, shapePropertyNames().join(QStringLiteral(", "))));
		return false;
	}
	const QList<QetShapeItem *> list = sortedShapes(folioIndex);
	if (shapeIndex < 0 || shapeIndex >= list.count()) {
		log(QStringLiteral("qet.%1: folio %2 has %3 shape(s), no index %4")
			.arg(caller).arg(folioIndex).arg(list.count()).arg(shapeIndex));
		return false;
	}
	QetShapeItem *shape = list.at(shapeIndex);

	QString what;
	QVariant old_value, new_value;
	const char *qt_property = nullptr;
	if (property == QLatin1String("color") || property == QLatin1String("width")
		|| property == QLatin1String("line-style"))
	{
		QPen pen = shape->pen();
		if (property == QLatin1String("color")) {
			const QColor c(value);
			if (!c.isValid()) { log(QStringLiteral("qet.%1: '%2' is not a valid colour").arg(caller, value)); return false; }
			pen.setColor(c);
		} else if (property == QLatin1String("width")) {
			bool ok = false;
			const double w = value.toDouble(&ok);
			if (!ok || w <= 0) { log(QStringLiteral("qet.%1: '%2' is not a positive width").arg(caller, value)); return false; }
			pen.setWidthF(w);
		} else {
			if (value == QLatin1String("solid"))        pen.setStyle(Qt::SolidLine);
			else if (value == QLatin1String("dashed"))  pen.setStyle(Qt::DashLine);
			else if (value == QLatin1String("dotted"))  pen.setStyle(Qt::DotLine);
			else if (value == QLatin1String("dashdot")) pen.setStyle(Qt::DashDotLine);
			else { log(QStringLiteral("qet.%1: unknown line-style '%2'").arg(caller, value)); return false; }
		}
		if (pen == shape->pen()) return true;
		old_value = shape->pen(); new_value = pen; qt_property = "pen"; what = QObject::tr("Modifier le trait d'une forme");
	}
	else if (property == QLatin1String("fill"))
	{
		QBrush brush = shape->brush();
		if (value == QLatin1String("none")) brush.setStyle(Qt::NoBrush);
		else {
			const QColor c(value);
			if (!c.isValid()) { log(QStringLiteral("qet.%1: '%2' is not a valid colour").arg(caller, value)); return false; }
			brush.setStyle(Qt::SolidPattern); brush.setColor(c);
		}
		if (brush == shape->brush()) return true;
		old_value = shape->brush(); new_value = brush; qt_property = "brush"; what = QObject::tr("Modifier le remplissage d'une forme");
	}
	else
	{
		bool ok = false;
		const double angle = value.toDouble(&ok);
		if (!ok) { log(QStringLiteral("qet.%1: '%2' is not an angle").arg(caller, value)); return false; }
		if (angle == shape->rotation()) return true;
		old_value = shape->rotation(); new_value = angle; qt_property = "rotation"; what = QObject::tr("Pivoter une forme");
	}

	auto *cmd = new QPropertyUndoCommand(shape, qt_property, old_value, new_value);
	cmd->setText(what);
	m_project->undoStack()->push(cmd);
	return true;
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

namespace {

bool variantToPointF(const QVariant &v, QPointF &out)
{
	const QVariantMap m = v.toMap();
	if (!m.contains(QStringLiteral("x")) || !m.contains(QStringLiteral("y"))) return false;
	bool okx = false, oky = false;
	const double x = m.value(QStringLiteral("x")).toDouble(&okx);
	const double y = m.value(QStringLiteral("y")).toDouble(&oky);
	if (!okx || !oky) return false;
	out = QPointF(x, y);
	return true;
}

QPolygonF variantToPolygon(const QVariantList &points, bool *ok)
{
	QPolygonF poly;
	for (const QVariant &v : points) {
		QPointF p;
		if (!variantToPointF(v, p)) { *ok = false; return {}; }
		poly << p;
	}
	*ok = true;
	return poly;
}

QVariantMap pointFToVariant(const QPointF &p)
{
	QVariantMap m;
	m.insert(QStringLiteral("x"), p.x());
	m.insert(QStringLiteral("y"), p.y());
	return m;
}

QetShapeItem::NodeKind nodeKindFromString(const QString &s)
{
	if (s == QLatin1String("smooth")) return QetShapeItem::NodeKind::Smooth;
	if (s == QLatin1String("symmetric")) return QetShapeItem::NodeKind::Symmetric;
	return QetShapeItem::NodeKind::Corner;
}

QString nodeKindToString(QetShapeItem::NodeKind k)
{
	switch (k) {
		case QetShapeItem::NodeKind::Smooth:    return QStringLiteral("smooth");
		case QetShapeItem::NodeKind::Symmetric: return QStringLiteral("symmetric");
		default:                                return QStringLiteral("corner");
	}
}

// Node format: {x, y, kind: "corner"|"smooth"|"symmetric", inHandle:
// {x,y}, outHandle: {x,y}} -- inHandle/outHandle are omitted (not merely
// null) when a node has none, matching PathNode's std::optional.
bool variantToPathNode(const QVariant &v, QetShapeItem::PathNode &out)
{
	const QVariantMap m = v.toMap();
	QPointF anchor;
	if (!variantToPointF(v, anchor)) return false;
	out.anchor = anchor;
	out.kind = nodeKindFromString(m.value(QStringLiteral("kind")).toString());
	if (m.contains(QStringLiteral("inHandle"))) {
		QPointF h;
		if (!variantToPointF(m.value(QStringLiteral("inHandle")), h)) return false;
		out.inHandle = h;
	}
	if (m.contains(QStringLiteral("outHandle"))) {
		QPointF h;
		if (!variantToPointF(m.value(QStringLiteral("outHandle")), h)) return false;
		out.outHandle = h;
	}
	return true;
}

QVariantMap pathNodeToVariant(const QetShapeItem::PathNode &n)
{
	QVariantMap m = pointFToVariant(n.anchor);
	m.insert(QStringLiteral("kind"), nodeKindToString(n.kind));
	if (n.inHandle) m.insert(QStringLiteral("inHandle"), pointFToVariant(*n.inHandle));
	if (n.outHandle) m.insert(QStringLiteral("outHandle"), pointFToVariant(*n.outHandle));
	return m;
}

} // namespace

/**
	@brief QetScriptApi::addPolygon
	Place a Polygon shape with as many points as given -- addShape()'s
	"polygon" only ever produces the degenerate two-point form, since it
	shares addShape()'s p1/p2 constructor and nothing else. Points are in
	scene coordinates, as [{x,y}, ...]. Returns the new shape's index, or
	-1 (at least 2 points are required).
*/
int QetScriptApi::addPolygon(int folioIndex, const QVariantList &points, bool closed)
{
	if (!m_project) return -1;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.addPolygon: project is read-only"));
		return -1;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return -1;
	if (points.count() < 2) {
		log(QStringLiteral("qet.addPolygon: at least 2 points are required, got %1")
			.arg(points.count()));
		return -1;
	}
	bool ok = false;
	const QPolygonF poly = variantToPolygon(points, &ok);
	if (!ok) {
		log(QStringLiteral("qet.addPolygon: every point must be an {x, y} object"));
		return -1;
	}

	Diagram *diagram = diagrams.at(folioIndex);
	auto *shape = new QetShapeItem(poly.first(), poly.last(), QetShapeItem::Polygon);
	shape->setPolygon(poly);
	shape->setClosed(closed);
	diagram->undoStack().push(new AddGraphicsObjectCommand(shape, diagram, QPointF(0, 0)));
	return sortedShapes(folioIndex).indexOf(shape);
}

/**
	@brief QetScriptApi::shapePolygon
	A Polygon shape's own points, in scene coordinates, as [{x,y}, ...].
	Empty for any other shape type or an out-of-range index.
*/
QVariantList QetScriptApi::shapePolygon(int folioIndex, int shapeIndex) const
{
	const QList<QetShapeItem *> list = sortedShapes(folioIndex);
	if (shapeIndex < 0 || shapeIndex >= list.count()) return {};
	QetShapeItem *shape = list.at(shapeIndex);
	if (shape->shapeType() != QetShapeItem::Polygon) return {};
	QVariantList result;
	for (const QPointF &p : shape->polygon())
		result << pointFToVariant(shape->mapToScene(p));
	return result;
}

/**
	@brief QetScriptApi::setShapePolygon
	Replace a Polygon shape's points through QPropertyUndoCommand on its
	"polygon" Q_PROPERTY, the same as dragging one of its point handles.
*/
bool QetScriptApi::setShapePolygon(int folioIndex, int shapeIndex, const QVariantList &points)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.setShapePolygon: project is read-only"));
		return false;
	}
	const QList<QetShapeItem *> list = sortedShapes(folioIndex);
	if (shapeIndex < 0 || shapeIndex >= list.count()) {
		log(QStringLiteral("qet.setShapePolygon: folio %1 has %2 shape(s), no index %3")
			.arg(folioIndex).arg(list.count()).arg(shapeIndex));
		return false;
	}
	QetShapeItem *shape = list.at(shapeIndex);
	if (shape->shapeType() != QetShapeItem::Polygon) {
		log(QStringLiteral("qet.setShapePolygon: shape %1 is not a polygon").arg(shapeIndex));
		return false;
	}
	if (points.count() < 2) {
		log(QStringLiteral("qet.setShapePolygon: at least 2 points are required, got %1")
			.arg(points.count()));
		return false;
	}
	bool ok = false;
	const QPolygonF poly = variantToPolygon(points, &ok);
	if (!ok) {
		log(QStringLiteral("qet.setShapePolygon: every point must be an {x, y} object"));
		return false;
	}

	const QVariant old_value = QVariant::fromValue(shape->polygon());
	const QVariant new_value = QVariant::fromValue(poly);
	if (shape->polygon() == poly) return true;
	auto *cmd = new QPropertyUndoCommand(shape, "polygon", old_value, new_value);
	cmd->setText(QObject::tr("Modifier la forme d'%1").arg(shape->name()));
	m_project->undoStack()->push(cmd);
	return true;
}

/**
	@brief QetScriptApi::addPath
	Place a Path shape -- a Polygon's points plus, per node, a kind
	(corner/smooth/symmetric) and optional bezier in/out handles, the same
	model the pen tool and node-edit mode build. See variantToPathNode()
	for the node format. Returns the new shape's index, or -1 (at least 2
	nodes are required).
*/
int QetScriptApi::addPath(int folioIndex, const QVariantList &nodes, bool closed)
{
	if (!m_project) return -1;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.addPath: project is read-only"));
		return -1;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return -1;
	if (nodes.count() < 2) {
		log(QStringLiteral("qet.addPath: at least 2 nodes are required, got %1").arg(nodes.count()));
		return -1;
	}
	QVector<QetShapeItem::PathNode> path_nodes;
	for (const QVariant &v : nodes) {
		QetShapeItem::PathNode node;
		if (!variantToPathNode(v, node)) {
			log(QStringLiteral("qet.addPath: every node must be an {x, y} object, "
							   "optionally with kind/inHandle/outHandle"));
			return -1;
		}
		path_nodes << node;
	}

	Diagram *diagram = diagrams.at(folioIndex);
	auto *shape = new QetShapeItem(path_nodes.first().anchor, path_nodes.last().anchor,
								   QetShapeItem::Path);
	shape->setPathNodes(path_nodes);
	shape->setClosed(closed);
	diagram->undoStack().push(new AddGraphicsObjectCommand(shape, diagram, QPointF(0, 0)));
	return sortedShapes(folioIndex).indexOf(shape);
}

/**
	@brief QetScriptApi::shapePathNodes
	A Path shape's own nodes, in scene coordinates -- see
	variantToPathNode()/pathNodeToVariant() for the format. Empty for any
	other shape type or an out-of-range index.
*/
QVariantList QetScriptApi::shapePathNodes(int folioIndex, int shapeIndex) const
{
	const QList<QetShapeItem *> list = sortedShapes(folioIndex);
	if (shapeIndex < 0 || shapeIndex >= list.count()) return {};
	QetShapeItem *shape = list.at(shapeIndex);
	if (shape->shapeType() != QetShapeItem::Path) return {};
	QVariantList result;
	for (const QetShapeItem::PathNode &n : shape->pathNodes()) {
		QetShapeItem::PathNode scene_node = n;
		scene_node.anchor = shape->mapToScene(n.anchor);
		if (n.inHandle) scene_node.inHandle = shape->mapToScene(*n.inHandle);
		if (n.outHandle) scene_node.outHandle = shape->mapToScene(*n.outHandle);
		result << pathNodeToVariant(scene_node);
	}
	return result;
}

/**
	@brief QetScriptApi::setShapePathNodes
	Replace a Path shape's nodes. PathNode/QVector<PathNode> is not a
	Q_PROPERTY-friendly type (it holds std::optional<QPointF> members), so
	this reuses PromoteShapeCommand's generic before/after XML snapshot
	mechanism instead -- the same one the node-edit handle drag itself
	falls back to, and for the identical reason (see the PathAnchor case
	in QetShapeItem::associatedUndoCommand()).
*/
bool QetScriptApi::setShapePathNodes(int folioIndex, int shapeIndex, const QVariantList &nodes)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.setShapePathNodes: project is read-only"));
		return false;
	}
	const QList<QetShapeItem *> list = sortedShapes(folioIndex);
	if (shapeIndex < 0 || shapeIndex >= list.count()) {
		log(QStringLiteral("qet.setShapePathNodes: folio %1 has %2 shape(s), no index %3")
			.arg(folioIndex).arg(list.count()).arg(shapeIndex));
		return false;
	}
	QetShapeItem *shape = list.at(shapeIndex);
	if (shape->shapeType() != QetShapeItem::Path) {
		log(QStringLiteral("qet.setShapePathNodes: shape %1 is not a path").arg(shapeIndex));
		return false;
	}
	if (nodes.count() < 2) {
		log(QStringLiteral("qet.setShapePathNodes: at least 2 nodes are required, got %1")
			.arg(nodes.count()));
		return false;
	}
	QVector<QetShapeItem::PathNode> path_nodes;
	for (const QVariant &v : nodes) {
		QetShapeItem::PathNode node;
		if (!variantToPathNode(v, node)) {
			log(QStringLiteral("qet.setShapePathNodes: every node must be an {x, y} "
							   "object, optionally with kind/inHandle/outHandle"));
			return false;
		}
		path_nodes << node;
	}
	if (path_nodes == shape->pathNodes()) return true;

	QDomDocument before_doc;
	const QDomElement before = shape->toXml(before_doc);
	before_doc.appendChild(before);
	shape->setPathNodes(path_nodes);
	QDomDocument after_doc;
	const QDomElement after = shape->toXml(after_doc);
	after_doc.appendChild(after);

	auto *cmd = new PromoteShapeCommand(shape, before, after);
	cmd->setText(QObject::tr("Modifier la forme d'%1").arg(shape->name()));
	m_project->undoStack()->push(cmd);
	return true;
}

/**
	@brief QetScriptApi::setShapeClosed
	Open or close a Polygon or Path shape through QPropertyUndoCommand on
	its "close" Q_PROPERTY. A no-op (returns true) on any other shape type,
	the same as QetShapeItem::setClosed() itself.
*/
bool QetScriptApi::setShapeClosed(int folioIndex, int shapeIndex, bool closed)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.setShapeClosed: project is read-only"));
		return false;
	}
	const QList<QetShapeItem *> list = sortedShapes(folioIndex);
	if (shapeIndex < 0 || shapeIndex >= list.count()) {
		log(QStringLiteral("qet.setShapeClosed: folio %1 has %2 shape(s), no index %3")
			.arg(folioIndex).arg(list.count()).arg(shapeIndex));
		return false;
	}
	QetShapeItem *shape = list.at(shapeIndex);
	if (shape->isClosed() == closed) return true;

	auto *cmd = new QPropertyUndoCommand(shape, "close", shape->isClosed(), closed);
	cmd->setText(QObject::tr("Fermer/Ouvrir %1").arg(shape->name()));
	m_project->undoStack()->push(cmd);
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
	A result that ran past projectDataBase::MaxResultRows is cut there and
	queryError() says so, so a truncated list is never mistaken for a
	complete one.
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
		//SQLite produces rows lazily, so a query that never stops
		//producing them makes this loop never stop either -- "WITH
		//RECURSIVE c(n) AS (SELECT 1 UNION ALL SELECT n+1 FROM c) SELECT n
		//FROM c" is one line and runs until memory is gone. The script
		//engine's own 30 s interrupt does not reach here: that aborts
		//JavaScript execution, and this is C++ inside a single call.
		//@see projectDataBase::MaxResultRows.
		if (rows.size() >= projectDataBase::MaxResultRows) {
			m_query_error = QStringLiteral(
						"result truncated at %1 rows; add a LIMIT or a "
						"WHERE clause")
					.arg(projectDataBase::MaxResultRows);
			log(QStringLiteral("qet.query: %1").arg(m_query_error));
			break;
		}

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

/**
	@brief QetScriptApi::deleteConductor
	Delete the single conductor attached to a terminal (same addressing as
	setConductorProperty()). Unlike a property change this removes only that
	conductor: DeleteQGraphicsItemCommand itself rebuilds the remaining
	conductors of the potential so it stays connected, exactly as when a user
	selects one conductor and presses Delete.
*/
bool QetScriptApi::deleteConductor(int folioIndex, const QString &elementUuid, int terminalIndex)
{
	if (!m_project) return false;
	const QString caller = QStringLiteral("deleteConductor");
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return false;
	}
	Conductor *conductor = findConductor(folioIndex, elementUuid, terminalIndex, caller);
	if (!conductor) return false;
	Diagram *diagram = m_project->diagrams().at(folioIndex);

	DiagramContent content;
	content.m_conductors_to_move << conductor;
	diagram->undoStack().push(new DeleteQGraphicsItemCommand(diagram, content));
	return true;
}

/**
	@brief QetScriptApi::removeFolio
	Remove a folio through RemoveDiagramCommand, the command the GUI's
	"delete folio" pushes (minus its confirmation box, which nobody could
	answer headlessly). Undoable. Later folio indexes shift down by one.
*/
bool QetScriptApi::removeFolio(int folioIndex)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.removeFolio: project is read-only"));
		return false;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return false;
	m_project->undoStack()->push(new RemoveDiagramCommand(m_project, diagrams.at(folioIndex)));
	return m_project->diagrams().count() == diagrams.count() - 1;
}

namespace {
QString *titleBlockField(TitleBlockProperties &p, const QString &name)
{
	if (name == QLatin1String("title"))     return &p.title;
	if (name == QLatin1String("author"))    return &p.author;
	if (name == QLatin1String("filename"))  return &p.filename;
	if (name == QLatin1String("plant"))     return &p.plant;
	if (name == QLatin1String("locmach"))   return &p.locmach;
	if (name == QLatin1String("indexrev"))  return &p.indexrev;
	// Not "version": TitleBlockProperties::version is the file-format stamp
	// QElectroTech writes on every save, so a value set here reports success
	// and is overwritten -- measured: set "V9-USER", read back "0.200.1-dev".
	if (name == QLatin1String("folio"))     return &p.folio;
	// Not "template" either: template_name resolves against the project's
	// embedded collection, not free text, so it goes through
	// setFolioProperty()'s own branch (embedTitleBlockTemplate() first)
	// rather than this direct field lookup.
	return nullptr;
}
const QStringList &titleBlockFieldNames()
{
	static const QStringList n{QStringLiteral("title"), QStringLiteral("author"),
		QStringLiteral("filename"), QStringLiteral("plant"), QStringLiteral("locmach"),
		QStringLiteral("indexrev"), QStringLiteral("folio"), QStringLiteral("template")};
	return n;
}
} // namespace

QString QetScriptApi::folioProperty(int folioIndex, const QString &property) const
{
	if (!m_project) return QString();
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return QString();
	Diagram *diagram = diagrams.at(folioIndex);
	if (property == QLatin1String("template"))
		return diagram->border_and_titleblock.titleBlockTemplateName();
	TitleBlockProperties p = diagram->border_and_titleblock.exportTitleBlock();
	QString *field = titleBlockField(p, property);
	return field ? *field : QString();
}

/**
	@brief QetScriptApi::setFolioProperty
	Set one text field of a folio's title block (title, author, filename,
	plant, locmach, indexrev, folio) via ChangeTitleBlockCommand,
	like setFolioTitle() which this generalises. The date and the template are
	not offered: the date has a use-current-date mode that a plain string
	cannot express honestly.
*/
bool QetScriptApi::setFolioProperty(int folioIndex, const QString &property, const QString &value)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.setFolioProperty: project is read-only"));
		return false;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return false;
	Diagram *diagram = diagrams.at(folioIndex);

	// Not a TitleBlockProperties field like the others below: the template
	// is named by whatever the project's embedded collection calls it, not
	// by a value stored on this folio's own properties, so it has to be
	// embedded (or already present) before Diagram::setTitleBlockTemplate()
	// -- the same public slot BorderTitleBlock's own needTitleBlockTemplate
	// signal calls -- can find it.
	if (property == QLatin1String("template")) {
		// BorderTitleBlock::titleBlockTemplateName() normalises a template
		// literally named "default" back to "" -- indistinguishable, once
		// set, from no override at all (a template named "default" ships
		// in the common collection and is genuinely what "no override"
		// renders with). Compare against that same normalised form, or a
		// script setting "default" would see this report failure although
		// the application applied it correctly -- measured: it did.
		const QString normalised = (value == QLatin1String("default")) ? QString() : value;
		if (diagram->border_and_titleblock.titleBlockTemplateName() == normalised) return true;
		if (!embedTitleBlockTemplate(value)) return false;
		diagram->setTitleBlockTemplate(value);
		return diagram->border_and_titleblock.titleBlockTemplateName() == normalised;
	}

	TitleBlockProperties old_p = diagram->border_and_titleblock.exportTitleBlock();
	TitleBlockProperties new_p = old_p;
	QString *field = titleBlockField(new_p, property);
	if (!field) {
		log(QStringLiteral("qet.setFolioProperty: unknown property '%1'; expected one of %2")
			.arg(property, titleBlockFieldNames().join(QStringLiteral(", "))));
		return false;
	}
	if (*field == value) return true;
	*field = value;
	m_project->undoStack()->push(new ChangeTitleBlockCommand(diagram, old_p, new_p));
	return true;
}

QStringList QetScriptApi::terminalStrips() const
{
	QStringList list;
	if (!m_project) return list;
	const QVector<TerminalStrip *> strips = m_project->terminalStrip();
	for (int i = 0 ; i < strips.count() ; ++i)
	{
		TerminalStrip *t = strips.at(i);
		list << QStringLiteral("%1: installation='%2' location='%3' name='%4' (%5 terminal(s))")
				.arg(i).arg(t->installation(), t->location(), t->name())
				.arg(t->realTerminals().count());
	}
	return list;
}

/**
	@brief QetScriptApi::addTerminalStrip
	Create an empty terminal strip through AddTerminalStripCommand, as the
	editor's creation dialog does after it is accepted.
	@return its index in terminalStrips(), or -1
*/
int QetScriptApi::addTerminalStrip(const QString &installation, const QString &location,
								   const QString &name)
{
	if (!m_project) return -1;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.addTerminalStrip: project is read-only"));
		return -1;
	}
	auto *strip = new TerminalStrip(installation, location, name, m_project);
	m_project->undoStack()->push(new AddTerminalStripCommand(strip, m_project));
	return m_project->terminalStrip().indexOf(strip);
}

bool QetScriptApi::removeTerminalStrip(int stripIndex)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.removeTerminalStrip: project is read-only"));
		return false;
	}
	const QVector<TerminalStrip *> strips = m_project->terminalStrip();
	if (stripIndex < 0 || stripIndex >= strips.count()) {
		log(QStringLiteral("qet.removeTerminalStrip: no strip at index %1").arg(stripIndex));
		return false;
	}
	m_project->undoStack()->push(new RemoveTerminalStripCommand(strips.at(stripIndex), m_project));
	return m_project->terminalStrip().count() == strips.count() - 1;
}

bool QetScriptApi::addTerminalToStrip(int stripIndex, int folioIndex, const QString &elementUuid)
{
	if (!m_project) return false;
	const QString caller = QStringLiteral("addTerminalToStrip");
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return false;
	}
	const QVector<TerminalStrip *> strips = m_project->terminalStrip();
	if (stripIndex < 0 || stripIndex >= strips.count()) {
		log(QStringLiteral("qet.%1: no strip at index %2").arg(caller).arg(stripIndex));
		return false;
	}
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return false;
	auto *terminal_element = qobject_cast<TerminalElement *>(element);
	if (!terminal_element) {
		log(QStringLiteral("qet.%1: %2 is not a terminal-type element").arg(caller, elementUuid));
		return false;
	}
	QSharedPointer<RealTerminal> real = terminal_element->realTerminal();
	if (!real) return false;
	if (real->parentStrip()) {
		log(QStringLiteral("qet.%1: %2 already belongs to a strip").arg(caller, elementUuid));
		return false;
	}
	m_project->undoStack()->push(new AddTerminalToStripCommand(real, strips.at(stripIndex)));
	return real->parentStrip() == strips.at(stripIndex);
}

/**
	@brief QetScriptApi::stripRealTerminals
	The strip's real terminals -- the wire-ends added by addTerminalToStrip()
	-- one line per index: the owning element's uuid, the terminal's own
	name, and which physical position (clamp) it currently sits on, by that
	position's own index (so several real terminals reporting the same
	physical index are already grouped together).
*/
QStringList QetScriptApi::stripRealTerminals(int stripIndex) const
{
	QStringList list;
	if (!m_project) return list;
	const QVector<TerminalStrip *> strips = m_project->terminalStrip();
	if (stripIndex < 0 || stripIndex >= strips.count()) return list;
	TerminalStrip *strip = strips.at(stripIndex);

	const QVector<QSharedPointer<PhysicalTerminal>> physical = strip->physicalTerminal();
	const QVector<QSharedPointer<RealTerminal>> real = strip->realTerminals();
	for (int i = 0 ; i < real.count() ; ++i)
	{
		QSharedPointer<RealTerminal> rt = real.at(i);
		QSharedPointer<PhysicalTerminal> pt = rt->physicalTerminal();
		const int physical_index = pt ? physical.indexOf(pt) : -1;
		list << QStringLiteral("%1: %2 terminal '%3', physical position %4 (%5 terminal(s) there)")
				.arg(i)
				.arg(rt->element() ? rt->element()->uuid().toString() : QStringLiteral("?"))
				.arg(rt->label())
				.arg(physical_index)
				.arg(pt ? pt->realTerminalCount() : 0);
	}
	return list;
}

namespace {
/**
	Resolve a list of indices into stripRealTerminals() to the RealTerminal
	objects groupTerminals()/bridgeTerminals() need, or an empty (and
	therefore refusable) list if any index is out of range or the list has
	fewer than the two terminals either operation requires.
*/
QVector<QSharedPointer<RealTerminal>> resolveRealTerminals(
		TerminalStrip *strip, const QVariantList &indices, const QString &caller,
		QetScriptApi *api)
{
	QVector<QSharedPointer<RealTerminal>> out;
	const QVector<QSharedPointer<RealTerminal>> all = strip->realTerminals();
	if (indices.count() < 2) {
		api->log(QStringLiteral("qet.%1: at least two real terminals are required").arg(caller));
		return {};
	}
	for (const QVariant &v : indices) {
		bool ok = false;
		const int i = v.toInt(&ok);
		if (!ok || i < 0 || i >= all.count()) {
			api->log(QStringLiteral("qet.%1: %2 is not a valid real terminal index (strip has %3)")
				.arg(caller, v.toString()).arg(all.count()));
			return {};
		}
		out << all.at(i);
	}
	return out;
}
} // namespace

/**
	@brief QetScriptApi::groupTerminals
	Merge several real terminals onto one physical position, through
	GroupTerminalsCommand exactly as the terminal strip editor's "group"
	button does -- including which position receives the others: the one
	among those named that already carries the most real terminals, the
	same heuristic the editor applies, not necessarily the first one given.
*/
bool QetScriptApi::groupTerminals(int stripIndex, const QVariantList &realTerminalIndices)
{
	if (!m_project) return false;
	const QString caller = QStringLiteral("groupTerminals");
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return false;
	}
	const QVector<TerminalStrip *> strips = m_project->terminalStrip();
	if (stripIndex < 0 || stripIndex >= strips.count()) {
		log(QStringLiteral("qet.%1: no strip at index %2").arg(caller).arg(stripIndex));
		return false;
	}
	TerminalStrip *strip = strips.at(stripIndex);
	const QVector<QSharedPointer<RealTerminal>> chosen = resolveRealTerminals(strip, realTerminalIndices, caller, this);
	if (chosen.isEmpty()) return false;

	QSharedPointer<PhysicalTerminal> receiver = chosen.first()->physicalTerminal();
	int best_count = 0;
	for (const QSharedPointer<RealTerminal> &rt : chosen) {
		QSharedPointer<PhysicalTerminal> pt = rt->physicalTerminal();
		const int count = pt ? pt->realTerminalCount() : 0;
		if (count > 1 && count > best_count) {
			best_count = count;
			receiver = pt;
		}
	}
	if (!receiver) {
		log(QStringLiteral("qet.%1: no physical position to receive the group").arg(caller));
		return false;
	}

	QVector<QSharedPointer<RealTerminal>> to_group = chosen;
	for (const QSharedPointer<RealTerminal> &rt : receiver->realTerminals()) {
		to_group.removeOne(rt);
	}
	if (to_group.isEmpty()) {
		log(QStringLiteral("qet.%1: every named terminal is already on the receiving position").arg(caller));
		return true;
	}
	const int before = strip->physicalTerminalCount();
	m_project->undoStack()->push(new GroupTerminalsCommand(strip, receiver, to_group));
	return strip->physicalTerminalCount() < before;
}

/**
	@brief QetScriptApi::bridgeTerminals
	Wire several real terminals together electrically, through
	BridgeTerminalsCommand as the editor's "bridge" button does. Refused,
	via TerminalStrip::isBridgeable(), when they are not all at the same
	level -- the editor's own check, not a rule reimplemented here.
*/
bool QetScriptApi::bridgeTerminals(int stripIndex, const QVariantList &realTerminalIndices)
{
	if (!m_project) return false;
	const QString caller = QStringLiteral("bridgeTerminals");
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return false;
	}
	const QVector<TerminalStrip *> strips = m_project->terminalStrip();
	if (stripIndex < 0 || stripIndex >= strips.count()) {
		log(QStringLiteral("qet.%1: no strip at index %2").arg(caller).arg(stripIndex));
		return false;
	}
	TerminalStrip *strip = strips.at(stripIndex);
	const QVector<QSharedPointer<RealTerminal>> chosen = resolveRealTerminals(strip, realTerminalIndices, caller, this);
	if (chosen.isEmpty()) return false;

	if (!strip->isBridgeable(chosen)) {
		log(QStringLiteral("qet.%1: these terminals cannot be bridged -- they are not all at the same level")
			.arg(caller));
		return false;
	}
	m_project->undoStack()->push(new BridgeTerminalsCommand(strip, chosen));
	return true;
}

/**
	@brief QetScriptApi::sortTerminalStrip
	Reorder a strip's physical positions into the canonical order the
	editor's own "sort" button computes, through SortTerminalStripCommand.
*/
bool QetScriptApi::sortTerminalStrip(int stripIndex)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.sortTerminalStrip: project is read-only"));
		return false;
	}
	const QVector<TerminalStrip *> strips = m_project->terminalStrip();
	if (stripIndex < 0 || stripIndex >= strips.count()) {
		log(QStringLiteral("qet.sortTerminalStrip: no strip at index %1").arg(stripIndex));
		return false;
	}
	m_project->undoStack()->push(new SortTerminalStripCommand(strips.at(stripIndex)));
	return true;
}

QList<QetGraphicsTableItem *> QetScriptApi::sortedTables(int folioIndex) const
{
	if (!m_project) return {};
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return {};
	DiagramContent content(diagrams.at(folioIndex), false);
	return sortedByPosition(content.m_tables);
}

QStringList QetScriptApi::tables(int folioIndex) const
{
	QStringList list;
	const QList<QetGraphicsTableItem *> all = sortedTables(folioIndex);
	for (int i = 0 ; i < all.count() ; ++i)
	{
		QetGraphicsTableItem *t = all.at(i);
		list << QStringLiteral("%1: '%2' at (%3, %4), %5 row(s)")
				.arg(i).arg(t->tableName()).arg(t->pos().x()).arg(t->pos().y())
				.arg(t->model() ? t->model()->rowCount() : 0);
	}
	return list;
}

/**
	@brief QetScriptApi::addTable
	Place a BOM/nomenclature or summary (table of contents) table, through
	QetGraphicsTableFactory::create() -- the same factory call the "add
	table" menu action makes, minus the modal AddTableDialog it collects
	its settings from first. That dialog is built here too, off-screen and
	never shown or exec'd: create() reads the table's name and the query
	widget's identifier/query string from it rather than taking them as
	plain arguments, so the dialog exists only to be read from, and its two
	checkboxes -- "adjust to folio" and "add a new folio if the table
	overflows" -- are forced off despite defaulting to checked in the .ui
	file, so one call creates exactly the one table asked for. A script
	that wants either behaviour can resize the result or add a folio itself.

	kind is "nomenclature" (an ElementQueryWidget, over placed elements) or
	"summary" (a SummaryQueryWidget, over folios); query is required, since
	both widgets otherwise build their own from a set of checkboxes that
	default to none checked, and "SELECT with no columns" is not a useful
	table -- query() against element_nomenclature_view or
	project_summary_view is the way to find one that is.

	Not undoable: newTable(), which create() calls, calls
	Diagram::addItem() directly, with no undo command of its own, in the
	stock action as much as here. Which of the (possibly several) tables
	create() left in the diagram is the new one is found by set difference
	against the folio's table listing taken just before the call, since
	create()'s return type is void and newTable() itself is private.
	@return the table's index in tables(), or -1
*/
int QetScriptApi::addTable(int folioIndex, const QString &kind, const QString &name,
						   const QString &query)
{
	if (!m_project) return -1;
	const QString caller = QStringLiteral("addTable");
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return -1;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) {
		log(QStringLiteral("qet.%1: no folio at index %2").arg(caller).arg(folioIndex));
		return -1;
	}
	if (query.isEmpty()) {
		// Both widgets build their query from a set of checkboxes that
		// default to none checked, so "no query" is not "the sensible
		// default" here the way it might look -- it is SELECT with no
		// columns. Measured: a table left to that default reports 0 rows
		// against a folio that plainly has some. A real SELECT is required
		// instead, over query() -- the same project database and the same
		// two views this project already exposes, element_nomenclature_view
		// for a nomenclature table and project_summary_view for a summary.
		log(QStringLiteral("qet.%1: a query is required -- try qet.query() against "
						   "element_nomenclature_view or project_summary_view first "
						   "to find one that returns what is wanted").arg(caller));
		return -1;
	}

	QWidget *content = nullptr;
	if (kind == QLatin1String("nomenclature")) {
		auto *w = new ElementQueryWidget();
		w->setQuery(query);
		content = w;
	} else if (kind == QLatin1String("summary")) {
		auto *w = new SummaryQueryWidget();
		w->setQuery(query);
		content = w;
	} else {
		log(QStringLiteral("qet.%1: unknown kind '%2'; expected nomenclature or summary").arg(caller, kind));
		return -1;
	}

	AddTableDialog dialog(content);
	dialog.setTableName(name);
	// QetGraphicsTableFactory::newTable() is private -- only create() (its
	// own class) may call it -- and create()'s own two checkboxes both
	// default to checked in the .ui file: "adjust to folio" and "add a new
	// folio if the table overflows". Forced off here rather than left at
	// that default, since a script calling addTable() once should create
	// exactly the one table it asked for, not possibly several across
	// folios it never asked to add.
	dialog.setAdjustTableToFolio(false);
	dialog.setAddNewTableToNewDiagram(false);

	const QList<QetGraphicsTableItem *> before = sortedTables(folioIndex);
	QetGraphicsTableFactory::create(diagrams.at(folioIndex), &dialog);
	const QList<QetGraphicsTableItem *> after = sortedTables(folioIndex);
	for (QetGraphicsTableItem *t : after) {
		if (!before.contains(t)) return after.indexOf(t);
	}
	log(QStringLiteral("qet.%1: the table could not be created").arg(caller));
	return -1;
}

bool QetScriptApi::deleteTable(int folioIndex, int tableIndex)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.deleteTable: project is read-only"));
		return false;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return false;
	Diagram *diagram = diagrams.at(folioIndex);

	const QList<QetGraphicsTableItem *> all = sortedTables(folioIndex);
	if (tableIndex < 0 || tableIndex >= all.count()) {
		log(QStringLiteral("qet.deleteTable: folio %1 has %2 table(s), no index %3")
			.arg(folioIndex).arg(all.count()).arg(tableIndex));
		return false;
	}
	DiagramContent to_remove;
	to_remove.m_tables << all.at(tableIndex);
	diagram->undoStack().push(new DeleteQGraphicsItemCommand(diagram, to_remove));
	return true;
}

/**
	@brief QetScriptApi::setTablePosition
	Move a table on its folio through QPropertyUndoCommand, the same
	mechanism setElementPosition() uses -- QetGraphicsTableFactory::newTable()
	places every new table at a fixed (50, 50), so a script adding more than
	one table must reposition all but the first itself or they stack exactly
	on top of each other.
*/
bool QetScriptApi::setTablePosition(int folioIndex, int tableIndex, double x, double y)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.setTablePosition: project is read-only"));
		return false;
	}
	const QList<QetGraphicsTableItem *> all = sortedTables(folioIndex);
	if (tableIndex < 0 || tableIndex >= all.count()) {
		log(QStringLiteral("qet.setTablePosition: folio %1 has %2 table(s), no index %3")
			.arg(folioIndex).arg(all.count()).arg(tableIndex));
		return false;
	}
	QetGraphicsTableItem *table = all.at(tableIndex);

	const QVariant old_value = table->pos();
	const QVariant new_value = QPointF(x, y);
	if (old_value == new_value) return true; // already there; nothing to push

	auto *cmd = new QPropertyUndoCommand(table, "pos", old_value, new_value);
	cmd->setText(QObject::tr("Déplacer %1").arg(table->tableName()));
	m_project->undoStack()->push(cmd);
	return true;
}

namespace {
QHash<QString, NumerotationContext> autoNumTable(QETProject *project, const QString &kind, bool *ok)
{
	*ok = true;
	if (kind == QLatin1String("conductor")) return project->conductorAutoNum();
	if (kind == QLatin1String("element"))   return project->elementAutoNum();
	if (kind == QLatin1String("folio"))     return project->folioAutoNum();
	*ok = false;
	return {};
}
} // namespace

QStringList QetScriptApi::autoNums(const QString &kind) const
{
	QStringList list;
	if (!m_project) return list;
	bool ok;
	const QHash<QString, NumerotationContext> table = autoNumTable(m_project, kind, &ok);
	if (!ok) return list;
	const QStringList names = table.keys();
	for (const QString &name : names) {
		list << QStringLiteral("%1: formula='%2'")
				.arg(name, autonum::numerotationContextToFormula(table.value(name)));
	}
	list.sort();
	return list;
}

/**
	@brief QetScriptApi::addAutoNum
	Define (or replace) a named numbering context. Each part is
	"type[:value[:increase]]"; a numeric type takes its starting value and
	how much it advances by, a text type its text. Anything the
	NumerotationContext rejects -- an unknown type, a non-numeric value for
	a numeric type -- is refused rather than dropped.
*/
bool QetScriptApi::addAutoNum(const QString &kind, const QString &name, const QStringList &parts)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.addAutoNum: project is read-only"));
		return false;
	}
	bool ok;
	autoNumTable(m_project, kind, &ok);
	if (!ok) {
		log(QStringLiteral("qet.addAutoNum: unknown kind '%1'; expected conductor, element or folio").arg(kind));
		return false;
	}
	if (name.isEmpty() || parts.isEmpty()) {
		log(QStringLiteral("qet.addAutoNum: a name and at least one part are required"));
		return false;
	}

	NumerotationContext context;
	for (const QString &part : parts)
	{
		const QStringList f = part.split(QLatin1Char(':'));
		const QString type = f.value(0);
		const QVariant value = f.size() > 1 ? QVariant(f.at(1)) : QVariant(1);
		bool inc_ok = true;
		const int increase = f.size() > 2 ? f.at(2).toInt(&inc_ok) : 1;
		if (!context.keyIsAcceptable(type) || !inc_ok || !context.addValue(type, value, increase)) {
			log(QStringLiteral("qet.addAutoNum: cannot use part '%1'").arg(part));
			return false;
		}
	}

	if (kind == QLatin1String("conductor"))     m_project->addConductorAutoNum(name, context);
	else if (kind == QLatin1String("element"))  m_project->addElementAutoNum(name, context);
	else                                        m_project->addFolioAutoNum(name, context);
	return true;
}

bool QetScriptApi::removeAutoNum(const QString &kind, const QString &name)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.removeAutoNum: project is read-only"));
		return false;
	}
	bool ok;
	const QHash<QString, NumerotationContext> table = autoNumTable(m_project, kind, &ok);
	if (!ok || !table.contains(name)) {
		log(QStringLiteral("qet.removeAutoNum: no %1 auto-numbering named '%2'").arg(kind, name));
		return false;
	}
	if (kind == QLatin1String("conductor"))     m_project->removeConductorAutoNum(name);
	else if (kind == QLatin1String("element"))  m_project->removeElementAutoNum(name);
	else                                        m_project->removeFolioAutoNum(name);
	return true;
}

/**
	@brief QetScriptApi::useConductorAutoNum
	Make new conductors on a folio take their number from a named context.
	Sets both what the folio reads and the project's current name, because
	ConductorAutoNumerotation reads the context by the former and writes the
	advanced counter back under the latter.
*/
bool QetScriptApi::useConductorAutoNum(int folioIndex, const QString &name)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.useConductorAutoNum: project is read-only"));
		return false;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return false;
	if (!name.isEmpty() && !m_project->conductorAutoNum().contains(name)) {
		log(QStringLiteral("qet.useConductorAutoNum: no conductor auto-numbering named '%1'").arg(name));
		return false;
	}
	diagrams.at(folioIndex)->setConductorsAutonumName(name);
	m_project->setCurrentConductorAutoNum(name);
	return true;
}

QList<DiagramImageItem *> QetScriptApi::sortedImages(int folioIndex) const
{
	if (!m_project) return {};
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return {};
	DiagramContent content(diagrams.at(folioIndex), false);
	return sortedByPosition(content.m_images);
}

QStringList QetScriptApi::images(int folioIndex) const
{
	QStringList out;
	const QList<DiagramImageItem *> list = sortedImages(folioIndex);
	for (int i = 0 ; i < list.count() ; ++i)
	{
		DiagramImageItem *item = list.at(i);
		const QPixmap px = item->pixmap();
		const QPointF at = item->sceneBoundingRect().topLeft();
		out << QStringLiteral("%1: %2x%3 px at (%4, %5) scale=%6 rotation=%7")
				.arg(i).arg(px.width()).arg(px.height()).arg(at.x()).arg(at.y())
				.arg(item->scaleFactorX()).arg(item->rotationAngle());
	}
	return out;
}

/**
	@brief QetScriptApi::addImage
	Place a picture from a file, as the "add image" tool does after its file
	dialog. The pixels are copied into the project (DiagramImageItem::toXml
	writes them inline), so the file need not exist afterwards.
	@return the image's index in images(), or -1
*/
int QetScriptApi::addImage(int folioIndex, const QString &filePath, double x, double y)
{
	if (!m_project) return -1;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.addImage: project is read-only"));
		return -1;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return -1;

	const QFileInfo info(filePath);
	if (!info.isFile()) {
		log(QStringLiteral("qet.addImage: '%1' is not a file").arg(filePath));
		return -1;
	}
	constexpr qint64 max_bytes = 10LL * 1024 * 1024;
	if (info.size() > max_bytes) {
		log(QStringLiteral("qet.addImage: '%1' is %2 bytes; images are embedded in the project, "
						   "so files over 10 MB are refused").arg(filePath).arg(info.size()));
		return -1;
	}
	const QImage image(filePath);
	if (image.isNull()) {
		log(QStringLiteral("qet.addImage: '%1' could not be read as an image").arg(filePath));
		return -1;
	}

	Diagram *diagram = diagrams.at(folioIndex);
	auto *item = new DiagramImageItem(QPixmap::fromImage(image));
	diagram->undoStack().push(new AddGraphicsObjectCommand(item, diagram, QPointF(x, y)));
	return sortedImages(folioIndex).indexOf(item);
}

bool QetScriptApi::setImageScale(int folioIndex, int imageIndex, double factor)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.setImageScale: project is read-only"));
		return false;
	}
	if (factor <= 0) {
		log(QStringLiteral("qet.setImageScale: the factor must be positive"));
		return false;
	}
	const QList<DiagramImageItem *> list = sortedImages(folioIndex);
	if (imageIndex < 0 || imageIndex >= list.count()) {
		log(QStringLiteral("qet.setImageScale: folio %1 has %2 image(s), no index %3")
			.arg(folioIndex).arg(list.count()).arg(imageIndex));
		return false;
	}
	DiagramImageItem *item = list.at(imageIndex);
	if (item->scaleFactorX() == factor && item->scaleFactorY() == factor) return true;

	// Both axes, one undo step: a script that scales an image means the
	// image, not one axis of it.
	m_project->undoStack()->beginMacro(QObject::tr("Redimensionner une image"));
	m_project->undoStack()->push(new QPropertyUndoCommand(item, "scaleFactorX",
								 QVariant(item->scaleFactorX()), QVariant(factor)));
	m_project->undoStack()->push(new QPropertyUndoCommand(item, "scaleFactorY",
								 QVariant(item->scaleFactorY()), QVariant(factor)));
	m_project->undoStack()->endMacro();
	return true;
}

bool QetScriptApi::setImageRotation(int folioIndex, int imageIndex, double angle)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.setImageRotation: project is read-only"));
		return false;
	}
	const QList<DiagramImageItem *> list = sortedImages(folioIndex);
	if (imageIndex < 0 || imageIndex >= list.count()) {
		log(QStringLiteral("qet.setImageRotation: folio %1 has %2 image(s), no index %3")
			.arg(folioIndex).arg(list.count()).arg(imageIndex));
		return false;
	}
	DiagramImageItem *item = list.at(imageIndex);
	if (item->rotationAngle() == angle) return true;
	auto *cmd = new QPropertyUndoCommand(item, "rotationAngle",
										 QVariant(item->rotationAngle()), QVariant(angle));
	cmd->setText(QObject::tr("Pivoter une image"));
	m_project->undoStack()->push(cmd);
	return true;
}

bool QetScriptApi::deleteImage(int folioIndex, int imageIndex)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.deleteImage: project is read-only"));
		return false;
	}
	const QList<DiagramImageItem *> list = sortedImages(folioIndex);
	if (imageIndex < 0 || imageIndex >= list.count()) {
		log(QStringLiteral("qet.deleteImage: folio %1 has %2 image(s), no index %3")
			.arg(folioIndex).arg(list.count()).arg(imageIndex));
		return false;
	}
	Diagram *diagram = m_project->diagrams().at(folioIndex);
	DiagramContent content;
	content.m_images << list.at(imageIndex);
	diagram->undoStack().push(new DeleteQGraphicsItemCommand(diagram, content));
	return true;
}

/**
	@brief QetScriptApi::addPdfPage
	Render one page of a PDF file to an image and place it, as the "add
	PDF" toolbar action does after its file and page-selection dialogs --
	same QPdfDocument::render() call, same white-background compositing
	for a transparent PDF, same DiagramImageItem/AddGraphicsObjectCommand
	underneath addImage() itself. Only reachable in a build with the
	QtPdf module (Qt >= 6.4); refused with a clear reason otherwise, since
	a missing module or a too-old Qt is a real possibility this project
	ships around (see diagrameventaddpdf.h) rather than something a script
	should read as "no such method".
	@param pageNumber 1-based, as PdfPagesDialog shows it
	@param dpi resolution to render at; the GUI dialog defaults to 150
	@return the new image's index in images(), or -1
*/
int QetScriptApi::addPdfPage(int folioIndex, const QString &pdfPath, int pageNumber,
							 int dpi, double x, double y)
{
#ifndef QET_HAS_QTPDF
	Q_UNUSED(folioIndex) Q_UNUSED(pdfPath) Q_UNUSED(pageNumber)
	Q_UNUSED(dpi) Q_UNUSED(x) Q_UNUSED(y)
	log(QStringLiteral("qet.addPdfPage: this build has no QtPdf module (or Qt < 6.4); "
					   "PDF page import is unavailable"));
	return -1;
#else
	if (!m_project) return -1;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.addPdfPage: project is read-only"));
		return -1;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return -1;
	if (dpi <= 0) {
		log(QStringLiteral("qet.addPdfPage: dpi must be positive, got %1").arg(dpi));
		return -1;
	}
	const QFileInfo info(pdfPath);
	if (!info.isFile()) {
		log(QStringLiteral("qet.addPdfPage: '%1' is not a file").arg(pdfPath));
		return -1;
	}

	QPdfDocument document;
	document.load(pdfPath);
	if (document.status() != QPdfDocument::Status::Ready) {
		log(QStringLiteral("qet.addPdfPage: '%1' could not be loaded as a PDF").arg(pdfPath));
		return -1;
	}
	const int pageCount = document.pageCount();
	if (pageNumber < 1 || pageNumber > pageCount) {
		log(QStringLiteral("qet.addPdfPage: '%1' has %2 page(s), no page %3")
			.arg(pdfPath).arg(pageCount).arg(pageNumber));
		return -1;
	}
	const int pageIndex = pageNumber - 1;

	// PDF point = 1/72 inch, same conversion PdfPagesDialog applies.
	const QSizeF pageSize = document.pagePointSize(pageIndex);
	const int pixelWidth = qRound((pageSize.width() / 72.0) * dpi);
	const int pixelHeight = qRound((pageSize.height() / 72.0) * dpi);
	if (pixelWidth <= 0 || pixelHeight <= 0) {
		log(QStringLiteral("qet.addPdfPage: could not determine page %1's size").arg(pageNumber));
		return -1;
	}

	const QImage rendered = document.render(pageIndex, QSize(pixelWidth, pixelHeight));
	if (rendered.isNull()) {
		log(QStringLiteral("qet.addPdfPage: page %1 could not be rendered").arg(pageNumber));
		return -1;
	}

	// A transparent PDF page would otherwise composite onto whatever is
	// under it on the folio, unlike every other placed image.
	QImage background(rendered.size(), QImage::Format_ARGB32_Premultiplied);
	background.fill(Qt::white);
	QPainter painter(&background);
	painter.drawImage(0, 0, rendered);
	painter.end();

	Diagram *diagram = diagrams.at(folioIndex);
	auto *item = new DiagramImageItem(QPixmap::fromImage(background));
	diagram->undoStack().push(new AddGraphicsObjectCommand(item, diagram, QPointF(x, y)));
	return sortedImages(folioIndex).indexOf(item);
#endif
}

namespace {
QString textSourceName(DynamicElementTextItem::TextFrom from)
{
	switch (from) {
		case DynamicElementTextItem::ElementInfo:   return QStringLiteral("info");
		case DynamicElementTextItem::CompositeText: return QStringLiteral("composite");
		default:                                    return QStringLiteral("text");
	}
}

const QStringList &elementTextPropertyNames()
{
	static const QStringList n{QStringLiteral("text"), QStringLiteral("source"), QStringLiteral("info"),
		QStringLiteral("composite"), QStringLiteral("frame"), QStringLiteral("size"),
		QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("rotation"), QStringLiteral("width")};
	return n;
}
} // namespace

DynamicElementTextItem *QetScriptApi::findElementText(int folioIndex, const QString &elementUuid,
													  int textIndex, const QString &caller) const
{
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return nullptr;
	const QList<DynamicElementTextItem *> list = element->dynamicTextItems();
	if (textIndex < 0 || textIndex >= list.count()) {
		const_cast<QetScriptApi *>(this)->log(
			QStringLiteral("qet.%1: %2 has %3 text field(s), no index %4")
				.arg(caller, element->name()).arg(list.count()).arg(textIndex));
		return nullptr;
	}
	return list.at(textIndex);
}

QStringList QetScriptApi::elementTexts(int folioIndex, const QString &elementUuid) const
{
	QStringList out;
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return out;
	const QList<DynamicElementTextItem *> list = element->dynamicTextItems();
	for (int i = 0 ; i < list.count() ; ++i)
	{
		DynamicElementTextItem *t = list.at(i);
		const QString source = textSourceName(t->textFrom());
		QString what;
		if (t->textFrom() == DynamicElementTextItem::ElementInfo) what = QStringLiteral(" info='%1'").arg(t->infoName());
		else if (t->textFrom() == DynamicElementTextItem::CompositeText) what = QStringLiteral(" composite='%1'").arg(t->compositeText());
		out << QStringLiteral("%1: source=%2%3 shows='%4' at (%5, %6) size=%7")
				.arg(i).arg(source, what, t->toPlainText())
				.arg(t->pos().x()).arg(t->pos().y()).arg(t->font().pointSizeF());
	}
	return out;
}

QString QetScriptApi::elementTextProperty(int folioIndex, const QString &elementUuid,
										  int textIndex, const QString &property) const
{
	DynamicElementTextItem *t = findElementText(folioIndex, elementUuid, textIndex,
												QStringLiteral("elementTextProperty"));
	if (!t) return QString();
	if (property == QLatin1String("text"))      return t->text();
	if (property == QLatin1String("shows"))     return t->toPlainText();
	if (property == QLatin1String("source"))    return textSourceName(t->textFrom());
	if (property == QLatin1String("info"))      return t->infoName();
	if (property == QLatin1String("composite")) return t->compositeText();
	if (property == QLatin1String("frame"))     return t->frame() ? QStringLiteral("true") : QStringLiteral("false");
	if (property == QLatin1String("size"))      return QString::number(t->font().pointSizeF());
	if (property == QLatin1String("x"))         return QString::number(t->pos().x());
	if (property == QLatin1String("y"))         return QString::number(t->pos().y());
	if (property == QLatin1String("rotation"))  return QString::number(t->rotation());
	if (property == QLatin1String("width"))     return QString::number(t->textWidth());
	return QString();
}

/**
	@brief QetScriptApi::addElementText
	Add a text field to a symbol, through AddElementTextCommand as the
	element-texts editor does.
	@param source "text" (value is the string shown), "info" (value is an
	information key such as "label", and the field then follows that key) or
	"composite" (value is a formula)
	@return the field's index in elementTexts(), or -1
*/
int QetScriptApi::addElementText(int folioIndex, const QString &elementUuid,
								 const QString &source, const QString &value,
								 double x, double y)
{
	if (!m_project) return -1;
	const QString caller = QStringLiteral("addElementText");
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return -1;
	}
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return -1;

	auto *item = new DynamicElementTextItem(element);
	if (source == QLatin1String("text")) {
		item->setTextFrom(DynamicElementTextItem::UserText);
		item->setText(value);
	} else if (source == QLatin1String("info")) {
		if (!QETInformation::elementInfoKeys().contains(value)) {
			log(QStringLiteral("qet.%1: '%2' is not an element information key").arg(caller, value));
			delete item;
			return -1;
		}
		item->setTextFrom(DynamicElementTextItem::ElementInfo);
		item->setInfoName(value);
	} else if (source == QLatin1String("composite")) {
		item->setTextFrom(DynamicElementTextItem::CompositeText);
		item->setCompositeText(value);
	} else {
		log(QStringLiteral("qet.%1: unknown source '%2'; expected text, info or composite").arg(caller, source));
		delete item;
		return -1;
	}
	item->setPos(x, y);
	m_project->undoStack()->push(new AddElementTextCommand(element, item));
	return element->dynamicTextItems().indexOf(item);
}

/**
	@brief QetScriptApi::setElementTextProperty
	Change one aspect of a symbol's text field through QPropertyUndoCommand on
	the item's own properties, the way the element-texts editor does.
	Properties: text, source (text|info|composite), info, composite, frame
	(true|false), size (points), x, y (in the element's coordinates),
	rotation, width.
*/
bool QetScriptApi::setElementTextProperty(int folioIndex, const QString &elementUuid,
										  int textIndex, const QString &property,
										  const QString &value)
{
	if (!m_project) return false;
	const QString caller = QStringLiteral("setElementTextProperty");
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return false;
	}
	if (!elementTextPropertyNames().contains(property)) {
		log(QStringLiteral("qet.%1: unknown property '%2'; expected one of %3")
			.arg(caller, property, elementTextPropertyNames().join(QStringLiteral(", "))));
		return false;
	}
	DynamicElementTextItem *t = findElementText(folioIndex, elementUuid, textIndex, caller);
	if (!t) return false;

	auto number = [&](double &out, bool positive) {
		bool ok = false;
		out = value.toDouble(&ok);
		if (!ok || (positive && out <= 0)) {
			log(QStringLiteral("qet.%1: '%2' is not a valid %3").arg(caller, value, property));
			return false;
		}
		return true;
	};

	const char *qt_property = nullptr;
	QVariant old_value, new_value;
	double d = 0;

	if (property == QLatin1String("text"))            { qt_property = "text"; old_value = t->text(); new_value = value; }
	else if (property == QLatin1String("info")) {
		if (!QETInformation::elementInfoKeys().contains(value)) {
			log(QStringLiteral("qet.%1: '%2' is not an element information key").arg(caller, value));
			return false;
		}
		qt_property = "infoName"; old_value = t->infoName(); new_value = value;
	}
	else if (property == QLatin1String("composite"))  { qt_property = "compositeText"; old_value = t->compositeText(); new_value = value; }
	else if (property == QLatin1String("source")) {
		DynamicElementTextItem::TextFrom from;
		if (value == QLatin1String("text"))           from = DynamicElementTextItem::UserText;
		else if (value == QLatin1String("info"))      from = DynamicElementTextItem::ElementInfo;
		else if (value == QLatin1String("composite")) from = DynamicElementTextItem::CompositeText;
		else { log(QStringLiteral("qet.%1: unknown source '%2'").arg(caller, value)); return false; }
		qt_property = "textFrom"; old_value = QVariant::fromValue(t->textFrom()); new_value = QVariant::fromValue(from);
	}
	else if (property == QLatin1String("frame")) {
		const QString v = value.toLower();
		if (v != QLatin1String("true") && v != QLatin1String("false")) {
			log(QStringLiteral("qet.%1: frame is true or false, not '%2'").arg(caller, value));
			return false;
		}
		qt_property = "frame"; old_value = t->frame(); new_value = (v == QLatin1String("true"));
	}
	else if (property == QLatin1String("size")) {
		if (!number(d, true)) return false;
		QFont f = t->font(); f.setPointSizeF(d);
		qt_property = "font"; old_value = t->font(); new_value = f;
	}
	else if (property == QLatin1String("x") || property == QLatin1String("y")) {
		if (!number(d, false)) return false;
		QPointF p = t->pos();
		(property == QLatin1String("x") ? p.rx() : p.ry()) = d;
		qt_property = "pos"; old_value = t->pos(); new_value = p;
	}
	else if (property == QLatin1String("rotation")) { if (!number(d, false)) return false; qt_property = "rotation"; old_value = t->rotation(); new_value = d; }
	else { if (!number(d, false)) return false; qt_property = "textWidth"; old_value = t->textWidth(); new_value = d; }

	if (old_value == new_value) return true;
	auto *cmd = new QPropertyUndoCommand(t, qt_property, old_value, new_value);
	cmd->setText(QObject::tr("Modifier un texte d'élément"));
	m_project->undoStack()->push(cmd);
	return true;
}

bool QetScriptApi::deleteElementText(int folioIndex, const QString &elementUuid, int textIndex)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.deleteElementText: project is read-only"));
		return false;
	}
	DynamicElementTextItem *t = findElementText(folioIndex, elementUuid, textIndex,
												QStringLiteral("deleteElementText"));
	if (!t) return false;
	Diagram *diagram = m_project->diagrams().at(folioIndex);
	DiagramContent content;
	content.m_element_texts << t;
	diagram->undoStack().push(new DeleteQGraphicsItemCommand(diagram, content));
	return true;
}

/**
	@brief QetScriptApi::useElementAutoNum
	Make an element numbering context the project's current one, as choosing
	it in the auto-numbering panel does. An empty name clears the selection.
*/
bool QetScriptApi::useElementAutoNum(const QString &name)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.useElementAutoNum: project is read-only"));
		return false;
	}
	if (!name.isEmpty() && !m_project->elementAutoNum().contains(name)) {
		log(QStringLiteral("qet.useElementAutoNum: no element auto-numbering named '%1'").arg(name));
		return false;
	}
	m_project->setCurrrentElementAutonum(name);
	return true;
}

/**
	@brief QetScriptApi::numberElement
	Give one element its label from the current element numbering context,
	through Element::setUpFormula() -- the call the "add element" tool makes
	right after placing one.

	Refused where setUpFormula() would do nothing, rather than reporting
	success: a slave or a report takes its label from its master, and with no
	current context there is no formula to apply.
*/
bool QetScriptApi::numberElement(int folioIndex, const QString &elementUuid)
{
	if (!m_project) return false;
	const QString caller = QStringLiteral("numberElement");
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return false;
	}
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return false;
	if (element->linkType() == Element::Slave || (element->linkType() & Element::AllReport)) {
		log(QStringLiteral("qet.%1: a slave or a report takes its label from its master").arg(caller));
		return false;
	}
	if (m_project->elementAutoNumCurrentFormula().isEmpty()) {
		log(QStringLiteral("qet.%1: no element auto-numbering is selected (see useElementAutoNum)").arg(caller));
		return false;
	}

	// setUpFormula() writes the label straight into the element's
	// information and pushes only the counter's advance onto the undo stack.
	// For a new element that is fine -- undoing the placement removes it --
	// but for one already on the folio, one undo rolled the counter back and
	// left the label behind (measured: c3 stayed "K3" while the counter went
	// back to expecting K3), so the next numbering would repeat a label the
	// counter had forgotten. So take the label it computed, put the
	// information back, and push the change as a command of its own inside
	// the same macro as the counter, making both one step.
	const DiagramContext old_info = element->elementInformations();
	QUndoStack *stack = m_project->undoStack();
	stack->beginMacro(QObject::tr("Numéroter automatiquement un élément"));
	element->setUpFormula(true);
	const DiagramContext new_info = element->elementInformations();
	if (new_info.value(QETInformation::ELMT_LABEL) == old_info.value(QETInformation::ELMT_LABEL)
		&& new_info.value(QStringLiteral("formula")) == old_info.value(QStringLiteral("formula"))) {
		stack->endMacro();
		return false;
	}
	element->setElementInformations(old_info);
	stack->push(new ChangeElementInformationCommand(element, old_info, new_info));
	stack->endMacro();
	return true;
}

/**
	@brief QetScriptApi::duplicateElements
	Copy elements, and the conductors between them, to another place.

	Mirrors DiagramView::copy() and DiagramView::paste(): the copy is
	Diagram::toXml(false, true) of the diagram's @em selection, so the named
	elements are selected for the moment and the previous selection restored
	before returning; the paste is Diagram::fromXml() at the position,
	followed by one PasteDiagramCommand so it is a single undo step.
	@return the uuids of the new elements, or an empty list on failure
*/
QStringList QetScriptApi::duplicateElements(int fromFolioIndex, const QStringList &elementUuids,
											int toFolioIndex, double x, double y)
{
	QStringList created;
	if (!m_project) return created;
	const QString caller = QStringLiteral("duplicateElements");
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return created;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (fromFolioIndex < 0 || fromFolioIndex >= diagrams.count()
		|| toFolioIndex < 0 || toFolioIndex >= diagrams.count()) {
		log(QStringLiteral("qet.%1: folio index out of range").arg(caller));
		return created;
	}
	if (elementUuids.isEmpty()) {
		log(QStringLiteral("qet.%1: no elements named").arg(caller));
		return created;
	}
	Diagram *source = diagrams.at(fromFolioIndex);
	Diagram *target = diagrams.at(toFolioIndex);

	QList<Element *> chosen;
	for (const QString &uuid : elementUuids) {
		Element *e = findElement(fromFolioIndex, uuid);
		if (!e) {
			log(QStringLiteral("qet.%1: no element %2 on folio %3").arg(caller, uuid).arg(fromFolioIndex));
			return created;
		}
		chosen << e;
	}

	// Copying works on the selection, so borrow it and give it back.
	const QList<QGraphicsItem *> previous = source->selectedItems();
	source->clearSelection();
	for (Element *e : std::as_const(chosen)) e->setSelected(true);
	const QDomDocument document = source->toXml(false, true);
	source->clearSelection();
	for (QGraphicsItem *item : previous) item->setSelected(true);

	DiagramContent pasted;
	QDomDocument copy = document;
	target->fromXml(copy, QPointF(x, y), false, &pasted);
	if (!pasted.count()) {
		log(QStringLiteral("qet.%1: nothing was pasted").arg(caller));
		return created;
	}
	target->clearSelection();
	target->undoStack().push(new PasteDiagramCommand(target, pasted));

	// The pasted list comes back in the scene's order, not the order the
	// caller asked in: requesting the elements at x = 700, 100, 900 returned
	// the copies of 100, 700, 900. A caller pairing copies with sources by
	// index would be wired to the wrong ones with no error. A paste is a pure
	// translation, so sorting sources and copies by position pairs them
	// correctly, and the result can be returned in the request's order.
	auto by_position = [](Element *a, Element *b) {
		const QPointF pa = a->pos(), pb = b->pos();
		if (pa.y() != pb.y()) return pa.y() < pb.y();
		return pa.x() < pb.x();
	};
	QList<Element *> sources_sorted = chosen;
	std::stable_sort(sources_sorted.begin(), sources_sorted.end(), by_position);
	QList<Element *> copies_sorted = pasted.m_elements;
	std::stable_sort(copies_sorted.begin(), copies_sorted.end(), by_position);
	if (copies_sorted.count() != sources_sorted.count()) {
		// A paste that produced a different number of elements than were
		// copied cannot be paired, and a wrong pairing is worse than none.
		log(QStringLiteral("qet.%1: %2 element(s) were copied but %3 pasted; "
						   "cannot say which copy is which").arg(caller)
			.arg(sources_sorted.count()).arg(copies_sorted.count()));
		for (Element *e : std::as_const(copies_sorted)) created << e->uuid().toString();
		return created;
	}
	for (Element *source_element : std::as_const(chosen)) {
		created << copies_sorted.at(sources_sorted.indexOf(source_element))->uuid().toString();
	}
	return created;
}

bool QetScriptApi::setProjectTitle(const QString &title)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.setProjectTitle: project is read-only"));
		return false;
	}
	m_project->setTitle(title);
	return m_project->title() == title;
}

namespace {
const QStringList &folioBorderNames()
{
	static const QStringList n{QStringLiteral("columns"), QStringLiteral("column-width"),
		QStringLiteral("display-columns"), QStringLiteral("rows"), QStringLiteral("row-height"),
		QStringLiteral("display-rows")};
	return n;
}
} // namespace

QString QetScriptApi::folioBorder(int folioIndex, const QString &property) const
{
	if (!m_project) return QString();
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return QString();
	const BorderProperties b = diagrams.at(folioIndex)->border_and_titleblock.exportBorder();
	if (property == QLatin1String("columns"))         return QString::number(b.columns_count);
	if (property == QLatin1String("column-width"))    return QString::number(b.columns_width);
	if (property == QLatin1String("display-columns")) return b.display_columns ? QStringLiteral("true") : QStringLiteral("false");
	if (property == QLatin1String("rows"))            return QString::number(b.rows_count);
	if (property == QLatin1String("row-height"))      return QString::number(b.rows_height);
	if (property == QLatin1String("display-rows"))    return b.display_rows ? QStringLiteral("true") : QStringLiteral("false");
	return QString();
}

/**
	@brief QetScriptApi::setFolioBorder
	Change one field of a folio's frame through ChangeBorderCommand, as the
	folio properties panel does. Counts are whole numbers from 1 to 99 and
	sizes are from 1 to 1000. The panel's own upper limits are 99 and 1000;
	its lower limit is 0, which is deliberately not offered -- a grid with
	no columns, or columns of no width, has no use here and 0 was not
	tested, so it is left refused rather than assumed safe. The extremes
	that are offered (99 x 99 cells, widths from 1 to 1000) were exported to
	PNG and did not hang or crash.
*/
bool QetScriptApi::setFolioBorder(int folioIndex, const QString &property, const QString &value)
{
	if (!m_project) return false;
	const QString caller = QStringLiteral("setFolioBorder");
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return false;
	}
	if (!folioBorderNames().contains(property)) {
		log(QStringLiteral("qet.%1: unknown property '%2'; expected one of %3")
			.arg(caller, property, folioBorderNames().join(QStringLiteral(", "))));
		return false;
	}
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return false;
	Diagram *diagram = diagrams.at(folioIndex);

	const BorderProperties old_b = diagram->border_and_titleblock.exportBorder();
	BorderProperties new_b = old_b;
	bool ok = false;
	if (property == QLatin1String("columns") || property == QLatin1String("rows")) {
		const int n = value.toInt(&ok);
		if (!ok || n < 1 || n > 99) {
			log(QStringLiteral("qet.%1: %2 must be a whole number from 1 to 99, not '%3'").arg(caller, property, value));
			return false;
		}
		(property == QLatin1String("columns") ? new_b.columns_count : new_b.rows_count) = n;
	} else if (property == QLatin1String("column-width") || property == QLatin1String("row-height")) {
		const double d = value.toDouble(&ok);
		if (!ok || d < 1 || d > 1000) {
			log(QStringLiteral("qet.%1: %2 must be a number from 1 to 1000, not '%3'").arg(caller, property, value));
			return false;
		}
		(property == QLatin1String("column-width") ? new_b.columns_width : new_b.rows_height) = d;
	} else {
		const QString v = value.toLower();
		if (v != QLatin1String("true") && v != QLatin1String("false")) {
			log(QStringLiteral("qet.%1: %2 is true or false, not '%3'").arg(caller, property, value));
			return false;
		}
		(property == QLatin1String("display-columns") ? new_b.display_columns : new_b.display_rows) = (v == QLatin1String("true"));
	}
	if (new_b == old_b) return true;
	m_project->undoStack()->push(new ChangeBorderCommand(diagram, old_b, new_b));
	return true;
}

/**
	@brief QetScriptApi::elementGeometry
	Where an element is: x and y are its origin (what setElementPosition()
	sets), rotation is in degrees, and left/top/right/bottom are the box it
	occupies on the folio, its drawn extent rather than its origin -- which
	differs from it by the hotspot and, for a rotated element, is the
	rotated extent. Empty if the element is not found.
*/
QVariantMap QetScriptApi::elementGeometry(int folioIndex, const QString &elementUuid) const
{
	QVariantMap g;
	Element *element = findElement(folioIndex, elementUuid);
	if (!element) return g;
	const QRectF box = element->sceneBoundingRect();
	g.insert(QStringLiteral("x"), element->pos().x());
	g.insert(QStringLiteral("y"), element->pos().y());
	g.insert(QStringLiteral("rotation"), element->rotation());
	g.insert(QStringLiteral("left"), box.left());
	g.insert(QStringLiteral("top"), box.top());
	g.insert(QStringLiteral("right"), box.right());
	g.insert(QStringLiteral("bottom"), box.bottom());
	return g;
}

/**
	@brief QetScriptApi::insertFolio
	Add a folio at a position (0 is first, folioCount() is last) through
	QETProject::addNewDiagram(pos) -- undoable. The position is checked
	here: QETProject::addDiagram() hands it straight to QList::insert(),
	which is undefined past the end.
	@return the new folio's index, or -1
*/
int QetScriptApi::insertFolio(int position)
{
	if (!m_project) return -1;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.insertFolio: project is read-only"));
		return -1;
	}
	if (position < 0 || position > m_project->diagrams().count()) {
		log(QStringLiteral("qet.insertFolio: position %1 is outside 0..%2")
			.arg(position).arg(m_project->diagrams().count()));
		return -1;
	}
	Diagram *diagram = m_project->addNewDiagram(position);
	if (!diagram) return -1;
	return m_project->diagrams().indexOf(diagram);
}

/**
	@brief QetScriptApi::titleBlockTemplates
	Every title block template this project can use right now (embedded)
	or could embed and then use (common, company, custom), each name
	suffixed with which. A name can appear more than once, under different
	sources -- embedding does not remove it from where it came from, and a
	project can have its own embedded copy of a name the common collection
	also has, which then shadows it (Diagram::setTitleBlockTemplate() only
	ever looks in the embedded one).
*/
QStringList QetScriptApi::titleBlockTemplates() const
{
	QStringList list;
	if (!m_project) return list;
	auto describe = [&list](TitleBlockTemplatesCollection *c, const QString &source) {
		if (!c) return;
		const QStringList names = c->templates();
		for (const QString &n : names) {
			list << QStringLiteral("%1 (%2)").arg(n, source);
		}
	};
	describe(m_project->embeddedTitleBlockTemplatesCollection(), QStringLiteral("embedded"));
	describe(QETApp::commonTitleBlockTemplatesCollection(), QStringLiteral("common"));
	describe(QETApp::companyTitleBlockTemplatesCollection(), QStringLiteral("company"));
	describe(QETApp::customTitleBlockTemplatesCollection(), QStringLiteral("custom"));
	return list;
}

/**
	@brief QetScriptApi::embedTitleBlockTemplate
	Copy a template's XML into the project's own embedded collection, from
	the first of common/company/custom that has it -- the same
	get/setTemplateXmlDescription() round trip the template editor itself
	uses to save one, not scripting-specific code. A no-op, reporting
	success, if the project already has an embedded copy of that name: the
	embedded one is what Diagram::setTitleBlockTemplate() will use either
	way, so re-embedding would only discard a project-specific edit to it
	for no reason.
*/
bool QetScriptApi::embedTitleBlockTemplate(const QString &name)
{
	if (!m_project) return false;
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.embedTitleBlockTemplate: project is read-only"));
		return false;
	}
	if (name.isEmpty()) {
		log(QStringLiteral("qet.embedTitleBlockTemplate: empty name"));
		return false;
	}
	auto *embedded = m_project->embeddedTitleBlockTemplatesCollection();
	if (embedded->templates().contains(name)) return true;

	const QList<TitleBlockTemplatesCollection *> sources{
		QETApp::commonTitleBlockTemplatesCollection(),
		QETApp::companyTitleBlockTemplatesCollection(),
		QETApp::customTitleBlockTemplatesCollection()};
	for (TitleBlockTemplatesCollection *source : sources)
	{
		if (!source || !source->templates().contains(name)) continue;
		const QDomElement xml = source->getTemplateXmlDescription(name);
		if (xml.isNull()) continue;
		return embedded->setTemplateXmlDescription(name, xml);
	}
	log(QStringLiteral("qet.embedTitleBlockTemplate: no collection has a template named '%1'").arg(name));
	return false;
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

/**
	@brief QetScriptApi::searchAndReplace
	Find and replace a pattern within one text field, across every folio,
	as a single undo step -- the project-wide, single-action counterpart
	to reading a value with elementInfo()/conductorProperty()/
	textContent(), computing a new one in the script itself, and writing
	it back one item at a time (each of those pushes its own undo step; a
	JS loop doing that across hundreds of items would leave hundreds of
	entries on the undo stack instead of one).

	This is NOT QET's own "Search and replace" panel, whose replace model
	is a batch overwrite-with-sentinel template (each field left empty
	keeps the original, a magic string clears it, anything else replaces
	it outright) built for picking items from a tree interactively -- a
	poor fit for a script, which can already express "which items"
	precisely without a GUI tree. This does what the name plainly says
	instead: an actual substring or regex replace within the field's
	current value, changing only the items where the pattern is found.

	@param kind "element_info" (field is an information key, e.g.
	"label"), "conductor" (field is one of setConductorProperty()'s
	property names -- replacing on one conductor of a potential updates
	the whole potential, the same as setConductorProperty() always does),
	or "text" (independent texts; field is ignored)
	@param field the information key or conductor property; ignored for
	"text"
	@param pattern the text (or, if useRegex, the regular expression) to
	search for; never matches an empty field, so nothing already blank
	is ever touched
	@param replacement the replacement text; with useRegex, \1 etc. in
	it refer to pattern's capturing groups, the same as
	QString::replace(QRegularExpression, QString)
	@return the number of items actually changed, or -1 on a usage error
	(nothing was touched)
*/
int QetScriptApi::searchAndReplace(const QString &kind, const QString &field,
								   const QString &pattern, const QString &replacement,
								   bool useRegex, bool caseSensitive)
{
	if (!m_project) return -1;
	const QString caller = QStringLiteral("searchAndReplace");
	if (m_project->isReadOnly()) {
		log(QStringLiteral("qet.%1: project is read-only").arg(caller));
		return -1;
	}
	if (pattern.isEmpty()) {
		log(QStringLiteral("qet.%1: pattern must not be empty").arg(caller));
		return -1;
	}
	if (kind != QLatin1String("element_info") && kind != QLatin1String("conductor")
		&& kind != QLatin1String("text"))
	{
		log(QStringLiteral("qet.%1: unknown kind '%2'; expected element_info, "
						   "conductor or text").arg(caller, kind));
		return -1;
	}
	if (kind == QLatin1String("element_info") && field.isEmpty()) {
		log(QStringLiteral("qet.%1: element_info needs a field (information key)").arg(caller));
		return -1;
	}
	if (kind == QLatin1String("conductor") && !conductorPropertyNames().contains(field)) {
		log(QStringLiteral("qet.%1: unknown conductor property '%2'; expected one of %3")
			.arg(caller, field, conductorPropertyNames().join(QStringLiteral(", "))));
		return -1;
	}

	QRegularExpression re;
	if (useRegex) {
		const auto opts = caseSensitive ? QRegularExpression::NoPatternOption
										: QRegularExpression::CaseInsensitiveOption;
		re = QRegularExpression(pattern, opts);
		if (!re.isValid()) {
			log(QStringLiteral("qet.%1: '%2' is not a valid regular expression: %3")
				.arg(caller, pattern, re.errorString()));
			return -1;
		}
	}
	const Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

	// nullopt when the pattern is not found -- distinguishes "no match"
	// from "matched but happened to produce the same text".
	auto replaced = [&](const QString &current) -> std::optional<QString> {
		if (current.isEmpty()) return std::nullopt;
		if (useRegex) {
			if (!current.contains(re)) return std::nullopt;
			QString updated = current;
			updated.replace(re, replacement);
			return updated;
		}
		if (!current.contains(pattern, cs)) return std::nullopt;
		QString updated = current;
		updated.replace(pattern, replacement, cs);
		return updated;
	};

	const QList<Diagram *> diagrams = m_project->diagrams();

	// Applies the search across every folio, either just counting matches
	// (dryRun) or actually writing them back. Run twice rather than
	// tracked with a flag on every write call: QUndoStack::endMacro()
	// still pushes an empty macro when nothing was added to it (it does
	// not silently discard one the way an empty QUndoCommand child list
	// might suggest), which would leave a no-op "Rechercher et remplacer"
	// entry on the undo stack for a run that changed nothing -- easy to
	// trigger (any search with zero matches) and confusing once there
	// (undoing it visibly does nothing). Counting matches first, and
	// never touching the undo stack at all when that count is zero, is
	// simpler than adding an after-the-fact "was anything pushed" check.
	auto apply = [&](bool dryRun) -> int {
		int count = 0;
		for (int f = 0; f < diagrams.count(); ++f) {
			if (kind == QLatin1String("element_info")) {
				DiagramContent content(diagrams.at(f), false);
				for (Element *elmt : std::as_const(content.m_elements)) {
					const QString current = elmt->elementInformations().value(field).toString();
					const auto updated = replaced(current);
					if (!updated) continue;
					if (dryRun || setInfoKey(f, elmt->uuid().toString(), field, *updated, caller))
						++count;
				}
			} else if (kind == QLatin1String("conductor")) {
				DiagramContent content(diagrams.at(f), false);
				const QList<Conductor *> all = content.conductors(DiagramContent::AnyConductor);
				for (Conductor *c : all) {
					const QString current = conductorPropertyValue(c->properties(), field);
					const auto updated = replaced(current);
					if (!updated) continue;
					// setConductorProperty() applies to the whole potential, so a
					// sibling conductor processed later in this same loop will
					// already read the new value above and find no more match --
					// each potential is touched once, not once per conductor in
					// it. On the dry run nothing is written, so this dedup
					// does not happen there; the dry run's count is only ever
					// used as a nonzero/zero test, not compared to the real one.
					// Prefer whichever terminal carries exactly this one
					// conductor: a hub terminal (several conductors meeting
					// at one point, as in a star topology) is ambiguous --
					// findConductor() (via setConductorProperty()) refuses
					// to address a conductor through it, the same as
					// conductorProperty()/setConductorProperty() called
					// directly would. Skip only if genuinely neither end is
					// addressable; the rest of the potential is still
					// covered by whichever other conductor in it has an
					// unambiguous terminal.
					Terminal *t = c->terminal1;
					if (!t || t->conductors().count() != 1) t = c->terminal2;
					if (!t || t->conductors().count() != 1 || !t->parentElement()) continue;
					Element *elmt = t->parentElement();
					const int terminal_index = elmt->terminals().indexOf(t);
					if (dryRun || setConductorProperty(f, elmt->uuid().toString(),
													   terminal_index, field, *updated))
						++count;
				}
			} else {
				const QList<IndependentTextItem *> list = sortedTexts(f);
				for (int i = 0; i < list.count(); ++i) {
					const auto updated = replaced(list.at(i)->toPlainText());
					if (!updated) continue;
					if (dryRun || setTextContent(f, i, *updated)) ++count;
				}
			}
		}
		return count;
	};

	if (apply(/*dryRun=*/true) == 0) return 0;

	m_project->undoStack()->beginMacro(QObject::tr("Rechercher et remplacer"));
	const int changed = apply(/*dryRun=*/false);
	m_project->undoStack()->endMacro();
	return changed;
}

/**
	@brief QetScriptApi::checkContinuity
	Structural electrical checks against the live scene graph -- Terminal/
	Conductor/relatedPotentialConductors() -- rather than a heuristic read
	of the saved XML the way qet_check's Python side works. Two checks:

	1. unconnected_terminal (severity "info"): a terminal with no
	   conductor at all. Reported at low confidence deliberately -- an
	   unconnected terminal is routine (a spare relay contact, an unused
	   optional pin), not necessarily a mistake, so this is a prompt to
	   look, not a claim that something is wrong.

	2. potential_mismatch (severity "error"): two conductors that
	   electrically belong to the same potential (connected transitively
	   through shared terminals, following bridged terminal strips and
	   linked report elements the same way setConductorProperty() does)
	   but disagree on num, conductor_color, conductor_section, function,
	   bus or cable. QElectroTech's own setConductorProperty() always
	   writes every member of a potential identically, so any divergence
	   found here did not come from this API or the GUI's equivalent
	   action -- it came from hand-edited XML, a legacy file, or an
	   external tool, and it is a real defect: two wire numbers on what
	   is electrically one node is exactly the kind of thing a human
	   reading the schematic would get wrong from.

	What this deliberately does NOT check, because QElectroTech's own
	terminal data model does not carry the information a real check would
	need: pin electrical direction/power conflicts (no terminal in this
	model is marked input/output/power the way a KiCad pin is -- only
	Generic/Inner/Outer/No/Nc/Common, which describe contact role within
	one relay/switch, not signal direction), and short circuits between a
	contact's No and Nc terminals sharing a Common (would need per-
	contact-group semantics this does not attempt). Treat this as
	continuity/consistency checking, not full ERC.

	@param folioIndex a single folio, or -1 for the whole project
	@return a list of {kind, severity, folio, message, ...} objects;
	kind-specific keys: unconnected_terminal has element/elementLabel/
	terminal/terminalName, potential_mismatch has property/values
*/
QVariantList QetScriptApi::checkContinuity(int folioIndex)
{
	QVariantList findings;
	if (!m_project) return findings;
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex >= diagrams.count()) {
		log(QStringLiteral("qet.checkContinuity: folio %1 does not exist (%2 folio(s))")
			.arg(folioIndex).arg(diagrams.count()));
		return findings;
	}

	for (int f = 0; f < diagrams.count(); ++f) {
		if (folioIndex >= 0 && f != folioIndex) continue;
		DiagramContent content(diagrams.at(f), false);
		for (Element *elmt : std::as_const(content.m_elements)) {
			const QList<Terminal *> terminals = elmt->terminals();
			for (int ti = 0; ti < terminals.count(); ++ti) {
				Terminal *t = terminals.at(ti);
				if (!t->conductors().isEmpty()) continue;
				QVariantMap finding;
				finding.insert(QStringLiteral("kind"), QStringLiteral("unconnected_terminal"));
				finding.insert(QStringLiteral("severity"), QStringLiteral("info"));
				finding.insert(QStringLiteral("folio"), f);
				finding.insert(QStringLiteral("element"), elmt->uuid().toString());
				finding.insert(QStringLiteral("elementLabel"), elmt->actualLabel());
				finding.insert(QStringLiteral("terminal"), ti);
				finding.insert(QStringLiteral("terminalName"), t->name());
				finding.insert(QStringLiteral("message"),
					QStringLiteral("terminal %1 ('%2') of %3 has no conductor")
						.arg(ti).arg(t->name(), elmt->actualLabel()));
				findings << finding;
			}
		}
	}

	// "color"/"style" are the rendered pen -- what a person actually sees
	// as "this wire is blue" -- distinct from "conductor_color"
	// (ConductorProperties::m_wire_color), a separate free-text
	// documentation field that is typically empty and says nothing about
	// how the wire is drawn. A mismatch check that only covered the
	// documentation field would miss exactly the visible kind of
	// inconsistency a person would report (issue #974: the two halves of
	// one folio-link conductor drawn in different colours).
	static const QStringList checked_properties = {
		QStringLiteral("num"), QStringLiteral("color"), QStringLiteral("style"),
		QStringLiteral("conductor_color"), QStringLiteral("conductor_section"),
		QStringLiteral("function"), QStringLiteral("bus"), QStringLiteral("cable")};

	QSet<Conductor *> visited;
	for (int f = 0; f < diagrams.count(); ++f) {
		if (folioIndex >= 0 && f != folioIndex) continue;
		DiagramContent content(diagrams.at(f), false);
		const QList<Conductor *> all = content.conductors(DiagramContent::AnyConductor);
		for (Conductor *c : all) {
			if (visited.contains(c)) continue;
			QSet<Conductor *> potential = c->relatedPotentialConductors(true);
			potential << c;
			visited += potential;
			if (potential.count() < 2) continue;

			QHash<QString, QSet<QString>> distinct_values;
			for (Conductor *pc : std::as_const(potential)) {
				for (const QString &prop : checked_properties) {
					distinct_values[prop].insert(conductorPropertyValue(pc->properties(), prop));
				}
			}
			for (const QString &prop : checked_properties) {
				if (distinct_values.value(prop).count() <= 1) continue;
				QStringList values(distinct_values.value(prop).begin(),
								   distinct_values.value(prop).end());
				values.sort();
				QVariantMap finding;
				finding.insert(QStringLiteral("kind"), QStringLiteral("potential_mismatch"));
				finding.insert(QStringLiteral("severity"), QStringLiteral("error"));
				finding.insert(QStringLiteral("folio"), f);
				finding.insert(QStringLiteral("property"), prop);
				finding.insert(QStringLiteral("values"), values);
				finding.insert(QStringLiteral("message"),
					QStringLiteral("conductors on the same electrical potential disagree "
								   "on %1: %2").arg(prop, values.join(QStringLiteral(", "))));
				findings << finding;
			}
		}
	}

	// report_link_mismatch (warning, not error): a next_report/
	// previous_report pair -- QElectroTech's folio-jump-arrow links,
	// e.g. "Folio suivant"/"Folio précédent" -- is meant to represent one
	// wire continuing across a folio boundary, but LinkElementCommand::
	// isLinkable() only ever checks type and freedom (see its own doc
	// comment), never conductor properties. Nothing in QElectroTech
	// copies one side's colour/style/num onto the other when the link is
	// made, or keeps them in sync afterwards, so this is a real,
	// unenforced gap rather than something a script or the GUI could
	// have broken -- hence "warning", not "error" the way
	// potential_mismatch is (which the app's own edits can never
	// produce, so any occurrence there is definitely external tampering).
	// Terminals are matched by index between the two linked elements: a
	// report pair is authored as matching symbols carrying the same set
	// of wires in the same declared order, the same convention terminal
	// indexing already follows everywhere else in this API.
	QSet<Element *> visited_report;
	for (int f = 0; f < diagrams.count(); ++f) {
		if (folioIndex >= 0 && f != folioIndex) continue;
		DiagramContent content(diagrams.at(f), false);
		for (Element *elmt : std::as_const(content.m_elements)) {
			if (elmt->linkType() != Element::NextReport
				&& elmt->linkType() != Element::PreviousReport) continue;
			if (visited_report.contains(elmt)) continue;

			const QList<Element *> linked = elmt->linkedElements();
			for (Element *other : linked) {
				if (visited_report.contains(other)) continue;
				visited_report << elmt << other;

				const QList<Terminal *> ta = elmt->terminals();
				const QList<Terminal *> tb = other->terminals();
				const int n = qMin(ta.count(), tb.count());
				auto *other_diagram = qobject_cast<Diagram *>(other->scene());
				const int other_folio = other_diagram ? diagrams.indexOf(other_diagram) : -1;

				for (int i = 0; i < n; ++i) {
					const QList<Conductor *> ca = ta.at(i)->conductors();
					const QList<Conductor *> cb = tb.at(i)->conductors();
					// ambiguous (>1) or unconnected (0) on either side:
					// nothing to meaningfully compare
					if (ca.count() != 1 || cb.count() != 1) continue;

					for (const QString &prop : checked_properties) {
						const QString va = conductorPropertyValue(ca.first()->properties(), prop);
						const QString vb = conductorPropertyValue(cb.first()->properties(), prop);
						if (va == vb) continue;
						QVariantMap finding;
						finding.insert(QStringLiteral("kind"), QStringLiteral("report_link_mismatch"));
						finding.insert(QStringLiteral("severity"), QStringLiteral("warning"));
						finding.insert(QStringLiteral("folio"), f);
						finding.insert(QStringLiteral("element"), elmt->uuid().toString());
						finding.insert(QStringLiteral("otherFolio"), other_folio);
						finding.insert(QStringLiteral("otherElement"), other->uuid().toString());
						finding.insert(QStringLiteral("terminal"), i);
						finding.insert(QStringLiteral("property"), prop);
						finding.insert(QStringLiteral("values"), QStringList{va, vb});
						finding.insert(QStringLiteral("message"),
							QStringLiteral("folio-link conductor disagrees on %1 across the "
										   "link: '%2' vs '%3'").arg(prop, va, vb));
						findings << finding;
					}
				}
			}
		}
	}

	return findings;
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

QStringList QetScriptApi::selectedElements(int folioIndex) const
{
	QStringList list;
	if (!m_project) return list;
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return list;
	for (QGraphicsItem *item : diagrams.at(folioIndex)->selectedItems()) {
		if (item->type() == Element::Type) {
			list << static_cast<Element *>(item)->uuid().toString();
		}
	}
	list.sort();
	return list;
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
