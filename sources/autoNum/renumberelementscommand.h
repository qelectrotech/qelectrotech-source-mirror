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

#ifndef RENUMBERELEMENTSCOMMAND_H
#define RENUMBERELEMENTSCOMMAND_H

#include <QUndoCommand>
#include <QHash>
#include <QVector>

#include "../diagramcontext.h"
#include "assignvariables.h" // defines autonum::sequentialNumbers

class Element;
class QETProject;

/**
 * @brief Undoable renumbering of element labels and sequence structs.
 */
class RenumberElementsCommand final : public QUndoCommand
{
public:
    struct ElementChange
    {
        Element *element = nullptr;
        DiagramContext old_infos;
        DiagramContext new_infos;
        autonum::sequentialNumbers old_seq;
        autonum::sequentialNumbers new_seq;
        bool old_frozen = false;
        bool new_frozen = false;
    };

    RenumberElementsCommand(
            QETProject *project,
            QVector<ElementChange> changes,
            QHash<QString, NumerotationContext> old_ctx,
            QHash<QString, NumerotationContext> new_ctx,
            const QString &text);

    void undo() override;
    void redo() override;

private:
    void apply(bool use_new);

private:
    QETProject *project_ = nullptr;
    QVector<ElementChange> changes_;
    QHash<QString, NumerotationContext> old_ctx_;
    QHash<QString, NumerotationContext> new_ctx_;
    bool first_redo_ = true;
};

#endif // RENUMBERELEMENTSCOMMAND_H
