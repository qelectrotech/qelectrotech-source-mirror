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
#include "elementprovider.h"
#include "qetproject.h"
#include "diagram.h"
#include "qetgraphicsitem/element.h"

/**
 * @brief ElementProvider::ElementProvider Constructor
 * @param prj the project where we must find element
 * @param diagram the diagram to exclude from the search
 */
ElementProvider::ElementProvider(QETProject *prj, Diagram *diagram)
{
	diag_list = prj->diagrams();
	diag_list.removeOne(diagram);
}

/**
 * @brief ElementProvider::ElementProvider Constructor
 * @param diag Diagram to search
 */
ElementProvider::ElementProvider(Diagram *diag) {
	diag_list << diag;
}

/**
 * @brief ElementProvider::FreeElement
 * Search and return the asked element corresponding  with the given filter
 * All returned element are free, ie element aren't connected with another element
 * @param filter
 * the filter for search element
 * (You can find all filter with the #define in Element.h)
 * @return
 */
QList <Element *> ElementProvider::freeElement(const int filter) const{
	QList <Element *> free_elmt;

	//serch in all diagram
	foreach (Diagram *d, diag_list) {
		//get all element in diagram d
		QList <Element *> elmt_list;
		elmt_list = d->elements();
		foreach (Element *elmt, elmt_list) {
			if (filter & elmt->linkType())
				if (elmt->isFree()) free_elmt << elmt;
		}
	}
	return (free_elmt);
}

/**
 * @brief ElementProvider::fromUuids
 * @param uuid_list list of uuid must be found
 * @return all elements with uuid corresponding to uuid in @uuid_list
 */
QList <Element *> ElementProvider::fromUuids(QList<QUuid> uuid_list) const {
	QList <Element *> found_element;

	foreach (Diagram *d, diag_list) {
		foreach(Element *elmt, d->elements()) {
			if (uuid_list.contains(elmt->uuid())) {
				found_element << elmt;
				uuid_list.removeAll(elmt->uuid());
			}
		}
	}
	return found_element;
}

/**
 * @brief ElementProvider::find
 * Search and return the asked element corresponding  with the given filter
 * @param filter
 * the filter for search element
 * (You can find all filter with the #define in Element.h)
 */
QList <Element *> ElementProvider::find(const int filter) const {
	QList <Element *> elmt_;

	//serch in all diagram
	foreach (Diagram *d, diag_list) {
		//get all element in diagram d
		QList <Element *> elmt_list;
		elmt_list = d->elements();
		foreach (Element *elmt, elmt_list) {
			if (filter & elmt->linkType())
				elmt_ << elmt;
		}
	}
	return (elmt_);
}
