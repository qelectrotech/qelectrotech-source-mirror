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
#include "cablerenumber.h"

#include "cable.h"
#include "../autoNum/assignvariables.h"
#include "../autoNum/numerotationcontextcommands.h"
#include "../diagram.h"
#include "../qetproject.h"

#include <QCoreApplication>
#include <QSet>

#include <algorithm>
#include <limits>

namespace {

/**
	@brief One cable of the project, and where it stands so that the
	cables can be laid out in an order everybody can see again.
*/
struct OrderedCable
{
	Cable *cable = nullptr;
		///The lowest folio the cable is drawn on
	int folio = 0;
		///How far it reaches to the left and how far to the top on
		///that folio
	qreal left = 0.0;
	qreal top = 0.0;
		///The folio the rule is read on for this cable
	Diagram *diagram = nullptr;
};

/**
	@brief Read a rule once, the way the numbering of a cable reads it.

	The rule is read on a copy, since reading it fills in what its
	folio-bound parts are worth on that one folio, and that must not
	creep into the rule itself.
	@param context the rule as it stands at this point of the numbering
	@param diagram the folio the number is worked out on
	@param key the name the rule is kept under in the project
	@return the number, empty when the rule says nothing here
*/
QString readRule(NumerotationContext context, Diagram *diagram, const QString &key)
{
	const QString formula = autonum::numerotationContextToFormula(context);
	autonum::sequentialNumbers seq;
	autonum::setSequential(formula, seq, context, diagram, key);
	return autonum::AssignVariables::formulaToLabel(formula, seq, diagram);
}

} // namespace

/**
	@brief CableRenumber::byHandCount
	@param project
	@return how many cables of that project carry a name typed in by hand
*/
int CableRenumber::byHandCount(QETProject *project)
{
	if (!project) return 0;

	int count = 0;
	for (Cable *cable : project->cables())
	{
		if (cable && cable->designationByHand()) {
			++count;
		}
	}
	return count;
}

/**
	@brief CableRenumber::plan
	Work the number of every cable of the project out, without writing
	anywhere. The cables are put in the order they are numbered in --
	folio by folio, and on each folio along whichever axis leads -- and
	the rule is then read from its beginning once for each of them, so
	that the numbering runs on without a hole.
	@param project the project whose cables are numbered
	@param rule the numbering rule to read
	@param x_axis_first true when the leftmost cable of a folio comes first
	@param include_by_hand false to leave the names typed in by hand alone
	@return the numbers, or what stopped the numbering
*/
CableRenumberPlan CableRenumber::plan(QETProject *project,
									  const NumerotationContext &rule,
									  bool x_axis_first,
									  bool include_by_hand)
{
	CableRenumberPlan result;
	if (!project)
	{
		result.error = QCoreApplication::translate(
			"CableRenumber", "Aucun projet n'est ouvert.");
		return result;
	}
	if (project->isReadOnly())
	{
		result.error = QCoreApplication::translate(
			"CableRenumber",
			"Ce projet est en lecture seule : ses câbles ne peuvent pas "
			"être renumérotés.");
		return result;
	}
		//The rule may be one the numbering window is drawing right now
		//and has not written into the project yet: the numbering writes
		//it together with the numbers themselves, so the project does
		//not have to hold it beforehand. What the counters are kept
		//under is the name the rule is stored with, which for cables is
		//fixed either way -- so numbering starts from a project which
		//has no rule saved yet just as well.
	QString key = project->cableCurrentAutoNum();
	if (key.isEmpty()) {
		key = QETProject::cableAutoNumRuleName();
	}
	if (rule.isEmpty())
	{
		result.error = QCoreApplication::translate(
			"CableRenumber",
			"Aucune règle de numérotation des câbles n'est définie pour ce "
			"projet. Définissez-en une dans cette fenêtre avant de "
			"renuméroter ses câbles.");
		return result;
	}

		//Where every cable stands, and which ones are left alone: their
		//names stay as they are, so the numbering must not hand the
		//same number out again
	QList<OrderedCable> ordered;
	QSet<QString> taken;
	for (Cable *cable : project->cables())
	{
		if (!cable) continue;
		if (!include_by_hand && cable->designationByHand())
		{
			if (!cable->designation().isEmpty()) {
				taken.insert(cable->designation());
			}
			continue;
		}

		OrderedCable entry;
		entry.cable = cable;

		for (const CablePartData &part : cable->parts())
		{
			Diagram *diagram = project->diagramByUuid(part.diagram);
			if (!diagram) continue;

			const int index = project->folioIndex(diagram);
			const qreal left = qMin(part.p1.x(), part.p2.x());
			const qreal top = qMin(part.p1.y(), part.p2.y());

			if (!entry.diagram || index < entry.folio)
			{
				entry.folio = index;
				entry.diagram = diagram;
				entry.left = left;
				entry.top = top;
			} else if (index == entry.folio) {
				entry.left = qMin(entry.left, left);
				entry.top = qMin(entry.top, top);
			}
		}
			//A cable which stands on no folio the project still knows
			//is numbered last rather than first by accident
		if (!entry.diagram) {
			entry.folio = std::numeric_limits<int>::max();
		}
		ordered.append(entry);
	}

		//Folio by folio, and on each folio along the axis which leads:
		//the other axis is only asked when the leading one says two
		//cables are level, so that the numbering never depends on the
		//order the cables happen to be stored in
	std::sort(ordered.begin(), ordered.end(),
			  [x_axis_first](const OrderedCable &a, const OrderedCable &b)
	{
		if (a.folio != b.folio) return a.folio < b.folio;

		const qreal a_first = x_axis_first ? a.left : a.top;
		const qreal b_first = x_axis_first ? b.left : b.top;
		if (a_first != b_first) return a_first < b_first;

		const qreal a_second = x_axis_first ? a.top : a.left;
		const qreal b_second = x_axis_first ? b.top : b.left;
		if (a_second != b_second) return a_second < b_second;

		return a.cable->uuid().toString() < b.cable->uuid().toString();
	});

		//Read the rule from the beginning, once for each cable: its
		//counters are put back first, since numbering a project over
		//starts at the beginning whatever the last number was
	NumerotationContext context = autonum::resetContextCounters(rule);
	int current_folio = -1;

	for (const OrderedCable &entry : std::as_const(ordered))
	{
		if (!entry.diagram)
		{
			result.error = QCoreApplication::translate(
				"CableRenumber",
				"Un câble n'est sur aucun folio du projet : rien n'a été "
				"changé.");
			return result;
		}

			//A counter bound to the folio counts from the beginning of
			//that folio: the first cable of a folio gets the first
			//number of it, and the folios after it start again rather
			//than carrying the number of the folio before on
		if (entry.folio != current_folio)
		{
			context = autonum::resetFolioCounters(context);
			current_folio = entry.folio;
		}

		const QString candidate = readRule(context, entry.diagram, key);
		if (candidate.isEmpty())
		{
			result.error = QCoreApplication::translate(
				"CableRenumber",
				"La règle en cours ne donne aucun numéro ici : rien n'a été "
				"changé.");
			return result;
		}
			//A rule which does not count hands out the same number
			//twice, and two cables of one project may not be called
			//the same thing
		if (result.designations.contains(candidate))
		{
			result.error = QCoreApplication::translate(
				"CableRenumber",
				"La règle en cours donne deux fois le même numéro (%1) : "
				"elle doit compter pour renuméroter un projet entier. "
				"Rien n'a été changé.")
				.arg(candidate);
			return result;
		}
			//A name which stays because it was typed in keeps its
			//number, and the numbering may not hand that number out
		if (taken.contains(candidate))
		{
			result.error = QCoreApplication::translate(
				"CableRenumber",
				"Le numéro %1 est déjà porté par un câble dont le nom est "
				"laissé tel quel : rien n'a été changé.")
				.arg(candidate);
			return result;
		}

		taken.insert(candidate);
		result.cables.append(entry.cable);
		result.designations.append(candidate);

			//Step the rule on for the next cable; a rule which cannot
			//step stays where it is and is caught by the check above
		NumerotationContextCommands ncc(context, entry.diagram);
		const NumerotationContext following = ncc.next();
		if (following.size() == context.size()) {
			context = following;
		}
	}

		//Where the rule stands after the last cable has been numbered:
		//the caller writes that into the project, so numbering goes on
		//from there instead of from wherever the rule happened to stand
		//before this numbering ran
	result.after = context;
	return result;
}
