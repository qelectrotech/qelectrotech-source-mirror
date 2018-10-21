/*
	Copyright 2006-2018 The QElectroTech Team
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
#include "searchandreplaceworker.h"
#include "diagram.h"
#include "changetitleblockcommand.h"

SearchAndReplaceWorker::SearchAndReplaceWorker()
{}

/**
 * @brief SearchAndReplaceWorker::clear
 * All registred properties
 */
void SearchAndReplaceWorker::clear()
{
	m_titleblock_properties = TitleBlockProperties();
}

/**
 * @brief SearchAndReplaceWorker::replaceDiagram
 * Replace all properties of each diagram in @diagram_list,
 * by the current titleblock propertie of this worker
 * @param diagram_list, list of diagram to be changed, all diagrams must belong to the same project;
 */
void SearchAndReplaceWorker::replaceDiagram(QList<Diagram *> diagram_list)
{
	if (diagram_list.isEmpty()) {
		return;
	}
	
	QETProject *project = diagram_list.first()->project();
	for (Diagram *d : diagram_list) {
		if (d->project() != project) {
			return;
		}
	}
	
	QUndoStack *us = project->undoStack();
	us->beginMacro(QObject::tr("Chercher remplacer les propriétés de folio"));
	for (Diagram *d : diagram_list)
	{
		TitleBlockProperties old_propertie = d->border_and_titleblock.exportTitleBlock();
		TitleBlockProperties new_properties = old_propertie;
		
		if (!m_titleblock_properties.title.isEmpty())
		{
			if (m_titleblock_properties.title == eraseText()) {
				new_properties.title.clear();
			} else {
				new_properties.title = m_titleblock_properties.title;
			}
		}
		if (!m_titleblock_properties.author.isEmpty())
		{
			if (m_titleblock_properties.author == eraseText()) {
				new_properties.author.clear();
			} else {
				new_properties.author = m_titleblock_properties.author;
			}
		}
		if (!m_titleblock_properties.filename.isEmpty())
		{
			if (m_titleblock_properties.filename == eraseText()) {
				new_properties.filename.clear();
			} else {
				new_properties.filename = m_titleblock_properties.filename;
			}
		}
		if (!m_titleblock_properties.machine.isEmpty())
		{
			if (m_titleblock_properties.machine == eraseText()) {
				new_properties.machine.clear();
			} else {
				new_properties.machine = m_titleblock_properties.machine;
			}
		}
		if (!m_titleblock_properties.locmach.isEmpty())
		{
			if (m_titleblock_properties.locmach == eraseText()) {
				new_properties.locmach.clear();
			} else {
				new_properties.locmach = m_titleblock_properties.locmach;
			}
		}
		if (!m_titleblock_properties.indexrev.isEmpty())
		{
			if (m_titleblock_properties.indexrev == eraseText()) {
				new_properties.indexrev.clear();
			} else {
				new_properties.indexrev = m_titleblock_properties.indexrev;
			}
		}
		if (!m_titleblock_properties.folio.isEmpty())
		{
			if (m_titleblock_properties.folio == eraseText()) {
				new_properties.folio.clear();
			} else {
				new_properties.folio = m_titleblock_properties.folio;
			}
		}

		if (m_titleblock_properties.date.isValid())
		{
			if (m_titleblock_properties.date == eraseDate()) {
				new_properties.date = QDate();
			} else {
				new_properties.date = m_titleblock_properties.date;
			}
		}
		
		new_properties.context.add(m_titleblock_properties.context);
		
		if (old_propertie != new_properties) {
			project->undoStack()->push(new ChangeTitleBlockCommand(d, old_propertie, new_properties));
		}
	}
	us->endMacro();
}

void SearchAndReplaceWorker::replaceDiagram(Diagram *diagram)
{
	QList<Diagram *> list;
	list.append(diagram);
	replaceDiagram(list);
}
