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
#include "pastenumberingimport.h"

#include "../../diagram.h"
#include "../../diagramcommands.h"
#include "../../qetmessagebox.h"
#include "../../qetproject.h"

#include <QSettings>

/// @return the numberings a copy carries, which pasted elements follow
QList<ElementAutoNumSchemeCommand::Scheme> PasteNumberingImport::copiedBy(const QDomDocument &clipboard)
{
	return ElementAutoNumSchemeCommand::copiedSchemes(clipboard.documentElement());
}

/**
	@brief PasteNumberingImport::push
	Push the paste of @p content on the undo stack of @p diagram, after
	asking whether to import the numberings of @p copied which the pasted
	elements follow and the project does not have.
	@param parent : parent of the question
	@param copied : the numberings the copy carries
*/
void PasteNumberingImport::push(QWidget *parent,
								Diagram *diagram,
								const DiagramContent &content,
								const QList<ElementAutoNumSchemeCommand::Scheme> &copied)
{
	QETProject *project = diagram->project();
	auto *paste = new PasteDiagramCommand(diagram, content);

	QList<ElementAutoNumSchemeCommand::Scheme> missing;
	if (project && !copied.isEmpty()
			&& QSettings().value("diagramcommands/autonumber-pasted-elements", true).toBool()) {
		missing = ElementAutoNumSchemeCommand::missingForPaste(project, copied, content.m_elements);
	}
	if (missing.isEmpty()) {
		diagram->undoStack().push(paste);
		return;
	}

	QStringList names;
	for (const auto &scheme : std::as_const(missing)) {
		names << QStringLiteral("« %1 »").arg(scheme.title);
	}
	const auto answer = QET::QetMessageBox::question(
				parent,
				tr("Numbering missing from this project"),
				tr("The pasted elements follow %n numberings that do not exist in this project: %1.\n"
				   "\n"
				   "Import them? The elements will then receive the next numbers, starting from 1. Otherwise "
				   "they will be pasted without numbering.", "", static_cast<int>(missing.size()))
				.arg(names.join(QStringLiteral(", "))),
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::Yes);
	if (answer != QMessageBox::Yes) {
		diagram->undoStack().push(paste);
		return;
	}

	diagram->undoStack().beginMacro(paste->text());
	for (const auto &scheme : std::as_const(missing))
	{
			//Under its own name, or one which is free here
		QString title = scheme.title;
		for (int n = 2 ; !project->elementAutoNumNameClash(title).isEmpty() ; ++n) {
			title = QStringLiteral("%1 (%2)").arg(scheme.title).arg(n);
		}
		if (auto *cmd = ElementAutoNumSchemeCommand::create(
					project, title,
					ElementAutoNumSchemeCommand::resetForRenumber(scheme.context),
					scheme.id, false)) {
			diagram->undoStack().push(cmd);
		}
	}
	diagram->undoStack().push(paste);
	diagram->undoStack().endMacro();
}
