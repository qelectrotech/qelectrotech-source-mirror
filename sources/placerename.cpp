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
#include "placerename.h"

#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "dataBase/projectdatabase.h"
#include "diagram.h"
#include "diagramcontext.h"
#include "qetgraphicsitem/structureboxitem.h"
#include "qetinformation.h"
#include "qetproject.h"
#include "titleblockproperties.h"
#include "undocommand/changeelementinformationcommand.h"
#include "undocommand/changetitleblockcommand.h"

#include <QCoreApplication>
#include <QPointer>
#include <QRectF>
#include <algorithm>
#include <QSqlQuery>
#include <QUndoCommand>

namespace
{
	struct Text
	{
		Q_DECLARE_TR_FUNCTIONS(PlaceRename)
	};

		/// One plant counter changing, as one undo step reads it
	struct CounterChange
	{
		QString scheme;
		QString plant;
		QStringList old_values;
		QStringList new_values;
	};

		/// The new-folio title block's plant or location, and the plant
		/// counters. Only that one field of the defaults is written back, so
		/// an undo cannot revert another default changed since
	class ProjectDataCommand : public QUndoCommand
	{
		public:
			ProjectDataCommand(QETProject *project, bool location,
							   const QString &old_name, const QString &new_name,
							   bool defaults, const QList<CounterChange> &counters,
							   QUndoCommand *parent = nullptr) :
				QUndoCommand(parent),
				m_project(project),
				m_location(location),
				m_old_name(old_name),
				m_new_name(new_name),
				m_defaults(defaults),
				m_counters(counters)
			{
				setText(Text::tr("Change the new folio defaults and wire counters"));
			}
			void redo() override {apply(true);}
			void undo() override {apply(false);}

		private:
			void apply(bool redo)
			{
				if (!m_project) return;
				if (m_defaults) {
					TitleBlockProperties defaults = m_project->defaultTitleBlockProperties();
					(m_location ? defaults.locmach : defaults.plant) = redo ? m_new_name : m_old_name;
					m_project->setDefaultTitleBlockProperties(defaults);
				}
					//A plant leaves before the other arrives, so a counter
					//moved onto a name is not removed by the one leaving it
				for (const CounterChange &c : std::as_const(m_counters))
					if (redo ? c.new_values.isEmpty() : c.old_values.isEmpty())
						m_project->setConductorPlantCounter(c.scheme, c.plant, QStringList());
				for (const CounterChange &c : std::as_const(m_counters))
					if (!(redo ? c.new_values.isEmpty() : c.old_values.isEmpty()))
						m_project->setConductorPlantCounter(c.scheme, c.plant,
															redo ? c.new_values : c.old_values);
			}

			QPointer<QETProject> m_project;
			bool m_location;
			QString m_old_name;
			QString m_new_name;
			bool m_defaults;
			QList<CounterChange> m_counters;
	};

	struct BoxPlace
	{
		QString plant;
		QString location;
	};

		/// Where @p box itself lies: its folio's place, then the larger boxes
		/// around it from the outermost in, then its own fields, each only
		/// when not empty. The rule referencePlace() applies to a symbol.
	BoxPlace placeOfBox(const StructureBoxItem *box,
						const QList<const StructureBoxItem *> &boxes,
						const TitleBlockProperties &block)
	{
		const QRectF mine = box->mapRectToScene(box->rect().normalized());
		const qreal area = mine.width() * mine.height();
		QList<QPair<qreal, const StructureBoxItem *>> outer;
		for (const StructureBoxItem *other : boxes) {
			if (other == box) continue;
			const QRectF r = other->mapRectToScene(other->rect().normalized());
			const qreal other_area = r.width() * r.height();
			if (other_area > area && r.contains(mine.center()))
				outer.append({other_area, other});
		}
		std::stable_sort(outer.begin(), outer.end(),
						 [](const auto &a, const auto &b) {return a.first > b.first;});
		BoxPlace place{block.plant, block.locmach};
		for (const auto &entry : std::as_const(outer)) {
			if (!entry.second->plant().isEmpty()) place.plant = entry.second->plant();
			if (!entry.second->location().isEmpty()) place.location = entry.second->location();
		}
		if (!box->plant().isEmpty()) place.plant = box->plant();
		if (!box->location().isEmpty()) place.location = box->location();
		return place;
	}

	bool foliosReady(QETProject *project)
	{
		return project && !project->unloadedFolioCount();
	}

		/// the stored title block of @p diagram matches the request's place
	bool titleBlockMatches(const TitleBlockProperties &p,
						   const PlaceRename::Request &r)
	{
		if (p.plant != r.plant) return false;
		return !r.rename_location || p.locmach == r.location;
	}

	QList<CounterChange> counterChanges(QETProject *project,
										const PlaceRename::Request &r,
										PlaceRename::CounterPolicy policy)
	{
		QList<CounterChange> list;
		if (r.rename_location) return list;
		const auto counters = project->conductorPlantCounters();
		for (auto it = counters.constBegin() ; it != counters.constEnd() ; ++it) {
			const QStringList old_values = it.value().value(r.plant);
			if (old_values.isEmpty()) continue;
			const QStringList target = it.value().value(r.new_name);
			CounterChange leaving{it.key(), r.plant, old_values, QStringList()};
			CounterChange arriving{it.key(), r.new_name, target,
								   PlaceRename::mergeCounter(old_values, target, policy)};
			list << leaving << arriving;
		}
		return list;
	}
}

namespace PlaceRename
{
	int Plan::count() const
	{
		return int(folios.size() + boxes.size() + symbols.size())
				+ (defaults ? 1 : 0) + int(counter_schemes.size());
	}

	/**
		@return the higher of two counter values: the larger number when
		both are numbers, else the longer text, else the later one
	*/
	QString higherCounter(const QString &a, const QString &b)
	{
		bool a_number = false, b_number = false;
		const qlonglong x = a.toLongLong(&a_number);
		const qlonglong y = b.toLongLong(&b_number);
		if (a_number && b_number) return x >= y ? a : b;
		if (a.size() != b.size()) return a.size() > b.size() ? a : b;
		return a >= b ? a : b;
	}

	/**
		@return the counters the new plant keeps, when the old plant's are
		@p old_plant and its own are @p new_plant
	*/
	QStringList mergeCounter(const QStringList &old_plant,
							 const QStringList &new_plant, CounterPolicy policy)
	{
		if (new_plant.isEmpty()) return old_plant;
		if (policy == CounterPolicy::KeepNew) return new_plant;
		if (policy == CounterPolicy::KeepOld) return old_plant;
		QStringList merged;
		const qsizetype n = qMax(old_plant.size(), new_plant.size());
		for (qsizetype i = 0 ; i < n ; ++i) {
			if (i >= old_plant.size()) merged << new_plant.at(i);
			else if (i >= new_plant.size()) merged << old_plant.at(i);
			else merged << higherCounter(old_plant.at(i), new_plant.at(i));
		}
		return merged;
	}

	bool isValid(const Request &request, QString *why)
	{
		auto fail = [why](const QString &text) {
			if (why) *why = text;
			return false;
		};
		const QString name = request.new_name.trimmed();
		if (request.plant.trimmed().isEmpty())
			return fail(Text::tr("A symbol with no plant has nothing to rename."));
		if (request.rename_location && request.location.trimmed().isEmpty())
			return fail(Text::tr("A symbol with no location has nothing to rename."));
		if (name.isEmpty())
			return fail(Text::tr("The new name is empty."));
		if (name != request.new_name)
			return fail(Text::tr("The new name starts or ends with a space."));
		const QString old_name = request.rename_location ? request.location : request.plant;
		if (name == old_name)
			return fail(Text::tr("The new name is the old one."));
		return true;
	}

	/**
		@return what @p request would change. Every folio must be built:
		structure boxes exist only on built folios.
	*/
	Plan collect(QETProject *project, const Request &request)
	{
		Plan plan;
		if (!foliosReady(project) || !isValid(request)) return plan;
		const QList<Diagram *> folios = project->folios();
		for (Diagram *diagram : folios) {
			if (!diagram) continue;
			const TitleBlockProperties block = diagram->border_and_titleblock.exportTitleBlock();
			if (titleBlockMatches(block, request))
				plan.folios << diagram;
			const QList<const StructureBoxItem *> boxes = StructureBoxItem::boxesOf(diagram);
			for (const StructureBoxItem *box : boxes) {
				if (!request.rename_location) {
					if (box->plant() == request.plant)
						plan.boxes << const_cast<StructureBoxItem *>(box);
				} else {
						//Only a box that holds the location itself is edited; one
					//with none takes it from the box or folio around it
					if (box->location() == request.location
							&& placeOfBox(box, boxes, block).plant == request.plant)
						plan.boxes << const_cast<StructureBoxItem *>(box);
				}
			}
		}

			//Symbols: the ones that hold the old name themselves. For a
			//location, only those whose plant, as the views work it out,
			//is the row's
		if (project->dataBase()) {
			project->dataBase()->updateDB();
			QSqlQuery query = project->dataBase()->newQuery(QStringLiteral(
					"SELECT device_uuid, plant FROM qet_devices"));
			const QString key = request.rename_location ? QETInformation::ELMT_LOCATION
														: QETInformation::ELMT_PLANT;
			const QString old_name = request.rename_location ? request.location : request.plant;
			while (query.next()) {
				const QUuid uuid(query.value(0).toString());
				if (uuid.isNull()) continue;
				if (request.rename_location
						&& query.value(1).toString() != request.plant) continue;
				const DiagramContext own = project->dataBase()->elementInformation(uuid);
				if (own.value(key).toString() == old_name)
					plan.symbols << uuid;
			}
		}

		const TitleBlockProperties defaults = project->defaultTitleBlockProperties();
		plan.defaults = titleBlockMatches(defaults, request);
		if (!request.rename_location) {
			const auto counters = project->conductorPlantCounters();
			for (auto it = counters.constBegin() ; it != counters.constEnd() ; ++it)
				if (!it.value().value(request.plant).isEmpty())
					plan.counter_schemes << it.key();
			plan.counter_schemes.sort();
		}
		return plan;
	}

		/// @return true if some wire numbering has counters for both the
		/// old and the new plant: a plant rename has to pick between them
	bool counterClash(QETProject *project, const Request &request)
	{
		if (!project || request.rename_location) return false;
		const auto counters = project->conductorPlantCounters();
		for (auto it = counters.constBegin() ; it != counters.constEnd() ; ++it)
			if (!it.value().value(request.plant).isEmpty()
					&& !it.value().value(request.new_name).isEmpty())
				return true;
		return false;
	}

		/// @return true if the project already has the place the rename
		/// would end in
	bool targetExists(QETProject *project, const Request &request)
	{
		if (!project || !project->dataBase()) return false;
		project->dataBase()->updateDB();
		QSqlQuery query = project->dataBase()->newQuery(QStringLiteral(
				"SELECT plant, location FROM qet_locations"));
		while (query.next()) {
			const QString plant = query.value(0).toString();
			const QString location = query.value(1).toString();
			if (request.rename_location) {
				if (plant == request.plant && location == request.new_name) return true;
			} else if (plant == request.new_name) {
				return true;
			}
		}
		return false;
	}

	/**
		@return one undo step that renames, or nullptr when the project
		stores the old name nowhere. The caller pushes it on the project's
		undo stack and owns it until then.
	*/
	QUndoCommand *command(QETProject *project, const Request &request,
						  CounterPolicy policy)
	{
		const Plan plan = collect(project, request);
		if (!plan.count()) return nullptr;
		const QString what = request.rename_location ? request.location : request.plant;
		auto *parent = new QUndoCommand(request.rename_location
				? Text::tr("Rename location %1 to %2").arg(what, request.new_name)
				: Text::tr("Rename plant %1 to %2").arg(what, request.new_name));
			//QPropertyUndoCommand keeps the pointer: a literal, never a copy
		const char *property = request.rename_location ? "location" : "plant";

		for (Diagram *diagram : plan.folios) {
			const TitleBlockProperties old_block = diagram->border_and_titleblock.exportTitleBlock();
			TitleBlockProperties new_block = old_block;
			(request.rename_location ? new_block.locmach : new_block.plant) = request.new_name;
			new ChangeTitleBlockCommand(diagram, old_block, new_block, parent);
		}
		for (StructureBoxItem *box : plan.boxes)
			new QPropertyUndoCommand(box, property,
									 what, request.new_name, parent);
		const QString key = request.rename_location ? QETInformation::ELMT_LOCATION
													: QETInformation::ELMT_PLANT;
		for (const QUuid &uuid : plan.symbols) {
			const DiagramContext old_info = project->dataBase()->elementInformation(uuid);
			DiagramContext new_info = old_info;
			new_info.addValue(key, request.new_name, old_info.keyMustShow(key));
			new ChangeStoredElementInformationCommand(project, uuid, old_info, new_info, parent);
		}

		const QList<CounterChange> counters = counterChanges(project, request, policy);
		if (plan.defaults || !counters.isEmpty())
			new ProjectDataCommand(project, request.rename_location, what,
								   request.new_name, plan.defaults, counters, parent);
		return parent;
	}
}
