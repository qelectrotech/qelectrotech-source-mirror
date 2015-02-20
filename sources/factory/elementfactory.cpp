/*
	Copyright 2006-2015 The QElectroTech Team
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
#include "elementfactory.h"
#include "elementdefinition.h"
#include "elementscollectionitem.h"
#include "qetapp.h"
#include "QDomElement"
#include "simpleelement.h"
#include "reportelement.h"
#include "masterelement.h"
#include "slaveelement.h"
#include "terminalelement.h"

ElementFactory* ElementFactory::factory_ = 0;
/**
 * @brief ElementFactory::createElement
 * @param location create element at this location
 * @param qgi parent item for this elemnt
 * @param state state of the creation
 * @return the element or 0
 */
Element * ElementFactory::createElement(const ElementsLocation &location, QGraphicsItem *qgi, int *state)
{
		// recupere la definition de l'element
	ElementsCollectionItem *element_item = QETApp::collectionItem(location);
	ElementDefinition *element_definition;
	if (!element_item ||\
		!element_item -> isElement() ||\
		!(element_definition = qobject_cast<ElementDefinition *>(element_item)))
	{
		if (state) *state = 1;
		return 0;
	}

	if (element_definition->xml().hasAttribute("link_type"))
	{
		QString link_type = element_definition->xml().attribute("link_type");
		if (link_type == "next_report" || link_type == "previous_report") return (new ReportElement(location, link_type, qgi, state));
		if (link_type == "master")   return (new MasterElement   (location, qgi, state));
		if (link_type == "slave")    return (new SlaveElement    (location, qgi, state));
		if (link_type == "terminal") return (new TerminalElement (location, qgi, state));
	}
	
		//default if nothing match for link_type
	return (new SimpleElement(location, qgi, state));
}
