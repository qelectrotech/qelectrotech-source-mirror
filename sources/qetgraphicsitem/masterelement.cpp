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
#include "elementtextitem.h"
#include "diagram.h"
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
	CustomElement(location, qgi, state),
	m_Xref_item (nullptr)
{
	link_type_ = Master;
	connect(this, SIGNAL(elementInfoChange(DiagramContext, DiagramContext)), this, SLOT(updateLabel(DiagramContext, DiagramContext)));
	connect(this, &Element::updateLabel, [this]() {this->updateLabel(this->elementInformations(), this->elementInformations());});
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

		if (!m_Xref_item) m_Xref_item = new CrossRefItem(this); //create cross ref item if not yet

		connect(elmt, SIGNAL(xChanged()),    m_Xref_item, SLOT(updateLabel()));
		connect(elmt, SIGNAL(yChanged()),    m_Xref_item, SLOT(updateLabel()));
		connect(elmt, SIGNAL(updateLabel()), m_Xref_item, SLOT(updateLabel()));
		m_Xref_item -> updateLabel();
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
		for (Element *elmt: connected_elements)
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

			//update the graphics cross ref
		disconnect(elmt, SIGNAL(xChanged()),    m_Xref_item, SLOT(updateLabel()));
		disconnect(elmt, SIGNAL(yChanged()),    m_Xref_item, SLOT(updateLabel()));
		disconnect(elmt, SIGNAL(updateLabel()), m_Xref_item, SLOT(updateLabel()));

		m_Xref_item -> updateLabel();
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
	updateLabel(DiagramContext(), elementInformations());
}

/**
 * @brief MasterElement::updateLabel
 * update label of this element
 * and the comment item if he's displayed.
 */
void MasterElement::updateLabel(DiagramContext old_info, DiagramContext new_info)
{
	QString old_formula = old_info["formula"].toString();
	QString new_formula = new_info["formula"].toString();

	setUpConnectionForFormula(old_formula, new_formula);

	QString label = autonum::AssignVariables::formulaToLabel(new_formula, m_autoNum_seq, diagram(), this);

	if (label.isEmpty())
	{
		setTaggedText("label", new_info["label"].toString());
	}
	else
	{
		bool visible = m_element_informations.contains("label") ? m_element_informations.keyMustShow("label") : true;
		m_element_informations.addValue("label", label, visible);
		setTaggedText("label", label);
	}

	if (ElementTextItem *eti = taggedText("label"))
	{
		new_info["label"].toString().isEmpty() ? eti->setVisible(true) : eti -> setVisible(new_info.keyMustShow("label"));
	}


	//Delete or update the xref
	if (m_Xref_item) {
		m_Xref_item -> updateLabel();
		aboutDeleteXref();
	}
	else {
		QString comment   = elementInformations()["comment"].toString();
		bool    must_show_comment = elementInformations().keyMustShow("comment");

		QString location  = elementInformations()["location"].toString();
		bool	must_show_location = elementInformations().keyMustShow("location");

		if (! (comment.isEmpty() || !must_show_comment) || !(location.isEmpty() || !must_show_location)) {
			m_Xref_item = new CrossRefItem(this);
		}
	}
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
bool MasterElement::aboutDeleteXref() {
	if(!m_Xref_item) return true;
	if(!linkedElements().isEmpty()) return false;

	if (m_Xref_item -> boundingRect().isNull()) {
		delete m_Xref_item;
		m_Xref_item = nullptr;
		return true;
	}

	return false;
}
