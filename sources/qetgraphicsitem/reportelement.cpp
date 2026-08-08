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
#include "reportelement.h"

#include "../diagram.h"
#include "../diagramposition.h"
#include "../qetgraphicsitem/conductor.h"
#include "../qetgraphicsitem/terminal.h"
#include "../qetproject.h"
#include "dynamicelementtextitem.h"

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QMessageBox>

ReportElement::ReportElement(const ElementsLocation &location, const QString& link_type,QGraphicsItem *qgi, int *state) :
	Element(location, qgi, state,
			link_type == "next_report"? Element::NextReport : Element::PreviousReport),
	m_inverse_report(link_type == "next_report"? Element::PreviousReport : Element::NextReport)
{}

/**
	@brief ReportElement::~ReportElement
	Destructor
*/
ReportElement::~ReportElement()
{
	unlinkAllElements();
	if (terminals().size())
		disconnect(terminals().first(), nullptr, nullptr, nullptr);
}

/**
	@brief ReportElement::linkToElement
	Link this element to the other element
	@param elmt
	element to be linked with this
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
		elmt->linkToElement(this);
		emit linkedElementChanged();
	}
}

/**
	@brief ReportElement::unLinkAllElements
	Unlink all of the element in the QList connected_elements
*/
void ReportElement::unlinkAllElements()
{
	if (isFree())
		return;

	const QList <Element *> tmp_elmt = connected_elements;

	for (Element *elmt : tmp_elmt)
		connected_elements.removeAll(elmt);

	for(Element *elmt : tmp_elmt)
	{
		elmt -> setHighlighted(false);
		elmt -> unlinkAllElements();
	}
	
	emit linkedElementChanged();
}
/**
	@brief ReportElement::unlinkElement
 *unlink the specified element.
 *for reportelement, they must be only one linked element, so we call
 *unlinkAllElements for clear the connected_elements list.
	@param elmt
*/
void ReportElement::unlinkElement(Element *elmt) {
	Q_UNUSED (elmt);
	unlinkAllElements();
}

/**
	@brief ReportElement::paint
	Draw the same blue halo already used elsewhere in QET to mean "linked"
	(Element::drawHighlight(), normally a transient hover effect) whenever
	this arrow actually resolves to a matching element on another folio --
	a persistent, hyperlink-like "this link is live" indicator instead of
	only showing on hover.
	@param painter
	@param options
	@param widget
*/
void ReportElement::paint(
		QPainter *painter,
		const QStyleOptionGraphicsItem *options,
		QWidget *widget)
{
	if (!isFree())
		drawHighlight(painter, options);
	Element::paint(painter, options, widget);
}

/**
	@brief ReportElement::mouseDoubleClickEvent
	When this report arrow is linked, jump to the folio holding its
	counterpart and select it there, so double-clicking an arrow follows it
	like a hyperlink instead of requiring the user to hunt for the matching
	folio by hand.

	When it isn't linked yet, fall back to the inherited
	QetGraphicsItem::mouseDoubleClickEvent() behaviour (open the properties
	dialog) exactly as before -- that dialog's "Folio referencing" tab is
	the only way to create the link in the first place, so double-click
	must keep reaching it until a link actually exists to follow.

	A link that resolves to a connected_elements entry but whose target no
	longer has a diagram (its folio, or the element itself, was deleted
	after linking) is reported with a small popup instead of silently
	doing nothing.
	@param event
*/
void ReportElement::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
	if (isFree() || connected_elements.isEmpty())
	{
		QetGraphicsItem::mouseDoubleClickEvent(event);
		return;
	}

	event->accept();

	Element *linked_element = connected_elements.first();
	Diagram *linked_diagram = linked_element->diagram();
	if (!linked_diagram)
	{
		QWidget *parent_widget = (diagram() && diagram()->views().size())
				? diagram()->views().at(0)
				: nullptr;
		QMessageBox::information(
					parent_widget,
					tr("Lien de renvoi de folio"),
					tr("L'élément relié à ce renvoi n'existe plus."));
		return;
	}

	linked_diagram->showMe();
	if (linked_diagram->views().size())
	{
		QGraphicsView *view = linked_diagram->views().at(0);
		linked_diagram->clearSelection();
		linked_element->setSelected(true);
		view->centerOn(linked_element);
	}
}
