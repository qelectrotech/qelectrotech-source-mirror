/*
	Copyright 2006-2022 The QElectroTech Team
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
#include <QGraphicsItemGroup>
#include <QPropertyAnimation>

#include "movegraphicsitemcommand.h"

#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/elementtextitemgroup.h"

#include "../diagram.h"

/**
 * @brief MoveGraphicsItemCommand::MoveGraphicsItemCommand
 * @param diagram : Diagram where the movement occur
 * @param content : content aka QGraphicsItem to move
 * @param movement : the movement to apply
 * @param parent : parent undo command
 */
MoveGraphicsItemCommand::MoveGraphicsItemCommand(Diagram *diagram,
												 const DiagramContent &content,
												 const QPointF &movement,
												 QUndoCommand *parent) :
	QUndoCommand{parent},
	m_diagram{diagram},
	m_content{content},
	m_movement{movement}
{
	const auto moved_content_sentence = m_content.sentence(DiagramContent::Elements
														   | DiagramContent::TextFields
														   | DiagramContent::ConductorsToUpdate
														   | DiagramContent::ConductorsToMove
														   | DiagramContent::Images
														   | DiagramContent::Shapes
														   | DiagramContent::ElementTextFields
														   | DiagramContent::TerminalStrip);

	setText(QString(QObject::tr("déplacer %1",
								"undo caption - %1 is a sentence listing the moved content").arg(moved_content_sentence)));
}

/**
 * @brief MoveGraphicsItemCommand::undo
 * Reimplemented from QUndoCommand::undo()
 */
void MoveGraphicsItemCommand::undo()
{
	if (m_diagram)
	{
		m_diagram->showMe();
		m_anim_group.setDirection(QAnimationGroup::Forward);
		m_anim_group.start();
	}
	QUndoCommand::undo();
}

/**
 * @brief MoveGraphicsItemCommand::redo
 * Reimplemented from QUndoCommand::redo()
 */
void MoveGraphicsItemCommand::redo()
{
	if (m_diagram)
	{
		m_diagram->showMe();
		if (m_first_redo)
		{
			m_first_redo = false;
			move(-m_movement);
		}
		else
		{
			m_anim_group.setDirection(QAnimationGroup::Backward);
			m_anim_group.start();
		}
	}
	QUndoCommand::redo();
}

/**
 * @brief MoveGraphicsItemCommand::move
 * Apply @a movement to items of m_content
 * @param movement
 */
void MoveGraphicsItemCommand::move(const QPointF &movement)
{
	for (auto &&qgi : m_content.items(DiagramContent::Elements
									  | DiagramContent::TextFields
									  | DiagramContent::Images
									  | DiagramContent::Shapes
									  | DiagramContent::TextGroup
									  | DiagramContent::ElementTextFields
									  | DiagramContent::Tables
									  | DiagramContent::TerminalStrip))
	{
			//If item have a parent and the parent is in m_content,
			//we don't apply movement because this item will be moved by his parent
		if (const auto parent_ = qgi->parentItem()) {
			if (m_content.items().contains(parent_)) {
				continue;
			}
		}

		if (const auto graphics_object = qgi->toGraphicsObject()) {
			setupAnimation(graphics_object,
						   "pos",
						   graphics_object->pos(),
						   graphics_object->pos() + movement);
		}
		else if (qgi->type() == QGraphicsItemGroup::Type)
		{
				//ElementTextItemGroup is a QObject not a QGraphicsObject
			if (ElementTextItemGroup *etig = dynamic_cast<ElementTextItemGroup *>(qgi)) {
				setupAnimation(etig,
							   "pos",
							   etig->pos(),
							   etig->pos() + movement);
			}
		}
		else
		{
			qgi->setPos(qgi->pos() + movement);
		}
	}

		//Move some conductors
	for (const auto &conductor : std::as_const(m_content.m_conductors_to_move)) {
		setupAnimation(conductor,
					   "pos",
					   conductor->pos(),
					   conductor->pos() + movement);
	}

		//Recalculate the path of other conductors
	for (const auto &conductor : std::as_const(m_content.m_conductors_to_update)) {
		setupAnimation(conductor, "animPath", 1, 1);
	}
}

/**
 * @brief MoveGraphicsItemCommand::setupAnimation
 * Create the animation used for the movement.
 * @see QPropertyAnimation.
 * @param target
 * @param property_name
 * @param start
 * @param end
 */
void MoveGraphicsItemCommand::setupAnimation(QObject *target,
											 const QByteArray &property_name,
											 const QVariant &start,
											 const QVariant &end)
{
	QPropertyAnimation *animation{new QPropertyAnimation(target, property_name)};
	animation->setDuration(300);
	animation->setStartValue(start);
	animation->setEndValue(end);
	animation->setEasingCurve(QEasingCurve::OutQuad);
	m_anim_group.addAnimation(animation);
}
