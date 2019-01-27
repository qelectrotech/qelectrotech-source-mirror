/*
	Copyright 2006-2019 The QElectroTech Team
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
#ifndef CONDUCTORCREATOR_H
#define CONDUCTORCREATOR_H

class Diagram;
class QPolygonF;
class Terminal;
class Conductor;

#include "conductorproperties.h"
#include "conductorproperties.h"
#include "assignvariables.h"

#include <QList>


/**
 * @brief The ConductorCreator class
 * This class create one or several or several conductors
 * in a given context
 */
class ConductorCreator
{
	public:
		ConductorCreator(Diagram *d, QList<Terminal *> terminals_list);
		static void create(Diagram *d, const QPolygonF &polygon);
		
	private:
		void setUpPropertieToUse();
		QList<Conductor *> existingPotential();
		Terminal *hubTerminal();
		
		
		QList<Terminal *> m_terminals_list;
		ConductorProperties m_properties;
		autonum::sequentialNumbers m_sequential_number;
		
};

#endif // CONDUCTORCREATOR_H
