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
#include "elementfactory.h"

#include "../qetgraphicsitem/masterelement.h"
#include "../qetgraphicsitem/reportelement.h"
#include "../qetgraphicsitem/simpleelement.h"
#include "../qetgraphicsitem/slaveelement.h"
#include "../qetgraphicsitem/terminalelement.h"

#include <QDomElement>

ElementFactory* ElementFactory::factory_ = nullptr;
/**
	@brief ElementFactory::createElement
	@param location create element at this location
	@param qgi parent item for this element
	@param state state of the creation
	@return the element or 0
*/
Element * ElementFactory::createElement(const ElementsLocation &location, QGraphicsItem *qgi, int *state)
{
	if (Q_UNLIKELY( !(location.isElement() && location.exist()) ))
	{
		if (state)
			*state = 1;
		return nullptr;
	}

		//Read the link_type from the (cached) QDom definition instead of
		//running a second, pugixml parse of the same definition: this
		//dispatch runs once per element instance on project load, and the
		//pugixml detour was ~4 % of the whole load time of a big project.
	const QString link_type =
		location.xml().attribute(QStringLiteral("link_type"));
	if (!link_type.isEmpty())
	{
		if (link_type == QLatin1String("next_report") || link_type == QLatin1String("previous_report"))
			return (new ReportElement(location, link_type, qgi, state));
		if (link_type == QLatin1String("master"))
			return (new MasterElement   (location, qgi, state));
		if (link_type == QLatin1String("slave"))
			return (new SlaveElement    (location, qgi, state));
		if (link_type == QLatin1String("terminal"))
			return (new TerminalElement (location, qgi, state));
	}

		//default if nothing match for link_type
	return (new SimpleElement(location, qgi, state));
}
