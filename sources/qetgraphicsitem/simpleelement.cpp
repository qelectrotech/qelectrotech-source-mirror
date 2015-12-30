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
#include "simpleelement.h"
#include "commentitem.h"
#include "elementtextitem.h"
/**
 * @brief SimpleElement::SimpleElement
 * @param location
 * @param qgi
 * @param s
 * @param state
 */
SimpleElement::SimpleElement(const ElementsLocation &location, QGraphicsItem *qgi, int *state) :
	CustomElement(location, qgi, state),
	m_comment_item (nullptr)
{
	link_type_ = Simple;
	connect(this, SIGNAL(elementInfoChange(DiagramContext, DiagramContext)), this, SLOT(updateLabel(DiagramContext, DiagramContext)));
}

SimpleElement::SimpleElement(ElementLocation &location, QGraphicsItem *parent, int *state) :
	CustomElement(location, parent, state),
	m_comment_item (nullptr)
{
	link_type_ = Simple;
	connect(this, SIGNAL(elementInfoChange(DiagramContext, DiagramContext)), this, SLOT(updateLabel(DiagramContext, DiagramContext)));
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
void SimpleElement::updateLabel(DiagramContext old_info, DiagramContext new_info) {
	//Label of element
	if (old_info["label"].toString() != new_info["label"].toString()) {
		if (new_info["label"].toString().isEmpty())
			setTaggedText("label", "_", false);
		else
			setTaggedText("label", new_info["label"].toString(), true);
	}

	if (ElementTextItem *eti = taggedText("label")) {
		new_info["label"].toString().isEmpty() ? eti->setVisible(true) : eti -> setVisible(new_info.keyMustShow("label"));
	}

	//Comment of element
	QString comment   = new_info["comment"].toString();
	bool    must_show = new_info.keyMustShow("comment");

	if (!(comment.isEmpty() || !must_show) && !m_comment_item) {
		m_comment_item = new CommentItem(this);
	}
	else if ((comment.isEmpty() || !must_show) && m_comment_item) {
		delete m_comment_item;
		m_comment_item = nullptr;
	}
}
