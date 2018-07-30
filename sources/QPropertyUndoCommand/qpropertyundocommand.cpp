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
#include "qpropertyundocommand.h"
#include <QPropertyAnimation>

/**
 * @brief QPropertyUndoCommand::QPropertyUndoCommand
 * Default constructor with old and new value
 * This command don't take ownership of @object
 * @param object
 * @param old_value
 * @param new_value
 */
QPropertyUndoCommand::QPropertyUndoCommand(QObject *object, const char *property_name, const QVariant &old_value, const QVariant &new_value, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_object(object),
	m_property_name(property_name),
	m_old_value(old_value),
	m_new_value(new_value)
{}

/**
 * @brief QPropertyUndoCommand::QPropertyUndoCommand
 * Default constructor with old value.
 * Call setNewValue to setup the new value of the edited QObject
 * This command don't take ownership of @object
 * @param object
 * @param old_value
 * @param parent
 */
QPropertyUndoCommand::QPropertyUndoCommand(QObject *object, const char *property_name, const QVariant &old_value, QUndoCommand *parent) :
	QUndoCommand(parent),
	m_object(object),
	m_property_name(property_name),
	m_old_value(old_value)
{}

QPropertyUndoCommand::QPropertyUndoCommand(const QPropertyUndoCommand *other)
{
	m_object        = other->m_object;
	m_property_name = other->m_property_name;
	m_old_value     = other->m_old_value;
	m_new_value     = other->m_new_value;
	m_animate       = other->m_animate;
	m_first_time    = other->m_first_time;
	setText(other->text());
}

/**
 * @brief QPropertyUndoCommand::setNewValue
 * Set the new value of the property (set with redo) to @new_value
 * @param new_value
 */
void QPropertyUndoCommand::setNewValue(const QVariant &new_value) {
	m_new_value = new_value;
}

/**
 * @brief QPropertyUndoCommand::enableAnimation
 * True to enable animation
 * @param animate
 */
void QPropertyUndoCommand::enableAnimation (bool animate) {
	m_animate = animate;
}

/**
 * @brief QPropertyUndoCommand::setAnimated
 * @param animate = true for animate this undo
 * @param first_time = if true, the first animation is done at the first call of redo  
 * if false, the first animation is done at the second call of redo.
 */
void QPropertyUndoCommand::setAnimated(bool animate, bool first_time)
{
	m_animate = animate;
	m_first_time = first_time;
}

/**
 * @brief QPropertyUndoCommand::mergeWith
 * Try to merge this command with other command
 * @param other
 * @return true if was merged, else false
 */
bool QPropertyUndoCommand::mergeWith(const QUndoCommand *other)
{
	if (id() != other->id() || other->childCount()) return false;
	QPropertyUndoCommand const *undo = static_cast<const QPropertyUndoCommand *>(other);
	if (m_object != undo->m_object || m_property_name != undo->m_property_name) return false;
	m_new_value = undo->m_new_value;
	return true;
}

/**
 * @brief QPropertyUndoCommand::redo
 * Redo this command
 */
void QPropertyUndoCommand::redo()
{
	if (m_object->property(m_property_name) != m_new_value)
	{
		if (m_animate && m_first_time)
		{
			QPropertyAnimation *animation = new QPropertyAnimation(m_object, m_property_name);
			animation->setStartValue(m_old_value);
			animation->setEndValue(m_new_value);
			animation->start(QAbstractAnimation::DeleteWhenStopped);
		}
		else
		{
			m_object->setProperty(m_property_name, m_new_value);
			m_first_time = true;
		}
	}

	QUndoCommand::redo();
}

/**
 * @brief QPropertyUndoCommand::undo
 * Undo this command
 */
void QPropertyUndoCommand::undo()
{
	if (m_object->property(m_property_name) != m_old_value)
	{
		if (m_animate)
		{
			QPropertyAnimation *animation = new QPropertyAnimation(m_object, m_property_name);
			animation->setStartValue(m_new_value);
			animation->setEndValue(m_old_value);
			animation->start(QAbstractAnimation::DeleteWhenStopped);
		}
		else
			m_object->setProperty(m_property_name, m_old_value);
	}

	QUndoCommand::undo();
}
