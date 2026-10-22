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
#include "terminalstripautoimport.h"

#include "../elementprovider.h"
#include "../qetgraphicsitem/terminalelement.h"
#include "../qetinformation.h"
#include "../qetproject.h"
#include "../utils/qetutils.h"
#include "UndoCommand/addterminalstripcommand.h"
#include "UndoCommand/addterminaltostripcommand.h"
#include "UndoCommand/sortterminalstripcommand.h"
#include "physicalterminal.h"
#include "realterminal.h"
#include "terminalstrip.h"

#include <QCoreApplication>
#include <QMap>
#include <QUndoStack>

namespace
{
	/**
	 * Terminals going to one strip : a label prefix plus the plant and
	 * the location typed on the terminals. A plant or location set on
	 * the terminals must match the strip exactly, an empty one accepts
	 * every strip of the same name (in most projects the plant and the
	 * location live on the strip, not on the terminals).
	 */
	struct StripGroup
	{
		QVector<QSharedPointer<RealTerminal>> free_terminals;
		QVector<QPair<QSharedPointer<RealTerminal>, TerminalStrip *>> placed_terminals;
		QString name, plant, location;
	};

	QString elementInfo(const QSharedPointer<RealTerminal> &terminal, const QString &key)
	{
		auto element_ = terminal->element();
		if (!element_) {
			return QString();
		}
		return element_->elementData().m_informations.value(key).toString().trimmed();
	}

		//An empty plant is saved and read back as "=" and an empty
		//location as "+" (the placeholders of the title block) : all
		//three spellings mean "no plant" / "no location"
	QString normalizedPlant(const QString &plant_)
	{
		const auto value_ = plant_.trimmed();
		return (value_.isEmpty() || value_ == QLatin1String("="))
				? QString() : value_;
	}

	QString normalizedLocation(const QString &location_)
	{
		const auto value_ = location_.trimmed();
		return (value_.isEmpty() || value_ == QLatin1String("+"))
				? QString() : value_;
	}

	QString groupKey(const QString &name, const QString &plant, const QString &location) {
		return name + QChar(0x1f) + plant + QChar(0x1f) + location;
	}

	/**
	 * @brief stripOf
	 * The strip of the group : same name, and same plant and location
	 * when the terminals carry some. Empty plant and location on the
	 * terminals mean "no opinion" : any strip of the name is reused,
	 * the one already holding terminals winning over an empty one.
	 * @return the strip to work on, null when none has to exist yet.
	 */
	TerminalStrip *stripOf(QETProject *project, const QString &name,
						   const QString &plant, const QString &location)
	{
		const bool strict_ = !plant.isEmpty() || !location.isEmpty();
		TerminalStrip *best_ = nullptr;
		int best_score_ = -1;
		for (auto strip_ : project->terminalStrip())
		{
			if (!strip_ || strip_->name() != name) {
				continue;
			}
			const bool same_ =
				normalizedPlant(strip_->installation()) == plant &&
				normalizedLocation(strip_->location())  == location;
			if (strict_ && !same_) {
				continue;
			}
				//Same plant and location first, then the strip already
				//holding terminals over an empty one
			const int score_ = (same_ ? 4 : 0)
					+ (strip_->realTerminals().isEmpty() ? 0 : 1);
			if (score_ > best_score_) {
				best_ = strip_;
				best_score_ = score_;
			}
		}
		return best_;
	}
}

/**
 * @brief TerminalStripAutoImport::importFreeTerminals
 * Put the terminals of @a project in the strip named after their label
 * prefix ("X1:2" goes to the strip "X1"), creating the strip when it
 * does not exist yet. The whole run is one undoable step.
 *
 * @a move_placed picks the action : false only places free terminals,
 * placed ones are never touched. True also moves a placed terminal
 * whose label prefix or plant and location no longer match its strip.
 *
 * In both cases the order the user gave to a strip is never changed,
 * only a strip created by the run itself is sorted -- the way the plan
 * is drawn : folio by folio, then left to right (see
 * SortTerminalStripCommand::planOrderLessThan).
 */
TerminalStripAutoImport::ImportResult TerminalStripAutoImport::importFreeTerminals(
	QETProject *project, bool move_placed)
{
	if (!project) {
		return ImportResult{};
	}

		//Group every real terminal of the project, the free ones and
		//the ones already in a strip alike
	QMap<QString, StripGroup> groups_;
	const auto collect_ = [&groups_](const QSharedPointer<RealTerminal> &real_,
									 TerminalStrip *current_)
	{
		if (!real_ || !real_->element()) {
			return;		//Spare terminals (no element) are left alone
		}
		const auto name_ = stripNameForLabel(real_->label());
		if (name_.isEmpty()) {
			return;
		}
		const auto plant_ = normalizedPlant(
			elementInfo(real_, QETInformation::ELMT_PLANT));
		const auto location_ = normalizedLocation(
			elementInfo(real_, QETInformation::ELMT_LOCATION));

		auto &group_ = groups_[groupKey(name_, plant_, location_)];
		group_.name = name_;
		group_.plant = plant_;
		group_.location = location_;
		if (current_) {
			group_.placed_terminals.append({real_, current_});
		} else {
			group_.free_terminals.append(real_);
		}
	};

	const ElementProvider provider(project);
	for (auto terminal_ : provider.freeTerminal()) {
		if (terminal_) {
			collect_(terminal_->realTerminal(), nullptr);
		}
	}
	for (auto strip_ : project->terminalStrip())
	{
		if (!strip_) {
			continue;
		}
		for (const auto &real_ : strip_->realTerminals()) {
			collect_(real_, strip_);
		}
	}
	if (groups_.isEmpty()) {
		return ImportResult{};
	}

	for (auto it_ = groups_.begin(); it_ != groups_.end(); ++it_)
	{
			//The plan order : folio by folio, then left to right, so the
			//strip comes out the way the plan is drawn instead of label
			//by label ("N" and "PE" stay where they are on the sheet).
			//SortTerminalStripCommand, pushed below for a strip this run
			//creates, sorts with the same rule and cannot undo it.
		auto by_plan_ = [](const QSharedPointer<RealTerminal> &a_,
						   const QSharedPointer<RealTerminal> &b_) {
			return SortTerminalStripCommand::planOrderLessThan(a_, b_);
		};
		std::sort(it_.value().free_terminals.begin(),
				  it_.value().free_terminals.end(), by_plan_);
		std::sort(it_.value().placed_terminals.begin(),
				  it_.value().placed_terminals.end(),
				  [&by_plan_](const auto &a_, const auto &b_) {
			return by_plan_(a_.first, b_.first);
		});
	}

	auto names_ = groups_.keys();
	std::sort(names_.begin(), names_.end(), QETUtils::naturalLessThan);

		//First phase : work out what has to change, without touching the
		//project. When there is nothing to do, nothing is pushed : no
		//undo step is recorded and the project stays untouched.
	struct Plan
	{
		const StripGroup *group{nullptr};
		TerminalStrip    *strip{nullptr};	//Target strip, null : to create
	};
	QVector<Plan> plans;

	for (const auto &key_ : std::as_const(names_))
	{
		auto &group_ = groups_[key_];
		Plan plan_;
		plan_.group = &group_;
		plan_.strip = stripOf(project, group_.name, group_.plant, group_.location);

			//Free terminals always, placed ones only when the action
			//is allowed to re-group them
		bool changed_ = !group_.free_terminals.isEmpty();
		if (move_placed) {
			for (const auto &entry_ : std::as_const(group_.placed_terminals)) {
				changed_ = changed_ || entry_.second != plan_.strip;
			}
		}
		if (changed_) {
			plans.append(plan_);
		}
	}
	if (plans.isEmpty()) {
		return ImportResult{};
	}

		//Second phase : the changes, as one undoable step
	int added_ = 0, moved_ = 0, created_strips_ = 0;
	auto undo_ = project->undoStack();
	undo_->beginMacro(move_placed
		? QCoreApplication::translate("TerminalStripAutoImport",
									  "Re-group all terminals")
		: QCoreApplication::translate("TerminalStripAutoImport",
									  "Assign free terminals"));

	for (const auto &plan_ : std::as_const(plans))
	{
		const auto &group_ = *plan_.group;

		bool created_ = false;
		auto strip_ = plan_.strip;
		if (!strip_) {
			strip_ = project->newTerminalStrip(
				group_.plant, group_.location, group_.name);
			undo_->push(new AddTerminalStripCommand(strip_, project));
			++created_strips_;
			created_ = true;
		}

		if (!group_.free_terminals.isEmpty()) {
			undo_->push(new AddTerminalToStripCommand(group_.free_terminals, strip_));
			added_ += group_.free_terminals.size();
		}
			//A terminal whose label prefix or plant and location no
			//longer match its strip moves to the right one, but only
			//when the action is allowed to re-group placed terminals
		if (move_placed)
		{
			for (const auto &entry_ : std::as_const(group_.placed_terminals))
			{
				if (entry_.second == strip_) {
					continue;
				}
				undo_->push(new MoveTerminalCommand(
					entry_.first->physicalTerminal(), entry_.second, strip_));
				++moved_;
			}
		}
			//A strip just created is sorted, a strip already known
			//keeps the order its user gave it
		if (created_) {
			undo_->push(new SortTerminalStripCommand(strip_));
		}
	}

	undo_->endMacro();
	ImportResult result_;
	result_.terminals = added_ + moved_;
	result_.strips = created_strips_;
	return result_;
}
