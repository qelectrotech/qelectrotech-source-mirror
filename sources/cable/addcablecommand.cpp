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
#include "addcablecommand.h"

#include "cablemanager.h"
#include "cablepart.h"
#include "../autoNum/numerotationcontextcommands.h"
#include "../diagram.h"
#include "../qetproject.h"

#include <utility>

/**
	@brief AddCableCommand::AddCableCommand
	@param cable the cable the line was just given to
	@param part the line which was just drawn
	@param diagram the folio it was drawn on
	@param own_cable true when the cable was created by this very action
	@param taken the cores this line took away from other cables, so
	undo can give them back
*/
AddCableCommand::AddCableCommand(Cable *cable,
								 CablePart *part,
								 Diagram *diagram,
								 bool own_cable,
								 const QList<CableCoreTaken> &taken) :
	m_cable(cable),
	m_part(part),
	m_diagram(diagram),
	m_taken(taken),
	m_own_cable(own_cable)
{
	if (!m_cable || !m_part) return;

	m_part_data.uuid = m_part->partUuid();
	m_part_data.diagram = m_diagram ? m_diagram->uuid() : QUuid();
	m_part_data.p1 = m_part->firstPoint();
	m_part_data.p2 = m_part->secondPoint();

	for (const CableCore &core : m_cable->usedCores()) {
		if (core.part == m_part_data.uuid) {
			m_cores.append(core);
		}
	}

		//A cable this action brought into being takes the next number
		//the project's numbering rule hands out. The number itself was
		//worked out before this command existed; what has to happen here
		//is moving the rule on, and moving it is undoable just like the
		//cable is -- so undoing gives the number back and the cable
		//drawn after that takes it again instead of skipping over it.
		//
		//Only the rule as it stands now is taken down. Where the rule
		//stands after this cable is worked out when the cable really
		//comes into being, because several cables pasted together are
		//all made before any of them is added, and each of them has to
		//move the rule on from wherever the one before it left it.
	if (m_own_cable && m_diagram && project() && project()->hasCableAutoNum())
	{
		const QString key = project()->cableCurrentAutoNum();
		const NumerotationContext current = project()->cableAutoNum(key);
		if (!current.isEmpty())
		{
			m_num_key = key;
			m_num_before = current;
		}
	}
}

/**
	@brief AddCableCommand::~AddCableCommand
	Whatever the stack leaves behind has to be cleaned up here: a cable
	which is not held by the project any more belongs to this command,
	and so does a line which never made it onto a folio.

	The cable is a QPointer, so if another command took it away first it
	is already null here and there is nothing left to do -- which is
	exactly the point, since following a raw pointer here meant freeing
	the same cable a second time.
*/
AddCableCommand::~AddCableCommand()
{
	if (m_cable && m_own_cable) {
		QETProject *proj = project();
		if (!proj || !proj->cables().contains(m_cable.data())) {
			delete m_cable.data();
		}
	}
	if (m_part && !m_part->scene()) {
		delete m_part;
	}
}

/**
	@brief AddCableCommand::project
	@return the project this command works in, or nullptr
*/
QETProject *AddCableCommand::project() const
{
	return m_diagram ? m_diagram->project() : nullptr;
}

/**
	@brief AddCableCommand::redo
	Put the line on the folio, together with everything the cable holds
	for it.
*/
void AddCableCommand::redo()
{
	QETProject *proj = project();
	if (!proj || !m_cable || !m_part || !m_diagram) return;

	if (m_own_cable) {
		proj->addCable(m_cable);
	} else {
		if (!m_cable->hasPart(m_part_data.uuid)) {
			m_cable->addPart(m_part_data);
		}
		for (const CableCore &core : m_cores) {
			m_cable->setCore(core);
		}
	}

	if (m_part->scene() != m_diagram) {
		m_diagram->addItem(m_part);
	}

		//Take the cores away from the cables which held them before this
		//line claimed those conductors. On the very first run they are
		//already gone, so nothing happens then; redoing after an undo
		//has to take them again.
	for (const CableCoreTaken &taken : std::as_const(m_taken))
	{
		if (taken.cable && taken.cable != m_cable) {
			taken.cable->removeCore(taken.core.core);
		}
	}
		//The cable field of every conductor is worked out again either
		//way: redoing after an undo puts the cable back into the project
		//without it having said anything, and the wires it feeds would go
		//on showing a blank cable field although the cable which
		//generates that field is right there again.
		//
		//And the number this cable was given is spent now: the rule
		//moves on to the one the next cable will be handed. Worked out
		//here rather than when the command was made, because a run of
		//cables pasted together is made from end to end before any of
		//it is added -- each of them has to move the rule on from where
		//the one before it left it.
	if (!m_num_key.isEmpty()) {
		NumerotationContextCommands ncc(proj->cableAutoNum(m_num_key),
										 m_diagram.data());
		proj->addCableAutoNum(m_num_key, ncc.next());
		proj->setCurrentCableAutoNum(m_num_key);
	}
	CableManager::refreshLabels(proj);
}

/**
	@brief AddCableCommand::undo
	Take the line back off the folio. An existing cable loses the
	section and the cores it was carrying on it -- and with them the
	cable field of the conductors, which CableManager works out again as
	soon as the cable says it changed.

	The cores this line had taken away from other cables go back to
	those cables afterwards: without them the conductors they belong to
	would be left with a blank cable field although the cable which
	generates that field is still there.
*/
void AddCableCommand::undo()
{
	QETProject *proj = project();
	if (!proj || !m_cable || !m_part || !m_diagram) return;

	if (m_part->scene()) {
		m_diagram->removeItem(m_part);
	}

	if (m_own_cable) {
			//The cable keeps everything it knows: it is only taken out of
			//the project, so redo finds it exactly as it was.
		proj->removeCable(m_cable);
	} else {
		m_cable->removePart(m_part_data.uuid);
	}

	for (const CableCoreTaken &taken : std::as_const(m_taken))
	{
		if (taken.cable && taken.cable != m_cable) {
			taken.cable->setCore(taken.core);
		}
	}
		//The number this cable was given goes back into the rule, so
		//the cable drawn after an undo takes that number rather than
		//the one after it: nothing of it is left spent.
	if (!m_num_key.isEmpty()) {
		proj->addCableAutoNum(m_num_key, m_num_before);
		proj->setCurrentCableAutoNum(m_num_key);
	}

		//Same as in redo(): worked out again whatever happened above,
		//so a cable section or a whole cable taken away never leaves
		//the wires it used to feed with a cable field it still writes.
	CableManager::refreshLabels(proj);
}
