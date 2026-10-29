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
#include "bridgecopy.h"

#include "addbridgecommand.h"
#include "bridge.h"
#include "bridgepart.h"
#include "bridgemanager.h"
#include "../diagram.h"
#include "../qetproject.h"

#include <QDomDocument>
#include <QColor>

namespace {
	const QString block_name = QStringLiteral("bridge_copies");
	const QString unit_tag = QStringLiteral("bridge");
	const QString line_tag = QStringLiteral("line");
}

/**
	@brief BridgeCopy::write
	@param diagram
	@param document
*/
void BridgeCopy::write(Diagram *diagram, QDomDocument &document)
{
	if (!diagram || document.documentElement().isNull()) {
		return;
	}

		//The lines of one bridge travel as one unit: what is pasted is
		//a bridge of its own holding all of them
	QList<Bridge *> units;
	QList<QList<BridgePart *>> lines;
	for (QGraphicsItem *item : diagram->selectedItems())
	{
		auto *part = qgraphicsitem_cast<BridgePart *>(item);
		if (!part || !part->bridge()) {
			continue;
		}
		int at = units.indexOf(part->bridge());
		if (at < 0) {
			units << part->bridge();
			lines << QList<BridgePart *>();
			at = units.size() - 1;
		}
		lines[at] << part;
	}
	if (units.isEmpty()) {
		return;
	}

	QDomElement block = document.createElement(block_name);
	document.documentElement().appendChild(block);
	for (int i = 0 ; i < units.size() ; ++i)
	{
		Bridge *bridge = units.at(i);
		QDomElement unit = document.createElement(unit_tag);
		unit.setAttribute(QStringLiteral("color"), bridge->color().name());
		unit.setAttribute(QStringLiteral("line_width"), QString::number(bridge->lineWidth()));
		unit.setAttribute(QStringLiteral("max"), QString::number(bridge->maxConnections()));
		unit.setAttribute(QStringLiteral("name"), bridge->name());
		unit.setAttribute(QStringLiteral("tension"), bridge->tensionText());
		block.appendChild(unit);
		for (BridgePart *part : lines.at(i))
		{
			const QLineF line = part->line();
			QDomElement line_elmt = document.createElement(line_tag);
			line_elmt.setAttribute(QStringLiteral("x1"), QString::number(line.x1()));
			line_elmt.setAttribute(QStringLiteral("y1"), QString::number(line.y1()));
			line_elmt.setAttribute(QStringLiteral("x2"), QString::number(line.x2()));
			line_elmt.setAttribute(QStringLiteral("y2"), QString::number(line.y2()));
			unit.appendChild(line_elmt);
		}
	}
}

/**
	@brief BridgeCopy::read
	@param diagram
	@param root
	@return the bridges built, not held by the project yet
*/
QList<Bridge *> BridgeCopy::read(Diagram *diagram, const QDomElement &root)
{
	QList<Bridge *> built;
	QETProject *project = diagram ? diagram->project() : nullptr;
	if (!project) {
		return built;
	}
	const QDomElement block = root.firstChildElement(block_name);
	for (QDomElement unit = block.firstChildElement(unit_tag) ;
		 !unit.isNull() ; unit = unit.nextSiblingElement(unit_tag))
	{
		Bridge *bridge = new Bridge(project);
		bridge->setColor(QColor(unit.attribute(QStringLiteral("color"), QStringLiteral("#000000"))));
		bridge->setLineWidth(unit.attribute(QStringLiteral("line_width"), QStringLiteral("1")).toDouble());
		bridge->setMaxConnections(unit.attribute(QStringLiteral("max")).toInt());
		bridge->setName(unit.attribute(QStringLiteral("name")));
		bridge->setTensionText(unit.attribute(QStringLiteral("tension")));

		for (QDomElement line_elmt = unit.firstChildElement(line_tag) ;
			 !line_elmt.isNull() ; line_elmt = line_elmt.nextSiblingElement(line_tag))
		{
			BridgePartData data;
			data.uuid = QUuid::createUuid();
			data.diagram = diagram->uuid();
			data.p1 = QPointF(line_elmt.attribute(QStringLiteral("x1")).toDouble(),
							  line_elmt.attribute(QStringLiteral("y1")).toDouble());
			data.p2 = QPointF(line_elmt.attribute(QStringLiteral("x2")).toDouble(),
							  line_elmt.attribute(QStringLiteral("y2")).toDouble());
			bridge->addPart(data);

			auto *item = new BridgePart(data);
			item->setBridge(bridge);
			diagram->addItem(item);
		}
		if (bridge->parts().isEmpty()) {
			delete bridge;
			continue;
		}
		built << bridge;
	}
	return built;
}

/**
	@brief BridgeCopy::land
	@param diagram
	@param bridge
	@return the commands which put the lines of @p bridge down
*/
QList<QUndoCommand *> BridgeCopy::land(Diagram *diagram, Bridge *bridge)
{
	QList<QUndoCommand *> commands;
	QETProject *project = diagram ? diagram->project() : nullptr;
	if (!project || !bridge) {
		return commands;
	}

		//What the copy was made of says what it is: it lands on nothing
		//yet, so the wires cannot say it first
	const QString carried = bridge->tensionText();
	const QList<BridgeTerminal> after =
			BridgeManager::terminalsOfBridge(project, bridge);

		//What the rules of a bridge refuse leaves the line standing
		//where it is: it lands on nothing until the hand pulls it into
		//a place it may be built
	int added = 0;
	for (const BridgeTerminal &terminal : after) {
		if (!terminal.reference && !bridge->hasTerminal(terminal)) {
			++added;
		}
	}
	const QStringList strips = BridgeManager::clashingStrips(project, after);
	const int column = BridgeManager::freeColumn(project, after);
	if ((added && bridge->wouldExceed(added)) || !strips.isEmpty() || !column) {
		return commands;
	}

	bridge->setColumn(column);
	bridge->setTerminals(after);

		//One step for the whole of it: the lines of the unit and what
		//the circle says of itself
	auto *first = static_cast<QUndoCommand *>(nullptr);
	const auto parts = bridge->parts();
	for (const BridgePartData &data : parts)
	{
		BridgePart *item = nullptr;
		for (BridgePart *candidate : BridgePart::allParts()) {
			if (candidate && candidate->partUuid() == data.uuid) {
				item = candidate;
				break;
			}
		}
		auto *command = new AddBridgeCommand(bridge, item, diagram, true,
											QList<BridgeTerminal>(),
											BridgeInformation{carried},
											BridgeInformation{carried});
		if (!first) {
			first = command;
			BridgeManager::writeInformation(project, bridge->terminals(),
											BridgeInformation{carried}, command);
		}
		commands << command;
	}
	return commands;
}
