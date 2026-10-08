// SPDX-License-Identifier: GPL-2.0-or-later
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
#ifndef CONDUCTORMULTIEDIT_H
#define CONDUCTORMULTIEDIT_H

#include "QPropertyUndoCommand/qpropertyundocommand.h"
#include "conductorproperties.h"

#include <QCoreApplication>
#include <QList>
#include <QPointF>
#include <QSet>

#include <algorithm>

/**
	@brief ConductorMultiEdit
	The rules for editing several conductors at once in the Selection
	properties panel (#500). They are templates over the conductor type so
	they can be tested without a diagram: T needs properties(), a
	"properties" Q_PROPERTY, and relatedPotentialConductors() returning
	QSet<T *>.
*/
namespace ConductorMultiEdit
{
	/**
		@brief sortByPosition
		Sort top to bottom, then left to right, so the panel shows the same
		"first" conductor whatever order the scene lists the selection in.
		@param list : the conductors
		@param pos_of : callable giving a conductor's position
	*/
	template <typename T, typename PosOf>
	void sortByPosition(QList<T *> &list, PosOf pos_of)
	{
		std::stable_sort(list.begin(), list.end(), [&pos_of](T *a, T *b) {
			const QPointF pa = pos_of(a);
			const QPointF pb = pos_of(b);
			if (pa.y() != pb.y())
				return pa.y() < pb.y();
			return pa.x() < pb.x();
		});
	}

	/**
		@brief targets
		@param selected : the edited conductors
		@param apply_all : true to add every conductor of their potentials
		@return each conductor an edit reaches, once
	*/
	template <typename T>
	QList<T *> targets(const QList<T *> &selected, bool apply_all)
	{
		QList<T *> list;
		QSet<T *> seen;
		for (T *conductor : selected)
		{
				//Already reached through an earlier conductor's potential:
				//its own potential is the same one.
			if (!conductor || seen.contains(conductor))
				continue;
			seen.insert(conductor);
			list << conductor;
			if (!apply_all)
				continue;
			const QSet<T *> potential = conductor->relatedPotentialConductors();
			for (T *other : potential)
			{
				if (!seen.contains(other))
				{
					seen.insert(other);
					list << other;
				}
			}
		}
		return list;
	}

	/**
		@brief undo
		@param targets : the conductors to change, see targets()
		@param shown : the properties as the panel showed them
		@param edited : the properties after the user's edit
		@return one undo step giving each target the fields that differ
		between shown and edited, or nullptr if no conductor changes
	*/
	template <typename T>
	QUndoCommand *undo(const QList<T *> &targets,
			   const ConductorProperties &shown,
			   const ConductorProperties &edited)
	{
		if (edited == shown)
			return nullptr;

		auto *undo = new QUndoCommand();
		int changed = 0;
		for (T *conductor : targets)
		{
			const ConductorProperties old_properties = conductor->properties();
			ConductorProperties properties = old_properties;
			properties.applyChanges(shown, edited);
			if (properties == old_properties)
				continue;

			QVariant old_value, new_value;
			old_value.setValue(old_properties);
			new_value.setValue(properties);
			new QPropertyUndoCommand(conductor, "properties", old_value, new_value, undo);
			++changed;
		}

		if (!changed)
		{
			delete undo;
			return nullptr;
		}
		undo->setText(changed == 1
			? QCoreApplication::translate("ConductorPropertiesEditorWidget",
				"Modifier les propriétés d'un conducteur", "undo caption")
			: QCoreApplication::translate("ConductorPropertiesEditorWidget",
				"Modifier les propriétés de plusieurs conducteurs", "undo caption"));
		return undo;
	}

		/// The fields a user types in. When the selected conductors do not
		/// agree on one, the panel shows it blank.
	enum TextField {
		Text, Formula, Function, TensionProtocol,
		WireColor, WireSection, Cable, Bus
	};

	inline QList<TextField> textFields()
	{
		return {Text, Formula, Function, TensionProtocol,
			WireColor, WireSection, Cable, Bus};
	}

	inline QString &textField(ConductorProperties &p, TextField field)
	{
		switch (field)
		{
			case Text:            return p.text;
			case Formula:         return p.m_formula;
			case Function:        return p.m_function;
			case TensionProtocol: return p.m_tension_protocol;
			case WireColor:       return p.m_wire_color;
			case WireSection:     return p.m_wire_section;
			case Cable:           return p.m_cable;
			case Bus:             break;
		}
		return p.m_bus;
	}

	/**
		@brief mixedTextFields
		@return the text fields whose value is not the same in all of list
	*/
	inline QList<TextField> mixedTextFields(QList<ConductorProperties> list)
	{
		QList<TextField> mixed;
		if (list.size() < 2)
			return mixed;
		for (TextField field : textFields())
		{
			const QString first = textField(list.first(), field);
			for (ConductorProperties &p : list)
			{
				if (textField(p, field) != first)
				{
					mixed << field;
					break;
				}
			}
		}
		return mixed;
	}

	/**
		@brief shown
		@return what the panel shows for list: the first conductor's
		properties, with the mixed text fields blank. Typing any value
		in a blank field, even the first conductor's, then differs from
		what was shown, so it is applied to every conductor.
	*/
	inline ConductorProperties shown(const QList<ConductorProperties> &list,
					 const QList<TextField> &mixed)
	{
		if (list.isEmpty())
			return ConductorProperties();
		ConductorProperties p = list.first();
		for (TextField field : mixed)
			textField(p, field).clear();
		return p;
	}
}

#endif // CONDUCTORMULTIEDIT_H
