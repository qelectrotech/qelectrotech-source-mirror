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
#include "openelmtcommand.h"
#include "../elementscene.h"
#include "../graphicspart/partterminal.h"

#include <QDomDocument>
#include <QObject>

OpenElmtCommand::OpenElmtCommand(const QDomDocument &document,
								 QPointer<ElementScene> scene,
								 QUndoCommand *parent) :
	QUndoCommand{parent},
	m_document{document.cloneNode().toDocument()},
	m_scene{scene}
{
	setText(QObject::tr("Ouvrir un element"));
}

OpenElmtCommand::~OpenElmtCommand()
{
	if (m_scene) {
		m_scene->qgiManager().release(m_graphics_item);
	}
}

void OpenElmtCommand::undo()
{
	m_scene->removeItems(m_graphics_item.toVector());
}

void OpenElmtCommand::redo()
{
	if (!m_scene) {
		return;
	}

	if (m_first_redo)
	{
		m_scene->fromXml(m_document, QPointF(), true, &m_graphics_item);
			//The terminals are added to an element that may already hold
			//the same ones -- importing the same symbol twice, or two old
			//symbols whose terminals get the same derived uuid -- so they
			//get new uuids, as a paste gives them (PastePartsCommand).
		for (QGraphicsItem *qgi : std::as_const(m_graphics_item)) {
			if (qgi->type() == PartTerminal::Type) {
				static_cast<PartTerminal *>(qgi)->setNewUuid();
			}
		}
		m_scene->qgiManager().manage(m_graphics_item);
		m_first_redo = false;

			//m_document is now useless,
			//we clear it to use less memory
		m_document.clear();
	}
	else {
		m_scene->addItems(m_graphics_item.toVector());
	}

#pragma message("@TODO uncomment slot_select when fixed, see itemChange function for each primitive")
		//Commented because it takes a lot of time
		//when a lot of primitives are loaded!
		//needs work
	//m_scene->slot_select(m_graphics_item);
}
