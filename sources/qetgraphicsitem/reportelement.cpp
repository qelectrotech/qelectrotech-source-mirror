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

/**
 * @brief ReportElement::linkToElement
 * Link this element to the other element
 * @param elmt
 * element to be linked with this
 */
void ReportElement::linkToElement(Element * elmt) {
	//ensure elmt isn't already linked
	bool i=true;
	if (!this->isFree()){
		if (connected_elements.first() == elmt) i = false;
	}

	//ensure elmt is a report
	if (elmt->linkType() == REPORT && i) {
		unLinkAllElements();
		connected_elements << elmt;
		connect(elmt, SIGNAL(positionChange(QPointF)), this, SLOT(updateLabel()));
		updateLabel();
		elmt->linkToElement(this);
	}
}

/**
 * @brief ReportElement::unLinkAllElements
 * Unlink all of the element in the QList connected_elements
 */
void ReportElement::unLinkAllElements(){
	if (!isFree()){
		QList <Element *> tmp_elmt = connected_elements;

		foreach(Element *elmt, connected_elements) {
			disconnect(elmt, SIGNAL(positionChange(QPointF)), this, SLOT(updateLabel()));
		}
		connected_elements.clear();
		updateLabel();

		foreach(Element *elmt, tmp_elmt){
			elmt->unLinkAllElements();
		}
	}
}

/**
 * @brief ReportElement::linkType
 * @return the kind of link type
 */
int ReportElement::linkType() const {
	return REPORT;
}

/**
 * @brief ReportElement::updateLabel
 * Update the displayed label.
 * ie the folio and position of the linked folio report
 */
void ReportElement::updateLabel() {
	if (!connected_elements.isEmpty()){
		Element *elmt = connected_elements.at(0);
		texts().at(0)->setPlainText(QString ("%1-%2").arg(elmt->diagram()->folioIndex() + 1)
													 .arg(elmt->diagram() -> convertPosition(elmt -> scenePos()).toString()));
	} else {
		texts().at(0)->setPlainText("_");
	}
}
