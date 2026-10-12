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
#include "cablecopy.h"

#include "cablemanager.h"
#include "cablepart.h"

#include "../autoNum/assignvariables.h"
#include "../autoNum/numerotationcontext.h"
#include "../diagram.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetproject.h"

#include <QDomDocument>
#include <QSet>

#include <utility>

/**
	@brief CableCopy::write
	Puts the cable lines selected on @a diagram into @a document, so a
	copy carries them the way it carries the elements and the texts of
	the same selection.

	Only the line which was picked is written, together with the whole
	cable it belongs to: the copy needs everything the cable knows, but
	only the one section it is standing on.
	@param diagram the folio being copied
	@param document the fragment the clipboard is about to hold
*/
void CableCopy::write(Diagram *diagram, QDomDocument &document)
{
	if (!diagram) return;
	QDomElement root = document.documentElement();
	if (root.isNull()) return;

	QList<CablePart *> parts;
	for (QGraphicsItem *item : diagram->selectedItems()) {
		if (auto *part = qgraphicsitem_cast<CablePart *>(item)) {
			parts << part;
		}
	}
	if (parts.isEmpty()) return;

	QDomElement block = document.createElement(QStringLiteral("cables"));
	bool anything = false;
	for (CablePart *part : std::as_const(parts))
	{
		Cable *cable = part->cable();
		if (!cable) continue;

		QDomElement element = cable->toXml(document);
		const QPointF p1 = part->firstPoint();
		const QPointF p2 = part->secondPoint();
		element.setAttribute(QStringLiteral("line"), part->partUuid().toString());
		element.setAttribute(QStringLiteral("x1"), QString::number(p1.x(), 'f', 2));
		element.setAttribute(QStringLiteral("y1"), QString::number(p1.y(), 'f', 2));
		element.setAttribute(QStringLiteral("x2"), QString::number(p2.x(), 'f', 2));
		element.setAttribute(QStringLiteral("y2"), QString::number(p2.y(), 'f', 2));
		block.appendChild(element);
		anything = true;
	}
	if (anything) root.appendChild(block);
}

/**
	@brief CableCopy::read
	Builds a cable of its own for every line @a root carries, each of
	them standing exactly where the fragment says it stood.

	The cable keeps everything the source cable knows -- type,
	installation, place, length, how many cores, which colours -- and it
	also keeps where the colour labels of the copied section stand, so a
	copy arrives as a whole: onto a folio which holds no wire at all,
	into an empty part of a sheet, wherever. What it does not keep is
	any wire: a copy may not take a wire away from the cable it was
	copied from, not even for as long as it is being built, so every
	core is let go while the cable still answers to the source's
	identity -- no conductor of the source is ever repointed, not even
	for the length of one signal.

	Which wire each carried core belongs to is worked out later, in
	CableCopy::wire(), because that depends on where the line has
	finally been put -- and a copy put where there is no wire simply
	keeps its labels, to be wired when the line is pulled into place.
	@param diagram the folio which is about to hold those lines
	@param root the &lt;diagram&gt; element of a copy fragment
	@return the lines built, not yet wired
*/
QList<CablePart *> CableCopy::read(Diagram *diagram, const QDomElement &root)
{
	QList<CablePart *> built;
	if (!diagram || !diagram->project()) return built;

	const QDomElement block = root.firstChildElement(QStringLiteral("cables"));
	if (block.isNull()) return built;

	QETProject *project = diagram->project();
		//One rule for the whole run. Every cable of a paste takes a
		//number from it and steps it on, so several cables pasted at
		//once do not all end up under the same number -- although the
		//project's own rule does not move until those cables really
		//exist, which is where the commands which add them take over.
		//A counter bound to the folio starts the folio the copies are
		//pasted onto at its own beginning, once for the whole run.
	NumerotationContext rule = autonum::resetFolioCounters(
				project->cableAutoNum(project->cableCurrentAutoNum()));
	for (QDomElement element = block.firstChildElement(QStringLiteral("cable"));
		 !element.isNull();
		 element = element.nextSiblingElement(QStringLiteral("cable")))
	{
		if (element.attribute(QStringLiteral("x1")).isEmpty()) continue;

		Cable *cable = project->newCable();
		cable->fromXml(element);

			//The section of the copy, standing where the section it was
			//copied from stood: the caller moves it to its final place,
			//and the colour labels come with it (CablePart::itemChange)
		CablePartData own;
		own.uuid = QUuid::createUuid();
		own.diagram = diagram->uuid();
		own.p1 = QPointF(element.attribute(QStringLiteral("x1")).toDouble(),
						 element.attribute(QStringLiteral("y1")).toDouble());
		own.p2 = QPointF(element.attribute(QStringLiteral("x2")).toDouble(),
						 element.attribute(QStringLiteral("y2")).toDouble());

			//The colour labels of that one section, carried over whole
			//and with every wire taken away
		const QUuid source_part(element.attribute(QStringLiteral("line")));
		QList<CableCore> carried;
		for (const CableCore &core : cable->usedCores())
		{
			if (core.part != source_part) continue;

			CableCore copy = core;
			copy.conductor = QUuid();
			copy.part = own.uuid;
			carried.append(copy);
		}
			//Goes first, while the copy still answers to the source's
			//identity: that way nothing of the source is left pointing
			//at a cable which is about to become somebody else
		cable->setCores(carried);

			//The copy is a cable of its own: nothing of the source's
			//identity, nothing of its sections and nothing of the wires
			//it wires may be taken over, only what the user filled in
		for (const CablePartData &other : cable->parts()) {
			cable->removePart(other.uuid);
		}
		cable->setUuid(QUuid::createUuid());
			//The copy is numbered by the rule like every other cable.
			//A project which has no rule gets no number invented for it,
			//and then what the copy was copied from keeps standing there
			//rather than being wiped out by an empty one.
		const QString designation =
			CableManager::nextDesignation(project, diagram, rule);
		if (!designation.isEmpty()) {
			cable->setDesignation(designation);
				//The copy is numbered by the rule, so whatever the
				//source was called by hand is not carried over
			cable->setDesignationByHand(false);
		}
		cable->addPart(own);

		auto *item = new CablePart(own);
		item->setCable(cable);
		diagram->addItem(item);
		built << item;
	}
	return built;
}

/**
	@brief CableCopy::wire
	Give the cores of every line in @a parts the wire each of them
	stands on now that the line is where it is going to stay.

	A copy arrives whole: it carries the colour labels it was copied
	with, and this is the moment they are matched against the wires of
	the new place. Matching never moves a label: each one goes on
	standing exactly where it was carried, and the wire it stands on --
	within reach, as everywhere else -- is the one which takes the cable
	field. A label standing on no wire at all simply goes on
	standing there -- that is what makes a copy useful onto a folio
	which holds no wire yet: the line is there, with its colours, ready
	to be pulled into place, and pulling it there wires it
	(CablePart::settleCores).

	A piece of wire belongs to one cable only, and a copy never takes
	one away: the wires of the folio it lands on which another cable
	already describes are left to that cable, nothing is asked about
	them and nothing is written for them. The wires which came along
	with the copy itself are different -- they are new wires on this
	folio, carrying the cable field they had back on the folio they
	come from, where they used to be one of that cable's own wires. If
	that were still believed here, every one of them would look like a
	wire another cable holds and the copy would be refused all of them,
	arriving without a single entry although it is standing on wires of
	its own. So they are let go of first, exactly like read() lets go
	of the wires the cores used to name.
	@param diagram the folio the lines are on
	@param parts the lines built by CableCopy::read()
	@param fresh the wires which came with this copy
	@return one entry per line which is going to stay, with what it
	had to give back to other cables, ready to be put on the undo stack
*/
QList<CableCopy::Wired> CableCopy::wire(Diagram *diagram,
										const QList<CablePart *> &parts,
										const QList<Conductor *> &fresh)
{
	QList<Wired> wired;
	if (!diagram || !diagram->project()) {
		for (CablePart *part : parts) discard(diagram, part);
		return wired;
	}
	QETProject *project = diagram->project();

		//A wire which came with this copy is a new wire here: what it
		//says about a cable describes the folio it was copied from.
		//Done by identity rather than by position, because the wire it
		//was copied from may well stand in this very place -- and that
		//one goes on belonging to its own cable.
	for (Conductor *conductor : fresh)
	{
		if (!conductor) continue;
		if (conductor->properties().m_cable_uuid.isNull()) continue;


		conductor->setCableReference(QString(), QUuid(), -1);
	}

	for (CablePart *part : parts)
	{
		Cable *cable = part ? part->cable() : nullptr;
		if (!cable) {
			discard(diagram, part);
			continue;
		}

			//One wire a colour label is dropped on. Two labels may
			//not be given the same wire, so each one is handed out
			//once.
		struct Offered
		{
			int core;
			Conductor *conductor;
		};
		QList<Offered> offered;
		QSet<QUuid> wires;

		const QLineF line(part->firstPoint(), part->secondPoint());
		const QList<CableCrossing> crossings = CableManager::crossedConductors(diagram, line);
		for (const CableCore &core : cable->usedCores())
		{
			if (core.part != part->partUuid()) continue;
			if (!core.conductor.isNull()) continue;

				//The wire this label stands on, worked out the way it
				//is worked out everywhere else (CablePart::wireFor):
				//the nearest place this line runs across a wire, close
				//enough for the slash and the wire to read as one
				//thing. The label itself is not moved to reach it --
				//a copy arrives with its colours where they were
				//carried, and no gesture moves one of them, so here
				//too the entry follows the label rather than the
				//label being sent off to find a wire. When that wire
				//has already been handed to another label of the same
				//copy, the next nearest one this label stands on wins,
				//rather than leaving it with no wire although one is
				//under it.
			QList<CableCrossing> open = crossings;
			Conductor *conductor = nullptr;
			while (true)
			{
				Conductor *nearest = part->wireFor(open, core, core.position);
				if (!nearest) break;
				if (!wires.contains(nearest->uuid())) {
					conductor = nearest;
					break;
				}
				for (int i = open.size() - 1; i >= 0; --i) {
					if (open.at(i).conductor == nearest) open.removeAt(i);
				}
			}

			if (!conductor) continue;

				//A wire another cable already describes stays with that
				//cable. Taking it away would empty a cable behind the
				//user's back, and asking about it on every paste turned
				//putting a copy into an existing drawing into a question
				//to answer first -- so a copy simply takes the wires
				//nobody holds, and its colour labels standing on the
				//others name nothing at all. An entry follows its label,
				//and here it comes to a stop. He can give such a label
				//one of those wires himself afterwards, by dragging it
				//there, which is where the question is still asked.
			const QUuid holder_uuid = conductor->properties().m_cable_uuid;
			if (!holder_uuid.isNull() && holder_uuid != cable->uuid())
			{
				const Cable *previous = CableManager::cableByUuid(project, holder_uuid);
				if (previous
					&& previous->hasCore(conductor->properties().m_cable_slot))
				{
					continue;
				}
			}

			wires.insert(conductor->uuid());
			Offered item;
			item.core = core.core;
			item.conductor = conductor;
			offered.append(item);
		}

		QList<Bound> bound;
		for (const Offered &item : std::as_const(offered))
		{
				//A wire is one piece of wire: whichever cable held it
				//before has to let it go.
			CableManager::claimConductor(project, item.conductor, cable);

				//A cable binds a conductor by its identity, so a
				//conductor read back from a file written before it had a
				//persistent one is given one to keep from now on.
			item.conductor->setUuid(item.conductor->uuid());

			CableCore core = cable->core(item.core);
			core.conductor = item.conductor->uuid();
			cable->setCore(core);

			Bound bind;
			bind.core = item.core;
			bind.conductor = item.conductor;
			bound.append(bind);
		}

		Wired wired_line;
		wired_line.part = part;
		wired_line.bound = bound;
		wired.append(wired_line);
	}
	return wired;
}

/**
	@brief CableCopy::repoint
	Work out again which wire each core of @a wired is on, using the
	identity the wire has right now.

	A copy is put on the undo stack just after it was wired, and the
	first run of that stack renews the identity of every conductor it
	pasted (PasteDiagramCommand::redo): a copy must not keep the
	identity of the conductor it was copied from. The cores of the copy
	would then go on naming an identity which no longer exists, and the
	next working out of the labels (CableManager::refreshLabels) would
	find nothing there and take the cable field away from those wires.

	So the cores are pointed at the very same wires again, with their
	new identity, and the cable fields are written out from the cables
	one more time afterwards: what the copy was given as it was put
	down still names the identity its wires had a moment earlier, and
	on a folio where those wires have their own new identity that
	leaves them without an entry although the copy is standing on them.
	Nothing moves and nothing is asked: an untouched copy is not
	changed by any of this at all.
	@param wired the lines CableCopy::wire() just gave wires to
*/
void CableCopy::repoint(const QList<Wired> &wired)
{
	for (const Wired &line : wired)
	{
		Cable *cable = line.part ? line.part->cable() : nullptr;
		if (!cable) continue;

		for (const Bound &bind : line.bound)
		{
			if (!bind.conductor) continue;

			const QUuid now = bind.conductor->uuid();
			const CableCore core = cable->core(bind.core);
			if (core.core != bind.core || core.conductor == now) continue;


			CableCore updated = core;
			updated.conductor = now;
			cable->setCore(updated);
		}
	}

		//With every core naming the identity its wire really has now,
		//the cable fields are written out from the cables once more:
		//this is what puts the entries a copy was given as it was put
		//down onto the wires it has actually landed on.
	Diagram *folio = nullptr;
	for (const Wired &line : wired) {
		if (line.part && line.part->diagram()) {
			folio = line.part->diagram();
			break;
		}
	}
	if (folio && folio->project()) {
		CableManager::refreshLabels(folio->project());
	}
}

/**
	@brief CableCopy::discard
	Take a line away again whole -- off the folio, out of the project,
	and deleted -- without putting anything on the undo stack, because
	nothing really happened: the user said no to the question above.

	Whatever the cable said while it was being built must not be left
	behind either: the project has to go on claiming exactly what it
	claimed before, or it would ask to be saved although nothing was
	changed at all.
	@param diagram the folio it was built on
	@param part the line to take away, may be null
*/
void CableCopy::discard(Diagram *diagram, CablePart *part)
{
	if (!part) return;
	Cable *cable = part->cable();
	QETProject *project = diagram ? diagram->project() : nullptr;

	if (part->scene()) part->scene()->removeItem(part);

	if (cable && project && project->cables().contains(cable))
	{
		const bool options_modified = project->projectOptionsWereModified();
		project->removeCable(cable);
		project->setModified(options_modified);
	}

	delete part;
	delete cable;
}
