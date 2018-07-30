/*
        Copyright 2006-2017 The QElectroTech Team
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

#include "diagrameventaddtext.h"
#include "independenttextitem.h"
#include "diagramcommands.h"
#include "diagram.h"

/**
 * @brief DiagramEventAddText::DiagramEventAddText
 * Default constructor
 * @param diagram : the diagram where this event must operate
 */
DiagramEventAddText::DiagramEventAddText(Diagram *diagram) :
    DiagramEventInterface(diagram)
{}

/**
 * @brief DiagramEventAddText::~DiagramEventAddText
 */
DiagramEventAddText::~DiagramEventAddText()
{}

/**
 * @brief DiagramEventAddText::mousePressEvent
 * @param event : event of mouse press event.
 * @return : true if this event is handled, otherwise false
 */
bool DiagramEventAddText::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        IndependentTextItem *text = new IndependentTextItem();
        m_diagram -> undoStack().push(new AddItemCommand<IndependentTextItem *>(text, m_diagram, event->scenePos()));
        text->setTextInteractionFlags(Qt::TextEditorInteraction);
        text->setFocus(Qt::MouseFocusReason);
        emit finish();
        return true;
    }
    return false;
}
