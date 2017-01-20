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
#ifndef DIAGRAMEVENTADDTEXT_H
#define DIAGRAMEVENTADDTEXT_H

#include "diagrameventinterface.h"

class Diagram;

/**
 * @brief The DiagramEventAddText class
 * This diagram event handle the creation of a new text in a diagram
 */
class DiagramEventAddText : public DiagramEventInterface
{
        Q_OBJECT

        public:
            DiagramEventAddText(Diagram *diagram);
            virtual ~DiagramEventAddText();

            virtual bool mousePressEvent (QGraphicsSceneMouseEvent *event);
};

#endif // DIAGRAMEVENTADDTEXT_H
