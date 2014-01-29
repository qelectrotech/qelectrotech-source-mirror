/*
	Copyright 2006-2014 The QElectroTech Team
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
#ifndef CONDUCTORAUTONUMEROTATION_H
#define CONDUCTORAUTONUMEROTATION_H

#include "qetgraphicsitem/conductor.h"
#include "numerotationcontext.h"
#include "autonumerotation.h"

class ConductorAutoNumerotation: public AutoNumerotation
{
	public:
	//constructors & destructor
	ConductorAutoNumerotation (Conductor *);
	ConductorAutoNumerotation (Diagram *);

	//methods
	void setConductor(Conductor *);
	void numerate();
	void numerateDiagram();
	void removeNumOfDiagram();

	public slots:
	void applyText(QString);

	private:
	//methods
	void numeratePotential ();
	void numerateNewConductor ();

	//attributes
	Conductor *conductor_;
	QSet <Conductor *> conductor_list;
};

bool eachIsEqual (const QStringList &);

#endif // CONDUCTORAUTONUMEROTATION_H
