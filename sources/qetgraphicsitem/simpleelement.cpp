/*
	Copyright 2006-2016 The QElectroTech Team
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
#include "simpleelement.h"
#include "commentitem.h"
#include "elementtextitem.h"
#include "diagram.h"

/**
 * @brief SimpleElement::SimpleElement
 * @param location
 * @param qgi
 * @param s
 * @param state
 */
SimpleElement::SimpleElement(const ElementsLocation &location, QGraphicsItem *qgi, int *state) :
	CustomElement(location, qgi, state),
	m_comment_item (nullptr),
	m_location_item (nullptr)
{
	link_type_ = Simple;
	connect(this, SIGNAL(elementInfoChange(DiagramContext, DiagramContext)), this, SLOT(updateLabel(DiagramContext, DiagramContext)));
	connect(this, &Element::updateLabel, [this]() {this->updateLabel(this->elementInformations(), this->elementInformations());});
}

/**
 * @brief SimpleElement::~SimpleElement
 */
SimpleElement::~SimpleElement() {
	if (m_comment_item) delete m_comment_item;
}

/**
 * @brief SimpleElement::initLink
 * @param project
 * Call init Link from custom element and after
 * call update label for setup it.
 */
void SimpleElement::initLink(QETProject *project) {
	CustomElement::initLink(project);
	updateLabel(DiagramContext(), elementInformations());
}

/**
 * @brief SimpleElement::updateLabel
 * update label of this element
 */
void SimpleElement::updateLabel(DiagramContext old_info, DiagramContext new_info)
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

		//Comment and Location of element
	QString comment   = new_info["comment"].toString();
	bool    must_show = new_info.keyMustShow("comment");
	QString location  = new_info["location"].toString();
	bool must_show_location = new_info.keyMustShow("location");

	if ((!(comment.isEmpty() || !must_show) && !m_comment_item)||(!(location.isEmpty() || !must_show_location) && !m_comment_item)) {
		m_comment_item = new CommentItem(this);
	}
	else if (((comment.isEmpty() || !must_show) && m_comment_item) && ((location.isEmpty() || !must_show_location) && m_comment_item)) {
		delete m_comment_item;
		m_comment_item = nullptr;
	}
}
