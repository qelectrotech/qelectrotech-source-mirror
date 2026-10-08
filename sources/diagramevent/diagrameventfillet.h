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
#ifndef DIAGRAMEVENTFILLET_H
#define DIAGRAMEVENTFILLET_H

#include "diagrameventinterface.h"

#include <QPointer>
#include <QPointF>

class QetShapeItem;

/**
	@brief The DiagramEventFillet class
	The "add a fillet" tool: rounds the corner between two drawn lines,
	the way a sketch fillet does in a CAD program. Click the two lines
	one after the other, or once where they meet; a dialog asks for the
	radius. The lines are trimmed (or extended) to the points where the
	arc touches them, and an arc is added between them -- an ordinary
	Ellipse shape with a start and end angle, so nothing new is saved.
*/
class DiagramEventFillet : public DiagramEventInterface
{
		Q_OBJECT

	public:
		DiagramEventFillet(Diagram *diagram);
		~DiagramEventFillet() override;
		void mousePressEvent (QGraphicsSceneMouseEvent *event) override;

	private:
		struct Pick {
			QetShapeItem *line = nullptr;
			QPointF       pos;   // where the line was clicked, in scene coordinates
		};

		QList<QetShapeItem *> linesNear(const QPointF &scenePos) const;
		void filletLines(const Pick &a, const Pick &b);
		void clearFirstPick();
		void showHint(const QString &text) const;
		void showDefaultHint() const;

		Pick m_first;
		QPointer<QetShapeItem> m_first_line;   // m_first.line, guarded: the line can be deleted between the two clicks
};

#endif // DIAGRAMEVENTFILLET_H
