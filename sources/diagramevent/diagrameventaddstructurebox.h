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
#ifndef DIAGRAMEVENTADDSTRUCTUREBOX_H
#define DIAGRAMEVENTADDSTRUCTUREBOX_H

#include "diagrameventaddshape.h"

/**
	@brief The DiagramEventAddStructureBox class
	Manages the creation of a StructureBoxItem: reuses DiagramEventAddShape's
	drag-a-rectangle mouse handling unchanged, only overriding which item
	class gets instantiated.
*/
class DiagramEventAddStructureBox : public DiagramEventAddShape
{
		Q_OBJECT

	public:
		explicit DiagramEventAddStructureBox(Diagram *diagram);

	protected:
		QetShapeItem *createShapeItem(QPointF p1, QPointF p2, QetShapeItem::ShapeType shape_type) override;
};

#endif // DIAGRAMEVENTADDSTRUCTUREBOX_H
