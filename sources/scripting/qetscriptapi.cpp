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

#include "../cli_export.h"
#include "../diagram.h"
#include "../diagramcontent.h"
#include "../qetproject.h"

#include <QTextStream>

QetScriptApi::QetScriptApi(QETProject *project, QObject *parent) :
	QObject(parent),
	m_project(project)
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

bool QetScriptApi::save(const QString &output)
{
	return runFlag(QStringLiteral("--resave"), {output});
}

void QetScriptApi::log(const QString &message)
{
	QTextStream(stderr) << message << "\n";
}
