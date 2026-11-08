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
#include "eseventsetorigin.h"

#include "../editorcommands.h"
#include "../elementscene.h"
#include "../graphicspart/customelementpart.h"
#include "../graphicspart/partterminal.h"
#include "../ui/qetelementeditor.h"

#include <QGraphicsSceneMouseEvent>
#include <QLineF>
#include <QObject>

ESEventSetOrigin::ESEventSetOrigin(ElementScene *scene) :
	ESEventInterface(scene)
{
	m_running = true;
}

QPointF ESEventSetOrigin::target(ElementScene *scene, const QPointF &scene_pos)
{
	constexpr qreal reach = 6;
	QPointF nearest;
	qreal distance = reach;
	for (CustomElementPart *part : scene->primitives()) {
		auto *terminal = dynamic_cast<PartTerminal *>(part);
		if (!terminal)
			continue;
		const qreal d = QLineF(terminal->scenePos(), scene_pos).length();
		if (d <= distance) {
			distance = d;
			nearest = terminal->scenePos();
		}
	}
	if (distance < reach)
		return nearest;
		//The folio grid: what lands on it when the symbol is placed
	return QPointF(qRound(scene_pos.x() / 10.0) * 10.0, qRound(scene_pos.y() / 10.0) * 10.0);
}

bool ESEventSetOrigin::moveOriginTo(ElementScene *scene, const QPointF &point)
{
	if (point.isNull())
		return false;
	QList<QGraphicsItem *> parts;
	for (CustomElementPart *part : scene->primitives()) {
		if (QGraphicsItem *item = part->toItem()) {
			item->setSelected(false);
			parts << item;
		}
	}
	if (parts.isEmpty())
		return false;
	const QPointF movement = -point;
	for (QGraphicsItem *item : parts)
		item->moveBy(movement.x(), movement.y());
	auto *command = new QUndoCommand(QObject::tr("set the insertion point", "undo caption"));
	new MovePartsCommand(movement, scene, parts, command);
		//A frame set by hand goes with the drawing
	const QRect frame = scene->fixedFrame();
	if (!frame.isNull())
		new FixedFrameCommand(scene, frame, frame.translated(movement.toPoint()), command);
	scene->undoStack().push(command);
	scene->update();
	return true;
}

bool ESEventSetOrigin::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
	updateHelpCross(target(m_scene, event->scenePos()));
	return true;
}

bool ESEventSetOrigin::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		moveOriginTo(m_scene, target(m_scene, event->scenePos()));
	else if (event->button() != Qt::RightButton)
		return false;
	m_running = false;
	m_editor->UncheckAddPrimitive();
	return true;
}
