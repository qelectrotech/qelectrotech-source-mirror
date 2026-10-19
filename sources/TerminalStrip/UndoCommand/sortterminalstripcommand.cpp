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
#include "sortterminalstripcommand.h"
#include "../terminalstrip.h"
#include "../physicalterminal.h"
#include "../realterminal.h"
#include "../../diagram.h"
#include "../../positionorder.h"
#include "../../qetgraphicsitem/element.h"
#include "../../utils/qetutils.h"

SortTerminalStripCommand::SortTerminalStripCommand(TerminalStrip *strip, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_strip(strip)
{
	setText(QObject::tr("Sort terminal block %1").arg(m_strip->name()));
	m_old_order = m_strip->physicalTerminal();
	m_new_order = m_strip->physicalTerminal();
	sort();
}

void SortTerminalStripCommand::undo()
{
	if (m_strip) {
		m_strip->setOrderTo(m_old_order);
	}
}

void SortTerminalStripCommand::redo()
{
	if (m_strip) {
		m_strip->setOrderTo(m_new_order);
	}
}

void SortTerminalStripCommand::sort()
{
		//The plan position of a physical terminal is the position of the
		//level that sits on a folio; a terminal whose levels are all
		//spares has no position and is ordered by label only.
	auto position_of = [](const QSharedPointer<PhysicalTerminal> &t_) -> QSharedPointer<RealTerminal>
	{
		if (!t_) {
			return {};
		}
		for (const auto &real_ : t_->realTerminals()) {
			if (real_ && real_->element()) {
				return real_;
			}
		}
		return t_->realTerminalCount() ? t_->realTerminals().constFirst()
									   : QSharedPointer<RealTerminal>();
	};

	std::stable_sort(m_new_order.begin(), m_new_order.end(),
			[&position_of](const QSharedPointer<PhysicalTerminal> &arg1,
						const QSharedPointer<PhysicalTerminal> &arg2)
	{
		return planOrderLessThan(position_of(arg1), position_of(arg2));
	});
}

/**
	@brief SortTerminalStripCommand::planOrderLessThan
	Order two real terminals the way the plan is drawn : folio by folio,
	then left to right. Terminals with no folio position (spares) come
	last, and the label only breaks ties, so the result stays stable and
	predictable whatever the labels look like ("X1:N" and "X1:PE" stand
	where they are drawn, not at the end).
	Both the auto import and sort() use this, so the two orders of the
	same strip can never fight each other.
	@a a_ terminal to compare
	@a b_ other terminal to compare
	@return true if @a a_ comes before @a b_
*/
bool SortTerminalStripCommand::planOrderLessThan(
		const QSharedPointer<RealTerminal> &a_,
		const QSharedPointer<RealTerminal> &b_)
{
	if (!a_ || !b_) {
		return a_ && !b_;		//a missing terminal goes last
	}

	const Element *ea = a_->element();
	const Element *eb = b_->element();
	auto folio_of = [](const Element *e_) -> int
	{
		return (e_ && e_->diagram()) ? e_->diagram()->folioIndex() : -1;
	};

	const int fa = folio_of(ea);
	const int fb = folio_of(eb);
	if (fa != fb) {
			//A terminal with no folio has no place in the plan order
		if (fa < 0) {
			return false;
		}
		if (fb < 0) {
			return true;
		}
		return fa < fb;
	}
	if (fa >= 0 && ea->pos() != eb->pos()) {
		return PositionOrder::roundedXThenY(ea->pos(), eb->pos());
	}
	return QETUtils::naturalLessThan(a_->label(), b_->label());
}
