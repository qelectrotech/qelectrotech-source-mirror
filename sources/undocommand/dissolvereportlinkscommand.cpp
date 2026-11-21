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
#include "dissolvereportlinkscommand.h"

#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/element.h"
#include "../qetproject.h"

/**
	@brief DissolveReportLinksCommand::DissolveReportLinksCommand
	The links are taken here, before the first redo() takes them away.
	@param conductors the wires whose potentials hold the links
	@param parent the undo step this belongs to, made its first child so
	that the links go before the names are written
*/
DissolveReportLinksCommand::DissolveReportLinksCommand(
		const QList<Conductor *> &conductors,
		QUndoCommand *parent) :
	QUndoCommand(parent)
{
	setText(QObject::tr("Take away the folio report links of a potential",
						"undo caption"));
	QETProject *project = nullptr;
	for (Conductor *conductor : conductors) {
		if (conductor && conductor->diagram()) {
			project = conductor->diagram()->project();
			break;
		}
	}
	if (!project) {
		return;
	}
	const auto links = project->reportLinksOfPotential(conductors);
	for (const auto &link : links) {
		m_links.append({QPointer<Element>(link.first),
						QPointer<Element>(link.second)});
	}
}

/**
	@brief DissolveReportLinksCommand::redo
	The wires a folio report joins need not carry the same name: the
	links go before a name is written over their potential.
*/
void DissolveReportLinksCommand::redo()
{
	for (const auto &link : std::as_const(m_links)) {
		if (link.first && !link.first->isFree()) {
			link.first->unlinkAllElements();
		}
	}
}

/**
	@brief DissolveReportLinksCommand::undo
	The links this edit took away come back as they were.
*/
void DissolveReportLinksCommand::undo()
{
	for (const auto &link : std::as_const(m_links)) {
		if (link.first && link.second
				&& link.first->isFree() && link.second->isFree()) {
			link.first->linkToElement(link.second.data());
		}
	}
}
