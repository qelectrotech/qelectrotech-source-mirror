/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#include "plcioprojectionservice.h"

#include "diagram.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "qetgraphicsitem/element.h"

#include <QHash>

#include <algorithm>

namespace {

QString elementLabel(Element *element)
{
	if (!element) {
		return {};
	}

	const QString label = element->elementInformations()
		.value(QETInformation::ELMT_LABEL)
		.toString();
	if (!label.isEmpty()) {
		return label;
	}

	return element->actualLabel();
}

int folioOf(QETProject &project, Element *element)
{
	return element && element->diagram() ? project.folioIndex(element->diagram()) : -1;
}

PlcIoProjection::Direction directionFor(ElementData::PlcIOType type)
{
	switch (type) {
		case ElementData::EntreeDigitale:
		case ElementData::EntreeAnalogique:
		case ElementData::EntreeUniverselle:
			return PlcIoProjection::Input;
		case ElementData::SortieDigitale:
		case ElementData::SortieAnalogique:
		case ElementData::SortieUniverselle:
			return PlcIoProjection::Output;
	}
	return PlcIoProjection::Input;
}

void addWarning(PlcIoProjection &projection, const QString &warning)
{
	if (!projection.warnings.contains(warning)) {
		projection.warnings << warning;
	}
}

void addIoWarnings(PlcIoProjection &projection, const ElementData::PlcIO &io)
{
	if (projection.address.trimmed().isEmpty()) {
		projection.empty_address = true;
		addWarning(projection, QStringLiteral("empty address"));
	}

	if (!io.terminals.isEmpty() && io.terminals.size() != io.terminalCount) {
		projection.terminal_label_count_mismatch = true;
		addWarning(
			projection,
			QStringLiteral("terminal label count %1 does not match terminal_count %2")
				.arg(io.terminals.size())
				.arg(io.terminalCount));
	}
}

QList<Element *> plcMastersInProject(QETProject &project)
{
	QList<Element *> result;

	for (Diagram *diagram : project.diagrams()) {
		if (!diagram) {
			continue;
		}

		for (Element *element : diagram->elements()) {
			if (!element || element->linkType() != Element::Master) {
				continue;
			}

			if (element->elementData().m_master_type == ElementData::PLC) {
				result << element;
			}
		}
	}

	std::sort(result.begin(), result.end(), [&project](Element *left, Element *right) {
		const int left_folio = folioOf(project, left);
		const int right_folio = folioOf(project, right);
		if (left_folio != right_folio) {
			return left_folio < right_folio;
		}
		const QString left_label = elementLabel(left);
		const QString right_label = elementLabel(right);
		if (left_label != right_label) {
			return left_label < right_label;
		}
		return left->uuid().toString() < right->uuid().toString();
	});

	return result;
}

PlcIoProjection projectionForChannel(
	QETProject &project,
	Element *master,
	const ElementData::PlcIO &io,
	int index)
{
	PlcIoProjection projection;
	projection.master_uuid = master->uuid();
	projection.master_label = elementLabel(master);
	projection.folio = folioOf(project, master);
	projection.io_index = index;
	projection.type = io.type;
	projection.direction = directionFor(io.type);
	projection.address = io.address;
	projection.function = io.functionText;
	projection.comment = io.comment;
	projection.terminal_count = io.terminalCount;
	projection.terminal_labels = io.effectiveTerminals();
	addIoWarnings(projection, io);
	return projection;
}

} // namespace

QList<PlcIoProjection> PlcIoProjectionService::channels(QETProject &project) const
{
	QList<PlcIoProjection> result;

	for (Element *master : plcMastersInProject(project)) {
		const ElementData::PlcMasterData plc_data = master->elementData().plcMasterData();
		QHash<int, QList<Element *>> slaves_by_group;
		QList<Element *> out_of_range_slaves;

		for (Element *slave : master->linkedElementsReadOnly()) {
			const int group_index = master->groupIndexForElement(slave);
			if (group_index >= 0 && group_index < plc_data.ios.size()) {
				slaves_by_group[group_index] << slave;
			} else {
				out_of_range_slaves << slave;
			}
		}

		for (int index = 0; index < plc_data.ios.size(); ++index) {
			QList<Element *> slaves = slaves_by_group.value(index);
			std::sort(slaves.begin(), slaves.end(), [](Element *left, Element *right) {
				const QString left_label = elementLabel(left);
				const QString right_label = elementLabel(right);
				if (left_label != right_label) {
					return left_label < right_label;
				}
				return left->uuid().toString() < right->uuid().toString();
			});

			if (slaves.isEmpty()) {
				PlcIoProjection projection = projectionForChannel(
					project,
					master,
					plc_data.ios.at(index),
					index);
				projection.unlinked = true;
				addWarning(projection, QStringLiteral("unlinked"));
				result << projection;
				continue;
			}

			for (Element *slave : slaves) {
				PlcIoProjection projection = projectionForChannel(
					project,
					master,
					plc_data.ios.at(index),
					index);
				projection.linked_slave_uuid = slave->uuid();
				projection.linked_slave_label = elementLabel(slave);
				projection.linked_slave_folio = folioOf(project, slave);
				projection.duplicate_group_index = slaves.size() > 1;
				if (projection.duplicate_group_index) {
					addWarning(
						projection,
						QStringLiteral("duplicate group_index %1 assignment")
							.arg(index));
				}
				result << projection;
			}
		}

		std::sort(out_of_range_slaves.begin(), out_of_range_slaves.end(), [master](Element *left, Element *right) {
			const int left_group = master->groupIndexForElement(left);
			const int right_group = master->groupIndexForElement(right);
			if (left_group != right_group) {
				return left_group < right_group;
			}
			return left->uuid().toString() < right->uuid().toString();
		});

		for (Element *slave : out_of_range_slaves) {
			PlcIoProjection projection;
			projection.master_uuid = master->uuid();
			projection.master_label = elementLabel(master);
			projection.folio = folioOf(project, master);
			projection.io_index = master->groupIndexForElement(slave);
			projection.linked_slave_uuid = slave ? slave->uuid() : QUuid();
			projection.linked_slave_label = elementLabel(slave);
			projection.linked_slave_folio = folioOf(project, slave);
			projection.out_of_range_group_index = true;
			addWarning(
				projection,
				QStringLiteral("group_index %1 out of range")
					.arg(projection.io_index));
			result << projection;
		}
	}

	return result;
}
