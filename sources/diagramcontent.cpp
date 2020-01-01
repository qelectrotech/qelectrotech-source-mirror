/*
	Copyright 2006-2019 The QElectroTech Team
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
#include "diagramcontent.h"
#include <QGraphicsItem>
#include "element.h"
#include "independenttextitem.h"
#include "conductor.h"
#include "diagramimageitem.h"
#include "qetshapeitem.h"
#include "dynamicelementtextitem.h"
#include "elementtextitemgroup.h"
#include "diagram.h"
#include "terminal.h"
#include "conductortextitem.h"

/**
 * @brief DiagramContent::DiagramContent
 */
DiagramContent::DiagramContent() {}

/**
 * @brief DiagramContent::DiagramContent
 * Constructor
 * @param diagram : Construct a diagramContent and fill it with the selected item of @diagram
 * @param selected : this diagramcontent get only selected items if true.
 */
DiagramContent::DiagramContent(Diagram *diagram, bool selected) :
	m_selected_items(diagram->selectedItems())
{
	QList <QGraphicsItem *> item_list;
	if (selected) {
		item_list = m_selected_items;
	} else {
		item_list = diagram->items();
	}

	
	for (QGraphicsItem *item : item_list)
	{
		if (Element *elmt = qgraphicsitem_cast<Element *>(item))
			m_elements << elmt;
		else if (IndependentTextItem *iti = qgraphicsitem_cast<IndependentTextItem *>(item))
			m_text_fields << iti;
		else if (Conductor *c = qgraphicsitem_cast<Conductor *>(item))
		{
				//Get the isolated selected conductor (= not movable, but deletable)
			if (!c->terminal1->parentItem()->isSelected() &&\
				!c->terminal2->parentItem()->isSelected()) {
				m_other_conductors << c;
			}
			
			if (m_potential_conductors.isEmpty()) {
				m_potential_conductors << c;
			}
			else
			{
				if (!potentialIsManaged(c->relatedPotentialConductors(true).toList()))
					m_potential_conductors << c;
			}
		}
		else if (DiagramImageItem *dii = qgraphicsitem_cast<DiagramImageItem *>(item))
			m_images << dii;
		else if (QetShapeItem *dsi = qgraphicsitem_cast<QetShapeItem *>(item))
			m_shapes << dsi;
		else if (DynamicElementTextItem *deti = qgraphicsitem_cast<DynamicElementTextItem *>(item))
			m_element_texts << deti;
		else if (QGraphicsItemGroup *group = qgraphicsitem_cast<QGraphicsItemGroup *>(item))
			if(ElementTextItemGroup *etig = dynamic_cast<ElementTextItemGroup *>(group))
				m_texts_groups << etig;
	}
		
	
		//For each selected element, we determine if conductors must be moved or updated.
	for(Element *elmt : m_elements)
	{
        if (elmt->isSelected())
        {
            for(Terminal *terminal : elmt->terminals())
            {
                for(Conductor *conductor : terminal->conductors())
                {
                    Terminal *other_terminal;
                    if (conductor->terminal1 == terminal)
                        other_terminal = conductor->terminal2;
                    else
                        other_terminal = conductor->terminal1;

                    //If the two elements of conductor are movable
                    if (m_elements.contains(other_terminal -> parentElement()) && !m_conductors_to_move.contains(conductor))
                        m_conductors_to_move << conductor;
                    else if (!m_conductors_to_update.contains(conductor))
                        m_conductors_to_update << conductor;
                }
            }
        }
	}
}

/**
 * @brief DiagramContent::DiagramContent
 * Copy constructor
 * @param other
 */
DiagramContent::DiagramContent(const DiagramContent &other) :
	m_elements(other.m_elements),
	m_text_fields(other.m_text_fields),
	m_images(other.m_images),
	m_shapes(other.m_shapes),
	m_conductors_to_update(other.m_conductors_to_update),
	m_conductors_to_move(other.m_conductors_to_move),
	m_other_conductors(other.m_other_conductors),
	m_potential_conductors(other.m_potential_conductors),
	m_element_texts(other.m_element_texts),
	m_texts_groups(other.m_texts_groups),
	m_selected_items(other.m_selected_items)
{}

DiagramContent::~DiagramContent()
{}

/**
 * @brief DiagramContent::selectedTexts
 * @return a list of every selected texts (every kind of texts)
 * Note that the returned list of texts, correspond to the selected texts
 * at the moment of the creation of this DiagramContent,
 * with the constructor :  DiagramContent::DiagramContent(Diagram *diagram)
 */
QList<DiagramTextItem *> DiagramContent::selectedTexts() const
{
	QList<DiagramTextItem *> selected_texts;
	for(QGraphicsItem *qgi : m_selected_items)
	{
		if (qgi->type() == ConductorTextItem::Type ||
			qgi->type() == IndependentTextItem::Type ||
			qgi->type() == DynamicElementTextItem::Type)
				selected_texts << static_cast<DiagramTextItem *>(qgi);
	}
	return(selected_texts);
}

/**
 * @brief DiagramContent::selectedTextsGroup
 * @return a list of selected texts group
 * Note that the returned list of texts group, correspond to the selected texts group
 * at the moment of the creation of this DiagramContent,
 * with the constructor :  DiagramContent::DiagramContent(Diagram *diagram)
 */
QList<ElementTextItemGroup *> DiagramContent::selectedTextsGroup() const
{
	QList<ElementTextItemGroup *> groups;
	
	for(QGraphicsItem *qgi : m_selected_items)
		if(qgi->type() == QGraphicsItemGroup::Type)
			if(ElementTextItemGroup *grp = dynamic_cast<ElementTextItemGroup *>(qgi))
				groups << grp;
	
	return groups;
}

/**
 * @brief DiagramContent::conductors
 * @param filter
 * @return Every conductors according to the filter
 */
QList<Conductor *> DiagramContent::conductors(int filter) const
{
	QList<Conductor *> result;
	if (filter & ConductorsToMove)   result += m_conductors_to_move;
	if (filter & ConductorsToUpdate) result += m_conductors_to_update;
	if (filter & OtherConductors)    result += m_other_conductors;
	if (filter & SelectedOnly) {
		for(Conductor *conductor : result)
		{
			if (!conductor->isSelected())
				result.removeAll(conductor);
		}
	}
	return(result);
}

/**
 * @brief DiagramContent::hasDeletableItems
 * @return true if this diagram content have deletable item
 * The deletable items correspond to the selected items of diagram
 * at the moment of the creation of this DiagramContent,
 * with the constructor :  DiagramContent::DiagramContent(Diagram *diagram)
 */
bool DiagramContent::hasDeletableItems() const
{
	for(QGraphicsItem *qgi : m_selected_items)
	{
		if (qgi->type() == Element::Type ||
			qgi->type() == Conductor::Type ||
			qgi->type() == IndependentTextItem::Type ||
			qgi->type() == QetShapeItem::Type ||
			qgi->type() == DiagramImageItem::Type ||
			qgi->type() == DynamicElementTextItem::Type)
			return true;
		if(qgi->type() == QGraphicsItemGroup::Type)
			if(dynamic_cast<ElementTextItemGroup *>(qgi))
				return true;
	}
	return(false);
}

/**
 * @brief DiagramContent::hasCopiableItems
 * @return true if this diagram content have copiable items.
 */
bool DiagramContent::hasCopiableItems() const
{
	if(!m_images.isEmpty())      return true;
	if(!m_shapes.isEmpty())      return true;
	if(!m_elements.isEmpty())    return true;
	if(!m_text_fields.isEmpty()) return true;
	
	return false;
}

/**
 * @brief DiagramContent::clear
 * Remove all items from the diagram content
 */
void DiagramContent::clear()
{
	m_elements.clear();
	m_text_fields.clear();
	m_images.clear();
	m_shapes.clear();
	m_conductors_to_update.clear();
	m_conductors_to_move.clear();
	m_other_conductors.clear();
	m_element_texts.clear();
	m_texts_groups.clear();
	m_selected_items.clear();
}

/**
 * @brief DiagramContent::removeNonMovableItems
 * Remove all non movable item.
 * @return : return the numbers of removed item
 */
int DiagramContent::removeNonMovableItems()
{
	int count_ = 0;
	
	const QList<Element *> elements_set = m_elements;
	for(Element *elmt : elements_set) {
		if (!elmt->isMovable()) {
			m_elements.removeAll(elmt);
			++count_;
		}
	}
	
	const QSet<DiagramImageItem *> images_set = m_images;
	for(DiagramImageItem *img : images_set) {
		if (!img->isMovable()) {
			m_images.remove(img);
			++count_;
		}
	}
	
	const QSet<QetShapeItem *> shapes_set = m_shapes;
	for(QetShapeItem *shape : shapes_set) {
		if (!shape->isMovable()) {
			m_shapes.remove(shape);
			++count_;
		}
	}

	return count_;
}

DiagramContent &DiagramContent::operator+=(const DiagramContent &other)
{
	for(Element *elmt : other.m_elements)
		if(!m_elements.contains(elmt))
			m_elements << elmt;
	
	for(IndependentTextItem *iti : other.m_text_fields)
		if(!m_text_fields.contains(iti))
			m_text_fields << iti;
	
	for(DiagramImageItem *dii : other.m_images)
		if(!m_images.contains(dii))
			m_images << dii;
	
	for(QetShapeItem *qsi : other.m_shapes)
		if(!m_shapes.contains(qsi))
			m_shapes << qsi;
	
	for(Conductor *c : other.m_conductors_to_update)
		if(!m_conductors_to_update.contains(c))
			m_conductors_to_update << c;
	
	for(Conductor *c : other.m_conductors_to_move)
		if(!m_conductors_to_move.contains(c))
			m_conductors_to_move << c;
	
	for(Conductor *c : other.m_other_conductors)
		if(!m_other_conductors.contains(c))
			m_other_conductors << c;
	
	for(DynamicElementTextItem *deti : other.m_element_texts)
		if(!m_element_texts.contains(deti))
			m_element_texts << deti;
	
	for(ElementTextItemGroup *etig : other.m_texts_groups)
		if(!m_texts_groups.contains(etig))
			m_texts_groups << etig;
	
	for(QGraphicsItem *qgi : other.m_selected_items)
		if(!m_selected_items.contains(qgi))
			m_selected_items << qgi;
	
	for (Conductor *c : other.m_potential_conductors)
	{
		QList<Conductor *> c_list = c->relatedPotentialConductors(true).toList();
		c_list << c;
		if (!potentialIsManaged(c_list)) {
			m_potential_conductors << c;
		}
	}
	
	return *this;
}

/**
 * @brief DiagramContent::potentialIsManaged
 * @param conductors a list of conductors at the same potential.
 * @return true, if m_potential_conductors already have a conductor of this potential.
 */
bool DiagramContent::potentialIsManaged(QList<Conductor *> conductors)
{
	bool b = false;
	
	for (Conductor *c : conductors)
	{
		if (m_potential_conductors.contains(c))
			b = true;
	}
	
	return b;
}

/**
 * @brief DiagramContent::hasTextEditing
 * @return true if handle a text currently in editing intercation
 */
bool DiagramContent::hasTextEditing()
{
	for (DiagramTextItem *dti : selectedTexts()) {
		if (dti->textInteractionFlags() == Qt::TextEditorInteraction) {
			return true;
		}
	}

	return false;
}

/**
 * @brief DiagramContent::items
 * @param filter
 * @return The items of this diagram content according to @filter
 */
QList<QGraphicsItem *> DiagramContent::items(int filter) const
{
	QList<QGraphicsItem *> items_list;
	
	for(QGraphicsItem *qgi : conductors(filter)) items_list << qgi;

	if (filter & Elements)          for(QGraphicsItem *qgi : m_elements)      items_list << qgi;
	if (filter & TextFields)        for(QGraphicsItem *qgi : m_text_fields)   items_list << qgi;
	if (filter & Images)            for(QGraphicsItem *qgi : m_images)        items_list << qgi;
	if (filter & Shapes)            for(QGraphicsItem *qgi : m_shapes)        items_list << qgi;
	if (filter & ElementTextFields) for(QGraphicsItem *qgi : m_element_texts) items_list << qgi;
	if (filter & TextGroup)			for(QGraphicsItem *qgi : m_texts_groups)   items_list << qgi;

	if (filter & SelectedOnly) {
		for(QGraphicsItem *qgi : items_list) {
			if (!qgi -> isSelected()) items_list.removeOne(qgi);
		}
	}
	return(items_list);
}

/**
 * @brief DiagramContent::count
 * @param filter
 * @return The number of items, according to @filter
 */
int DiagramContent::count(int filter) const
{
	int count = 0;
	if (filter & SelectedOnly) {
		if (filter & Elements)           for(Element *element :     m_elements)               { if (element   -> isSelected()) ++ count; }
		if (filter & TextFields)         for(DiagramTextItem *dti : m_text_fields)            { if (dti       -> isSelected()) ++ count; }
		if (filter & Images)             for(DiagramImageItem *dii : m_images)                { if (dii       -> isSelected()) ++ count; }
		if (filter & Shapes)             for(QetShapeItem *dsi : m_shapes)                    { if (dsi       -> isSelected()) ++ count; }
		if (filter & ConductorsToMove)   for(Conductor *conductor : m_conductors_to_move)     { if (conductor -> isSelected()) ++ count; }
		if (filter & ConductorsToUpdate) for(Conductor *conductor : m_conductors_to_update)   { if (conductor -> isSelected()) ++ count; }
		if (filter & OtherConductors)    for(Conductor *conductor : m_other_conductors)       { if (conductor -> isSelected()) ++ count; }
		if (filter & ElementTextFields)  for(DynamicElementTextItem *deti : m_element_texts)  { if (deti      -> isSelected()) ++ count; }
		if (filter & TextGroup)          for(ElementTextItemGroup *etig : m_texts_groups)      { if (etig      -> isSelected()) ++ count; }
	}
	else {
		if (filter & Elements)           count += m_elements.count();
		if (filter & TextFields)         count += m_text_fields.count();
		if (filter & Images)             count += m_images.count();
		if (filter & Shapes)             count += m_shapes.count();
		if (filter & ConductorsToMove)   count += m_conductors_to_move.count();
		if (filter & ConductorsToUpdate) count += m_conductors_to_update.count();
		if (filter & OtherConductors)    count += m_other_conductors.count();
		if (filter & ElementTextFields)  count += m_element_texts.count();
		if (filter & TextGroup)			 count += m_texts_groups.count();
	}
	return(count);
}

/**
 * @brief DiagramContent::sentence
 * @param filter
 * @return A string that describe the items of the diagram content according to @filter.
 * Exemple : X elements, Y conductors etc....
 */
QString DiagramContent::sentence(int filter) const
{
	int elements_count   = (filter & Elements) ? m_elements.count() : 0;
	int conductors_count = conductors(filter).count();
	int textfields_count = (filter & TextFields) ? (m_text_fields.count()) : 0;
	int images_count	 = (filter & Images) ? m_images.count() : 0;
	int shapes_count	 = (filter & Shapes) ? m_shapes.count() : 0;
	int elmt_text_count  = (filter & ElementTextFields) ? m_element_texts.count() : 0;
	
	return(
		QET::ElementsAndConductorsSentence(
			elements_count,
			conductors_count,
			textfields_count,
			images_count,
			shapes_count,
			elmt_text_count		
		)
	);
}

/**
 * @brief operator << Use to debug a diagram content
 * @param d : QDebug to use for display the debug info
 * @param content : content to debug
 * @return 
 */
QDebug &operator<<(QDebug d, DiagramContent &content) {
	Q_UNUSED(content);
	d << "DiagramContent {" << "\n";
	/*
	FIXME Le double-heritage QObject / QGraphicsItem a casse cet operateur
	d << "  elements :" << c.elements << "\n";
	d << "  conductorsToUpdate :" << c.conductorsToUpdate << "\n";
	d << "  conductorsToMove :" << c.conductorsToMove << "\n";
	d << "  otherConductors :" << c.otherConductors << "\n";
	*/
	d << "}";
	return(d.space());
}
