/*
	Copyright 2006-2013 The QElectroTech Team
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
#include "reportelement.h"
#include "elementtextitem.h"
#include "diagramposition.h"

ReportElement::ReportElement(const ElementsLocation &location, QGraphicsItem *qgi, Diagram *s, int *state) :
	CustomElement(location, qgi, s, state)
{
	texts().at(0)->setNoEditable();
}

void ReportElement::linkToElement(Element * elmt) {
	texts().at(0)->setPlainText(QString ("%1-%2").arg(elmt->diagram()->folioIndex() + 1)
												 .arg(elmt->diagram() -> convertPosition(elmt -> scenePos()).toString()));
}

int ReportElement::linkType() const {
	return REPORT;
}
