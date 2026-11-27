// SPDX-License-Identifier: GPL-2.0-or-later
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
#include "placecopy.h"

#include "autoNum/elementautonumschemecommand.h"
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "diagramcontext.h"
#include "itemgroups.h"
#include "placerename.h"
#include "qetgraphicsitem/conductor.h"
#include "qetgraphicsitem/diagramimageitem.h"
#include "qetgraphicsitem/dynamicelementtextitem.h"
#include "qetgraphicsitem/element.h"
#include "qetgraphicsitem/independenttextitem.h"
#include "qetgraphicsitem/qetshapeitem.h"
#include "qetgraphicsitem/structureboxitem.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "titleblockproperties.h"

#include <QCoreApplication>
#include <QDomDocument>
#include <QHash>
#include <QPointer>
#include <QSet>
#include <QSqlQuery>
#include <QUndoCommand>
#include <QUndoStack>

namespace
{
	struct Text
	{
		Q_DECLARE_TR_FUNCTIONS(PlaceCopy)
	};

		/// The new plant's wire counters, one numbering scheme
	struct CounterChange
	{
		QString scheme;
		QStringList old_values;
		QStringList new_values;
	};

		/// Gives the copy the wire counters of the plant it was made from
	class CopyCountersCommand : public QUndoCommand
	{
		public:
			CopyCountersCommand(QETProject *project, const QString &plant,
								const QList<CounterChange> &changes) :
				m_project(project),
				m_plant(plant),
				m_changes(changes)
			{
				setText(Text::tr("Give the copied plant its wire counters"));
			}
			void redo() override {apply(true);}
			void undo() override {apply(false);}

		private:
			void apply(bool redo)
			{
				if (!m_project) return;
				for (const CounterChange &c : std::as_const(m_changes))
					m_project->setConductorPlantCounter(c.scheme, m_plant,
														redo ? c.new_values : c.old_values);
			}

			QPointer<QETProject> m_project;
			QString m_plant;
			QList<CounterChange> m_changes;
	};

		/**
			A copied contact or folio report whose partner was not copied
			(it lies outside the plant) is linked to nothing: like a folio
			duplicate's, it loses the text it showed from that partner, so
			it does not name a device of the plant that does not exist.
		*/
	void forgetPartner(Element *element)
	{
		DiagramContext info = element->elementInformations();
		for (const QString &key : {QStringLiteral("formula"), QStringLiteral("label"),
								   QStringLiteral("comment"), QStringLiteral("location")})
			info.addValue(key, QString());
		for (const QString &key : {
				QETInformation::ELMT_PLC_TYPE, QETInformation::ELMT_PLC_ADDRESS,
				QETInformation::ELMT_PLC_FUNCTION, QETInformation::ELMT_PLC_COMMENT,
				QETInformation::ELMT_PLC_CROSSREF, QETInformation::ELMT_PLC_TC,
				QETInformation::ELMT_PLC_T1, QETInformation::ELMT_PLC_T2,
				QETInformation::ELMT_PLC_T3, QETInformation::ELMT_PLC_T4,
				QStringLiteral("xref")})
			info.remove(key);
		element->setElementInformations(info);
		for (DynamicElementTextItem *text : element->dynamicTextItems()) {
			if (text->textFrom() != DynamicElementTextItem::UserText) {
				text->m_block_alignment = true;
				text->setPlainText(QString());
				text->m_block_alignment = false;
			}
		}
	}

		/// Fills the folios @p copies with what @p sources hold, under
		/// @p new_name where they held @p plant
	void fill(QETProject *project, const QString &plant, const QString &new_name,
			  const QList<Diagram *> &sources, const QList<Diagram *> &copies)
	{
		const QString plant_key = QETInformation::ELMT_PLANT;
		QList<Element *> new_elements;
		for (qsizetype i = 0 ; i < sources.size() ; ++i) {
			Diagram *source = sources.at(i);
			Diagram *copy = copies.at(i);
			copy->setTitleBlockTemplate(
						source->border_and_titleblock.titleBlockTemplateName());
			TitleBlockProperties block = source->border_and_titleblock.exportTitleBlock();
			block.plant = new_name;
			copy->border_and_titleblock.importTitleBlock(block);
			copy->border_and_titleblock.importBorder(
						source->border_and_titleblock.exportBorder());
			copy->defaultConductorProperties = source->defaultConductorProperties;
				//The folio's own numbering: fromXml() reads it only with the
				//folio's informations, which would also take its uuid
			copy->setConductorsAutonumName(source->conductorsAutonumName());
			copy->setFreezeNewElements(source->freezeNewElements());
			copy->setFreezeNewConductors(source->freezeNewConductors());
			copy->m_elmt_unitfolio_max = source->m_elmt_unitfolio_max;
			copy->m_elmt_tenfolio_max = source->m_elmt_tenfolio_max;
			copy->m_elmt_hundredfolio_max = source->m_elmt_hundredfolio_max;
			copy->m_cnd_unitfolio_max = source->m_cnd_unitfolio_max;
			copy->m_cnd_tenfolio_max = source->m_cnd_tenfolio_max;
			copy->m_cnd_hundredfolio_max = source->m_cnd_hundredfolio_max;

				//As the folio duplication: is_copy_command writes the
				//texts of slaves, reports and conductor definitions as the
				//copy has to read them
			QDomDocument doc = source->toXml(true, true);
			QDomElement root = doc.documentElement();
			copy->fromXml(root, QPointF(0, 0), false, nullptr);

			for (QGraphicsItem *item : copy->items())
				if (auto *element = dynamic_cast<Element *>(item))
					new_elements << element;

				//The boxes that held the plant hold the new one
			for (const StructureBoxItem *box : StructureBoxItem::boxesOf(copy))
				if (box->plant() == plant)
					const_cast<StructureBoxItem *>(box)->setPlant(new_name);
		}

			//Still under the source uuids: the copies find each other,
			//across the copied folios, and nothing outside them
		for (Element *element : std::as_const(new_elements))
			element->initLink(new_elements);

		QHash<QUuid, QUuid> renewed_groups;
		for (Diagram *copy : std::as_const(copies)) {
			for (QGraphicsItem *item : copy->items()) {
				if (auto *element = dynamic_cast<Element *>(item)) {
					element->newUuid();
					if (element->linkType() == Element::Slave
							|| element->linkType() & Element::AllReport
							|| element->linkType() == Element::ConductorDefinition)
						copy->restoreText(element);
					if (element->linkedElements().isEmpty()
							&& (element->linkType() == Element::Slave
								|| element->linkType() & Element::AllReport))
						forgetPartner(element);
					DiagramContext info = element->elementInformations();
					if (info.value(plant_key).toString() == plant) {
						info.addValue(plant_key, new_name, info.keyMustShow(plant_key));
						const QList<DynamicElementTextItem *> texts = element->dynamicTextItems();
						for (DynamicElementTextItem *text : texts)
							text->m_block_alignment = true;
						element->setElementInformations(info);
						for (DynamicElementTextItem *text : texts)
							text->m_block_alignment = false;
					}
				}
				else if (auto *text = dynamic_cast<IndependentTextItem *>(item))
					text->newUuid();
				else if (auto *image = dynamic_cast<DiagramImageItem *>(item))
					image->newUuid();
				else if (auto *shape = dynamic_cast<QetShapeItem *>(item))
					shape->newUuid();
				else if (auto *conductor = dynamic_cast<Conductor *>(item))
						//Its text, the wire number, is kept as a label is
					conductor->newUuid();

				const QUuid group = ItemGroups::groupOf(item);
				if (!group.isNull()) {
					if (!renewed_groups.contains(group))
						renewed_groups.insert(group, QUuid::createUuid());
					copy->setItemGroup(item, renewed_groups.value(group));
				}
			}
		}
			//Formulas follow the project's numbering schemes; the labels
			//themselves are not numbered again
		ElementAutoNumSchemeCommand::linkPasted(project, new_elements);
	}

	bool plantExists(QETProject *project, const QString &plant)
	{
		PlaceRename::Request request;
		request.plant = QStringLiteral("-");
		request.new_name = plant;
		return PlaceRename::targetExists(project, request);
	}
}

namespace PlaceCopy
{
	bool isValid(QETProject *project, const QString &plant,
				 const QString &new_name, QString *why)
	{
		auto fail = [why](const QString &text) {
			if (why) *why = text;
			return false;
		};
		if (!project) return fail(QString());
		if (project->isReadOnly())
			return fail(Text::tr("The project is read-only."));
		if (plant.trimmed().isEmpty())
			return fail(Text::tr("A symbol with no plant has no plant to copy."));
		const QString name = new_name.trimmed();
		if (name.isEmpty())
			return fail(Text::tr("The new name is empty."));
		if (name != new_name)
			return fail(Text::tr("The new name starts or ends with a space."));
		if (name == plant)
			return fail(Text::tr("The new name is the old one."));
			//Copying onto a plant that exists would give the copies the
			//designations its own symbols have
		if (plantExists(project, name))
			return fail(Text::tr("The plant \"%1\" already exists.").arg(name));
		return true;
	}

	/**
		@return the folios a copy of @p plant would copy, and how many of
		their symbols lie in another plant. Every folio must be built.
	*/
	Plan collect(QETProject *project, const QString &plant)
	{
		Plan plan;
		if (!project || project->unloadedFolioCount() || plant.isEmpty()) return plan;
		QSet<int> numbers;
		const QList<Diagram *> folios = project->folios();
		for (Diagram *diagram : folios) {
			if (diagram && diagram->border_and_titleblock.plant() == plant) {
				plan.folios << diagram;
				numbers.insert(project->folioIndex(diagram) + 1);
			}
		}
		if (plan.folios.isEmpty() || !project->dataBase()) return plan;
		project->dataBase()->updateDB();
		QSqlQuery query = project->dataBase()->newQuery(QStringLiteral(
				"SELECT folio_number, plant FROM qet_devices"));
		while (query.next())
			if (numbers.contains(query.value(0).toInt())
					&& query.value(1).toString() != plant)
				++plan.other_plant_symbols;
		return plan;
	}

	/**
		Copies @p plant to @p new_name as one step of the project's undo
		stack. Every folio must be built.
		@return false, and nothing done, when it cannot (see isValid()) or
		no folio has the plant in its title block
	*/
	bool copy(QETProject *project, const QString &plant, const QString &new_name)
	{
		if (!isValid(project, plant, new_name)) return false;
		const Plan plan = collect(project, plant);
		if (plan.folios.isEmpty()) return false;

		QList<CounterChange> counters;
		const auto all = project->conductorPlantCounters();
		for (auto it = all.constBegin() ; it != all.constEnd() ; ++it) {
			const QStringList source = it.value().value(plant);
			if (source.isEmpty()) continue;
			const QStringList target = it.value().value(new_name);
			counters << CounterChange{it.key(), target,
				PlaceRename::mergeCounter(source, target,
										  PlaceRename::CounterPolicy::MergeHigher)};
		}

			//One macro: undo takes the folios out whole, redo puts the
			//same folios back, so their content is made only once
		QUndoStack *stack = project->undoStack();
		stack->beginMacro(Text::tr("Copy plant %1 to %2").arg(plant, new_name));
		int position = -1;
		for (Diagram *source : plan.folios)
			position = qMax(position, project->folioIndex(source));
		QList<Diagram *> copies;
		for (qsizetype i = 0 ; i < plan.folios.size() ; ++i) {
			Diagram *copy = project->addNewDiagram(++position);
			if (copy) copies << copy;
		}
		if (copies.size() != plan.folios.size()) {
				//A folio could not be added: take back the ones that were
			stack->endMacro();
			stack->undo();
			return false;
		}
		fill(project, plant, new_name, plan.folios, copies);
		if (!counters.isEmpty())
			stack->push(new CopyCountersCommand(project, new_name, counters));
		stack->endMacro();
		return true;
	}
}
