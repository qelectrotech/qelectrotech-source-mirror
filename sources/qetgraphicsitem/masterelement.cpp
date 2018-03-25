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
#include "masterelement.h"
#include "crossrefitem.h"
#include "diagram.h"
#include "dynamicelementtextitem.h"
#include <QRegularExpression>

/**
 * @brief MasterElement::MasterElement
 * Default constructor
 * @param location location of xml definition
 * @param qgi parent QGraphicItem
 * @param s parent diagram
 * @param state int used to know if the creation of element have error
 */
MasterElement::MasterElement(const ElementsLocation &location, QGraphicsItem *qgi, int *state) :
	CustomElement(location, qgi, state)
{
	m_link_type = Element::Master;
}

/**
 * @brief MasterElement::~MasterElement
 * default destructor
 */
MasterElement::~MasterElement() {
	unlinkAllElements();
}

/**
 * @brief MasterElement::linkToElement
 * Link this master to another element
 * For this class element must be a slave
 * @param elmt
 */
void MasterElement::linkToElement(Element *elmt)
{
		// check if element is slave and if isn't already linked
	if (elmt->linkType() == Slave && !connected_elements.contains(elmt))
	{
		connected_elements << elmt;
		elmt->linkToElement(this);

		XRefProperties xrp = diagram()->project()->defaultXRefProperties(kindInformations()["type"].toString());
		if (!m_Xref_item && xrp.snapTo() == XRefProperties::Bottom)
			m_Xref_item = new CrossRefItem(this); //create cross ref item if not yet
		else
			aboutDeleteXref();
		
		emit linkedElementChanged();
	}
}

/**
 * @brief MasterElement::unlinkAllElements
 * Unlink all of the element in the QList connected_elements
 */
void MasterElement::unlinkAllElements()
{
		// if this element is free no need to do something
	if (!isFree())
	{
		foreach(Element *elmt, connected_elements)
			unlinkElement(elmt);
		emit linkedElementChanged();
	}
}

/**
 * @brief MasterElement::unlinkElement
 * Unlink the given elmt in parametre
 * @param elmt element to unlink from this
 */
void MasterElement::unlinkElement(Element *elmt)
{
		//Ensure elmt is linked to this element
	if (connected_elements.contains(elmt))
	{
		connected_elements.removeOne(elmt);
		elmt -> unlinkElement  (this);
		elmt -> setHighlighted (false);

		aboutDeleteXref();
		emit linkedElementChanged();
	}
}

/**
 * @brief MasterElement::initLink
 * @param project
 * Call init Link from custom element and after
 * call update label for setup it.
 */
void MasterElement::initLink(QETProject *project) {
	//Create the link with other element if needed
	CustomElement::initLink(project);
}

/**
 * @brief MasterElement::XrefBoundingRect
 * @return The bounding rect of the Xref, if this element
 * haven't got a xref, return a default QRectF
 */
QRectF MasterElement::XrefBoundingRect() const
{
	if(m_Xref_item)
		return m_Xref_item->boundingRect();
	else
		return QRectF();
}

QVariant MasterElement::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
	if(change == QGraphicsItem::ItemSceneHasChanged && m_first_scene_change)
	{
		m_first_scene_change = false;
		connect(diagram()->project(), &QETProject::XRefPropertiesChanged, this, &MasterElement::xrefPropertiesChanged);
	}
	return CustomElement::itemChange(change, value);
}

void MasterElement::xrefPropertiesChanged()
{
	if(!diagram())
		return;
	
	XRefProperties xrp = diagram()->project()->defaultXRefProperties(kindInformations()["type"].toString());
	if(xrp.snapTo() == XRefProperties::Bottom)
	{
			//We create a Xref, and just after we call aboutDeleteXref,
			//because the Xref may be useless.
		if(!m_Xref_item)
			m_Xref_item = new CrossRefItem(this);
	}
	aboutDeleteXref();
}

/**
 * @brief MasterElement::aboutDeleteXref
 * Check if Xref item must be displayed, if not, delete it.
 * If Xref item is deleted or already not used (nullptr) return true;
 * Else return false if Xref item is used
 * NOTICE : Xref can display nothing but not be deleted so far.
 * For exemple, if Xref is display has cross, only power contact are linked and
 * option show power contact is disable, the cross isn't draw.
 * @return
 */
void MasterElement::aboutDeleteXref()
{
	if(!m_Xref_item)
		return;
	
	XRefProperties xrp = diagram()->project()->defaultXRefProperties(kindInformations()["type"].toString());
	if (xrp.snapTo() != XRefProperties::Bottom && m_Xref_item)
	{
		delete m_Xref_item;
		m_Xref_item = nullptr;
		return;
	}
	
	if (m_Xref_item->boundingRect().isNull())
	{
		delete m_Xref_item;
		m_Xref_item = nullptr;
		return;
	}
}
