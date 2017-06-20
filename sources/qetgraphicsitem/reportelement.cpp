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
#include "reportelement.h"
#include "elementtextitem.h"
#include "diagramposition.h"
#include "qetproject.h"
#include "diagram.h"
#include "terminal.h"
#include "conductor.h"

ReportElement::ReportElement(const ElementsLocation &location, QString link_type,QGraphicsItem *qgi, int *state) :
	CustomElement(location, qgi, state),
	m_text_field (nullptr),
	m_watched_conductor(nullptr)
{	
		/*
		 * Get text tagged label. This is work for report
		 * create after the revision 3559.
		 * for report create before, we take the first text field
		 * because report haven't got a text field tagged label
		 */
	m_text_field = taggedText("label");
	if (!m_text_field && !texts().isEmpty())
		m_text_field = texts().first();
	if (m_text_field)
		m_text_field -> setNoEditable();

	link_type == "next_report"? link_type_=NextReport : link_type_=PreviousReport;
	link_type == "next_report"? m_inverse_report=PreviousReport : m_inverse_report=NextReport;

		//We make these connections, to be always aware about the conductor properties
	if (terminals().size())
	{
		connect (terminals().first(), &Terminal::conductorWasAdded, this, &ReportElement::conductorWasAdded);
		connect (terminals().first(), &Terminal::conductorWasRemoved, this, &ReportElement::conductorWasRemoved);
	}
}

/**
 * @brief ReportElement::~ReportElement
 * Destructor
 */
ReportElement::~ReportElement()
{
	unlinkAllElements();
	if (terminals().size())
		disconnect(terminals().first(), 0, 0, 0);
	if (m_watched_conductor)
		disconnect(m_watched_conductor, &Conductor::propertiesChange, this, &ReportElement::updateLabel);
}

/**
 * @brief ReportElement::linkToElement
 * Link this element to the other element
 * @param elmt
 * element to be linked with this
 */
void ReportElement::linkToElement(Element * elmt)
{
	if (!diagram() && !elmt -> diagram())
	{
		qDebug() << "ReportElement : linkToElement : Unable to link this or element to link isn't in a diagram";
		return;
	}

		//ensure elmt isn't already linked
	bool i = true;
	if (!this -> isFree() && (connected_elements.first() == elmt))
		i = false;

		//ensure elmt is an inverse report of this element
	if ((elmt->linkType() == m_inverse_report) && i)
	{
		unlinkAllElements();
		connected_elements << elmt;
		m_formula = diagram()->project()->defaultReportProperties();
		setConnectionForFormula(m_formula);
		connect(diagram()->project(), &QETProject::reportPropertiesChanged, this, &ReportElement::reportPropertiesChange);
        connect(diagram()->project(), &QETProject::diagramRemoved, this, &ReportElement::updateLabel);
		
		if (elmt->terminals().size())
		{
			connect(elmt->terminals().first(), &Terminal::conductorWasAdded, this, &ReportElement::conductorWasAdded);
			connect(elmt->terminals().first(), &Terminal::conductorWasRemoved, this, &ReportElement::conductorWasRemoved);
		}
		
		if (!m_watched_conductor && elmt->conductors().size())
			conductorWasAdded(elmt->conductors().first());
		else
			updateLabel();

		elmt->linkToElement(this);

		emit linkedElementChanged();
	}
}

/**
 * @brief ReportElement::unLinkAllElements
 * Unlink all of the element in the QList connected_elements
 */
void ReportElement::unlinkAllElements()
{
	if (isFree())
		return;

	const QList <Element *> tmp_elmt = connected_elements;

	for (Element *elmt : tmp_elmt)
	{
		removeConnectionForFormula(m_formula);
		disconnect(diagram()->project(), &QETProject::reportPropertiesChanged, this, &ReportElement::reportPropertiesChange);
        disconnect(diagram()->project(), &QETProject::diagramRemoved, this, &ReportElement::updateLabel);
		
		if (elmt->terminals().size())
		{
			disconnect(elmt->terminals().first(), &Terminal::conductorWasAdded, this, &ReportElement::conductorWasAdded);
			disconnect(elmt->terminals().first(), &Terminal::conductorWasRemoved, this, &ReportElement::conductorWasRemoved);
		}
		connected_elements.removeAll(elmt);
			//if elmt is the owner of m_watched_conductor, we remove it
		if (elmt->conductors().contains(m_watched_conductor))
			conductorWasRemoved(m_watched_conductor);
	}
	updateLabel();

	for(Element *elmt : tmp_elmt)
	{
		elmt -> setHighlighted(false);
		elmt -> unlinkAllElements();
	}
	
	emit linkedElementChanged();
}
/**
 * @brief ReportElement::unlinkElement
 *unlink the specified element.
 *for reportelement, they must be only one linked element, so we call
 *unlinkAllElements for clear the connected_elements list.
 * @param elmt
 */
void ReportElement::unlinkElement(Element *elmt) {
	Q_UNUSED (elmt);
	unlinkAllElements();
}

/**
 * @brief ReportElement::conductorWasAdded
 * This method is called when a conduxtor is added to the potential
 * @param conductor : added conductor
 */
void ReportElement::conductorWasAdded(Conductor *conductor)
{
	if (m_watched_conductor) return;

	m_watched_conductor = conductor;
	connect(m_watched_conductor, &Conductor::propertiesChange, this, &ReportElement::updateLabel);
	updateLabel();
}

/**
 * @brief ReportElement::conductorWasRemoved
 * This method is called when a conductor is removed to the potential
 * @param conductor : removed conductor
 */
void ReportElement::conductorWasRemoved(Conductor *conductor)
{
	if (m_watched_conductor != conductor) return;

	disconnect(m_watched_conductor, &Conductor::propertiesChange, this, &ReportElement::updateLabel);
	m_watched_conductor = nullptr;

		//Get another conductor to be always up to date about the properties of the potential.

		//Get a conducteur docked to this report
	if (conductors().size())
		conductorWasAdded(conductors().first());
		//Else we get a conductor of the linked report (if any)
	else if (linkedElements().size() && linkedElements().first()->conductors().size())
		conductorWasAdded(linkedElements().first()->conductors().first());
	else
		updateLabel();
}

/**
 * @brief ReportElement::setFormula
 * Set new Formula and call updatelabel
 * @param formula : the new formula
 */
void ReportElement::setFormula(QString formula)
{
	removeConnectionForFormula(m_formula);
	m_formula = formula;
	setConnectionForFormula(m_formula);
	updateLabel();
}

/**
 * @brief ReportElement::updateLabel
 * Update the displayed label.
 * ie the folio and position of the linked folio report
 */
void ReportElement::updateLabel()
{
	if (!m_text_field) return;

	if (!connected_elements.isEmpty())
	{
		Element *elmt = connected_elements.at(0);
		QString label = m_formula;
		label = autonum::AssignVariables::formulaToLabel(label, elmt->rSequenceStruct(), elmt->diagram(), elmt);
		m_text_field -> setPlainText(label);
	}
	else
		m_text_field -> setPlainText("/");

	setTaggedText("function", (m_watched_conductor? m_watched_conductor->properties().m_function : ""));
	setTaggedText("tension-protocol", (m_watched_conductor? m_watched_conductor->properties().m_tension_protocol : ""));
}

void ReportElement::reportPropertiesChange(const QString &old_str, const QString &new_str)
{
	Q_UNUSED(old_str);
	setFormula(new_str);
}

/**
 * @brief ReportElement::setConnectionForFormula
 * Set up the required connection for the formula @str.
 * @param str
 */
void ReportElement::setConnectionForFormula(const QString &str)
{
	if (connected_elements.isEmpty() || str.isEmpty())
		return;
	
	QString string = str;
	Element *other_elmt = connected_elements.first();
	Diagram *other_diagram = other_elmt->diagram();
	
		//Because the variable %F is a reference to another text which can contain variables,
		//we must to replace %F by the real text, to check if the real text contain the variable %id
	if (other_diagram && string.contains("%F"))
	{
		m_F_str = other_diagram->border_and_titleblock.folio();
		string.replace("%F", m_F_str);
		connect(&other_diagram->border_and_titleblock, &BorderTitleBlock::titleBlockFolioChanged, this, &ReportElement::updateFormulaConnection);
	}
	
	if (other_diagram && (string.contains("%f") || string.contains("%id")))
		connect(other_diagram->project(), &QETProject::projectDiagramsOrderChanged, this, &ReportElement::updateLabel);
	if (string.contains("%l"))
		connect(other_elmt, &Element::yChanged, this, &ReportElement::updateLabel);
	if (string.contains("%c"))
		connect(other_elmt, &Element::xChanged, this, &ReportElement::updateLabel);
}

/**
 * @brief ReportElement::removeConnectionForFormula
 * Remove the existing connection made for the formula @str
 * @param str
 */
void ReportElement::removeConnectionForFormula(const QString &str)
{
	if (connected_elements.isEmpty() || str.isEmpty())
		return;
	
	QString string = str;
	Element *other_element = connected_elements.first();
	Diagram *other_diagram = other_element->diagram();
	
		//Because the variable %F is a reference to another text which can contain variables,
		//we must to replace %F by the real text, to check if the real text contain the variable %id
	if (other_diagram && string.contains("%F"))
	{
		string.replace("%F", m_F_str);
		disconnect(&other_diagram->border_and_titleblock, &BorderTitleBlock::titleBlockFolioChanged, this, &ReportElement::updateFormulaConnection);
	}
	
	if (other_diagram && (string.contains("%f") || string.contains("%id")))
		disconnect(other_diagram->project(), &QETProject::projectDiagramsOrderChanged, this, &ReportElement::updateLabel);
	if (string.contains("%l"))
		disconnect(other_element, &Element::yChanged, this, &ReportElement::updateLabel);
	if (string.contains("%c"))
		disconnect(other_element, &Element::xChanged, this, &ReportElement::updateLabel);
}

void ReportElement::updateFormulaConnection()
{
	removeConnectionForFormula(m_formula);
	setConnectionForFormula(m_formula);
	updateLabel();
}
