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
#include "elementautonumschemecommand.h"

#include "assignvariables.h"
#include "numerotationcontextcommands.h"
#include "../diagram.h"
#include "../qetgraphicsitem/element.h"
#include "../qetinformation.h"
#include "../qetproject.h"

#include <QRegularExpression>

#include <algorithm>
#include <optional>

namespace {
bool sameContext(const NumerotationContext &a, const NumerotationContext &b)
{
	if (a.size() != b.size()) return false;
	for (int i = 0; i < a.size(); ++i) {
		if (a[i] != b[i]) return false;
	}
	return true;
}
} // namespace

ElementAutoNumSchemeCommand::ElementAutoNumSchemeCommand(QETProject *project) :
	m_project(project)
{}

/**
	@brief ElementAutoNumSchemeCommand::nameProblem
	@return why @p name cannot be the name of an element numbering scheme
	of @p project, translated, or an empty string if it can
	@param ignored_title : the scheme being renamed, if any
*/
QString ElementAutoNumSchemeCommand::nameProblem(const QETProject *project,
												 const QString &name,
												 const QString &ignored_title)
{
	if (QETProject::normalizedAutoNumName(name).isEmpty()) {
		return tr("The numbering name cannot be empty.");
	}
	if (!project) {
		return QString();
	}
	const QString clash = project->elementAutoNumNameClash(name, ignored_title);
	if (!clash.isEmpty()) {
		return tr("A numbering named “%1” already exists.").arg(clash);
	}
	return QString();
}

/**
	@brief ElementAutoNumSchemeCommand::sameSequentialParts
	@return true if both formulas use the same sequential numbers
	(%sequ_1, %seqtf_2...), so the numbers an element holds for one still
	fit the other
*/
bool ElementAutoNumSchemeCommand::sameSequentialParts(const QString &formula_a,
													  const QString &formula_b)
{
	static const QRegularExpression rx(QStringLiteral("%seq[a-z]+_\\d+"));
	const auto tokens = [](const QString &formula) {
		QStringList list;
		auto it = rx.globalMatch(formula);
		while (it.hasNext()) {
			list << it.next().captured(0);
		}
		list.sort();
		list.removeDuplicates();
		return list;
	};
	return tokens(formula_a) == tokens(formula_b);
}

/**
	@brief ElementAutoNumSchemeCommand::resetForRenumber
	@return @p context with every number part set back to 1, the other
	parts unchanged, to number elements again from the start
*/
NumerotationContext ElementAutoNumSchemeCommand::resetForRenumber(const NumerotationContext &context)
{
	NumerotationContext out = context;
	for (int i = 0; i < out.size(); ++i) {
		const QStringList parts = out.itemAt(i);
		if (parts.isEmpty()) continue;
		if (out.keyIsNumber(parts.at(0))) {
			out.replaceValue(i, QStringLiteral("1"));
		}
	}
	return out;
}

/**
	@brief ElementAutoNumSchemeCommand::frozenFollowers
	@return the elements with a frozen label which follow the element
	numbering scheme @p title
*/
QVector<Element *> ElementAutoNumSchemeCommand::frozenFollowers(
		const QETProject *project,
		const QString &title)
{
	QVector<Element *> list;
	if (!project) {
		return list;
	}
	const auto followers = project->elementsUsingElementAutoNum(title);
	for (Element *el : followers) {
		if (el->isFreezeLabel()) {
			list << el;
		}
	}
	return list;
}

/**
	@brief ElementAutoNumSchemeCommand::editBlockedBy
	@return the elements with a frozen label which stop the element
	numbering scheme @p title from being given the definition @p context:
	none when its formula stays the same (a rename, a change of the
	counter), otherwise every frozen follower
*/
QVector<Element *> ElementAutoNumSchemeCommand::editBlockedBy(
		const QETProject *project,
		const QString &title,
		const NumerotationContext &context)
{
	if (!project || !project->elementAutoNum().contains(title)) {
		return {};
	}
	if (project->elementAutoNumFormula(title)
			== autonum::numerotationContextToFormula(context)) {
		return {};
	}
	return frozenFollowers(project, title);
}

/**
	@brief ElementAutoNumSchemeCommand::renumberChanges
	Number @p elements again, in folio and position order, with @p context
	started from 1, and give them @p formula.
	@param hash_key : key of the scheme in the folios' maxima of folio
	sequential numbers (its title)
	@param final_context : if not null, receives the context after the
	last element, to store as the scheme's counter
	@return one change per element
*/
QVector<RenumberElementsCommand::ElementChange> ElementAutoNumSchemeCommand::renumberChanges(
		const QString &hash_key,
		const NumerotationContext &context,
		QVector<Element *> elements,
		const QString &formula,
		NumerotationContext *final_context,
		bool from_start,
		const QUuid &scheme_id,
		const QSet<QString> &reserved)
{
	QVector<RenumberElementsCommand::ElementChange> changes;
	std::sort(elements.begin(), elements.end(),
			  [](Element *a, Element *b){ return comparPos(a, b); });

	NumerotationContext nc = from_start ? resetForRenumber(context) : context;
	for (Element *el : std::as_const(elements))
	{
		RenumberElementsCommand::ElementChange ch;
		ch.element = el;
		ch.old_infos = el->elementInformations();
		ch.old_seq = el->sequenceStruct();
		ch.old_frozen = el->isFreezeLabel();
		ch.new_frozen = ch.old_frozen;

			//The next number which does not give the element a label an
			//element left as it is already has. A numbering whose number
			//parts cannot move on (fixed text only) has nothing to skip
			//to: it keeps its label.
		autonum::sequentialNumbers new_seq;
		QString label;
		for (int attempt = 0 ; ; ++attempt)
		{
			new_seq = autonum::sequentialNumbers();
			autonum::setSequential(formula, new_seq, nc, el->diagram(), hash_key);
			label = autonum::AssignVariables::formulaToLabel(
						formula, new_seq, el->diagram(), el, nullptr);
			if (label.isEmpty() || !reserved.contains(label) || attempt >= 100000) {
				break;
			}
			NumerotationContextCommands skip(nc);
			const NumerotationContext advanced = skip.next();
			if (sameContext(advanced, nc)) {
				break;
			}
			nc = advanced;
		}

		DiagramContext new_infos = ch.old_infos;
		new_infos.addValue(QETInformation::ELMT_FORMULA, formula);
		if (!scheme_id.isNull()) {
			new_infos.addValue(QETInformation::ELMT_FORMULA_ID,
							   scheme_id.toString(), false);
		}
		new_infos.addValue(QETInformation::ELMT_LABEL, label);
		ch.new_infos = new_infos;
		ch.new_seq = new_seq;
		changes << ch;

		NumerotationContextCommands ncc(nc);
		nc = ncc.next();
	}
	if (final_context) {
		*final_context = nc;
	}
	return changes;
}

/**
	@brief ElementAutoNumSchemeCommand::reservedLabels
	@return the labels the elements following the scheme @p title keep
	while @p changing change: numbers an operation on @p changing must not
	give again, or two elements would carry the same label
*/
QSet<QString> ElementAutoNumSchemeCommand::reservedLabels(
		const QETProject *project,
		const QString &title,
		const QVector<Element *> &changing)
{
	QSet<QString> labels;
	if (!project) {
		return labels;
	}
	const QSet<const Element *> moving(changing.constBegin(), changing.constEnd());
	const auto followers = project->elementsUsingElementAutoNum(title);
	for (const Element *el : followers)
	{
		if (moving.contains(el)
				|| el->linkType() == Element::Slave
				|| (el->linkType() & Element::AllReport)) {
			continue;
		}
		const QString label = el->elementInformations().value(QETInformation::ELMT_LABEL).toString();
		if (!label.isEmpty()) {
			labels << label;
		}
	}
	return labels;
}

/**
	@brief ElementAutoNumSchemeCommand::renumber
	Number again, from the first number, the elements following the
	element numbering scheme @p title, or every scheme if @p title is
	empty, in folio and position order. One undo step.

	An element whose label is frozen is left as it is, and its label is not
	given to another element: the numbering goes past it.
	@param frozen : if not null, receives the elements left as they are
	because their label is frozen
	@param text : caption of the undo step
	@return the command, nullptr if there is nothing to number
*/
RenumberElementsCommand *ElementAutoNumSchemeCommand::renumber(
		QETProject *project,
		const QString &title,
		QVector<Element *> *frozen,
		const QString &text)
{
	if (frozen) frozen->clear();
	if (!project || project->isReadOnly()) {
		return nullptr;
	}

	QVector<RenumberElementsCommand::ElementChange> changes;
	QHash<QString, NumerotationContext> old_ctx;
	QHash<QString, NumerotationContext> new_ctx;

	QStringList titles = title.isEmpty() ? QStringList(project->elementAutoNum().keys())
										 : QStringList{title};
	titles.sort(Qt::CaseInsensitive);
	for (const QString &key : std::as_const(titles))
	{
		if (!project->elementAutoNum().contains(key)) {
			continue;
		}

		QVector<Element *> elements;
		const auto followers = project->elementsUsingElementAutoNum(key);
		for (Element *el : followers)
		{
			if (el->linkType() == Element::Slave || (el->linkType() & Element::AllReport)) {
				continue;
			}
			if (el->isFreezeLabel()) {
				if (frozen) *frozen << el;
				continue;
			}
			elements << el;
		}
		if (elements.isEmpty()) {
			continue;
		}

		const NumerotationContext tmpl = project->elementAutoNum(key);
		NumerotationContext final_ctx;
		changes << renumberChanges(key, tmpl, elements,
								   autonum::numerotationContextToFormula(tmpl),
								   &final_ctx, true, QUuid(),
								   reservedLabels(project, key, elements));
		old_ctx.insert(key, tmpl);
		new_ctx.insert(key, final_ctx);
	}
	if (changes.isEmpty()) {
		return nullptr;
	}
	return new RenumberElementsCommand(project, changes, old_ctx, new_ctx, text);
}

/**
	@brief ElementAutoNumSchemeCommand::numberSupport
	Giving an element a number by hand makes sense when the numbers of a
	scheme are one sequence: a definition with exactly one number part, a
	"unit", "ten" or "hundred" (a folio number, a cycle or letters would make
	"which number is free" depend on more than one value).
*/
ElementAutoNumSchemeCommand::NumberSupport ElementAutoNumSchemeCommand::numberSupport(
		const NumerotationContext &context)
{
	NumberSupport support;
	int numbers = 0;
	for (int i = 0 ; i < context.size() ; ++i)
	{
		const QStringList part = context.itemAt(i);
		const QString type = part.value(0);
		if (type == QLatin1String("unit") || type == QLatin1String("ten")
				|| type == QLatin1String("hundred")) {
			++numbers;
			support.partIndex = i;
			support.type = type;
			support.increase = std::max(1, part.value(2).toInt());
		} else if (type == QLatin1String("unitfolio") || type == QLatin1String("tenfolio")
				   || type == QLatin1String("hundredfolio") || type == QLatin1String("alpha")
				   || type == QLatin1String("wrap")) {
			return NumberSupport();
		}
	}
	support.supported = numbers == 1;
	if (!support.supported) {
		return NumberSupport();
	}
	return support;
}

/// @return the number @p element was given by the scheme, if it knows it
std::optional<int> ElementAutoNumSchemeCommand::numberOf(const NumberSupport &support,
														 const Element *element)
{
	if (!support.supported || !element) {
		return std::nullopt;
	}
	const autonum::sequentialNumbers seq = element->sequenceStruct();
	const QStringList &list = support.type == QLatin1String("unit") ? seq.unit
							: support.type == QLatin1String("ten") ? seq.ten
							: seq.hundred;
	bool ok = false;
	const int number = list.value(0).toInt(&ok);
	if (!ok) {
		return std::nullopt;
	}
	return number;
}

namespace {
/// The elements of @p project which follow the scheme @p title and are not slaves or reports
QVector<Element *> numberedFollowers(const QETProject *project, const QString &title)
{
	QVector<Element *> list;
	const auto followers = project->elementsUsingElementAutoNum(title);
	for (Element *el : followers) {
		if (el->linkType() != Element::Slave && !(el->linkType() & Element::AllReport)) {
			list << el;
		}
	}
	return list;
}
} // namespace

/**
	@brief ElementAutoNumSchemeCommand::labelsHeldBesides
	@return the labels the elements of @p project carry, @p element apart:
	those a number given to @p element must not turn into
*/
QSet<QString> ElementAutoNumSchemeCommand::labelsHeldBesides(const QETProject *project,
															 const Element *element)
{
	QSet<QString> labels;
	if (!project) {
		return labels;
	}
	for (Diagram *d : project->diagrams()) {
		for (QGraphicsItem *item : d->items()) {
			if (auto *el = qgraphicsitem_cast<Element *>(item)) {
				if (el != element) {
					labels << el->elementInformations().value(QETInformation::ELMT_LABEL).toString();
				}
			}
		}
	}
	labels.remove(QString());
	return labels;
}

/**
	@brief ElementAutoNumSchemeCommand::counterConflict
	Giving the numbering @p title the counter of @p proposed, with the same
	formula it has, makes its next element meet numbers in use when elements
	of the numbering have a number of at least that counter. Those numbers
	are skipped when the element is placed, but the next number is not the
	one asked for, which is worth telling.
	@return what the counter would meet, nothing if the counter does not
	change, the formula does, or the numbers are not a single sequence
*/
std::optional<ElementAutoNumSchemeCommand::CounterConflict> ElementAutoNumSchemeCommand::counterConflict(
		const QETProject *project, const QString &title, const NumerotationContext &proposed)
{
	if (!project || !project->elementAutoNum().contains(title)) {
		return std::nullopt;
	}
	const NumerotationContext stored = project->elementAutoNum().value(title);
	const NumberSupport support = numberSupport(stored);
	if (!support.supported || proposed.size() != stored.size()
			|| autonum::numerotationContextToFormula(proposed) != autonum::numerotationContextToFormula(stored)) {
		return std::nullopt;
	}
	const int counter = proposed.itemAt(support.partIndex).value(1).toInt();
	if (counter == stored.itemAt(support.partIndex).value(1).toInt()) {
		return std::nullopt;
	}

	CounterConflict conflict;
	conflict.counter = counter;
	for (const Element *el : numberedFollowers(project, title)) {
		if (const auto number = numberOf(support, el)) {
			if (*number >= counter) {
				++conflict.count;
				conflict.highest = std::max(conflict.highest, *number);
			}
		}
	}
	if (!conflict.count) {
		return std::nullopt;
	}
	return conflict;
}

/**
	@brief ElementAutoNumSchemeCommand::gapRanges
	@return the numbers between the lowest and the highest which the elements
	following the scheme @p title carry, that none of them carries: where an
	element was deleted or given another number. Empty for a scheme whose
	elements cannot be given a number by hand (see numberSupport()).
*/
QList<ElementAutoNumSchemeCommand::GapRange> ElementAutoNumSchemeCommand::gapRanges(
		const QETProject *project, const QString &title)
{
	QList<GapRange> ranges;
	if (!project || !project->elementAutoNum().contains(title)) {
		return ranges;
	}
	const NumberSupport support = numberSupport(project->elementAutoNum().value(title));
	if (!support.supported) {
		return ranges;
	}
	QSet<int> used;
	for (const Element *el : numberedFollowers(project, title)) {
		if (const auto number = numberOf(support, el)) {
			used << *number;
		}
	}
	if (used.size() < 2) {
		return ranges;
	}
	const int low = *std::min_element(used.cbegin(), used.cend());
	const int high = *std::max_element(used.cbegin(), used.cend());
	for (int n = low + 1 ; n < high ; ++n)
	{
		if (used.contains(n)) {
			continue;
		}
		if (!ranges.isEmpty() && ranges.last().to == n - 1) {
			ranges.last().to = n;
		} else {
			ranges << GapRange{n, n};
		}
	}
	return ranges;
}

/**
	@brief ElementAutoNumSchemeCommand::labelForNumber
	@return the label @p element would have with the number @p number of the
	scheme @p title, empty if the scheme cannot give a number by hand
*/
QString ElementAutoNumSchemeCommand::labelForNumber(const QETProject *project,
													const QString &title,
													Element *element,
													int number)
{
	if (!project || !element || !element->diagram() || !project->elementAutoNum().contains(title)) {
		return QString();
	}
	NumerotationContext context = project->elementAutoNum().value(title);
	const NumberSupport support = numberSupport(context);
	if (!support.supported) {
		return QString();
	}
	context.replaceValue(support.partIndex, QString::number(number));
	const QString formula = autonum::numerotationContextToFormula(context);
	autonum::sequentialNumbers seq;
	autonum::setSequential(formula, seq, context, element->diagram(), title);
	return autonum::AssignVariables::formulaToLabel(formula, seq, element->diagram(), element, nullptr);
}

/**
	@brief ElementAutoNumSchemeCommand::freeNumbers
	@return the numbers @p element of the scheme @p title may be given by
	hand, in order: from 1 to a few past the highest in use or the counter,
	those no other element of the scheme carries and which would not give it
	the label of any other element. At most @p limit.
*/
QList<int> ElementAutoNumSchemeCommand::freeNumbers(const QETProject *project,
													const QString &title,
													const Element *element,
													int limit)
{
	QList<int> numbers;
	if (!project || !element || !project->elementAutoNum().contains(title)) {
		return numbers;
	}
	const NumerotationContext context = project->elementAutoNum().value(title);
	const NumberSupport support = numberSupport(context);
	if (!support.supported) {
		return numbers;
	}

	QSet<int> used;
	int highest = context.itemAt(support.partIndex).value(1).toInt();
	for (const Element *el : numberedFollowers(project, title)) {
		if (el == element) continue;
		if (const auto number = numberOf(support, el)) {
			used << *number;
			highest = std::max(highest, *number);
		}
	}

	QSet<QString> labels;
	for (Diagram *d : project->diagrams()) {
		for (QGraphicsItem *item : d->items()) {
			if (auto *el = qgraphicsitem_cast<Element *>(item)) {
				if (el != element) {
					labels << el->elementInformations().value(QETInformation::ELMT_LABEL).toString();
				}
			}
		}
	}
	labels.remove(QString());

	for (int n = 1 ; n <= highest + 10 && numbers.size() < limit ; ++n)
	{
		if (used.contains(n)) {
			continue;
		}
		const QString label = labelForNumber(project, title, const_cast<Element *>(element), n);
		if (!label.isEmpty() && labels.contains(label)) {
			continue;
		}
		numbers << n;
	}
	return numbers;
}

/**
	@brief ElementAutoNumSchemeCommand::assignNumber
	Give @p element, which follows an element numbering scheme, the number
	@p number of it, which must be free (see freeNumbers()): its label is the
	formula worked out with that number and it keeps following the scheme.
	When the scheme's counter is at or below @p number it moves to the next
	number after it, so that the next element is not given the same label.
	One undo step.
	@param problem : if not null, receives why nothing is done, translated
	@return the command, nullptr if @p number cannot be given
*/
RenumberElementsCommand *ElementAutoNumSchemeCommand::assignNumber(QETProject *project,
																   Element *element,
																   int number,
																   QString *problem,
																   QUndoCommand *parent)
{
	const auto fail = [&](const QString &text) -> RenumberElementsCommand * {
		if (problem) *problem = text;
		return nullptr;
	};
	if (!project || !element || !element->diagram()) {
		return fail(tr("This element is not in any sheet."));
	}
	const DiagramContext info = element->elementInformations();
	const QString title = project->elementAutoNumTitle(
				QUuid(info.value(QETInformation::ELMT_FORMULA_ID).toString()));
	if (title.isEmpty()) {
		return fail(tr("This element does not follow any numbering."));
	}
	const NumerotationContext context = project->elementAutoNum().value(title);
	const NumberSupport support = numberSupport(context);
	if (!support.supported) {
		return fail(tr("The “%1” numbering does not have a single number: one cannot be chosen by hand.").arg(title));
	}
	if (number < 1) {
		return fail(tr("The number must be at least 1."));
	}
	if (!freeNumbers(project, title, element, 100000).contains(number)) {
		return fail(tr("Number %1 is not free.").arg(number));
	}

	const QString formula = autonum::numerotationContextToFormula(context);
	NumerotationContext at = context;
	at.replaceValue(support.partIndex, QString::number(number));
	autonum::sequentialNumbers seq;
	autonum::setSequential(formula, seq, at, element->diagram(), title);

	RenumberElementsCommand::ElementChange change;
	change.element = element;
	change.old_infos = info;
	change.old_seq = element->sequenceStruct();
	change.old_frozen = element->isFreezeLabel();
	change.new_frozen = change.old_frozen;
	change.new_infos = info;
	change.new_infos.addValue(QETInformation::ELMT_FORMULA, formula);
	change.new_infos.addValue(QETInformation::ELMT_FORMULA_ID,
							  project->elementAutoNumId(title).toString(), false);
	change.new_infos.addValue(QETInformation::ELMT_LABEL,
							  autonum::AssignVariables::formulaToLabel(
								  formula, seq, element->diagram(), element, nullptr));
	change.new_seq = seq;

	NumerotationContext counter = context;
	if (counter.itemAt(support.partIndex).value(1).toInt() <= number) {
		counter.replaceValue(support.partIndex, QString::number(number + support.increase));
	}
	return new RenumberElementsCommand(project, {change},
									   {{title, context}}, {{title, counter}},
									   tr("Assign number %1").arg(number), parent);
}

/**
	@brief ElementAutoNumSchemeCommand::skipReason
	@return why assign() would leave @p element alone, None if it would
	give it the scheme @p title
*/
ElementAutoNumSchemeCommand::Skipped ElementAutoNumSchemeCommand::skipReason(
		const QETProject *project,
		const QString &title,
		const Element *element)
{
	if (!project || !element) {
		return Skipped::Linked;
	}
	if (element->linkType() == Element::Slave
			|| (element->linkType() & Element::AllReport)) {
		return Skipped::Linked;
	}
	const DiagramContext &info = element->elementInformations();
	const QString formula = info.value(QETInformation::ELMT_FORMULA).toString();
	const QUuid wanted = project->elementAutoNumId(title);
	if (!formula.isEmpty()) {
		if (!wanted.isNull()
				&& QUuid(info.value(QETInformation::ELMT_FORMULA_ID).toString()) == wanted) {
			return Skipped::FollowsAlready;
		}
		return Skipped::OtherFormula;
	}
	if (element->isFreezeLabel()) {
		return Skipped::Frozen;
	}
	return Skipped::None;
}

/**
	@brief ElementAutoNumSchemeCommand::assignPlan
	Sort @p elements by what assign() would do with them. A frozen label
	is told apart from a formula held already: both are left alone unless
	the user agrees to replace them.
*/
ElementAutoNumSchemeCommand::AssignPlan ElementAutoNumSchemeCommand::assignPlan(
		const QETProject *project,
		const QString &title,
		const QVector<Element *> &elements)
{
	AssignPlan plan;
	for (Element *el : elements)
	{
		switch (skipReason(project, title, el)) {
			case Skipped::None:
				plan.todo << el;
				break;
			case Skipped::Frozen:
				plan.frozen << el;
				break;
			case Skipped::OtherFormula:
				plan.otherFormula << el;
				break;
			case Skipped::FollowsAlready:
				++plan.followsAlready;
				break;
			case Skipped::Linked:
				++plan.linked;
				break;
		}
	}
	return plan;
}

/**
	@brief ElementAutoNumSchemeCommand::numberElements
	Give every one of @p elements (a slave or a report excepted) the
	element numbering scheme @p title: its formula and the next number of
	the scheme, in folio and position order, the scheme's counter moving
	on. The numbering goes past the labels the other elements which follow
	the scheme keep. One undo step.
	@param freeze : what the frozen state of the elements becomes
	@param parent : parent undo command, if the numbering is a part of one
	@return the command, nullptr if there is nothing to number
*/
RenumberElementsCommand *ElementAutoNumSchemeCommand::numberElements(
		QETProject *project,
		const QString &title,
		const QVector<Element *> &elements,
		const QString &text,
		FreezeRule freeze,
		QUndoCommand *parent)
{
	if (!project || !project->elementAutoNum().contains(title)) {
		return nullptr;
	}
	QVector<Element *> todo;
	for (Element *el : elements) {
		if (el && el->linkType() != Element::Slave && !(el->linkType() & Element::AllReport)) {
			todo << el;
		}
	}
	if (todo.isEmpty()) {
		return nullptr;
	}

	const NumerotationContext context = project->elementAutoNum().value(title);
	NumerotationContext final_context;
	QVector<RenumberElementsCommand::ElementChange> changes = renumberChanges(
				title, context, todo,
				autonum::numerotationContextToFormula(context),
				&final_context, false, project->elementAutoNumId(title),
				reservedLabels(project, title, todo));
	for (auto &change : changes) {
		change.new_frozen = freeze == FreezeRule::NewElementPolicy
				&& change.element && change.element->diagram()
				&& (change.element->diagram()->freezeNewElements()
					|| project->isFreezeNewElements());
	}

	return new RenumberElementsCommand(
				project, changes,
				{{title, context}}, {{title, final_context}},
				text, parent);
}

/**
	@brief ElementAutoNumSchemeCommand::assign
	Make @p elements follow the element numbering scheme @p title: each
	takes its formula and the next number of the scheme, in folio and
	position order, and the scheme's counter moves on. One undo step.

	An element is left alone (see assignPlan()) when it follows the scheme
	already, is a slave or a report, or when something would be lost: a
	frozen label, or a formula it follows or holds already, unless
	@p overwrite. An element with a label typed by hand and no formula is
	given the scheme; its label is replaced by the number.
	@param skipped : if not null, receives how many elements were left alone
	@return the command, nullptr if there is nothing to give
*/
RenumberElementsCommand *ElementAutoNumSchemeCommand::assign(
		QETProject *project,
		const QString &title,
		const QVector<Element *> &elements,
		bool overwrite,
		int *skipped,
		QUndoCommand *parent)
{
	if (skipped) *skipped = 0;
	if (!project || !project->elementAutoNum().contains(title)) {
		return nullptr;
	}

	const AssignPlan plan = assignPlan(project, title, elements);
	QVector<Element *> todo = plan.todo;
	int left = plan.followsAlready + plan.linked;
	if (overwrite) {
		todo += plan.frozen;
		todo += plan.otherFormula;
	} else {
		left += plan.frozen.size() + plan.otherFormula.size();
	}
	if (skipped) *skipped = left;
	if (todo.isEmpty()) {
		return nullptr;
	}
	return numberElements(project, title, todo,
						  tr("Apply numbering %1").arg(title),
						  FreezeRule::Unfrozen, parent);
}

/**
	@brief ElementAutoNumSchemeCommand::schemeForFormula
	@return the title of the element numbering scheme of @p project which
	gives @p formula, empty if there is none. When several schemes have the
	same formula (the same parts, other counters), the project's current one
	if it is among them, otherwise the first by name: the label is worked
	out afresh in either case, only the counter which goes on differs.
*/
QString ElementAutoNumSchemeCommand::schemeForFormula(const QETProject *project,
													  const QString &formula)
{
	if (!project || formula.isEmpty()) {
		return QString();
	}
	QStringList matches;
	const auto titles = project->elementAutoNum().keys();
	for (const QString &title : titles) {
		if (project->elementAutoNumFormula(title) == formula) {
			matches << title;
		}
	}
	if (matches.isEmpty()) {
		return QString();
	}
	if (matches.contains(project->elementCurrentAutoNum())) {
		return project->elementCurrentAutoNum();
	}
	matches.sort(Qt::CaseInsensitive);
	return matches.first();
}

/**
	@brief ElementAutoNumSchemeCommand::followedScheme
	@return the title of the scheme of @p project which an element with the
	information @p info follows, empty if none.

	The id names it, but only counts when the scheme has the formula the
	element has: an id which comes from another project (a copy of the
	project edited since, two projects which both have a numbering of the
	same name...) may name a scheme here which is not the same numbering.
*/
QString ElementAutoNumSchemeCommand::followedScheme(const QETProject *project,
													const DiagramContext &info)
{
	if (!project) {
		return QString();
	}
	const QString title = project->elementAutoNumTitle(
				QUuid(info.value(QETInformation::ELMT_FORMULA_ID).toString()));
	if (title.isEmpty()
			|| project->elementAutoNumFormula(title) != info.value(QETInformation::ELMT_FORMULA).toString()) {
		return QString();
	}
	return title;
}

/**
	@brief ElementAutoNumSchemeCommand::writeCopiedSchemes
	Add to the copy @p root the definition of every element numbering scheme
	of @p project which the @p copied elements follow, so that a paste into
	another project, which knows nothing of it, can offer to import it.
*/
void ElementAutoNumSchemeCommand::writeCopiedSchemes(QDomDocument &document,
													 QDomElement &root,
													 const QETProject *project,
													 const QVector<Element *> &copied)
{
	if (!project) {
		return;
	}
	QSet<QUuid> done;
	QDomElement list;
	for (const Element *el : copied)
	{
		const QUuid id(el->elementInformations().value(QETInformation::ELMT_FORMULA_ID).toString());
		const QString title = project->elementAutoNumTitle(id);
		if (title.isEmpty() || done.contains(id)) {
			continue;
		}
		done << id;
		if (list.isNull()) {
			list = document.createElement(QStringLiteral("copied_element_autonums"));
			root.appendChild(list);
		}
		NumerotationContext context = project->elementAutoNum().value(title);
		QDomElement scheme = context.toXml(document, QStringLiteral("element_autonum"));
		scheme.setAttribute(QStringLiteral("title"), title);
		scheme.setAttribute(QStringLiteral("id"), id.toString());
		scheme.setAttribute(QStringLiteral("formula"), autonum::numerotationContextToFormula(context));
		list.appendChild(scheme);
	}
}

/// @return the numbering schemes a copy carries (see writeCopiedSchemes())
QList<ElementAutoNumSchemeCommand::Scheme> ElementAutoNumSchemeCommand::copiedSchemes(
		const QDomElement &root)
{
	QList<Scheme> schemes;
	const QDomElement list = root.firstChildElement(QStringLiteral("copied_element_autonums"));
	for (QDomElement e = list.firstChildElement(QStringLiteral("element_autonum"));
		 !e.isNull();
		 e = e.nextSiblingElement(QStringLiteral("element_autonum")))
	{
		Scheme scheme;
		scheme.title = e.attribute(QStringLiteral("title"));
		scheme.id = QUuid(e.attribute(QStringLiteral("id")));
		scheme.context.fromXml(e);
		if (!scheme.title.isEmpty() && !scheme.id.isNull() && !scheme.context.isEmpty()) {
			schemes << scheme;
		}
	}
	return schemes;
}

/**
	@brief ElementAutoNumSchemeCommand::missingForPaste
	@return those of the @p copied schemes which the @p pasted elements
	follow and which @p project has no use for: it has neither their id nor
	a scheme with their formula (see schemeForFormula()), so the pasted
	elements would follow no numbering. Slaves and reports follow none.
*/
QList<ElementAutoNumSchemeCommand::Scheme> ElementAutoNumSchemeCommand::missingForPaste(
		const QETProject *project,
		const QList<Scheme> &copied,
		const QList<Element *> &pasted)
{
	QList<Scheme> missing;
	if (!project) {
		return missing;
	}
	QSet<QUuid> followed;
	for (const Element *el : pasted) {
		if (el->linkType() == Element::Slave || (el->linkType() & Element::AllReport)) {
			continue;
		}
		followed << QUuid(el->elementInformations().value(QETInformation::ELMT_FORMULA_ID).toString());
	}
	for (const Scheme &scheme : copied)
	{
		const QString formula = autonum::numerotationContextToFormula(scheme.context);
		const QString by_id = project->elementAutoNumTitle(scheme.id);
			//Present: under its id with the same formula, or under another id
			//(another name, other project) with the same formula
		const bool present = (!by_id.isEmpty() && project->elementAutoNumFormula(by_id) == formula)
				|| !schemeForFormula(project, formula).isEmpty();
		if (!followed.contains(scheme.id) || present) {
			continue;
		}
		missing << scheme;
	}
	return missing;
}

/**
	@brief ElementAutoNumSchemeCommand::pastedSchemes
	@return the element numbering schemes of @p project which freshly
	pasted @p elements follow, by title, each with its elements.

	An element follows a scheme when its formula_id names one, or, when
	it names none (the element comes from another project, or from a file
	written before the ids), when one has its formula (see
	schemeForFormula()). A slave or a report follows none: it takes its
	label from its master.
*/
QMap<QString, QVector<Element *>> ElementAutoNumSchemeCommand::pastedSchemes(
		const QETProject *project,
		const QList<Element *> &elements)
{
	QMap<QString, QVector<Element *>> schemes;
	if (!project || project->elementAutoNum().isEmpty()) {
		return schemes;
	}

	for (Element *el : elements)
	{
		if (!el || el->linkType() == Element::Slave || (el->linkType() & Element::AllReport)) {
			continue;
		}
		const DiagramContext &info = el->elementInformations();
		const QString formula = info.value(QETInformation::ELMT_FORMULA).toString();
		if (formula.isEmpty()) {
			continue;
		}
		QString title = followedScheme(project, info);
		if (title.isEmpty()) {
			title = schemeForFormula(project, formula);
		}
		if (!title.isEmpty()) {
			schemes[title] << el;
		}
	}
	return schemes;
}

/**
	@brief ElementAutoNumSchemeCommand::linkPasted
	Make the formula_id of pasted @p elements name a scheme of @p project,
	as QETProject does for the elements of a file it loads: an id which
	names one with the element's formula is kept (see followedScheme());
	otherwise the scheme with the element's formula (see schemeForFormula()),
	else none. Labels are not touched.
*/
void ElementAutoNumSchemeCommand::linkPasted(const QETProject *project,
											 const QList<Element *> &elements)
{
	if (!project) {
		return;
	}
	for (Element *el : elements)
	{
		if (!el) continue;
		const DiagramContext &info = el->elementInformations();
		const QString formula = info.value(QETInformation::ELMT_FORMULA).toString();
		if (formula.isEmpty()) {
			if (info.contains(QETInformation::ELMT_FORMULA_ID)) {
				el->setFormulaSchemeId(QUuid());
			}
			continue;
		}
		if (!followedScheme(project, info).isEmpty()) {
			continue;
		}
		const QString title = schemeForFormula(project, formula);
		el->setFormulaSchemeId(title.isEmpty() ? QUuid() : project->elementAutoNumId(title));
	}
}

/**
	@brief ElementAutoNumSchemeCommand::numberPasted
	Number the pasted elements of @p schemes (see pastedSchemes()) the way
	elements placed one after the other are: each scheme gives its next
	numbers, in folio and position order, and its counter moves on. One
	numbering command per scheme, children of @p parent, which runs them
	with its own redo(), so that undoing the paste gives the numbers back.
	@return how many elements are numbered
*/
int ElementAutoNumSchemeCommand::numberPasted(
		QETProject *project,
		const QMap<QString, QVector<Element *>> &schemes,
		QUndoCommand *parent)
{
	int count = 0;
	for (auto it = schemes.constBegin() ; it != schemes.constEnd() ; ++it)
	{
		if (numberElements(project, it.key(), it.value(),
						   tr("Number pasted elements (%1)").arg(it.key()),
						   FreezeRule::NewElementPolicy, parent)) {
			count += it.value().size();
		}
	}
	return count;
}

/**
	@brief ElementAutoNumSchemeCommand::create
	A new element numbering scheme.
	@param id : its uuid; a new one if null or already used
	@param make_current : make it the scheme given to new elements
	@return nullptr if @p title is not an acceptable name
*/
ElementAutoNumSchemeCommand *ElementAutoNumSchemeCommand::create(
		QETProject *project,
		const QString &title,
		const NumerotationContext &context,
		const QUuid &id,
		bool make_current)
{
	if (!project || !nameProblem(project, title).isEmpty()) {
		return nullptr;
	}

	auto *cmd = new ElementAutoNumSchemeCommand(project);
	Scheme after;
	after.title = QETProject::normalizedAutoNumName(title);
	after.id = (id.isNull() || !project->elementAutoNumTitle(id).isEmpty())
			   ? QUuid::createUuid()
			   : id;
	after.context = context;
	cmd->m_after = after;
	cmd->m_current_before = project->elementCurrentAutoNum();
	cmd->m_current_after = make_current ? after.title : cmd->m_current_before;
	cmd->setText(tr("Create element numbering %1").arg(after.title));
	return cmd;
}

/**
	@brief ElementAutoNumSchemeCommand::edit
	Rename the element numbering scheme @p old_title to @p new_title and
	give it @p context. The elements following it follow it still (see
	the class description).
	@param make_current : make it the scheme given to new elements; if
	false, it stays so if it was
	@return nullptr if there is no scheme @p old_title, @p new_title is not
	an acceptable name, an element with a frozen label follows the scheme
	and its formula would change (see editBlockedBy()), or nothing changes
*/
ElementAutoNumSchemeCommand *ElementAutoNumSchemeCommand::edit(
		QETProject *project,
		const QString &old_title,
		const QString &new_title,
		const NumerotationContext &context,
		bool make_current)
{
	if (!project || !project->elementAutoNum().contains(old_title)) {
		return nullptr;
	}
	if (!nameProblem(project, new_title, old_title).isEmpty()) {
		return nullptr;
	}
	if (!editBlockedBy(project, old_title, context).isEmpty()) {
		return nullptr;
	}

	Scheme before;
	before.title = old_title;
	before.id = project->elementAutoNumId(old_title);
	before.context = project->elementAutoNum(old_title);

	Scheme after;
	after.title = QETProject::normalizedAutoNumName(new_title);
	after.id = before.id;
	after.context = context;

	auto *cmd = new ElementAutoNumSchemeCommand(project);
	cmd->m_before = before;
	cmd->m_current_before = project->elementCurrentAutoNum();
	cmd->m_current_after = (make_current || cmd->m_current_before == old_title)
						   ? after.title
						   : cmd->m_current_before;

	const QString old_formula = autonum::numerotationContextToFormula(before.context);
	const QString new_formula = autonum::numerotationContextToFormula(after.context);
	if (old_formula != new_formula)
	{
		QVector<Element *> elements;
		const auto followers = project->elementsUsingElementAutoNum(old_title);
		for (Element *el : followers)
		{
			if (el->linkType() == Element::Slave || (el->linkType() & Element::AllReport)) {
				continue;
			}
			elements << el;
		}
		if (sameSequentialParts(old_formula, new_formula))
		{
			for (Element *el : elements)
			{
				RenumberElementsCommand::ElementChange ch;
				ch.element = el;
				ch.old_infos = el->elementInformations();
				ch.old_seq = el->sequenceStruct();
				ch.new_seq = ch.old_seq;
				ch.old_frozen = el->isFreezeLabel();
				ch.new_frozen = ch.old_frozen;

				autonum::sequentialNumbers seq = ch.old_seq;
				ch.new_infos = ch.old_infos;
				ch.new_infos.addValue(QETInformation::ELMT_FORMULA, new_formula);
				ch.new_infos.addValue(QETInformation::ELMT_LABEL,
									  autonum::AssignVariables::formulaToLabel(
										  new_formula, seq, el->diagram(), el, nullptr));
				cmd->m_changes << ch;
			}
		}
		else if (!elements.isEmpty())
		{
			cmd->m_changes = renumberChanges(old_title, after.context,
											 elements, new_formula,
											 &after.context, true, QUuid(),
											 reservedLabels(project, old_title, elements));
		}
	}
	cmd->m_after = after;

	const bool renamed = before.title != after.title;
	const bool redefined = !sameContext(before.context, after.context);
	if (!renamed && !redefined && cmd->m_changes.isEmpty()
			&& cmd->m_current_before == cmd->m_current_after) {
		delete cmd;
		return nullptr;
	}

	if (renamed && !redefined) {
		cmd->setText(tr("Rename element numbering %1 to %2")
					 .arg(before.title, after.title));
	} else {
		cmd->setText(tr("Modify element numbering %1").arg(after.title));
	}
	return cmd;
}

/**
	@brief ElementAutoNumSchemeCommand::remove
	Remove the element numbering scheme @p title.
	@return nullptr if there is no such scheme or elements still follow
	it: removing it would leave them with a formula nothing defines
*/
ElementAutoNumSchemeCommand *ElementAutoNumSchemeCommand::remove(
		QETProject *project,
		const QString &title)
{
	if (!project || !project->elementAutoNum().contains(title)) {
		return nullptr;
	}
	if (!project->elementsUsingElementAutoNum(title).isEmpty()) {
		return nullptr;
	}

	auto *cmd = new ElementAutoNumSchemeCommand(project);
	Scheme before;
	before.title = title;
	before.id = project->elementAutoNumId(title);
	before.context = project->elementAutoNum(title);
	cmd->m_before = before;
	cmd->m_current_before = project->elementCurrentAutoNum();
	cmd->m_current_after = cmd->m_current_before == title
						   ? QString()
						   : cmd->m_current_before;
	cmd->setText(tr("Delete element numbering %1").arg(title));
	return cmd;
}

void ElementAutoNumSchemeCommand::redo()
{
	if (!m_project) return;

	if (m_before && m_after) {
		m_project->renameElementAutoNum(m_before->title, m_after->title);
	} else if (m_before) {
		m_project->removeElementAutoNum(m_before->title);
	}
	if (m_after) {
		m_project->addElementAutoNum(m_after->title, m_after->context, m_after->id);
	}
	m_project->setCurrrentElementAutonum(m_current_after);

	for (const auto &change : std::as_const(m_changes)) {
		RenumberElementsCommand::applyChange(change, true);
	}
}

void ElementAutoNumSchemeCommand::undo()
{
	if (!m_project) return;

	for (const auto &change : std::as_const(m_changes)) {
		RenumberElementsCommand::applyChange(change, false);
	}

	if (m_before && m_after) {
		m_project->renameElementAutoNum(m_after->title, m_before->title);
	} else if (m_after) {
		m_project->removeElementAutoNum(m_after->title);
	}
	if (m_before) {
		m_project->addElementAutoNum(m_before->title, m_before->context, m_before->id);
	}
	m_project->setCurrrentElementAutonum(m_current_before);
}
