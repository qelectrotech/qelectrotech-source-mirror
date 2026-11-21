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
#include "reportlinkkeycommand.h"

#include "../elementprovider.h"
#include "../qetgraphicsitem/element.h"

/**
	@brief ChangeReportLinkKeyCommand::ChangeReportLinkKeyCommand
	@param project the project which starts chaining by another key
	@param key the new key
	@param parent the parent undo command, if any
*/
ChangeReportLinkKeyCommand::ChangeReportLinkKeyCommand(
		QETProject *project,
		QETProject::ReportLinkKey key,
		QUndoCommand *parent) :
	QUndoCommand(parent),
	m_project(project),
	m_new_key(key)
{
	setText(QObject::tr("Change what chains folio report elements together",
						"undo caption"));
	if (!m_project) {
		return;
	}
	m_old_key = m_project->chainedReportsKey();

		//The links this project has now: undo puts them back. They are
		//taken here, before the first redo() takes them away.
	ElementProvider ep(m_project);
	for (const QPointer<Element> &elmt_ptr : ep.find(ElementData::AllReport)) {
		if (!elmt_ptr || elmt_ptr->isFree()) {
			continue;
		}
		const QList<Element *> linked = elmt_ptr->linkedElements();
		if (!linked.isEmpty()) {
			m_links.append(qMakePair(elmt_ptr, QPointer<Element>(linked.first())));
		}
	}
}

/**
	@brief ChangeReportLinkKeyCommand::redo
	The chains this project has were made with the old key: they do not
	describe what the new one pairs, so they go and are made again from
	it. Nothing while the project does not chain its reports at all: the
	links are then made by hand and are nobody's to take away.
*/
void ChangeReportLinkKeyCommand::redo()
{
	if (!m_project) {
		return;
	}
	m_project->setChainedReportsKey(m_new_key);
	if (!m_project->chainedReports()) {
		return;
	}
	m_project->dissolveReportLinks();
	m_project->autoLinkReportElements();
}

/**
	@brief ChangeReportLinkKeyCommand::undo
	Put the key back and, with it, the links this project had before the
	change: the ones the new key made are not the old ones and go with it.
*/
void ChangeReportLinkKeyCommand::undo()
{
	if (!m_project) {
		return;
	}
	m_project->setChainedReportsKey(m_old_key);
	if (!m_project->chainedReports()) {
		return;
	}
	m_project->dissolveReportLinks();
	for (const auto &pair : std::as_const(m_links)) {
		if (!pair.first || !pair.second) {
			continue;
		}
		if (pair.first->isFree() && pair.second->isFree()) {
			pair.first->linkToElement(pair.second.data());
		}
	}
	m_project->refreshReportTexts();
}
