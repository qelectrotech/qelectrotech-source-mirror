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
#include "promoteshapecommand.h"
#include "../qetgraphicsitem/qetshapeitem.h"

PromoteShapeCommand::PromoteShapeCommand(
		QetShapeItem *shape,
		const QDomElement &priorStateXml,
		const QDomElement &newStateXml,
		QUndoCommand *parent) :
	QUndoCommand(QObject::tr("Transformer %1").arg(shape ? shape->name() : QString()), parent),
	m_shape(shape)
{
	m_priorDoc.appendChild(m_priorDoc.importNode(priorStateXml, true));
	m_newDoc.appendChild(m_newDoc.importNode(newStateXml, true));
}

void PromoteShapeCommand::undo()
{
	if (m_shape)
		m_shape->fromXml(m_priorDoc.documentElement());
}

void PromoteShapeCommand::redo()
{
	if (m_shape)
		m_shape->fromXml(m_newDoc.documentElement());
}
