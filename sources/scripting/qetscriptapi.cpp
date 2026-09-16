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
#include "../qetgraphicsitem/element.h"
#include "../qetmessagebox.h"
#include "../qetproject.h"
#include "../undocommand/addgraphicsobjectcommand.h"
#include "../undocommand/deleteqgraphicsitemcommand.h"

#include <QFile>
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
	@param output path to write to; the project's own file path if empty
	@return false if there is no output path to use, or the file could not
	be opened for writing
*/
bool QetScriptApi::save(const QString &output)
{
	if (!m_project) return false;
	const QString path = output.isEmpty() ? m_project->filePath() : output;
	if (path.isEmpty()) {
		log(QStringLiteral("qet.save: no output path given and the project has none of its own -- pass one"));
		return false;
	}
	QFile file(path);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		log(QStringLiteral("qet.save: cannot open '%1' for writing").arg(path));
		return false;
	}
	QTextStream file_out(&file);
	file_out << m_project->toXml().toString(4);
	file.close();
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

/**
	@brief QetScriptApi::addElement
	Place a new element on a folio, through the same AddGraphicsObjectCommand
	the interactive drag-from-collection-panel path uses (see
	DiagramEventAddElement::addElement()) -- so Ctrl+Z undoes it exactly as
	it would a manually dropped element.
	@param folioIndex
	@param locationPath an element collection path, e.g.
	"embed://some/path.elmt" or "common://10_electric/...elmt"
	@param x @param y target position, in the diagram's own coordinates
	@return the new element's uuid (empty string on failure -- bad folio
	index, or the location could not be resolved/built)
*/
QString QetScriptApi::addElement(int folioIndex, const QString &locationPath, double x, double y)
{
	if (!m_project) return QString();
	const QList<Diagram *> diagrams = m_project->diagrams();
	if (folioIndex < 0 || folioIndex >= diagrams.count()) return QString();
	Diagram *diagram = diagrams.at(folioIndex);

	ElementsLocation location(locationPath, m_project);
	int state = 0;
	Element *element = ElementFactory::Instance()->createElement(location, nullptr, &state);
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
