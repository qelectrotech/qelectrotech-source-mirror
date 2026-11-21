/*
	Copyright 2006-2026 The QElectroTech Team
	This file is part of QElectroTech.

	QElectroTech is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	(at your option) any later version.

	QElectroTech is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with QElectroTech. If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef JUNCTIONELEMENT_H
#define JUNCTIONELEMENT_H

#include "element.h"

class QETProject;

/**
	@brief The JunctionElement class

	A junction is a purely visual connection point on a wire: a corner, a
	T-junction, a crossing. It is drawn in the cabinet nowhere, it is bought
	never, and it is numbered never (#1500).

	What it shares with a terminal is the pass-through: its terminals all sit
	on one potential, so the voltage, colour and section of a wire carry on
	through it, exactly as they did when the connection symbols were typed as
	terminals. What it does not share is the terminal strip: it is no terminal,
	so the terminal strip manager, the terminal numbering and the parts list
	never see it.

	The terminals of a junction also name the wiring list's way through it:
	which one the wire comes in at and which ones it goes on to are read
	from the symbol's own definition, so that a wire through a junction is
	listed as the one run it is, rather than as two rows ending at the
	junction (@see WiringListExport::junctionRoles).
*/
class JunctionElement : public Element {

	Q_OBJECT

	public:
		explicit JunctionElement(
			const ElementsLocation &,
			QGraphicsItem * = nullptr,
			int * = nullptr);
		~JunctionElement() override;

		void initLink(QETProject *project) override;
};

#endif // JUNCTIONELEMENT_H
