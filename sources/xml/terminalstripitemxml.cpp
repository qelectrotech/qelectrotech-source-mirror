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
#include "terminalstripitemxml.h"

#include "../diagram.h"
#include "../qetproject.h"
#include "../qetxml.h"
#include "../TerminalStrip/GraphicsItem/terminalstripitem.h"
#include "../TerminalStrip/terminalstrip.h"

#include <QUuid>

const QString STRIP_ITEM_TAG_NAME { QStringLiteral("terminal_strip_item") };
const QString STRIP_ITEMS_TAG_NAME { QStringLiteral("terminal_strip_items") };

/**
 * @brief TerminalStripItemXml::toXml
 * Save the vector of @a items as child of an xml element with tag "terminal_strip_tems"
 * @param items : items to save in xml
 * @param document : parent document used to create the QDomElement returned by this function.
 * @return QDomElement where are saved @a items.
 */
QDomElement TerminalStripItemXml::toXml(const QVector<TerminalStripItem *> &items, QDomDocument &document)
{
	auto dom_element = document.createElement(STRIP_ITEMS_TAG_NAME);
	for (const auto &item : items)
	{
		const auto child_ = toXml(item, document);
		if (!child_.isNull()) {
			dom_element.appendChild(child_);
		}
	}

	return dom_element;
}

/**
 * @brief TerminalStripItemXml::fromXml
 * Load items stored in @a xml_elmt into @a diagram
 * @a xml_elmt must have a child element with tag name "terminal_strip_tems"
 * @param diagram
 * @param xml_elmt
 * @return a vector of added terminal strip item
 */
QVector<TerminalStripItem *> TerminalStripItemXml::fromXml(Diagram *diagram, const QDomElement &xml_elmt)
{
	QVector<TerminalStripItem *> returned_vector;

	for (const auto &dom_elmt : QETXML::subChild(xml_elmt,
												 STRIP_ITEMS_TAG_NAME,
												 STRIP_ITEM_TAG_NAME))
	{
		auto strip_item = new TerminalStripItem();
		diagram->addItem(strip_item);
		returned_vector << strip_item;

		TerminalStripItemXml::fromXml(strip_item, diagram->project(), dom_elmt);
	}

	return returned_vector;
}

/**
 * @brief TerminalStripItemXml::toXml
 * Save @a item to an xml element with tag "terminal_strip_item"
 * @param item : item to save in xml
 * @param document : parent document used to create the QDomElement returned by this function.
 * @return QDomElement where are saved @a item, note that the returned QDomElement can be null.
 */
QDomElement TerminalStripItemXml::toXml(TerminalStripItem *item, QDomDocument &document)
{
	if (item->terminalStrip())
	{
		//Terminal strip item dom element
		auto dom_element = document.createElement(STRIP_ITEM_TAG_NAME);

		auto dom_strip = document.createElement(QStringLiteral("terminal_strip"));
		dom_strip.setAttribute(QStringLiteral("uuid"), item->terminalStrip()->uuid().toString());
		dom_element.appendChild(dom_strip);

		dom_element.appendChild(QETXML::qGraphicsItemPosToXml(item, document));

		return dom_element;
	} else {
		return QDomElement();
	}
}

/**
 * @brief TerminalStripItemXml::fromXml
 * Restor the state of a terminal strip item from @a xml_elmt.
 * The @a xml_elmt tag name must be "terminal_strip_item"
 * @param item
 * @param project
 * @param xml_elmt
 * @return
 */
bool TerminalStripItemXml::fromXml(TerminalStripItem *item, QETProject *project, const QDomElement &xml_elmt)
{
	if (xml_elmt.tagName() == STRIP_ITEM_TAG_NAME)
	{
		bool a{false};

		const auto ts = xml_elmt.firstChildElement(QStringLiteral("terminal_strip"));
		if (!ts.isNull())
		{
			const QUuid uuid_(ts.attribute(QStringLiteral("uuid")));
			item->m_pending_strip_uuid = uuid_;

			for (const auto &ts : project->terminalStrip()) {
				if (ts->uuid() == uuid_) {
					item->setTerminalStrip(ts);
					a = true;
					break;
				}
			}
		}

		bool b{QETXML::qGraphicsItemPosFromXml(item,
											   xml_elmt.firstChildElement(QStringLiteral("pos")))};

		return (a && b);
	}
	return false;
}
