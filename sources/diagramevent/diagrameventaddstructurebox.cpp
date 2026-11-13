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
#include "diagrameventaddstructurebox.h"

#include "../qetgraphicsitem/structureboxitem.h"

DiagramEventAddStructureBox::DiagramEventAddStructureBox(Diagram *diagram) :
	DiagramEventAddShape(diagram, QetShapeItem::Rectangle)
{
}

QetShapeItem *DiagramEventAddStructureBox::createShapeItem(QPointF p1, QPointF p2, QetShapeItem::ShapeType shape_type)
{
	Q_UNUSED(shape_type)
	return new StructureBoxItem(p1, p2);
}
