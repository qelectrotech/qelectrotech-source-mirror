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

#include "renumberelementscommand.h"

#include "../qetproject.h"
#include "../qetgraphicsitem/element.h"

RenumberElementsCommand::RenumberElementsCommand(
        QETProject *project,
        QVector<ElementChange> changes,
        QHash<QString, NumerotationContext> old_ctx,
        QHash<QString, NumerotationContext> new_ctx,
        const QString &text)
    : QUndoCommand(text)
    , project_(project)
    , changes_(std::move(changes))
    , old_ctx_(std::move(old_ctx))
    , new_ctx_(std::move(new_ctx))
{
}

void RenumberElementsCommand::undo()
{
    apply(false);
}

void RenumberElementsCommand::redo()
{
    // QUndoStack calls redo() once immediately after push(); keep standard behaviour.
    if (first_redo_) {
        first_redo_ = false;
    }
    apply(true);
}

void RenumberElementsCommand::apply(bool use_new)
{
    if (!project_) return;

    // Restore project contexts
    const auto &ctx = use_new ? new_ctx_ : old_ctx_;
    for (auto it = ctx.constBegin(); it != ctx.constEnd(); ++it) {
        project_->addElementAutoNum(it.key(), it.value());
    }

    // Apply per-element changes
    for (const ElementChange &c : changes_) {
        if (!c.element) continue;

        const bool frozen = use_new ? c.new_frozen : c.old_frozen;
        const auto &infos = use_new ? c.new_infos : c.old_infos;
        const auto &seq   = use_new ? c.new_seq   : c.old_seq;

        // Temporarily unfreeze so that label/infos update correctly.
        const bool was_frozen = c.element->isFreezeLabel();
        if (was_frozen) c.element->freezeLabel(false);

        c.element->rSequenceStruct() = seq;
        c.element->setElementInformations(infos);
        c.element->freezeLabel(frozen);
    }
}
