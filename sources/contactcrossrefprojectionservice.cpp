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
#include "contactcrossrefprojectionservice.h"

#include "diagram.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/masterelement.h"

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

ContactGroupProjection groupProjection(const ElementData::SlaveContactGroup &group, int index)
{
	ContactGroupProjection projection;
	projection.index = index;
	projection.type = group.type;
	projection.subtype = group.subtype;
	projection.contact_count = group.contactCount;
	projection.terminal_count = group.terminalCount;
	projection.terminal_labels = group.labels;
	return projection;
}

QList<Element *> mastersInProject(QETProject &project)
{
	QList<Element *> result;

	const QList<Diagram *> diagrams = project.diagrams();
	for (Diagram *diagram : diagrams) {
		if (!diagram) {
			continue;
		}

		const QList<Element *> elements = diagram->elements();
		for (Element *element : elements) {
			if (element && element->linkType() == Element::Master) {
				result << element;
			}
		}
	}

	std::sort(result.begin(), result.end(), [](Element *left, Element *right) {
		const Diagram *left_diagram = left ? left->diagram() : nullptr;
		const Diagram *right_diagram = right ? right->diagram() : nullptr;
		const QETProject *project = left_diagram ? left_diagram->project() : nullptr;
		const int left_folio = project && left_diagram ? project->folioIndex(left_diagram) : -1;
		const int right_folio = project && right_diagram ? project->folioIndex(right_diagram) : -1;
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

QString typeName(ElementData::SlaveState state)
{
	switch (state) {
		case ElementData::NO: return QStringLiteral("NO");
		case ElementData::NC: return QStringLiteral("NC");
		case ElementData::SW: return QStringLiteral("SW");
		case ElementData::Other: break;
	}
	return QStringLiteral("Other");
}

ContactUsage::Type usageType(ElementData::SlaveState state)
{
	switch (state) {
		case ElementData::NO: return ContactUsage::NO;
		case ElementData::NC: return ContactUsage::NC;
		case ElementData::SW: return ContactUsage::SW;
		case ElementData::Other: break;
	}
	return ContactUsage::Other;
}

} // namespace

QList<ContactMasterProjection> ContactCrossRefProjectionService::masters(QETProject &project) const
{
	QList<ContactMasterProjection> result;

	for (Element *element : mastersInProject(project)) {
		const ElementData data = element->elementData();
		ContactMasterProjection master;
		master.uuid = element->uuid();
		master.label = elementLabel(element);
		master.folio = element->diagram() ? project.folioIndex(element->diagram()) : -1;
		master.master_type = data.m_master_type;
		master.linked_slave_count = element->linkedElementsReadOnly().size();

		for (int i = 0; i < data.m_slave_contact_groups.size(); ++i) {
			const auto group = groupProjection(data.m_slave_contact_groups.at(i), i);
			master.groups << group;
			master.capacity.addSlave(usageType(group.type), group.contact_count);
		}

		if (auto *master_element = dynamic_cast<MasterElement *>(element)) {
			master.usage = master_element->contactUsage();
			master.capacity = master_element->contactCapacity();
		}

		if (master.groups.isEmpty()) {
			master.validation_messages << QStringLiteral("master declares no slave contact groups");
		}

		for (Element *slave : element->linkedElementsReadOnly()) {
			const int group_index = element->groupIndexForElement(slave);
			if (group_index < 0) {
				master.validation_messages << QStringLiteral("linked slave %1 has no group_index")
					.arg(slave ? slave->uuid().toString() : QStringLiteral("<null>"));
			} else if (group_index >= master.groups.size()) {
				master.validation_messages << QStringLiteral("linked slave %1 references out-of-range group_index %2")
					.arg(slave ? slave->uuid().toString() : QStringLiteral("<null>"))
					.arg(group_index);
			}
		}

		result << master;
	}

	return result;
}

QList<ContactAssignmentProjection> ContactCrossRefProjectionService::assignments(QETProject &project) const
{
	QList<ContactAssignmentProjection> result;

	for (Element *master : mastersInProject(project)) {
		const ElementData master_data = master->elementData();
		QList<Element *> linked = master->linkedElementsReadOnly();
		std::sort(linked.begin(), linked.end(), [master](Element *left, Element *right) {
			const int left_group = master->groupIndexForElement(left);
			const int right_group = master->groupIndexForElement(right);
			if (left_group != right_group) {
				return left_group < right_group;
			}
			const QString left_label = elementLabel(left);
			const QString right_label = elementLabel(right);
			if (left_label != right_label) {
				return left_label < right_label;
			}
			return left->uuid().toString() < right->uuid().toString();
		});

		for (Element *slave : linked) {
			if (!slave) {
				continue;
			}

			const ElementData slave_data = slave->elementData();
			ContactAssignmentProjection assignment;
			assignment.master_uuid = master->uuid();
			assignment.slave_uuid = slave->uuid();
			assignment.master_label = elementLabel(master);
			assignment.slave_label = elementLabel(slave);
			assignment.master_folio = master->diagram() ? project.folioIndex(master->diagram()) : -1;
			assignment.slave_folio = slave->diagram() ? project.folioIndex(slave->diagram()) : -1;
			assignment.group_index = master->groupIndexForElement(slave);
			assignment.slave_contact_type = slave_data.m_slave_state;
			assignment.slave_contact_subtype = slave_data.m_slave_type;
			assignment.slave_contact_count = slave_data.m_contact_count;

			if (assignment.group_index >= 0
				&& assignment.group_index < master_data.m_slave_contact_groups.size()) {
				assignment.group_index_resolves = true;
				assignment.group = groupProjection(
					master_data.m_slave_contact_groups.at(assignment.group_index),
					assignment.group_index);
				if (assignment.group.type != assignment.slave_contact_type) {
					assignment.validation_messages
						<< QStringLiteral("slave contact type %1 differs from group type %2")
							.arg(typeName(assignment.slave_contact_type),
								 typeName(assignment.group.type));
				}
			} else if (assignment.group_index < 0) {
				assignment.validation_messages << QStringLiteral("missing group_index");
			} else {
				assignment.validation_messages
					<< QStringLiteral("out-of-range group_index %1").arg(assignment.group_index);
			}

			result << assignment;
		}
	}

	return result;
}
