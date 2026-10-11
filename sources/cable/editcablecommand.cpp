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
#include "editcablecommand.h"

#include "cablemanager.h"
#include "cablepart.h"
#include "../diagram.h"
#include "../qetproject.h"

#include <QCoreApplication>

#include <utility>

/**
	@brief BatchCommand::BatchCommand
	@param first step to run first, kept by this command
	@param second step to run right after it, kept by this command
*/
BatchCommand::BatchCommand(QUndoCommand *first, QUndoCommand *second) :
	m_steps({first, second})
{
	m_steps.removeAll(nullptr);
	setText(QCoreApplication::translate("BatchCommand", "Delete"));
}

/**
	@brief BatchCommand::BatchCommand
	@param steps every step of one single gesture, in the order the
	user did them, kept by this command
	@param text what the gesture was, as the undo history shows it
*/
BatchCommand::BatchCommand(const QList<QUndoCommand *> &steps, const QString &text) :
	m_steps(steps)
{
	m_steps.removeAll(nullptr);
	setText(text);
}

/**
	@brief BatchCommand::~BatchCommand
	The steps it holds go with it: they were never given to anything else.
*/
BatchCommand::~BatchCommand()
{
	for (QUndoCommand *step : std::as_const(m_steps)) {
		delete step;
	}
}

/**
	@brief BatchCommand::redo
	Every step, in the order the user did them.
*/
void BatchCommand::redo()
{
	for (QUndoCommand *step : std::as_const(m_steps)) {
		if (step) step->redo();
	}
}

/**
	@brief BatchCommand::undo
	Every step taken back in the reverse order, so what one step changed
	is still there when the next one is undone.
*/
void BatchCommand::undo()
{
	for (auto it = m_steps.crbegin(); it != m_steps.crend(); ++it) {
		if (*it) (*it)->undo();
	}
}

/**
	@brief MoveCablePartCommand::MoveCablePartCommand
	@param cable the cable the section belongs to
	@param part the line being dragged
	@param before_p1
	@param before_p2
	@param after_p1
	@param after_p2
	@param before_cores where every colour label stood before the drag
	@param after_cores where it stands after it
*/
MoveCablePartCommand::MoveCablePartCommand(Cable *cable,
										   CablePart *part,
										   const QPointF &before_p1,
										   const QPointF &before_p2,
										   const QPointF &after_p1,
										   const QPointF &after_p2,
										   const QList<CableCore> &before_cores,
										   const QList<CableCore> &after_cores) :
	m_cable(cable),
	m_part(part),
	m_before_p1(before_p1),
	m_before_p2(before_p2),
	m_after_p1(after_p1),
	m_after_p2(after_p2),
	m_before_cores(before_cores),
	m_after_cores(after_cores)
{
	setText(QCoreApplication::translate("MoveCablePartCommand", "Move a cable"));
}

void MoveCablePartCommand::undo()
{
	apply(m_before_p1, m_before_p2, m_before_cores);
}

void MoveCablePartCommand::redo()
{
	apply(m_after_p1, m_after_p2, m_after_cores);
}

/**
	@brief MoveCablePartCommand::apply
	Put the line where it belongs and the colour labels with it. The
	positions are written last: they are stored in scene coordinates, so
	they only make sense once the line itself is back in place.
	@param p1
	@param p2
	@param cores
*/
void MoveCablePartCommand::apply(const QPointF &p1,
								 const QPointF &p2,
								 const QList<CableCore> &cores)
{
	if (m_part) {
		m_part->setLine(p1, p2);
	}
	if (m_cable) {
		m_cable->setCores(cores);
	}
}

/**
	@brief ChangeCablePropertiesCommand::ChangeCablePropertiesCommand
	@param cable
	@param before every field as it was before the edit
	@param after every field as the user left it
*/
ChangeCablePropertiesCommand::ChangeCablePropertiesCommand(Cable *cable,
														   const CableProperties &before,
														   const CableProperties &after) :
	m_cable(cable),
	m_before(before),
	m_after(after)
{
	setText(QCoreApplication::translate("ChangeCablePropertiesCommand",
										"Edit the properties of a cable"));
}

void ChangeCablePropertiesCommand::undo()
{
	if (m_cable) {
		m_cable->setProperties(m_before);
	}
}

void ChangeCablePropertiesCommand::redo()
{
	if (m_cable) {
		m_cable->setProperties(m_after);
	}
}

/**
	@brief RenumberCablesCommand::RenumberCablesCommand
	@param cables the cables to number, in the order they are numbered in
	@param designations the number each of them gets
	@param project the project whose numbering rule moves along with
	the numbering
	@param num_key the name that rule is kept under in the project
	@param num_before where its counters stood before the numbering ran
	@param num_after where the numbering leaves them
*/
RenumberCablesCommand::RenumberCablesCommand(const QList<Cable *> &cables,
											 const QStringList &designations,
											 QETProject *project,
											 const QString &num_key,
											 const NumerotationContext &num_before,
											 const NumerotationContext &num_after) :
	m_project(project),
	m_num_key(num_key),
	m_num_before(num_before),
	m_num_after(num_after)
{
	for (int i = 0; i < cables.size() && i < designations.size(); ++i)
	{
		Cable *cable = cables.at(i);
		if (!cable) continue;

		m_cables.append(cable);
		m_before.append(cable->designation());
		m_by_hand_before.append(cable->designationByHand());
		m_after.append(designations.at(i));
			//A number the rule has just handed out is the rule's, so
			//nothing here stays marked as having been typed in
		m_by_hand_after.append(false);
	}

	setText(QCoreApplication::translate("RenumberCablesCommand",
										"Renumber %n cable(s)",
										"", m_cables.size()));
}

void RenumberCablesCommand::undo()
{
	apply(false);
}

void RenumberCablesCommand::redo()
{
	apply(true);
}

/**
	@brief RenumberCablesCommand::apply
	@param use_after true to write the numbers the rule handed out,
	false to put the ones back which were there before
*/
void RenumberCablesCommand::apply(bool use_after)
{
	for (int i = 0; i < m_cables.size(); ++i)
	{
		Cable *cable = m_cables.at(i).data();
		if (!cable) continue;

			//Where the name comes from is written first: a number the
			//rule has just handed out is the rule's whatever the cable
			//was called before, and undo has to say so again
		cable->setDesignationByHand(use_after ? m_by_hand_after.at(i)
											  : m_by_hand_before.at(i));
		cable->setDesignation(use_after ? m_after.at(i) : m_before.at(i));
	}

		//Where the numbering rule itself stands moves along with the
		//numbering: once it has run, its counters are where the last
		//number was taken from, so the next cable drawn goes on from
		//there rather than skipping over every number in between. Undo
		//puts them back like the names, since numbering a project over
		//is one single act.
	if (m_project && !m_num_key.isEmpty())
	{
		const NumerotationContext context = use_after ? m_num_after
													  : m_num_before;
			//An empty rule is never written: losing the rule altogether
			//is not something numbering may do, not even on the way back
		if (!context.isEmpty())
		{
			m_project->addCableAutoNum(m_num_key, context);
			m_project->setCurrentCableAutoNum(m_num_key);
		}
	}
}

/**
	@brief ChangeCableTypeCommand::ChangeCableTypeCommand
	@param cable
	@param before the type, the count, the colours and the cores as
	they were before the change
	@param after the very same four as the chosen type says
*/
ChangeCableTypeCommand::ChangeCableTypeCommand(Cable *cable,
											   const CableTypeState &before,
											   const CableTypeState &after) :
	m_cable(cable),
	m_before(before),
	m_after(after)
{
	setText(QCoreApplication::translate("ChangeCableTypeCommand",
										"Change the cable type"));
}

void ChangeCableTypeCommand::undo()
{
	apply(m_before);
}

void ChangeCableTypeCommand::redo()
{
	apply(m_after);
}

/**
	@brief ChangeCableTypeCommand::apply
	One block in one pass: name, colours, count, then the cores. The
	count has to stand before the cores, since a cable with fewer cores
	cannot hold the others -- and the other way round, undoing a type
	with fewer cores has to open the room before the cores come back
	into it.
	@param state the block to write
*/
void ChangeCableTypeCommand::apply(const CableTypeState &state)
{
	if (m_cable) {
		m_cable->applyTypeState(state);
	}
}

/**
	@brief ChangeCableCoresCommand::ChangeCableCoresCommand
	@param cable
	@param before every core the cable wired before the change
	@param after every core it wires after it
	@param text what undo says about this step, empty for a core which
	is merely moved
*/
ChangeCableCoresCommand::ChangeCableCoresCommand(Cable *cable,
												 const QList<CableCore> &before,
												 const QList<CableCore> &after,
												 const QString &text) :
	m_cable(cable),
	m_before(before),
	m_after(after)
{
	setText(text.isEmpty()
			? QCoreApplication::translate("ChangeCableCoresCommand",
										  "Move a cable core")
			: text);
}

void ChangeCableCoresCommand::undo()
{
	apply(m_before);
}

void ChangeCableCoresCommand::redo()
{
	apply(m_after);
}

/**
	@brief ChangeCableCoresCommand::apply
	Write one whole set of assignments in place of another. The cable
	says it changed, which is what makes the project work out the cable
	field of every conductor it feeds again, so the drawing follows.
	@param cores
*/
void ChangeCableCoresCommand::apply(const QList<CableCore> &cores)
{
	if (m_cable) {
		m_cable->setCores(cores);
	}
}

/**
	@brief RemoveCableCommand::RemoveCableCommand
	@param diagram the folios the lines are drawn on
	@param parts the lines the user asked to delete
*/
RemoveCableCommand::RemoveCableCommand(Diagram *diagram, const QList<CablePart *> &parts) :
	m_diagram(diagram)
{
	setText(QCoreApplication::translate("RemoveCableCommand", "Delete a cable"));

	for (CablePart *part : parts)
	{
		if (!part) continue;

		Entry entry;
		entry.part = part;
		entry.cable = part->cable();
		entry.data.uuid = part->partUuid();
		entry.data.diagram = m_diagram ? m_diagram->uuid() : QUuid();
		entry.data.p1 = part->firstPoint();
		entry.data.p2 = part->secondPoint();
		m_entries.append(entry);

		if (entry.cable && !m_cables.contains(entry.cable)) {
			m_cables.append(entry.cable);
			m_cores.insert(entry.cable.data(), entry.cable->usedCores());
		}
	}
}

/**
	@brief RemoveCableCommand::~RemoveCableCommand
	Whatever the stack leaves behind has to be cleaned up here: a cable
	which is not held by the project any more, and a line which never
	made it back onto a folio, belong to this command.
*/
RemoveCableCommand::~RemoveCableCommand()
{
	QETProject *proj = project();
	for (const QPointer<Cable> &cable : std::as_const(m_cables))
	{
		if (cable && (!proj || !proj->cables().contains(cable))) {
			delete cable;
		}
	}
	for (const Entry &entry : std::as_const(m_entries))
	{
		if (entry.part && !entry.part->scene()) {
			delete entry.part;
		}
	}
}

/**
	@brief RemoveCableCommand::project
	@return the project this command works in, or nullptr
*/
QETProject *RemoveCableCommand::project() const
{
	return m_diagram ? m_diagram->project() : nullptr;
}

/**
	@brief RemoveCableCommand::redo
	Take the lines off their folios. Each cable loses the sections which
	are gone together with the cores drawn on them, and a cable which has
	no section left anywhere leaves the project: a cable nothing draws is
	no cable of this project.
*/
void RemoveCableCommand::redo()
{
	QETProject *proj = project();

	for (const Entry &entry : std::as_const(m_entries))
	{
		if (entry.cable) {
			entry.cable->removePart(entry.data.uuid);
		}
		if (entry.part && entry.part->scene() && m_diagram) {
			m_diagram->removeItem(entry.part);
		}
	}

	if (proj)
	{
		for (const QPointer<Cable> &cable : std::as_const(m_cables))
		{
			if (cable && cable->parts().isEmpty() && proj->cables().contains(cable)) {
				proj->removeCable(cable);
			}
		}
	}

	CableManager::refreshLabels(proj);
}

/**
	@brief RemoveCableCommand::undo
	Put the lines back on their folios, the sections back in their cable
	and the cores back on those sections: exactly the state the delete
	action took away.
*/
void RemoveCableCommand::undo()
{
	QETProject *proj = project();

	if (proj)
	{
		for (const QPointer<Cable> &cable : std::as_const(m_cables))
		{
			if (cable && !proj->cables().contains(cable)) {
				proj->addCable(cable);
			}
		}
	}

	for (const Entry &entry : std::as_const(m_entries))
	{
		if (entry.cable)
		{
			if (!entry.cable->hasPart(entry.data.uuid)) {
				entry.cable->addPart(entry.data);
			}
			for (const CableCore &core : m_cores.value(entry.cable.data())) {
				entry.cable->setCore(core);
			}
		}
		if (entry.part && !entry.part->scene() && m_diagram) {
			m_diagram->addItem(entry.part);
		}
	}

	CableManager::refreshLabels(proj);
}
