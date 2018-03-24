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
#ifndef QPROPERTYUNDOCOMMAND_H
#define QPROPERTYUNDOCOMMAND_H

#include <QUndoCommand>
#include <QVariant>

class QObject;

/**
 * @brief The QPropertyUndoCommand class
 * This undo command manage QProperty of a QObject.
 * This undo command can use QPropertyAnimation to animate the change when undo/redo is call
 * To use animation call setAnimated(true). By default animation is disable.
 * Some QVariant date can't be animated and result this command don't work.
 */
class QPropertyUndoCommand : public QUndoCommand
{
	public:
		QPropertyUndoCommand(QObject *object, const char *property_name, const QVariant &old_value, const QVariant &new_value, QUndoCommand *parent = nullptr);
		QPropertyUndoCommand(QObject *object, const char *property_name, const QVariant &old_value, QUndoCommand *parent = nullptr);
		QPropertyUndoCommand(const QPropertyUndoCommand *other);

		void setNewValue(const QVariant &new_value);
		void enableAnimation (bool animate = true);
		void setAnimated(bool animate = true, bool first_time = true);

		int id() const override{return 10000;}
		bool mergeWith(const QUndoCommand *other) override;
		void redo() override;
		void undo() override;

	private:
		QObject *m_object = nullptr;
		const char *m_property_name;
		QVariant m_old_value, m_new_value;
		bool m_animate = false,
			 m_first_time = true;
};

#endif // QPROPERTYUNDOCOMMAND_H
