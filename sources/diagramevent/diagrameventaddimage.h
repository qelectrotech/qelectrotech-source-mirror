/*
        Copyright 2006-2015 The QElectroTech Team
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
#ifndef DIAGRAMEVENTADDIMAGE_H
#define DIAGRAMEVENTADDIMAGE_H

#include "diagrameventinterface.h"

class Diagram;
class DiagramImageItem;

/**
 * @brief The DiagramEventAddImage class
 * This diagram event, handle the adding of an image in a diagram.
 */
class DiagramEventAddImage : public DiagramEventInterface
{
        Q_OBJECT

        public:
            DiagramEventAddImage(Diagram *diagram);
            virtual ~DiagramEventAddImage();

            virtual bool mousePressEvent       (QGraphicsSceneMouseEvent *event);
            virtual bool mouseMoveEvent        (QGraphicsSceneMouseEvent *event);
            virtual bool mouseDoubleClickEvent (QGraphicsSceneMouseEvent *event);
            virtual bool wheelEvent            (QGraphicsSceneWheelEvent *event);

            bool isNull () const;
    private:
            void openDialog();

            DiagramImageItem *m_image;
            bool m_is_added;
};

#endif // DIAGRAMEVENTADDIMAGE_H
